#pragma once
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <vector>
#include <mc/deps/core/math/Color.h>

namespace MapRenderState {
    inline std::atomic<int> frameCallCount{0};
    inline int lastFrameTotalCalls{1};
    inline bool showBigMap = false; // 大地图开启状态
    inline float bigMapOffsetX = 0.0f;
    inline float bigMapOffsetZ = 0.0f;
    inline float bigMapZoom = 3.0f; // 默认放大 3 倍
    inline bool bigMapShowEntities = false; // 全屏大地图生物头像显示开关 (直观图标按钮切换)
    inline bool bigMapShowMarkers = true;  // 全屏大地图标记显示开关 (路径点 + 死亡点，图标按钮切换，持久化)
    inline bool showChunkGrid = false;     // 区块网格显示开关 (小地图 + 大地图共用，图标按钮切换，持久化)

    // 当前玩家所处生物群系名称 (拆分为原始命名空间ID与本地化名称)
    inline std::string rawBiomeName = "minecraft:unknown";
    inline std::string translatedBiomeName = "Unknown Biome";

    // [大地图鼠标悬停生物群系] 独立于玩家当前生物群系，不影响小地图
    inline std::string hoverBiomeRawName = "";          // 鼠标位置原始生物群系名
    inline std::string hoverBiomeTranslatedName = "";   // 鼠标位置翻译后生物群系名
    inline float hoverBiomeAlpha = 0.0f;                 // 当前显示透明度 (0=隐藏, 1=完全显示)
    inline float hoverBiomeTargetAlpha = 0.0f;           // 目标透明度
    inline int hoverBiomeFrameCounter = 0;               // 节流帧计数器
    inline float hoverBiomeLastQueryX = 0.0f;            // 上次查询的世界 X
    inline float hoverBiomeLastQueryZ = 0.0f;            // 上次查询的世界 Z
    inline bool hoverBiomeHasValidResult = false;        // 是否有有效查询结果

    // [新增] 世界与维度隔离系统状态
    inline std::string currentWorldId = "";
    inline int currentDimensionId = -999;
    inline std::atomic<bool> clearGPUCache{false}; // 用于通知 GPU 清理旧世界的贴图残留

    // [维度切换] 全屏大地图浏览维度 (-999 表示跟随玩家物理维度，0=主世界, 1=下界, 2=末地)
    inline int bigMapViewDimensionId = -999;

    inline int GetEffectiveViewDimensionId() {
        if (bigMapViewDimensionId >= 0 && bigMapViewDimensionId <= 2) {
            return bigMapViewDimensionId;
        }
        return currentDimensionId;
    }

    // [世界出生点坐标]
    inline int worldSpawnX = 0;
    inline int worldSpawnY = 64;
    inline int worldSpawnZ = 0;
    inline bool hasWorldSpawn = false;

    inline void GetWorldSpawnCoords(int& outX, int& outZ) {
        if (hasWorldSpawn) {
            outX = worldSpawnX;
            outZ = worldSpawnZ;
            return;
        }
        size_t pPos = currentWorldId.rfind("_P");
        if (pPos != std::string::npos) {
            std::string s = currentWorldId.substr(pPos + 2);
            size_t under = s.find('_');
            if (under != std::string::npos) {
                try {
                    outX = std::stoi(s.substr(0, under));
                    outZ = std::stoi(s.substr(under + 1));
                    return;
                } catch (...) {}
            }
        }
        outX = 0;
        outZ = 0;
    }

    struct DimCameraState {
        float centerWx = 0.0f;
        float centerWz = 0.0f;
        float zoom = 3.0f;
        bool initialized = false;
    };
    inline DimCameraState dimCameraStates[3];

    inline void ResetDimCameraStates() {
        for (int i = 0; i < 3; ++i) {
            dimCameraStates[i] = DimCameraState{};
        }
    }

