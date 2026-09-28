#include "state/MapCacheManager.h"
#include "state/WaypointManager.h"
#include "mod/ChiyanMap.h"
#include <fstream>
#include <filesystem>
#include <cstring>
#include <thread>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <Windows.h>
#include <wincodec.h>
#include <shellapi.h>

namespace MapCacheManager {
    std::unordered_map<uint64_t, RegionData*> g_loadedRegions;
    std::vector<uint64_t> g_loadQueue;
    std::mutex g_cacheMutex;
    std::atomic<bool> g_running{false};
    // 协程退出同步信号：Init 时为 0（被占用），协程结束时 release；Shutdown 时 acquire 等待退出
    std::binary_semaphore g_ioDone{0};
    
    // 初始化为空，等待进入世界时分配
    std::string g_cacheDir = "";
    static int s_currentLoadedDimId = -999;

    int GetLoadedDimensionId() {
        return s_currentLoadedDimId;
    }

    void SwitchViewDimension(int dimensionId) {
        if (MapRenderState::currentWorldId.empty()) return;
        SwitchWorld(MapRenderState::currentWorldId, dimensionId);
    }

    // ==========================================
    // [生物群系段读写辅助] 文件格式：colors + heights + biomeCount(1) + biomeTable + biomeCells(4096)
    // 旧文件无此段（fileSize <= colors+heights），新文件包含
    // ==========================================
    static void WriteBiomeSection(std::ofstream& out, const RegionData& region) {
        uint8_t biomeCount = (uint8_t)std::min<size_t>(region.biomeTable.size(), 255);
        out.write((char*)&biomeCount, 1);
        for (uint8_t i = 0; i < biomeCount; i++) {
            const std::string& name = region.biomeTable[i];
            uint8_t len = (uint8_t)std::min<size_t>(name.size(), 255);
            out.write((char*)&len, 1);
            if (len > 0) out.write(name.data(), len);
        }
        out.write((char*)region.biomeCells, sizeof(region.biomeCells));
    }

    static void ReadBiomeSection(std::ifstream& in, RegionData& region, std::streamoff fileSize, std::streamoff sectionStart) {
        if (fileSize <= sectionStart) return;  // 旧文件，无生物群系段
        uint8_t biomeCount = 0;
        in.read((char*)&biomeCount, 1);
        region.biomeTable.clear();
        region.biomeTable.reserve(biomeCount);
        for (uint8_t i = 0; i < biomeCount; i++) {
            uint8_t len = 0;
            in.read((char*)&len, 1);
            std::string name(len, '\0');
            if (len > 0) in.read(&name[0], len);
            region.biomeTable.push_back(std::move(name));
        }
        // 仅当剩余字节足够时读取 biomeCells（防止截断文件）
        std::streamoff curPos = in.tellg();
        if (curPos >= 0 && fileSize - curPos >= (std::streamoff)sizeof(region.biomeCells)) {
            in.read((char*)region.biomeCells, sizeof(region.biomeCells));
        }
    }

    void IOWorkerThread() {
        auto lastSaveTime = std::chrono::steady_clock::now();

        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));

            try {

            std::vector<uint64_t> toLoad;
            std::string currentDir;
            {
                std::lock_guard<std::mutex> lock(g_cacheMutex);
                if (g_cacheDir.empty()) continue; // 没有世界路径时挂起
                currentDir = g_cacheDir;
                if (!g_loadQueue.empty()) {
                    toLoad = g_loadQueue;
                    g_loadQueue.clear();
                }
            }

            // 通道 1：极速读取（地表直接存放在 dim_<n>/ 下，洞穴在 cave/ 子目录）
            // [修复] 使用锁内拷贝的 currentDir 而非无锁读取 g_cacheDir，
            // 避免 SwitchWorld 并发赋值 g_cacheDir 时的 std::string use-after-free
            for (uint64_t hash : toLoad) {
                int rx, rz; DecodeRegionHash(hash, rx, rz);
                std::string dir = currentDir + GetRegionSubdir(IsCaveHash(hash));
                std::string filePath = dir + "region_" + std::to_string(rx) + "_" + std::to_string(rz) + ".bin";

                RegionData* newRegion = new RegionData();
                std::ifstream in(filePath, std::ios::binary | std::ios::ate);
                if (in) {
                    auto fileSize = in.tellg();
                    in.seekg(0, std::ios::beg);
                    in.read((char*)newRegion->colors, sizeof(newRegion->colors));
                    // 新格式：colors + heights；旧格式仅 colors（heights 保持 HEIGHT_UNKNOWN）
                    if (fileSize >= (std::streamoff)(sizeof(newRegion->colors) + sizeof(newRegion->heights))) {
                        in.read((char*)newRegion->heights, sizeof(newRegion->heights));
                        // 清理旧版本全 0 污染：未探索像素 (a < 10) 强制置为 HEIGHT_UNKNOWN
                        for (int i = 0; i < REGION_SIZE * REGION_SIZE; ++i) {
                            if (newRegion->colors[i * 4 + 3] < 10) {
                                newRegion->heights[i] = HEIGHT_UNKNOWN;
                            }
                        }
                    }
                    // [新增] 生物群系段（colors+heights 之后）
                    ReadBiomeSection(in, *newRegion, fileSize,
                                     (std::streamoff)(sizeof(newRegion->colors) + sizeof(newRegion->heights)));
                }
                newRegion->textureDirty = true;

                {
                    std::lock_guard<std::mutex> lock(g_cacheMutex);
                    if (g_loadedRegions[hash] == nullptr) g_loadedRegions[hash] = newRegion;
                    else delete newRegion;
                }
            }

            // 通道 2：低频保存（地表）
            auto now = std::chrono::steady_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(now - lastSaveTime).count() >= 2) {
                lastSaveTime = now;
                std::vector<std::pair<uint64_t, RegionData>> regionsToSave;
                {
                    std::lock_guard<std::mutex> lock(g_cacheMutex);
                    for (auto& pair : g_loadedRegions) {
                        if (pair.second && pair.second->dirty) {
                            regionsToSave.push_back({pair.first, *pair.second});
                            pair.second->dirty = false;
                        }
                    }
                }
                for (auto& item : regionsToSave) {
                    int rx, rz; DecodeRegionHash(item.first, rx, rz);
                    std::string dir = currentDir + GetRegionSubdir(IsCaveHash(item.first));
                    std::string filePath = dir + "region_" + std::to_string(rx) + "_" + std::to_string(rz) + ".bin";
                    std::ofstream out(filePath, std::ios::binary);
                    if (out) {
                        out.write((char*)item.second.colors, sizeof(item.second.colors));
                        out.write((char*)item.second.heights, sizeof(item.second.heights));
                        // [新增] 生物群系段
                        WriteBiomeSection(out, item.second);
                    }
                }
            }

            } catch (...) {
                // [修复] 捕获所有异常防止 std::terminate → 0xC0000409 FAST_FAIL_FATAL_APP_EXIT
                // 项目记忆：MapCacheManager.cpp 的 IOWorkerCoro/内存分配 必须包裹 try-catch
            }
        }
        g_ioDone.release();
    }

    // ==========================================
    // [缓存净化自愈引擎]
    // 自动扫描下界/末地缓存目录，若检测到含有不属于该维度的生物群系(如 birch_forest 等)
    // 或在下界存在纯蓝水体像素，立即彻底删除被污染的对应 region 文件，使其能纯净重新探索
    // ==========================================
    static void CleanupCorruptedDimensionCache(const std::filesystem::path& cacheDir, int dimensionId) {
        if (dimensionId != 1 && dimensionId != 2) return;
        std::error_code ec;

        auto checkAndPurgeFile = [&](const std::filesystem::path& filePath, bool isCaveSubdir) {
            if (!std::filesystem::exists(filePath, ec)) return false;
            std::ifstream in(filePath, std::ios::binary | std::ios::ate);
            if (!in) return false;
            auto fileSize = in.tellg();
            in.seekg(0, std::ios::beg);

            constexpr size_t colorSize = REGION_SIZE * REGION_SIZE * 4;
            constexpr size_t heightSize = REGION_SIZE * REGION_SIZE * sizeof(int16_t);
            constexpr size_t headerSize = colorSize + heightSize;
            if (fileSize < (std::streamoff)colorSize) return false;

            bool isCorrupted = false;

            // 1. 下界水体检测: 下界无自然水体，若洞穴图存在大面积纯蓝水体像素则判定为主世界洞穴污染
            if (dimensionId == 1 && isCaveSubdir) {
                std::vector<uint8_t> colors(colorSize);
                in.read((char*)colors.data(), colorSize);
                int waterPixels = 0;
                for (size_t i = 0; i < colorSize; i += 4) {
                    uint8_t r = colors[i + 0];
                    uint8_t g = colors[i + 1];
                    uint8_t b = colors[i + 2];
                    uint8_t a = colors[i + 3];
                    if (a > 200 && b > 180 && b > r + 80 && b > g + 50) {
                        waterPixels++;
                        if (waterPixels > 30) {
                            isCorrupted = true;
                            break;
                        }
                    }
                }
            }

            // 2. 生物群系检测: 验证文件中记录的群系是否属于该维度
            if (!isCorrupted && fileSize > (std::streamoff)headerSize) {
                in.seekg(headerSize, std::ios::beg);
                uint8_t biomeCount = 0;
                in.read((char*)&biomeCount, 1);
                if (biomeCount > 0 && biomeCount <= 255) {
                    for (uint8_t i = 0; i < biomeCount; i++) {
                        uint8_t len = 0;
                        in.read((char*)&len, 1);
                        std::string bName(len, '\0');
                        if (len > 0) in.read(&bName[0], len);
                        if (!IsValidBiomeForDimension(dimensionId, bName)) {
                            isCorrupted = true;
                            break;
                        }
                    }
                }
            }
            in.close();

            if (isCorrupted) {
                std::filesystem::remove(filePath, ec);
                std::string fname = filePath.filename().string();
                if (isCaveSubdir) {
                    std::filesystem::remove(cacheDir / fname, ec);
                } else {
                    std::filesystem::remove(cacheDir / "cave" / fname, ec);
                }
                return true;
            }
            return false;
        };

        if (std::filesystem::is_directory(cacheDir, ec)) {
            for (auto const& entry : std::filesystem::directory_iterator(cacheDir, ec)) {
                if (!entry.is_regular_file()) continue;
                std::string fname = entry.path().filename().string();
                if (fname.rfind("region_", 0) == 0 && fname.ends_with(".bin")) {
                    checkAndPurgeFile(entry.path(), false);
                }
            }
        }
        auto caveDir = cacheDir / "cave";
        if (std::filesystem::is_directory(caveDir, ec)) {
            for (auto const& entry : std::filesystem::directory_iterator(caveDir, ec)) {
                if (!entry.is_regular_file()) continue;
                std::string fname = entry.path().filename().string();
                if (fname.rfind("region_", 0) == 0 && fname.ends_with(".bin")) {
                    checkAndPurgeFile(entry.path(), true);
                }
            }
        }
    }

    static int CountRegionBinFiles(const std::filesystem::path& dir) {
        std::error_code ec;
        if (!std::filesystem::is_directory(dir, ec)) return 0;
        int count = 0;
        for (auto const& entry : std::filesystem::recursive_directory_iterator(dir, ec)) {
            if (entry.is_regular_file()) {
                std::string fn = entry.path().filename().string();
                if (fn.rfind("region_", 0) == 0 && fn.ends_with(".bin")) {
                    count++;
                }
            }
        }
        return count;
    }

    void SwitchWorld(const std::string& worldId, int dimensionId) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        s_currentLoadedDimId = dimensionId;
        
        if (!g_cacheDir.empty()) {
            for (auto& pair : g_loadedRegions) {
                if (pair.second && pair.second->dirty) {
                    int rx, rz; DecodeRegionHash(pair.first, rx, rz);
                    std::string dir = g_cacheDir + GetRegionSubdir(IsCaveHash(pair.first));
                    std::string filePath = dir + "region_" + std::to_string(rx) + "_" + std::to_string(rz) + ".bin";
                    std::ofstream out(filePath, std::ios::binary);
                    if (out) {
                        out.write((char*)pair.second->colors, sizeof(pair.second->colors));
                        out.write((char*)pair.second->heights, sizeof(pair.second->heights));
                        // [新增] 生物群系段
                        WriteBiomeSection(out, *pair.second);
                    }
                }
                if (pair.second) delete pair.second;
            }
        }
        
        g_loadedRegions.clear();
        g_loadQueue.clear();

        // 路径示例：mods/ChiyanMap/data/cache/<worldId>/dim_<n>/
        // 地表 (region_*.bin) 直接存放于 dim_<n>/ 下；洞穴存放于 dim_<n>/cave/ 下
        auto baseDataDir = chiyan_map::ChiyanMap::getInstance().getSelf().getDataDir();
        auto cacheRoot = baseDataDir / "cache";
        auto worldDir = cacheRoot / worldId;
        auto cacheDir = worldDir / ("dim_" + std::to_string(dimensionId));

        int exactCount = CountRegionBinFiles(worldDir);

        // 如果当前世界缓存目录为空或只有极少量文件，尝试从历史/跨版本智能检索并恢复已存地图
        if (exactCount < 5) {
            std::string rawLevelId = worldId;
            size_t sPos = rawLevelId.find("_S");
            if (sPos != std::string::npos) rawLevelId = rawLevelId.substr(0, sPos);

            std::filesystem::path bestCandidate;
            int bestCount = exactCount;

            // 1. 本地 cache 目录检索同 levelId 的历史目录 (如旧种子/不同出生点后缀)
            std::error_code ec;
            if (std::filesystem::is_directory(cacheRoot, ec)) {
                for (auto const& entry : std::filesystem::directory_iterator(cacheRoot, ec)) {
                    if (!entry.is_directory()) continue;
                    if (entry.path() == worldDir) continue;
                    std::string fn = entry.path().filename().string();
                    if (fn.rfind(rawLevelId, 0) == 0) {
                        int c = CountRegionBinFiles(entry.path());
                        if (c > bestCount) {
                            bestCount = c;
                            bestCandidate = entry.path();
                        }
                    }
                }
            }

            // 2. 跨版本检索 (针对 LeviLauncher 等将各版本置于 versions/<ver>/ 的启动器)
            if (bestCount < 5) {
                try {
                    // baseDataDir 结构一般为: <root>/versions/<currentVer>/mods/ChiyanMap/data
                    auto curVerDir = baseDataDir.parent_path().parent_path().parent_path();
                    auto versionsDir = curVerDir.parent_path();
                    if (std::filesystem::is_directory(versionsDir, ec)) {
                        for (auto const& verEntry : std::filesystem::directory_iterator(versionsDir, ec)) {
                            if (!verEntry.is_directory()) continue;
                            if (verEntry.path() == curVerDir) continue;
                            auto otherCacheRoot = verEntry.path() / "mods" / "ChiyanMap" / "data" / "cache";
                            if (!std::filesystem::is_directory(otherCacheRoot, ec)) continue;

                            for (auto const& entry : std::filesystem::directory_iterator(otherCacheRoot, ec)) {
                                if (!entry.is_directory()) continue;
                                std::string fn = entry.path().filename().string();
                                if (fn == worldId || fn.rfind(rawLevelId, 0) == 0) {
                                    int c = CountRegionBinFiles(entry.path());
                                    if (c > bestCount) {
                                        bestCount = c;
                                        bestCandidate = entry.path();
                                    }
                                }
                            }
                        }
                    }
                } catch(...) {}
            }

            // 3. 若发现包含更多历史缓存的有效候选目录，自动将其补齐迁移至当前世界目录
            if (!bestCandidate.empty() && bestCount > exactCount) {
                std::filesystem::create_directories(worldDir, ec);
                std::filesystem::copy(bestCandidate, worldDir, 
                                      std::filesystem::copy_options::recursive | std::filesystem::copy_options::skip_existing, ec);
                // 同时也检查并迁移该历史版本的 waypoints
                try {
                    auto oldWpDir = bestCandidate.parent_path().parent_path() / "waypoints";
                    auto curWpDir = std::filesystem::path("mods/ChiyanMap/waypoints");
                    if (std::filesystem::is_directory(oldWpDir, ec)) {
                        std::filesystem::create_directories(curWpDir, ec);
                        for (auto const& wpEntry : std::filesystem::directory_iterator(oldWpDir, ec)) {
                            if (!wpEntry.is_regular_file()) continue;
                            std::string wfn = wpEntry.path().filename().string();
                            if (wfn.rfind(rawLevelId, 0) == 0) {
                                std::filesystem::copy_file(wpEntry.path(), curWpDir / wfn, 
                                                          std::filesystem::copy_options::skip_existing, ec);
                            }
                        }
                    }
                } catch(...) {}
            }
        }

        std::filesystem::create_directories(cacheDir);
        std::filesystem::create_directories(cacheDir / "cave");

        // [向后兼容] 迁移旧版主世界地表缓存：
        //   旧布局: dim_<n>/surface/region_*.bin
        //   新布局: dim_<n>/region_*.bin
        // 将 surface/ 目录下的 region_*.bin 移动到 dim_<n>/ 下，避免老玩家的地表地图失效。
        std::error_code ec;
        auto oldSurfaceDir = cacheDir / "surface";
        if (std::filesystem::is_directory(oldSurfaceDir, ec)) {
            for (auto const& entry : std::filesystem::directory_iterator(oldSurfaceDir, ec)) {
                if (!entry.is_regular_file()) continue;
                std::string filename = entry.path().filename().string();
                if (filename.rfind("region_", 0) != 0) continue;
                auto target = cacheDir / filename;
                // 若新位置已存在同名文件则保留新文件，仅在不存在时移动
                if (!std::filesystem::exists(target)) {
                    std::filesystem::rename(entry.path(), target, ec);
                    if (ec) {
                        // rename 失败（跨分区等）则回退到 copy + remove
                        std::filesystem::copy_file(entry.path(), target, std::filesystem::copy_options::overwrite_existing, ec);
                        if (!ec) std::filesystem::remove(entry.path(), ec);
                    }
                }
            }
            // 尝试清理空的 surface 目录
            std::filesystem::remove(oldSurfaceDir, ec);
        }

        // [自愈引擎] 自动检测并净化下界/末地中被主世界数据污染的历史缓存文件 (如 birch_forest_mutated 等)
        CleanupCorruptedDimensionCache(cacheDir, dimensionId);

        // 末尾保留分隔符以兼容现有 currentDir + "region_..." 字符串拼接
        g_cacheDir = cacheDir.string() + "/";

        // [预加载] 扫描磁盘上的 region 文件并排队异步加载, 使全屏大地图能立即显示已有数据
        // 而不是从黑屏开始逐步加载。扫描地表(dim_<n>/)和洞穴(dim_<n>/cave/)两个目录。
        auto preloadDir = [&](const std::string& subdir, bool isCave) {
            auto fullDir = cacheDir / subdir;
            std::error_code ec2;
            if (!std::filesystem::is_directory(fullDir, ec2)) return;
            for (auto const& entry : std::filesystem::directory_iterator(fullDir, ec2)) {
                if (!entry.is_regular_file()) continue;
                std::string filename = entry.path().filename().string();
                if (filename.rfind("region_", 0) != 0) continue;
                // 解析文件名 region_RX_RZ.bin → rx, rz (手动解析避免 sscanf 警告)
                int rx = 0, rz = 0;
                if (filename.size() > 8 && filename.compare(0, 7, "region_") == 0) {
                    size_t pos = 7;
                    bool negX = false;
                    if (pos < filename.size() && filename[pos] == '-') { negX = true; pos++; }
                    while (pos < filename.size() && filename[pos] >= '0' && filename[pos] <= '9') {
                        rx = rx * 10 + (filename[pos] - '0'); pos++;
                    }
                    if (negX) rx = -rx;
                    if (pos < filename.size() && filename[pos] == '_') {
                        pos++;
                        bool negZ = false;
                        if (pos < filename.size() && filename[pos] == '-') { negZ = true; pos++; }
                        while (pos < filename.size() && filename[pos] >= '0' && filename[pos] <= '9') {
                            rz = rz * 10 + (filename[pos] - '0'); pos++;
                        }
                        if (negZ) rz = -rz;
                    } else { continue; }
                    uint64_t hash = GetRegionHash(rx, rz, isCave);
                    if (g_loadedRegions.find(hash) == g_loadedRegions.end()) {
                        g_loadedRegions[hash] = nullptr;
                        g_loadQueue.push_back(hash);
                    }
                }
            }
        };
        preloadDir("", false);       // 地表数据
        preloadDir("cave", true);   // 洞穴/下界数据
    }

    void Init() {
        g_running = true;
        // 启动 IO 工作线程; 线程退出时释放信号量
        std::thread(IOWorkerThread).detach();
    }

    void Shutdown() {
        g_running = false;
        // 等待 IO 线程退出（最多 20ms 一次循环 + 一次 IO 处理时间）
        g_ioDone.acquire();

        // [持久化] 游戏关闭时保存所有脏区域 (含下界/末地), 避免退出时数据丢失
        if (!g_cacheDir.empty()) {
            for (auto& pair : g_loadedRegions) {
                if (pair.second && pair.second->dirty) {
                    int rx, rz; DecodeRegionHash(pair.first, rx, rz);
                    std::string dir = g_cacheDir + GetRegionSubdir(IsCaveHash(pair.first));
                    std::string filePath = dir + "region_" + std::to_string(rx) + "_" + std::to_string(rz) + ".bin";
                    std::ofstream out(filePath, std::ios::binary);
                    if (out) {
                        out.write((char*)pair.second->colors, sizeof(pair.second->colors));
                        out.write((char*)pair.second->heights, sizeof(pair.second->heights));
                        WriteBiomeSection(out, *pair.second);
                    }
                }
            }
        }

        for (auto& pair : g_loadedRegions) if(pair.second) delete pair.second;
        g_loadedRegions.clear();
        g_loadQueue.clear();
    }

    void UpdateFromScan(int targetDim, int centerX, int centerZ, mce::Color scanColors[MAP_DATA_SIZE][MAP_DATA_SIZE], float scanHeights[MAP_DATA_SIZE][MAP_DATA_SIZE], bool isCave) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        if (g_cacheDir.empty()) return; // 未进世界前禁止写入
        // [严格维度防护] 拒绝写入不属于当前加载维度或玩家物理维度的扫描数据，彻底阻断跨维度污染
        if (targetDim != s_currentLoadedDimId || targetDim != MapRenderState::currentDimensionId) return;
        if (s_currentLoadedDimId != MapRenderState::currentDimensionId) return;
        int startX = centerX - MAP_DATA_RADIUS; int startZ = centerZ - MAP_DATA_RADIUS;

        for (int z = 0; z < MAP_DATA_SIZE; z++) {
            for (int x = 0; x < MAP_DATA_SIZE; x++) {
                mce::Color c = scanColors[x][z];
                if (c.a <= 0.01f) continue;

                int worldX = startX + x; int worldZ = startZ + z;
                int rx = (worldX < 0 ? (worldX + 1) / REGION_SIZE - 1 : worldX / REGION_SIZE);
                int rz = (worldZ < 0 ? (worldZ + 1) / REGION_SIZE - 1 : worldZ / REGION_SIZE);
                uint64_t hash = GetRegionHash(rx, rz, isCave);
                
                if (g_loadedRegions.find(hash) == g_loadedRegions.end() || g_loadedRegions[hash] == nullptr) {
                    RegionData* newRegion = new RegionData();
                    std::string dir = g_cacheDir + GetRegionSubdir(isCave);
                    std::string filePath = dir + "region_" + std::to_string(rx) + "_" + std::to_string(rz) + ".bin";
                    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
                    if (in) {
                        auto fileSize = in.tellg();
                        in.seekg(0, std::ios::beg);
                        in.read((char*)newRegion->colors, sizeof(newRegion->colors));
                        if (fileSize >= (std::streamoff)(sizeof(newRegion->colors) + sizeof(newRegion->heights))) {
                            in.read((char*)newRegion->heights, sizeof(newRegion->heights));
                            // 清理旧版本全 0 污染：未探索像素 (a < 10) 强制置为 HEIGHT_UNKNOWN
                            for (int i = 0; i < REGION_SIZE * REGION_SIZE; ++i) {
                                if (newRegion->colors[i * 4 + 3] < 10) {
                                    newRegion->heights[i] = HEIGHT_UNKNOWN;
                                }
                            }
                        }
                        // [新增] 生物群系段（colors+heights 之后）
                        ReadBiomeSection(in, *newRegion, fileSize,
                                         (std::streamoff)(sizeof(newRegion->colors) + sizeof(newRegion->heights)));
                    }
                    newRegion->textureDirty = true;
                    g_loadedRegions[hash] = newRegion;
                }

                RegionData* region = g_loadedRegions[hash];
                int localX = worldX - (rx * REGION_SIZE); int localZ = worldZ - (rz * REGION_SIZE);

                float currentY = scanHeights[x][z];
                int index = (localZ * REGION_SIZE + localX) * 4;

                float northY = currentY;
                if (z > 0 && scanColors[x][z - 1].a > 0.01f) {
                    float h = scanHeights[x][z - 1];
                    if (std::abs(currentY - h) < 64.0f) northY = h;
                }
                
                float westY = currentY;
                if (x > 0 && scanColors[x - 1][z].a > 0.01f) {
                    float h = scanHeights[x - 1][z];
                    if (std::abs(currentY - h) < 64.0f) westY = h;
                }

                float diff = (currentY - northY) * 0.15f + (currentY - westY) * 0.15f;
                float shade = std::clamp(1.0f + diff, 0.65f, 1.25f);

                region->colors[index + 0] = (uint8_t)(std::clamp(c.r * shade, 0.0f, 1.0f) * 255.0f);
                region->colors[index + 1] = (uint8_t)(std::clamp(c.g * shade, 0.0f, 1.0f) * 255.0f);
                region->colors[index + 2] = (uint8_t)(std::clamp(c.b * shade, 0.0f, 1.0f) * 255.0f);
                region->colors[index + 3] = (uint8_t)(c.a * 255.0f);

                // [地表Y缓存] 记录此列的地表高度，供未加载区域传送时查询
                int heightIndex = localZ * REGION_SIZE + localX;
                region->heights[heightIndex] = (int16_t)std::clamp(currentY, -64.0f, 320.0f);

                region->dirty = true;
                region->textureDirty = true;
            }
        }
    }

    bool FetchRegionTextureData(uint64_t hash, uint8_t* outBuffer, bool forceCopy) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        auto it = g_loadedRegions.find(hash);
        if (it == g_loadedRegions.end()) {
            g_loadedRegions[hash] = nullptr;
            g_loadQueue.push_back(hash);
            return false;
        } else {
            RegionData* region = it->second;
            if (region && (forceCopy || region->textureDirty)) {
                std::memcpy(outBuffer, region->colors, REGION_SIZE * REGION_SIZE * 4);
                region->textureDirty = false;
                return true;
            }
        }
        return false;
    }

    void MarkTextureDirty(uint64_t hash) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        auto it = g_loadedRegions.find(hash);
        if (it != g_loadedRegions.end() && it->second) it->second->textureDirty = true;
    }

    int16_t GetCachedSurfaceHeight(int worldX, int worldZ, bool isCave) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        if (g_cacheDir.empty()) return HEIGHT_UNKNOWN;
        int rx = (worldX < 0 ? (worldX + 1) / REGION_SIZE - 1 : worldX / REGION_SIZE);
        int rz = (worldZ < 0 ? (worldZ + 1) / REGION_SIZE - 1 : worldZ / REGION_SIZE);
        uint64_t hash = GetRegionHash(rx, rz, isCave);

        auto it = g_loadedRegions.find(hash);
        if (it == g_loadedRegions.end() || it->second == nullptr) {
            // 如果磁盘存在该 region 文件，尝试直接同步加载，确保查询高度时不因异步排队未完成而丢失
            std::string dir = g_cacheDir + GetRegionSubdir(isCave);
            std::string filePath = dir + "region_" + std::to_string(rx) + "_" + std::to_string(rz) + ".bin";
            std::ifstream in(filePath, std::ios::binary | std::ios::ate);
            if (in) {
                auto fileSize = in.tellg();
                in.seekg(0, std::ios::beg);
                RegionData* newRegion = (it != g_loadedRegions.end() && it->second) ? it->second : new RegionData();
                in.read((char*)newRegion->colors, sizeof(newRegion->colors));
                if (fileSize >= (std::streamoff)(sizeof(newRegion->colors) + sizeof(newRegion->heights))) {
                    in.read((char*)newRegion->heights, sizeof(newRegion->heights));
                    // 清理旧版本全 0 污染：未探索像素 (a < 10) 强制置为 HEIGHT_UNKNOWN
                    for (int i = 0; i < REGION_SIZE * REGION_SIZE; ++i) {
                        if (newRegion->colors[i * 4 + 3] < 10) {
                            newRegion->heights[i] = HEIGHT_UNKNOWN;
                        }
                    }
                }
                ReadBiomeSection(in, *newRegion, fileSize,
                                 (std::streamoff)(sizeof(newRegion->colors) + sizeof(newRegion->heights)));
                newRegion->textureDirty = true;
                g_loadedRegions[hash] = newRegion;
                it = g_loadedRegions.find(hash);
            } else {
                if (it == g_loadedRegions.end()) {
                    g_loadedRegions[hash] = nullptr;
                    g_loadQueue.push_back(hash);
                }
                return HEIGHT_UNKNOWN;
            }
        }

        RegionData* region = it->second;
        if (!region) return HEIGHT_UNKNOWN;
        int localX = worldX - (rx * REGION_SIZE);
        int localZ = worldZ - (rz * REGION_SIZE);
        int colorIndex = (localZ * REGION_SIZE + localX) * 4;

        // [严谨有效性检查]
        // 1. 若该像素未渲染/完全透明 (a < 10)，绝对无有效高度，杜绝未渲染区域误判为有缓存
        if (region->colors[colorIndex + 3] < 10) {
            return HEIGHT_UNKNOWN;
        }

        int16_t h = region->heights[localZ * REGION_SIZE + localX];

        // [水域检测与水面高度校正]
        // 若当前像素为地表水体（海洋/河流/沼泽或水色像素）：
        // 在老存档或旧缓存中，水域可能错误记录了海床高度 (h < 63)。
        // 主世界地表水域标准海平面水面高度为 63 (脚部位于水面上方)，因此当检测到水域且 h < 63 时，自动校正返回水面高度 63。
        if (!isCave) {
            uint8_t r = region->colors[colorIndex + 0];
            uint8_t g = region->colors[colorIndex + 1];
            uint8_t b = region->colors[colorIndex + 2];
            bool isWater = false;

            int cellX = localX / BIOME_CELL_SIZE;
            int cellZ = localZ / BIOME_CELL_SIZE;
            int biomeCellIdx = cellZ * BIOME_CELLS_PER_REGION + cellX;
            if (biomeCellIdx >= 0 && biomeCellIdx < (int)sizeof(region->biomeCells)) {
                uint8_t bidx = region->biomeCells[biomeCellIdx];
                if (bidx < region->biomeTable.size()) {
                    std::string const& bname = region->biomeTable[bidx];
                    if (bname.find("ocean") != std::string::npos ||
                        bname.find("river") != std::string::npos ||
                        bname.find("swamp") != std::string::npos) {
                        isWater = true;
                    }
                }
            }
            if (!isWater && b > r + 20 && b > g && b >= 90) {
                isWater = true;
            }

            if (isWater) {
                if (h <= -64 || h == HEIGHT_UNKNOWN || h < 63) {
                    return 63;
                }
            }
        }

        if (h <= -64 || h >= 320 || h == HEIGHT_UNKNOWN) {
            return HEIGHT_UNKNOWN;
        }

        return h;
    }

    // ==========================================
    // [生物群系缓存写入] 从扫描数据批量写入 biomeCells + biomeTable
    // 线程安全：单次锁获取，处理所有条目
    // ==========================================
    void UpdateBiomesFromScan(int targetDim, const std::vector<BiomeEntry>& entries) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        if (g_cacheDir.empty()) return;
        // [严格维度防护] 拒绝写入不属于当前加载维度或玩家物理维度的群系数据，彻底阻断跨维度污染
        if (targetDim != s_currentLoadedDimId || targetDim != MapRenderState::currentDimensionId) return;
        if (s_currentLoadedDimId != MapRenderState::currentDimensionId) return;
        for (const auto& e : entries) {
            // [群系合法性防护] 严格核实生物群系是否属于该维度，绝不将主世界群系写入下界/末地
            if (!IsValidBiomeForDimension(targetDim, e.name)) continue;

            // 世界单元格坐标 → region 坐标
            int blockX = e.cellWorldX * BIOME_CELL_SIZE;
            int blockZ = e.cellWorldZ * BIOME_CELL_SIZE;
            int rx = (blockX < 0 ? (blockX + 1) / REGION_SIZE - 1 : blockX / REGION_SIZE);
            int rz = (blockZ < 0 ? (blockZ + 1) / REGION_SIZE - 1 : blockZ / REGION_SIZE);
            uint64_t hash = GetRegionHash(rx, rz);

            // 确保 region 在内存中（若未加载则跳过，下次扫描会补）
            auto it = g_loadedRegions.find(hash);
            if (it == g_loadedRegions.end() || it->second == nullptr) continue;

            RegionData* region = it->second;
            int localCellX = (blockX - rx * REGION_SIZE) / BIOME_CELL_SIZE;
            int localCellZ = (blockZ - rz * REGION_SIZE) / BIOME_CELL_SIZE;
            if (localCellX < 0 || localCellX >= BIOME_CELLS_PER_REGION ||
                localCellZ < 0 || localCellZ >= BIOME_CELLS_PER_REGION) continue;

            // 查找或插入 biomeTable
            uint8_t idx = BIOME_INDEX_UNKNOWN;
            for (size_t i = 0; i < region->biomeTable.size() && i < 255; i++) {
                if (region->biomeTable[i] == e.name) { idx = (uint8_t)i; break; }
            }
            if (idx == BIOME_INDEX_UNKNOWN) {
                if (region->biomeTable.size() >= 255) continue; // 表满，跳过（极少见）
                region->biomeTable.push_back(e.name);
                idx = (uint8_t)(region->biomeTable.size() - 1);
            }
            region->biomeCells[localCellZ * BIOME_CELLS_PER_REGION + localCellX] = idx;
            region->dirty = true;
        }
    }

    // ==========================================
    // [生物群系缓存查询] 供大地图悬停显示调用
    // 已扫描区域即使区块卸载也能命中（核心修复点）
    // ==========================================
    bool GetCachedBiomeName(int worldX, int worldZ, std::string& outName) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        int rx = (worldX < 0 ? (worldX + 1) / REGION_SIZE - 1 : worldX / REGION_SIZE);
        int rz = (worldZ < 0 ? (worldZ + 1) / REGION_SIZE - 1 : worldZ / REGION_SIZE);
        uint64_t hash = GetRegionHash(rx, rz);

        auto it = g_loadedRegions.find(hash);
        if (it == g_loadedRegions.end()) {
            // 区域未加载 → 排队异步加载，下次查询可命中
            g_loadedRegions[hash] = nullptr;
            g_loadQueue.push_back(hash);
            return false;
        }
        if (it->second == nullptr) return false;  // 已排队但尚未加载完成

        RegionData* region = it->second;
        int localCellX = (worldX - rx * REGION_SIZE) / BIOME_CELL_SIZE;
        int localCellZ = (worldZ - rz * REGION_SIZE) / BIOME_CELL_SIZE;
        if (localCellX < 0 || localCellX >= BIOME_CELLS_PER_REGION ||
            localCellZ < 0 || localCellZ >= BIOME_CELLS_PER_REGION) return false;

        uint8_t idx = region->biomeCells[localCellZ * BIOME_CELLS_PER_REGION + localCellX];
        if (idx == BIOME_INDEX_UNKNOWN || idx >= region->biomeTable.size()) return false;
        outName = region->biomeTable[idx];
        return !outName.empty();
    }

    // ==========================================
    // [缓存水域检测] 判断某世界坐标是否为地表水体
    // ==========================================
    bool IsCachedWater(int worldX, int worldZ, bool isCave) {
        if (isCave) return false;
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        if (g_cacheDir.empty()) return false;
        int rx = (worldX < 0 ? (worldX + 1) / REGION_SIZE - 1 : worldX / REGION_SIZE);
        int rz = (worldZ < 0 ? (worldZ + 1) / REGION_SIZE - 1 : worldZ / REGION_SIZE);
        uint64_t hash = GetRegionHash(rx, rz, isCave);

        auto it = g_loadedRegions.find(hash);
        if (it == g_loadedRegions.end() || it->second == nullptr) {
            return false;
        }

        RegionData* region = it->second;
        int localX = worldX - (rx * REGION_SIZE);
        int localZ = worldZ - (rz * REGION_SIZE);
        int colorIndex = (localZ * REGION_SIZE + localX) * 4;
        if (region->colors[colorIndex + 3] < 10) return false;

        uint8_t r = region->colors[colorIndex + 0];
        uint8_t g = region->colors[colorIndex + 1];
        uint8_t b = region->colors[colorIndex + 2];

        int cellX = localX / BIOME_CELL_SIZE;
        int cellZ = localZ / BIOME_CELL_SIZE;
        int biomeCellIdx = cellZ * BIOME_CELLS_PER_REGION + cellX;
        if (biomeCellIdx >= 0 && biomeCellIdx < (int)sizeof(region->biomeCells)) {
            uint8_t bidx = region->biomeCells[biomeCellIdx];
            if (bidx < region->biomeTable.size()) {
                std::string const& bname = region->biomeTable[bidx];
                if (bname.find("ocean") != std::string::npos ||
                    bname.find("river") != std::string::npos ||
                    bname.find("swamp") != std::string::npos) {
                    return true;
                }
            }
        }
        if (b > r + 20 && b > g && b >= 90) {
            return true;
        }
        return false;
    }

    // ==========================================
    // [小地图极速装载与防黑块核心] 从缓存预填充 513x513 小地图网格
    // ==========================================
    void PrefillMapGrid(int centerX, int centerZ, 
                        mce::Color outColors[MAP_DATA_SIZE][MAP_DATA_SIZE], 
                        float outHeights[MAP_DATA_SIZE][MAP_DATA_SIZE], 
                        bool isCave, 
                        bool onlyIfMissing,
                        uint8_t* outTextureData) {
        std::lock_guard<std::mutex> lock(g_cacheMutex);
        if (g_cacheDir.empty()) {
            if (!onlyIfMissing) {
                std::memset(outColors, 0, sizeof(mce::Color) * MAP_DATA_SIZE * MAP_DATA_SIZE);
                std::memset(outHeights, 0, sizeof(float) * MAP_DATA_SIZE * MAP_DATA_SIZE);
                if (outTextureData) {
                    std::memset(outTextureData, 0, MAP_DATA_SIZE * MAP_DATA_SIZE * 4);
                }
            }
            return;
        }

        int startX = centerX - MAP_DATA_RADIUS;
        int endX   = centerX + MAP_DATA_RADIUS;
        int startZ = centerZ - MAP_DATA_RADIUS;
        int endZ   = centerZ + MAP_DATA_RADIUS;

        int minRX = (startX < 0 ? (startX + 1) / REGION_SIZE - 1 : startX / REGION_SIZE);
        int maxRX = (endX   < 0 ? (endX   + 1) / REGION_SIZE - 1 : endX   / REGION_SIZE);
        int minRZ = (startZ < 0 ? (startZ + 1) / REGION_SIZE - 1 : startZ / REGION_SIZE);
        int maxRZ = (endZ   < 0 ? (endZ   + 1) / REGION_SIZE - 1 : endZ   / REGION_SIZE);

        if (!onlyIfMissing) {
            std::memset(outColors, 0, sizeof(mce::Color) * MAP_DATA_SIZE * MAP_DATA_SIZE);
            std::memset(outHeights, 0, sizeof(float) * MAP_DATA_SIZE * MAP_DATA_SIZE);
            if (outTextureData) {
                std::memset(outTextureData, 0, MAP_DATA_SIZE * MAP_DATA_SIZE * 4);
            }
        }

        for (int rx = minRX; rx <= maxRX; ++rx) {
            for (int rz = minRZ; rz <= maxRZ; ++rz) {
                uint64_t hash = GetRegionHash(rx, rz, isCave);
                auto it = g_loadedRegions.find(hash);
                RegionData* region = (it != g_loadedRegions.end()) ? it->second : nullptr;

                // 若内存中尚未加载，且磁盘存在该区域文件，则极速就地加载
                if (!region) {
                    std::string dir = g_cacheDir + GetRegionSubdir(isCave);
                    std::string filePath = dir + "region_" + std::to_string(rx) + "_" + std::to_string(rz) + ".bin";
                    std::ifstream in(filePath, std::ios::binary | std::ios::ate);
                    if (in) {
                        auto fileSize = in.tellg();
                        if (fileSize >= (std::streamoff)sizeof(RegionData::colors)) {
                            RegionData* newRegion = new RegionData();
                            in.seekg(0, std::ios::beg);
                            in.read((char*)newRegion->colors, sizeof(newRegion->colors));
                            if (fileSize >= (std::streamoff)(sizeof(newRegion->colors) + sizeof(newRegion->heights))) {
                                in.read((char*)newRegion->heights, sizeof(newRegion->heights));
                                for (int i = 0; i < REGION_SIZE * REGION_SIZE; ++i) {
                                    if (newRegion->colors[i * 4 + 3] < 10) {
                                        newRegion->heights[i] = HEIGHT_UNKNOWN;
                                    }
                                }
                            }
                            ReadBiomeSection(in, *newRegion, fileSize, sizeof(newRegion->colors) + sizeof(newRegion->heights));
                            newRegion->textureDirty = true;
                            g_loadedRegions[hash] = newRegion;
                            region = newRegion;
                        }
                    }
                }

                if (!region) continue;

                // 计算该 Region 与当前 513x513 地图网格的相交矩形
                int regMinX = rx * REGION_SIZE;
                int regMaxX = regMinX + REGION_SIZE - 1;
                int regMinZ = rz * REGION_SIZE;
                int regMaxZ = regMinZ + REGION_SIZE - 1;

                int boxMinX = std::max(startX, regMinX);
                int boxMaxX = std::min(endX, regMaxX);
                int boxMinZ = std::max(startZ, regMinZ);
                int boxMaxZ = std::min(endZ, regMaxZ);

                if (boxMinX > boxMaxX || boxMinZ > boxMaxZ) continue;

                for (int wx = boxMinX; wx <= boxMaxX; ++wx) {
                    int arrX = wx - startX;
                    int localX = wx - regMinX;
                    for (int wz = boxMinZ; wz <= boxMaxZ; ++wz) {
                        int arrZ = wz - startZ;
                        int localZ = wz - regMinZ;

                        if (onlyIfMissing && outColors[arrX][arrZ].a > 0.01f) {
                            continue;
                        }

                        int regColorIdx = (localZ * REGION_SIZE + localX) * 4;
                        uint8_t a = region->colors[regColorIdx + 3];
                        if (a >= 10) {
                            uint8_t r = region->colors[regColorIdx + 0];
                            uint8_t g = region->colors[regColorIdx + 1];
                            uint8_t b = region->colors[regColorIdx + 2];
                            int16_t h = region->heights[localZ * REGION_SIZE + localX];
                            float currentY = (h != HEIGHT_UNKNOWN) ? (float)h : 64.0f;
                            outHeights[arrX][arrZ] = currentY;

                            // 逆运算还原未阴影基础色，供 BakingWorkerFunc 统一动态光影渲染
                            float northY = currentY, westY = currentY;
                            if (localZ > 0) {
                                int16_t nh = region->heights[(localZ - 1) * REGION_SIZE + localX];
                                if (nh != HEIGHT_UNKNOWN && std::abs(currentY - (float)nh) < 64.0f) northY = (float)nh;
                            }
                            if (localX > 0) {
                                int16_t wh = region->heights[localZ * REGION_SIZE + (localX - 1)];
                                if (wh != HEIGHT_UNKNOWN && std::abs(currentY - (float)wh) < 64.0f) westY = (float)wh;
                            }
                            float shade = std::clamp(1.0f + (currentY - northY) * 0.15f + (currentY - westY) * 0.15f, 0.65f, 1.25f);
                            float unshadedR = std::clamp((r / 255.0f) / shade, 0.0f, 1.0f);
                            float unshadedG = std::clamp((g / 255.0f) / shade, 0.0f, 1.0f);
                            float unshadedB = std::clamp((b / 255.0f) / shade, 0.0f, 1.0f);

                            outColors[arrX][arrZ] = mce::Color(unshadedR, unshadedG, unshadedB, a / 255.0f);

                            if (outTextureData) {
                                int texIdx = (arrZ * MAP_DATA_SIZE + arrX) * 4;
                                outTextureData[texIdx + 0] = r;
                                outTextureData[texIdx + 1] = g;
                                outTextureData[texIdx + 2] = b;
                                outTextureData[texIdx + 3] = a;
                            }
                        }
                    }
                }
            }
        }
    }


    // ==========================================
    // [PNG导出] 使用 Windows WIC 编码 RGBA 缓冲区为 PNG 文件
    // ==========================================
    static bool SaveRGBAToPNG(const std::filesystem::path& filePath, const uint8_t* rgbaData, int width, int height) {
        if (!rgbaData || width <= 0 || height <= 0) return false;

        HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bool needUninit = SUCCEEDED(hrCo);

        IWICImagingFactory* pFactory = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&pFactory)
        );
        if (FAILED(hr) || !pFactory) {
            if (needUninit) CoUninitialize();
            return false;
        }

        IWICStream* pStream = nullptr;
        IWICBitmapEncoder* pEncoder = nullptr;
        IWICBitmapFrameEncode* pFrame = nullptr;
        IPropertyBag2* pPropBag = nullptr;
        bool success = false;

        do {
            hr = pFactory->CreateStream(&pStream);
            if (FAILED(hr) || !pStream) break;

            hr = pStream->InitializeFromFilename(filePath.wstring().c_str(), GENERIC_WRITE);
            if (FAILED(hr)) break;

            hr = pFactory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &pEncoder);
            if (FAILED(hr) || !pEncoder) break;

            hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
            if (FAILED(hr)) break;

            hr = pEncoder->CreateNewFrame(&pFrame, &pPropBag);
            if (FAILED(hr) || !pFrame) break;

            hr = pFrame->Initialize(pPropBag);
            if (FAILED(hr)) break;

            hr = pFrame->SetSize((UINT)width, (UINT)height);
            if (FAILED(hr)) break;

            WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;
            hr = pFrame->SetPixelFormat(&formatGUID);
            if (FAILED(hr)) break;

            // 将 RGBA 转换为 BGRA 供 WIC 32bppBGRA 写入
            size_t totalPixels = (size_t)width * (size_t)height;
            std::vector<uint8_t> bgra(totalPixels * 4);
            for (size_t i = 0; i < totalPixels; ++i) {
                bgra[i * 4 + 0] = rgbaData[i * 4 + 2]; // B
                bgra[i * 4 + 1] = rgbaData[i * 4 + 1]; // G
                bgra[i * 4 + 2] = rgbaData[i * 4 + 0]; // R
                bgra[i * 4 + 3] = rgbaData[i * 4 + 3]; // A
            }

            UINT stride = (UINT)(width * 4);
            UINT bufferSize = (UINT)(totalPixels * 4);
            hr = pFrame->WritePixels((UINT)height, stride, bufferSize, bgra.data());
            if (FAILED(hr)) break;

            hr = pFrame->Commit();
            if (FAILED(hr)) break;

            hr = pEncoder->Commit();
            if (FAILED(hr)) break;

            success = true;
        } while (false);

        if (pPropBag) pPropBag->Release();
        if (pFrame) pFrame->Release();
        if (pEncoder) pEncoder->Release();
        if (pStream) pStream->Release();
        pFactory->Release();
        if (needUninit) CoUninitialize();

        return success;
    }

    // ==========================================
    // [PNG导出] 自动计算最大化无损整数像素倍率 (1方块 = SxS 像素，最高达 32x32 像素/方块)
    // 使导出的每一张 PNG 图片在看图软件中放到最大时，效果与游戏内全屏大地图缩放到最大完全一致
    // ==========================================
    static int ComputeAutoLosslessPixelScale(int areaW, int areaH, bool multipleImages, int scaleDownSquare) {
        int maxDim = std::max(1, std::max(areaW, areaH));
        if (multipleImages) {
            // 多张无缩放分片模式：每个分片最大 256x256 方块，自动以 8x (即 2048x2048 像素/张) 无损放大
            // 8x 下每个方块拥有 8x8 = 64 个纯色像素，看图软件缩放时极度锐利，同时保存速度大幅提升
            int s = 2048 / maxDim;
            return std::clamp(s, 2, 8);
        }

        // 单张图像模式：根据单张图像最大尺寸设置自动最大化整数像素倍率 (最高 32x，与游戏内大地图最大缩放一致)
        long long maxSidePx = (scaleDownSquare <= 0)
                                  ? 16384LL
                                  : std::clamp((long long)scaleDownSquare * 512LL, 1024LL, 16384LL);
        maxSidePx = std::min(maxSidePx, 16384LL);

        int s = (int)(maxSidePx / (long long)maxDim);
        s = std::clamp(s, 1, 32);
        while (s > 1 && ((long long)areaW * s > 16384LL || (long long)areaH * s > 16384LL)) {
            s--;
        }
        return std::max(1, s);
    }

    // ==========================================
    // [PNG导出] 计算导出范围与实时预览信息 (极速剪枝 + 缓存加速，彻底消除每帧计算卡顿)
    // ==========================================
    static ExportPreviewInfo s_cachedExportPreview{};
    static bool s_hasCachedPreview = false;
    static bool s_exportPreviewDirty = true;
    static bool s_lastForceFull = false;
    static bool s_lastMulti = false;
    static int  s_lastScaleDown = -1;
    static bool s_lastHasSel = false;
    static int  s_lastSelMinX = 0, s_lastSelMaxX = 0, s_lastSelMinZ = 0, s_lastSelMaxZ = 0;
    static int  s_lastViewMinX = 0, s_lastViewMaxX = 0, s_lastViewMinZ = 0, s_lastViewMaxZ = 0;
    static int  s_lastDimId = -999;
    static bool s_lastCave = false;

    void InvalidateExportPreview() {
        s_exportPreviewDirty = true;
    }

    ExportPreviewInfo GetExportPreviewInfo() {
        if (g_cacheDir.empty()) return {};

        int viewDim = MapRenderState::GetEffectiveViewDimensionId();
        int currentDim = MapRenderState::currentDimensionId;
        bool isCave = (viewDim == 1) || (viewDim == 0 && currentDim == 0 && MapRenderState::g_caveModeActive);

        if (s_hasCachedPreview && !s_exportPreviewDirty &&
            MapRenderState::exportForceFullMap == s_lastForceFull &&
            MapRenderState::exportMultipleImages == s_lastMulti &&
            MapRenderState::exportScaleDownSquare == s_lastScaleDown &&
            MapRenderState::hasExportSelection == s_lastHasSel &&
            MapRenderState::exportSelMinX == s_lastSelMinX &&
            MapRenderState::exportSelMaxX == s_lastSelMaxX &&
            MapRenderState::exportSelMinZ == s_lastSelMinZ &&
            MapRenderState::exportSelMaxZ == s_lastSelMaxZ &&
            MapRenderState::exportViewMinX == s_lastViewMinX &&
            MapRenderState::exportViewMaxX == s_lastViewMaxX &&
            MapRenderState::exportViewMinZ == s_lastViewMinZ &&
            MapRenderState::exportViewMaxZ == s_lastViewMaxZ &&
            viewDim == s_lastDimId &&
            isCave == s_lastCave)
        {
            return s_cachedExportPreview;
        }

        ExportPreviewInfo info{};
        int fullMinX = INT_MAX, fullMaxX = INT_MIN;
        int fullMinZ = INT_MAX, fullMaxZ = INT_MIN;

        {
            std::lock_guard<std::mutex> lock(g_cacheMutex);
            for (const auto& pair : g_loadedRegions) {
                if (IsCaveHash(pair.first) != isCave) continue;
                int rx, rz;
                DecodeRegionHash(pair.first, rx, rz);
                if (pair.second != nullptr) {
                    // 1. 快速 64 位无序空域探测（若全透明，直接跳过）
                    const uint64_t* ptr64 = reinterpret_cast<const uint64_t*>(pair.second->colors);
                    bool hasPixel = false;
                    for (int i = 0; i < (REGION_SIZE * REGION_SIZE * 4) / 8; ++i) {
                        if (ptr64[i] != 0) {
                            hasPixel = true;
                            break;
                        }
                    }
                    if (!hasPixel) continue;

                    // 2. 双向剪枝采样：快速确定该 Region 的有效像素包围盒
                    const uint32_t* c32 = reinterpret_cast<const uint32_t*>(pair.second->colors);
                    int rMinLx = REGION_SIZE, rMaxLx = -1;
                    int rMinLz = REGION_SIZE, rMaxLz = -1;

                    // 从上到下查找第一个有效行
                    for (int lz = 0; lz < REGION_SIZE; ++lz) {
                        const uint32_t* row = c32 + lz * REGION_SIZE;
                        for (int lx = 0; lx < REGION_SIZE; ++lx) {
                            if ((row[lx] >> 24) >= 10) { rMinLz = lz; break; }
                        }
                        if (rMinLz != REGION_SIZE) break;
                    }

                    // 从下到上查找最后一个有效行
                    for (int lz = REGION_SIZE - 1; lz >= rMinLz; --lz) {
                        const uint32_t* row = c32 + lz * REGION_SIZE;
                        for (int lx = 0; lx < REGION_SIZE; ++lx) {
                            if ((row[lx] >> 24) >= 10) { rMaxLz = lz; break; }
                        }
                        if (rMaxLz != -1) break;
                    }

                    if (rMinLz <= rMaxLz) {
                        for (int lz = rMinLz; lz <= rMaxLz; ++lz) {
                            const uint32_t* row = c32 + lz * REGION_SIZE;
                            for (int lx = 0; lx < REGION_SIZE; ++lx) {
                                if ((row[lx] >> 24) >= 10) {
                                    if (lx < rMinLx) rMinLx = lx;
                                    break;
                                }
                            }
                            for (int lx = REGION_SIZE - 1; lx >= 0; --lx) {
                                if ((row[lx] >> 24) >= 10) {
                                    if (lx > rMaxLx) rMaxLx = lx;
                                    break;
                                }
                            }
                        }

                        int curMinX = rx * REGION_SIZE + rMinLx;
                        int curMaxX = rx * REGION_SIZE + rMaxLx;
                        int curMinZ = rz * REGION_SIZE + rMinLz;
                        int curMaxZ = rz * REGION_SIZE + rMaxLz;
                        if (curMinX < fullMinX) fullMinX = curMinX;
                        if (curMaxX > fullMaxX) fullMaxX = curMaxX;
                        if (curMinZ < fullMinZ) fullMinZ = curMinZ;
                        if (curMaxZ > fullMaxZ) fullMaxZ = curMaxZ;
                    }
                } else {
                    int rMinX = rx * REGION_SIZE;
                    int rMaxX = rMinX + REGION_SIZE - 1;
                    int rMinZ = rz * REGION_SIZE;
                    int rMaxZ = rMinZ + REGION_SIZE - 1;
                    if (rMinX < fullMinX) fullMinX = rMinX;
                    if (rMaxX > fullMaxX) fullMaxX = rMaxX;
                    if (rMinZ < fullMinZ) fullMinZ = rMinZ;
                    if (rMaxZ > fullMaxZ) fullMaxZ = rMaxZ;
                }
            }
        }

        if (fullMinX > fullMaxX || fullMinZ > fullMaxZ) {
            s_cachedExportPreview = info;
            s_hasCachedPreview = true;
            s_exportPreviewDirty = false;
            return info;
        }

        fullMinX = (fullMinX >> 4) << 4;
        fullMinZ = (fullMinZ >> 4) << 4;
        fullMaxX = ((fullMaxX >> 4) << 4) + 15;
        fullMaxZ = ((fullMaxZ >> 4) << 4) + 15;

        int targetMinX = fullMinX, targetMaxX = fullMaxX;
        int targetMinZ = fullMinZ, targetMaxZ = fullMaxZ;

        // [功能 1] 强制全图导出 (Force Full Map):
        // - 关 (false): 导出用户在全屏大地图框选的区域，或当前大地图屏幕视野范围 (与已探索全图求交集)
        // - 开 (true):  忽略当前视野/框选范围，强制导出整个已探索世界的所有区块
        if (!MapRenderState::exportForceFullMap) {
            int clipMinX = MapRenderState::hasExportSelection ? MapRenderState::exportSelMinX : MapRenderState::exportViewMinX;
            int clipMaxX = MapRenderState::hasExportSelection ? MapRenderState::exportSelMaxX : MapRenderState::exportViewMaxX;
            int clipMinZ = MapRenderState::hasExportSelection ? MapRenderState::exportSelMinZ : MapRenderState::exportViewMinZ;
            int clipMaxZ = MapRenderState::hasExportSelection ? MapRenderState::exportSelMaxZ : MapRenderState::exportViewMaxZ;
            if (clipMinX > clipMaxX) std::swap(clipMinX, clipMaxX);
            if (clipMinZ > clipMaxZ) std::swap(clipMinZ, clipMaxZ);

            clipMinX = (clipMinX >> 4) << 4;
            clipMinZ = (clipMinZ >> 4) << 4;
            clipMaxX = ((clipMaxX >> 4) << 4) + 15;
            clipMaxZ = ((clipMaxZ >> 4) << 4) + 15;

            int interMinX = std::max(fullMinX, clipMinX);
            int interMaxX = std::min(fullMaxX, clipMaxX);
            int interMinZ = std::max(fullMinZ, clipMinZ);
            int interMaxZ = std::min(fullMaxZ, clipMaxZ);

            if (interMinX <= interMaxX && interMinZ <= interMaxZ) {
                targetMinX = interMinX;
                targetMaxX = interMaxX;
                targetMinZ = interMinZ;
                targetMaxZ = interMaxZ;
            }
        }

        info.minBlockX = targetMinX;
        info.maxBlockX = targetMaxX;
        info.minBlockZ = targetMinZ;
        info.maxBlockZ = targetMaxZ;
        info.areaW = std::max(1, targetMaxX - targetMinX + 1);
        info.areaH = std::max(1, targetMaxZ - targetMinZ + 1);
        info.hasValidPixels = true;

        if (MapRenderState::exportMultipleImages) {
            int span = REGION_SIZE; // 256x256 区域切片
            if (info.areaW <= REGION_SIZE && info.areaH <= REGION_SIZE) {
                span = std::max(64, std::min(info.areaW, info.areaH) / 2);
            }
            info.tileSpan = span;
            int tx = (info.areaW + span - 1) / span;
            int tz = (info.areaH + span - 1) / span;
            info.tileCount = std::max(1, tx * tz);
            int tileBlockW = std::min(info.areaW, span);
            int tileBlockH = std::min(info.areaH, span);
            int s = ComputeAutoLosslessPixelScale(tileBlockW, tileBlockH, true, MapRenderState::exportScaleDownSquare);
            info.effectivePixelScale = s;
            info.outW = tileBlockW * s;
            info.outH = tileBlockH * s;
        } else {
            info.tileCount = 1;
            int s = ComputeAutoLosslessPixelScale(info.areaW, info.areaH, false, MapRenderState::exportScaleDownSquare);
            info.effectivePixelScale = s;
            info.outW = info.areaW * s;
            info.outH = info.areaH * s;
        }

        s_lastForceFull = MapRenderState::exportForceFullMap;
        s_lastMulti = MapRenderState::exportMultipleImages;
        s_lastScaleDown = MapRenderState::exportScaleDownSquare;
        s_lastHasSel = MapRenderState::hasExportSelection;
        s_lastSelMinX = MapRenderState::exportSelMinX;
        s_lastSelMaxX = MapRenderState::exportSelMaxX;
        s_lastSelMinZ = MapRenderState::exportSelMinZ;
        s_lastSelMaxZ = MapRenderState::exportSelMaxZ;
        s_lastViewMinX = MapRenderState::exportViewMinX;
        s_lastViewMaxX = MapRenderState::exportViewMaxX;
        s_lastViewMinZ = MapRenderState::exportViewMinZ;
        s_lastViewMaxZ = MapRenderState::exportViewMaxZ;
        s_lastDimId = viewDim;
        s_lastCave = isCave;
        s_exportPreviewDirty = false;
        s_hasCachedPreview = true;
        s_cachedExportPreview = info;

        return s_cachedExportPreview;
    }

    static void ExecuteExportPNG() {
        try {
            if (g_cacheDir.empty()) {
                MapRenderState::exportResultType.store(2); // NOT_PREPARED
                MapRenderState::exportStage.store(2);
                return;
            }

            int viewDim = MapRenderState::GetEffectiveViewDimensionId();
            int currentDim = MapRenderState::currentDimensionId;
            bool isCave = (viewDim == 1) || (viewDim == 0 && currentDim == 0 && MapRenderState::g_caveModeActive);
            bool forceFull = MapRenderState::exportForceFullMap;
            bool multipleImages = MapRenderState::exportMultipleImages;
            bool openFolder = MapRenderState::exportOpenFolder;
            int scaleDownSquare = MapRenderState::exportScaleDownSquare;

            struct ExportRegionItem {
                int rx;
                int rz;
                std::vector<uint8_t> colors;
            };
            std::vector<ExportRegionItem> regions;

            // 1. 收集所有已加载及磁盘上的 region 数据
            {
                std::lock_guard<std::mutex> lock(g_cacheMutex);
                std::string dir = g_cacheDir + GetRegionSubdir(isCave);

                std::unordered_map<uint64_t, bool> seenHashes;
                for (const auto& pair : g_loadedRegions) {
                    if (IsCaveHash(pair.first) != isCave) continue;
                    if (pair.second != nullptr) {
                        int rx, rz;
                        DecodeRegionHash(pair.first, rx, rz);
                        bool hasPixel = false;
                        for (int i = 0; i < REGION_SIZE * REGION_SIZE; ++i) {
                            if (pair.second->colors[i * 4 + 3] >= 10) { hasPixel = true; break; }
                        }
                        if (hasPixel) {
                            ExportRegionItem item;
                            item.rx = rx;
                            item.rz = rz;
                            item.colors.assign(pair.second->colors, pair.second->colors + sizeof(pair.second->colors));
                            regions.push_back(std::move(item));
                        }
                        seenHashes[pair.first] = true;
                    }
                }

                // 同步补充磁盘上尚未完成异步加载的 region_*.bin
                if (std::filesystem::exists(dir)) {
                    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                        if (!entry.is_regular_file()) continue;
                        std::string filename = entry.path().filename().string();
                        if (filename.find("region_") != 0 || filename.find(".bin") == std::string::npos) continue;

                        int rx = 0, rz = 0;
                        size_t pos = 7;
                        bool negX = false;
                        if (pos < filename.size() && filename[pos] == '-') { negX = true; pos++; }
                        while (pos < filename.size() && filename[pos] >= '0' && filename[pos] <= '9') {
                            rx = rx * 10 + (filename[pos] - '0'); pos++;
                        }
                        if (negX) rx = -rx;
                        if (pos >= filename.size() || filename[pos] != '_') continue;
                        pos++;
                        bool negZ = false;
                        if (pos < filename.size() && filename[pos] == '-') { negZ = true; pos++; }
                        while (pos < filename.size() && filename[pos] >= '0' && filename[pos] <= '9') {
                            rz = rz * 10 + (filename[pos] - '0'); pos++;
                        }
                        if (negZ) rz = -rz;

                        uint64_t hash = GetRegionHash(rx, rz, isCave);
                        if (seenHashes.find(hash) != seenHashes.end()) continue;

                        std::ifstream in(entry.path(), std::ios::binary);
                        if (in) {
                            ExportRegionItem item;
                            item.rx = rx;
                            item.rz = rz;
                            item.colors.resize(REGION_SIZE * REGION_SIZE * 4, 0);
                            in.read((char*)item.colors.data(), item.colors.size());
                            bool hasPixel = false;
                            for (int i = 0; i < REGION_SIZE * REGION_SIZE; ++i) {
                                if (item.colors[i * 4 + 3] >= 10) { hasPixel = true; break; }
                            }
                            if (hasPixel) {
                                regions.push_back(std::move(item));
                            }
                        }
                    }
                }
            }

            if (regions.empty()) {
                MapRenderState::exportResultType.store(1); // EMPTY
                MapRenderState::exportStage.store(2);
                return;
            }

            // 2. 计算完整已探索世界包围盒
            int fullMinX = INT_MAX, fullMaxX = INT_MIN;
            int fullMinZ = INT_MAX, fullMaxZ = INT_MIN;
            for (const auto& reg : regions) {
                for (int lz = 0; lz < REGION_SIZE; ++lz) {
                    for (int lx = 0; lx < REGION_SIZE; ++lx) {
                        if (reg.colors[(lz * REGION_SIZE + lx) * 4 + 3] >= 10) {
                            int wx = reg.rx * REGION_SIZE + lx;
                            int wz = reg.rz * REGION_SIZE + lz;
                            if (wx < fullMinX) fullMinX = wx;
                            if (wx > fullMaxX) fullMaxX = wx;
                            if (wz < fullMinZ) fullMinZ = wz;
                            if (wz > fullMaxZ) fullMaxZ = wz;
                        }
                    }
                }
            }

            if (fullMinX > fullMaxX || fullMinZ > fullMaxZ) {
                MapRenderState::exportResultType.store(1); // EMPTY
                MapRenderState::exportStage.store(2);
                return;
            }

            fullMinX = (fullMinX >> 4) << 4;
            fullMinZ = (fullMinZ >> 4) << 4;
            fullMaxX = ((fullMaxX >> 4) << 4) + 15;
            fullMaxZ = ((fullMaxZ >> 4) << 4) + 15;

            int minBlockX = fullMinX, maxBlockX = fullMaxX;
            int minBlockZ = fullMinZ, maxBlockZ = fullMaxZ;

            // 强制全图导出: 关 -> 裁剪至当前框选区域或当前大地图视野范围
            if (!forceFull) {
                int clipMinX = MapRenderState::hasExportSelection ? MapRenderState::exportSelMinX : MapRenderState::exportViewMinX;
                int clipMaxX = MapRenderState::hasExportSelection ? MapRenderState::exportSelMaxX : MapRenderState::exportViewMaxX;
                int clipMinZ = MapRenderState::hasExportSelection ? MapRenderState::exportSelMinZ : MapRenderState::exportViewMinZ;
                int clipMaxZ = MapRenderState::hasExportSelection ? MapRenderState::exportSelMaxZ : MapRenderState::exportViewMaxZ;
                if (clipMinX > clipMaxX) std::swap(clipMinX, clipMaxX);
                if (clipMinZ > clipMaxZ) std::swap(clipMinZ, clipMaxZ);

                clipMinX = (clipMinX >> 4) << 4;
                clipMinZ = (clipMinZ >> 4) << 4;
                clipMaxX = ((clipMaxX >> 4) << 4) + 15;
                clipMaxZ = ((clipMaxZ >> 4) << 4) + 15;

                int interMinX = std::max(fullMinX, clipMinX);
                int interMaxX = std::min(fullMaxX, clipMaxX);
                int interMinZ = std::max(fullMinZ, clipMinZ);
                int interMaxZ = std::min(fullMaxZ, clipMaxZ);

                if (interMinX <= interMaxX && interMinZ <= interMaxZ) {
                    minBlockX = interMinX;
                    maxBlockX = interMaxX;
                    minBlockZ = interMinZ;
                    maxBlockZ = interMaxZ;
                }
            }

            long long fullW = (long long)maxBlockX - (long long)minBlockX + 1;
            long long fullH = (long long)maxBlockZ - (long long)minBlockZ + 1;
            if (fullW <= 0 || fullH <= 0) {
                MapRenderState::exportResultType.store(1);
                MapRenderState::exportStage.store(2);
                return;
            }

            // 构建 region 快速查找表
            std::unordered_map<uint64_t, size_t> regLookup;
            for (size_t i = 0; i < regions.size(); ++i) {
                regLookup[GetRegionHash(regions[i].rx, regions[i].rz, false)] = i;
            }

            auto sampleRawWorldPixel = [&](int wx, int wz, uint8_t& outR, uint8_t& outG, uint8_t& outB, uint8_t& outA) {
                int rx = (wx < 0 ? (wx + 1) / REGION_SIZE - 1 : wx / REGION_SIZE);
                int rz = (wz < 0 ? (wz + 1) / REGION_SIZE - 1 : wz / REGION_SIZE);
                auto it = regLookup.find(GetRegionHash(rx, rz, false));
                if (it == regLookup.end()) {
                    outR = outG = outB = outA = 0;
                    return;
                }
                const auto& reg = regions[it->second];
                int lx = wx - rx * REGION_SIZE;
                int lz = wz - rz * REGION_SIZE;
                int idx = (lz * REGION_SIZE + lx) * 4;
                outR = reg.colors[idx + 0];
                outG = reg.colors[idx + 1];
                outB = reg.colors[idx + 2];
                outA = reg.colors[idx + 3];
            };

            // 自动计算最大化无损整数像素倍率 (1方块 = pixelScale x pixelScale 像素，最高达 32x32 像素/方块)
            int pixelScale = ComputeAutoLosslessPixelScale((int)fullW, (int)fullH, multipleImages, scaleDownSquare);

            // 生成时间戳
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm tm_buf{};
            localtime_s(&tm_buf, &now_c);
            std::ostringstream tsStream;
            tsStream << std::put_time(&tm_buf, "%Y-%m-%d_%H.%M.%S");
            std::string timestamp = tsStream.str();

            std::filesystem::path exportRoot = std::filesystem::path("mods/ChiyanMap/data/export");
            std::filesystem::create_directories(exportRoot);

            if (!multipleImages) {
                if (fullW > 16384LL || fullH > 16384LL) {
                    MapRenderState::exportResultType.store(3); // TOO_BIG
                    MapRenderState::exportStage.store(2);
                    return;
                }

                int outW = 0;
                int outH = 0;
                std::vector<uint8_t> outBuf;
                while (pixelScale >= 1) {
                    long long w64 = fullW * (long long)pixelScale;
                    long long h64 = fullH * (long long)pixelScale;
                    if (w64 <= 16384LL && h64 <= 16384LL && w64 * h64 <= 268435456LL) {
                        try {
                            outBuf.resize((size_t)w64 * (size_t)h64 * 4);
                            outW = (int)w64;
                            outH = (int)h64;
                            break;
                        } catch (const std::bad_alloc&) {
                            outBuf.clear();
                            outBuf.shrink_to_fit();
                        }
                    }
                    pixelScale--;
                }

                if (outW <= 0 || outH <= 0 || outBuf.empty()) {
                    MapRenderState::exportResultType.store(4); // OUT_OF_MEMORY
                    MapRenderState::exportStage.store(2);
                    return;
                }

                // 使用与游戏内全屏大地图完全一致的深色背景 RGB(20, 20, 20, 255)，消除看图软件放大时的透明边缘色晕
                uint32_t* buf32 = reinterpret_cast<uint32_t*>(outBuf.data());
                size_t totalPixels = (size_t)outW * (size_t)outH;
                std::fill(buf32, buf32 + totalPixels, 0xFF141414u);

                // 将每个世界方块以最近邻整数倍无损填充为 pixelScale x pixelScale 的纯色像素块
                for (int lz = 0; lz < (int)fullH; ++lz) {
                    for (int lx = 0; lx < (int)fullW; ++lx) {
                        uint8_t r, g, b, a;
                        sampleRawWorldPixel(minBlockX + lx, minBlockZ + lz, r, g, b, a);
                        if (a >= 10) {
                            uint32_t rgba = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0xFF000000u;
                            int basePx = lx * pixelScale;
                            int basePz = lz * pixelScale;
                            for (int sz = 0; sz < pixelScale; ++sz) {
                                uint32_t* rowPtr = buf32 + (size_t)(basePz + sz) * (size_t)outW + (size_t)basePx;
                                std::fill(rowPtr, rowPtr + pixelScale, rgba);
                            }
                        }
                    }
                }

                std::string modeTag = forceFull ? "_full" : "_view";
                std::string baseName = timestamp + modeTag + "_x" + std::to_string(minBlockX) + "_z" + std::to_string(minBlockZ) +
                                       "_" + std::to_string(outW) + "x" + std::to_string(outH);
                std::filesystem::path outPath = exportRoot / (baseName + ".png");
                int suffix = 1;
                while (std::filesystem::exists(outPath)) {
                    suffix++;
                    outPath = exportRoot / (baseName + "_" + std::to_string(suffix) + ".png");
                }

                if (!SaveRGBAToPNG(outPath, outBuf.data(), outW, outH)) {
                    MapRenderState::exportResultType.store(5); // IO_EXCEPTION
                    MapRenderState::exportStage.store(2);
                    return;
                }

                std::filesystem::path absFolder = std::filesystem::absolute(exportRoot);
                {
                    std::lock_guard<std::mutex> lk(MapRenderState::exportResultMutex);
                    MapRenderState::exportResultPath = outPath.string();
                }
                MapRenderState::exportResultType.store(0); // SUCCESS
                MapRenderState::exportStage.store(2);
                if (openFolder) {
                    ShellExecuteW(nullptr, L"open", absFolder.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
            } else {
                // 多张无缩放图像模式 (Multiple Unscaled Images) + 多线程并行急速导出 + 支持终止与清空
                std::filesystem::path multiFolder = exportRoot / timestamp;
                int folderSuffix = 1;
                while (std::filesystem::exists(multiFolder)) {
                    folderSuffix++;
                    multiFolder = exportRoot / (timestamp + "_" + std::to_string(folderSuffix));
                }
                std::filesystem::create_directories(multiFolder);

                int tileSpan = REGION_SIZE;
                if (fullW <= REGION_SIZE && fullH <= REGION_SIZE) {
                    tileSpan = std::max(64, (int)std::min(fullW, fullH) / 2);
                }
                int tilesX = (int)((fullW + tileSpan - 1) / tileSpan);
                int tilesZ = (int)((fullH + tileSpan - 1) / tileSpan);

                struct ActiveTileTask {
                    int tx, tz;
                    int tileMinX, tileMinZ;
                    int tileW, tileH;
                };
                std::vector<ActiveTileTask> activeTasks;

                // 1. 预扫描分片：筛选出所有实际包含已探索像素的分片任务
                for (int tz = 0; tz < tilesZ; ++tz) {
                    for (int tx = 0; tx < tilesX; ++tx) {
                        int tileMinX = minBlockX + tx * tileSpan;
                        int tileMinZ = minBlockZ + tz * tileSpan;
                        int tileW = (int)std::min<long long>(tileSpan, (long long)maxBlockX - tileMinX + 1);
                        int tileH = (int)std::min<long long>(tileSpan, (long long)maxBlockZ - tileMinZ + 1);
                        if (tileW <= 0 || tileH <= 0) continue;

                        bool tileHasPixels = false;
                        for (int lz = 0; lz < tileH && !tileHasPixels; ++lz) {
                            for (int lx = 0; lx < tileW; ++lx) {
                                uint8_t r, g, b, a;
                                sampleRawWorldPixel(tileMinX + lx, tileMinZ + lz, r, g, b, a);
                                if (a >= 10) {
                                    tileHasPixels = true;
                                    break;
                                }
                            }
                        }
                        if (tileHasPixels) {
                            activeTasks.push_back({tx, tz, tileMinX, tileMinZ, tileW, tileH});
                        }
                    }
                }

                if (activeTasks.empty()) {
                    std::error_code ec;
                    std::filesystem::remove_all(multiFolder, ec);
                    MapRenderState::exportResultType.store(1); // EMPTY
                    MapRenderState::exportStage.store(2);
                    return;
                }

                // 2. 初始化进度条原子计数器
                MapRenderState::exportTotalTiles.store((int)activeTasks.size());
                MapRenderState::exportCompletedTiles.store(0);

                // 3. 多线程并行导出分片图片，大幅加快导出速度
                unsigned int hwThreads = std::thread::hardware_concurrency();
                int numWorkers = std::clamp((int)hwThreads, 2, 6);
                if (numWorkers > (int)activeTasks.size()) numWorkers = (int)activeTasks.size();

                std::atomic<size_t> nextTaskIdx{0};
                std::atomic<int> exportedCount{0};

                auto workerFunc = [&]() {
                    while (!MapRenderState::exportCancelRequested.load()) {
                        size_t taskIdx = nextTaskIdx.fetch_add(1);
                        if (taskIdx >= activeTasks.size()) break;
                        const auto& task = activeTasks[taskIdx];

                        int tileScale = ComputeAutoLosslessPixelScale(task.tileW, task.tileH, true, scaleDownSquare);
                        int tileOutW = 0;
                        int tileOutH = 0;
                        std::vector<uint8_t> tileBuf;
                        while (tileScale >= 1) {
                            long long w64 = (long long)task.tileW * tileScale;
                            long long h64 = (long long)task.tileH * tileScale;
                            if (w64 <= 16384LL && h64 <= 16384LL) {
                                try {
                                    tileBuf.resize((size_t)w64 * (size_t)h64 * 4);
                                    tileOutW = (int)w64;
                                    tileOutH = (int)h64;
                                    break;
                                } catch (const std::bad_alloc&) {
                                    tileBuf.clear();
                                    tileBuf.shrink_to_fit();
                                }
                            }
                            tileScale--;
                        }

                        if (tileOutW > 0 && tileOutH > 0 && !tileBuf.empty()) {
                            uint32_t* tileBuf32 = reinterpret_cast<uint32_t*>(tileBuf.data());
                            std::fill(tileBuf32, tileBuf32 + (size_t)tileOutW * (size_t)tileOutH, 0xFF141414u);

                            for (int lz = 0; lz < task.tileH; ++lz) {
                                for (int lx = 0; lx < task.tileW; ++lx) {
                                    uint8_t r, g, b, a;
                                    sampleRawWorldPixel(task.tileMinX + lx, task.tileMinZ + lz, r, g, b, a);
                                    if (a >= 10) {
                                        uint32_t rgba = (uint32_t)r | ((uint32_t)g << 8) | ((uint32_t)b << 16) | 0xFF000000u;
                                        int basePx = lx * tileScale;
                                        int basePz = lz * tileScale;
                                        for (int sz = 0; sz < tileScale; ++sz) {
                                            uint32_t* rowPtr = tileBuf32 + (size_t)(basePz + sz) * (size_t)tileOutW + (size_t)basePx;
                                            std::fill(rowPtr, rowPtr + tileScale, rgba);
                                        }
                                    }
                                }
                            }

                            if (!MapRenderState::exportCancelRequested.load()) {
                                std::string tileFile = std::to_string(task.tx) + "_" + std::to_string(task.tz) +
                                                       "_x" + std::to_string(task.tileMinX) + "_z" + std::to_string(task.tileMinZ) + ".png";
                                if (SaveRGBAToPNG(multiFolder / tileFile, tileBuf.data(), tileOutW, tileOutH)) {
                                    exportedCount.fetch_add(1);
                                }
                            }
                        }

                        MapRenderState::exportCompletedTiles.fetch_add(1);
                    }
                };

                std::vector<std::thread> workers;
                workers.reserve(numWorkers);
                for (int w = 0; w < numWorkers; ++w) {
                    workers.emplace_back(workerFunc);
                }
                for (auto& th : workers) {
                    if (th.joinable()) th.join();
                }

                // 4. 若导出过程中玩家点击了“终止并清空”，立即清空刚刚导出的图片并删除文件夹
                if (MapRenderState::exportCancelRequested.load()) {
                    std::error_code ec;
                    std::filesystem::remove_all(multiFolder, ec);
                    MapRenderState::exportResultType.store(6); // 6 = CANCELED
                    MapRenderState::exportStage.store(2);
                    return;
                }

                if (exportedCount.load() == 0) {
                    std::error_code ec;
                    std::filesystem::remove_all(multiFolder, ec);
                    MapRenderState::exportResultType.store(5); // IO_EXCEPTION
                    MapRenderState::exportStage.store(2);
                    return;
                }

                std::filesystem::path absFolder = std::filesystem::absolute(multiFolder);
                {
                    std::lock_guard<std::mutex> lk(MapRenderState::exportResultMutex);
                    MapRenderState::exportResultPath = multiFolder.string() + " (" + std::to_string(exportedCount.load()) + " PNGs)";
                }
                MapRenderState::exportResultType.store(0); // SUCCESS
                MapRenderState::exportStage.store(2);
                if (openFolder) {
                    ShellExecuteW(nullptr, L"open", absFolder.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                }
            }
        } catch (const std::bad_alloc&) {
            MapRenderState::exportResultType.store(4); // OUT_OF_MEMORY
            MapRenderState::exportStage.store(2);
        } catch (...) {
            MapRenderState::exportResultType.store(5); // IO_EXCEPTION
            MapRenderState::exportStage.store(2);
        }
    }

    void TriggerExportMapToPNG() {
        int stage = MapRenderState::exportStage.load();
        if (stage == 1) return; // 已经在导出中
        MapRenderState::exportCancelRequested.store(false);
        MapRenderState::exportTotalTiles.store(0);
        MapRenderState::exportCompletedTiles.store(0);
        MapRenderState::exportStage.store(1);
        MapRenderState::exportResultType.store(-1);
        std::thread(ExecuteExportPNG).detach();
    }
}