    inline void SaveDimCamera(int dim, float smoothPX, float smoothPZ) {
        if (dim >= 0 && dim < 3) {
            dimCameraStates[dim].centerWx = smoothPX - bigMapOffsetX / bigMapZoom;
            dimCameraStates[dim].centerWz = smoothPZ - bigMapOffsetZ / bigMapZoom;
            dimCameraStates[dim].zoom = bigMapZoom;
            dimCameraStates[dim].initialized = true;
        }
    }

    inline void RestoreDimCamera(int targetDim, int fromDim, float smoothPX, float smoothPZ) {
        if (targetDim < 0 || targetDim >= 3) return;
        // [下界/异界切主世界] 当位于下界（或末地）切换到主世界时，必须以出生点为中心
        if ((fromDim == 1 || fromDim == 2) && targetDim == 0) {
            int spX = 0, spZ = 0;
            GetWorldSpawnCoords(spX, spZ);
            float targetWx = (float)spX;
            float targetWz = (float)spZ;
            bigMapZoom = 3.0f;
            bigMapOffsetX = (smoothPX - targetWx) * bigMapZoom;
            bigMapOffsetZ = (smoothPZ - targetWz) * bigMapZoom;
            dimCameraStates[0].centerWx = targetWx;
            dimCameraStates[0].centerWz = targetWz;
            dimCameraStates[0].zoom = bigMapZoom;
            dimCameraStates[0].initialized = true;
            return;
        }
        if (dimCameraStates[targetDim].initialized) {
            bigMapZoom = dimCameraStates[targetDim].zoom;
            float targetWx = dimCameraStates[targetDim].centerWx;
            float targetWz = dimCameraStates[targetDim].centerWz;
            bigMapOffsetX = (smoothPX - targetWx) * bigMapZoom;
            bigMapOffsetZ = (smoothPZ - targetWz) * bigMapZoom;
        } else {
            bigMapZoom = 3.0f;
            float targetWx = 0.0f;
            float targetWz = 0.0f;
            if (fromDim == 0 && targetDim == 1) {
                targetWx = smoothPX / 8.0f;
                targetWz = smoothPZ / 8.0f;
            } else if (fromDim == 1 && targetDim == 0) {
                int spX = 0, spZ = 0;
                GetWorldSpawnCoords(spX, spZ);
                targetWx = (float)spX;
                targetWz = (float)spZ;
            } else if (targetDim == 2) {
                targetWx = 0.0f;
                targetWz = 0.0f;
            } else {
                targetWx = smoothPX;
                targetWz = smoothPZ;
            }
            bigMapOffsetX = (smoothPX - targetWx) * bigMapZoom;
            bigMapOffsetZ = (smoothPZ - targetWz) * bigMapZoom;
            dimCameraStates[targetDim].centerWx = targetWx;
            dimCameraStates[targetDim].centerWz = targetWz;
            dimCameraStates[targetDim].zoom = bigMapZoom;
            dimCameraStates[targetDim].initialized = true;
        }
    }

    inline void CenterCameraOnViewDimension(float smoothPX, float smoothPZ) {
        int viewDim = GetEffectiveViewDimensionId();
        if (viewDim == currentDimensionId) {
            bigMapOffsetX = 0.0f;
            bigMapOffsetZ = 0.0f;
        } else {
            float targetWx = 0.0f;
            float targetWz = 0.0f;
            if (currentDimensionId == 0 && viewDim == 1) {
                targetWx = smoothPX / 8.0f;
                targetWz = smoothPZ / 8.0f;
            } else if ((currentDimensionId == 1 || currentDimensionId == 2) && viewDim == 0) {
                int spX = 0, spZ = 0;
                GetWorldSpawnCoords(spX, spZ);
                targetWx = (float)spX;
                targetWz = (float)spZ;
            } else if (viewDim == 2) {
                targetWx = 0.0f;
                targetWz = 0.0f;
            }
            bigMapOffsetX = (smoothPX - targetWx) * bigMapZoom;
            bigMapOffsetZ = (smoothPZ - targetWz) * bigMapZoom;
        }
    }

    // [新增] 路径点 UI 开启状态
    inline bool showWaypointUI = false;
    inline bool showDeathPointUI = false;
    inline int waypointSortMode = 0; // 0=时间(最近), 1=时间(最远), 2=名称A-Z, 3=名称Z-A, 4=距离近-远, 5=距离远-近, 6=手动排序
    inline std::string waypointFolderFilter = ""; // ""=全部文件夹, "__ROOT__"=未分类, 其他=文件夹名

    // [新增] 小地图路径点显示开关 (独立于路径点 enabled 属性，控制是否在小地图上绘制)
    inline bool showWaypointsOnMinimap = true;

    // [新增] 小地图雷达显示开关 (控制是否扫描并显示周围生物/实体)
    inline bool showRadar = true;

    // [新增] 快捷键设置面板开启状态
    inline bool showHotkeySettings = false;
    inline bool showCaveSettings = false; // [洞穴地图] 洞穴设置面板
    inline bool showExportPNGScreen = false; // [PNG导出] World Map PNG Export 全屏面板开启状态
    inline bool showSeedMap = false; // [种子全知地图] 结构预测/群系查找/Slime区块面板开启状态

    // [PNG导出] PNG 导出面板状态与配置 (全部持久化保存于 config.json)
    inline bool exportForceFullMap = false;     // 强制全图导出 (持久化，默认关)
    inline bool exportMultipleImages = false;   // 多张无缩放图像 (持久化，默认关)
    inline bool exportOpenFolder = true;        // 导出后自动打开所在文件夹 (持久化，默认开)
    inline int  exportScaleDownSquare = 20;     // 单张图像最大尺寸 (0=不限制, 1..90=NxN 区域，持久化，默认20)
    inline std::atomic<int> exportStage{0};     // 0=Idle, 1=Exporting, 2=Finished
    inline std::atomic<int> exportResultType{-1}; // -1=None, 0=SUCCESS, 1=EMPTY, 2=NOT_PREPARED, 3=TOO_BIG, 4=OUT_OF_MEMORY, 5=IO_EXCEPTION, 6=CANCELED
    inline std::atomic<bool> exportCancelRequested{false}; // 用户请求终止导出并清空已导出图片
    inline std::atomic<int>  exportTotalTiles{0};          // 多张分片导出总片数
    inline std::atomic<int>  exportCompletedTiles{0};      // 多张分片已完成片数
    inline std::string exportResultPath = "";   // 导出结果路径展示
    inline std::mutex exportResultMutex;

    // [PNG导出] 当前大地图视野范围与框选区域 (用于区分 强制全图导出: 关 vs 开)
    inline int  exportViewMinX = -256;
    inline int  exportViewMaxX = 256;
    inline int  exportViewMinZ = -256;
    inline int  exportViewMaxZ = 256;
    inline bool hasExportSelection = false;
    inline int  exportSelMinX = 0;
    inline int  exportSelMaxX = 0;
    inline int  exportSelMinZ = 0;
    inline int  exportSelMaxZ = 0;

    inline void OpenExportPNGScreen() {
        showExportPNGScreen = true;
        if (exportStage.load() != 1) {
            exportStage.store(0);
            exportResultType.store(-1);
            exportCancelRequested.store(false);
            exportTotalTiles.store(0);
            exportCompletedTiles.store(0);
            std::lock_guard<std::mutex> lk(exportResultMutex);
            exportResultPath.clear();
        }
    }

    // [快捷键增强] 单键或多按键组合结构 (支持 Ctrl / Shift / Alt 修饰键)
    // 默认值: M=0x4D, U=0x55, N=0x4E, Y=0x59, J=0x4A, Tab=0x09
    // key=0 表示已禁用 (清除设置)，不匹配任何按键事件
    // openBigMap 支持自定义按键，但不可清除 (严禁设为 0)，防止玩家因误清除导致无法呼出操作面板
    struct Hotkey {
        int key = 0;              // 虚拟键码 (Win32 VK_*)，0 表示已禁用 (清除)
        uint8_t modifiers = 0;    // 修饰键掩码 (MOD_CTRL, MOD_SHIFT, MOD_ALT)

        static constexpr uint8_t HK_MOD_CTRL  = 1 << 0;
        static constexpr uint8_t HK_MOD_SHIFT = 1 << 1;
        static constexpr uint8_t HK_MOD_ALT   = 1 << 2;

        constexpr bool IsEmpty() const { return key == 0; }
        void Clear() { key = 0; modifiers = 0; }

        constexpr bool operator==(const Hotkey& o) const {
            return key == o.key && modifiers == o.modifiers;
        }
        constexpr bool operator!=(const Hotkey& o) const {
            return !(*this == o);
        }
    };

    struct HotkeyBindings {
        Hotkey openBigMap        = { 0x4D, 0 }; // M: 切换大地图 (可自定义，不可清除)
        Hotkey openWaypointMgr   = { 0x55, 0 }; // U: 切换路径点管理器
        Hotkey openDeathPointMgr = { 0x49, 0 }; // I: 切换死亡记录管理器
        Hotkey toggleMinimap     = { 0x4E, 0 }; // N: 切换小地图显示
        Hotkey toggleMinimapShape= { 0x59, 0 }; // Y: 切换小地图形状
        Hotkey toggleMinimapRot  = { 0x4A, 0 }; // J: 切换小地图旋转
        Hotkey holdEntities      = { 0x09, 0 }; // Tab: 显示生物头像 (小地图/大地图)
        Hotkey toggleSeedMap     = { 0x4B, 0 }; // K: 切换种子全知地图

        // 系统预设默认值 (单一来源，供"单行重置"与"全部重置"共用)
        static constexpr HotkeyBindings Defaults() {
            return {
                { 0x4D, 0 },
                { 0x55, 0 },
                { 0x49, 0 },
                { 0x4E, 0 },
                { 0x59, 0 },
                { 0x4A, 0 },
                { 0x09, 0 },
                { 0x4B, 0 }
            };
        }
    };
    inline HotkeyBindings g_hotkeys;
    // 正在监听按键的重绑目标 (nullptr=未在监听)
    inline Hotkey* g_listeningHotkey = nullptr;
    // 监听状态下实时记录的修饰键组合 (Ctrl / Shift / Alt)
    inline uint8_t g_listeningModifiers = 0;
    // [快捷键增强] Ctrl+Z 撤销请求标志 (由 WndProc 设置，渲染线程消费)
    inline std::atomic<bool> g_hotkeyUndoRequested{false};

    // [新增] 小地图偏移与位置设置
    inline float miniMapOffsetX = 0.0f; // 小地图 X 偏移
    inline float miniMapOffsetY = 0.0f; // 小地图 Y 偏移
    inline bool showMiniMapPosSettings = false; // 小地图位置设置面板

    // [传送状态机] 用于 UI 加载提示与异常反馈
    // Idle: 无传送 / Loading: 等待区块加载 / Validating: 验证地表 / Failed: 异常回退
    enum class TeleportState : int { Idle = 0, Loading = 1, Validating = 2, Done = 3, Failed = 4 };
    inline std::atomic<int> teleportState{(int)TeleportState::Idle};
    inline std::string teleportStatusMsg;     // 给 UI 显示的简短状态文本（如"加载地形中..."）
    inline std::string teleportFailReason;    // 失败原因（供日志与调试）

    // [统一拦截枢纽] 判断是否有任何全屏 UI 处于活动状态
    inline bool IsUIActive() {
        int tp = teleportState.load();
        bool isTeleportUIActive = (tp == (int)TeleportState::Loading || 
                                   tp == (int)TeleportState::Validating || 
                                   tp == (int)TeleportState::Failed);
        return showBigMap || showWaypointUI || showDeathPointUI || showMiniMapPosSettings || showHotkeySettings || showCaveSettings ||
               showExportPNGScreen || showSeedMap || isTeleportUIActive;
    }

    // [新增] 跨菜单桥接：大地图右键唤起新建地标的预设坐标与归属维度
    inline bool triggerAddWaypoint = false;
    inline int addWaypointX = -999999;
    inline int addWaypointY = -999999;
    inline int addWaypointZ = -999999;
    inline int addWaypointDim = -1; // -1=当前维度, 0=主世界, 1=下界, 2=末地

    // [新增] 原生瞬间传送信号器
    inline std::atomic<bool> triggerTeleport{false};
    inline float tpTargetX = 0.0f;
    inline float tpTargetY = 0.0f;
    inline float tpTargetZ = 0.0f;
    inline int tpTargetDim = -1; // 目标维度: 0=主世界 1=下界 2=末地; -1=未指定(默认当前维度)

    // [两阶段地表探测] 未访问区域的分阶段传送状态
    // Phase 0: 已 tp 到 Y=320 触发服务器下发目标区块，等待客户端区块就绪
    // Phase 1: 区块就绪后 re-tp 到实际地表（/tp 重置坠落距离，无需缓降）
    // 仅在缓存与实时探测均未命中（玩家从未到访）时启用
    inline std::atomic<bool> pendingSurfaceProbe{false};
    inline std::atomic<int> probeTargetX{0};
    inline std::atomic<int> probeTargetZ{0};
    inline float probeOriginalX = 0.0f;  // 探测超时回退坐标
    inline float probeOriginalY = 0.0f;
    inline float probeOriginalZ = 0.0f;
    inline std::chrono::steady_clock::time_point probeStartTime;

    // [维度感知传送] 探测模式: 0=地表(主世界非洞穴), 1=洞穴/下界, 2=末地
    // Phase 1 轮询时根据此值选择 SafeFindSafeSpawnY(地表) 或 SafeFindSafeSpawnYNearY(洞穴/下界/末地)
    inline int probeMode = 0;

    // [Y范围限制] 探测时 SafeFindSafeSpawnYNearY 的扫描范围
    // 下界 [2,125] 避开基岩层; 洞穴 [refY-48, refY+48] 避免穿过天花板到地表
    inline int probeMinY = -64;
    inline int probeMaxY = 319;

    // [探测参考Y] Phase 1 搜索的起始Y参考值
    // 下界=125(从顶部往下搜); 洞穴/末地=玩家原始Y
    inline int probeRefY = 64;

    // [下界探测标志] 标记当前探测是否为下界传送, 用于延长超时时间
    inline bool probeIsNether = false;

    // [安全传送增强] 探测稳定性检查：连续 N 帧返回相同 Y 才视为"区块加载完成"
    // 防止部分加载区块返回临时错误 Y（如地下结构）导致地下传送
    // v2: 从 2 帧升级到 3 帧，进一步过滤部分加载区块的"稳定但错误"Y 值
    inline short probeLastY = -32000;         // 上一帧探测到的 Y
    inline int  probeLastX = 0;               // 上一帧探测到的 X
    inline int  probeLastZ = 0;               // 上一帧探测到的 Z
    inline int  probeStableCount = 0;         // 连续一致帧数
    constexpr static int kProbeStableThreshold = 8;  // 需要的连续一致帧数（6→8，约133~160ms）


    inline bool showMiniMap = true;  // 是否显示小地图
    inline bool isSquareMap = false; // 是否为方形小地图
    inline bool rotateMiniMap = false; // 小地图跟随视角旋转
    inline float uiTextScale = 1.0f; // UI 文本缩放比例
    inline float miniMapScale = 1.0f; // 小地图本身大小缩放
    inline float miniMapZoomRadius = 50.0f; // 小地图可视范围（玩家周围方块半径，10-200）

    // [关闭期安全标志] disable() 入口处置 true，所有钩子和后台线程在入口检查此标志并提前返回，
    // 防止进程退出阶段访问已释放的 D3D/ImGui 资源导致 0xC0000005 退出崩溃
    inline std::atomic<bool> g_isShuttingDown{false};

    // ==================== 洞穴地图系统 (Xaero's Cave Map 1:1 复刻) ====================
    // 洞穴模式类型 (对应 Xaero's DEFAULT_CAVE_MODE_TYPE)
    // 0=Off(关闭), 1=Layered(分层)
    enum class CaveModeType : int { Off = 0, Layered = 1 };

    // 洞穴模式设置 (持久化到 config) — 对齐 Xaero's 设置面板
    inline int g_caveModeType = (int)CaveModeType::Layered;  // 默认 Layered (Xaero 默认值)
    inline bool g_caveTopYAuto = true;                        // Cave Mode Top Y: auto=自动检测, false=手动指定
    inline int g_caveTopY = 64;                               // Top Y: 手动模式下的洞穴顶层Y (默认 64=海平面, 有效范围 [-64,320])
    inline int g_caveDepth = 30;                               // 洞穴扫描深度 (Xaero: 1-64, 默认 30)
    inline bool g_legibleCaveMaps = false;                     // 清晰洞穴地图 (Xaero: 默认 false)

    // 洞穴模式运行时状态 (每帧计算)
    inline bool g_caveModeActive = false;       // 当前是否处于洞穴模式 (玩家在地下)
    inline int g_caveStartY = 0;                // 当前洞穴起始 Y (扫描顶部)
}

// 【全球探索级】：匹配 16 区块能见度的究极扫描半径（513x513个方块）！
constexpr int MAP_DATA_RADIUS = 256; 
constexpr int MAP_DATA_SIZE   = 513;

struct RadarEntity {
    float x;
    float y;
    float z;
    int type; 
    std::string typeName;
    std::string nameTag;
    std::string uuid;
};

struct PlayerSkinHead {
    uint8_t pixels[8 * 8 * 4]{}; // 8x8 RGBA
    uint64_t fingerprint = 0;
    uint64_t revision = 0;
    bool valid = false;
};

extern std::unordered_map<std::string, PlayerSkinHead> g_playerSkinHeads; // keyed by UUID
inline std::mutex g_playerSkinMutex;
extern std::string g_localPlayerUuid;
inline std::atomic<bool> clearPlayerHeadTextures{false};

inline std::mutex g_radarMutex;
extern std::atomic<bool> g_radarUpdated;
inline std::atomic<uint64_t> g_radarGeneration{1};
inline std::atomic<bool> g_tabHeld{false};
extern std::vector<RadarEntity> g_radarEntities;
inline std::atomic<bool> g_mapDataUpdated{true}; 

// 前台缓冲（仅供显卡渲染读取，绝不闪烁）
extern mce::Color g_mapColors[MAP_DATA_SIZE][MAP_DATA_SIZE];
extern float g_mapHeights[MAP_DATA_SIZE][MAP_DATA_SIZE];

// 后台缓冲（供 CPU 高速扫描写入）
inline mce::Color g_mapColorsBack[MAP_DATA_SIZE][MAP_DATA_SIZE];
inline float g_mapHeightsBack[MAP_DATA_SIZE][MAP_DATA_SIZE];

// [洞穴地图] 洞穴模式前台缓冲 (渲染线程读取)
extern mce::Color g_caveColors[MAP_DATA_SIZE][MAP_DATA_SIZE];
extern float g_caveHeights[MAP_DATA_SIZE][MAP_DATA_SIZE];

// 记录最后一次生成贴图时的绝对中心坐标
inline int g_lastRenderX = 0;
inline int g_lastRenderZ = 0;

// [小地图极速装载核心] 核心视野半径 (80 格已完整覆盖所有缩放下的小地图圆形视口)
constexpr int SCAN_INNER_RADIUS = 80;

// 小地图烘焙与推送到 GPU 的全局纹理字节缓冲及中心坐标
inline uint8_t g_textureData[MAP_DATA_SIZE * MAP_DATA_SIZE * 4];
inline float g_textureCenterX = 0.0f;
inline float g_textureCenterZ = 0.0f;
inline std::atomic<bool> g_textureReadyToUpload{false};

