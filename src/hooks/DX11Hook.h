#pragma once
#include <windows.h>
#include <ctime>
#include <vector>
#include <filesystem>
// 定义这些宏，避免与 LeviLamina 中的重复定义冲突
#define D3D12_FEATURE_DATA_D3D12_OPTIONS  D3D12_FEATURE_DATA_D3D12_OPTIONS_LEGACY
#define D3D12_FEATURE_DATA_ARCHITECTURE  D3D12_FEATURE_DATA_ARCHITECTURE_LEGACY
#define D3D12_RAYTRACING_GEOMETRY_DESC  D3D12_RAYTRACING_GEOMETRY_DESC_LEGACY
// 包含我们需要的 DX 头文件
#include <d3d11.h>
#include <d3d12.h>
#include <d3d11on12.h>
#include <dxgi1_4.h>
// 取消这些宏定义，避免后续问题
#undef D3D12_FEATURE_DATA_D3D12_OPTIONS
#undef D3D12_FEATURE_DATA_ARCHITECTURE
#undef D3D12_RAYTRACING_GEOMETRY_DESC
// 现在包含其他头文件
#include <imgui.h>
#include <imgui_internal.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx11.h>
#include <ll/api/memory/Hook.h>
#include <MinHook.h>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <filesystem>
#include "hooks/PlayerHook.h"
#include "state/MapCacheManager.h"
#include "state/WaypointManager.h"
#include "state/DeathPointManager.h"
#include "state/LanguageManager.h"
#include "state/SeedMapIconAtlas.h"
#include "state/SeedMapManager.h"
#include "render/EntityIconManager.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace DX11Hook {
    
    // 【全局 UI 缩放】基准样式与状态缓存
    inline ImGuiStyle g_baseImGuiStyle;
    inline bool g_baseStyleSaved = false;
    inline float g_lastAppliedUIScale = -1.0f;

    // 【极致平滑引擎】供全局调用的亚像素平滑坐标
    inline float g_smoothPX = 0.0f;
    inline float g_smoothPZ = 0.0f;

    // 大地图转到坐标输入与靶心标记状态
    inline char s_gotoXBuf[64] = "";
    inline char s_gotoZBuf[64] = "";
    inline bool s_gotoTargetActive = false;
    inline float s_gotoTargetX = 0.0f;
    inline float s_gotoTargetZ = 0.0f;
    inline int s_gotoTargetDim = 0;
    inline float s_gotoTargetTimer = 0.0f;

    inline bool ParseGotoCoords(const char* xStr, const char* zStr, float& outX, float& outZ) {
        auto extractNums = [](const char* s) {
            std::vector<float> res;
            if (!s) return res;
            const char* p = s;
            while (*p) {
                if ((*p >= '0' && *p <= '9') || (*p == '-' && (p[1] >= '0' && p[1] <= '9'))) {
                    char* endPtr = nullptr;
                    float val = std::strtof(p, &endPtr);
                    if (endPtr > p) {
                        res.push_back(val);
                        p = endPtr;
                        continue;
                    }
                }
                p++;
            }
            return res;
        };

        std::vector<float> xNums = extractNums(xStr);
        std::vector<float> zNums = extractNums(zStr);

        // 情况1：X 输入框中粘贴了多个数字（例如 "100 200" 或 "100 64 200" 或 "/tp 100 64 200"）
        if (xNums.size() >= 2) {
            outX = xNums[0];
            outZ = (xNums.size() >= 3) ? xNums[2] : xNums[1];
            return true;
        }

        // 情况2：常规输入，两框各有数字或至少一个框有有效输入
        if (!xNums.empty() && !zNums.empty()) {
            outX = xNums[0];
            outZ = zNums[0];
            return true;
        }

        if (!xNums.empty() && zNums.empty()) {
            outX = xNums[0];
            outZ = 0.0f;
            return true;
        }

        if (xNums.empty() && !zNums.empty()) {
            outX = 0.0f;
            outZ = zNums[0];
            return true;
        }

        return false;
    }

    inline ImVec4 OreColor(int r, int g, int b, int a = 255) {
        return ImVec4(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
    }

    enum class OreButtonKind { Default, Primary, Success, Warning, Danger };

    inline void PushOreButtonStyle(OreButtonKind kind) {
        ImVec4 base = OreColor(58, 63, 67);
        ImVec4 hover = OreColor(74, 81, 86);
        ImVec4 active = OreColor(48, 53, 57);
        if (kind == OreButtonKind::Primary) { base = OreColor(40, 122, 173); hover = OreColor(54, 146, 199); active = OreColor(30, 96, 140); }
        else if (kind == OreButtonKind::Success) { base = OreColor(56, 139, 80); hover = OreColor(68, 166, 96); active = OreColor(43, 112, 64); }
        else if (kind == OreButtonKind::Warning) { base = OreColor(158, 118, 38); hover = OreColor(188, 143, 50); active = OreColor(130, 95, 28); }
        else if (kind == OreButtonKind::Danger) { base = OreColor(168, 58, 56); hover = OreColor(202, 72, 69); active = OreColor(132, 42, 41); }
        ImGui::PushStyleColor(ImGuiCol_Button, base);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hover);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
    }

    inline void PopOreButtonStyle() {
        ImGui::PopStyleColor(3);
    }

    inline void OreTag(const char* text, ImVec4 color) {
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("[%s]", text);
        ImGui::PopStyleColor();
    }

    inline std::string FormatDeathTime(long long timestamp) {
        std::time_t raw = static_cast<std::time_t>(timestamp);
        std::tm tm{};
        if (localtime_s(&tm, &raw) != 0) return "-";
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        return buf;
    }

    inline const char* DimensionText(int dimensionId) {
        switch (dimensionId) {
        case 0: return LanguageManager::GetText("DIM_OVERWORLD");
        case 1: return LanguageManager::GetText("DIM_NETHER");
        case 2: return LanguageManager::GetText("DIM_END");
        default: return LanguageManager::GetText("DIM_UNKNOWN");
        }
    }

    inline bool IsDeathPointConverted(const DeathPoint& point) {
        std::lock_guard<std::mutex> lock(WaypointManager::g_wpMutex);
        // 已记录转换路径点 ID 时，以该路径点是否仍存在为准：
        // 路径点被删除（含批量删除）后自动恢复“转为路径点”按钮；若撤销删除恢复，按钮重新禁用
        if (point.converted && !point.convertedWaypointId.empty()) {
            for (const auto& wp : WaypointManager::g_waypoints) {
                if (wp.id == point.convertedWaypointId) return true;
            }
            return false;
        }
        // 兼容旧存档（无关联 ID）：按命名前缀 + 坐标 + 维度动态匹配
        std::string expectedName = std::string(LanguageManager::GetText("DEATH_POINT_WP_PREFIX")) + " " + FormatDeathTime(point.timestamp);
        for (const auto& wp : WaypointManager::g_waypoints) {
            if (wp.dimId == point.dimensionId && wp.x == point.x && wp.y == point.y && wp.z == point.z) {
                if (wp.name == expectedName) {
                    return true;
                }
            }
        }
        return false;
    }

    inline void DbgLog(const char* msg, HRESULT hr = 0) {
        std::ofstream out("MapMod_Debug.txt", std::ios::app);
        if (hr != 0) {
            char hexBuf[32]; snprintf(hexBuf, sizeof(hexBuf), " (HR: 0x%08X)", hr);
            out << msg << hexBuf << "\n";
        } else {
            out << msg << "\n";
        }
    }

    typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
    typedef HRESULT(__stdcall* Present1_t)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
    typedef HRESULT(__stdcall* ResizeBuffers_t)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
    typedef void(__stdcall* ExecuteCommandLists_t)(ID3D12CommandQueue*, UINT, ID3D12CommandList* const*);

    inline Present_t oPresent = nullptr;
    inline Present1_t oPresent1 = nullptr;
    inline ResizeBuffers_t oResizeBuffers = nullptr;
    inline ExecuteCommandLists_t oExecuteCommandLists = nullptr;

    inline ID3D11Device* g_pd3dDevice = nullptr;
    inline ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
    inline ID3D11On12Device* g_d3d11On12Device = nullptr;
    inline ID3D12CommandQueue* g_pGameCommandQueue = nullptr;

    inline HWND g_hWnd = nullptr;
    inline WNDPROC oWndProc = nullptr;
    inline bool g_imguiInitialized = false;

    // [性能·核心优化] D3D11On12 包装资源缓存
    // 原实现每帧调用 CreateWrappedResource + CreateRenderTargetView，这两个是极其昂贵的
    // GPU 内存操作（各约 0.1-0.5ms），且伴随内核态切换。在双缓冲/三缓冲交换链下，
    // 后备缓冲索引在 0..N-1 间循环，因此缓存每个索引对应的包装资源 + RTV，
    // 仅在首次遇到或 ResizeBuffers 后重建，消除 99%+ 的每帧创建开销。
    // 同时缓存 IDXGISwapChain3 指针，避免每帧 QueryInterface。
    inline IDXGISwapChain3* g_cachedSwapChain3 = nullptr;
    inline ID3D11Resource* g_cachedWrappedBuffers[8] = {};
    inline ID3D11RenderTargetView* g_cachedRTVs[8] = {};
    inline UINT g_cachedBufferCount = 0;
    inline void InvalidateCachedWrappedResources() {
        for (UINT i = 0; i < g_cachedBufferCount && i < 8; ++i) {
            if (g_cachedRTVs[i]) { g_cachedRTVs[i]->Release(); g_cachedRTVs[i] = nullptr; }
            if (g_cachedWrappedBuffers[i]) { g_cachedWrappedBuffers[i]->Release(); g_cachedWrappedBuffers[i] = nullptr; }
        }
        g_cachedBufferCount = 0;
        if (g_cachedSwapChain3) { g_cachedSwapChain3->Release(); g_cachedSwapChain3 = nullptr; }
    }

    inline ID3D11Texture2D* g_mapTexture = nullptr;
    inline ID3D11ShaderResourceView* g_mapTextureView = nullptr;

    inline std::unordered_map<uint64_t, ID3D11Texture2D*> g_regionTextures;
    inline std::unordered_map<uint64_t, ID3D11ShaderResourceView*> g_regionSRVs;

    // ==========================================
    // [另辟蹊径] Windows Raw Input 硬件级欺骗器
    // ==========================================
    typedef UINT(WINAPI* PGETRAWINPUTDATA_HOOK)(HRAWINPUT, UINT, LPVOID, PUINT, UINT);
    inline PGETRAWINPUTDATA_HOOK oGetRawInputData = nullptr;

    inline UINT WINAPI hkGetRawInputData(HRAWINPUT hRawInput, UINT uiCommand, LPVOID pData, PUINT pcbSize, UINT cbSizeHeader) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oGetRawInputData) return oGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
            return 0;
        }
        if (MapRenderState::IsUIActive()) {
            return (UINT)-1;
        }
        if (oGetRawInputData) return oGetRawInputData(hRawInput, uiCommand, pData, pcbSize, cbSizeHeader);
        return 0;
    }

    // ==========================================
    // [终极防御] 拦截 GetRawInputBuffer 与异步按键状态 (杀灭侧键)
    // ==========================================
    typedef UINT(WINAPI* PGETRAWINPUTBUFFER_HOOK)(PRAWINPUT, PUINT, UINT);
    inline PGETRAWINPUTBUFFER_HOOK oGetRawInputBuffer = nullptr;
    inline UINT WINAPI hkGetRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oGetRawInputBuffer) return oGetRawInputBuffer(pData, pcbSize, cbSizeHeader);
            return 0;
        }
        if (MapRenderState::IsUIActive()) return (UINT)-1;
        if (oGetRawInputBuffer) return oGetRawInputBuffer(pData, pcbSize, cbSizeHeader);
        return 0;
    }

    typedef SHORT(WINAPI* PGETASYNCKEYSTATE_HOOK)(int);
    inline PGETASYNCKEYSTATE_HOOK oGetAsyncKeyState = nullptr;
    typedef SHORT(WINAPI* PGETKEYSTATE_HOOK)(int);
    inline PGETKEYSTATE_HOOK oGetKeyState = nullptr;

    // 获取底层真实物理按键状态 (直接调用未挂钩的原生 API，彻底避开 UI 激活时的输入屏蔽)
    inline SHORT ReadPhysicalKeyState(int vKey) {
        if (oGetAsyncKeyState) {
            SHORT s = oGetAsyncKeyState(vKey);
            if (s & 0x8000) return s;
        }
        if (oGetKeyState) {
            SHORT s = oGetKeyState(vKey);
            if (s & 0x8000) return s;
        }
        SHORT s = GetAsyncKeyState(vKey);
        if (s & 0x8000) return s;
        return GetKeyState(vKey);
    }

    inline SHORT WINAPI hkGetAsyncKeyState(int vKey) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oGetAsyncKeyState) return oGetAsyncKeyState(vKey);
            return 0;
        }
        // 快捷键重绑监听状态下，放行修饰键检测
        if (MapRenderState::g_listeningHotkey != nullptr) {
            if (vKey == VK_CONTROL || vKey == VK_LCONTROL || vKey == VK_RCONTROL ||
                vKey == VK_SHIFT   || vKey == VK_LSHIFT   || vKey == VK_RSHIFT   ||
                vKey == VK_MENU    || vKey == VK_LMENU    || vKey == VK_RMENU) {
                if (oGetAsyncKeyState) return oGetAsyncKeyState(vKey);
                return 0;
            }
        }
        int holdKey = MapRenderState::g_hotkeys.holdEntities.key;
        if (MapRenderState::IsUIActive() && vKey != VK_F11 && (holdKey == 0 || vKey != holdKey)) {
            return 0;
        }
        if (oGetAsyncKeyState) return oGetAsyncKeyState(vKey);
        return 0;
    }

    inline SHORT WINAPI hkGetKeyState(int vKey) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oGetKeyState) return oGetKeyState(vKey);
            return 0;
        }
        // 快捷键重绑监听状态下，放行修饰键检测
        if (MapRenderState::g_listeningHotkey != nullptr) {
            if (vKey == VK_CONTROL || vKey == VK_LCONTROL || vKey == VK_RCONTROL ||
                vKey == VK_SHIFT   || vKey == VK_LSHIFT   || vKey == VK_RSHIFT   ||
                vKey == VK_MENU    || vKey == VK_LMENU    || vKey == VK_RMENU) {
                if (oGetKeyState) return oGetKeyState(vKey);
                return 0;
            }
        }
        int holdKey = MapRenderState::g_hotkeys.holdEntities.key;
        if (MapRenderState::IsUIActive() && vKey != VK_F11 && (holdKey == 0 || vKey != holdKey)) {
            return 0;
        }
        if (oGetKeyState) return oGetKeyState(vKey);
        return 0;
    }

    inline void RefreshRadarEntitySnapshot(
        std::vector<RadarEntity>& cachedEntities,
        std::string& cachedLocalPlayerUuid,
        uint64_t& cachedGeneration
    ) {
        const uint64_t publishedGeneration = g_radarGeneration.load(std::memory_order_acquire);
        if (publishedGeneration == cachedGeneration) return;
        std::lock_guard<std::mutex> lock(g_radarMutex);
        cachedEntities = g_radarEntities;
        cachedLocalPlayerUuid = g_localPlayerUuid;
        cachedGeneration = publishedGeneration;
    }

    typedef BOOL(WINAPI* PGETCURSORPOS_HOOK)(LPPOINT);
    inline PGETCURSORPOS_HOOK oGetCursorPos = nullptr;
    inline BOOL WINAPI hkGetCursorPos(LPPOINT lpPoint) {
        if (oGetCursorPos) return oGetCursorPos(lpPoint);
        return FALSE;
    }

    typedef BOOL(WINAPI* PSETCURSORPOS_HOOK)(int, int);
    inline PSETCURSORPOS_HOOK oSetCursorPos = nullptr;
    inline BOOL WINAPI hkSetCursorPos(int X, int Y) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oSetCursorPos) return oSetCursorPos(X, Y);
            return FALSE;
        }
        // 【修复大地图鼠标横跳】UI 激活时，游戏底层仍会用 raw input 持续把光标
        // 锁回屏幕中心 (SetCursorPos 到中心)，这会生成 WM_MOUSEMOVE 使 ImGui 的
        // io.MousePos 在「用户真实位置」与「屏幕中心」之间反复横跳，表现为鼠标乱晃。
        // 此时 UI 已吞掉所有鼠标/键盘输入，游戏视角本就不会转动，故直接拦截该调用，
        // 让 ImGui 只接收用户真实移动，光标不再被强行拉回中心。
        if (MapRenderState::IsUIActive()) {
            return TRUE;
        }
        if (oSetCursorPos) return oSetCursorPos(X, Y);
        return FALSE;
    }

    // [性能] 持久化烘焙线程变量声明 (必须在 ShutdownImGuiAndBuffers 之前声明)
    inline std::atomic<bool> g_textureBaking{false};
    inline std::mutex g_bakingMutex;
    inline std::condition_variable g_bakingCV;
    inline std::atomic<bool> g_bakingRequest{false};
    inline std::atomic<bool> g_bakingExit{false};
    inline std::thread* g_bakingThread = nullptr;

    inline void ShutdownImGuiAndBuffers() {
        // [性能] 通知持久化烘焙线程退出并等待其结束
        if (g_bakingThread) {
            g_bakingExit.store(true);
            g_bakingCV.notify_one();
            if (g_bakingThread->joinable()) g_bakingThread->join();
            delete g_bakingThread;
            g_bakingThread = nullptr;
            g_bakingExit.store(false);
            g_bakingRequest.store(false);
            g_textureBaking.store(false);
        }
        InvalidateCachedWrappedResources();
        for(auto& p : g_regionSRVs) if(p.second) p.second->Release();
        g_regionSRVs.clear();
        for(auto& p : g_regionTextures) if(p.second) p.second->Release();
        g_regionTextures.clear();
        EntityIconManager::ReleaseAll();
        if (g_imguiInitialized) {
            if (g_mapTextureView) { g_mapTextureView->Release(); g_mapTextureView = nullptr; }
            if (g_mapTexture) { g_mapTexture->Release(); g_mapTexture = nullptr; }
            ImGui_ImplDX11_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            g_imguiInitialized = false;
        }
        if (g_pd3dDeviceContext) {
            ID3D11RenderTargetView* nullRTV = nullptr;
            g_pd3dDeviceContext->OMSetRenderTargets(1, &nullRTV, NULL);
            g_pd3dDeviceContext->ClearState();
            g_pd3dDeviceContext->Flush();
        }
        if (g_d3d11On12Device) { g_d3d11On12Device->Release(); g_d3d11On12Device = nullptr; }
        if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
        if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
        if (g_pGameCommandQueue) { g_pGameCommandQueue->Release(); g_pGameCommandQueue = nullptr; }
    }

    // 【关闭期清理】由 ChiyanMap::disable() 在 unregisterAllHooks() 之后调用。
    // 职责：(1) 等待在飞钩子回调观察到 g_isShuttingDown 并退出；(2) 卸载 DX11Hook::init() 安装的
    // MinHook 钩子（Present/Present1/ResizeBuffers/ExecuteCommandLists + user32 输入 API）；
    // (3) 恢复游戏原始 WndProc；(4) 释放所有 D3D11/D3D12/ImGui 资源。
    // 此前这些资源在游戏退出时从未被显式释放，是 0xC0000005 退出崩溃的根因。
    inline void shutdown() {
        // g_isShuttingDown 已由 ChiyanMap::disable() 入口处置位；此处等待在飞回调退出。
        // 钩子入口处的 g_isShuttingDown 检查会让新一帧的 Present/Update 直接 pass-through，
        // 等待 30ms 足以让上一帧的 RenderImGui（约 5-15ms）走完。
        Sleep(30);

        // 恢复游戏原始窗口过程，防止窗口销毁期间 WndProcHook 访问已释放的 ImGui 上下文
        if (oWndProc && g_hWnd) {
            SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
            oWndProc = nullptr;
        }

        // 释放 D3D11/D3D12/ImGui 资源（包括停止持久化烘焙线程）
        ShutdownImGuiAndBuffers();
    }

    inline HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oResizeBuffers) return oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
            return S_OK;
        }
        // [性能] ResizeBuffers 会使所有后备缓冲失效，必须释放缓存的包装资源 + RTV，
        // 否则下一帧 GetBuffer 返回的新资源与缓存的旧包装资源不匹配 → 渲染到已释放的缓冲 → 崩溃。
        InvalidateCachedWrappedResources();
        if (g_imguiInitialized) {
            ImGui_ImplDX11_InvalidateDeviceObjects();
        }

        HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

        if (SUCCEEDED(hr) && g_imguiInitialized) {
            ImGui_ImplDX11_CreateDeviceObjects();
        }
        return hr;
    }

    inline void __stdcall hkExecuteCommandLists(ID3D12CommandQueue* pQueue, UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oExecuteCommandLists) oExecuteCommandLists(pQueue, NumCommandLists, ppCommandLists);
            return;
        }
        if (!g_pGameCommandQueue) {
            D3D12_COMMAND_QUEUE_DESC desc = pQueue->GetDesc();
            if (desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT) {
                g_pGameCommandQueue = pQueue;
                g_pGameCommandQueue->AddRef();
            }
        }
        if (oExecuteCommandLists) oExecuteCommandLists(pQueue, NumCommandLists, ppCommandLists);
    }

    // 【原生独立输入系统】悬浮宿主模式，归属于游戏窗口但无跨线程死锁
    namespace NativeIME {
        inline std::atomic<bool> isTyping{false};
        inline char* targetBuffer = nullptr;
        inline size_t targetBufferSize = 0;
        inline char localBuffer[256] = {0};
        inline WNDPROC oEditProc = nullptr;
        inline HWND hImeWnd = nullptr;

        inline void Close() {
            if (isTyping.load() && hImeWnd) {
                PostMessage(hImeWnd, WM_CLOSE, 0, 0);
                isTyping = false;
            }
        }

        inline LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
            // 【修复系统快捷键】将 F1~F24 快捷键透传给游戏窗口 (例如 F11 全屏切换)
            if (msg == WM_KEYDOWN || msg == WM_KEYUP || msg == WM_SYSKEYDOWN || msg == WM_SYSKEYUP) {
                if (wParam >= VK_F1 && wParam <= VK_F24) {
                    PostMessage(g_hWnd, msg, wParam, lParam);
                    return 0; // 拦截掉，防止发给文本框产生无效音效
                }
            }
            if (msg == WM_KEYDOWN) {
                if (wParam == VK_RETURN) { // 回车确认
                    NativeIME::Close();
                    return 0;
                } else if (wParam == VK_ESCAPE) { // Esc取消
                    NativeIME::Close();
                    return 0;
                }
            }
            // 【修复光标被隐形/覆盖】Windows 默认的文本光标 (I-Beam) 是纯黑色细线，在深灰背景下会完全隐形！
            // 这里我们强制将鼠标悬浮在输入框时也显示为系统的白色标准箭头，确保它在最顶层绝对可见！
            if (msg == WM_SETCURSOR) {
                SetCursor(LoadCursor(NULL, IDC_ARROW));
                return TRUE;
            }
            return CallWindowProc(oEditProc, hwnd, msg, wParam, lParam);
        }

        inline void Open(char* buf, size_t bufSize, const char* title) {
            if (isTyping.load()) {
                Close();
                if (targetBuffer == buf) return; // 再次点击同一按钮则仅仅是关闭
            }
            isTyping = true;
            targetBuffer = buf;
            targetBufferSize = bufSize;
            strncpy_s(localBuffer, 256, buf, _TRUNCATE);

            std::thread([]() {
                WNDCLASSW wc = {0};
                wc.lpfnWndProc = [](HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) -> LRESULT {
                    if (msg == WM_CREATE) {
                        // 文本框右侧留出 30 像素给关闭按钮 (X)
                        HWND hEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_LEFT,
                            10, 10, 250, 20, hwnd, (HMENU)1, NULL, NULL);
                        SendMessageA(hEdit, WM_SETTEXT, 0, (LPARAM)localBuffer);
                        oEditProc = (WNDPROC)SetWindowLongPtr(hEdit, GWLP_WNDPROC, (LONG_PTR)EditProc);
                        
                        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
                        SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
                    } else if (msg == WM_COMMAND) {
                        // 【核心优化：实时同步】只要输入框的内容发生任何变更，瞬间同步至目标 ImGui 缓冲
                        if (HIWORD(wParam) == EN_CHANGE) {
                            HWND hEdit = (HWND)lParam;
                            GetWindowTextA(hEdit, localBuffer, 256);
                            if (targetBuffer) {
                                strncpy_s(targetBuffer, targetBufferSize, localBuffer, _TRUNCATE);
                            }
                        }
                    } else if (msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORSTATIC) {
                        HDC hdc = (HDC)wParam;
                        SetTextColor(hdc, RGB(240, 240, 240)); 
                        SetBkColor(hdc, RGB(35, 35, 35));      
                        static HBRUSH hBrush = CreateSolidBrush(RGB(35, 35, 35));
                        return (LRESULT)hBrush;
                    } else if (msg == WM_PAINT) {
                        PAINTSTRUCT ps;
                        HDC hdc = BeginPaint(hwnd, &ps);
                        RECT rc; GetClientRect(hwnd, &rc);
                        HBRUSH bg = CreateSolidBrush(RGB(35, 35, 35));
                        FillRect(hdc, &rc, bg);
                        DeleteObject(bg);
                        
                        // 绘制边框
                        HPEN pen = CreatePen(PS_SOLID, 2, RGB(80, 80, 80));
                        HGDIOBJ oldPen = SelectObject(hdc, pen);
                        MoveToEx(hdc, 0, 0, NULL);
                        LineTo(hdc, rc.right, 0);
                        LineTo(hdc, rc.right, rc.bottom);
                        LineTo(hdc, 0, rc.bottom);
                        LineTo(hdc, 0, 0);
                        SelectObject(hdc, oldPen);
                        DeleteObject(pen);
                        
                        // 绘制关闭按钮 (X)
                        SetBkMode(hdc, TRANSPARENT);
                        SetTextColor(hdc, RGB(220, 80, 80));
                        RECT rcX = { rc.right - 25, 0, rc.right, rc.bottom };
                        DrawTextW(hdc, L"X", -1, &rcX, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

                        EndPaint(hwnd, &ps);
                        return 0;
                    } else if (msg == WM_LBUTTONDOWN) {
                        int x = LOWORD(lParam);
                        int y = HIWORD(lParam);
                        RECT rc; GetClientRect(hwnd, &rc);
                        if (x >= rc.right - 30 && x <= rc.right && y >= 0 && y <= rc.bottom) {
                            NativeIME::Close();
                        }
                    } else if (msg == WM_SETCURSOR) {
                        // 【修复指针覆盖问题】强制此小窗口内显示标准箭头鼠标
                        SetCursor(LoadCursor(NULL, IDC_ARROW));
                        return TRUE;
                    } else if (msg == WM_CLOSE) {
                        isTyping = false;
                        PostQuitMessage(0);
                    }
                    return DefWindowProcW(hwnd, msg, wParam, lParam);
                };
                wc.hInstance = GetModuleHandle(NULL);
                wc.lpszClassName = L"NativeIMEWnd";
                RegisterClassW(&wc);

                RECT rcGame;
                if (g_hWnd) {
                    GetClientRect(g_hWnd, &rcGame);
                    ClientToScreen(g_hWnd, (LPPOINT)&rcGame.left);
                    ClientToScreen(g_hWnd, (LPPOINT)&rcGame.right);
                } else {
                    rcGame = {0, 0, 1920, 1080}; 
                }
                int w = 300; int h = 40;
                int x = rcGame.left + (rcGame.right - rcGame.left - w) / 2;
                int y = rcGame.top + (rcGame.bottom - rcGame.top - h) / 2;

                HWND hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"NativeIMEWnd", L"",
                    WS_POPUP, x, y, w, h, g_hWnd, NULL, wc.hInstance, NULL);

                hImeWnd = hwnd;
                ShowWindow(hwnd, SW_SHOW);
                UpdateWindow(hwnd);
                
                SetForegroundWindow(hwnd);
                SetFocus(GetDlgItem(hwnd, 1)); // 精确聚焦到输入框子控件

                MSG msg;
                while (GetMessage(&msg, NULL, 0, 0)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                hImeWnd = nullptr;
                UnregisterClassW(L"NativeIMEWnd", wc.hInstance);
            }).detach();
        }
    }

    inline ImGuiKey VirtualKeyToImGuiKey(int vk) {
        if (vk >= '0' && vk <= '9') return (ImGuiKey)(ImGuiKey_0 + (vk - '0'));
        if (vk >= 'A' && vk <= 'Z') return (ImGuiKey)(ImGuiKey_A + (vk - 'A'));
        if (vk >= VK_F1 && vk <= VK_F12) return (ImGuiKey)(ImGuiKey_F1 + (vk - VK_F1));
        switch (vk) {
            case VK_SPACE: return ImGuiKey_Space;
            case VK_HOME: return ImGuiKey_Home;
            case VK_END: return ImGuiKey_End;
            case VK_PRIOR: return ImGuiKey_PageUp;
            case VK_NEXT: return ImGuiKey_PageDown;
            case VK_RETURN: return ImGuiKey_Enter;
            case VK_ESCAPE: return ImGuiKey_Escape;
            case VK_TAB: return ImGuiKey_Tab;
            case VK_BACK: return ImGuiKey_Backspace;
            case VK_INSERT: return ImGuiKey_Insert;
            case VK_DELETE: return ImGuiKey_Delete;
            case VK_LEFT: return ImGuiKey_LeftArrow;
            case VK_RIGHT: return ImGuiKey_RightArrow;
            case VK_UP: return ImGuiKey_UpArrow;
            case VK_DOWN: return ImGuiKey_DownArrow;
            case VK_OEM_3: return ImGuiKey_GraveAccent;
            case VK_OEM_MINUS: return ImGuiKey_Minus;
            case VK_OEM_PLUS: return ImGuiKey_Equal;
            case VK_OEM_4: return ImGuiKey_LeftBracket;
            case VK_OEM_6: return ImGuiKey_RightBracket;
            case VK_OEM_5: return ImGuiKey_Backslash;
            case VK_OEM_1: return ImGuiKey_Semicolon;
            case VK_OEM_7: return ImGuiKey_Apostrophe;
            case VK_OEM_COMMA: return ImGuiKey_Comma;
            case VK_OEM_PERIOD: return ImGuiKey_Period;
            case VK_OEM_2: return ImGuiKey_Slash;
            default: return ImGuiKey_None;
        }
    }

    inline LRESULT __stdcall WndProcHook(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        // 【硬核硬件光标守护】游戏底层维护了一个负数的隐藏层级，导致脱离小框后鼠标在游戏画面上彻底隐形。
        // 这里我们在主线程通过 ShowCursor(TRUE) 将层级强制拉回 >=0 的可见状态！
        bool isNativeTyping = NativeIME::isTyping.load();
        static bool s_wasNativeTyping = false;
        static int s_cursorForceCount = 0;
        if (isNativeTyping && !s_wasNativeTyping) {
            s_wasNativeTyping = true;
            s_cursorForceCount = 0;
            int currentCount = ShowCursor(TRUE);
            s_cursorForceCount++;
            while (currentCount < 0) {
                currentCount = ShowCursor(TRUE);
                s_cursorForceCount++;
            }
        } else if (!isNativeTyping && s_wasNativeTyping) {
            s_wasNativeTyping = false;
            while (s_cursorForceCount > 0) {
                ShowCursor(FALSE);
                s_cursorForceCount--;
            }
        }

        // 【防暂停黑科技】欺骗游戏底层，防止因为唤出原生的中文输入窗口导致游戏自动暂停！
        if (isNativeTyping) {
            if (uMsg == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) return 0;
            if (uMsg == WM_ACTIVATEAPP && wParam == FALSE) return 0;
            if (uMsg == WM_KILLFOCUS) return 0;
        }

        if (g_imguiInitialized && g_hasPlayer) {
            bool wasTextInputActive = NativeIME::isTyping.load() || (ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput);
            const auto& holdHk = MapRenderState::g_hotkeys.holdEntities;
            bool isHoldDown = (!holdHk.IsEmpty() && (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == (WPARAM)holdHk.key);

            // 当用户在全屏大地图按下实体头像热键（如 Tab）时：
            // 若此时并没有打开模态子窗口（如路径点管理、死亡点管理等），即使跳转坐标输入框曾被鼠标点击聚焦，
            // 也立即解除输入框聚焦，避免将光标锁定在坐标框中，确保热键能无阻碍切换大地图生物头像显示
            if (isHoldDown && MapRenderState::showBigMap && !MapRenderState::showWaypointUI && 
                !MapRenderState::showDeathPointUI && !MapRenderState::showBigMapSettings && 
                !MapRenderState::showMiniMapSettings && !MapRenderState::showHotkeySettings && 
                !MapRenderState::showCaveSettings && !MapRenderState::showExportPNGScreen && 
                !MapRenderState::showSeedMap) {
                if (ImGui::GetCurrentContext()) {
                    ImGui::ClearActiveID();
                }
                wasTextInputActive = false;
            }

            ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);
            
            bool isTextInputActive = wasTextInputActive;
            bool isTyping = isTextInputActive;
            if (!holdHk.IsEmpty()) {
                if ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == (WPARAM)holdHk.key) {
                    if (!isTextInputActive && MapRenderState::g_listeningHotkey == nullptr) {
                        uint8_t currentMods = 0;
                        if ((ReadPhysicalKeyState(VK_CONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_LCONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_RCONTROL) & 0x8000) != 0) currentMods |= MapRenderState::Hotkey::HK_MOD_CTRL;
                        if ((ReadPhysicalKeyState(VK_SHIFT) & 0x8000) != 0   || (ReadPhysicalKeyState(VK_LSHIFT) & 0x8000) != 0   || (ReadPhysicalKeyState(VK_RSHIFT) & 0x8000) != 0)   currentMods |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                        if ((ReadPhysicalKeyState(VK_MENU) & 0x8000) != 0    || (ReadPhysicalKeyState(VK_LMENU) & 0x8000) != 0    || (ReadPhysicalKeyState(VK_RMENU) & 0x8000) != 0)    currentMods |= MapRenderState::Hotkey::HK_MOD_ALT;
                        if (uMsg == WM_SYSKEYDOWN) currentMods |= MapRenderState::Hotkey::HK_MOD_ALT;
                        if (ImGui::GetCurrentContext()) {
                            if (ImGui::GetIO().KeyCtrl)  currentMods |= MapRenderState::Hotkey::HK_MOD_CTRL;
                            if (ImGui::GetIO().KeyShift) currentMods |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                            if (ImGui::GetIO().KeyAlt)   currentMods |= MapRenderState::Hotkey::HK_MOD_ALT;
                        }
                        if (currentMods == holdHk.modifiers) {
                            g_tabHeld = true;
                            if (MapRenderState::showBigMap) {
                                MapRenderState::bigMapShowEntities = !MapRenderState::bigMapShowEntities;
                                LanguageManager::SaveConfig();
                            }
                        }
                    }
                }
                if ((uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && (wParam == (WPARAM)holdHk.key ||
                    (wParam == VK_CONTROL && (holdHk.modifiers & MapRenderState::Hotkey::HK_MOD_CTRL)) ||
                    (wParam == VK_SHIFT   && (holdHk.modifiers & MapRenderState::Hotkey::HK_MOD_SHIFT)) ||
                    (wParam == VK_MENU    && (holdHk.modifiers & MapRenderState::Hotkey::HK_MOD_ALT)))) {
                    g_tabHeld = false;
                }
            } else {
                g_tabHeld = false;
            }
            if (uMsg == WM_KILLFOCUS) {
                g_tabHeld = false;
            }

            // [快捷键重绑捕获] 当用户在快捷键设置面板点击某个按键后，捕获单键或多键组合
            // 只要处于监听状态，无条件优先捕获键盘输入，必须在 IsUIActive() 吞噬逻辑之前执行
            if ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && MapRenderState::g_listeningHotkey != nullptr && MapRenderState::showHotkeySettings) {
                if (wParam == VK_ESCAPE) {
                    // Esc 取消重绑
                    MapRenderState::g_listeningHotkey = nullptr;
                    MapRenderState::g_listeningModifiers = 0;
                    return 1;
                } else if (wParam == VK_F11) {
                    // F11 (全屏切换) 透传，不作为可绑定按键
                    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
                } else if (wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL) {
                    MapRenderState::g_listeningModifiers |= MapRenderState::Hotkey::HK_MOD_CTRL;
                    return 1;
                } else if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) {
                    MapRenderState::g_listeningModifiers |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                    return 1;
                } else if (wParam == VK_MENU || wParam == VK_LMENU || wParam == VK_RMENU) {
                    MapRenderState::g_listeningModifiers |= MapRenderState::Hotkey::HK_MOD_ALT;
                    return 1;
                } else {
                    // 用户按下了组合主键！汇总修饰键
                    uint8_t mods = MapRenderState::g_listeningModifiers;
                    if ((ReadPhysicalKeyState(VK_CONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_LCONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_RCONTROL) & 0x8000) != 0) mods |= MapRenderState::Hotkey::HK_MOD_CTRL;
                    if ((ReadPhysicalKeyState(VK_SHIFT) & 0x8000) != 0   || (ReadPhysicalKeyState(VK_LSHIFT) & 0x8000) != 0   || (ReadPhysicalKeyState(VK_RSHIFT) & 0x8000) != 0)   mods |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                    if ((ReadPhysicalKeyState(VK_MENU) & 0x8000) != 0    || (ReadPhysicalKeyState(VK_LMENU) & 0x8000) != 0    || (ReadPhysicalKeyState(VK_RMENU) & 0x8000) != 0)    mods |= MapRenderState::Hotkey::HK_MOD_ALT;
                    if (uMsg == WM_SYSKEYDOWN) mods |= MapRenderState::Hotkey::HK_MOD_ALT;
                    if (ImGui::GetCurrentContext()) {
                        if (ImGui::GetIO().KeyCtrl)  mods |= MapRenderState::Hotkey::HK_MOD_CTRL;
                        if (ImGui::GetIO().KeyShift) mods |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                        if (ImGui::GetIO().KeyAlt)   mods |= MapRenderState::Hotkey::HK_MOD_ALT;
                    }

                    if (MapRenderState::g_listeningHotkey == &MapRenderState::g_hotkeys.openBigMap && (int)wParam == 0) {
                        // 严禁将 openBigMap 置为空
                    } else {
                        MapRenderState::g_listeningHotkey->key = (int)wParam;
                        MapRenderState::g_listeningHotkey->modifiers = mods;
                        LanguageManager::SaveConfig();
                    }
                    MapRenderState::g_listeningHotkey = nullptr;
                    MapRenderState::g_listeningModifiers = 0;
                }
                return 1;
            }
            // 监听状态下吸收修饰键松开消息，更新修饰键状态并防止误穿透
            if ((uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && MapRenderState::g_listeningHotkey != nullptr && MapRenderState::showHotkeySettings) {
                if (wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL) {
                    MapRenderState::g_listeningModifiers &= ~MapRenderState::Hotkey::HK_MOD_CTRL;
                    return 1;
                } else if (wParam == VK_SHIFT || wParam == VK_LSHIFT || wParam == VK_RSHIFT) {
                    MapRenderState::g_listeningModifiers &= ~MapRenderState::Hotkey::HK_MOD_SHIFT;
                    return 1;
                } else if (wParam == VK_MENU || wParam == VK_LMENU || wParam == VK_RMENU) {
                    MapRenderState::g_listeningModifiers &= ~MapRenderState::Hotkey::HK_MOD_ALT;
                    return 1;
                }
            }

            // [快捷键增强] Ctrl+Z 撤销 (仅在快捷键设置面板打开且非监听态时)
            // ImGui 已在上方收到此事件，此处 return 1 仅阻止 Z 键触发已绑定的快捷键
            if (!isTyping && (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && MapRenderState::showHotkeySettings && MapRenderState::g_listeningHotkey == nullptr) {
                bool ctrlDown = (ReadPhysicalKeyState(VK_CONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_LCONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_RCONTROL) & 0x8000) != 0;
                if (ctrlDown && wParam == 'Z') {
                    MapRenderState::g_hotkeyUndoRequested.store(true);
                    return 1;
                }
            }

            // 热键触发 (打字状态下失效，同时不影响原生聊天栏输入，支持多键组合)
            if (!isTyping && (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN)) {
                // 【修复聊天框内误触快捷键】热键仅在「准心游玩画面」生效：当游戏处于聊天栏、
                // 容器、暂停菜单等原生 UI 时 isInGameInputEnabled() 为 false，此时让按键直接
                // 穿透给游戏，不再触发打开大地图等热键，避免输入指令时误开地图。
                // 模组自身 UI 激活时仍允许热键 (如再次按 M 关闭大地图)。
                bool nativeScreenOpen = g_clientInstance && !g_clientInstance->isInGameInputEnabled();
                if (nativeScreenOpen && !MapRenderState::IsUIActive()) {
                    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
                }

                uint8_t currentMods = 0;
                if ((ReadPhysicalKeyState(VK_CONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_LCONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_RCONTROL) & 0x8000) != 0) currentMods |= MapRenderState::Hotkey::HK_MOD_CTRL;
                if ((ReadPhysicalKeyState(VK_SHIFT) & 0x8000) != 0   || (ReadPhysicalKeyState(VK_LSHIFT) & 0x8000) != 0   || (ReadPhysicalKeyState(VK_RSHIFT) & 0x8000) != 0)   currentMods |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                if ((ReadPhysicalKeyState(VK_MENU) & 0x8000) != 0    || (ReadPhysicalKeyState(VK_LMENU) & 0x8000) != 0    || (ReadPhysicalKeyState(VK_RMENU) & 0x8000) != 0)    currentMods |= MapRenderState::Hotkey::HK_MOD_ALT;
                if (uMsg == WM_SYSKEYDOWN) currentMods |= MapRenderState::Hotkey::HK_MOD_ALT;
                if (ImGui::GetCurrentContext()) {
                    if (ImGui::GetIO().KeyCtrl)  currentMods |= MapRenderState::Hotkey::HK_MOD_CTRL;
                    if (ImGui::GetIO().KeyShift) currentMods |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                    if (ImGui::GetIO().KeyAlt)   currentMods |= MapRenderState::Hotkey::HK_MOD_ALT;
                }

                auto matches = [&](const MapRenderState::Hotkey& hk) {
                    if (hk.IsEmpty()) return false;
                    return hk.key == (int)wParam && hk.modifiers == currentMods;
                };

                if (matches(MapRenderState::g_hotkeys.openBigMap) ||
                    matches(MapRenderState::g_hotkeys.openWaypointMgr) ||
                    matches(MapRenderState::g_hotkeys.openDeathPointMgr) ||
                    matches(MapRenderState::g_hotkeys.toggleMinimap) ||
                    matches(MapRenderState::g_hotkeys.toggleMinimapShape) ||
                    matches(MapRenderState::g_hotkeys.toggleMinimapRot) ||
                    matches(MapRenderState::g_hotkeys.toggleSeedMap) ||
                    (MapRenderState::enlargeMinimapToggle && matches(MapRenderState::g_hotkeys.enlargeMinimap)) ||
                    (MapRenderState::showBigMap && (matches(MapRenderState::g_hotkeys.centerCamera) || ((int)wParam == VK_HOME && currentMods == 0)))) {
                    CURSORINFO ci = {}; ci.cbSize = sizeof(CURSORINFO);
                    if (GetCursorInfo(&ci)) {
                        if (ci.flags == CURSOR_SHOWING && !MapRenderState::IsUIActive()) {
                            return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
                        }
                    }
                    if (matches(MapRenderState::g_hotkeys.openBigMap)) { // 大地图
                        NativeIME::Close();
                        MapRenderState::showBigMap = !MapRenderState::showBigMap;
                        if (MapRenderState::showBigMap) {
                            MapRenderState::bigMapViewDimensionId = -999;
                            MapRenderState::bigMapOffsetX = 0;
                            MapRenderState::bigMapOffsetZ = 0;
                        }
                    } else if (matches(MapRenderState::g_hotkeys.openWaypointMgr)) { // 路径点
                        NativeIME::Close();
                        MapRenderState::showWaypointUI = !MapRenderState::showWaypointUI;
                    } else if (matches(MapRenderState::g_hotkeys.openDeathPointMgr)) { // 死亡记录
                        NativeIME::Close();
                        MapRenderState::showDeathPointUI = !MapRenderState::showDeathPointUI;
                    } else if (matches(MapRenderState::g_hotkeys.toggleMinimap)) { // 小地图开关
                        MapRenderState::showMiniMap = !MapRenderState::showMiniMap;
                        LanguageManager::SaveConfig();
                    } else if (matches(MapRenderState::g_hotkeys.toggleMinimapShape)) { // 小地图形状
                        if (MapRenderState::showMiniMap) { MapRenderState::isSquareMap = !MapRenderState::isSquareMap; LanguageManager::SaveConfig(); }
                        else return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
                    } else if (matches(MapRenderState::g_hotkeys.toggleMinimapRot)) { // 旋转
                        if (MapRenderState::showMiniMap) { MapRenderState::rotateMiniMap = !MapRenderState::rotateMiniMap; LanguageManager::SaveConfig(); }
                        else return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
                    } else if (matches(MapRenderState::g_hotkeys.toggleSeedMap)) { // 种子地图
                        NativeIME::Close();
                        if (!MapRenderState::showBigMap) {
                            MapRenderState::showBigMap = true;
                            MapRenderState::bigMapViewDimensionId = -999;
                            MapRenderState::bigMapOffsetX = 0;
                            MapRenderState::bigMapOffsetZ = 0;
                            MapRenderState::showSeedMap = true;
                        } else {
                            MapRenderState::showSeedMap = !MapRenderState::showSeedMap;
                        }
                    } else if (matches(MapRenderState::g_hotkeys.enlargeMinimap)) { // 放大小地图 (切换模式)
                        MapRenderState::g_enlargeToggled = !MapRenderState::g_enlargeToggled;
                    } else if (MapRenderState::showBigMap && (matches(MapRenderState::g_hotkeys.centerCamera) || (int)wParam == VK_HOME)) { // 回到玩家位置
                        MapRenderState::CenterCameraOnViewDimension(g_smoothPX, g_smoothPZ);
                    }
                    return 1;
                }
            }

            // 模组 UI 激活时的拦截器（净化所有多余逻辑，安全防穿透）
            if (MapRenderState::IsUIActive()) {
                ClipCursor(NULL);
                if (uMsg == WM_KEYDOWN && wParam == VK_ESCAPE) {
                    NativeIME::Close();
                    if (MapRenderState::showHotkeySettings) {
                        // [Task 3] 优先关闭快捷键设置面板，清除监听状态
                        MapRenderState::showHotkeySettings = false;
                        MapRenderState::g_listeningHotkey = nullptr;
                    } else if (MapRenderState::showMiniMapPosSettings) {
                        MapRenderState::showMiniMapPosSettings = false;
                        MapRenderState::showBigMap = true;
                        MapRenderState::showMiniMapSettings = true;
                    } else if (MapRenderState::showMiniMapSettings) {
                        MapRenderState::showMiniMapSettings = false;
                    } else if (MapRenderState::showBigMapSettings) {
                        MapRenderState::showBigMapSettings = false;
                    } else if (MapRenderState::showCaveSettings) {
                        MapRenderState::showCaveSettings = false;
                    } else if (MapRenderState::showExportPNGScreen) {
                        if (MapRenderState::exportStage.load() != 1) {
                            MapRenderState::showExportPNGScreen = false;
                        }
                    } else if (MapRenderState::showSeedMap) {
                        MapRenderState::showSeedMap = false;
                    } else if (MapRenderState::showDeathPointUI) {
                        MapRenderState::showDeathPointUI = false;
                    } else {
                        MapRenderState::showBigMap = false;
                        MapRenderState::showWaypointUI = false; 
                        MapRenderState::showDeathPointUI = false;
                    }
                    return 1;
                }
                
                // 【回归本源：修复被我误删的防穿透代码】吞噬多余鼠标信号，完美阻断挥臂与音效
                if (uMsg == WM_INPUT || uMsg == WM_INPUT_DEVICE_CHANGE) return 1;
                if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST) return 1;
                if (uMsg >= 0x0240 && uMsg <= 0x0253) return 1; // WM_POINTERFIRST ~ WM_POINTERLAST (WM_POINTERDOWN 等)

                // 吞噬多余键盘按键，防止游戏内人物移动
                if (uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST && wParam != VK_F11) {
                    if (isTyping) {
                        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
                    }
                    return 1; 
                }
                
                if (uMsg == WM_SETCURSOR) { SetCursor(LoadCursor(NULL, IDC_ARROW)); return TRUE; }
            }
        }
        return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
    }

    inline void InitImGuiFonts(ImGuiIO& io) {
        io.Fonts->Clear();

        ImFontConfig config;
        config.OversampleH = 1;
        config.OversampleV = 1;

        std::string baseFont = "c:\\Windows\\Fonts\\segoeui.ttf";
        if (!std::filesystem::exists(baseFont)) baseFont = "c:\\Windows\\Fonts\\arial.ttf";

        if (std::filesystem::exists(baseFont)) {
            io.Fonts->AddFontFromFileTTF(baseFont.c_str(), 18.0f, &config, io.Fonts->GetGlyphRangesCyrillic());
            config.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(baseFont.c_str(), 18.0f, &config, io.Fonts->GetGlyphRangesVietnamese());
        } else {
            io.Fonts->AddFontDefault();
            config.MergeMode = true;
        }

        std::string cnHan = "c:\\Windows\\Fonts\\msyh.ttc";
        if (std::filesystem::exists(cnHan)) {
            config.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(cnHan.c_str(), 18.0f, &config, io.Fonts->GetGlyphRangesChineseFull());
        }

        std::string jpFont = "c:\\Windows\\Fonts\\meiryo.ttc";
        if (!std::filesystem::exists(jpFont)) jpFont = "c:\\Windows\\Fonts\\msgothic.ttc";
        if (std::filesystem::exists(jpFont)) {
            config.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(jpFont.c_str(), 18.0f, &config, io.Fonts->GetGlyphRangesJapanese());
        }

        std::string koFont = "c:\\Windows\\Fonts\\malgun.ttf";
        if (std::filesystem::exists(koFont)) {
            config.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(koFont.c_str(), 18.0f, &config, io.Fonts->GetGlyphRangesKorean());
        }

        std::string thFont = "c:\\Windows\\Fonts\\leelawdb.ttf";
        if (std::filesystem::exists(thFont)) {
            config.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(thFont.c_str(), 18.0f, &config, io.Fonts->GetGlyphRangesThai());
        }

        std::string symbolFont = "c:\\Windows\\Fonts\\seguisym.ttf";
        if (std::filesystem::exists(symbolFont)) {
            // 包含 Arrows 块 (U+2190-U+21FF，用于 ↺ 重置按钮) 与 Miscellaneous Symbols 块 (U+2600-U+26FF)
            static const ImWchar symbolRanges[] = { 0x2190, 0x21FF, 0x2600, 0x26FF, 0 };
            config.MergeMode = true;
            io.Fonts->AddFontFromFileTTF(symbolFont.c_str(), 18.0f, &config, symbolRanges);
        }
    }

    inline void InitMapTexture() {
        if (!g_pd3dDevice) return;
        D3D11_TEXTURE2D_DESC desc;
        ZeroMemory(&desc, sizeof(desc));
        desc.Width = MAP_DATA_SIZE; desc.Height = MAP_DATA_SIZE;
        desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE; desc.CPUAccessFlags = 0;

        if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, NULL, &g_mapTexture))) {
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
            ZeroMemory(&srvDesc, sizeof(srvDesc));
            srvDesc.Format = desc.Format; srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            if (FAILED(g_pd3dDevice->CreateShaderResourceView(g_mapTexture, &srvDesc, &g_mapTextureView))) {
                g_mapTexture->Release(); g_mapTexture = nullptr;
            }
        }
    }

    inline void BakingWorkerFunc() {
        // [性能] 使用 static 局部变量避免栈溢出 (三块缓冲合计约 6MB, 远超默认 1MB 栈大小)
        // 仅有一个持久化 worker 线程, 不会并发访问, static 安全。
        static mce::Color localColors[MAP_DATA_SIZE][MAP_DATA_SIZE];
        static float localHeights[MAP_DATA_SIZE][MAP_DATA_SIZE];
        static uint8_t bakedData[MAP_DATA_SIZE * MAP_DATA_SIZE * 4];

        while (true) {
            {
                std::unique_lock<std::mutex> lock(g_bakingMutex);
                g_bakingCV.wait(lock, [] { return g_bakingRequest.load() || g_bakingExit.load(); });
                if (g_bakingExit.load()) return;
                g_bakingRequest.store(false);
            }

            if (MapRenderState::g_isShuttingDown.load()) {
                g_textureBaking.store(false);
                continue;
            }

            float centerX, centerZ;
            {
                std::lock_guard<std::mutex> lock(g_mapDataMutex);
                std::memcpy(localColors, g_mapColors, sizeof(localColors));
                std::memcpy(localHeights, g_mapHeights, sizeof(localHeights));
                centerX = static_cast<float>(g_lastRenderX);
                centerZ = static_cast<float>(g_lastRenderZ);
            }

            for (int x = 0; x < MAP_DATA_SIZE; x++) {
                for (int z = 0; z < MAP_DATA_SIZE; z++) {
                    int index = (z * MAP_DATA_SIZE + x) * 4;
                    mce::Color col = localColors[x][z];

                    if (col.a > 0.01f) {
                        float currentY = localHeights[x][z];
                        float northY = currentY, westY = currentY, northWestY = currentY;
                        if (z > 0 && localColors[x][z - 1].a > 0.01f && std::abs(currentY - localHeights[x][z - 1]) < 64.0f) northY = localHeights[x][z - 1];
                        if (x > 0 && localColors[x - 1][z].a > 0.01f && std::abs(currentY - localHeights[x - 1][z]) < 64.0f) westY = localHeights[x - 1][z];
                        if (x > 0 && z > 0 && localColors[x - 1][z - 1].a > 0.01f && std::abs(currentY - localHeights[x - 1][z - 1]) < 64.0f) northWestY = localHeights[x - 1][z - 1];

                        float shade = MapRenderState::ComputeTerrainShading(
                            currentY, northY, westY, northWestY,
                            MapRenderState::terrainSlopes,
                            MapRenderState::terrainDepth,
                            (MapRenderState::currentDimensionId == 1 || MapRenderState::g_caveModeActive)
                        );
                        bakedData[index]     = (uint8_t)(std::clamp(col.r * shade, 0.0f, 1.0f) * 255.0f);
                        bakedData[index + 1] = (uint8_t)(std::clamp(col.g * shade, 0.0f, 1.0f) * 255.0f);
                        bakedData[index + 2] = (uint8_t)(std::clamp(col.b * shade, 0.0f, 1.0f) * 255.0f);
                        bakedData[index + 3] = (uint8_t)(col.a * 255.0f);
                    } else {
                        bakedData[index] = bakedData[index+1] = bakedData[index+2] = bakedData[index+3] = 0;
                    }
                }
            }

            {
                std::lock_guard<std::mutex> lock(g_mapDataMutex);
                std::memcpy(g_textureData, bakedData, sizeof(bakedData));
                g_textureCenterX = centerX;
                g_textureCenterZ = centerZ;
                g_textureReadyToUpload.store(true);
            }
            g_textureBaking.store(false);
        }
    }

    inline void UpdateMapTexture() {
        if (!g_pd3dDeviceContext || !g_mapTexture) return;

        // [关闭期安全] 烘焙线程在 shutdown() 50ms 等待窗口内可能仍在运行；若退出阶段已开始，
        // 不再启动新烘焙任务（防止 Detached 线程在进程静态析构阶段访问全局静态缓冲）。
        if (MapRenderState::g_isShuttingDown.load()) return;

        if (g_mapDataUpdated.load() && !g_textureBaking.load()) {
            g_textureBaking.store(true);
            g_mapDataUpdated.store(false);

            // [性能] 懒启动持久化烘焙线程，后续仅通过条件变量唤醒
            if (!g_bakingThread) {
                g_bakingThread = new std::thread(BakingWorkerFunc);
            }
            g_bakingRequest.store(true);
            g_bakingCV.notify_one();
        }

        if (g_textureReadyToUpload.load()) {
            std::lock_guard<std::mutex> lock(g_mapDataMutex);
            g_pd3dDeviceContext->UpdateSubresource(g_mapTexture, 0, NULL, g_textureData, MAP_DATA_SIZE * 4, 0);
            g_textureReadyToUpload.store(false);
        }
    }

    inline void DrawWaypointIcon(ImDrawList* draw_list, ImVec2 center, mce::Color color, const std::string& name, bool isEdge = false, float scaleMult = 1.0f, bool isTemp = false, bool isEnabled = true, const std::string& extraSubText = "") {
        float size = (isEdge ? 5.5f : 8.0f) * scaleMult; 
        int alpha = isEnabled ? 255 : 110;
        ImU32 col32 = IM_COL32((int)(color.r * 255.0f), (int)(color.g * 255.0f), (int)(color.b * 255.0f), alpha);
        ImU32 outline = isTemp ? IM_COL32(255, 230, 80, alpha) : IM_COL32(0, 0, 0, alpha); 
        
        ImVec2 pts[4] = {
            ImVec2(center.x, center.y - size),
            ImVec2(center.x + size, center.y),
            ImVec2(center.x, center.y + size),
            ImVec2(center.x - size, center.y)
        };
        
        draw_list->AddConvexPolyFilled(pts, 4, col32);
        draw_list->AddPolyline(pts, 4, outline, ImDrawFlags_Closed, isTemp ? 2.2f : 1.5f);
        
        if (!isEdge && !name.empty()) {
            ImFont* font = ImGui::GetFont();
            float fontSize = ImGui::GetFontSize();
            ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, name.c_str());
            ImVec2 textPos(center.x - textSize.x / 2.0f, center.y + size + 3.0f * MapRenderState::globalUIScale);
            draw_list->AddText(font, fontSize, ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, isEnabled ? 200 : 90), name.c_str(), NULL, 0.0f, NULL); 
            draw_list->AddText(font, fontSize, textPos, isTemp ? IM_COL32(100, 230, 255, alpha) : IM_COL32(255, 255, 255, alpha), name.c_str(), NULL, 0.0f, NULL); 

            if (!extraSubText.empty()) {
                float subFontSize = fontSize * 0.82f;
                ImVec2 subSize = font->CalcTextSizeA(subFontSize, FLT_MAX, 0.0f, extraSubText.c_str());
                ImVec2 subPos(center.x - subSize.x / 2.0f, textPos.y + textSize.y + 1.0f);
                draw_list->AddText(font, subFontSize, ImVec2(subPos.x + 1, subPos.y + 1), IM_COL32(0, 0, 0, isEnabled ? 180 : 80), extraSubText.c_str(), NULL, 0.0f, NULL);
                draw_list->AddText(font, subFontSize, subPos, IM_COL32(200, 230, 255, alpha), extraSubText.c_str(), NULL, 0.0f, NULL);
            }
        }
    }

    inline void DrawDeathPointIcon(ImDrawList* draw_list, ImVec2 center, const std::string& name, bool isEdge = false) {
        float size = isEdge ? 6.0f : 8.5f; 
        
        // 绘制高对比度红叉 ❌：外层黑色底线描边，内层鲜红实线
        draw_list->AddLine(ImVec2(center.x - size, center.y - size), ImVec2(center.x + size, center.y + size), IM_COL32(0, 0, 0, 255), 4.5f);
        draw_list->AddLine(ImVec2(center.x - size, center.y + size), ImVec2(center.x + size, center.y - size), IM_COL32(0, 0, 0, 255), 4.5f);
        draw_list->AddLine(ImVec2(center.x - size, center.y - size), ImVec2(center.x + size, center.y + size), IM_COL32(235, 45, 45, 255), 2.5f);
        draw_list->AddLine(ImVec2(center.x - size, center.y + size), ImVec2(center.x + size, center.y - size), IM_COL32(235, 45, 45, 255), 2.5f);
        
        if (!isEdge && !name.empty()) {
            ImFont* font = ImGui::GetFont();
            float fontSize = ImGui::GetFontSize();
            ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, name.c_str());
            ImVec2 textPos(center.x - textSize.x / 2.0f, center.y + size + 3.0f * MapRenderState::globalUIScale);
            draw_list->AddText(font, fontSize, ImVec2(textPos.x + 1, textPos.y + 1), IM_COL32(0, 0, 0, 220), name.c_str(), NULL, 0.0f, NULL); 
            draw_list->AddText(font, fontSize, textPos, IM_COL32(255, 120, 120, 255), name.c_str(), NULL, 0.0f, NULL); 
        }
    }

    inline ImU32 GetPlayerArrowColor(float alpha = 1.0f) {
        int a = (int)(255.0f * alpha);
        switch (MapRenderState::playerArrowColor) {
            case 1: return IM_COL32(245, 245, 245, a); // White
            case 2: return IM_COL32(50, 220, 60, a);   // Green
            case 3: return IM_COL32(50, 150, 255, a);  // Blue
            case 4: return IM_COL32(255, 220, 40, a);  // Yellow
            case 5: return IM_COL32(185, 75, 255, a);  // Purple
            case 6: return IM_COL32(40, 40, 40, a);    // Black
            case 7: return IM_COL32(40, 235, 225, a);  // Cyan
            case 0:
            default: return IM_COL32(220, 20, 20, a);  // Red
        }
    }

    inline void PointSamplerCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
        if (!g_pd3dDeviceContext) return;
        static ID3D11SamplerState* pPointSampler = nullptr;
        if (!pPointSampler) {
            D3D11_SAMPLER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            desc.MipLODBias = 0.0f;
            desc.MaxAnisotropy = 1;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.MinLOD = 0.0f;
            desc.MaxLOD = D3D11_FLOAT32_MAX;
            g_pd3dDevice->CreateSamplerState(&desc, &pPointSampler);
        }
        g_pd3dDeviceContext->PSSetSamplers(0, 1, &pPointSampler);
    }

    inline void LinearSamplerCallback(const ImDrawList* parent_list, const ImDrawCmd* cmd) {
        if (!g_pd3dDeviceContext) return;
        static ID3D11SamplerState* pLinearSampler = nullptr;
        if (!pLinearSampler) {
            D3D11_SAMPLER_DESC desc;
            ZeroMemory(&desc, sizeof(desc));
            desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
            desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
            desc.MipLODBias = 0.0f;
            desc.MaxAnisotropy = 1;
            desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
            desc.MinLOD = 0.0f;
            desc.MaxLOD = D3D11_FLOAT32_MAX;
            g_pd3dDevice->CreateSamplerState(&desc, &pLinearSampler);
        }
        g_pd3dDeviceContext->PSSetSamplers(0, 1, &pLinearSampler);
    }

    // ==========================================
    // 【极致平滑引擎】更新核心航位推测坐标
    // ==========================================
    inline void UpdateSmoothCamera() {
        static float s_lastSeenX = g_playerX;
        static float s_lastSeenZ = g_playerZ;
        static float s_velX = 0.0f;
        static float s_velZ = 0.0f;
        static auto s_lastUpdateTime = std::chrono::steady_clock::now();
        static auto s_lastFrameTime = std::chrono::steady_clock::now();

        // 当底层逻辑坐标更新时，计算真实物理速度
        if (std::abs(g_playerX - s_lastSeenX) > 0.001f || std::abs(g_playerZ - s_lastSeenZ) > 0.001f) {
            auto now = std::chrono::steady_clock::now();
            float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - s_lastUpdateTime).count();
            if (dt > 0.005f && dt < 1.0f) { // 过滤异常时间跳变
                s_velX = (g_playerX - s_lastSeenX) / dt;
                s_velZ = (g_playerZ - s_lastSeenZ) / dt;
            }
            s_lastSeenX = g_playerX;
            s_lastSeenZ = g_playerZ;
            s_lastUpdateTime = now;
        }

        // 超过 150ms 没收到坐标更新，判定玩家已彻底停下，强制阻断速度消除滑步
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::duration<float>>(now - s_lastUpdateTime).count() > 0.15f) {
            s_velX = 0.0f;
            s_velZ = 0.0f;
        }

        // [性能] 用 chrono 计算 delta time，不再依赖 ImGui::GetIO().DeltaTime，
        // 使此函数可在 ImGui 帧外调用（无 UI 时跳过整个 ImGui 渲染管线）。
        float frameDt = std::chrono::duration_cast<std::chrono::duration<float>>(now - s_lastFrameTime).count();
        s_lastFrameTime = now;
        if (frameDt > 0.1f) frameDt = 0.1f;

        // 初始化兜底
        if (g_smoothPX == 0.0f && g_smoothPZ == 0.0f) {
            g_smoothPX = g_playerX;
            g_smoothPZ = g_playerZ;
        }

        // 1. 根据物理速度连续推测坐标 (完全脱离 20Hz 阶梯感)
        g_smoothPX += s_velX * frameDt;
        g_smoothPZ += s_velZ * frameDt;

        // 2. 软弹簧纠偏：微弱拉向真实坐标，防止漂移误差累积
        g_smoothPX += (g_playerX - g_smoothPX) * 12.0f * frameDt;
        g_smoothPZ += (g_playerZ - g_smoothPZ) * 12.0f * frameDt;

        // 3. 传送/死亡瞬间断层强行复位
        if (std::abs(g_playerX - g_smoothPX) > 10.0f || std::abs(g_playerZ - g_smoothPZ) > 10.0f) {
            g_smoothPX = g_playerX;
            g_smoothPZ = g_playerZ;
            s_velX = 0.0f;
            s_velZ = 0.0f;
        }
    }

    // ==========================================
    // [小地图 UI 渲染引擎]
    // ==========================================
    inline void RenderImGuiMiniMap() {
        if (!MapRenderState::showMiniMap) return; 

        // 【原生界面避让系统】如果我们的模组界面未开启，但系统鼠标却处于显示状态
        // 意味着玩家正处于聊天栏、命令输入、背包或暂停菜单中，此时主动隐藏小地图以避免阻碍视线。
        // [性能] 每帧调用 GetCursorInfo 涉及系统调用开销不小（单次约 300~800ns）；
        // 通过 10 帧（约 166ms）节流调用，足够快于任何 GUI 打开/关闭反应延迟；
        // 同时用计数器放在函数入口做一次静态变量缓存，计数器静态即可避免了对每一帧都去调用。
        static int s_cursorCheckCounter = 0;
        static bool s_cursorShowingCached = false;
        if (!MapRenderState::IsUIActive()) {
            s_cursorCheckCounter = 9; // UI 激活时强制下帧立即重查，避免切换瞬间漏显示
        }
        if (++s_cursorCheckCounter >= 10) {
            s_cursorCheckCounter = 0;
            CURSORINFO ci = {};
            ci.cbSize = sizeof(CURSORINFO);
            if (GetCursorInfo(&ci)) {
                s_cursorShowingCached = (ci.flags == CURSOR_SHOWING);
            } else {
                s_cursorShowingCached = false;
            }
        }
        bool nativeScreenOpen = s_cursorShowingCached || (g_clientInstance && !g_clientInstance->isInGameInputEnabled());
        if (!MapRenderState::IsUIActive() && nativeScreenOpen) {
            return;
        }

        if (!g_mapTextureView) return;
        UpdateMapTexture();

        ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
        
        bool isEnlarged = !MapRenderState::IsUIActive() && (MapRenderState::enlargeMinimapToggle ? MapRenderState::g_enlargeToggled : MapRenderState::g_enlargeHeld.load());

        // 乘以小地图大小缩放因子，动态调整地图尺寸 (临时放大时半径乘以 1.75 并限制在屏幕以内)
        float displayW = ImGui::GetIO().DisplaySize.x;
        float displayH = ImGui::GetIO().DisplaySize.y;
        float fontSize = ImGui::GetFontSize();
        float IM_MAP_R = std::floor(135.0f * MapRenderState::miniMapScale * (isEnlarged ? 1.75f : 1.0f)); 
        IM_MAP_R = std::min(IM_MAP_R, std::min(displayW, displayH) * 0.45f);
        float IM_MAP_MARGIN = 20.0f;
        
        // 计算原生基础中心点
        float base_cx = std::floor(displayW - IM_MAP_MARGIN - IM_MAP_R);
        float base_cy = std::floor(IM_MAP_MARGIN + IM_MAP_R);
        
        // 应用偏移量
        float cx = base_cx + MapRenderState::miniMapOffsetX;
        float cy = base_cy + MapRenderState::miniMapOffsetY;

        // 【屏幕越界保护系统】计算边界并强行将地图锁死在屏幕内
        float min_cx = IM_MAP_MARGIN + IM_MAP_R;
        float max_cx = displayW - IM_MAP_MARGIN - IM_MAP_R;
        float min_cy = IM_MAP_MARGIN + IM_MAP_R;
        float max_cy = displayH - IM_MAP_MARGIN - IM_MAP_R;

        if (cx < min_cx) cx = min_cx;
        if (cx > max_cx) cx = max_cx;
        if (cy < min_cy) cy = min_cy;
        if (cy > max_cy) cy = max_cy;

        // 非放大状态下，反写回真实限制过的偏移值，确保拖动条数值与实际物理边界完美同步
        if (!isEnlarged) {
            MapRenderState::miniMapOffsetX = cx - base_cx;
            MapRenderState::miniMapOffsetY = cy - base_cy;
        }
        
        // 强制向下取整，防止 ImGui 渲染到亚像素网格导致 DX11 采样边缘发毛
        cx = std::floor(cx);
        cy = std::floor(cy);

        // [小地图可视化拖拽吸附系统] 处于“调整小地图布局”状态时，允许玩家直接按住鼠标左键在屏幕任意位置拖拽小地图
        static bool s_isDraggingMinimap = false;
        static ImVec2 s_dragStartMouse;
        static float s_dragStartOffsetX = 0.0f;
        static float s_dragStartOffsetY = 0.0f;

        if (MapRenderState::showMiniMapPosSettings) {
            ImGuiIO& io = ImGui::GetIO();
            float mDx = io.MousePos.x - cx;
            float mDy = io.MousePos.y - cy;
            float mDist = std::sqrt(mDx * mDx + mDy * mDy);

            // 绘制拖拽指示外光晕
            draw_list->AddCircle(ImVec2(cx, cy), IM_MAP_R + 6.0f, IM_COL32(80, 200, 255, 220), 48, 2.5f);
            draw_list->AddCircle(ImVec2(cx, cy), IM_MAP_R + 10.0f, IM_COL32(80, 200, 255, 100), 48, 1.2f);

            if (io.MouseDown[0]) {
                if (!s_isDraggingMinimap) {
                    if (mDist <= IM_MAP_R + 12.0f && !ImGui::IsAnyItemHovered() && !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) {
                        s_isDraggingMinimap = true;
                        s_dragStartMouse = io.MousePos;
                        s_dragStartOffsetX = MapRenderState::miniMapOffsetX;
                        s_dragStartOffsetY = MapRenderState::miniMapOffsetY;
                    }
                } else {
                    float deltaX = io.MousePos.x - s_dragStartMouse.x;
                    float deltaY = io.MousePos.y - s_dragStartMouse.y;
                    MapRenderState::miniMapOffsetX = s_dragStartOffsetX + deltaX;
                    MapRenderState::miniMapOffsetY = s_dragStartOffsetY + deltaY;
                }
            } else {
                s_isDraggingMinimap = false;
            }
        } else {
            s_isDraggingMinimap = false;
        }

        float pX = g_smoothPX;
        float pZ = g_smoothPZ;

        // 【极致防撕裂核心】UV 偏移只以最新的同步 Texture 中心为基准运算！
        float dx = pX - g_textureCenterX;
        float dz = pZ - g_textureCenterZ;

        float playerYaw = g_playerYaw;
        // 算出地图需要旋转的弧度：如果要将玩家朝向置于正北，则地图需反向旋转其 yaw 角
        float mapRotateRad = MapRenderState::rotateMiniMap ? -(playerYaw + 180.0f) * (3.14159265f / 180.0f) : 0.0f;
        float c_rot = std::cos(mapRotateRad);
        float s_rot = std::sin(mapRotateRad);

        // 运行时夹取为安全区间 [10, 400]，临时放大时同步扩展视野
        float ZOOM_RADIUS = std::clamp(MapRenderState::miniMapZoomRadius * (isEnlarged ? 1.75f : 1.0f), 10.0f, 400.0f);
        float uvR = ZOOM_RADIUS / MAP_DATA_SIZE;

        float drawRadius = IM_MAP_R;
        float uvDrawR = uvR;
        // 对于方形地图，如果开启旋转，为了避免边角露馅，渲染内容必须扩大 1.415 倍（正方形对角线长度）
        if (MapRenderState::isSquareMap && MapRenderState::rotateMiniMap) {
            drawRadius = IM_MAP_R * 1.415f;
            uvDrawR = uvR * 1.415f;
        }
        // 采样窗口半宽不能超过纹理半径, 否则窗口无法完整落入纹理 [0,1] 区间
        // (仅影响 方形+旋转+zoom>181 的极端配置, 视野略有缩小以换取采样安全)
        uvDrawR = std::min(uvDrawR, 0.5f);

        // [防越界条纹] 跑图时小地图出现"东西向移动 → 横条纹、南北向移动 → 竖条纹"的整屏伪影,
        // 根因是: 玩家跑图时扫描中心 (g_textureCenterX/Z, 上一次完成扫描的中心) 大幅滞后,
        // 玩家距离纹理中心超过 (MAP_DATA_RADIUS - ZOOM_RADIUS) 后, UV 采样窗口越出纹理 [0,1] 区间,
        // POINT 采样器的 CLAMP 寻址把纹理边缘"一列/一行"拉伸铺满整个小地图:
        //   - 东西向移动 → u 越界 → 边缘列 (南北向地形线) 拉伸 → 横条纹;
        //   - 南北向移动 → v 越界 → 边缘行 (东西向地形线) 拉伸 → 竖条纹。
        // (扫描侧已通过 ShiftScanGrid 跟随平移根治滞后; 此处钳制为最后防线。)
        // 修复: 将采样窗口中心钳制在 [uvDrawR, 1-uvDrawR], 保证窗口完整落在纹理内;
        // 同时推算"视图有效中心" (viewPX/viewPZ, 钳制后窗口中心对应的真实世界坐标),
        // 供雷达/路径点/玩家标记定位, 保证窗口被钳制时标记仍然地理精确。
        float maxOff = 0.5f - uvDrawR;
        float uOff = std::clamp(dx / (float)MAP_DATA_SIZE, -maxOff, maxOff);
        float vOff = std::clamp(dz / (float)MAP_DATA_SIZE, -maxOff, maxOff);
        float u = 0.5f + uOff;
        float v = 0.5f + vOff;
        float viewPX = g_textureCenterX + uOff * (float)MAP_DATA_SIZE;
        float viewPZ = g_textureCenterZ + vOff * (float)MAP_DATA_SIZE;

        ImVec2 uv0(u - uvDrawR, v - uvDrawR);
        ImVec2 uv1(u + uvDrawR, v + uvDrawR);
        ImVec2 drawMin(cx - drawRadius, cy - drawRadius);
        ImVec2 drawMax(cx + drawRadius, cy + drawRadius);
        ImVec2 mapMin(cx - IM_MAP_R, cy - IM_MAP_R);
        ImVec2 mapMax(cx + IM_MAP_R, cy + IM_MAP_R);

        // 如果是方形地图，直接利用 ImGui 的底层 DrawList 屏幕空间裁剪实现 Mask 遮罩
        if (MapRenderState::isSquareMap) draw_list->PushClipRect(mapMin, mapMax, true);
        
        int vtxStart = draw_list->VtxBuffer.Size;

        if (MapRenderState::isSquareMap) {
            draw_list->AddRectFilled(drawMin, drawMax, IM_COL32(20, 20, 20, 255));
            draw_list->AddCallback(PointSamplerCallback, nullptr);
            draw_list->AddImage((void*)g_mapTextureView, drawMin, drawMax, uv0, uv1, IM_COL32_WHITE);
            draw_list->AddCallback(LinearSamplerCallback, nullptr);
        } else {
            draw_list->AddCircleFilled(ImVec2(cx, cy), drawRadius, IM_COL32(20, 20, 20, 255), 64);
            draw_list->AddCallback(PointSamplerCallback, nullptr);
            draw_list->AddImageRounded((void*)g_mapTextureView, drawMin, drawMax, uv0, uv1, IM_COL32_WHITE, drawRadius);
            draw_list->AddCallback(LinearSamplerCallback, nullptr);
        }

        // 强行在提交前对绘制顶点进行矩阵旋转
        if (MapRenderState::rotateMiniMap) {
            for (int i = vtxStart; i < draw_list->VtxBuffer.Size; i++) {
                ImVec2& p = draw_list->VtxBuffer[i].pos;
                float p_dx = p.x - cx;
                float p_dy = p.y - cy;
                p.x = cx + p_dx * c_rot - p_dy * s_rot;
                p.y = cy + p_dx * s_rot + p_dy * c_rot;
            }
        }

        if (MapRenderState::isSquareMap) draw_list->PopClipRect();

        // 绘制物理外边框
        if (MapRenderState::isSquareMap) {
            draw_list->AddRect(mapMin, mapMax, IM_COL32(30, 30, 30, 255), 0.0f, 0, 2.0f);
        } else {
            draw_list->AddCircle(ImVec2(cx, cy), IM_MAP_R, IM_COL32(30, 30, 30, 255), 64, 2.0f);
        }

        ImFont* font = ImGui::GetFont();

        // ==========================================
        // [小地图多行 HUD 信息显示系统]
        // ==========================================
        struct InfoLine {
            std::string text;
            ImU32 color;
        };
        std::vector<InfoLine> hudLines;
        hudLines.reserve(5);

        // 1. 坐标与下界/主世界双向换算
        if (MapRenderState::infoShowCoords) {
            char coordBuf[128];
            int curDim = MapRenderState::currentDimensionId;
            if (MapRenderState::infoShowNetherCoords && curDim == 0) {
                // 主世界 -> 显示当前坐标并附带下界对应坐标
                snprintf(coordBuf, sizeof(coordBuf), "%d, %d, %d (%s: %d, %d)",
                    g_playerBlockX, (int)std::floor(g_playerY), g_playerBlockZ,
                    LanguageManager::GetText("LABEL_NETHER"),
                    g_playerBlockX / 8, g_playerBlockZ / 8);
            } else if (MapRenderState::infoShowNetherCoords && curDim == 1) {
                // 下界 -> 显示当前坐标并附带主世界对应坐标
                snprintf(coordBuf, sizeof(coordBuf), "%d, %d, %d (%s: %d, %d)",
                    g_playerBlockX, (int)std::floor(g_playerY), g_playerBlockZ,
                    LanguageManager::GetText("LABEL_OVERWORLD"),
                    g_playerBlockX * 8, g_playerBlockZ * 8);
            } else {
                snprintf(coordBuf, sizeof(coordBuf), "%d, %d, %d",
                    g_playerBlockX, (int)std::floor(g_playerY), g_playerBlockZ);
            }
            hudLines.push_back({ coordBuf, IM_COL32(255, 255, 255, 255) });
        }

        // 1.5. 区块局部坐标与区块索引
        if (MapRenderState::infoShowChunkCoords) {
            int cxIdx = g_playerBlockX >> 4;
            int czIdx = g_playerBlockZ >> 4;
            int lx = g_playerBlockX & 15;
            int lz = g_playerBlockZ & 15;
            char chunkBuf[96];
            snprintf(chunkBuf, sizeof(chunkBuf), "%s: [%d, %d] in [%d, %d]",
                LanguageManager::GetText("LABEL_CHUNK"), lx, lz, cxIdx, czIdx);
            hudLines.push_back({ chunkBuf, IM_COL32(200, 225, 255, 255) });
        }

        // 2. 玩家朝向及轴向与角度
        if (MapRenderState::infoShowFacing) {
            float normYaw = std::fmod(playerYaw, 360.0f);
            if (normYaw < -180.0f) normYaw += 360.0f;
            else if (normYaw > 180.0f) normYaw -= 360.0f;

            const char* dirStr = "";
            const char* axisStr = "";
            const char* N = LanguageManager::GetText("COMPASS_N");
            const char* S = LanguageManager::GetText("COMPASS_S");
            const char* E = LanguageManager::GetText("COMPASS_E");
            const char* W = LanguageManager::GetText("COMPASS_W");
            const char* NE = LanguageManager::GetText("DIR_NE");
            const char* NW = LanguageManager::GetText("DIR_NW");
            const char* SE = LanguageManager::GetText("DIR_SE");
            const char* SW = LanguageManager::GetText("DIR_SW");

            if (normYaw >= -22.5f && normYaw < 22.5f) {
                dirStr = S;
                axisStr = "+Z";
            } else if (normYaw >= 22.5f && normYaw < 67.5f) {
                dirStr = SW;
                axisStr = "-X +Z";
            } else if (normYaw >= 67.5f && normYaw < 112.5f) {
                dirStr = W;
                axisStr = "-X";
            } else if (normYaw >= 112.5f && normYaw < 157.5f) {
                dirStr = NW;
                axisStr = "-X -Z";
            } else if (normYaw >= -67.5f && normYaw < -22.5f) {
                dirStr = SE;
                axisStr = "+X +Z";
            } else if (normYaw >= -112.5f && normYaw < -67.5f) {
                dirStr = E;
                axisStr = "+X";
            } else if (normYaw >= -157.5f && normYaw < -112.5f) {
                dirStr = NE;
                axisStr = "+X -Z";
            } else {
                dirStr = N;
                axisStr = "-Z";
            }

            char facingBuf[96];
            snprintf(facingBuf, sizeof(facingBuf), "%s: %s (%s) (%.1f\xc2\xb0)",
                LanguageManager::GetText("LABEL_FACING"), dirStr, axisStr, normYaw);
            hudLines.push_back({ facingBuf, IM_COL32(230, 230, 255, 255) });
        }

        // 3. 生物群系 (直接显示群系名称，不带前缀)
        if (MapRenderState::infoShowBiome && !MapRenderState::translatedBiomeName.empty()) {
            hudLines.push_back({ MapRenderState::translatedBiomeName, IM_COL32(200, 240, 200, 255) });
        }

        // 3.5. 实时光照等级 (方块光照 / 天空光照)
        if (MapRenderState::infoShowLight) {
            int bl = MapRenderState::g_blockLight.load();
            int sl = MapRenderState::g_skyLight.load();
            if (bl >= 0 && sl >= 0) {
                int totalL = std::max(bl, sl);
                char lightBuf[96];
                snprintf(lightBuf, sizeof(lightBuf), "%s: %d (%s: %d, %s: %d)",
                    LanguageManager::GetText("LABEL_LIGHT"), totalL,
                    LanguageManager::GetText("LABEL_BLOCK_LIGHT"), bl,
                    LanguageManager::GetText("LABEL_SKY_LIGHT"), sl);
                ImU32 lightCol = (totalL <= 0) ? IM_COL32(255, 120, 100, 255) : IM_COL32(255, 235, 140, 255);
                hudLines.push_back({ lightBuf, lightCol });
            }
        }

        // 4. 游戏时间与本地时间
        if (MapRenderState::infoShowTime) {
            time_t rawtime;
            time(&rawtime);
            struct tm timeinfo = {};
            char realTimeBuf[32] = {0};
            if (localtime_s(&timeinfo, &rawtime) == 0) {
                if (MapRenderState::timeFormat24h) {
                    snprintf(realTimeBuf, sizeof(realTimeBuf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
                } else {
                    int rHour = timeinfo.tm_hour % 12;
                    if (rHour == 0) rHour = 12;
                    const char* rAmpm = (timeinfo.tm_hour < 12) ? "AM" : "PM";
                    snprintf(realTimeBuf, sizeof(realTimeBuf), "%02d:%02d %s", rHour, timeinfo.tm_min, rAmpm);
                }
            } else {
                snprintf(realTimeBuf, sizeof(realTimeBuf), "--:--");
            }

            int gameTicks = MapRenderState::g_gameTimeTicks.load();
            char timeBuf[96] = {0};
            if (gameTicks >= 0) {
                int day = gameTicks / 24000;
                int timeOfDay = (gameTicks + 6000) % 24000;
                if (timeOfDay < 0) timeOfDay += 24000;
                int gameHour = timeOfDay / 1000;
                int gameMin = (int)((timeOfDay % 1000) * 60 / 1000);

                char dayBuf[32] = {0};
                snprintf(dayBuf, sizeof(dayBuf), LanguageManager::GetText("DAY_COUNT"), day);

                if (MapRenderState::timeFormat24h) {
                    snprintf(timeBuf, sizeof(timeBuf), "%s, %02d:%02d (%s)", dayBuf, gameHour, gameMin, realTimeBuf);
                } else {
                    int gHour = gameHour % 12;
                    if (gHour == 0) gHour = 12;
                    const char* gAmpm = (gameHour < 12) ? "AM" : "PM";
                    snprintf(timeBuf, sizeof(timeBuf), "%s, %02d:%02d %s (%s)", dayBuf, gHour, gameMin, gAmpm, realTimeBuf);
                }
            } else {
                snprintf(timeBuf, sizeof(timeBuf), "%s", realTimeBuf);
            }
            hudLines.push_back({ timeBuf, IM_COL32(255, 240, 180, 255) });
        }

        // 统一绘制 HUD 文本
        if (!hudLines.empty()) {
            bool inUpperHalf = (cy <= displayH * 0.5f);
            float availableH = inUpperHalf ? (displayH - (cy + IM_MAP_R + 14.0f) - 8.0f)
                                           : (cy - IM_MAP_R - 14.0f - 8.0f);
            float maxFitFontSize = (availableH > 20.0f) ? (availableH / (hudLines.size() * 1.25f)) : fontSize;
            float hudFontSize = std::clamp(fontSize, 11.0f, std::max(11.0f, maxFitFontSize));

            float lineSpacing = hudFontSize * 1.25f;
            float totalTextHeight = (hudLines.size() - 1) * lineSpacing + hudFontSize;

            // 检查小地图下方是否能容纳全部文本信息
            float textTopBelow = cy + IM_MAP_R + 12.0f + hudFontSize;
            float textBottomBelow = textTopBelow + totalTextHeight + 4.0f;

            // 仅当小地图位于屏幕下半区且下方空间不足时，才转至小地图上方渲染；上半区绝不翻转到负坐标
            bool renderAbove = !inUpperHalf && (textBottomBelow > displayH - 8.0f);

            float lineY;
            if (!renderAbove) {
                // 空间充裕，在小地图下方正常绘制
                lineY = textTopBelow;
            } else {
                // 下方空间不足，将所有文本移至小地图上方（从上至下排列）
                float bottomLineY = cy - IM_MAP_R - 12.0f - hudFontSize;
                lineY = bottomLineY - (hudLines.size() - 1) * lineSpacing;
                if (lineY < 6.0f) lineY = 6.0f; // 顶部安全防溢出边界
            }

            for (const auto& item : hudLines) {
                ImVec2 txtSize = font->CalcTextSizeA(hudFontSize, FLT_MAX, 0.0f, item.text.c_str());
                float padX = std::clamp(6.0f * (hudFontSize / 18.0f), 5.0f, 12.0f);
                float padY = std::clamp(2.0f * (hudFontSize / 18.0f), 1.0f, 4.0f);
                float lineX = cx - txtSize.x * 0.5f;

                // 屏幕左右边缘防溢出安全保护，保证全部长文本（如区块、光照、坐标等）100% 完整显示在屏幕内
                if (lineX + txtSize.x + padX > displayW - 4.0f) {
                    lineX = displayW - 4.0f - padX - txtSize.x;
                }
                if (lineX - padX < 4.0f) {
                    lineX = 4.0f + padX;
                }

                ImVec2 txtPos(lineX, lineY);
                draw_list->AddRectFilled(
                    ImVec2(txtPos.x - padX, txtPos.y - padY),
                    ImVec2(txtPos.x + txtSize.x + padX, txtPos.y + txtSize.y + padY),
                    IM_COL32(15, 15, 15, 140), 4.0f);
                draw_list->AddText(font, hudFontSize, ImVec2(txtPos.x + 1.0f, txtPos.y + 1.0f), IM_COL32(0, 0, 0, 220), item.text.c_str());
                draw_list->AddText(font, hudFontSize, txtPos, item.color, item.text.c_str());
                lineY += lineSpacing;
            }
        }

        // 绘制正向的指南针东南西北（根据地图旋转角度推算正确的位置）
        // [小地图指南针字号] 适当调大指南针文字尺寸（12-16.5px），使其清晰醒目，
        // 精确居中于 20px 安全边距（textDist = IM_MAP_R + 10.0f），并设有屏幕边缘防溢出硬保护
        if (MapRenderState::showCompass) {
            float curCompScale = std::clamp(MapRenderState::compassScale, 0.8f, 2.0f);
            float compassFontSize = std::clamp(16.5f * (IM_MAP_R / 135.0f) * curCompScale, 10.0f * curCompScale, 24.0f * curCompScale);
            float textDist = IM_MAP_R + 10.0f * curCompScale;

            auto drawRotatedText = [&](const char* text, float offX, float offY, ImU32 textCol = IM_COL32(220, 220, 255, 255)) {
                float rotX = offX * c_rot - offY * s_rot;
                float rotY = offX * s_rot + offY * c_rot;
                
                // 如果是方形地图且在旋转，文字会因为距离固定而跑到地图框内部。
                // 这里将其动态投影到方形外边框上，确保方向字母始终在方形外部平移。
                if (MapRenderState::isSquareMap) {
                    float maxAxis = std::max(std::abs(rotX), std::abs(rotY));
                    if (maxAxis > 0.001f) {
                        rotX = (rotX / maxAxis) * textDist;
                        rotY = (rotY / maxAxis) * textDist;
                    }
                }
                
                ImVec2 ts = font->CalcTextSizeA(compassFontSize, FLT_MAX, 0.0f, text);
                ImVec2 pos(cx + rotX - ts.x / 2.0f, cy + rotY - ts.y / 2.0f);

                // 屏幕边缘安全保护，严防任何角度旋转或边界拉伸时指南针字符被裁切
                if (pos.x < 1.0f) pos.x = 1.0f;
                if (pos.x + ts.x > displayW - 1.5f) pos.x = displayW - 1.5f - ts.x;
                if (pos.y < 1.0f) pos.y = 1.0f;
                if (pos.y + ts.y > displayH - 1.5f) pos.y = displayH - 1.5f - ts.y;

                draw_list->AddText(font, compassFontSize, ImVec2(pos.x + 1, pos.y + 1), IM_COL32(0,0,0,200), text, NULL, 0.0f, NULL);
                draw_list->AddText(font, compassFontSize, pos, textCol, text, NULL, 0.0f, NULL);
            };
            drawRotatedText(LanguageManager::GetText("COMPASS_N"), 0.0f, -textDist, IM_COL32(255, 75, 75, 255));
            drawRotatedText(LanguageManager::GetText("COMPASS_S"), 0.0f, textDist, IM_COL32(220, 220, 255, 255));
            drawRotatedText(LanguageManager::GetText("COMPASS_E"), textDist, 0.0f, IM_COL32(220, 220, 255, 255));
            drawRotatedText(LanguageManager::GetText("COMPASS_W"), -textDist, 0.0f, IM_COL32(220, 220, 255, 255));
        }

        float scale = IM_MAP_R / ZOOM_RADIUS;

        // [小地图区块网格] 每 16 格绘制一条区块边界，与地形一同旋转；
        // 方形地图用矩形裁剪，圆形地图将每条线段解析裁剪到圆内
        if (MapRenderState::showChunkGrid) {
            constexpr float CHUNK_SIZE = 16.0f;
            const ImU32 gridLineCol = IM_COL32(255, 255, 255, 55);
            // 方形 + 旋转时视野需覆盖到正方形角点（对角线半径）
            float coverR = ZOOM_RADIUS * ((MapRenderState::isSquareMap && MapRenderState::rotateMiniMap) ? 1.42f : 1.0f);
            float coverPx = coverR * scale;

            if (MapRenderState::isSquareMap) draw_list->PushClipRect(mapMin, mapMax, true);


            // 输入未旋转的局部屏幕坐标，旋转后按地图形状裁剪并画线（可指定颜色/线宽）
            auto drawGridLine = [&](float lx0, float lz0, float lx1, float lz1,
                                    ImU32 lineCol = 0, float lineThick = -1.0f) {
                if (lineCol == 0) lineCol = gridLineCol;
                if (lineThick < 0.0f) lineThick = 1.0f;
                float r0x = lx0 * c_rot - lz0 * s_rot;
                float r0y = lx0 * s_rot + lz0 * c_rot;
                float r1x = lx1 * c_rot - lz1 * s_rot;
                float r1y = lx1 * s_rot + lz1 * c_rot;
                ImVec2 p0(cx + r0x, cy + r0y);
                ImVec2 p1(cx + r1x, cy + r1y);

                if (MapRenderState::isSquareMap) {
                    draw_list->AddLine(p0, p1, lineCol, lineThick);
                    return;
                }
                // 线段与圆形地图求交：|f + t*d|^2 = r^2，取圆内参数区间
                float dx = p1.x - p0.x, dy = p1.y - p0.y;
                float fx = p0.x - cx, fy = p0.y - cy;
                float a = dx * dx + dy * dy;
                if (a < 0.0001f) return;
                float bq = 2.0f * (fx * dx + fy * dy);
                float cq = fx * fx + fy * fy - IM_MAP_R * IM_MAP_R;
                float disc = bq * bq - 4.0f * a * cq;
                if (disc < 0.0f) return;
                float sq = std::sqrt(disc);
                float t0 = std::clamp((-bq - sq) / (2.0f * a), 0.0f, 1.0f);
                float t1 = std::clamp((-bq + sq) / (2.0f * a), 0.0f, 1.0f);
                if (t0 < t1) {
                    draw_list->AddLine(ImVec2(p0.x + dx * t0, p0.y + dy * t0),
                                       ImVec2(p0.x + dx * t1, p0.y + dy * t1), lineCol, lineThick);
                }
            };

            // 纵向区块边界（世界 X = 16k，线段沿 Z 方向）
            int firstB = (int)std::floor((viewPX - coverR) / CHUNK_SIZE) * 16;
            int lastB  = (int)std::ceil ((viewPX + coverR) / CHUNK_SIZE) * 16;
            for (int b = firstB; b <= lastB; b += 16) {
                float lx = ((float)b - viewPX) * scale;
                drawGridLine(lx, -coverPx, lx, coverPx);
            }
            // 横向区块边界（世界 Z = 16k，线段沿 X 方向）
            firstB = (int)std::floor((viewPZ - coverR) / CHUNK_SIZE) * 16;
            lastB  = (int)std::ceil ((viewPZ + coverR) / CHUNK_SIZE) * 16;
            for (int b = firstB; b <= lastB; b += 16) {
                float lz = ((float)b - viewPZ) * scale;
                drawGridLine(-coverPx, lz, coverPx, lz);
            }

            // [玩家所在区块高亮] 四条网格边换色加粗，随地形一同旋转
            {
                const ImU32 chunkEdgeCol = IM_COL32(255, 205, 70, 235);

                int pcx = (int)std::floor(viewPX / CHUNK_SIZE);
                int pcz = (int)std::floor(viewPZ / CHUNK_SIZE);
                float hx0 = ((float)(pcx * 16)      - viewPX) * scale;
                float hx1 = ((float)(pcx * 16 + 16) - viewPX) * scale;
                float hz0 = ((float)(pcz * 16)      - viewPZ) * scale;
                float hz1 = ((float)(pcz * 16 + 16) - viewPZ) * scale;

                // 四条网格边换为金色加粗重绘（drawGridLine 内部已处理圆形/方形裁剪）
                drawGridLine(hx0, hz0, hx1, hz0, chunkEdgeCol, 1.6f);
                drawGridLine(hx1, hz0, hx1, hz1, chunkEdgeCol, 1.6f);
                drawGridLine(hx1, hz1, hx0, hz1, chunkEdgeCol, 1.6f);
                drawGridLine(hx0, hz1, hx0, hz0, chunkEdgeCol, 1.6f);
            }

            if (MapRenderState::isSquareMap) draw_list->PopClipRect();
        }

        // [小地图雷达] 开启或按住Tab时绘制周围实体头像与标记
        if (MapRenderState::showRadar || g_tabHeld) {
            EntityIconManager::ClearPlayerHeadTexturesIfRequested();
            static std::vector<RadarEntity> s_cachedEntities;
            static std::string s_cachedLocalPlayerUuid;
            static uint64_t s_cachedMinimapGeneration = 0;
            RefreshRadarEntitySnapshot(s_cachedEntities, s_cachedLocalPlayerUuid, s_cachedMinimapGeneration);

            for (const auto& ent : s_cachedEntities) {
                // 以"视图有效中心"为基准定位, 窗口被钳制时实体位置仍地理精确
                float edx = ent.x - viewPX;
                float edz = ent.z - viewPZ;
                
                // 应用相对雷达坐标的矩阵旋转
                float rotDx = edx * c_rot - edz * s_rot;
                float rotDz = edx * s_rot + edz * c_rot;
                
                float ex = cx + rotDx * scale;
                float ez = cy + rotDz * scale;
                
                bool inBounds = false;
                if (MapRenderState::isSquareMap) {
                    inBounds = (std::abs(rotDx * scale) <= IM_MAP_R && std::abs(rotDz * scale) <= IM_MAP_R);
                } else {
                    float distSq = (ex - cx) * (ex - cx) + (ez - cy) * (ez - cy);
                    inBounds = (distSq <= IM_MAP_R * IM_MAP_R);
                }

                if (inBounds) {
                    if (ent.type == 0 && !s_cachedLocalPlayerUuid.empty() && ent.uuid == s_cachedLocalPlayerUuid) {
                        continue;
                    }
                    if (ent.type == 0 && !MapRenderState::radarShowPlayers) continue;
                    if (ent.type == 1 && !MapRenderState::radarShowHostile) continue;
                    if (ent.type == 2 && !MapRenderState::radarShowFriendly) continue;
                    if (ent.type == 3 && !MapRenderState::radarShowItems) continue;

                    float localPlayerY = (g_playerY > -9000.0f) ? g_playerY : 64.0f;
                    float dy = ent.y - localPlayerY;

                    // 垂直高度限制过滤 (超出高度限制的实体不显示)
                    if (MapRenderState::entityHeightLimit > 0 && std::abs(dy) > (float)MapRenderState::entityHeightLimit) {
                        continue;
                    }

                    ID3D11ShaderResourceView* texSRV = nullptr;
                    if (ent.type == 0 && g_pd3dDevice) {
                        texSRV = EntityIconManager::GetOrCreatePlayerHeadTexture(g_pd3dDevice, ent.uuid);
                    } else if (ent.type != 3 && g_pd3dDevice) {
                        texSRV = EntityIconManager::GetEntityHeadTexture(g_pd3dDevice, ent.typeName, ent.nameTag);
                    }

                    float hs = 6.0f * (IM_MAP_R / 135.0f);
                    if (ent.type == 0) hs *= 1.3f;
                    if (g_tabHeld) hs *= 2.0f;

                    // 显示实体深度：根据相对玩家的 Y 高低差暗化实体
                    float depthFactor = 1.0f;
                    if (MapRenderState::entityDepth && dy < -1.5f) {
                        depthFactor = std::clamp(1.0f + (dy / 30.0f) * 0.45f, 0.45f, 1.0f);
                    }

                    if (texSRV) {
                        ImU32 tintCol = IM_COL32((int)(255 * depthFactor), (int)(255 * depthFactor), (int)(255 * depthFactor), 255);
                        draw_list->AddImage((ImTextureID)texSRV, ImVec2(ex - hs, ez - hs), ImVec2(ex + hs, ez + hs), ImVec2(0, 0), ImVec2(1, 1), tintCol);
                    } else {
                        ImU32 col;
                        if (ent.type == 0) col = IM_COL32((int)(255 * depthFactor), (int)(255 * depthFactor), (int)(255 * depthFactor), 255);
                        else if (ent.type == 1) col = IM_COL32((int)(255 * depthFactor), (int)(50 * depthFactor), (int)(50 * depthFactor), 255);
                        else if (ent.type == 2) col = IM_COL32((int)(50 * depthFactor), (int)(255 * depthFactor), (int)(50 * depthFactor), 255);
                        else col = IM_COL32((int)(255 * depthFactor), (int)(255 * depthFactor), (int)(50 * depthFactor), 255);
                        float dotR = (ent.type == 0) ? 3.0f : 2.0f;
                        if (g_tabHeld) dotR *= 2.0f;
                        draw_list->AddRectFilled(ImVec2(ex - dotR, ez - dotR), ImVec2(ex + dotR, ez + dotR), col);
                        hs = dotR;
                    }

                    // 实体高低差指示箭头 (▲ / ▼)
                    if (MapRenderState::radarHeightIndicators) {
                        if (dy > 2.5f) {
                            // 实体在玩家上方 -> 向上箭头 ▲
                            float arrowTop = ez - hs - 2.0f;
                            ImVec2 p0(ex, arrowTop - 4.5f);
                            ImVec2 p1(ex - 3.5f, arrowTop);
                            ImVec2 p2(ex + 3.5f, arrowTop);
                            draw_list->AddTriangle(p0, p1, p2, IM_COL32(0, 0, 0, 240), 1.5f);
                            draw_list->AddTriangleFilled(p0, p1, p2, IM_COL32(255, 255, 255, 250));
                        } else if (dy < -2.5f) {
                            // 实体在玩家下方 -> 向下箭头 ▼
                            float arrowBottom = ez + hs + 2.0f;
                            ImVec2 p0(ex, arrowBottom + 4.5f);
                            ImVec2 p1(ex - 3.5f, arrowBottom);
                            ImVec2 p2(ex + 3.5f, arrowBottom);
                            draw_list->AddTriangle(p0, p1, p2, IM_COL32(0, 0, 0, 240), 1.5f);
                            draw_list->AddTriangleFilled(p0, p1, p2, IM_COL32(255, 255, 255, 250));
                        }
                    }

                    // 命名牌实体始终显示名称（按住 Tab 放大小地图生物头像时，未命名生物不显示名称，仅命名牌生物显示）
                    bool showThisName = !ent.nameTag.empty() && (MapRenderState::alwaysShowNametags || g_tabHeld);
                    if (showThisName) {
                        std::string displayName = ent.nameTag;
                        if (!displayName.empty()) {
                            float tagFontSize = std::clamp(fontSize * 0.72f, 9.0f * MapRenderState::globalUIScale, 16.0f * MapRenderState::globalUIScale);
                            ImVec2 nameSz = font->CalcTextSizeA(tagFontSize, FLT_MAX, 0.0f, displayName.c_str());
                            float tagY = ez + hs + ((dy < -2.5f && MapRenderState::radarHeightIndicators) ? 7.0f * MapRenderState::globalUIScale : 2.0f * MapRenderState::globalUIScale);
                            ImVec2 tagPos(ex - nameSz.x * 0.5f, tagY);
                            draw_list->AddText(font, tagFontSize, ImVec2(tagPos.x + 1.0f, tagPos.y + 1.0f), IM_COL32(0, 0, 0, 220), displayName.c_str(), NULL, 0.0f, NULL);
                            draw_list->AddText(font, tagFontSize, tagPos, IM_COL32(255, 255, 255, 235), displayName.c_str(), NULL, 0.0f, NULL);
                        }
                    }
                }
            }
        }

        // [Task 2] 小地图路径点显示开关：关闭时不绘制路径点
        if (MapRenderState::showWaypointsOnMinimap) {
            std::lock_guard<std::mutex> lock(WaypointManager::g_wpMutex);
            for (const auto& wp : WaypointManager::g_waypoints) {
                if (wp.dimId != MapRenderState::currentDimensionId) continue; // 仅显示当前维度路径点
                if (!wp.enabled) continue;
                
                // 以"视图有效中心"为基准定位, 窗口被钳制时路径点位置仍地理精确
                float wDx = wp.x - viewPX;
                float wDz = wp.z - viewPZ;
                float physicalDist = std::sqrt(wDx * wDx + wDz * wDz);
                if (physicalDist < 0.001f) physicalDist = 0.001f;

                float rotDx = wDx * c_rot - wDz * s_rot;
                float rotDz = wDx * s_rot + wDz * c_rot;

                float ex = cx + rotDx * scale;
                float ez = cy + rotDz * scale;
                
                bool inMap = false;
                float edgeX = cx, edgeZ = cy;

                if (MapRenderState::isSquareMap) {
                    if (std::abs(rotDx * scale) <= IM_MAP_R && std::abs(rotDz * scale) <= IM_MAP_R) {
                        inMap = true;
                    } else {
                        float maxDist = std::max(std::abs(rotDx), std::abs(rotDz));
                        edgeX = cx + (rotDx / maxDist) * IM_MAP_R;
                        edgeZ = cy + (rotDz / maxDist) * IM_MAP_R;
                    }
                } else {
                    if (physicalDist <= ZOOM_RADIUS) {
                        inMap = true;
                    } else {
                        edgeX = cx + (rotDx / physicalDist) * IM_MAP_R;
                        edgeZ = cy + (rotDz / physicalDist) * IM_MAP_R;
                    }
                }

                std::string distStr = "";
                if (MapRenderState::showWaypointDistance) {
                    float dy = (float)wp.y - g_playerY;
                    float d3d = std::sqrt(wDx * wDx + dy * dy + wDz * wDz);
                    char distBuf[32];
                    if (d3d >= 1000.0f) {
                        snprintf(distBuf, sizeof(distBuf), "%.1fkm", d3d / 1000.0f);
                    } else {
                        snprintf(distBuf, sizeof(distBuf), "%dm", (int)std::round(d3d));
                    }
                    distStr = distBuf;
                }

                if (inMap) {
                    DrawWaypointIcon(draw_list, ImVec2(ex, ez), mce::Color(wp.r, wp.g, wp.b, 1.0f), wp.name, false, 1.0f, wp.isTemporary, wp.enabled, distStr);
                } else {
                    DrawWaypointIcon(draw_list, ImVec2(edgeX, edgeZ), mce::Color(wp.r, wp.g, wp.b, 1.0f), "", true, 1.0f, wp.isTemporary, wp.enabled);
                }
            }
        }

        // 小地图死亡点显示 (红叉 ❌，超出视野钳制在边框上指向死亡点)
        // 与路径点共用"小地图显示标记"开关：关闭时路径点与死亡点均不绘制
        if (MapRenderState::showWaypointsOnMinimap) {
            std::lock_guard<std::mutex> lock(DeathPointManager::g_deathMutex);
            for (const auto& dp : DeathPointManager::g_deathPoints) {
                if (dp.dimensionId != MapRenderState::currentDimensionId) continue;
                
                float wDx = (float)dp.x + 0.5f - viewPX;
                float wDz = (float)dp.z + 0.5f - viewPZ;
                float physicalDist = std::sqrt(wDx * wDx + wDz * wDz);
                if (physicalDist < 0.001f) physicalDist = 0.001f;

                float rotDx = wDx * c_rot - wDz * s_rot;
                float rotDz = wDx * s_rot + wDz * c_rot;

                float ex = cx + rotDx * scale;
                float ez = cy + rotDz * scale;
                
                bool inMap = false;
                float edgeX = cx, edgeZ = cy;

                if (MapRenderState::isSquareMap) {
                    if (std::abs(rotDx * scale) <= IM_MAP_R && std::abs(rotDz * scale) <= IM_MAP_R) {
                        inMap = true;
                    } else {
                        float maxDist = std::max(std::abs(rotDx), std::abs(rotDz));
                        edgeX = cx + (rotDx / maxDist) * IM_MAP_R;
                        edgeZ = cy + (rotDz / maxDist) * IM_MAP_R;
                    }
                } else {
                    if (physicalDist <= ZOOM_RADIUS) {
                        inMap = true;
                    } else {
                        edgeX = cx + (rotDx / physicalDist) * IM_MAP_R;
                        edgeZ = cy + (rotDz / physicalDist) * IM_MAP_R;
                    }
                }

                if (inMap) {
                    DrawDeathPointIcon(draw_list, ImVec2(ex, ez), LanguageManager::GetText("DEATH_POINT_WP_PREFIX"), false);
                } else {
                    DrawDeathPointIcon(draw_list, ImVec2(edgeX, edgeZ), "", true);
                }
            }
        }

        // 绘制玩家朝向指示器。如果地图旋转开启，指示器永远朝上 (角度 0)。
        // 位置按玩家真实坐标相对"视图有效中心"换算: 正常情况两者重合 (箭头居中),
        // 跑图脱离扫描数据范围时 (窗口被钳制), 箭头沿行进方向滑向地图边缘并钉在边内。
        float yawRad = MapRenderState::rotateMiniMap ? 0.0f : (playerYaw + 180.0f) * (3.14159265f / 180.0f);
        float cosY = std::cos(yawRad);
        float sinY = std::sin(yawRad);
        float pOffX = (pX - viewPX) * scale;
        float pOffZ = (pZ - viewPZ) * scale;
        float pRotX = pOffX * c_rot - pOffZ * s_rot;
        float pRotY = pOffX * s_rot + pOffZ * c_rot;
        float pDist = std::sqrt(pRotX * pRotX + pRotY * pRotY);
        float pMaxDist = IM_MAP_R - 14.0f;
        if (pDist > pMaxDist) {
            float k = pMaxDist / pDist;
            pRotX *= k;
            pRotY *= k;
        }
        float ax = cx + pRotX;
        float ay = cy + pRotY;
        auto rotate = [&](float x, float y) -> ImVec2 { return ImVec2(ax + (x * cosY - y * sinY), ay + (x * sinY + y * cosY)); };

        // [玩家足迹追踪] 小地图渲染足迹点
        if (MapRenderState::showFootsteps) {
            std::lock_guard<std::mutex> lock(MapRenderState::g_footstepsMutex);
            static auto s_startEpoch = std::chrono::steady_clock::now();
            float curTime = std::chrono::duration<float>(std::chrono::steady_clock::now() - s_startEpoch).count();
            size_t count = MapRenderState::g_footsteps.size();
            for (size_t i = 0; i < count; ++i) {
                const auto& step = MapRenderState::g_footsteps[i];
                if (step.dimId != MapRenderState::currentDimensionId) continue;
                float age = curTime - step.timestamp;
                if (age > 600.0f) continue;

                float fOffX = (step.x - viewPX) * scale;
                float fOffZ = (step.z - viewPZ) * scale;
                float fRotX = fOffX * c_rot - fOffZ * s_rot;
                float fRotY = fOffX * s_rot + fOffZ * c_rot;
                float fDist = std::sqrt(fRotX * fRotX + fRotY * fRotY);
                if (fDist > IM_MAP_R - 5.0f) continue;

                float fx = cx + fRotX;
                float fy = cy + fRotY;
                float progress = (float)(i + 1) / (float)(count + 1);
                float alpha = std::clamp(progress * 0.80f, 0.18f, 0.80f);
                if (age > 300.0f) alpha *= (1.0f - (age - 300.0f) / 300.0f);

                float r = std::clamp(2.0f * MapRenderState::globalUIScale, 1.2f, 3.2f);
                draw_list->AddCircleFilled(ImVec2(fx, fy), r + 0.6f, IM_COL32(0, 0, 0, (int)(150 * alpha)));
                draw_list->AddCircleFilled(ImVec2(fx, fy), r, IM_COL32(255, 235, 120, (int)(230 * alpha)));
            }
        }

        float aScale = MapRenderState::playerArrowScale;
        draw_list->AddTriangleFilled(rotate(0, -10.0f * aScale), rotate(-7.0f * aScale, 10.0f * aScale), rotate(7.0f * aScale, 10.0f * aScale), IM_COL32(0, 0, 0, 255));
        draw_list->AddTriangleFilled(rotate(0, -8.0f * aScale), rotate(-5.0f * aScale, 8.0f * aScale), rotate(5.0f * aScale, 8.0f * aScale), GetPlayerArrowColor());
    }

    inline void UpdateRegionTexture(uint64_t hash, int& texCount) {
        if (!g_pd3dDevice || !g_pd3dDeviceContext) return;
        
        bool isKnown = (g_regionTextures.find(hash) != g_regionTextures.end());
        
        // 1. 【极速状态探测】直接窥探底层 IO 状态，如果缺失直接排队，绝不阻塞主线程，完美修复加载断层
        bool isLoaded = false;
        bool isLoadedAndDirty = false;
        {
            std::lock_guard<std::mutex> lock(MapCacheManager::g_cacheMutex);
            auto it = MapCacheManager::g_loadedRegions.find(hash);
            if (it == MapCacheManager::g_loadedRegions.end()) {
                MapCacheManager::g_loadedRegions[hash] = nullptr;
                MapCacheManager::g_loadQueue.push_back(hash);
                return; // 刚排队，直接返回
            } else if (it->second != nullptr) {
                isLoaded = true;
                isLoadedAndDirty = it->second->textureDirty;
            }
        }

        // 正在异步加载中
        if (!isLoaded) return;

        // 如果纹理已在 GPU 中且数据未变，无需更新
        if (isKnown && !isLoadedAndDirty) return;

        // 需要建图但本帧建图配额（提升至8以加快大地图初次渲染）已满，直接返回（保留给下帧）
        if (!isKnown && texCount >= 8) return;

        // 内存对齐以支持极速 64 位空域探测
        alignas(8) static uint8_t tempBuffer[256 * 256 * 4];
        if (MapCacheManager::FetchRegionTextureData(hash, tempBuffer, !isKnown)) {
            
            // 【DrawCall级优化核心】透明区块免疫技术：如果这片区域完全没探索过（纯透明），彻底免渲染！
            bool isEmpty = true;
            uint64_t* ptr64 = (uint64_t*)tempBuffer;
            for (int i = 0; i < (256 * 256 * 4) / 8; ++i) {
                if (ptr64[i] != 0) { isEmpty = false; break; }
            }

            // 若原本未知，或原本是空贴图但现在有了数据
            if (!isKnown || (isKnown && g_regionTextures[hash] == nullptr && !isEmpty)) {
                if (isEmpty) {
                    // 标记为空贴图，不占用显存，极大幅度缩减 ImGui DrawCall 数量
                    g_regionTextures[hash] = nullptr;
                    g_regionSRVs[hash] = nullptr;
                } else {
                    texCount++;
                    D3D11_TEXTURE2D_DESC desc = {};
                    desc.Width = 256; desc.Height = 256;
                    desc.MipLevels = 1; desc.ArraySize = 1;
                    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                    desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
                    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                    
                    D3D11_SUBRESOURCE_DATA initData = {};
                    initData.pSysMem = tempBuffer;
                    initData.SysMemPitch = 256 * 4;
                    
                    ID3D11Texture2D* tex = nullptr;
                    ID3D11ShaderResourceView* srv = nullptr;
                    
                    if (SUCCEEDED(g_pd3dDevice->CreateTexture2D(&desc, &initData, &tex))) {
                        g_pd3dDevice->CreateShaderResourceView(tex, NULL, &srv);
                        g_regionTextures[hash] = tex;
                        g_regionSRVs[hash] = srv;
                    }
                }
            } else if (g_regionTextures[hash] != nullptr) {
                g_pd3dDeviceContext->UpdateSubresource(g_regionTextures[hash], 0, NULL, tempBuffer, 256 * 4, 0);
            }
        }
    }

    // ==========================================
    // 提取公共重命名模态弹窗组件
    // ==========================================
    // ==========================================
    // [调整小地图布局] 面板 (切换到小地图游戏画面中渲染)
    // ==========================================
    inline void RenderMiniMapPosSettings() {
        static float origX = 0.0f;
        static float origY = 0.0f;
        static float origScale = 1.0f;
        static float origZoomRadius = 50.0f;
        static bool initialized = false;

        if (!MapRenderState::showMiniMapPosSettings) {
            if (initialized) {
                // 窗口异常关闭时的状态还原
                MapRenderState::miniMapOffsetX = origX;
                MapRenderState::miniMapOffsetY = origY;
                MapRenderState::miniMapScale = origScale;
                MapRenderState::miniMapZoomRadius = origZoomRadius;
                initialized = false;
            }
            return;
        }

        if (!initialized) {
            origX = MapRenderState::miniMapOffsetX;
            origY = MapRenderState::miniMapOffsetY;
            origScale = MapRenderState::miniMapScale;
            origZoomRadius = MapRenderState::miniMapZoomRadius;
            initialized = true;
        }

        // 窗口正居中显示
        float curScale = std::clamp(MapRenderState::globalUIScale, 0.25f, 4.0f);
        float displayX = ImGui::GetIO().DisplaySize.x;
        float displayY = ImGui::GetIO().DisplaySize.y;
        ImGui::SetNextWindowPos(ImVec2(displayX * 0.5f, displayY * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(480.0f * curScale, 0), ImGuiCond_Appearing); 

        if (ImGui::Begin(LanguageManager::GetText("EDIT_MINIMAP_POS"), &MapRenderState::showMiniMapPosSettings, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize)) {
            
            float availWidth = ImGui::GetContentRegionAvail().x;
            float btnSize = ImGui::GetFrameHeight();
            float inputWidth = 55.0f * curScale;
            float spacing = ImGui::GetStyle().ItemSpacing.x;
            float labelWidth = std::max({
                ImGui::CalcTextSize(LanguageManager::GetText("X_OFFSET")).x,
                ImGui::CalcTextSize(LanguageManager::GetText("MINIMAP_SCALE")).x,
                ImGui::CalcTextSize(LanguageManager::GetText("MINIMAP_ZOOM_RADIUS")).x
            });
            float sliderWidth = availWidth - labelWidth - (btnSize * 3.0f) - inputWidth - (spacing * 5.0f) - 15.0f * curScale;

            float IM_MAP_R = std::floor(135.0f * MapRenderState::miniMapScale);
            float IM_MAP_MARGIN = 20.0f;
            // 动态计算绝对边界范围（拉满可直达屏幕底部，文本不足时会自动移至小地图上方）
            float minOffsetX = 2.0f * (IM_MAP_MARGIN + IM_MAP_R) - displayX;
            float maxOffsetX = 0.0f;
            float minOffsetY = 0.0f;
            float maxOffsetY = displayY - 2.0f * (IM_MAP_MARGIN + IM_MAP_R);

            auto drawRow = [&](const char* label, const char* idSlider, const char* idSub, const char* idInput, const char* idAdd, const char* idReset, float& value, float minVal, float maxVal, float step, const char* format, float resetValue) {
                ImGui::Text("%s", label);
                ImGui::SameLine(labelWidth + 15.0f * curScale);

                ImGui::PushItemWidth(sliderWidth);
                ImGui::SliderFloat(idSlider, &value, minVal, maxVal, format);
                ImGui::PopItemWidth();

                ImGui::SameLine();
                if (ImGui::ArrowButton(idSub, ImGuiDir_Left)) value -= step;

                ImGui::SameLine();
                ImGui::PushItemWidth(inputWidth);
                ImGui::InputFloat(idInput, &value, 0.0f, 0.0f, format);
                ImGui::PopItemWidth();

                ImGui::SameLine();
                if (ImGui::ArrowButton(idAdd, ImGuiDir_Right)) value += step;

                ImGui::SameLine();
                if (ImGui::Button(idReset, ImVec2(btnSize, btnSize))) value = resetValue;
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("RESET"));
            };

            drawRow(LanguageManager::GetText("X_OFFSET"), "##XSlider", "##XSub", "##XInput", "##XAdd", "\u21BA##XReset", MapRenderState::miniMapOffsetX, minOffsetX, maxOffsetX, 5.0f, "%.0f", 0.0f);
            drawRow(LanguageManager::GetText("Y_OFFSET"), "##YSlider", "##YSub", "##YInput", "##YAdd", "\u21BA##YReset", MapRenderState::miniMapOffsetY, minOffsetY, maxOffsetY, 5.0f, "%.0f", 0.0f);
            drawRow(LanguageManager::GetText("MINIMAP_SCALE"), "##ScaleSlider", "##ScaleSub", "##ScaleInput", "##ScaleAdd", "\u21BA##ScaleReset", MapRenderState::miniMapScale, 0.2f, 2.5f, 0.1f, "%.2f", 1.0f);
            drawRow(LanguageManager::GetText("MINIMAP_ZOOM_RADIUS"), "##ZoomSlider", "##ZoomSub", "##ZoomInput", "##ZoomAdd", "\u21BA##ZoomReset", MapRenderState::miniMapZoomRadius, 10.0f, 200.0f, 5.0f, "%.0f", 50.0f);

            // 输入框/箭头按钮不受 Slider 边界限制，手动夹取为安全区间防御越界
            if (MapRenderState::miniMapScale < 0.2f) MapRenderState::miniMapScale = 0.2f;
            if (MapRenderState::miniMapScale > 2.5f) MapRenderState::miniMapScale = 2.5f;
            if (MapRenderState::miniMapZoomRadius < 10.0f) MapRenderState::miniMapZoomRadius = 10.0f;
            if (MapRenderState::miniMapZoomRadius > 200.0f) MapRenderState::miniMapZoomRadius = 200.0f;

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s:", LanguageManager::GetText("PRESET_POSITIONS"));
            
            float pBtnW = (availWidth - spacing * 2.0f) / 3.0f;
            float current_R = std::floor(135.0f * MapRenderState::miniMapScale);
            float spanX = displayX - (IM_MAP_MARGIN + current_R) * 2.0f;
            float spanY = displayY - (IM_MAP_MARGIN + current_R) * 2.0f;

            if (ImGui::Button(LanguageManager::GetText("PRESET_TOP_LEFT"), ImVec2(pBtnW, 0))) {
                MapRenderState::miniMapOffsetX = -spanX;
                MapRenderState::miniMapOffsetY = 0.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button(LanguageManager::GetText("PRESET_CENTER"), ImVec2(pBtnW, 0))) {
                MapRenderState::miniMapOffsetX = -spanX * 0.5f;
                MapRenderState::miniMapOffsetY = spanY * 0.5f;
            }
            ImGui::SameLine();
            if (ImGui::Button(LanguageManager::GetText("PRESET_TOP_RIGHT"), ImVec2(pBtnW, 0))) {
                MapRenderState::miniMapOffsetX = 0.0f;
                MapRenderState::miniMapOffsetY = 0.0f;
            }

            if (ImGui::Button(LanguageManager::GetText("PRESET_BOTTOM_LEFT"), ImVec2(pBtnW, 0))) {
                MapRenderState::miniMapOffsetX = -spanX;
                MapRenderState::miniMapOffsetY = spanY;
            }
            ImGui::SameLine();
            if (ImGui::Button(LanguageManager::GetText("DEFAULT_POS"), ImVec2(pBtnW, 0))) {
                MapRenderState::miniMapOffsetX = 0.0f;
                MapRenderState::miniMapOffsetY = 0.0f;
                MapRenderState::miniMapScale = 1.0f;
                MapRenderState::miniMapZoomRadius = 50.0f;
            }
            ImGui::SameLine();
            if (ImGui::Button(LanguageManager::GetText("PRESET_BOTTOM_RIGHT"), ImVec2(pBtnW, 0))) {
                MapRenderState::miniMapOffsetX = 0.0f;
                MapRenderState::miniMapOffsetY = spanY;
            }

            ImGui::Spacing();
            ImGui::TextDisabled("%s", LanguageManager::GetText("MINIMAP_DRAG_HINT"));
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            float btnWidth = (availWidth - spacing) / 2.0f;
            if (ImGui::Button(LanguageManager::GetText("SAVE_AND_EXIT"), ImVec2(btnWidth, 0))) {
                LanguageManager::SaveConfig();
                MapRenderState::showMiniMapPosSettings = false;
                MapRenderState::showBigMap = true;
                MapRenderState::showMiniMapSettings = true;
                initialized = false;
            }
            ImGui::SameLine();
            if (ImGui::Button(LanguageManager::GetText("DONT_SAVE"), ImVec2(btnWidth, 0))) {
                MapRenderState::miniMapOffsetX = origX;
                MapRenderState::miniMapOffsetY = origY;
                MapRenderState::miniMapScale = origScale;
                MapRenderState::miniMapZoomRadius = origZoomRadius;
                MapRenderState::showMiniMapPosSettings = false;
                MapRenderState::showBigMap = true;
                MapRenderState::showMiniMapSettings = true;
                initialized = false;
            }
        }
        ImGui::End();
    }

    // ==========================================
    // [小地图设置] 面板 (在大地图内渲染)
    // ==========================================
    inline void RenderMiniMapSettings() {
        if (!MapRenderState::showMiniMapSettings) return;

        float curScale = std::clamp(MapRenderState::globalUIScale, 0.25f, 4.0f);
        float displayX = ImGui::GetIO().DisplaySize.x;
        float displayY = ImGui::GetIO().DisplaySize.y;
        ImGui::SetNextWindowPos(ImVec2(displayX * 0.5f, displayY * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(380.0f * curScale, 100.0f * curScale), ImVec2(520.0f * curScale, displayY * 0.95f));
        ImGui::SetNextWindowSize(ImVec2(420.0f * curScale, 0), ImGuiCond_Appearing);

        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin(LanguageManager::GetText("MINIMAP_SETTINGS"), &MapRenderState::showMiniMapSettings, winFlags)) {

            // 1. 小地图基础开关
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("MINIMAP_DISPLAY_SETTINGS"));
            ImGui::Separator();
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_MINIMAP"), &MapRenderState::showMiniMap)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SQUARE_MINIMAP"), &MapRenderState::isSquareMap)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("ROTATE_MINIMAP"), &MapRenderState::rotateMiniMap)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_WAYPOINTS_MINIMAP"), &MapRenderState::showWaypointsOnMinimap)) {
                LanguageManager::SaveConfig();
            }
            if (MapRenderState::showWaypointsOnMinimap) {
                ImGui::Indent(18.0f * curScale);
                if (ImGui::Checkbox(LanguageManager::GetText("SHOW_WAYPOINT_DISTANCE"), &MapRenderState::showWaypointDistance)) {
                    LanguageManager::SaveConfig();
                }
                ImGui::Unindent(18.0f * curScale);
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_RADAR"), &MapRenderState::showRadar)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_COMPASS"), &MapRenderState::showCompass)) {
                LanguageManager::SaveConfig();
            }
            if (MapRenderState::showCompass) {
                float btnSize = ImGui::GetFrameHeight();
                float totalAvail = ImGui::GetContentRegionAvail().x;
                float labelW = ImGui::CalcTextSize(LanguageManager::GetText("COMPASS_SCALE")).x;
                float sliderW = std::clamp(totalAvail - labelW - btnSize - 16.0f * curScale, 110.0f * curScale, 180.0f * curScale);

                float cScale = MapRenderState::compassScale;
                ImGui::SetNextItemWidth(sliderW);
                if (ImGui::SliderFloat("##CompassScaleSlider", &cScale, 0.8f, 2.0f, "%.2fx")) {
                    MapRenderState::compassScale = cScale;
                    LanguageManager::SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button("\u21BA##CompassScaleReset", ImVec2(btnSize, btnSize))) {
                    MapRenderState::compassScale = 1.0f;
                    LanguageManager::SaveConfig();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s (1.00x)", LanguageManager::GetText("RESET"));
                }
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("COMPASS_SCALE"));
            }
            {
                float btnSize = ImGui::GetFrameHeight();
                float totalAvail = ImGui::GetContentRegionAvail().x;
                float labelW = ImGui::CalcTextSize(LanguageManager::GetText("PLAYER_ARROW_SCALE")).x;
                float sliderW = std::clamp(totalAvail - labelW - btnSize - 16.0f * curScale, 110.0f * curScale, 180.0f * curScale);

                float pScale = MapRenderState::playerArrowScale;
                ImGui::SetNextItemWidth(sliderW);
                if (ImGui::SliderFloat("##PlayerArrowScaleMiniSlider", &pScale, 0.5f, 2.0f, "%.2fx")) {
                    MapRenderState::playerArrowScale = pScale;
                    LanguageManager::SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button("\u21BA##PlayerArrowScaleMiniReset", ImVec2(btnSize, btnSize))) {
                    MapRenderState::playerArrowScale = 1.0f;
                    LanguageManager::SaveConfig();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s (1.00x)", LanguageManager::GetText("RESET"));
                }
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("PLAYER_ARROW_SCALE"));
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 2. HUD 实时信息栏选项
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("MINIMAP_INFO_SETTINGS"));
            ImGui::Separator();
            if (ImGui::Checkbox(LanguageManager::GetText("INFO_SHOW_COORDS"), &MapRenderState::infoShowCoords)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("INFO_SHOW_NETHER_COORDS"), &MapRenderState::infoShowNetherCoords)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("INFO_SHOW_FACING"), &MapRenderState::infoShowFacing)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("INFO_SHOW_BIOME"), &MapRenderState::infoShowBiome)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("INFO_SHOW_TIME"), &MapRenderState::infoShowTime)) {
                LanguageManager::SaveConfig();
            }
            if (MapRenderState::infoShowTime) {
                ImGui::Indent(18.0f * curScale);
                if (ImGui::Checkbox(LanguageManager::GetText("TIME_FORMAT_24H"), &MapRenderState::timeFormat24h)) {
                    LanguageManager::SaveConfig();
                }
                ImGui::Unindent(18.0f * curScale);
            }
            if (ImGui::Checkbox(LanguageManager::GetText("INFO_SHOW_LIGHT"), &MapRenderState::infoShowLight)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("INFO_SHOW_CHUNK_COORDS"), &MapRenderState::infoShowChunkCoords)) {
                LanguageManager::SaveConfig();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 3. 雷达与实体高低差设置
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("RADAR_SETTINGS"));
            ImGui::Separator();
            if (ImGui::Checkbox(LanguageManager::GetText("RADAR_HEIGHT_INDICATORS"), &MapRenderState::radarHeightIndicators)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("ENTITY_DEPTH"), &MapRenderState::entityDepth)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("ALWAYS_SHOW_NAMETAGS"), &MapRenderState::alwaysShowNametags)) {
                LanguageManager::SaveConfig();
            }
            {
                float btnSize = ImGui::GetFrameHeight();
                float totalAvail = ImGui::GetContentRegionAvail().x;
                float labelW = ImGui::CalcTextSize(LanguageManager::GetText("ENTITY_HEIGHT_LIMIT")).x;
                float sliderW = std::clamp(totalAvail - labelW - btnSize - 16.0f * curScale, 110.0f * curScale, 180.0f * curScale);

                int limit = MapRenderState::entityHeightLimit;
                char limitBuf[64];
                if (limit == 0) snprintf(limitBuf, sizeof(limitBuf), "%s", LanguageManager::GetText("ENTITY_HEIGHT_LIMIT_UNLIMITED"));
                else snprintf(limitBuf, sizeof(limitBuf), "%d", limit);

                ImGui::SetNextItemWidth(sliderW);
                if (ImGui::SliderInt("##EntityHeightLimitSlider", &limit, 0, 128, limitBuf)) {
                    MapRenderState::entityHeightLimit = limit;
                    LanguageManager::SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button("\u21BA##EntityHeightLimitReset", ImVec2(btnSize, btnSize))) {
                    MapRenderState::entityHeightLimit = 0;
                    LanguageManager::SaveConfig();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s (%s)", LanguageManager::GetText("RESET"), LanguageManager::GetText("ENTITY_HEIGHT_LIMIT_UNLIMITED"));
                }
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("ENTITY_HEIGHT_LIMIT"));
            }

            // 实体雷达分类过滤
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.35f, 0.75f, 0.95f, 1.0f), "%s", LanguageManager::GetText("RADAR_CATEGORIES"));
            if (ImGui::Checkbox(LanguageManager::GetText("RADAR_SHOW_PLAYERS"), &MapRenderState::radarShowPlayers)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("RADAR_SHOW_HOSTILE"), &MapRenderState::radarShowHostile)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("RADAR_SHOW_FRIENDLY"), &MapRenderState::radarShowFriendly)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("RADAR_SHOW_ITEMS"), &MapRenderState::radarShowItems)) {
                LanguageManager::SaveConfig();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 4. 探索与交互设置
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("EXPLORATION_SETTINGS"));
            ImGui::Separator();
            if (ImGui::Checkbox(LanguageManager::GetText("AUTO_REMOVE_DEATHPOINTS"), &MapRenderState::autoRemoveDeathpoints)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("ENLARGE_MINIMAP_TOGGLE"), &MapRenderState::enlargeMinimapToggle)) {
                LanguageManager::SaveConfig();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 5. 调整小地图布局 (点击后才切换到显示小地图的界面)
            if (ImGui::Button(LanguageManager::GetText("EDIT_MINIMAP_POS"), ImVec2(-1, 0))) {
                MapRenderState::showMiniMapPosSettings = true;
                MapRenderState::showBigMap = false;
                MapRenderState::showMiniMapSettings = false;
            }

            ImGui::Spacing();
        }
        ImGui::End();
    }

    // ==========================================
    // [全屏大地图设置] 面板 (在大地图内渲染)
    // ==========================================
    inline void RenderBigMapSettings() {
        if (!MapRenderState::showBigMapSettings) return;

        float curScale = std::clamp(MapRenderState::globalUIScale, 0.25f, 4.0f);
        float displayX = ImGui::GetIO().DisplaySize.x;
        float displayY = ImGui::GetIO().DisplaySize.y;
        ImGui::SetNextWindowPos(ImVec2(displayX * 0.5f, displayY * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(380.0f * curScale, 100.0f * curScale), ImVec2(540.0f * curScale, displayY * 0.95f));
        ImGui::SetNextWindowSize(ImVec2(440.0f * curScale, 0), ImGuiCond_Appearing);

        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin(LanguageManager::GetText("BIGMAP_SETTINGS"), &MapRenderState::showBigMapSettings, winFlags)) {

            // 1. 大地图显示选项
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("BIGMAP_DISPLAY_SETTINGS"));
            ImGui::Separator();
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_BIGMAP_HOVER_BOX"), &MapRenderState::bigMapShowHoverBox)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_BIGMAP_ENTITIES"), &MapRenderState::bigMapShowEntities)) {
                LanguageManager::SaveConfig();
            }
            if (MapRenderState::bigMapShowEntities) {
                ImGui::Indent(15.0f * curScale);
                ImGui::TextColored(ImVec4(0.35f, 0.75f, 0.95f, 1.0f), "%s", LanguageManager::GetText("RADAR_CATEGORIES"));
                if (ImGui::Checkbox(LanguageManager::GetText("RADAR_SHOW_PLAYERS"), &MapRenderState::radarShowPlayers)) {
                    LanguageManager::SaveConfig();
                }
                if (ImGui::Checkbox(LanguageManager::GetText("RADAR_SHOW_HOSTILE"), &MapRenderState::radarShowHostile)) {
                    LanguageManager::SaveConfig();
                }
                if (ImGui::Checkbox(LanguageManager::GetText("RADAR_SHOW_FRIENDLY"), &MapRenderState::radarShowFriendly)) {
                    LanguageManager::SaveConfig();
                }
                if (ImGui::Checkbox(LanguageManager::GetText("RADAR_SHOW_ITEMS"), &MapRenderState::radarShowItems)) {
                    LanguageManager::SaveConfig();
                }
                {
                    float btnSize = ImGui::GetFrameHeight();
                    float totalAvail = ImGui::GetContentRegionAvail().x;
                    float labelW = ImGui::CalcTextSize(LanguageManager::GetText("BIGMAP_ENTITY_SCALE")).x;
                    float sliderW = std::clamp(totalAvail - labelW - btnSize - 16.0f * curScale, 110.0f * curScale, 180.0f * curScale);

                    float entScale = MapRenderState::bigMapEntityScale;
                    ImGui::SetNextItemWidth(sliderW);
                    if (ImGui::SliderFloat("##BigMapEntityScaleSlider", &entScale, 0.5f, 2.5f, "%.2fx")) {
                        MapRenderState::bigMapEntityScale = entScale;
                        LanguageManager::SaveConfig();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("\u21BA##BigMapEntityScaleReset", ImVec2(btnSize, btnSize))) {
                        MapRenderState::bigMapEntityScale = 1.0f;
                        LanguageManager::SaveConfig();
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s (1.00x)", LanguageManager::GetText("RESET"));
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s", LanguageManager::GetText("BIGMAP_ENTITY_SCALE"));
                }
                ImGui::Unindent(15.0f * curScale);
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_BIGMAP_MARKERS"), &MapRenderState::bigMapShowMarkers)) {
                LanguageManager::SaveConfig();
            }
            if (MapRenderState::bigMapShowMarkers) {
                ImGui::Indent(15.0f * curScale);
                if (ImGui::Checkbox(LanguageManager::GetText("SHOW_WAYPOINT_DISTANCE"), &MapRenderState::showWaypointDistance)) {
                    LanguageManager::SaveConfig();
                }
                ImGui::Unindent(15.0f * curScale);
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_DISABLED_WAYPOINTS"), &MapRenderState::bigMapShowDisabledWaypoints)) {
                LanguageManager::SaveConfig();
            }
            {
                float btnSize = ImGui::GetFrameHeight();
                float totalAvail = ImGui::GetContentRegionAvail().x;
                float labelW = ImGui::CalcTextSize(LanguageManager::GetText("BIGMAP_WAYPOINT_SCALE")).x;
                float sliderW = std::clamp(totalAvail - labelW - btnSize - 16.0f * curScale, 110.0f * curScale, 180.0f * curScale);

                float wpScale = MapRenderState::bigMapWaypointScale;
                ImGui::SetNextItemWidth(sliderW);
                if (ImGui::SliderFloat("##BigMapWaypointScaleSlider", &wpScale, 0.5f, 2.5f, "%.2fx")) {
                    MapRenderState::bigMapWaypointScale = wpScale;
                    LanguageManager::SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button("\u21BA##BigMapWaypointScaleReset", ImVec2(btnSize, btnSize))) {
                    MapRenderState::bigMapWaypointScale = 1.0f;
                    LanguageManager::SaveConfig();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s (1.00x)", LanguageManager::GetText("RESET"));
                }
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("BIGMAP_WAYPOINT_SCALE"));
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_CHUNK_GRID"), &MapRenderState::showChunkGrid)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_FOOTSTEPS"), &MapRenderState::showFootsteps)) {
                LanguageManager::SaveConfig();
            }
            if (ImGui::Checkbox(LanguageManager::GetText("SHOW_ZOOM_BUTTONS"), &MapRenderState::showZoomButtons)) {
                LanguageManager::SaveConfig();
            }

            const char* arrowColors[] = {
                LanguageManager::GetText("COLOR_RED"),
                LanguageManager::GetText("COLOR_WHITE"),
                LanguageManager::GetText("COLOR_GREEN"),
                LanguageManager::GetText("COLOR_BLUE"),
                LanguageManager::GetText("COLOR_YELLOW"),
                LanguageManager::GetText("COLOR_PURPLE"),
                LanguageManager::GetText("COLOR_BLACK"),
                LanguageManager::GetText("COLOR_CYAN")
            };
            int curArrowCol = std::clamp(MapRenderState::playerArrowColor, 0, 7);
            {
                float btnSize = ImGui::GetFrameHeight();
                float totalAvail = ImGui::GetContentRegionAvail().x;
                float labelW = ImGui::CalcTextSize(LanguageManager::GetText("ARROW_COLOR")).x;
                float comboW = std::clamp(totalAvail - labelW - btnSize - 16.0f * curScale, 110.0f * curScale, 180.0f * curScale);

                ImGui::SetNextItemWidth(comboW);
                if (ImGui::Combo("##PlayerArrowColorCombo", &curArrowCol, arrowColors, 8)) {
                    MapRenderState::playerArrowColor = curArrowCol;
                    LanguageManager::SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button("\u21BA##PlayerArrowColorReset", ImVec2(btnSize, btnSize))) {
                    MapRenderState::playerArrowColor = 0;
                    LanguageManager::SaveConfig();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s (%s)", LanguageManager::GetText("RESET"), LanguageManager::GetText("COLOR_RED"));
                }
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("ARROW_COLOR"));
            }
            {
                float btnSize = ImGui::GetFrameHeight();
                float totalAvail = ImGui::GetContentRegionAvail().x;
                float labelW = ImGui::CalcTextSize(LanguageManager::GetText("PLAYER_ARROW_SCALE")).x;
                float sliderW = std::clamp(totalAvail - labelW - btnSize - 16.0f * curScale, 110.0f * curScale, 180.0f * curScale);

                float pScale = MapRenderState::playerArrowScale;
                ImGui::SetNextItemWidth(sliderW);
                if (ImGui::SliderFloat("##PlayerArrowScaleBigSlider", &pScale, 0.5f, 2.0f, "%.2fx")) {
                    MapRenderState::playerArrowScale = pScale;
                    LanguageManager::SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button("\u21BA##PlayerArrowScaleBigReset", ImVec2(btnSize, btnSize))) {
                    MapRenderState::playerArrowScale = 1.0f;
                    LanguageManager::SaveConfig();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s (1.00x)", LanguageManager::GetText("RESET"));
                }
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("PLAYER_ARROW_SCALE"));
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 2. 地图光影与地形渲染设置 (3D 浮雕 / 坡度 / 深度)
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("MAP_SHADING_SETTINGS"));
            ImGui::Separator();

            const char* slopeModes[] = {
                LanguageManager::GetText("TERRAIN_SLOPES_DEFAULT_2D"),
                LanguageManager::GetText("TERRAIN_SLOPES_LEGACY"),
                LanguageManager::GetText("TERRAIN_SLOPES_DEFAULT_3D")
            };
            int curSlopeMode = std::clamp(MapRenderState::terrainSlopes, 0, 2);
            {
                float btnSize = ImGui::GetFrameHeight();
                float totalAvail = ImGui::GetContentRegionAvail().x;
                float labelW = ImGui::CalcTextSize(LanguageManager::GetText("TERRAIN_SLOPES")).x;
                float comboW = std::clamp(totalAvail - labelW - btnSize - 16.0f * curScale, 120.0f * curScale, 190.0f * curScale);

                ImGui::SetNextItemWidth(comboW);
                if (ImGui::Combo("##TerrainSlopesCombo", &curSlopeMode, slopeModes, 3)) {
                    MapRenderState::terrainSlopes = curSlopeMode;
                    MapRenderState::clearGPUCache.store(true);
                    g_mapDataUpdated.store(true);
                    LanguageManager::SaveConfig();
                }
                ImGui::SameLine();
                if (ImGui::Button("\u21BA##TerrainSlopesReset", ImVec2(btnSize, btnSize))) {
                    MapRenderState::terrainSlopes = 2;
                    MapRenderState::clearGPUCache.store(true);
                    g_mapDataUpdated.store(true);
                    LanguageManager::SaveConfig();
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s (%s)", LanguageManager::GetText("RESET"), LanguageManager::GetText("TERRAIN_SLOPES_DEFAULT_3D"));
                }
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("TERRAIN_SLOPES"));
            }

            if (ImGui::Checkbox(LanguageManager::GetText("TERRAIN_DEPTH"), &MapRenderState::terrainDepth)) {
                MapRenderState::clearGPUCache.store(true);
                g_mapDataUpdated.store(true);
                LanguageManager::SaveConfig();
            }

            if (ImGui::Checkbox(LanguageManager::GetText("ADJUST_HEIGHT_SHORT_BLOCKS"), &MapRenderState::adjustHeightForShortBlocks)) {
                MapRenderState::clearGPUCache.store(true);
                g_mapDataUpdated.store(true);
                LanguageManager::SaveConfig();
            }

            ImGui::Spacing();
        }
        ImGui::End();
    }

    inline void RenderEditModal(const char* modalId, std::string& wpId, bool& trigger) {
        if (trigger) {
            ImGui::OpenPopup(modalId);
            trigger = false;
        }

        float curScale = MapRenderState::globalUIScale;
        if (curScale < 0.1f) curScale = 1.0f;

        bool isOpen = true;
        // 传入 &isOpen 以在右上角渲染出打叉关闭按钮
        if (ImGui::BeginPopupModal(modalId, &isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
            static char nameBuf[256] = "";
            static int  pos[3]      = {0, 0, 0};
            static float col[3]     = {1.0f, 1.0f, 1.0f};
            static int  rgb[3]      = {255, 255, 255};
            static bool showOnMap   = true;
            static bool isPinned    = false;
            static char folderBuf[128] = "";
            static bool initialized = false;

            Waypoint targetWp;
            bool found = false;
            {
                std::lock_guard<std::mutex> lock(WaypointManager::g_wpMutex);
                for (auto& w : WaypointManager::g_waypoints) {
                    if (w.id == wpId) { targetWp = w; found = true; break; }
                }
            }

            if (!found) {
                ImGui::CloseCurrentPopup();
            } else {
                if (!initialized) {
                    snprintf(nameBuf, sizeof(nameBuf), "%s", targetWp.name.c_str());
                    pos[0] = targetWp.x; pos[1] = targetWp.y; pos[2] = targetWp.z;
                    col[0] = targetWp.r; col[1] = targetWp.g; col[2] = targetWp.b;
                    rgb[0] = (int)(col[0] * 255.0f);
                    rgb[1] = (int)(col[1] * 255.0f);
                    rgb[2] = (int)(col[2] * 255.0f);
                    showOnMap = targetWp.enabled;
                    isPinned = targetWp.pinned;
                    snprintf(folderBuf, sizeof(folderBuf), "%s", targetWp.folder.c_str());
                    initialized = true;
                }

                ImGui::PushItemWidth(180.0f * curScale);
                ImGui::InputText("##EditWPInput", nameBuf, sizeof(nameBuf));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("\xe2\x9c\x8e##EditWP")) { // U+270E Edit
                    NativeIME::Open(nameBuf, sizeof(nameBuf), LanguageManager::GetText("WP_NAME"));
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("NATIVE_IME_TOOLTIP"));
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("WP_NAME"));

                ImGui::InputInt3("X / Y / Z", pos);
                if (ImGui::ColorEdit3(LanguageManager::GetText("WP_COLOR"), col)) {
                    rgb[0] = (int)(col[0] * 255.0f);
                    rgb[1] = (int)(col[1] * 255.0f);
                    rgb[2] = (int)(col[2] * 255.0f);
                }
                if (ImGui::InputInt3("R / G / B", rgb)) {
                    rgb[0] = rgb[0] < 0 ? 0 : (rgb[0] > 255 ? 255 : rgb[0]);
                    rgb[1] = rgb[1] < 0 ? 0 : (rgb[1] > 255 ? 255 : rgb[1]);
                    rgb[2] = rgb[2] < 0 ? 0 : (rgb[2] > 255 ? 255 : rgb[2]);
                    col[0] = (float)rgb[0] / 255.0f;
                    col[1] = (float)rgb[1] / 255.0f;
                    col[2] = (float)rgb[2] / 255.0f;
                }

                // 文件夹输入与选择
                ImGui::PushItemWidth(180.0f * curScale);
                ImGui::InputText("##EditWPFolder", folderBuf, sizeof(folderBuf));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("\xe2\x9c\x8e##EditFolderIME")) {
                    NativeIME::Open(folderBuf, sizeof(folderBuf), LanguageManager::GetText("WP_FOLDER"));
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("NATIVE_IME_TOOLTIP"));
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("WP_FOLDER"));

                auto existingFolders = WaypointManager::GetFolders();
                if (!existingFolders.empty()) {
                    std::string fPreview = folderBuf[0] ? folderBuf : LanguageManager::GetText("WP_FOLDER_NONE");
                    ImGui::PushItemWidth(180.0f * curScale);
                    if (ImGui::BeginCombo("##EditWPFolderCombo", fPreview.c_str())) {
                        if (ImGui::Selectable(LanguageManager::GetText("WP_FOLDER_NONE"), folderBuf[0] == '\0')) {
                            folderBuf[0] = '\0';
                        }
                        for (const auto& ef : existingFolders) {
                            bool isSel = (ef == folderBuf);
                            if (ImGui::Selectable(ef.c_str(), isSel)) {
                                snprintf(folderBuf, sizeof(folderBuf), "%s", ef.c_str());
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                }

                ImGui::Checkbox(LanguageManager::GetText("WP_SHOW_ON_MAP"), &showOnMap);
                ImGui::SameLine(180.0f * curScale);
                ImGui::Checkbox(LanguageManager::GetText("WP_PIN"), &isPinned);

                ImGui::Spacing();
                if (ImGui::Button(LanguageManager::GetText("WP_SAVE"), ImVec2(120.0f * curScale, 0))) {
                    WaypointManager::UpdateWaypoint(wpId, nameBuf, pos[0], pos[1], pos[2], col[0], col[1], col[2], showOnMap, isPinned, folderBuf);
                    ImGui::CloseCurrentPopup();
                    initialized = false;
                    NativeIME::Close();
                }
                ImGui::SameLine();
                if (ImGui::Button(LanguageManager::GetText("WP_CANCEL"), ImVec2(120.0f * curScale, 0))) {
                    ImGui::CloseCurrentPopup();
                    initialized = false;
                    NativeIME::Close();
                }
            }

            // 状态联动：如果玩家点击了右上角的 [X] 打叉按钮
            if (!isOpen) {
                ImGui::CloseCurrentPopup();
                initialized = false;
                NativeIME::Close();
            }

            ImGui::EndPopup();
        }
    }

    inline std::optional<ChiyanMap::WorldGen::BlockRect> g_seedMapVisibleBounds;
    inline std::optional<ChiyanMap::WorldGen::Dimension> g_seedMapVisibleDimension;

    inline const char* SeedMapDimensionText(ChiyanMap::WorldGen::Dimension dimension) {
        switch (dimension) {
        case ChiyanMap::WorldGen::Dimension::Overworld: return LanguageManager::GetText("DIM_OVERWORLD");
        case ChiyanMap::WorldGen::Dimension::Nether: return LanguageManager::GetText("DIM_NETHER");
        case ChiyanMap::WorldGen::Dimension::End: return LanguageManager::GetText("DIM_END");
        }
        return LanguageManager::GetText("DIM_UNKNOWN");
    }

    inline const char* SeedMapLayerTextKey(ChiyanMap::WorldGen::LayerId layer) {
        using ChiyanMap::WorldGen::LayerId;
        switch (layer) {
        case LayerId::Village: return "SEED_MAP_LAYER_VILLAGE";
        case LayerId::PillagerOutpost: return "SEED_MAP_LAYER_PILLAGER_OUTPOST";
        case LayerId::DesertPyramid: return "SEED_MAP_LAYER_DESERT_PYRAMID";
        case LayerId::JungleTemple: return "SEED_MAP_LAYER_JUNGLE_TEMPLE";
        case LayerId::SwampHut: return "SEED_MAP_LAYER_SWAMP_HUT";
        case LayerId::Igloo: return "SEED_MAP_LAYER_IGLOO";
        case LayerId::WoodlandMansion: return "SEED_MAP_LAYER_WOODLAND_MANSION";
        case LayerId::OceanMonument: return "SEED_MAP_LAYER_OCEAN_MONUMENT";
        case LayerId::OceanRuin: return "SEED_MAP_LAYER_OCEAN_RUIN";
        case LayerId::Shipwreck: return "SEED_MAP_LAYER_SHIPWRECK";
        case LayerId::BuriedTreasure: return "SEED_MAP_LAYER_BURIED_TREASURE";
        case LayerId::RuinedPortal: return "SEED_MAP_LAYER_RUINED_PORTAL";
        case LayerId::RuinedPortalNether: return "SEED_MAP_LAYER_RUINED_PORTAL_NETHER";
        case LayerId::Stronghold: return "SEED_MAP_LAYER_STRONGHOLD";
        case LayerId::Mineshaft: return "SEED_MAP_LAYER_MINESHAFT";
        case LayerId::AncientCity: return "SEED_MAP_LAYER_ANCIENT_CITY";
        case LayerId::TrailRuins: return "SEED_MAP_LAYER_TRAIL_RUINS";
        case LayerId::TrialChambers: return "SEED_MAP_LAYER_TRIAL_CHAMBERS";
        case LayerId::NetherFortress: return "SEED_MAP_LAYER_NETHER_FORTRESS";
        case LayerId::BastionRemnant: return "SEED_MAP_LAYER_BASTION_REMNANT";
        case LayerId::NetherFossil: return "SEED_MAP_LAYER_NETHER_FOSSIL";
        case LayerId::EndCity: return "SEED_MAP_LAYER_END_CITY";
        case LayerId::EndGateway: return "SEED_MAP_LAYER_END_GATEWAY";
        case LayerId::OverworldBiomes: return "SEED_MAP_LAYER_OVERWORLD_BIOMES";
        case LayerId::NetherBiomes: return "SEED_MAP_LAYER_NETHER_BIOMES";
        case LayerId::EndBiomes: return "SEED_MAP_LAYER_END_BIOMES";
        case LayerId::SlimeChunks: return "SEED_MAP_LAYER_SLIME_CHUNKS";
        case LayerId::OverworldOres: return "SEED_MAP_LAYER_OVERWORLD_ORES";
        case LayerId::NetherOres: return "SEED_MAP_LAYER_NETHER_ORES";
        case LayerId::OverworldUndergroundFeatures: return "SEED_MAP_LAYER_UNDERGROUND_FEATURES";
        case LayerId::DungeonSpawner: return "SEED_MAP_LAYER_DUNGEON_SPAWNER";
        case LayerId::WorldSpawn: return "SEED_MAP_LAYER_WORLD_SPAWN";
        case LayerId::MushroomFields: return "SEED_MAP_LAYER_MUSHROOM_FIELDS";
        case LayerId::CherryGrove: return "SEED_MAP_LAYER_CHERRY_GROVE";
        case LayerId::Badlands: return "SEED_MAP_LAYER_BADLANDS";
        case LayerId::IceSpikes: return "SEED_MAP_LAYER_ICE_SPIKES";
        case LayerId::MangroveSwamp: return "SEED_MAP_LAYER_MANGROVE_SWAMP";
        case LayerId::PaleGarden: return "SEED_MAP_LAYER_PALE_GARDEN";
        case LayerId::SoulSandValley: return "SEED_MAP_LAYER_SOUL_SAND_VALLEY";
        case LayerId::CrimsonForest: return "SEED_MAP_LAYER_CRIMSON_FOREST";
        case LayerId::WarpedForest: return "SEED_MAP_LAYER_WARPED_FOREST";
        case LayerId::BasaltDeltas: return "SEED_MAP_LAYER_BASALT_DELTAS";
        case LayerId::EndSmallIslands: return "SEED_MAP_LAYER_END_SMALL_ISLANDS";
        case LayerId::EndMidlands: return "SEED_MAP_LAYER_END_MIDLANDS";
        case LayerId::EndHighlands: return "SEED_MAP_LAYER_END_HIGHLANDS";
        case LayerId::EndBarrens: return "SEED_MAP_LAYER_END_BARRENS";
        case LayerId::AbandonedCamp: return "SEED_MAP_LAYER_ABANDONED_CAMP";
        case LayerId::SulfurCaves: return "SEED_MAP_LAYER_SULFUR_CAVES";
        case LayerId::DappledForest: return "SEED_MAP_LAYER_DAPPLED_FOREST";
        }
        return "SEED_MAP_UNKNOWN_LAYER";
    }

    inline const char* SeedMapLayerText(ChiyanMap::WorldGen::LayerId layer) {
        return LanguageManager::GetText(SeedMapLayerTextKey(layer));
    }

    inline ImU32 SeedMapMarkerColor(ChiyanMap::WorldGen::LayerId layer) {
        if (const auto* icon = ChiyanMap::SeedMapIcons::FindIcon(layer)) {
            return IM_COL32(icon->accent.r, icon->accent.g, icon->accent.b, 255);
        }
        using ChiyanMap::WorldGen::LayerId;
        switch (layer) {
        case LayerId::OverworldBiomes:
        case LayerId::NetherBiomes:
        case LayerId::EndBiomes:
            return IM_COL32(64, 196, 112, 255);
        case LayerId::SlimeChunks:
            return IM_COL32(105, 214, 99, 255);
        case LayerId::OverworldOres:
        case LayerId::NetherOres:
            return IM_COL32(235, 186, 68, 255);
        case LayerId::OverworldUndergroundFeatures:
        case LayerId::DungeonSpawner:
            return IM_COL32(178, 112, 228, 255);
        case LayerId::WorldSpawn:
            return IM_COL32(240, 240, 240, 255);
        default:
            return IM_COL32(74, 146, 226, 255);
        }
    }

    inline ImVec4 SeedMapMarkerColorFloat(ChiyanMap::WorldGen::LayerId layer) {
        const ImVec4 color = ImGui::ColorConvertU32ToFloat4(SeedMapMarkerColor(layer));
        return ImVec4(color.x, color.y, color.z, 1.0f);
    }

    inline ImU32 SeedMapIconColor(ChiyanMap::SeedMapIcons::IconColor color) {
        return IM_COL32(color.r, color.g, color.b, 255);
    }

    inline void DrawSeedMapIcon(
        ImDrawList* drawList,
        ChiyanMap::WorldGen::LayerId layer,
        ImVec2 topLeft,
        float size
    ) {
        const auto* icon = ChiyanMap::SeedMapIcons::FindIcon(layer);
        if (icon == nullptr) {
            const float fallbackHalf = std::max(2.0f, size * 0.125f);
            const ImVec2 center(topLeft.x + size * 0.5f, topLeft.y + size * 0.5f);
            drawList->AddRectFilled(
                ImVec2(center.x - fallbackHalf, center.y - fallbackHalf),
                ImVec2(center.x + fallbackHalf, center.y + fallbackHalf),
                SeedMapMarkerColor(layer)
            );
            return;
        }

        const float pixelScale = size / static_cast<float>(ChiyanMap::SeedMapIcons::kIconSizePixels);
        for (const auto& pixel : icon->pixels) {
            const ImVec2 pixelMin(
                topLeft.x + static_cast<float>(pixel.x) * pixelScale,
                topLeft.y + static_cast<float>(pixel.y) * pixelScale
            );
            drawList->AddRectFilled(
                pixelMin,
                ImVec2(
                    pixelMin.x + static_cast<float>(pixel.width) * pixelScale,
                    pixelMin.y + static_cast<float>(pixel.height) * pixelScale
                ),
                SeedMapIconColor(pixel.color)
            );
        }
    }

    inline std::optional<ChiyanMap::WorldGen::BlockRect> MakeSeedMapVisibleBounds(
        float minX,
        float minZ,
        float maxX,
        float maxZ
    ) {
        if (!std::isfinite(minX) || !std::isfinite(minZ) || !std::isfinite(maxX) || !std::isfinite(maxZ)) {
            return std::nullopt;
        }

        const double floorX = std::floor(static_cast<double>(minX));
        const double floorZ = std::floor(static_cast<double>(minZ));
        const double ceilX = std::ceil(static_cast<double>(maxX));
        const double ceilZ = std::ceil(static_cast<double>(maxZ));
        constexpr double minInt64 = static_cast<double>(std::numeric_limits<std::int64_t>::min());
        constexpr double maxInt64 = static_cast<double>(std::numeric_limits<std::int64_t>::max());
        if (floorX < minInt64 || floorZ < minInt64 || ceilX >= maxInt64 || ceilZ >= maxInt64) return std::nullopt;

        ChiyanMap::WorldGen::BlockRect bounds{
            static_cast<std::int64_t>(floorX),
            static_cast<std::int64_t>(floorZ),
            static_cast<std::int64_t>(ceilX),
            static_cast<std::int64_t>(ceilZ)
        };
        return bounds.IsValid() ? std::optional{bounds} : std::nullopt;
    }

    inline void RenderSeedMapMarkers(
        ImDrawList* drawList,
        float cx,
        float cy,
        float minWorldX,
        float minWorldZ,
        float maxWorldX,
        float maxWorldZ,
        int viewDim
    ) {
        const auto currentDimension = SeedMapManager::DimensionFromGameDimensionId(viewDim);
        const auto visibleBounds = MakeSeedMapVisibleBounds(minWorldX, minWorldZ, maxWorldX, maxWorldZ);
        g_seedMapVisibleBounds = visibleBounds;
        g_seedMapVisibleDimension = currentDimension;
        if (!visibleBounds || !currentDimension) return;

        const auto settings = SeedMapManager::GetSettings();
        if (!settings.enabled || settings.targetDimension != *currentDimension) return;

        SeedMapManager::RequestVisibleMap(*visibleBounds);
        const auto markers = SeedMapManager::GetMarkersInBounds(*visibleBounds, *currentDimension);
        const auto selected = SeedMapManager::GetSelectedMarker();
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        for (const auto& marker : markers) {
            const auto block = marker.Block();
            if (marker.Kind() == ChiyanMap::WorldGen::MarkerKind::ChunkRegion) {
                const auto chunk = marker.Chunk();
                const float minScreenX = cx + (static_cast<float>(chunk.x * 16) - g_smoothPX) * MapRenderState::bigMapZoom
                    + MapRenderState::bigMapOffsetX;
                const float minScreenY = cy + (static_cast<float>(chunk.z * 16) - g_smoothPZ) * MapRenderState::bigMapZoom
                    + MapRenderState::bigMapOffsetZ;
                const float maxScreenX = minScreenX + 16.0f * MapRenderState::bigMapZoom;
                const float maxScreenY = minScreenY + 16.0f * MapRenderState::bigMapZoom;
                const ImVec2 topLeft(std::min(minScreenX, maxScreenX), std::min(minScreenY, maxScreenY));
                const ImVec2 bottomRight(std::max(minScreenX, maxScreenX), std::max(minScreenY, maxScreenY));
                if (bottomRight.x < 0.0f || topLeft.x > ImGui::GetIO().DisplaySize.x
                    || bottomRight.y < 0.0f || topLeft.y > ImGui::GetIO().DisplaySize.y) {
                    continue;
                }

                const ImU32 accent = SeedMapMarkerColor(marker.Layer());
                drawList->AddRectFilled(topLeft, bottomRight, IM_COL32(105, 214, 99, 58));
                drawList->AddRect(topLeft, bottomRight, IM_COL32(105, 214, 99, 170));
                const bool isSelected = selected && ChiyanMap::WorldGen::GetMarkerKey(*selected)
                    == ChiyanMap::WorldGen::GetMarkerKey(marker);
                if (isSelected) drawList->AddRect(topLeft, bottomRight, IM_COL32(232, 168, 74, 255), 0.0f, 0, 2.0f);

                const bool hovered = mouse.x >= topLeft.x && mouse.x <= bottomRight.x
                    && mouse.y >= topLeft.y && mouse.y <= bottomRight.y;
                if (hovered) {
                    drawList->AddRect(ImVec2(topLeft.x - 1.0f, topLeft.y - 1.0f),
                                      ImVec2(bottomRight.x + 1.0f, bottomRight.y + 1.0f), accent, 0.0f, 0, 2.0f);
                    if (MapRenderState::showSeedMap && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        SeedMapManager::SelectMarker(marker);
                    }
                }
                continue;
            }
            const float screenX = cx + (static_cast<float>(block.x) - g_smoothPX) * MapRenderState::bigMapZoom
                + MapRenderState::bigMapOffsetX;
            const float screenY = cy + (static_cast<float>(block.z) - g_smoothPZ) * MapRenderState::bigMapZoom
                + MapRenderState::bigMapOffsetZ;
            constexpr float referenceZoom = 3.0f;
            constexpr float referenceMarkerSize = 32.0f;
            const float markerSize = std::clamp(
                referenceMarkerSize * std::sqrt(std::max(MapRenderState::bigMapZoom, 0.0f) / referenceZoom),
                20.0f,
                64.0f
            );
            const float halfMarkerSize = markerSize * 0.5f;
            if (screenX < -halfMarkerSize || screenX > ImGui::GetIO().DisplaySize.x + halfMarkerSize
                || screenY < -halfMarkerSize || screenY > ImGui::GetIO().DisplaySize.y + halfMarkerSize) {
                continue;
            }

            const ImVec2 topLeft(screenX - halfMarkerSize, screenY - halfMarkerSize);
            const ImVec2 bottomRight(screenX + halfMarkerSize, screenY + halfMarkerSize);
            const ImU32 accent = SeedMapMarkerColor(marker.Layer());
            DrawSeedMapIcon(drawList, marker.Layer(), topLeft, markerSize);

            const bool isSelected = selected && ChiyanMap::WorldGen::GetMarkerKey(*selected)
                == ChiyanMap::WorldGen::GetMarkerKey(marker);
            if (isSelected) drawList->AddRect(topLeft, bottomRight, IM_COL32(232, 168, 74, 255), 0.0f, 0, 2.0f);

            const bool hovered = mouse.x >= topLeft.x && mouse.x <= bottomRight.x
                && mouse.y >= topLeft.y && mouse.y <= bottomRight.y;
            if (hovered) {
                drawList->AddRect(ImVec2(topLeft.x - 2.0f, topLeft.y - 2.0f),
                                  ImVec2(bottomRight.x + 2.0f, bottomRight.y + 2.0f), accent, 0.0f, 0, 1.0f);
                if (MapRenderState::showSeedMap && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    SeedMapManager::SelectMarker(marker);
                }
            }
        }
    }

    [[nodiscard]] inline std::vector<ChiyanMap::WorldGen::LayerId> CollectEnabledSeedMapLayers(
        ChiyanMap::WorldGen::Dimension dimension
    ) {
        std::vector<ChiyanMap::WorldGen::LayerId> layers;
        const auto settings = SeedMapManager::GetSettings();
        if (!settings.enabled || settings.targetDimension != dimension) return layers;

        for (const auto& descriptor : ChiyanMap::WorldGen::LayerCatalog()) {
            const auto index = static_cast<std::size_t>(descriptor.id);
            if (descriptor.dimension != dimension
                || !ChiyanMap::WorldGen::IsMarkerEmissionAllowed(descriptor.support)
                || index >= settings.enabledLayers.size()
                || !settings.enabledLayers[index]) {
                continue;
            }
            layers.push_back(descriptor.id);
        }
        return layers;
    }

    struct SeedMapLegendLayout final {
        ImVec2 position{};
        ImVec2 size{};
    };

    [[nodiscard]] inline std::optional<SeedMapLegendLayout> MakeSeedMapLegendLayout(int viewDim) {
        const auto dimension = SeedMapManager::DimensionFromGameDimensionId(viewDim);
        if (!dimension) return std::nullopt;
        const auto layers = CollectEnabledSeedMapLayers(*dimension);
        if (layers.empty()) return std::nullopt;

        constexpr float width = 226.0f;
        constexpr float right = 18.0f;
        constexpr float bottom = 18.0f;
        constexpr float rowHeight = 28.0f;
        constexpr float headerHeight = 34.0f;
        const ImGuiIO& io = ImGui::GetIO();
        const float maximumHeight = std::max(112.0f, io.DisplaySize.y - bottom * 2.0f);
        const float height = std::min(maximumHeight, headerHeight + rowHeight * static_cast<float>(layers.size()));
        const float y = std::max(bottom, io.DisplaySize.y - bottom - height);
        return SeedMapLegendLayout{ImVec2(io.DisplaySize.x - width - right, y), ImVec2(width, height)};
    }

    [[nodiscard]] inline bool IsMouseOverSeedMapLegend(int viewDim) {
        const auto layout = MakeSeedMapLegendLayout(viewDim);
        if (!layout) return false;
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        return mouse.x >= layout->position.x && mouse.x <= layout->position.x + layout->size.x
            && mouse.y >= layout->position.y && mouse.y <= layout->position.y + layout->size.y;
    }

    inline void RenderSeedMapLegend(int viewDim) {
        const auto layout = MakeSeedMapLegendLayout(viewDim);
        if (!layout) return;
        const auto dimension = SeedMapManager::DimensionFromGameDimensionId(viewDim);
        if (!dimension) return;
        const auto layers = CollectEnabledSeedMapLayers(*dimension);
        if (layers.empty()) return;

        ImGui::SetCursorScreenPos(layout->position);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, OreColor(25, 25, 25, 235));
        ImGui::PushStyleColor(ImGuiCol_Border, OreColor(100, 100, 100));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
        if (ImGui::BeginChild("##SeedMapLegend", layout->size, true, ImGuiWindowFlags_NoNavFocus)) {
            ImGui::TextColored(OreColor(234, 197, 79), "%s",
                               LanguageManager::GetText("SEED_MAP_LEGEND"));
            ImGui::Separator();
            for (const auto layer : layers) {
                ImGui::PushID(static_cast<int>(layer));
                const ImVec2 iconPosition = ImGui::GetCursorScreenPos();
                DrawSeedMapIcon(ImGui::GetWindowDrawList(), layer, iconPosition, 20.0f);
                ImGui::Dummy(ImVec2(24.0f, 20.0f));
                ImGui::SameLine(0.0f, 7.0f);
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted(SeedMapLayerText(layer));
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }

    // [全屏大地图多维度精确定位核心引擎]
    // 支持死亡记录、普通路径点、种子结构地图的跨维度定位与无缝跳转。
    // 自动保存源维度视角，切换地图缓存目录至目标维度，彻底清理旧维度 GPU 贴图缓存，
    // 精准居中目标坐标并记录目标维度视角，防止跨维度贴图与标记污染。
    inline void LocateBigMapOn(int targetDim, float targetWx, float targetWz, float smoothPX, float smoothPZ) {
        if (targetDim < 0 || targetDim > 2) return;

        int oldViewDim = MapRenderState::GetEffectiveViewDimensionId();
        if (oldViewDim != targetDim) {
            MapRenderState::SaveDimCamera(oldViewDim, smoothPX, smoothPZ);
            if (MapCacheManager::GetLoadedDimensionId() != targetDim && !MapRenderState::currentWorldId.empty()) {
                MapCacheManager::SwitchViewDimension(targetDim);
                MapRenderState::clearGPUCache.store(true);
            }
        }

        MapRenderState::bigMapViewDimensionId = targetDim;
        MapRenderState::bigMapOffsetX = -(targetWx - smoothPX) * MapRenderState::bigMapZoom;
        MapRenderState::bigMapOffsetZ = -(targetWz - smoothPZ) * MapRenderState::bigMapZoom;
        MapRenderState::SaveDimCamera(targetDim, smoothPX, smoothPZ);
        MapRenderState::showBigMap = true;
        MapRenderState::hoverBiomeAlpha = 0.0f;
        MapRenderState::hoverBiomeTargetAlpha = 0.0f;
        MapRenderState::hoverBiomeHasValidResult = false;
        MapCacheManager::InvalidateExportPreview();
    }

    inline void CenterSeedMapOnMarker(const ChiyanMap::WorldGen::SeedMapMarker& marker) {
        const auto block = marker.Block();
        int targetDim = static_cast<int>(marker.TargetDimension());
        LocateBigMapOn(targetDim, static_cast<float>(block.x) + 0.5f, static_cast<float>(block.z) + 0.5f, g_smoothPX, g_smoothPZ);
    }

    inline void SaveSeedMapMarkerWaypoint(const ChiyanMap::WorldGen::SeedMapMarker& marker) {
        const auto block = marker.Block();
        const auto clampCoordinate = [](std::int64_t value) {
            return static_cast<int>(std::clamp<std::int64_t>(
                value,
                static_cast<std::int64_t>(std::numeric_limits<int>::min()),
                static_cast<std::int64_t>(std::numeric_limits<int>::max())
            ));
        };
        const auto* descriptor = ChiyanMap::WorldGen::FindLayerDescriptor(marker.Layer());
        const std::string name = descriptor == nullptr
            ? std::string(LanguageManager::GetText("SEED_MAP_UNKNOWN_LAYER"))
            : std::string(SeedMapLayerText(marker.Layer()));
        const ImVec4 color = SeedMapMarkerColorFloat(marker.Layer());
        int targetDim = static_cast<int>(marker.TargetDimension());
        WaypointManager::AddWaypoint(
            name,
            clampCoordinate(block.x),
            (targetDim == 1) ? 64 : static_cast<int>(std::floor(g_playerY)),
            clampCoordinate(block.z),
            color.x,
            color.y,
            color.z,
            targetDim
        );
    }

    inline void RenderSeedMapLayerControls() {
        auto settings = SeedMapManager::GetSettings();
        ImGui::TextColored(OreColor(234, 197, 79), "%s", LanguageManager::GetText("SEED_MAP_LAYERS"));
        if (ImGui::Button(LanguageManager::GetText("SEED_MAP_CLEAR_ALL"), ImVec2(-1.0f, 0.0f))) {
            if (SeedMapManager::ClearAllLayers()) {
                LanguageManager::SaveConfig();
                settings = SeedMapManager::GetSettings();
            }
        }
        for (const auto& descriptor : ChiyanMap::WorldGen::LayerCatalog()) {
            if (descriptor.dimension != settings.targetDimension) continue;

            ImGui::PushID(static_cast<int>(descriptor.id));
            const bool queryable = ChiyanMap::WorldGen::IsMarkerEmissionAllowed(descriptor.support);
            bool enabled = SeedMapManager::IsLayerEnabled(descriptor.id);
            if (!queryable) ImGui::BeginDisabled();
            if (ImGui::Checkbox(SeedMapLayerText(descriptor.id), &enabled) && queryable) {
                SeedMapManager::SetLayerEnabled(descriptor.id, enabled);
                LanguageManager::SaveConfig();
            }
            if (!queryable) ImGui::EndDisabled();
            ImGui::SameLine();
            const char* supportLabel = descriptor.support == ChiyanMap::WorldGen::LayerSupportState::FixtureVerified
                ? LanguageManager::GetText("SEED_MAP_VERIFIED")
                : descriptor.support == ChiyanMap::WorldGen::LayerSupportState::CubiomesReference
                    ? LanguageManager::GetText("SEED_MAP_CUBIOMES_REFERENCE")
                    : descriptor.support == ChiyanMap::WorldGen::LayerSupportState::AlgorithmAwaitingFixture
                        ? LanguageManager::GetText("SEED_MAP_AWAITING_FIXTURE")
                        : LanguageManager::GetText("SEED_MAP_UNAVAILABLE_LAYER");
            ImGui::TextDisabled("%s", supportLabel);
            ImGui::PopID();
        }
    }

    inline void RenderSeedMapResults() {
        const auto settings = SeedMapManager::GetSettings();
        std::vector<ChiyanMap::WorldGen::SeedMapMarker> markers;
        if (settings.searchMode == SeedMapManager::SearchMode::Manual) {
            markers = SeedMapManager::GetManualMarkers();
        } else if (g_seedMapVisibleBounds && g_seedMapVisibleDimension
                   && settings.targetDimension == *g_seedMapVisibleDimension) {
            markers = SeedMapManager::GetMarkersInBounds(*g_seedMapVisibleBounds, *g_seedMapVisibleDimension);
        }

        ImGui::TextColored(OreColor(234, 197, 79), "%s", LanguageManager::GetText("SEED_MAP_RESULTS"));
        if (markers.empty()) {
            ImGui::TextDisabled("%s", LanguageManager::GetText("SEED_MAP_NO_RESULTS"));
            return;
        }

        const auto selected = SeedMapManager::GetSelectedMarker();
        ImGui::BeginChild("##SeedMapResults", ImVec2(0.0f, 112.0f * MapRenderState::globalUIScale), true, ImGuiWindowFlags_NoScrollbar);
        for (const auto& marker : markers) {
            const auto* descriptor = ChiyanMap::WorldGen::FindLayerDescriptor(marker.Layer());
            const char* label = descriptor == nullptr ? LanguageManager::GetText("SEED_MAP_UNKNOWN_LAYER")
                                                       : SeedMapLayerText(marker.Layer());
            const auto block = marker.Block();
            char row[192];
            snprintf(row, sizeof(row), "%s  X %lld  Z %lld", label,
                     static_cast<long long>(block.x), static_cast<long long>(block.z));
            const bool isSelected = selected && ChiyanMap::WorldGen::GetMarkerKey(*selected)
                == ChiyanMap::WorldGen::GetMarkerKey(marker);
            if (ImGui::Selectable(row, isSelected)) SeedMapManager::SelectMarker(marker);
        }
        ImGui::EndChild();
    }

    inline void RenderSeedMapDetail() {
        const auto marker = SeedMapManager::GetSelectedMarker();
        if (!marker) return;

        const auto* descriptor = ChiyanMap::WorldGen::FindLayerDescriptor(marker->Layer());
        const char* name = descriptor == nullptr ? LanguageManager::GetText("SEED_MAP_UNKNOWN_LAYER")
                                                 : SeedMapLayerText(marker->Layer());
        const auto block = marker->Block();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(SeedMapMarkerColorFloat(marker->Layer()), "%s", name);
        ImGui::TextDisabled("%s", descriptor != nullptr
                && descriptor->support == ChiyanMap::WorldGen::LayerSupportState::FixtureVerified
            ? LanguageManager::GetText("SEED_MAP_FIXTURE_LOCATION")
            : LanguageManager::GetText("SEED_MAP_SEED_PREDICTION"));
        if (descriptor != nullptr) ImGui::TextWrapped("%s", std::string(descriptor->detail).c_str());
        ImGui::Text("X %lld   Z %lld", static_cast<long long>(block.x), static_cast<long long>(block.z));

        if (ImGui::Button(LanguageManager::GetText("SEED_MAP_CENTER"), ImVec2(0.0f, 0.0f))) {
            CenterSeedMapOnMarker(*marker);
        }
        ImGui::SameLine();
        if (ImGui::Button(LanguageManager::GetText("SEED_MAP_SAVE_WAYPOINT"), ImVec2(0.0f, 0.0f))) {
            SaveSeedMapMarkerWaypoint(*marker);
        }
    }

    inline void RenderSeedMapPanel() {
        if (!MapRenderState::showSeedMap) return;

        ImGuiIO& io = ImGui::GetIO();
        float curScale = std::clamp(MapRenderState::globalUIScale, 0.25f, 4.0f);
        const float width = 340.0f * curScale;
        const float height = std::clamp(io.DisplaySize.y - 110.0f * curScale, 360.0f * curScale, 760.0f * curScale);
        ImGui::SetNextWindowPos(ImVec2(18.0f * curScale, 70.0f * curScale), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, OreColor(25, 29, 34, 250));
        ImGui::PushStyleColor(ImGuiCol_Border, OreColor(79, 90, 105));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, OreColor(18, 22, 27, 210));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f * curScale);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f * curScale, 16.0f * curScale));

        const char* windowName = LanguageManager::GetText("SEED_MAP_TITLE");
        if (ImGui::Begin(windowName, &MapRenderState::showSeedMap,
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::TextColored(OreColor(234, 197, 79), "%s", LanguageManager::GetText("SEED_MAP_TITLE"));
            ImGui::SameLine();
            ImGui::TextDisabled("%s", LanguageManager::GetText("SEED_MAP_PROFILE"));
            ImGui::Separator();

            const auto status = SeedMapManager::GetStatus();
            ImGui::TextDisabled("%s", LanguageManager::GetText("SEED_MAP_AUTO_SEED"));
            ImGui::SameLine();
            if (status.seedAvailable) {
                if (status.isManual) {
                    ImGui::TextColored(OreColor(79, 195, 247), "%s", LanguageManager::GetText("SEED_MAP_CUSTOM_ACTIVE"));
                } else {
                    ImGui::TextColored(OreColor(33, 204, 76), "%s", LanguageManager::GetText("SEED_MAP_CAPTURED"));
                }
            } else {
                ImGui::TextColored(OreColor(221, 91, 91), "%s", LanguageManager::GetText("SEED_MAP_UNAVAILABLE"));
            }

            if (status.capturedSeedBits) {
                ImGui::TextDisabled("%s", LanguageManager::GetText("SEED_MAP_RAW_SEED"));
                ImGui::SameLine();
                int64_t signedSeed = static_cast<int64_t>(*status.capturedSeedBits);
                ImGui::Text("%lld (0x%016llX)",
                              static_cast<long long>(signedSeed),
                              static_cast<unsigned long long>(*status.capturedSeedBits));
            }

            // 手动种子输入框 + 应用按钮 + 恢复自动按钮
            static char s_manualSeedBuf[64] = "";
            float applyBtnW = 55.0f * curScale;
            float resetBtnW = status.isManual ? (68.0f * curScale) : 0.0f;
            float inputW = ImGui::GetContentRegionAvail().x - applyBtnW - (status.isManual ? (resetBtnW + 8.0f * curScale) : 4.0f * curScale);
            if (inputW > 100.0f) {
                ImGui::PushItemWidth(inputW);
                bool enterPressed = ImGui::InputTextWithHint("##ManualSeedInput", LanguageManager::GetText("SEED_MAP_INPUT_HINT"), s_manualSeedBuf, sizeof(s_manualSeedBuf), ImGuiInputTextFlags_EnterReturnsTrue);
                ImGui::PopItemWidth();
                ImGui::SameLine();
                bool applyClicked = ImGui::Button(LanguageManager::GetText("SEED_MAP_APPLY_SEED"), ImVec2(applyBtnW, 0.0f));
                if (enterPressed || applyClicked) {
                    std::string inputStr = s_manualSeedBuf;
                    while (!inputStr.empty() && (inputStr.front() == ' ' || inputStr.front() == '\t')) inputStr.erase(inputStr.begin());
                    while (!inputStr.empty() && (inputStr.back() == ' ' || inputStr.back() == '\t')) inputStr.pop_back();
                    if (!inputStr.empty()) {
                        uint64_t seedVal = 0;
                        bool parsed = false;
                        try {
                            if (inputStr.rfind("0x", 0) == 0 || inputStr.rfind("0X", 0) == 0) {
                                seedVal = std::stoull(inputStr, nullptr, 16);
                                parsed = true;
                            } else if (inputStr[0] == '-') {
                                int64_t s64 = std::stoll(inputStr);
                                seedVal = static_cast<uint64_t>(s64);
                                parsed = true;
                            } else {
                                seedVal = std::stoull(inputStr);
                                parsed = true;
                            }
                        } catch (...) {
                            parsed = false;
                        }
                        if (!parsed) {
                            int64_t h = 0;
                            for (char ch : inputStr) {
                                h = 31 * h + static_cast<unsigned char>(ch);
                            }
                            seedVal = static_cast<uint64_t>(h);
                        }
                        SeedMapManager::SetManualSeed(seedVal);
                        LanguageManager::SaveConfig();
                        s_manualSeedBuf[0] = '\0';
                    }
                }
                if (status.isManual) {
                    ImGui::SameLine();
                    if (ImGui::Button(LanguageManager::GetText("SEED_MAP_RESET_AUTO"), ImVec2(resetBtnW, 0.0f))) {
                        SeedMapManager::ResetToAutoSeed();
                        LanguageManager::SaveConfig();
                    }
                }
            }

            ImGui::TextDisabled("Candidates %llu  Markers %llu",
                                static_cast<unsigned long long>(status.cachedQueryStatistics.placementCandidates),
                                static_cast<unsigned long long>(status.cachedQueryStatistics.emittedMarkers));
            ImGui::TextDisabled("Biome rejected %llu",
                                static_cast<unsigned long long>(status.cachedQueryStatistics.biomeRejectedCandidates));

            auto settings = SeedMapManager::GetSettings();
            ImGui::TextDisabled("%s", LanguageManager::GetText("SEED_MAP_DIMENSION"));
            ImGui::SameLine();
            if (ImGui::BeginCombo("##SeedMapDimension", SeedMapDimensionText(settings.targetDimension))) {
                constexpr std::array dimensions{
                    ChiyanMap::WorldGen::Dimension::Overworld,
                    ChiyanMap::WorldGen::Dimension::Nether,
                    ChiyanMap::WorldGen::Dimension::End
                };
                for (const auto dimension : dimensions) {
                    const bool selected = dimension == settings.targetDimension;
                    if (ImGui::Selectable(SeedMapDimensionText(dimension), selected)) {
                        settings.targetDimension = dimension;
                        SeedMapManager::SetSettings(settings);
                        LanguageManager::SaveConfig();
                        settings = SeedMapManager::GetSettings();
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Spacing();
            float btnW = (ImGui::GetContentRegionAvail().x - 6.0f * curScale) * 0.5f;
            bool isVis = (settings.searchMode == SeedMapManager::SearchMode::VisibleMap);
            if (isVis) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.45f, 0.85f, 1.0f));
            }
            if (ImGui::Button(LanguageManager::GetText("SEED_MAP_VISIBLE"), ImVec2(btnW, 28.0f * curScale))) {
                settings.searchMode = SeedMapManager::SearchMode::VisibleMap;
                SeedMapManager::SetSettings(settings);
                LanguageManager::SaveConfig();
                settings = SeedMapManager::GetSettings();
            }
            if (isVis) ImGui::PopStyleColor();

            ImGui::SameLine();
            bool isMan = (settings.searchMode == SeedMapManager::SearchMode::Manual);
            if (isMan) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.45f, 0.85f, 1.0f));
            }
            if (ImGui::Button(LanguageManager::GetText("SEED_MAP_MANUAL"), ImVec2(btnW, 28.0f * curScale))) {
                settings.searchMode = SeedMapManager::SearchMode::Manual;
                SeedMapManager::SetSettings(settings);
                LanguageManager::SaveConfig();
                settings = SeedMapManager::GetSettings();
            }
            if (isMan) ImGui::PopStyleColor();

            ImGui::Spacing();
            ImGui::BeginChild("##SeedMapControls", ImVec2(0.0f, (settings.searchMode == SeedMapManager::SearchMode::Manual ? 225.0f : 168.0f) * curScale), false);
            RenderSeedMapLayerControls();
            if (settings.searchMode == SeedMapManager::SearchMode::Manual) {
                ImGui::Separator();
                bool changed = false;
                changed |= ImGui::InputScalar(LanguageManager::GetText("SEED_MAP_SEARCH_X"), ImGuiDataType_S64,
                                              &settings.manualCenterX, nullptr, nullptr, "%lld");
                changed |= ImGui::InputScalar(LanguageManager::GetText("SEED_MAP_SEARCH_Z"), ImGuiDataType_S64,
                                              &settings.manualCenterZ, nullptr, nullptr, "%lld");
                changed |= ImGui::InputScalar(LanguageManager::GetText("SEED_MAP_RADIUS"), ImGuiDataType_S64,
                                              &settings.manualRadius, nullptr, nullptr, "%lld");
                if (changed) {
                    SeedMapManager::SetSettings(settings);
                    LanguageManager::SaveConfig();
                    settings = SeedMapManager::GetSettings();
                }
                if (ImGui::Button(LanguageManager::GetText("SEED_MAP_SEARCH"), ImVec2(-1.0f, 0.0f))) {
                    SeedMapManager::StartManualSearch(settings.manualCenterX, settings.manualCenterZ, settings.manualRadius);
                    LanguageManager::SaveConfig();
                }
            }
            ImGui::EndChild();

            const auto progress = SeedMapManager::GetManualSearchProgress();
            if (settings.searchMode == SeedMapManager::SearchMode::Manual && progress.requested) {
                char progressText[96];
                snprintf(progressText, sizeof(progressText), LanguageManager::GetText("SEED_MAP_PROGRESS"),
                         progress.completedTiles, progress.totalTiles);
                ImGui::TextDisabled("%s", progressText);
                const float fraction = progress.totalTiles == 0 ? 0.0f
                    : static_cast<float>(progress.completedTiles) / static_cast<float>(progress.totalTiles);
                ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f));
            } else if (settings.searchMode == SeedMapManager::SearchMode::VisibleMap) {
                ImGui::TextDisabled("%s", LanguageManager::GetText("SEED_MAP_WORKING"));
            }
            if (status.currentGameDimension && settings.targetDimension != *status.currentGameDimension) {
                ImGui::TextColored(OreColor(234, 197, 79), "%s", LanguageManager::GetText("SEED_MAP_DIMENSION_MISMATCH"));
            }

            ImGui::Separator();
            RenderSeedMapResults();
            RenderSeedMapDetail();
        }
        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(3);
    }

    inline void RenderImGuiBigMap() {
        int currentDim = MapRenderState::currentDimensionId;
        int viewDim = MapRenderState::GetEffectiveViewDimensionId();

        // [自动维度同步与隔离] 若当前大地图查看维度与 MapCacheManager 加载的维度不一致，立即同步切换并清理旧维度 GPU 缓存
        if (MapCacheManager::GetLoadedDimensionId() != viewDim && !MapRenderState::currentWorldId.empty()) {
            MapCacheManager::SwitchViewDimension(viewDim);
            MapRenderState::clearGPUCache.store(true);
        }

        if (MapRenderState::clearGPUCache.load()) {
            for(auto& p : g_regionSRVs) if(p.second) p.second->Release();
            g_regionSRVs.clear();
            for(auto& p : g_regionTextures) if(p.second) p.second->Release();
            g_regionTextures.clear();
            MapRenderState::clearGPUCache.store(false);
        }

        // [主世界地表与洞穴切换] 当查看主世界(viewDim == 0)时，由 bigMapOverworldCave 决定显示地表还是洞穴地图；
        // 下界(viewDim == 1)固定为洞穴模式(isCave = true)；末地(viewDim == 2)为常规地表(isCave = false)。
        bool isCave = (viewDim == 1) || (viewDim == 0 && MapRenderState::bigMapOverworldCave);

        ImGuiIO& io = ImGui::GetIO();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(io.DisplaySize);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | 
                                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | 
                                        ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoNav |
                                        ImGuiWindowFlags_NoBackground;
        
        ImGui::Begin("BigMapCanvas", nullptr, window_flags);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        const bool isHoveringLegend = IsMouseOverSeedMapLegend(viewDim);
        bool isHoveringCanvas = ImGui::IsWindowHovered() && !isHoveringLegend && !MapRenderState::showExportPNGScreen;

        static bool s_isDraggingMap = false;
        static bool s_isSelectingExportBox = false;
        float curHoverWx = g_smoothPX + (io.MousePos.x - io.DisplaySize.x * 0.5f - MapRenderState::bigMapOffsetX) / MapRenderState::bigMapZoom;
        float curHoverWz = g_smoothPZ + (io.MousePos.y - io.DisplaySize.y * 0.5f - MapRenderState::bigMapOffsetZ) / MapRenderState::bigMapZoom;

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && isHoveringCanvas) {
            if (io.KeyShift) {
                s_isSelectingExportBox = true;
                MapRenderState::hasExportSelection = true;
                MapRenderState::exportSelMinX = (int)std::floor(curHoverWx);
                MapRenderState::exportSelMaxX = MapRenderState::exportSelMinX;
                MapRenderState::exportSelMinZ = (int)std::floor(curHoverWz);
                MapRenderState::exportSelMaxZ = MapRenderState::exportSelMinZ;
            } else {
                s_isDraggingMap = true;
            }
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            s_isDraggingMap = false;
            s_isSelectingExportBox = false;
        }

        if (s_isSelectingExportBox) {
            MapRenderState::exportSelMaxX = (int)std::floor(curHoverWx);
            MapRenderState::exportSelMaxZ = (int)std::floor(curHoverWz);
        } else if (s_isDraggingMap) {
            MapRenderState::bigMapOffsetX += io.MouseDelta.x;
            MapRenderState::bigMapOffsetZ += io.MouseDelta.y;
        }

        if (io.MouseWheel != 0.0f && isHoveringCanvas) {
            float oldZoom = MapRenderState::bigMapZoom;
            float zoomSpeed = 0.15f * oldZoom;
            MapRenderState::bigMapZoom += io.MouseWheel * zoomSpeed;
            if (MapRenderState::bigMapZoom < 0.05f) MapRenderState::bigMapZoom = 0.05f;
            if (MapRenderState::bigMapZoom > 40.0f) MapRenderState::bigMapZoom = 40.0f;
            
            float k = MapRenderState::bigMapZoom / oldZoom;
            float cx = io.DisplaySize.x * 0.5f + MapRenderState::bigMapOffsetX;
            float cy = io.DisplaySize.y * 0.5f + MapRenderState::bigMapOffsetZ;
            float dx = io.MousePos.x - cx;
            float dy = io.MousePos.y - cy;
            
            MapRenderState::bigMapOffsetX -= dx * (k - 1.0f);
            MapRenderState::bigMapOffsetZ -= dy * (k - 1.0f);
        }

        draw_list->AddRectFilled(ImVec2(0, 0), io.DisplaySize, IM_COL32(20, 20, 20, 255));

        float cx = io.DisplaySize.x * 0.5f;
        float cy = io.DisplaySize.y * 0.5f;
        
        float minWx = g_smoothPX - (cx + MapRenderState::bigMapOffsetX) / MapRenderState::bigMapZoom;
        float maxWx = g_smoothPX + (cx - MapRenderState::bigMapOffsetX) / MapRenderState::bigMapZoom;
        float minWz = g_smoothPZ - (cy + MapRenderState::bigMapOffsetZ) / MapRenderState::bigMapZoom;
        float maxWz = g_smoothPZ + (cy - MapRenderState::bigMapOffsetZ) / MapRenderState::bigMapZoom;

        // 实时记录当前大地图视野范围，供 PNG 导出 (Force Full Map: 关) 使用
        MapRenderState::exportViewMinX = (int)std::floor(minWx);
        MapRenderState::exportViewMaxX = (int)std::ceil(maxWx);
        MapRenderState::exportViewMinZ = (int)std::floor(minWz);
        MapRenderState::exportViewMaxZ = (int)std::ceil(maxWz);

        int startRx = (int)std::floor(minWx / 256.0f);
        int endRx   = (int)std::floor(maxWx / 256.0f);
        int startRz = (int)std::floor(minWz / 256.0f);
        int endRz   = (int)std::floor(maxWz / 256.0f);

        int texturesCreatedThisFrame = 0;
        {
            // [统一渲染] 所有维度(地表/洞穴/下界/末地)均使用缓存区域纹理
            // 洞穴/下界扫描数据已通过 UpdateFromScan 写入缓存, 可显示所有已保存数据
            // UpdateMapTexture 仍为小地图提供实时纹理
            UpdateMapTexture();

            // [地表模式] 渲染缓存区域纹理 (原有逻辑)
            static int s_vramGcTimer = 0;
            if (++s_vramGcTimer > 600) {
                s_vramGcTimer = 0;
                std::vector<uint64_t> keysToErase;
                for (auto& p : g_regionSRVs) {
                    int rx, rz; MapCacheManager::DecodeRegionHash(p.first, rx, rz);
                    if (rx < startRx - 15 || rx > endRx + 15 || rz < startRz - 15 || rz > endRz + 15) {
                        keysToErase.push_back(p.first);
                    }
                }
                for (uint64_t k : keysToErase) {
                    if (g_regionSRVs[k]) g_regionSRVs[k]->Release();
                    g_regionSRVs.erase(k);
                    if (g_regionTextures[k]) g_regionTextures[k]->Release();
                    g_regionTextures.erase(k);
                    MapCacheManager::MarkTextureDirty(k);
                }
            }

            draw_list->AddCallback(PointSamplerCallback, nullptr);

                for (int rx = startRx; rx <= endRx; rx++) {
                for (int rz = startRz; rz <= endRz; rz++) {
                    // [洞穴/下界隔离] 洞穴或下界模式下请求/绘制对应缓存纹理, 避免相互污染
                    uint64_t hash = MapCacheManager::GetRegionHash(rx, rz, isCave);
                    UpdateRegionTexture(hash, texturesCreatedThisFrame);

                    if (g_regionSRVs.find(hash) != g_regionSRVs.end()) {
                        // 【渲染管线减负】只有包含实际像素的非空贴图才会被加入 ImGui 的 DrawCall 绘制队列！
                        // 极大减负显卡在微缩大地图时的渲染压力，实现绝对满帧体验。
                        if (g_regionSRVs[hash] != nullptr) {
                            float sx_min = std::floor(cx + (rx * 256.0f - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX);
                            float sy_min = std::floor(cy + (rz * 256.0f - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ);
                            float sx_max = std::floor(cx + ((rx + 1) * 256.0f - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX);
                            float sy_max = std::floor(cy + ((rz + 1) * 256.0f - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ);

                            draw_list->AddImage((void*)g_regionSRVs[hash], ImVec2(sx_min, sy_min), ImVec2(sx_max, sy_max));
                        }
                    }
                }
            }

            // [全屏大地图实时纹理融合] 将当前扫描所得的 513x513 动态实时纹理无缝覆叠在大地图底层区块纹理之上
            // 解决大地图区块持久化延迟导致的“大地图刷新滞后、破坏/放置方块数分钟才显示”的问题
            // 达到与小地图完全一致的毫秒级实时刷新响应，且仅增 1 个 DrawCall，0 性能开销，绝对不掉帧
            // 注意：仅当大地图当前查看层与实时扫描层一致时才进行覆叠（例如查看主世界洞穴时，玩家也必须在地下洞穴模式中）
            bool liveIsCave = (currentDim == 1) || (currentDim == 0 && MapRenderState::g_caveModeActive);
            if (viewDim == currentDim && g_mapTextureView && (isCave == liveIsCave)) {
                float texMinX = g_textureCenterX - 256.0f;
                float texMinZ = g_textureCenterZ - 256.0f;
                float texMaxX = g_textureCenterX + 257.0f;
                float texMaxZ = g_textureCenterZ + 257.0f;

                float liveX0 = std::floor(cx + (texMinX - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX);
                float liveY0 = std::floor(cy + (texMinZ - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ);
                float liveX1 = std::floor(cx + (texMaxX - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX);
                float liveY1 = std::floor(cy + (texMaxZ - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ);

                if (liveX1 > 0.0f && liveX0 < io.DisplaySize.x && liveY1 > 0.0f && liveY0 < io.DisplaySize.y) {
                    draw_list->AddImage((void*)g_mapTextureView, ImVec2(liveX0, liveY0), ImVec2(liveX1, liveY1));
                }
            }

            draw_list->AddCallback(LinearSamplerCallback, nullptr);
        }

        // Seed predictions are composited after terrain but before the player,
        // entities, and waypoint overlays. The manager only returns markers
        // from pure cached worker results.
        RenderSeedMapMarkers(draw_list, cx, cy, minWx, minWz, maxWx, maxWz, viewDim);
        RenderSeedMapLegend(viewDim);

        float px = cx + MapRenderState::bigMapOffsetX;
        float py = cy + MapRenderState::bigMapOffsetZ;
        
        static std::string selectedWpId = "";
        static bool triggerWpMenu = false;
        static std::string selectedDeathPointId = "";
        static bool triggerDeathMenu = false;
        static RadarEntity selectedEntity;
        static bool triggerEntityMenu = false;

        // [全屏大地图实体雷达] 开启生物头像显示时，且当前查看维度与物理维度一致时，在大地图上显示周围实体与玩家头像
        if (viewDim == currentDim && MapRenderState::bigMapShowEntities) {
            EntityIconManager::ClearPlayerHeadTexturesIfRequested();
            static std::vector<RadarEntity> s_cachedBigMapEntities;
            static std::string s_cachedBigMapLocalUuid;
            static uint64_t s_cachedBigMapGeneration = 0;
            RefreshRadarEntitySnapshot(s_cachedBigMapEntities, s_cachedBigMapLocalUuid, s_cachedBigMapGeneration);

            for (const auto& ent : s_cachedBigMapEntities) {
                if (ent.type == 0 && !s_cachedBigMapLocalUuid.empty() && ent.uuid == s_cachedBigMapLocalUuid) {
                    continue;
                }
                if (ent.type == 0 && !MapRenderState::radarShowPlayers) continue;
                if (ent.type == 1 && !MapRenderState::radarShowHostile) continue;
                if (ent.type == 2 && !MapRenderState::radarShowFriendly) continue;
                if (ent.type == 3 && !MapRenderState::radarShowItems) continue;

                // 计算实体在大地图屏幕上的像素坐标
                float ex = cx + (ent.x - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX;
                float ez = cy + (ent.z - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ;

                if (ex >= -50.0f && ex <= io.DisplaySize.x + 50.0f && ez >= -50.0f && ez <= io.DisplaySize.y + 50.0f) {
                    float localPlayerY = (g_playerY > -9000.0f) ? g_playerY : 64.0f;
                    float dy = ent.y - localPlayerY;

                    // 垂直高度限制过滤 (超出高度限制的实体不显示)
                    if (MapRenderState::entityHeightLimit > 0 && std::abs(dy) > (float)MapRenderState::entityHeightLimit) {
                        continue;
                    }

                    ID3D11ShaderResourceView* texSRV = nullptr;
                    if (ent.type == 0 && g_pd3dDevice) {
                        texSRV = EntityIconManager::GetOrCreatePlayerHeadTexture(g_pd3dDevice, ent.uuid);
                    } else if (ent.type != 3 && g_pd3dDevice) {
                        texSRV = EntityIconManager::GetEntityHeadTexture(g_pd3dDevice, ent.typeName, ent.nameTag);
                    }

                    float is = 8.0f * MapRenderState::bigMapEntityScale;
                    if (ent.type == 0) is *= 1.3f;

                    // 显示实体深度：根据相对玩家的 Y 高低差暗化实体
                    float depthFactor = 1.0f;
                    if (MapRenderState::entityDepth && dy < -1.5f) {
                        depthFactor = std::clamp(1.0f + (dy / 30.0f) * 0.45f, 0.45f, 1.0f);
                    }

                    if (texSRV) {
                        ImU32 tintCol = IM_COL32((int)(255 * depthFactor), (int)(255 * depthFactor), (int)(255 * depthFactor), 255);
                        draw_list->AddImage((ImTextureID)texSRV, ImVec2(ex - is, ez - is), ImVec2(ex + is, ez + is), ImVec2(0, 0), ImVec2(1, 1), tintCol);
                    } else {
                        ImU32 col;
                        if (ent.type == 0) col = IM_COL32((int)(255 * depthFactor), (int)(255 * depthFactor), (int)(255 * depthFactor), 255);
                        else if (ent.type == 1) col = IM_COL32((int)(255 * depthFactor), (int)(50 * depthFactor), (int)(50 * depthFactor), 255);
                        else if (ent.type == 2) col = IM_COL32((int)(50 * depthFactor), (int)(255 * depthFactor), (int)(50 * depthFactor), 255);
                        else col = IM_COL32((int)(255 * depthFactor), (int)(255 * depthFactor), (int)(50 * depthFactor), 255);
                        float dotR = 2.0f * MapRenderState::bigMapEntityScale;
                        draw_list->AddRectFilled(ImVec2(ex - dotR, ez - dotR), ImVec2(ex + dotR, ez + dotR), col);
                        is = dotR;
                    }

                    // 实体高低差指示箭头 (▲ / ▼)
                    if (MapRenderState::radarHeightIndicators) {
                        float arrowH = 4.5f * MapRenderState::bigMapEntityScale;
                        float arrowW = 3.5f * MapRenderState::bigMapEntityScale;
                        if (dy > 2.5f) {
                            float arrowTop = ez - is - 2.0f * MapRenderState::bigMapEntityScale;
                            ImVec2 p0(ex, arrowTop - arrowH);
                            ImVec2 p1(ex - arrowW, arrowTop);
                            ImVec2 p2(ex + arrowW, arrowTop);
                            draw_list->AddTriangle(p0, p1, p2, IM_COL32(0, 0, 0, 240), 1.5f);
                            draw_list->AddTriangleFilled(p0, p1, p2, IM_COL32(255, 255, 255, 250));
                        } else if (dy < -2.5f) {
                            float arrowBottom = ez + is + 2.0f * MapRenderState::bigMapEntityScale;
                            ImVec2 p0(ex, arrowBottom + arrowH);
                            ImVec2 p1(ex - arrowW, arrowBottom);
                            ImVec2 p2(ex + arrowW, arrowBottom);
                            draw_list->AddTriangle(p0, p1, p2, IM_COL32(0, 0, 0, 240), 1.5f);
                            draw_list->AddTriangleFilled(p0, p1, p2, IM_COL32(255, 255, 255, 250));
                        }
                    }

                    // 命名牌实体始终显示名称
                    if (MapRenderState::alwaysShowNametags && !ent.nameTag.empty()) {
                        ImFont* bFont = ImGui::GetFont();
                        float bFontSize = ImGui::GetFontSize();
                        float tagScale = std::clamp(MapRenderState::globalUIScale, 0.5f, 2.5f);
                        float tagFontSize = std::clamp(bFontSize * 0.75f, 10.0f * tagScale, 16.0f * tagScale);
                        ImVec2 nameSz = bFont->CalcTextSizeA(tagFontSize, FLT_MAX, 0.0f, ent.nameTag.c_str());
                        float tagY = ez + is + ((dy < -2.5f && MapRenderState::radarHeightIndicators) ? (7.0f * MapRenderState::bigMapEntityScale) : (2.0f * MapRenderState::bigMapEntityScale));
                        ImVec2 tagPos(ex - nameSz.x * 0.5f, tagY);
                        draw_list->AddText(bFont, tagFontSize, ImVec2(tagPos.x + 1.0f, tagPos.y + 1.0f), IM_COL32(0, 0, 0, 220), ent.nameTag.c_str(), NULL, 0.0f, NULL);
                        draw_list->AddText(bFont, tagFontSize, tagPos, IM_COL32(255, 255, 255, 235), ent.nameTag.c_str(), NULL, 0.0f, NULL);
                    }

                    // 鼠标悬停实体与点击交互
                    if (isHoveringCanvas && !MapRenderState::showExportPNGScreen) {
                        float mouseDistSq = (io.MousePos.x - ex) * (io.MousePos.x - ex) + (io.MousePos.y - ez) * (io.MousePos.y - ez);
                        float hitR = is + 4.0f * MapRenderState::bigMapEntityScale;
                        if (mouseDistSq <= hitR * hitR) {
                            double eDist = std::sqrt((double)(ent.x - g_playerBlockX) * (ent.x - g_playerBlockX) + (double)(ent.z - g_playerBlockZ) * (ent.z - g_playerBlockZ));
                            char distBuf[64];
                            if (viewDim == currentDim) {
                                if (eDist >= 1000.0) snprintf(distBuf, sizeof(distBuf), " (%.1fkm)", eDist / 1000.0);
                                else snprintf(distBuf, sizeof(distBuf), " (%.0fm)", eDist);
                            } else {
                                distBuf[0] = '\0';
                            }

                            std::string displayName;
                            const char* catTag = "";
                            if (ent.type == 0) {
                                catTag = LanguageManager::GetText("RADAR_CAT_PLAYER");
                                displayName = !ent.nameTag.empty() ? ent.nameTag : LanguageManager::GetText("RADAR_CAT_PLAYER");
                            } else if (ent.type == 1) {
                                catTag = LanguageManager::GetText("RADAR_CAT_HOSTILE");
                                displayName = LanguageManager::GetEntityDisplayName(ent.typeName, ent.nameTag);
                            } else if (ent.type == 2) {
                                catTag = LanguageManager::GetText("RADAR_CAT_FRIENDLY");
                                displayName = LanguageManager::GetEntityDisplayName(ent.typeName, ent.nameTag);
                            } else {
                                catTag = LanguageManager::GetText("RADAR_CAT_ITEM");
                                displayName = LanguageManager::GetEntityDisplayName(ent.typeName, ent.nameTag);
                            }

                            ImGui::SetTooltip("[%s] %s\nX: %.1f, Y: %.1f, Z: %.1f%s", catTag, displayName.c_str(), ent.x, ent.y, ent.z, distBuf);

                            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                                selectedEntity = ent;
                                triggerEntityMenu = true;
                                triggerWpMenu = false;
                                triggerDeathMenu = false;
                                s_isDraggingMap = false;
                            }
                        }
                    }
                }
            }
        }

        // [玩家足迹追踪] 大地图渲染足迹点 (Footstep / Breadcrumbs)
        if (MapRenderState::showFootsteps) {
            std::lock_guard<std::mutex> lock(MapRenderState::g_footstepsMutex);
            static auto s_startEpoch = std::chrono::steady_clock::now();
            float curTime = std::chrono::duration<float>(std::chrono::steady_clock::now() - s_startEpoch).count();
            size_t count = MapRenderState::g_footsteps.size();
            for (size_t i = 0; i < count; ++i) {
                const auto& step = MapRenderState::g_footsteps[i];
                if (step.dimId != viewDim) continue;
                float age = curTime - step.timestamp;
                if (age > 600.0f) continue;

                float fx = cx + (step.x - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX;
                float fz = cy + (step.z - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ;

                if (fx >= -20.0f && fx <= io.DisplaySize.x + 20.0f && fz >= -20.0f && fz <= io.DisplaySize.y + 20.0f) {
                    float progress = (float)(i + 1) / (float)(count + 1);
                    float alpha = std::clamp(progress * 0.85f, 0.18f, 0.85f);
                    if (age > 300.0f) alpha *= (1.0f - (age - 300.0f) / 300.0f);

                    float r = std::clamp(2.5f * MapRenderState::bigMapZoom, 1.8f, 4.5f);
                    draw_list->AddCircleFilled(ImVec2(fx, fz), r + 0.8f, IM_COL32(0, 0, 0, (int)(160 * alpha)));
                    draw_list->AddCircleFilled(ImVec2(fx, fz), r, IM_COL32(255, 235, 120, (int)(240 * alpha)));
                }
            }
        }

        if (viewDim == currentDim) {
            float yawRad = (g_playerYaw + 180.0f) * (3.14159265f / 180.0f); 
            float cosY = std::cos(yawRad);
            float sinY = std::sin(yawRad);
            auto rotate = [&](float x, float y) -> ImVec2 { 
                return ImVec2(px + (x * cosY - y * sinY), py + (x * sinY + y * cosY)); 
            };

            float aScale = MapRenderState::playerArrowScale;
            draw_list->AddTriangleFilled(rotate(0, -10.0f * aScale), rotate(-7.0f * aScale, 10.0f * aScale), rotate(7.0f * aScale, 10.0f * aScale), IM_COL32(0, 0, 0, 255));
            draw_list->AddTriangleFilled(rotate(0, -8.0f * aScale), rotate(-5.0f * aScale, 8.0f * aScale), rotate(5.0f * aScale, 8.0f * aScale), GetPlayerArrowColor());
        }

        // 若玩家通过 Shift+左键拖拽框选了导出区域，在大地图上绘制高亮选区框
        if (MapRenderState::hasExportSelection) {
            int sMinX = std::min(MapRenderState::exportSelMinX, MapRenderState::exportSelMaxX);
            int sMaxX = std::max(MapRenderState::exportSelMinX, MapRenderState::exportSelMaxX) + 1;
            int sMinZ = std::min(MapRenderState::exportSelMinZ, MapRenderState::exportSelMaxZ);
            int sMaxZ = std::max(MapRenderState::exportSelMinZ, MapRenderState::exportSelMaxZ) + 1;
            float sx0 = cx + MapRenderState::bigMapOffsetX + (sMinX - g_smoothPX) * MapRenderState::bigMapZoom;
            float sy0 = cy + MapRenderState::bigMapOffsetZ + (sMinZ - g_smoothPZ) * MapRenderState::bigMapZoom;
            float sx1 = cx + MapRenderState::bigMapOffsetX + (sMaxX - g_smoothPX) * MapRenderState::bigMapZoom;
            float sy1 = cy + MapRenderState::bigMapOffsetZ + (sMaxZ - g_smoothPZ) * MapRenderState::bigMapZoom;
            draw_list->AddRectFilled(ImVec2(sx0, sy0), ImVec2(sx1, sy1), IM_COL32(80, 210, 255, 42));
            draw_list->AddRect(ImVec2(sx0, sy0), ImVec2(sx1, sy1), IM_COL32(80, 225, 255, 235), 0.0f, 0, 2.0f);
        }

        // [全屏大地图区块网格] 每 16 格一条边界，铺满当前可视区域；缩放过小（网格过密）时自动跳过
        if (MapRenderState::showChunkGrid && MapRenderState::bigMapZoom * 16.0f >= 4.0f) {
            constexpr float CHUNK_SIZE = 16.0f;
            const ImU32 gridLineCol = IM_COL32(255, 255, 255, 45);
            float zm = MapRenderState::bigMapZoom;
            // 由屏幕坐标反推可视世界范围（与路径点同一套映射公式）
            float wX0 = g_smoothPX - (cx + MapRenderState::bigMapOffsetX) / zm;
            float wX1 = g_smoothPX + (io.DisplaySize.x - cx - MapRenderState::bigMapOffsetX) / zm;
            float wZ0 = g_smoothPZ - (cy + MapRenderState::bigMapOffsetZ) / zm;
            float wZ1 = g_smoothPZ + (io.DisplaySize.y - cy - MapRenderState::bigMapOffsetZ) / zm;

            for (int b = (int)std::floor(wX0 / CHUNK_SIZE) * 16; b <= (int)std::ceil(wX1 / CHUNK_SIZE) * 16; b += 16) {
                float lineX = cx + ((float)b - g_smoothPX) * zm + MapRenderState::bigMapOffsetX;
                draw_list->AddLine(ImVec2(lineX, 0.0f), ImVec2(lineX, io.DisplaySize.y), gridLineCol, 1.0f);
            }
            for (int b = (int)std::floor(wZ0 / CHUNK_SIZE) * 16; b <= (int)std::ceil(wZ1 / CHUNK_SIZE) * 16; b += 16) {
                float lineY = cy + ((float)b - g_smoothPZ) * zm + MapRenderState::bigMapOffsetZ;
                draw_list->AddLine(ImVec2(0.0f, lineY), ImVec2(io.DisplaySize.x, lineY), gridLineCol, 1.0f);
            }
        }

        // [鼠标悬停区块选择框] 鼠标所在区块半透明填充 + 描边
        if (MapRenderState::bigMapShowHoverBox && isHoveringCanvas) {
            int hoverChunkX = (int)std::floor(curHoverWx / 16.0f);
            int hoverChunkZ = (int)std::floor(curHoverWz / 16.0f);

            float zm = MapRenderState::bigMapZoom;
            float hx0 = cx + ((float)(hoverChunkX * 16)      - g_smoothPX) * zm + MapRenderState::bigMapOffsetX;
            float hx1 = cx + ((float)(hoverChunkX * 16 + 16) - g_smoothPX) * zm + MapRenderState::bigMapOffsetX;
            float hz0 = cy + ((float)(hoverChunkZ * 16)      - g_smoothPZ) * zm + MapRenderState::bigMapOffsetZ;
            float hz1 = cy + ((float)(hoverChunkZ * 16 + 16) - g_smoothPZ) * zm + MapRenderState::bigMapOffsetZ;

            draw_list->AddRectFilled(ImVec2(hx0, hz0), ImVec2(hx1, hz1), IM_COL32(255, 255, 255, 28));
            draw_list->AddRect(ImVec2(hx0, hz0), ImVec2(hx1, hz1), IM_COL32(255, 255, 255, 125), 0.0f, 0, 1.2f);
        }

        // 路径点标记受底部“地图标记”总开关控制（关闭时不绘制、不响应点击）
        if (!MapRenderState::showExportPNGScreen && MapRenderState::bigMapShowMarkers) {
            std::lock_guard<std::mutex> lock(WaypointManager::g_wpMutex);
            for (const auto& wp : WaypointManager::g_waypoints) {
                if (wp.dimId != viewDim) continue; // 仅显示当前查看维度的路径点
                if (!wp.enabled && !MapRenderState::bigMapShowDisabledWaypoints) continue;
                
                float wx = cx + (wp.x - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX;
                float wz = cy + (wp.z - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ;
                
                if (wx > -50.0f && wx < io.DisplaySize.x + 50.0f && wz > -50.0f && wz < io.DisplaySize.y + 50.0f) {
                    std::string distStr = "";
                    if (MapRenderState::showWaypointDistance && wp.dimId == MapRenderState::currentDimensionId) {
                        float bDx = (float)wp.x - g_playerBlockX;
                        float bDy = (float)wp.y - g_playerY;
                        float bDz = (float)wp.z - g_playerBlockZ;
                        float d3d = std::sqrt(bDx * bDx + bDy * bDy + bDz * bDz);
                        char distBuf[32];
                        if (d3d >= 1000.0f) {
                            snprintf(distBuf, sizeof(distBuf), "%.1fkm", d3d / 1000.0f);
                        } else {
                            snprintf(distBuf, sizeof(distBuf), "%dm", (int)std::round(d3d));
                        }
                        distStr = distBuf;
                    }
                    DrawWaypointIcon(draw_list, ImVec2(wx, wz), mce::Color(wp.r, wp.g, wp.b, 1.0f), wp.name, false, MapRenderState::bigMapWaypointScale, wp.isTemporary, wp.enabled, distStr);
                    
                    float hitRadius = 12.0f * MapRenderState::bigMapWaypointScale;
                    float distSq = (io.MousePos.x - wx) * (io.MousePos.x - wx) + (io.MousePos.y - wz) * (io.MousePos.y - wz);
                    if (distSq <= hitRadius * hitRadius) {
                        double dDist = std::sqrt((double)(wp.x - g_playerBlockX) * (wp.x - g_playerBlockX) + (double)(wp.z - g_playerBlockZ) * (wp.z - g_playerBlockZ));
                        char distBuf[64];
                        if (wp.dimId == MapRenderState::currentDimensionId) {
                            if (dDist >= 1000.0) snprintf(distBuf, sizeof(distBuf), " (%.1fkm)", dDist / 1000.0);
                            else snprintf(distBuf, sizeof(distBuf), " (%.0fm)", dDist);
                        } else {
                            distBuf[0] = '\0';
                        }
                        std::string prefixTag = "";
                        if (!wp.enabled) {
                            prefixTag = std::string("[") + LanguageManager::GetText("DISABLED_TAG") + "] ";
                        } else if (wp.isTemporary) {
                            prefixTag = std::string("[") + LanguageManager::GetText("TEMP_WAYPOINT_NAME") + "] ";
                        }
                        ImGui::SetTooltip("%s%s\nX: %d, Y: %d, Z: %d%s", prefixTag.c_str(), wp.name.c_str(), wp.x, wp.y, wp.z, distBuf);

                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            selectedWpId = wp.id;
                            triggerWpMenu = true;
                            triggerDeathMenu = false;
                            triggerEntityMenu = false;
                            s_isDraggingMap = false;
                        }
                    }
                }
            }
        }

        // 大地图死亡点渲染与左键交互 (红叉 ❌)
        if (!MapRenderState::showExportPNGScreen) {
            std::lock_guard<std::mutex> lock(DeathPointManager::g_deathMutex);
            for (const auto& dp : DeathPointManager::g_deathPoints) {
                if (dp.dimensionId != viewDim) continue; // 仅显示当前查看维度的死亡点
                
                float dx = cx + (dp.x + 0.5f - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX;
                float dz = cy + (dp.z + 0.5f - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ;
                
                if (dx > -50.0f && dx < io.DisplaySize.x + 50.0f && dz > -50.0f && dz < io.DisplaySize.y + 50.0f) {
                    DrawDeathPointIcon(draw_list, ImVec2(dx, dz), LanguageManager::GetText("DEATH_POINT_WP_PREFIX"), false);
                    
                    float distSq = (io.MousePos.x - dx) * (io.MousePos.x - dx) + (io.MousePos.y - dz) * (io.MousePos.y - dz);
                    if (distSq <= 196.0f) {
                        double dDist = std::sqrt((double)(dp.x - g_playerBlockX) * (dp.x - g_playerBlockX) + (double)(dp.z - g_playerBlockZ) * (dp.z - g_playerBlockZ));
                        char distBuf[64];
                        if (dp.dimensionId == MapRenderState::currentDimensionId) {
                            if (dDist >= 1000.0) snprintf(distBuf, sizeof(distBuf), " (%.1fkm)", dDist / 1000.0);
                            else snprintf(distBuf, sizeof(distBuf), " (%.0fm)", dDist);
                        } else {
                            distBuf[0] = '\0';
                        }
                        ImGui::SetTooltip("%s\nX: %d, Y: %d, Z: %d%s\n%s",
                            LanguageManager::GetText("DEATH_POINT_WP_PREFIX"),
                            dp.x, dp.y, dp.z, distBuf,
                            FormatDeathTime(dp.timestamp).c_str());
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                            selectedDeathPointId = dp.id;
                            triggerDeathMenu = true;
                            triggerWpMenu = false;
                            triggerEntityMenu = false;
                            s_isDraggingMap = false;
                        }
                    }
                }
            }
        }


        float hoverWx = g_smoothPX + (io.MousePos.x - cx - MapRenderState::bigMapOffsetX) / MapRenderState::bigMapZoom;
        float hoverWz = g_smoothPZ + (io.MousePos.y - cy - MapRenderState::bigMapOffsetZ) / MapRenderState::bigMapZoom;

        float curScale = std::clamp(MapRenderState::globalUIScale, 0.25f, 4.0f);
        ImFont* mFont = ImGui::GetFont();
        float mFontSize = ImGui::GetFontSize();
        
        char infoBuf[256];
        snprintf(infoBuf, sizeof(infoBuf), LanguageManager::GetText("BIGMAP_TITLE"), MapRenderState::bigMapZoom);
        draw_list->AddText(mFont, mFontSize, ImVec2(20.0f * curScale, 20.0f * curScale), IM_COL32(255, 200, 50, 255), infoBuf, NULL, 0.0f, NULL);
        draw_list->AddText(mFont, mFontSize, ImVec2(20.0f * curScale, 20.0f * curScale + mFontSize + 5.0f * curScale), IM_COL32(200, 200, 200, 255), LanguageManager::GetText("BIGMAP_HELP"), NULL, 0.0f, NULL);

        // ==========================================
        // [转到坐标] 靶心与标注渲染 (高亮显示定位目标)
        // ==========================================
        if (s_gotoTargetActive) {
            s_gotoTargetTimer -= io.DeltaTime;
            if (s_gotoTargetTimer <= 0.0f) {
                s_gotoTargetActive = false;
            } else if (s_gotoTargetDim == viewDim && !MapRenderState::showExportPNGScreen) {
                float gtx = cx + (s_gotoTargetX + 0.5f - g_smoothPX) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetX;
                float gtz = cy + (s_gotoTargetZ + 0.5f - g_smoothPZ) * MapRenderState::bigMapZoom + MapRenderState::bigMapOffsetZ;

                if (gtx > -100.0f && gtx < io.DisplaySize.x + 100.0f && gtz > -100.0f && gtz < io.DisplaySize.y + 100.0f) {
                    float alpha = (s_gotoTargetTimer < 1.0f) ? std::clamp(s_gotoTargetTimer, 0.0f, 1.0f) : 1.0f;
                    float pulse = (std::sin((float)ImGui::GetTime() * 6.0f) + 1.0f) * 0.5f;
                    float r = 14.0f + pulse * 4.0f;

                    ImU32 cBorder = IM_COL32(0, 220, 255, (int)(230.0f * alpha));
                    ImU32 cShadow = IM_COL32(0, 0, 0, (int)(180.0f * alpha));
                    ImU32 cFill   = IM_COL32(0, 200, 255, (int)(45.0f * alpha));

                    // 1. 半透明填充与光圈
                    draw_list->AddCircleFilled(ImVec2(gtx, gtz), r, cFill);
                    draw_list->AddCircle(ImVec2(gtx, gtz), r + 1.0f, cShadow, 32, 2.5f);
                    draw_list->AddCircle(ImVec2(gtx, gtz), r, cBorder, 32, 2.0f);

                    // 2. 十字准星标线
                    float tickLen = 6.0f;
                    auto drawTick = [&](ImVec2 p1, ImVec2 p2) {
                        draw_list->AddLine(ImVec2(p1.x + 0.5f, p1.y + 0.5f), ImVec2(p2.x + 0.5f, p2.y + 0.5f), cShadow, 2.5f);
                        draw_list->AddLine(p1, p2, cBorder, 1.5f);
                    };
                    drawTick(ImVec2(gtx, gtz - r - tickLen), ImVec2(gtx, gtz - r + 2.0f));
                    drawTick(ImVec2(gtx, gtz + r - 2.0f), ImVec2(gtx, gtz + r + tickLen));
                    drawTick(ImVec2(gtx - r - tickLen, gtz), ImVec2(gtx - r + 2.0f, gtz));
                    drawTick(ImVec2(gtx + r - 2.0f, gtz), ImVec2(gtx + r + tickLen, gtz));

                    // 3. 中心圆点
                    draw_list->AddCircleFilled(ImVec2(gtx, gtz), 3.0f, cBorder);
                    draw_list->AddCircle(ImVec2(gtx, gtz), 3.5f, cShadow, 16, 1.0f);

                    // 4. 坐标气泡标签
                    char tagBuf[64];
                    if (std::floor(s_gotoTargetX) == s_gotoTargetX && std::floor(s_gotoTargetZ) == s_gotoTargetZ) {
                        snprintf(tagBuf, sizeof(tagBuf), "X: %d, Z: %d", (int)s_gotoTargetX, (int)s_gotoTargetZ);
                    } else {
                        snprintf(tagBuf, sizeof(tagBuf), "X: %.1f, Z: %.1f", s_gotoTargetX, s_gotoTargetZ);
                    }
                    ImVec2 tagSz = mFont->CalcTextSizeA(mFontSize * 0.85f, FLT_MAX, 0.0f, tagBuf);
                    float tagPadX = 6.0f * curScale;
                    float tagPadY = 3.0f * curScale;
                    float tagX = gtx - tagSz.x * 0.5f;
                    float tagY = gtz + r + 8.0f * curScale;

                    draw_list->AddRectFilled(ImVec2(tagX - tagPadX, tagY - tagPadY),
                                             ImVec2(tagX + tagSz.x + tagPadX, tagY + tagSz.y + tagPadY),
                                             IM_COL32(15, 20, 25, (int)(220.0f * alpha)), 4.0f * curScale);
                    draw_list->AddRect(ImVec2(tagX - tagPadX, tagY - tagPadY),
                                       ImVec2(tagX + tagSz.x + tagPadX, tagY + tagSz.y + tagPadY),
                                       IM_COL32(0, 220, 255, (int)(160.0f * alpha)), 4.0f * curScale, 0, 1.0f);
                    draw_list->AddText(mFont, mFontSize * 0.85f, ImVec2(tagX, tagY),
                                       IM_COL32(235, 245, 255, (int)(255.0f * alpha)), tagBuf);
                }
            }
        }
        
        // ==========================================
        // [维度快捷切换按钮] 供玩家快速切换浏览主世界/下界/末地大地图
        // ==========================================
        float dimBarY = 20.0f * curScale + (mFontSize + 5.0f * curScale) * 2.0f + 6.0f * curScale;
        if (!MapRenderState::showExportPNGScreen) {
            ImGui::SetCursorPos(ImVec2(20.0f * curScale, dimBarY));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * curScale);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f * curScale, 0.0f));
            ImGui::BeginGroup();

            // 1. 主世界地表
            {
                bool isSelected = (viewDim == 0 && !MapRenderState::bigMapOverworldCave);
                bool isPhysical = (currentDim == 0 && !MapRenderState::g_caveModeActive);
                const char* dimLabel = LanguageManager::GetText("WP_TAB_OVERWORLD");

                ImVec4 bgCol, bgHover, bgActive;
                if (isSelected) {
                    bgCol    = ImVec4(0.18f, 0.50f, 0.22f, 0.95f);
                    bgHover  = ImVec4(0.24f, 0.62f, 0.28f, 1.0f);
                    bgActive = ImVec4(0.14f, 0.42f, 0.18f, 1.0f);
                } else {
                    bgCol    = ImVec4(0.12f, 0.12f, 0.15f, 0.75f);
                    bgHover  = ImVec4(0.25f, 0.25f, 0.30f, 0.90f);
                    bgActive = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Button, bgCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bgHover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, bgActive);
                ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.80f, 0.80f, 0.85f, 0.90f));

                char btnId[64];
                snprintf(btnId, sizeof(btnId), "%s###DimBtn_0", dimLabel);
                if (ImGui::Button(btnId, ImVec2(0.0f, 25.0f * curScale))) {
                    bool needReload = (viewDim != 0) || MapRenderState::bigMapOverworldCave;
                    if (viewDim != 0) {
                        MapRenderState::SaveDimCamera(viewDim, g_smoothPX, g_smoothPZ);
                        MapCacheManager::SwitchViewDimension(0);
                        MapRenderState::RestoreDimCamera(0, viewDim, g_smoothPX, g_smoothPZ);
                        MapRenderState::bigMapViewDimensionId = 0;
                    }
                    if (needReload) {
                        MapRenderState::bigMapOverworldCave = false;
                        MapRenderState::clearGPUCache.store(true);
                        MapRenderState::hoverBiomeAlpha = 0.0f;
                        MapRenderState::hoverBiomeTargetAlpha = 0.0f;
                        MapRenderState::hoverBiomeHasValidResult = false;
                        MapCacheManager::InvalidateExportPreview();
                    }
                }
                ImGui::PopStyleColor(4);

                if (ImGui::IsItemHovered()) {
                    char tipBuf[256];
                    if (isPhysical) {
                        snprintf(tipBuf, sizeof(tipBuf), "%s %s", dimLabel, LanguageManager::GetText("DIM_CURRENT_HINT"));
                    } else {
                        snprintf(tipBuf, sizeof(tipBuf), LanguageManager::GetText("DIM_SWITCH_TOOLTIP"), dimLabel);
                    }
                    ImGui::SetTooltip("%s", tipBuf);
                }
            }

            // 2. 主世界洞穴
            ImGui::SameLine();
            {
                bool isSelected = (viewDim == 0 && MapRenderState::bigMapOverworldCave);
                bool isPhysical = (currentDim == 0 && MapRenderState::g_caveModeActive);
                const char* dimLabel = LanguageManager::GetText("DIM_CAVE");

                ImVec4 bgCol, bgHover, bgActive;
                if (isSelected) {
                    bgCol    = ImVec4(0.16f, 0.38f, 0.46f, 0.95f);
                    bgHover  = ImVec4(0.22f, 0.48f, 0.58f, 1.0f);
                    bgActive = ImVec4(0.12f, 0.30f, 0.38f, 1.0f);
                } else {
                    bgCol    = ImVec4(0.12f, 0.12f, 0.15f, 0.75f);
                    bgHover  = ImVec4(0.25f, 0.25f, 0.30f, 0.90f);
                    bgActive = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Button, bgCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bgHover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, bgActive);
                ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.80f, 0.80f, 0.85f, 0.90f));

                char btnId[64];
                snprintf(btnId, sizeof(btnId), "%s###DimBtn_Cave", dimLabel);
                if (ImGui::Button(btnId, ImVec2(0.0f, 25.0f * curScale))) {
                    bool needReload = (viewDim != 0) || !MapRenderState::bigMapOverworldCave;
                    if (viewDim != 0) {
                        MapRenderState::SaveDimCamera(viewDim, g_smoothPX, g_smoothPZ);
                        MapCacheManager::SwitchViewDimension(0);
                        MapRenderState::RestoreDimCamera(0, viewDim, g_smoothPX, g_smoothPZ);
                        MapRenderState::bigMapViewDimensionId = 0;
                    }
                    if (needReload) {
                        MapRenderState::bigMapOverworldCave = true;
                        MapRenderState::clearGPUCache.store(true);
                        MapRenderState::hoverBiomeAlpha = 0.0f;
                        MapRenderState::hoverBiomeTargetAlpha = 0.0f;
                        MapRenderState::hoverBiomeHasValidResult = false;
                        MapCacheManager::InvalidateExportPreview();
                    }
                }
                ImGui::PopStyleColor(4);

                if (ImGui::IsItemHovered()) {
                    char tipBuf[256];
                    if (isPhysical) {
                        snprintf(tipBuf, sizeof(tipBuf), "%s %s", dimLabel, LanguageManager::GetText("DIM_CURRENT_HINT"));
                    } else {
                        snprintf(tipBuf, sizeof(tipBuf), "%s", LanguageManager::GetText("DIM_SWITCH_TOOLTIP_CAVE"));
                    }
                    ImGui::SetTooltip("%s", tipBuf);
                }
            }

            // 3. 下界 & 4. 末地
            for (int d = 1; d <= 2; ++d) {
                ImGui::SameLine();
                bool isSelected = (viewDim == d);
                bool isPhysical = (currentDim == d);
                const char* dimLabel = LanguageManager::GetText(d == 1 ? "WP_TAB_NETHER" : "WP_TAB_END");

                ImVec4 bgCol, bgHover, bgActive;
                if (isSelected) {
                    if (d == 1) { // 下界: 地狱深红
                        bgCol    = ImVec4(0.65f, 0.18f, 0.18f, 0.95f);
                        bgHover  = ImVec4(0.78f, 0.24f, 0.24f, 1.0f);
                        bgActive = ImVec4(0.52f, 0.14f, 0.14f, 1.0f);
                    } else { // 末地: 终界幽紫
                        bgCol    = ImVec4(0.42f, 0.18f, 0.60f, 0.95f);
                        bgHover  = ImVec4(0.54f, 0.24f, 0.74f, 1.0f);
                        bgActive = ImVec4(0.34f, 0.14f, 0.48f, 1.0f);
                    }
                } else {
                    bgCol    = ImVec4(0.12f, 0.12f, 0.15f, 0.75f);
                    bgHover  = ImVec4(0.25f, 0.25f, 0.30f, 0.90f);
                    bgActive = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
                }

                ImGui::PushStyleColor(ImGuiCol_Button, bgCol);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, bgHover);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, bgActive);
                ImGui::PushStyleColor(ImGuiCol_Text, isSelected ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.80f, 0.80f, 0.85f, 0.90f));

                char btnId[64];
                snprintf(btnId, sizeof(btnId), "%s###DimBtn_%d", dimLabel, d);
                if (ImGui::Button(btnId, ImVec2(0.0f, 25.0f * curScale))) {
                    if (viewDim != d) {
                        MapRenderState::SaveDimCamera(viewDim, g_smoothPX, g_smoothPZ);
                        MapCacheManager::SwitchViewDimension(d);
                        MapRenderState::clearGPUCache.store(true);
                        MapRenderState::RestoreDimCamera(d, viewDim, g_smoothPX, g_smoothPZ);
                        MapRenderState::bigMapViewDimensionId = d;
                        MapRenderState::hoverBiomeAlpha = 0.0f;
                        MapRenderState::hoverBiomeTargetAlpha = 0.0f;
                        MapRenderState::hoverBiomeHasValidResult = false;
                        MapCacheManager::InvalidateExportPreview();
                    }
                }

                ImGui::PopStyleColor(4);

                if (ImGui::IsItemHovered()) {
                    char tipBuf[256];
                    if (isPhysical) {
                        snprintf(tipBuf, sizeof(tipBuf), "%s %s", dimLabel, LanguageManager::GetText("DIM_CURRENT_HINT"));
                    } else {
                        snprintf(tipBuf, sizeof(tipBuf), LanguageManager::GetText("DIM_SWITCH_TOOLTIP"), dimLabel);
                    }
                    ImGui::SetTooltip("%s", tipBuf);
                }
            }

            ImGui::EndGroup();
            ImGui::PopStyleVar(2);
        }

        // ==========================================
        // [转到坐标控件] 支持输入 X/Z 坐标按回车或点击按钮定位大地图
        // ==========================================
        if (!MapRenderState::showExportPNGScreen) {
            float gotoBarY = dimBarY + 25.0f * curScale + 6.0f * curScale;
            ImGui::SetCursorPos(ImVec2(20.0f * curScale, gotoBarY));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * curScale);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f * curScale, 3.0f * curScale));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f * curScale, 0.0f));
            ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.12f, 0.12f, 0.15f, 0.75f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.18f, 0.18f, 0.22f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.22f, 0.22f, 0.28f, 1.0f));

            ImGui::BeginGroup();

            ImGui::AlignTextToFramePadding();
            ImGui::TextColored(ImVec4(0.75f, 0.75f, 0.80f, 0.95f), "%s:", LanguageManager::GetText("BIGMAP_GOTO_TITLE"));
            ImGui::SameLine();

            float inputW = 60.0f * curScale;

            ImGui::PushTabStop(false);

            // X 坐标输入框
            ImGui::SetNextItemWidth(inputW);
            bool enterX = ImGui::InputTextWithHint("##GotoX", "X", s_gotoXBuf, sizeof(s_gotoXBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("BIGMAP_GOTO_TIP"));

            ImGui::SameLine();

            // Z 坐标输入框
            ImGui::SetNextItemWidth(inputW);
            bool enterZ = ImGui::InputTextWithHint("##GotoZ", "Z", s_gotoZBuf, sizeof(s_gotoZBuf), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll);
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("BIGMAP_GOTO_TIP"));

            ImGui::SameLine();

            // 转到按钮
            PushOreButtonStyle(OreButtonKind::Primary);
            bool clickGo = ImGui::Button(LanguageManager::GetText("BIGMAP_GOTO_BTN"), ImVec2(0.0f, 0.0f));
            PopOreButtonStyle();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("BIGMAP_GOTO_TIP"));

            ImGui::PopTabStop();

            if (enterX || enterZ || clickGo) {
                if (ImGui::GetCurrentContext()) {
                    ImGui::ClearActiveID();
                }
                float tx = 0.0f, tz = 0.0f;
                if (ParseGotoCoords(s_gotoXBuf, s_gotoZBuf, tx, tz)) {
                    if (std::floor(tx) == tx && std::abs(tx) < 1e7f) {
                        snprintf(s_gotoXBuf, sizeof(s_gotoXBuf), "%d", (int)tx);
                    } else {
                        snprintf(s_gotoXBuf, sizeof(s_gotoXBuf), "%.2f", tx);
                    }
                    if (std::floor(tz) == tz && std::abs(tz) < 1e7f) {
                        snprintf(s_gotoZBuf, sizeof(s_gotoZBuf), "%d", (int)tz);
                    } else {
                        snprintf(s_gotoZBuf, sizeof(s_gotoZBuf), "%.2f", tz);
                    }
                    LocateBigMapOn(viewDim, tx + 0.5f, tz + 0.5f, g_smoothPX, g_smoothPZ);
                    s_gotoTargetActive = true;
                    s_gotoTargetX = tx;
                    s_gotoTargetZ = tz;
                    s_gotoTargetDim = viewDim;
                    s_gotoTargetTimer = 8.0f;
                }
            }

            // 清除定位标记按钮 (仅在标记活跃且位于当前维度时显示)
            if (s_gotoTargetActive && s_gotoTargetDim == viewDim) {
                ImGui::SameLine();
                PushOreButtonStyle(OreButtonKind::Default);
                if (ImGui::Button("✕##ClearGoto", ImVec2(0.0f, 0.0f))) {
                    s_gotoTargetActive = false;
                    s_gotoTargetTimer = 0.0f;
                }
                PopOreButtonStyle();
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("✕");
                }
            }

            ImGui::SameLine();
            PushOreButtonStyle(OreButtonKind::Default);
            ImGui::Button("?##BigMapHelp", ImVec2(0.0f, 0.0f));
            PopOreButtonStyle();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s:\n• %s\n• %s\n• %s\n• %s\n• %s",
                    LanguageManager::GetText("BIGMAP_CONTROLS_HELP"),
                    LanguageManager::GetText("HELP_DRAG_ZOOM"),
                    LanguageManager::GetText("HELP_RIGHT_CLICK"),
                    LanguageManager::GetText("HELP_SPACE_CENTER"),
                    LanguageManager::GetText("HELP_SHIFT_DRAG_EXPORT"),
                    LanguageManager::GetText("HELP_GOTO_COORDS"));
            }

            ImGui::EndGroup();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(3);
        }
        
        snprintf(infoBuf, sizeof(infoBuf), LanguageManager::GetText("CURSOR_POS"), (int)std::floor(hoverWx), (int)std::floor(hoverWz));
        ImVec2 textSize = mFont->CalcTextSizeA(mFontSize, FLT_MAX, 0.0f, infoBuf);
        float padH = 15.0f * curScale;
        float padV = 8.0f * curScale;
        float boxY = io.DisplaySize.y - textSize.y - padV * 2.0f - 10.0f * curScale;
        draw_list->AddRectFilled(ImVec2(io.DisplaySize.x / 2 - textSize.x / 2 - padH, boxY), 
                                 ImVec2(io.DisplaySize.x / 2 + textSize.x / 2 + padH, io.DisplaySize.y - 10.0f * curScale), 
                                 IM_COL32(0, 0, 0, 180), 5.0f * curScale);
        draw_list->AddText(mFont, mFontSize, ImVec2(io.DisplaySize.x / 2 - textSize.x / 2, boxY + padV), IM_COL32(255, 255, 255, 255), infoBuf, NULL, 0.0f, NULL);

        // ==========================================
        // [大地图鼠标悬停生物群系显示] 替换原玩家当前生物群系显示
        // 仅在鼠标悬停于已渲染区域时显示，未渲染/未加载/地狱维度 → 淡出
        // alpha lerp 实现帧率无关的平滑过渡
        // ==========================================
        {
            // 大地图刚打开时重置状态，实现淡入
            static bool wasBigMapOpen = false;
            if (!wasBigMapOpen && MapRenderState::showBigMap) {
                MapRenderState::hoverBiomeAlpha = 0.0f;
                MapRenderState::hoverBiomeTargetAlpha = 0.0f;
                MapRenderState::hoverBiomeHasValidResult = false;
                MapRenderState::hoverBiomeFrameCounter = 6;
            }
            wasBigMapOpen = MapRenderState::showBigMap;

            // 前置条件：鼠标悬停画布
            bool canQuery = isHoveringCanvas;
            MapRenderState::hoverBiomeFrameCounter++;
            bool frameThrottle = (MapRenderState::hoverBiomeFrameCounter >= 6);
            float dxh = hoverWx - MapRenderState::hoverBiomeLastQueryX;
            float dzh = hoverWz - MapRenderState::hoverBiomeLastQueryZ;
            bool movedEnough = (dxh * dxh + dzh * dzh) > 16.0f;  // 鼠标世界位移 >4 格

            if (canQuery && (frameThrottle || (movedEnough && MapRenderState::hoverBiomeFrameCounter >= 2))) {
                MapRenderState::hoverBiomeFrameCounter = 0;
                int bx = (int)std::floor(hoverWx);
                int bz = (int)std::floor(hoverWz);
                int rx = (int)std::floor(hoverWx / 256.0f);
                int rz = (int)std::floor(hoverWz / 256.0f);
                uint64_t hash = MapCacheManager::GetRegionHash(rx, rz, isCave);

                // region 渲染检查：g_regionSRVs 中存在非空 SRV 表示该区域已渲染
                bool regionRendered = false;
                auto srvIt = g_regionSRVs.find(hash);
                if (srvIt != g_regionSRVs.end() && srvIt->second) regionRendered = true;

                if (regionRendered) {
                    std::string rawName;
                    bool ok = false;
                    // [优先级1] 缓存生物群系（持久化显示核心）
                    // 玩家抵达过的区域，生物群系已由扫描采集并持久化到 region_*.bin
                    // 即使区块已卸载（玩家离开），仍可从缓存读取并显示
                    if (MapCacheManager::GetCachedBiomeName(bx, bz, rawName)) {
                        ok = true;
                    }
                    // [优先级2] 实时查询（仅对当前已加载区块有效，刚进入尚未扫描的新区域，且仅物理维度有效）
                    // 使用 SafeGetSurfaceY 验证区块已加载，避免部分加载区块返回错误生物群系
                    if (!ok && g_clientInstance && (viewDim == currentDim) && (currentDim != 1)) {
                        BlockSource* region = g_clientInstance->getRegion();
                        if (region) {
                            // 区块加载探测：SafeGetSurfaceY 返回有效 Y 表示区块已加载
                            //  - 返回 > -64 且 < 319：区块已加载，有真实地表
                            //  - 返回 ≤ -64：区块完全未加载（哨兵）
                            //  - 返回 -32000：部分加载触发 AV（SEH __except 捕获）
                            short liveY = SafeGetSurfaceY(*region, bx, bz);
                            if (liveY > -64 && liveY < 319) {
                                // 区块已加载 → 在地表 Y 实时查询生物群系（Y 精确）
                                ok = SafeGetBiomeName(*region, bx, (int)liveY, bz, rawName);
                            }
                        }
                    }
                    if (ok) {
                        // 剥离命名空间前缀
                        std::string displayRaw = rawName;
                        size_t colonPos = displayRaw.find(":");
                        if (colonPos != std::string::npos) displayRaw = displayRaw.substr(colonPos + 1);
                        MapRenderState::hoverBiomeRawName = displayRaw;
                        MapRenderState::hoverBiomeTranslatedName = TranslateBiomeName(rawName);
                        MapRenderState::hoverBiomeHasValidResult = true;
                        MapRenderState::hoverBiomeLastQueryX = hoverWx;
                        MapRenderState::hoverBiomeLastQueryZ = hoverWz;
                        MapRenderState::hoverBiomeTargetAlpha = 1.0f;
                    } else {
                        // 无缓存且区块未加载 → 淡出
                        MapRenderState::hoverBiomeTargetAlpha = 0.0f;
                        MapRenderState::hoverBiomeHasValidResult = false;
                    }
                } else {
                    // 未渲染 region → 不显示
                    MapRenderState::hoverBiomeTargetAlpha = 0.0f;
                    MapRenderState::hoverBiomeHasValidResult = false;
                }
            } else if (!canQuery) {
                // 鼠标离开画布或地狱维度 → 淡出
                MapRenderState::hoverBiomeTargetAlpha = 0.0f;
                MapRenderState::hoverBiomeHasValidResult = false;
            }

            // alpha 平滑过渡（帧率无关）
            float dt = ImGui::GetIO().DeltaTime;
            if (dt > 0.1f) dt = 0.1f;  // 防止切窗后大跳变
            float lerpF = std::clamp(12.0f * dt, 0.0f, 1.0f);
            MapRenderState::hoverBiomeAlpha +=
                (MapRenderState::hoverBiomeTargetAlpha - MapRenderState::hoverBiomeAlpha) * lerpF;

            // 绘制（仅 alpha 足够且有有效结果时）
            if (MapRenderState::hoverBiomeAlpha > 0.01f && MapRenderState::hoverBiomeHasValidResult) {
                std::string combined = MapRenderState::hoverBiomeRawName + " (" +
                                       MapRenderState::hoverBiomeTranslatedName + ")";
                char biomeBuf[512];
                snprintf(biomeBuf, sizeof(biomeBuf), LanguageManager::GetText("BIOME_LABEL"), combined.c_str());
                ImVec2 biomeTextSize = ImGui::CalcTextSize(biomeBuf);
                ImU32 bgCol = IM_COL32(0, 0, 0, (ImU32)(180 * MapRenderState::hoverBiomeAlpha));
                ImU32 textCol = IM_COL32(180, 255, 180, (ImU32)(255 * MapRenderState::hoverBiomeAlpha));
                draw_list->AddRectFilled(
                    ImVec2(io.DisplaySize.x / 2 - biomeTextSize.x / 2 - 20.0f * curScale, 15.0f * curScale),
                    ImVec2(io.DisplaySize.x / 2 + biomeTextSize.x / 2 + 20.0f * curScale, 15.0f * curScale + biomeTextSize.y + 15.0f * curScale),
                    bgCol, 5.0f * curScale);
                draw_list->AddText(mFont, mFontSize,
                    ImVec2(io.DisplaySize.x / 2 - biomeTextSize.x / 2, 22.0f * curScale), textCol, biomeBuf, NULL, 0.0f, NULL);
            }
        }

        float sidebarWidth = 270.0f * curScale;
        float sidebarHeight = 155.0f * curScale;
        ImGui::SetCursorPos(ImVec2(io.DisplaySize.x - sidebarWidth - 20.0f * curScale, 20.0f * curScale));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.6f));
        ImGui::BeginChild("MapSidebar", ImVec2(sidebarWidth, sidebarHeight), true, ImGuiWindowFlags_NoScrollbar);
        
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), LanguageManager::GetText("SIDEBAR_PLAYER_STATUS"));
        ImGui::Separator();
        ImGui::Text(LanguageManager::GetText("PLAYER_POS_X"), g_playerBlockX);
        ImGui::Text(LanguageManager::GetText("PLAYER_POS_Y"), (int)std::floor(g_playerY));
        ImGui::Text(LanguageManager::GetText("PLAYER_POS_Z"), g_playerBlockZ);
        
        ImGui::Spacing(); ImGui::Spacing();
        
        // 居中并排摆放 [⛶ 视角回中]、[👤 生物头像]、[◈ 种子地图]、[☠ 死亡记录]、[👁 地图标记]、[▦ 区块网格]、[🔲 悬停选框] 与 [⚙ 齿轮设置] 按钮
        constexpr int buttonCount = 8;
        float btnWidth = 28.0f * curScale;
        float btnSpacing = 4.0f * curScale;
        // 与侧栏实际内容宽度联动：空间不足时先压缩间隙（下限 2px），再压缩按钮尺寸，保证永不溢出
        float padX = ImGui::GetStyle().WindowPadding.x;
        float contentW = ImGui::GetWindowWidth() - padX * 2.0f;
        if (btnWidth * buttonCount + btnSpacing * (buttonCount - 1) > contentW) {
            btnSpacing = (contentW - btnWidth * buttonCount) / (buttonCount - 1);
            if (btnSpacing < 2.0f * curScale) {
                btnSpacing = 2.0f * curScale;
                btnWidth = (contentW - btnSpacing * (buttonCount - 1)) / buttonCount;
            }
        }
        float totalButtonsWidth = btnWidth * buttonCount + btnSpacing * (buttonCount - 1);
        ImGui::SetCursorPosX((ImGui::GetWindowWidth() - totalButtonsWidth) * 0.5f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(btnSpacing, ImGui::GetStyle().ItemSpacing.y));
        
        if (ImGui::Button("\xe2\x9b\xb6", ImVec2(btnWidth, btnWidth))) { // U+26F6 (Square with crosshairs)
            MapRenderState::CenterCameraOnViewDimension(g_smoothPX, g_smoothPZ);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", LanguageManager::GetText("CENTER_CAMERA"));
        }
        
        ImGui::SameLine();

        // [生物头像显示开关图标按钮] 点击切换开启/关闭
        bool showEnts = MapRenderState::bigMapShowEntities;
        if (showEnts) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.55f, 0.28f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.68f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.45f, 0.22f, 1.0f));
        }

        if (ImGui::Button("\xe2\x98\xbb##ToggleEntityHeads", ImVec2(btnWidth, btnWidth))) { // U+263B ☻ (Black Smiling Face)
            MapRenderState::bigMapShowEntities = !MapRenderState::bigMapShowEntities;
            LanguageManager::SaveConfig();
        }

        if (showEnts) {
            draw_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(80, 255, 120, 230), ImGui::GetStyle().FrameRounding, 0, 1.5f);
            ImGui::PopStyleColor(3);
        }

        if (ImGui::IsItemHovered()) {
            const char* tip = showEnts 
                ? LanguageManager::GetText("BIGMAP_ENTITIES_TOGGLE_OFF") 
                : LanguageManager::GetText("BIGMAP_ENTITIES_TOGGLE_ON");
            ImGui::SetTooltip("%s", tip);
        }

        ImGui::SameLine();

        // [种子地图开关图标按钮] 点击切换开启/关闭种子地图控制面板
        bool seedMapActive = MapRenderState::showSeedMap;
        if (seedMapActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.42f, 0.72f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.52f, 0.85f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.14f, 0.35f, 0.62f, 1.0f));
        }

        if (ImGui::Button("\xe2\x97\x88##ToggleSeedMapBtn", ImVec2(btnWidth, btnWidth))) { // U+25C8 ◈
            MapRenderState::showSeedMap = !MapRenderState::showSeedMap;
        }

        if (seedMapActive) {
            draw_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(80, 175, 255, 230), ImGui::GetStyle().FrameRounding, 0, 1.5f);
            ImGui::PopStyleColor(3);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", LanguageManager::GetText("SEED_MAP_TITLE"));
        }

        ImGui::SameLine();

        // [死亡记录开关图标按钮] 点击切换开启/关闭死亡记录面板
        bool deathActive = MapRenderState::showDeathPointUI;
        if (deathActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.22f, 0.22f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.28f, 0.28f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.52f, 0.18f, 0.18f, 1.0f));
        }

        if (ImGui::Button("\xe2\x98\xa0##ToggleDeathBtn", ImVec2(btnWidth, btnWidth))) { // U+2620 ☠
            MapRenderState::showDeathPointUI = !MapRenderState::showDeathPointUI;
        }

        if (deathActive) {
            draw_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 90, 90, 230), ImGui::GetStyle().FrameRounding, 0, 1.5f);
            ImGui::PopStyleColor(3);
        }

        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", LanguageManager::GetText("MODERN_DEATH_MANAGER"));
        }

        ImGui::SameLine();

        // [地图标记开关图标按钮] 控制路径点与死亡点在大地图上的显隐，状态持久化
        bool markersOn = MapRenderState::bigMapShowMarkers;
        if (markersOn) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.55f, 0.15f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.88f, 0.66f, 0.22f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.62f, 0.44f, 0.10f, 1.0f));
        }

        if (ImGui::Button("##ToggleMarkersBtn", ImVec2(btnWidth, btnWidth))) {
            MapRenderState::bigMapShowMarkers = !MapRenderState::bigMapShowMarkers;
            LanguageManager::SaveConfig();
        }

        // 在按钮上绘制眼睛图标（睁眼=显示；闭眼红斜杠=隐藏）
        {
            ImVec2 eyeCtr;
            eyeCtr.x = (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f;
            eyeCtr.y = (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f;
            ImU32 eyeCol = markersOn ? IM_COL32(255, 255, 255, 255) : IM_COL32(170, 170, 170, 255);
            draw_list->AddEllipse(eyeCtr, ImVec2(10.0f * curScale, 6.5f * curScale), eyeCol, 0.0f, 20, 1.7f * curScale);
            draw_list->AddCircleFilled(eyeCtr, 3.0f * curScale, eyeCol, 20);
            if (!markersOn) {
                draw_list->AddLine(ImVec2(eyeCtr.x - 8.0f * curScale, eyeCtr.y + 8.0f * curScale),
                                   ImVec2(eyeCtr.x + 8.0f * curScale, eyeCtr.y - 8.0f * curScale),
                                   IM_COL32(235, 95, 95, 255), 2.2f * curScale);
            }
        }

        if (markersOn) {
            draw_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(255, 200, 80, 230), ImGui::GetStyle().FrameRounding, 0, 1.5f);
            ImGui::PopStyleColor(3);
        }

        if (ImGui::IsItemHovered()) {
            const char* tip = markersOn
                ? LanguageManager::GetText("BIGMAP_MARKERS_TOGGLE_OFF")
                : LanguageManager::GetText("BIGMAP_MARKERS_TOGGLE_ON");
            ImGui::SetTooltip("%s", tip);
        }

        ImGui::SameLine();

        // [区块网格开关图标按钮] 同时控制小地图与大地图的 16 格区块边界显示，状态持久化
        bool gridOn = MapRenderState::showChunkGrid;
        if (gridOn) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.42f, 0.32f, 0.72f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.52f, 0.40f, 0.85f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.34f, 0.25f, 0.60f, 1.0f));
        }

        if (ImGui::Button("##ToggleChunkGridBtn", ImVec2(btnWidth, btnWidth))) {
            MapRenderState::showChunkGrid = !MapRenderState::showChunkGrid;
            LanguageManager::SaveConfig();
        }

        // 在按钮上绘制 2x2 网格图标（外框 + 十字分隔线）
        {
            ImVec2 gMin = ImGui::GetItemRectMin();
            ImVec2 gMax = ImGui::GetItemRectMax();
            ImU32 gridIconCol = gridOn ? IM_COL32(255, 255, 255, 255) : IM_COL32(200, 200, 200, 255);
            const float inset = 9.0f * curScale;
            ImVec2 fMin(gMin.x + inset, gMin.y + inset);
            ImVec2 fMax(gMax.x - inset, gMax.y - inset);
            draw_list->AddRect(fMin, fMax, gridIconCol, 2.0f * curScale, 0, 1.6f * curScale);
            draw_list->AddLine(ImVec2((fMin.x + fMax.x) * 0.5f, fMin.y), ImVec2((fMin.x + fMax.x) * 0.5f, fMax.y), gridIconCol, 1.6f * curScale);
            draw_list->AddLine(ImVec2(fMin.x, (fMin.y + fMax.y) * 0.5f), ImVec2(fMax.x, (fMin.y + fMax.y) * 0.5f), gridIconCol, 1.6f * curScale);
        }

        if (gridOn) {
            draw_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(180, 150, 255, 230), ImGui::GetStyle().FrameRounding, 0, 1.5f);
            ImGui::PopStyleColor(3);
        }

        if (ImGui::IsItemHovered()) {
            const char* tip = gridOn
                ? LanguageManager::GetText("CHUNK_GRID_TOGGLE_OFF")
                : LanguageManager::GetText("CHUNK_GRID_TOGGLE_ON");
            ImGui::SetTooltip("%s", tip);
        }

        ImGui::SameLine();

        // [鼠标悬停选框开关图标按钮] 控制全屏大地图鼠标悬停位置半透明白色框的显示，状态持久化
        bool hoverBoxOn = MapRenderState::bigMapShowHoverBox;
        if (hoverBoxOn) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.65f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.55f, 0.78f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.38f, 0.55f, 1.0f));
        }

        if (ImGui::Button("##ToggleHoverBoxBtn", ImVec2(btnWidth, btnWidth))) {
            MapRenderState::bigMapShowHoverBox = !MapRenderState::bigMapShowHoverBox;
            LanguageManager::SaveConfig();
        }

        // 在按钮上绘制单个悬停区块框图标（半透明填充矩形 + 描边）
        {
            ImVec2 gMin = ImGui::GetItemRectMin();
            ImVec2 gMax = ImGui::GetItemRectMax();
            const float inset = 7.0f * curScale;
            ImVec2 fMin(gMin.x + inset, gMin.y + inset);
            ImVec2 fMax(gMax.x - inset, gMax.y - inset);
            if (hoverBoxOn) {
                draw_list->AddRectFilled(fMin, fMax, IM_COL32(255, 255, 255, 70));
                draw_list->AddRect(fMin, fMax, IM_COL32(255, 255, 255, 255), 2.0f * curScale, 0, 1.6f * curScale);
            } else {
                draw_list->AddRect(fMin, fMax, IM_COL32(160, 160, 160, 200), 2.0f * curScale, 0, 1.4f * curScale);
                draw_list->AddLine(ImVec2(fMin.x - 1.0f * curScale, fMax.y + 1.0f * curScale), ImVec2(fMax.x + 1.0f * curScale, fMin.y - 1.0f * curScale), IM_COL32(235, 95, 95, 255), 2.0f * curScale);
            }
        }

        if (hoverBoxOn) {
            draw_list->AddRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), IM_COL32(120, 200, 255, 230), ImGui::GetStyle().FrameRounding, 0, 1.5f);
            ImGui::PopStyleColor(3);
        }

        if (ImGui::IsItemHovered()) {
            const char* tip = hoverBoxOn
                ? LanguageManager::GetText("BIGMAP_HOVER_BOX_TOGGLE_OFF")
                : LanguageManager::GetText("BIGMAP_HOVER_BOX_TOGGLE_ON");
            ImGui::SetTooltip("%s", tip);
        }

        ImGui::SameLine();

        if (ImGui::Button("\xe2\x9a\x99", ImVec2(btnWidth, btnWidth))) { // U+2699 (Gear)
            ImGui::OpenPopup("SettingsPopup");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", LanguageManager::GetText("SETTINGS_TOOLTIP"));
        }
        ImGui::PopStyleVar();
        
        ImGui::SetNextWindowSize(ImVec2(340.0f * curScale, 0));
        if (ImGui::BeginPopup("SettingsPopup")) {
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), LanguageManager::GetText("SIDEBAR_OPS"));
            ImGui::Separator();
            
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", LanguageManager::GetText("GLOBAL_UI_SCALE"));
            float availWidth = ImGui::GetContentRegionAvail().x;
            float btnSize = ImGui::GetFrameHeight();
            float inputWidth = 48.0f * curScale;
            float popupSpacing = ImGui::GetStyle().ItemSpacing.x;
            float sliderWidth = availWidth - (btnSize * 3.0f) - inputWidth - (popupSpacing * 4.0f);
            
            bool tScaleChanged = false;
            
            ImGui::PushItemWidth(sliderWidth);
            tScaleChanged |= ImGui::SliderFloat("##UIScaleSlider", &MapRenderState::globalUIScale, 0.5f, 2.5f, "%.2f x");
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            if (ImGui::ArrowButton("##UIScaleSub", ImGuiDir_Left)) {
                MapRenderState::globalUIScale -= 0.1f;
                tScaleChanged = true;
            }
            
            ImGui::SameLine();
            ImGui::PushItemWidth(inputWidth);
            tScaleChanged |= ImGui::InputFloat("##UIScaleInput", &MapRenderState::globalUIScale, 0.0f, 0.0f, "%.2f");
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            if (ImGui::ArrowButton("##UIScaleAdd", ImGuiDir_Right)) {
                MapRenderState::globalUIScale += 0.1f;
                tScaleChanged = true;
            }

            ImGui::SameLine();
            float optimalScale = MapRenderState::GetOptimalUIScale(ImGui::GetIO().DisplaySize.y);
            if (ImGui::Button("\u21BA##UIScaleReset", ImVec2(btnSize, btnSize))) {
                MapRenderState::globalUIScale = optimalScale;
                tScaleChanged = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s (%.2f x)", LanguageManager::GetText("RESET_OPTIMAL_SCALE"), optimalScale);
            }
            
            if (tScaleChanged) {
                if (MapRenderState::globalUIScale < 0.1f) MapRenderState::globalUIScale = 0.1f;
                LanguageManager::SaveConfig();
            }
            
            ImGui::Spacing();
            ImGui::Spacing();
            
            if (ImGui::Button(LanguageManager::GetText("BIGMAP_SETTINGS"), ImVec2(-1, 0))) {
                MapRenderState::showBigMapSettings = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button(LanguageManager::GetText("MINIMAP_SETTINGS"), ImVec2(-1, 0))) {
                MapRenderState::showMiniMapSettings = true;
                ImGui::CloseCurrentPopup();
            }
            // [Task 3] 快捷键设置入口按钮
            if (ImGui::Button(LanguageManager::GetText("HOTKEY_SETTINGS"), ImVec2(-1, 0))) {
                MapRenderState::showHotkeySettings = true;
                MapRenderState::g_listeningHotkey = nullptr;
                ImGui::CloseCurrentPopup();
            }
            // [洞穴地图] 洞穴设置入口按钮
            if (ImGui::Button(LanguageManager::GetText("CAVE_SETTINGS"), ImVec2(-1, 0))) {
                MapRenderState::showCaveSettings = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::Spacing();

            std::string previewName = LanguageManager::g_currentLanguage;
            for (const auto& p : LanguageManager::g_availableLanguages) {
                if (p.first == LanguageManager::g_currentLanguage) {
                    previewName = p.second;
                    break;
                }
            }
            
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", LanguageManager::GetText("LANG_SELECT"));
            ImGui::PushItemWidth(-1);
            if (ImGui::BeginCombo("##LangSelectCombo", previewName.c_str())) {
                for (const auto& p : LanguageManager::g_availableLanguages) {
                    bool isSelected = (LanguageManager::g_currentLanguage == p.first);
                    if (ImGui::Selectable(p.second.c_str(), isSelected)) {
                        LanguageManager::g_currentLanguage = p.first;
                        LanguageManager::LoadLanguage(p.first);
                        LanguageManager::SaveConfig();
                    }
                    if (isSelected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            
            ImGui::EndPopup();
        }
        
        ImGui::EndChild();
        ImGui::PopStyleColor();

        // [双击新建路径点] 在大地图空白处双击直接打开创建路径点窗口
        if (isHoveringCanvas && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !MapRenderState::showExportPNGScreen && !triggerWpMenu && !triggerDeathMenu && !triggerEntityMenu) {
            int bx = (int)std::floor(curHoverWx);
            int bz = (int)std::floor(curHoverWz);
            int by = 320;
            if (viewDim == 1) {
                by = 64;
            } else {
                int16_t cachedY = MapCacheManager::GetCachedSurfaceHeight(bx, bz, isCave);
                if (cachedY != MapCacheManager::HEIGHT_UNKNOWN && cachedY > -64 && cachedY < 319) {
                    by = (int)cachedY;
                }
                if (viewDim == 0 && MapCacheManager::IsCachedWater(bx, bz, isCave) && by < 63) {
                    by = 63;
                }
            }
            MapRenderState::addWaypointX = bx;
            MapRenderState::addWaypointY = (by == 320) ? 64 : by;
            MapRenderState::addWaypointZ = bz;
            MapRenderState::addWaypointDim = viewDim;
            MapRenderState::triggerAddWaypoint = true;
            MapRenderState::showWaypointUI = true;
        }

        // [Space / 自定义快捷键快速重置视角] 快速回到玩家位置
        bool triggerCenter = false;
        if (!io.WantTextInput) {
            const auto& hk = MapRenderState::g_hotkeys.centerCamera;
            if (!hk.IsEmpty()) {
                bool ctrlMatch = ((hk.modifiers & MapRenderState::Hotkey::HK_MOD_CTRL) != 0) == io.KeyCtrl;
                bool shiftMatch = ((hk.modifiers & MapRenderState::Hotkey::HK_MOD_SHIFT) != 0) == io.KeyShift;
                bool altMatch = ((hk.modifiers & MapRenderState::Hotkey::HK_MOD_ALT) != 0) == io.KeyAlt;
                if (ctrlMatch && shiftMatch && altMatch) {
                    ImGuiKey igk = VirtualKeyToImGuiKey(hk.key);
                    if (igk != ImGuiKey_None && ImGui::IsKeyPressed(igk)) triggerCenter = true;
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_Home)) triggerCenter = true;
        }
        if (triggerCenter) {
            MapRenderState::CenterCameraOnViewDimension(g_smoothPX, g_smoothPZ);
        }

        static float rcWorldX = 0.0f;
        static float rcWorldZ = 0.0f;

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !MapRenderState::showExportPNGScreen && !triggerWpMenu && !triggerDeathMenu && !triggerEntityMenu) {
            rcWorldX = hoverWx;
            rcWorldZ = hoverWz;
            ImGui::OpenPopup("BigMapContextMenu");
        }

        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        if (ImGui::BeginPopup("BigMapContextMenu")) {
            int bx = (int)std::floor(rcWorldX);
            int bz = (int)std::floor(rcWorldZ);
            
            // [地表直达] 全屏大地图添加地标/传送时，获取真正的底层地形表面高度
            // 探测优先级：实时区块 > 缓存高度图 > 默认 320（由 PlayerHook 两阶段探测兜底）
            // [安全] 使用 SafeGetSurfaceY (SEH 包装)，防止部分加载区块 AV 崩溃
            int by = 320; 
            if (viewDim == 1) {
                // [下界环境] 绝不使用 SafeGetSurfaceY（会命中 Y=127/128 基岩顶层）
                // 探测优先级：实时洞穴安全落脚点 [33, 100] > 缓存洞穴地表Y [33, 100] > 默认安全中层 64
                if (g_clientInstance && (viewDim == currentDim)) {
                    BlockSource* region = g_clientInstance->getRegion();
                    if (region) {
                        short safeY = SafeFindSafeSpawnY(*region, bx, bz, 1);
                        if (safeY >= 33 && safeY <= 100) {
                            by = (int)safeY;
                        }
                    }
                }
                if (by == 320) {
                    int16_t cachedY = MapCacheManager::GetCachedSurfaceHeight(bx, bz, true);
                    if (cachedY != MapCacheManager::HEIGHT_UNKNOWN && cachedY >= 33 && cachedY <= 100) {
                        by = (int)cachedY + 1;
                    }
                }
                if (by >= 120 || by < 33 || by == 320) {
                    by = 64; // 下界安全中层基准
                }
            } else {
                if (g_clientInstance && (viewDim == currentDim)) {
                    BlockSource* region = g_clientInstance->getRegion();
                    if (region) {
                        short topY = SafeGetSurfaceY(*region, bx, bz);
                        if (topY > -64 && topY < 319) {
                            by = (int)topY; // topY 已经是地表站立高度 (水面空气格/方块上方格)
                        }
                    }
                }
                // 实时未命中 → 查询缓存高度图（可识别已扫描但已卸载的区域）
                if (by == 320) {
                    int16_t cachedY = MapCacheManager::GetCachedSurfaceHeight(bx, bz, isCave);
                    if (cachedY != MapCacheManager::HEIGHT_UNKNOWN && cachedY > -64 && cachedY < 319) {
                        by = (int)cachedY;
                    }
                }
                // 水域安全强化：主世界水体在未加载或历史旧缓存时，确保高度为水面高度 (>=63)
                if (viewDim == 0 && MapCacheManager::IsCachedWater(bx, bz, isCave)) {
                    if (by < 63) by = 63;
                }
            }

            ImVec2 titleSize = ImGui::CalcTextSize(LanguageManager::GetText("CONTEXT_TITLE"));
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
            ImGui::Text("%s", LanguageManager::GetText("CONTEXT_TITLE"));
            ImGui::Separator();

            char chunkBuf[64]; snprintf(chunkBuf, sizeof(chunkBuf), LanguageManager::GetText("CHUNK_POS"), bx >> 4, bz >> 4);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(chunkBuf).x) * 0.5f);
            ImGui::TextDisabled("%s", chunkBuf);

            char blockBuf[64]; snprintf(blockBuf, sizeof(blockBuf), LanguageManager::GetText("BLOCK_POS"), bx, by, bz);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(blockBuf).x) * 0.5f);
            ImGui::TextDisabled("%s", blockBuf);
            ImGui::Separator();

            if (ImGui::Selectable(LanguageManager::GetText("COPY_COORDS"))) {
                char buf[128]; snprintf(buf, sizeof(buf), "%d %d %d", bx, by, bz);
                ImGui::SetClipboardText(buf);
            }

            if (ImGui::Selectable(LanguageManager::GetText("SHARE_LOCATION_CHAT"))) {
                if (g_localPlayer) {
                    char msg[256];
                    const char* dName = DimensionText(viewDim);
                    const char* fmt = LanguageManager::GetText("CHAT_SHARE_LOCATION");
                    std::snprintf(msg, sizeof(msg), fmt, bx, by, bz, dName);
                    std::string cmd = "say " + std::string(msg);
                    SendServerCommand(*g_localPlayer, cmd);
                }
            }
            
            ImGui::Separator();
            
            if (ImGui::Selectable(LanguageManager::GetText("CREATE_WAYPOINT"))) {
                MapRenderState::addWaypointX = bx;
                MapRenderState::addWaypointY = (viewDim == 1) ? ((by >= 33 && by <= 100) ? by : 64) : by;
                MapRenderState::addWaypointZ = bz;
                MapRenderState::addWaypointDim = viewDim;
                MapRenderState::triggerAddWaypoint = true;
                MapRenderState::showWaypointUI = true;
            }

            if (WaypointManager::HasTemporaryWaypoint()) {
                if (ImGui::Selectable(LanguageManager::GetText("SET_TEMP_WAYPOINT"))) {
                    int tempY = (viewDim == 1) ? ((by >= 33 && by <= 100) ? by : 64) : by;
                    WaypointManager::SetTemporaryWaypoint(bx, tempY, bz, viewDim);
                }
                if (ImGui::Selectable(LanguageManager::GetText("CLEAR_TEMP_WAYPOINT"))) {
                    WaypointManager::ClearTemporaryWaypoint();
                }
            } else {
                if (ImGui::Selectable(LanguageManager::GetText("SET_TEMP_WAYPOINT"))) {
                    int tempY = (viewDim == 1) ? ((by >= 33 && by <= 100) ? by : 64) : by;
                    WaypointManager::SetTemporaryWaypoint(bx, tempY, bz, viewDim);
                }
            }

            if (ImGui::Selectable(LanguageManager::GetText("TELEPORT_HERE"))) {
                MapRenderState::tpTargetX = (float)bx + 0.5f;
                // 下界传送强制设为 -999.0f，必须触发下界安全落脚点探测，绝不直接 tp 到基岩顶或实心地狱岩
                // 抬高 +0.5 格 (+1.5f 对应方块表面上方半格)，杜绝卡入地毯/半砖/睡莲
                MapRenderState::tpTargetY = (viewDim == 1) ? -999.0f : ((by > -64 && by < 319) ? ((float)by + 1.5f) : -999.0f);
                MapRenderState::tpTargetZ = (float)bz + 0.5f;
                MapRenderState::tpTargetDim = viewDim;
                MapRenderState::triggerTeleport.store(true);
                MapRenderState::showBigMap = false;
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::Separator();
            
            if (ImGui::Selectable(LanguageManager::GetText("OPEN_WP_MENU"))) {
                MapRenderState::showWaypointUI = true;
            }

            if (ImGui::Selectable(LanguageManager::GetText("EXPORT_MAP_PNG"))) {
                MapRenderState::OpenExportPNGScreen();
                MapCacheManager::InvalidateExportPreview();
            }

            if (ImGui::Selectable(LanguageManager::GetText("BIGMAP_SETTINGS"))) {
                MapRenderState::showBigMapSettings = true;
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);

        if (triggerWpMenu) {
            ImGui::OpenPopup("WaypointContextMenu");
            triggerWpMenu = false;
        }

        static std::string bigMapEditId = "";
        static bool bigMapTriggerEdit = false;

        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
        if (ImGui::BeginPopup("WaypointContextMenu")) {
            Waypoint targetWp;
            bool found = false;
            {
                std::lock_guard<std::mutex> lock(WaypointManager::g_wpMutex);
                for(auto& w : WaypointManager::g_waypoints) {
                    if(w.id == selectedWpId) {
                        targetWp = w;
                        found = true;
                        break;
                    }
                }
            }

            if (found) {
                // 标题栏: 黑色背景 + 白色标题文字
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::BeginChild("##wp_title", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() + 6), false, ImGuiWindowFlags_NoScrollbar);
                const char* titleText = targetWp.name.empty() ? LanguageManager::GetText("WAYPOINT") : targetWp.name.c_str();
                ImVec2 titleSize = ImGui::CalcTextSize(titleText);
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                ImGui::TextColored(ImVec4(1, 1, 1, 1), "%s", titleText);
                ImGui::EndChild();
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // 坐标
                char coordBuf[64]; snprintf(coordBuf, sizeof(coordBuf), "X: %d, Y: %d, Z: %d", targetWp.x, targetWp.y, targetWp.z);
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(coordBuf).x) * 0.5f);
                ImGui::TextDisabled("%s", coordBuf);
                ImGui::Separator();

                // 编辑路径点
                if (ImGui::Selectable(LanguageManager::GetText("EDIT_WP"))) {
                    bigMapEditId = selectedWpId;
                    bigMapTriggerEdit = true;
                }

                // 启用/禁用路径点 (右键直接切换启用状态)
                const char* toggleWpText = targetWp.enabled ? LanguageManager::GetText("DISABLE_WP") : LanguageManager::GetText("ENABLE_WP");
                if (ImGui::Selectable(toggleWpText)) {
                    WaypointManager::ToggleWaypoint(selectedWpId);
                }

                // 复制坐标
                if (ImGui::Selectable(LanguageManager::GetText("COPY_COORDS"))) {
                    char buf[128]; snprintf(buf, sizeof(buf), "%d %d %d", targetWp.x, targetWp.y, targetWp.z);
                    ImGui::SetClipboardText(buf);
                }

                // 传送到路径点
                if (ImGui::Selectable(LanguageManager::GetText("TELEPORT_WP"))) {
                    MapRenderState::tpTargetX = (float)targetWp.x + 0.5f;
                    MapRenderState::tpTargetY = (float)targetWp.y;
                    MapRenderState::tpTargetZ = (float)targetWp.z + 0.5f;
                    MapRenderState::tpTargetDim = targetWp.dimId;
                    MapRenderState::triggerTeleport.store(true);
                    MapRenderState::showBigMap = false;
                    ImGui::CloseCurrentPopup();
                }

                // 在聊天栏分享
                if (ImGui::Selectable(LanguageManager::GetText("SHARE_WP"))) {
                    if (g_localPlayer) {
                        char msg[256];
                        const char* fmt = LanguageManager::GetText("CHAT_SHARE_WAYPOINT");
                        std::snprintf(msg, sizeof(msg), fmt,
                                      targetWp.name.c_str(),
                                      targetWp.x, targetWp.y, targetWp.z);
                        std::string cmd = "say " + std::string(msg);
                        SendServerCommand(*g_localPlayer, cmd);
                    }
                }

                // 恢复已删除路径点
                if (WaypointManager::g_hasDeletedWaypoint) {
                    if (ImGui::Selectable(LanguageManager::GetText("RESTORE_WP"))) {
                        WaypointManager::RestoreLastDeletedWaypoint();
                    }
                }

                ImGui::Separator();

                // 删除路径点
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Selectable(LanguageManager::GetText("CONFIRM_DELETE"))) {
                    WaypointManager::RemoveWaypoint(selectedWpId);
                }
                ImGui::PopStyleColor();
            } else {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);

        // ==========================================
        // 死亡点右键/点击交互菜单 (DeathPointContextMenu)
        // ==========================================
        if (triggerDeathMenu) {
            ImGui::OpenPopup("DeathPointContextMenu");
            triggerDeathMenu = false;
        }

        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.7f, 0.25f, 0.25f, 1.0f));
        if (ImGui::BeginPopup("DeathPointContextMenu")) {
            DeathPoint targetDp;
            bool found = false;
            {
                std::lock_guard<std::mutex> lock(DeathPointManager::g_deathMutex);
                for (const auto& dp : DeathPointManager::g_deathPoints) {
                    if (dp.id == selectedDeathPointId) {
                        targetDp = dp;
                        found = true;
                        break;
                    }
                }
            }

            if (found) {
                // 标题栏: 警示深红底色 + 鲜红标题
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.22f, 0.04f, 0.04f, 1.0f));
                ImGui::BeginChild("##dp_title", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() + 6), false, ImGuiWindowFlags_NoScrollbar);
                char titleText[128];
                snprintf(titleText, sizeof(titleText), "%s (%s)",
                         LanguageManager::GetText("DEATH_POINT_WP_PREFIX"),
                         DimensionText(targetDp.dimensionId));
                ImVec2 titleSize = ImGui::CalcTextSize(titleText);
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", titleText);
                ImGui::EndChild();
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // 坐标
                char coordBuf[64];
                snprintf(coordBuf, sizeof(coordBuf), "X: %d, Y: %d, Z: %d", targetDp.x, targetDp.y, targetDp.z);
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(coordBuf).x) * 0.5f);
                ImGui::TextDisabled("%s", coordBuf);

                // 死亡时间
                std::string timeStr = FormatDeathTime(targetDp.timestamp);
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(timeStr.c_str()).x) * 0.5f);
                ImGui::TextDisabled("%s", timeStr.c_str());

                ImGui::Separator();

                // 1. 传送到死亡点 (支持跨维度传送)
                if (ImGui::Selectable(LanguageManager::GetText("DEATH_POINT_TELEPORT"))) {
                    MapRenderState::tpTargetX = (float)targetDp.x + 0.5f;
                    MapRenderState::tpTargetY = (float)targetDp.y;
                    MapRenderState::tpTargetZ = (float)targetDp.z + 0.5f;
                    MapRenderState::tpTargetDim = targetDp.dimensionId;
                    MapRenderState::triggerTeleport.store(true);
                    MapRenderState::showBigMap = false;
                    ImGui::CloseCurrentPopup();
                }

                // 2. 转为路径点 (保存至普通路径点列表；对应路径点删除后可再次转换)
                bool isConverted = IsDeathPointConverted(targetDp);
                if (isConverted) ImGui::BeginDisabled();
                const char* wpItemLabel = isConverted ? LanguageManager::GetText("DEATH_POINT_ALREADY_CONVERTED") : LanguageManager::GetText("DEATH_POINT_CREATE_WP");
                if (ImGui::Selectable(wpItemLabel)) {
                    if (!isConverted) {
                        std::string wpName = std::string(LanguageManager::GetText("DEATH_POINT_WP_PREFIX")) + " " + FormatDeathTime(targetDp.timestamp);
                        std::string newWaypointId = WaypointManager::AddWaypoint(wpName, targetDp.x, targetDp.y, targetDp.z, 0.95f, 0.25f, 0.25f, targetDp.dimensionId);
                        WaypointManager::SaveWaypoints();
                        DeathPointManager::SetConverted(targetDp.id, true, newWaypointId);
                    }
                }
                if (isConverted) ImGui::EndDisabled();

                // 3. 复制坐标
                if (ImGui::Selectable(LanguageManager::GetText("COPY_COORDS"))) {
                    char buf[128];
                    snprintf(buf, sizeof(buf), "%d %d %d", targetDp.x, targetDp.y, targetDp.z);
                    ImGui::SetClipboardText(buf);
                }

                // 4. 在聊天栏分享
                if (ImGui::Selectable(LanguageManager::GetText("SHARE_WP"))) {
                    if (g_localPlayer) {
                        char msg[256];
                        const char* fmt = LanguageManager::GetText("CHAT_SHARE_DEATHPOINT");
                        std::snprintf(msg, sizeof(msg), fmt,
                                      targetDp.x, targetDp.y, targetDp.z,
                                      DimensionText(targetDp.dimensionId));
                        std::string cmd = "say " + std::string(msg);
                        SendServerCommand(*g_localPlayer, cmd);
                    }
                }

                // 5. 打开死亡记录
                if (ImGui::Selectable(LanguageManager::GetText("OPEN_DEATH_MANAGER"))) {
                    MapRenderState::showDeathPointUI = true;
                }

                ImGui::Separator();

                // 6. 删除死亡点
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                if (ImGui::Selectable(LanguageManager::GetText("DEATH_POINT_DELETE"))) {
                    DeathPointManager::RemoveDeathPoint(selectedDeathPointId);
                }
                ImGui::PopStyleColor();
            } else {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);

        // ==========================================
        // 实体右键/点击交互菜单 (EntityContextMenu)
        // ==========================================
        if (triggerEntityMenu) {
            ImGui::OpenPopup("EntityContextMenu");
            triggerEntityMenu = false;
        }

        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.75f, 0.95f, 1.0f));
        if (ImGui::BeginPopup("EntityContextMenu")) {
            std::string displayName;
            const char* catTag = "";
            if (selectedEntity.type == 0) {
                catTag = LanguageManager::GetText("RADAR_CAT_PLAYER");
                displayName = !selectedEntity.nameTag.empty() ? selectedEntity.nameTag : LanguageManager::GetText("RADAR_CAT_PLAYER");
            } else if (selectedEntity.type == 1) {
                catTag = LanguageManager::GetText("RADAR_CAT_HOSTILE");
                displayName = LanguageManager::GetEntityDisplayName(selectedEntity.typeName, selectedEntity.nameTag);
            } else if (selectedEntity.type == 2) {
                catTag = LanguageManager::GetText("RADAR_CAT_FRIENDLY");
                displayName = LanguageManager::GetEntityDisplayName(selectedEntity.typeName, selectedEntity.nameTag);
            } else {
                catTag = LanguageManager::GetText("RADAR_CAT_ITEM");
                displayName = LanguageManager::GetEntityDisplayName(selectedEntity.typeName, selectedEntity.nameTag);
            }

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
            ImGui::BeginChild("##ent_title", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() + 6), false, ImGuiWindowFlags_NoScrollbar);
            char titleBuf[256];
            snprintf(titleBuf, sizeof(titleBuf), "[%s] %s", catTag, displayName.c_str());
            ImVec2 titleSize = ImGui::CalcTextSize(titleBuf);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleSize.x) * 0.5f);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
            ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", titleBuf);
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::Spacing();
            char coordBuf[64];
            snprintf(coordBuf, sizeof(coordBuf), "X: %.1f, Y: %.1f, Z: %.1f", selectedEntity.x, selectedEntity.y, selectedEntity.z);
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize(coordBuf).x) * 0.5f);
            ImGui::TextDisabled("%s", coordBuf);
            ImGui::Separator();

            // 1. 传送到该玩家 / 实体
            const char* tpText = (selectedEntity.type == 0) ? LanguageManager::GetText("TELEPORT_TO_PLAYER") : LanguageManager::GetText("TELEPORT_TO_ENTITY");
            if (ImGui::Selectable(tpText)) {
                if (selectedEntity.type == 0 && !selectedEntity.nameTag.empty() && g_localPlayer) {
                    std::string cmd = "tp " + selectedEntity.nameTag;
                    SendServerCommand(*g_localPlayer, cmd);
                } else {
                    MapRenderState::tpTargetX = selectedEntity.x;
                    MapRenderState::tpTargetY = selectedEntity.y + 0.2f;
                    MapRenderState::tpTargetZ = selectedEntity.z;
                    MapRenderState::tpTargetDim = viewDim;
                    MapRenderState::triggerTeleport.store(true);
                }
                MapRenderState::showBigMap = false;
                ImGui::CloseCurrentPopup();
            }

            // 2. 在聊天栏中分享位置
            if (ImGui::Selectable(LanguageManager::GetText("SHARE_LOCATION_CHAT"))) {
                if (g_localPlayer) {
                    char msg[256];
                    const char* dName = DimensionText(viewDim);
                    const char* fmt = LanguageManager::GetText("SHARE_PLAYER_LOCATION");
                    const char* targetName = (selectedEntity.type == 0 && !selectedEntity.nameTag.empty()) 
                        ? selectedEntity.nameTag.c_str() 
                        : displayName.c_str();
                    std::snprintf(msg, sizeof(msg), fmt,
                                  targetName,
                                  (int)std::floor(selectedEntity.x),
                                  (int)std::floor(selectedEntity.y),
                                  (int)std::floor(selectedEntity.z),
                                  dName);
                    std::string cmd = "say " + std::string(msg);
                    SendServerCommand(*g_localPlayer, cmd);
                }
            }

            // 3. 在此处创建路径点
            if (ImGui::Selectable(LanguageManager::GetText("CREATE_WAYPOINT"))) {
                MapRenderState::addWaypointX = (int)std::floor(selectedEntity.x);
                MapRenderState::addWaypointY = (int)std::floor(selectedEntity.y);
                MapRenderState::addWaypointZ = (int)std::floor(selectedEntity.z);
                MapRenderState::addWaypointDim = viewDim;
                MapRenderState::triggerAddWaypoint = true;
                MapRenderState::showWaypointUI = true;
            }

            // 4. 复制坐标
            if (ImGui::Selectable(LanguageManager::GetText("COPY_COORDS"))) {
                char buf[128];
                snprintf(buf, sizeof(buf), "%d %d %d", (int)std::floor(selectedEntity.x), (int)std::floor(selectedEntity.y), (int)std::floor(selectedEntity.z));
                ImGui::SetClipboardText(buf);
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);

        // 调用大地图右键地标重命名弹窗模块
        RenderEditModal((std::string(LanguageManager::GetText("EDIT_WP_TITLE")) + "##ModalBigMapEdit").c_str(), bigMapEditId, bigMapTriggerEdit);

        // ==========================================
        // [大地图缩放按钮 (+ / -)] 合并归一，浮动于右下角
        // 融合最大放大(40.0x)与最小缩小(0.05x)优势，支持按住连击与屏幕中心平滑锚定缩放
        // ==========================================
        if (MapRenderState::showZoomButtons && !MapRenderState::showExportPNGScreen) {
            float zBtnSz = 32.0f * curScale;
            float zoomBarX = io.DisplaySize.x - 20.0f * curScale - zBtnSz;
            float zoomBarY = io.DisplaySize.y - 20.0f * curScale - (zBtnSz * 2.0f + 6.0f * curScale);

            ImGui::SetCursorPos(ImVec2(zoomBarX, zoomBarY));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f * curScale);
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 6.0f * curScale));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.12f, 0.15f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.30f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.35f, 0.40f, 1.0f));

            ImGui::BeginGroup();

            // 1. 放大按钮 (+) - 支持最大放大至 40.0f
            bool canZoomIn = (MapRenderState::bigMapZoom < 40.0f);
            if (!canZoomIn) ImGui::BeginDisabled();
            ImGui::PushButtonRepeat(true);
            bool clickZoomIn = ImGui::Button("##BigMapZoomInMerged", ImVec2(zBtnSz, zBtnSz));
            if (ImGui::IsItemActive()) s_isDraggingMap = false;
            ImGui::PopButtonRepeat();
            if (!canZoomIn) ImGui::EndDisabled();

            // 绘制矢量加号 (+) 图标与边框
            {
                ImVec2 bMin = ImGui::GetItemRectMin();
                ImVec2 bMax = ImGui::GetItemRectMax();
                ImVec2 ctr((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);
                float arm = 6.0f * curScale;
                bool isHov = ImGui::IsItemHovered();
                ImU32 iconCol = canZoomIn ? (isHov ? IM_COL32(255, 255, 255, 255) : IM_COL32(220, 220, 225, 230)) : IM_COL32(120, 120, 125, 120);
                ImU32 borderCol = canZoomIn ? (isHov ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 30)) : IM_COL32(255, 255, 255, 15);
                draw_list->AddRect(bMin, bMax, borderCol, 5.0f * curScale, 0, 1.0f);
                draw_list->AddLine(ImVec2(ctr.x - arm, ctr.y), ImVec2(ctr.x + arm, ctr.y), iconCol, 2.0f * curScale);
                draw_list->AddLine(ImVec2(ctr.x, ctr.y - arm), ImVec2(ctr.x, ctr.y + arm), iconCol, 2.0f * curScale);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", LanguageManager::GetText("BIGMAP_ZOOM_IN"));
            }

            // 2. 缩小按钮 (-) - 支持最小缩小至 0.05f
            bool canZoomOut = (MapRenderState::bigMapZoom > 0.05f);
            if (!canZoomOut) ImGui::BeginDisabled();
            ImGui::PushButtonRepeat(true);
            bool clickZoomOut = ImGui::Button("##BigMapZoomOutMerged", ImVec2(zBtnSz, zBtnSz));
            if (ImGui::IsItemActive()) s_isDraggingMap = false;
            ImGui::PopButtonRepeat();
            if (!canZoomOut) ImGui::EndDisabled();

            // 绘制矢量减号 (-) 图标与边框
            {
                ImVec2 bMin = ImGui::GetItemRectMin();
                ImVec2 bMax = ImGui::GetItemRectMax();
                ImVec2 ctr((bMin.x + bMax.x) * 0.5f, (bMin.y + bMax.y) * 0.5f);
                float arm = 6.0f * curScale;
                bool isHov = ImGui::IsItemHovered();
                ImU32 iconCol = canZoomOut ? (isHov ? IM_COL32(255, 255, 255, 255) : IM_COL32(220, 220, 225, 230)) : IM_COL32(120, 120, 125, 120);
                ImU32 borderCol = canZoomOut ? (isHov ? IM_COL32(255, 255, 255, 60) : IM_COL32(255, 255, 255, 30)) : IM_COL32(255, 255, 255, 15);
                draw_list->AddRect(bMin, bMax, borderCol, 5.0f * curScale, 0, 1.0f);
                draw_list->AddLine(ImVec2(ctr.x - arm, ctr.y), ImVec2(ctr.x + arm, ctr.y), iconCol, 2.0f * curScale);
            }
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                ImGui::SetTooltip("%s", LanguageManager::GetText("BIGMAP_ZOOM_OUT"));
            }

            ImGui::EndGroup();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar(2);

            // 执行平滑无漂移以屏幕中心为锚点的缩放 (范围 0.05f ~ 40.0f)
            if (clickZoomIn && canZoomIn) {
                float oldZoom = MapRenderState::bigMapZoom;
                float newZoom = oldZoom * 1.20f;
                if (newZoom > 40.0f) newZoom = 40.0f;
                if (newZoom != oldZoom) {
                    float k = newZoom / oldZoom;
                    MapRenderState::bigMapZoom = newZoom;
                    MapRenderState::bigMapOffsetX *= k;
                    MapRenderState::bigMapOffsetZ *= k;
                }
                s_isDraggingMap = false;
            }
            if (clickZoomOut && canZoomOut) {
                float oldZoom = MapRenderState::bigMapZoom;
                float newZoom = oldZoom / 1.20f;
                if (newZoom < 0.05f) newZoom = 0.05f;
                if (newZoom != oldZoom) {
                    float k = newZoom / oldZoom;
                    MapRenderState::bigMapZoom = newZoom;
                    MapRenderState::bigMapOffsetX *= k;
                    MapRenderState::bigMapOffsetZ *= k;
                }
                s_isDraggingMap = false;
            }
        }

        // 渲染种子地图悬浮控制面板
        RenderSeedMapPanel();

        ImGui::End();
        ImGui::PopStyleVar(2);
    }

    // ==========================================
    // [Task 3] 虚拟键码 → 可读名称转换
    // ==========================================
    inline std::string GetVKKeyName(int vk) {
        if (vk >= '0' && vk <= '9') return std::string(1, (char)vk);
        if (vk >= 'A' && vk <= 'Z') return std::string(1, (char)vk);
        switch (vk) {
            case VK_SPACE:   return "Space";
            case VK_RETURN:  return "Enter";
            case VK_TAB:     return "Tab";
            case VK_ESCAPE:  return "Esc";
            case VK_BACK:    return "Backspace";
            case VK_INSERT:  return "Insert";
            case VK_DELETE:  return "Delete";
            case VK_HOME:    return "Home";
            case VK_END:     return "End";
            case VK_PRIOR:   return "Page Up";
            case VK_NEXT:    return "Page Down";
            case VK_LEFT:    return "Left";
            case VK_RIGHT:   return "Right";
            case VK_UP:      return "Up";
            case VK_DOWN:    return "Down";
            case VK_CAPITAL: return "Caps Lock";
            case VK_NUMLOCK: return "Num Lock";
            case VK_SCROLL:  return "Scroll Lock";
            case VK_SHIFT:   return "Shift";
            case VK_CONTROL: return "Ctrl";
            case VK_MENU:    return "Alt";
            case VK_LWIN:    return "Win (L)";
            case VK_RWIN:    return "Win (R)";
            case VK_OEM_3:   return "`";
            case VK_OEM_MINUS: return "-";
            case VK_OEM_PLUS:  return "=";
            case VK_OEM_5:   return "\\";
            case VK_OEM_4:   return "[";
            case VK_OEM_6:   return "]";
            case VK_OEM_1:   return ";";
            case VK_OEM_7:   return "'";
            case VK_OEM_COMMA: return ",";
            case VK_OEM_PERIOD: return ".";
            case VK_OEM_2:   return "/";
            case VK_F1:  return "F1";  case VK_F2:  return "F2";
            case VK_F3:  return "F3";  case VK_F4:  return "F4";
            case VK_F5:  return "F5";  case VK_F6:  return "F6";
            case VK_F7:  return "F7";  case VK_F8:  return "F8";
            case VK_F9:  return "F9";  case VK_F10: return "F10";
            case VK_F11: return "F11"; case VK_F12: return "F12";
            case VK_F13: return "F13"; case VK_F14: return "F14";
            case VK_F15: return "F15"; case VK_F16: return "F16";
            case VK_F17: return "F17"; case VK_F18: return "F18";
            case VK_F19: return "F19"; case VK_F20: return "F20";
            case VK_F21: return "F21"; case VK_F22: return "F22";
            case VK_F23: return "F23"; case VK_F24: return "F24";
            default: {
                char buf[16];
                snprintf(buf, sizeof(buf), "VK 0x%02X", vk);
                return buf;
            }
        }
    }

    // ==========================================
    // [快捷键增强] 格式化热键名称（包含修饰键前缀）
    // ==========================================
    inline std::string GetHotkeyName(const MapRenderState::Hotkey& hk) {
        if (hk.IsEmpty()) return LanguageManager::GetText("HOTKEY_DISABLED");
        std::string result;
        if (hk.modifiers & MapRenderState::Hotkey::HK_MOD_CTRL)  result += "Ctrl + ";
        if (hk.modifiers & MapRenderState::Hotkey::HK_MOD_SHIFT) result += "Shift + ";
        if (hk.modifiers & MapRenderState::Hotkey::HK_MOD_ALT)   result += "Alt + ";
        result += GetVKKeyName(hk.key);
        return result;
    }

    // ==========================================
    // [Task 3] 快捷键设置面板 (查看/重绑/重置所有快捷键，支持多键组合)
    // ==========================================
    inline void RenderHotkeySettingsWindow() {
        // 面板关闭时清除监听状态 (处理 X 按钮关闭的情况)
        if (!MapRenderState::showHotkeySettings) {
            if (MapRenderState::g_listeningHotkey != nullptr) {
                MapRenderState::g_listeningHotkey = nullptr;
            }
            return;
        }

        // === 持久化 UI 状态 (跨帧保持，面板关闭后重置) ===
        // 撤销条目：单级，支持批量变更 (如"全部重置"包含 6 个变更)
        struct UndoChange { MapRenderState::Hotkey* target; MapRenderState::Hotkey prevValue; };
        struct UndoEntry { bool valid = false; std::string label; std::vector<UndoChange> changes; };
        static UndoEntry s_undo;
        // 行闪烁：重置=绿, 清除=红, 撤销=蓝 (从闪烁色插值到正常色)
        struct FlashEntry { MapRenderState::Hotkey* bindPtr; float timeLeft; ImVec4 flashColor; };
        static std::vector<FlashEntry> s_flashes;
        // 底部状态消息 (带淡出效果)
        struct StatusMsg { std::string text; float timeLeft = 0.0f; ImVec4 color; };
        static StatusMsg s_status;

        static constexpr float kFlashDuration  = 1.5f; // 闪烁持续秒数
        static constexpr float kStatusDuration = 3.0f; // 状态消息持续秒数

        // 衰减计时器 (每帧更新)
        float dt = ImGui::GetIO().DeltaTime;
        for (auto& f : s_flashes) f.timeLeft -= dt;
        s_flashes.erase(std::remove_if(s_flashes.begin(), s_flashes.end(),
            [](const FlashEntry& f) { return f.timeLeft <= 0.0f; }), s_flashes.end());
        if (s_status.timeLeft > 0.0f) s_status.timeLeft -= dt;

        float curScale = MapRenderState::globalUIScale;
        if (curScale < 0.1f) curScale = 1.0f;

        ImGui::SetNextWindowSize(ImVec2(640.0f * curScale, 420.0f * curScale), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(400.0f * curScale, 300.0f * curScale), ImVec2(1200.0f * curScale, 900.0f * curScale));
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 320.0f * curScale, ImGui::GetIO().DisplaySize.y / 2 - 210.0f * curScale), ImGuiCond_FirstUseEver);

        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin(LanguageManager::GetText("HOTKEY_SETTINGS_TITLE"), &MapRenderState::showHotkeySettings, winFlags)) {

            // === 辅助函数 ===
            // 添加/刷新某行的闪烁 (绿色=重置, 红色=清除, 蓝色=撤销)
            auto addFlash = [](MapRenderState::Hotkey* bindPtr, ImVec4 color) {
                for (auto& f : s_flashes) {
                    if (f.bindPtr == bindPtr) { f.timeLeft = kFlashDuration; f.flashColor = color; return; }
                }
                s_flashes.push_back({ bindPtr, kFlashDuration, color });
            };
            // 设置底部状态消息
            auto setStatus = [](const char* text, ImVec4 color) {
                s_status.text = text ? text : "";
                s_status.timeLeft = kStatusDuration;
                s_status.color = color;
            };
            // 推入撤销条目 (覆盖上一条，单级撤销)
            auto pushUndo = [](const char* label, std::vector<UndoChange> changes) {
                s_undo.valid = true;
                s_undo.label = label ? label : "";
                s_undo.changes = std::move(changes);
            };
            // 执行撤销：恢复所有变更，蓝色闪烁，清空撤销条目
            auto performUndo = [&]() {
                if (!s_undo.valid) return;
                for (auto& c : s_undo.changes) {
                    if (c.target) {
                        if (c.target == &MapRenderState::g_hotkeys.openBigMap && c.prevValue.IsEmpty()) {
                            c.prevValue = MapRenderState::HotkeyBindings::Defaults().openBigMap;
                        }
                        *c.target = c.prevValue;
                        addFlash(c.target, ImVec4(0.2f, 0.4f, 0.8f, 1.0f)); // 蓝色闪烁
                    }
                }
                setStatus(LanguageManager::GetText("HOTKEY_STATUS_UNDONE"), ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
                s_undo.valid = false;
                MapRenderState::g_listeningHotkey = nullptr;
                LanguageManager::SaveConfig();
            };

            // Ctrl+Z 撤销快捷键 (由 WndProc 设置标志，此处消费)
            if (MapRenderState::g_hotkeyUndoRequested.exchange(false) && s_undo.valid) {
                performUndo();
            }

            // 快捷键提示条 (提示支持组合键)
            ImGui::TextColored(ImVec4(0.65f, 0.75f, 0.85f, 1.0f), "%s", LanguageManager::GetText("HOTKEY_COMBO_HINT"));
            ImGui::Spacing();

            // === 四列表格：操作 | 按键 | 重置 | 清除 ===
            ImGui::Columns(4, "HkCols", false);
            float colW = ImGui::GetWindowWidth();
            ImGui::SetColumnWidth(0, colW * 0.38f);
            ImGui::SetColumnWidth(1, colW * 0.28f);
            ImGui::SetColumnWidth(2, colW * 0.17f);
            ImGui::SetColumnWidth(3, colW * 0.17f);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", LanguageManager::GetText("HOTKEY_ACTION"));
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", LanguageManager::GetText("HOTKEY_KEY"));
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", LanguageManager::GetText("HOTKEY_RESET"));
            ImGui::NextColumn();
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", LanguageManager::GetText("HOTKEY_CLEAR"));
            ImGui::NextColumn();
            ImGui::Separator();

            // 单行渲染：操作名 | 按键按钮(监听/禁用/闪烁态) | 重置按钮 | 清除按钮
            auto renderRow = [&](const char* actionName, MapRenderState::Hotkey* bindPtr, MapRenderState::Hotkey defaultHk, bool allowClear = true) {
                ImGui::PushID(bindPtr);
                ImGui::Text("%s", actionName);
                ImGui::NextColumn();

                // --- 按键列 ---
                int popCount = 0;
                bool useRedText = false;
                if (MapRenderState::g_listeningHotkey == bindPtr) {
                    // 监听中：动态实时捕获当前按下的修饰键 (Ctrl / Shift / Alt)
                    uint8_t curMods = MapRenderState::g_listeningModifiers;
                    if ((ReadPhysicalKeyState(VK_CONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_LCONTROL) & 0x8000) != 0 || (ReadPhysicalKeyState(VK_RCONTROL) & 0x8000) != 0) curMods |= MapRenderState::Hotkey::HK_MOD_CTRL;
                    if ((ReadPhysicalKeyState(VK_SHIFT) & 0x8000) != 0   || (ReadPhysicalKeyState(VK_LSHIFT) & 0x8000) != 0   || (ReadPhysicalKeyState(VK_RSHIFT) & 0x8000) != 0)   curMods |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                    if ((ReadPhysicalKeyState(VK_MENU) & 0x8000) != 0    || (ReadPhysicalKeyState(VK_LMENU) & 0x8000) != 0    || (ReadPhysicalKeyState(VK_RMENU) & 0x8000) != 0)    curMods |= MapRenderState::Hotkey::HK_MOD_ALT;
                    if (ImGui::GetCurrentContext()) {
                        if (ImGui::GetIO().KeyCtrl)  curMods |= MapRenderState::Hotkey::HK_MOD_CTRL;
                        if (ImGui::GetIO().KeyShift) curMods |= MapRenderState::Hotkey::HK_MOD_SHIFT;
                        if (ImGui::GetIO().KeyAlt)   curMods |= MapRenderState::Hotkey::HK_MOD_ALT;
                    }

                    std::string listeningText;
                    if (curMods & MapRenderState::Hotkey::HK_MOD_CTRL)  listeningText += "Ctrl + ";
                    if (curMods & MapRenderState::Hotkey::HK_MOD_SHIFT) listeningText += "Shift + ";
                    if (curMods & MapRenderState::Hotkey::HK_MOD_ALT)   listeningText += "Alt + ";
                    listeningText += "...";

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.5f, 0.1f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.6f, 0.15f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.4f, 0.08f, 1.0f));
                    popCount = 3;

                    std::string keyBtnId = listeningText + "##Key";
                    if (ImGui::Button(keyBtnId.c_str(), ImVec2(-1, 0))) {
                        MapRenderState::g_listeningHotkey = nullptr;
                        MapRenderState::g_listeningModifiers = 0;
                    }
                } else {
                    bool disabled = bindPtr->IsEmpty();
                    // 查找当前行的闪烁条目
                    float flashTime = 0.0f;
                    ImVec4 flashColor;
                    bool flashing = false;
                    for (auto& f : s_flashes) {
                        if (f.bindPtr == bindPtr) { flashTime = f.timeLeft; flashColor = f.flashColor; flashing = true; break; }
                    }
                    if (flashing) {
                        float t = flashTime / kFlashDuration;
                        ImVec4 normal = ImGui::GetStyle().Colors[ImGuiCol_Button];
                        ImVec4 cur(
                            normal.x + (flashColor.x - normal.x) * t,
                            normal.y + (flashColor.y - normal.y) * t,
                            normal.z + (flashColor.z - normal.z) * t, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_Button, cur);
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, cur);
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, cur);
                        popCount = 3;
                    } else if (disabled) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.15f, 0.15f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.2f, 0.2f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.12f, 0.12f, 1.0f));
                        popCount = 3;
                        useRedText = true;
                    }
                    if (useRedText) ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.6f, 0.6f, 1.0f));
                    std::string keyName = GetHotkeyName(*bindPtr);
                    std::string keyBtnId = keyName + "##Key";
                    if (ImGui::Button(keyBtnId.c_str(), ImVec2(-1, 0))) {
                        MapRenderState::g_listeningHotkey = bindPtr;
                        MapRenderState::g_listeningModifiers = 0;
                    }
                    if (useRedText) ImGui::PopStyleColor();
                }
                if (popCount > 0) ImGui::PopStyleColor(popCount);
                ImGui::NextColumn();

                // --- 重置列 (恢复该行为默认按键) ---
                {
                    bool isDefault = (*bindPtr == defaultHk);
                    if (isDefault) ImGui::BeginDisabled();
                    std::string resetBtnId = std::string(LanguageManager::GetText("HOTKEY_RESET")) + "##Reset";
                    if (ImGui::Button(resetBtnId.c_str(), ImVec2(-1, 0))) {
                        if (*bindPtr != defaultHk) {
                            pushUndo(LanguageManager::GetText("HOTKEY_STATUS_RESET"), { {bindPtr, *bindPtr} });
                            *bindPtr = defaultHk;
                            addFlash(bindPtr, ImVec4(0.2f, 0.7f, 0.3f, 1.0f)); // 绿色闪烁
                            setStatus(LanguageManager::GetText("HOTKEY_STATUS_RESET"), ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
                            MapRenderState::g_listeningHotkey = nullptr;
                            LanguageManager::SaveConfig();
                        }
                    }
                    if (isDefault) ImGui::EndDisabled();
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("HOTKEY_STATUS_RESET"));
                }
                ImGui::NextColumn();

                // --- 清除列 (设置为空=禁用该快捷键) ---
                {
                    bool isCleared = bindPtr->IsEmpty();
                    bool cantClear = !allowClear;
                    if (isCleared || cantClear) ImGui::BeginDisabled();
                    std::string clearBtnId = std::string(LanguageManager::GetText("HOTKEY_CLEAR")) + "##Clear";
                    if (ImGui::Button(clearBtnId.c_str(), ImVec2(-1, 0))) {
                        if (!bindPtr->IsEmpty() && allowClear) {
                            pushUndo(LanguageManager::GetText("HOTKEY_STATUS_CLEARED"), { {bindPtr, *bindPtr} });
                            bindPtr->Clear();
                            addFlash(bindPtr, ImVec4(0.8f, 0.2f, 0.2f, 1.0f)); // 红色闪烁
                            setStatus(LanguageManager::GetText("HOTKEY_STATUS_CLEARED"), ImVec4(0.95f, 0.4f, 0.4f, 1.0f));
                            MapRenderState::g_listeningHotkey = nullptr;
                            LanguageManager::SaveConfig();
                        }
                    }
                    if (isCleared || cantClear) ImGui::EndDisabled();
                    if (cantClear) {
                        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                            ImGui::SetTooltip("%s", LanguageManager::GetText("HOTKEY_CANNOT_CLEAR"));
                        }
                    } else {
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("%s", LanguageManager::GetText("HOTKEY_STATUS_CLEARED"));
                        }
                    }
                }
                ImGui::NextColumn();
                ImGui::PopID();
            };

            auto defaults = MapRenderState::HotkeyBindings::Defaults();
            renderRow(LanguageManager::GetText("HOTKEY_OPEN_BIGMAP"),     &MapRenderState::g_hotkeys.openBigMap,        defaults.openBigMap, false);
            renderRow(LanguageManager::GetText("HOTKEY_OPEN_WPMGR"),      &MapRenderState::g_hotkeys.openWaypointMgr,    defaults.openWaypointMgr);
            renderRow(LanguageManager::GetText("HOTKEY_OPEN_DEATH_MGR"),  &MapRenderState::g_hotkeys.openDeathPointMgr,  defaults.openDeathPointMgr);
            renderRow(LanguageManager::GetText("HOTKEY_TOGGLE_MINIMAP"),  &MapRenderState::g_hotkeys.toggleMinimap,      defaults.toggleMinimap);
            renderRow(LanguageManager::GetText("HOTKEY_TOGGLE_SHAPE"),    &MapRenderState::g_hotkeys.toggleMinimapShape, defaults.toggleMinimapShape);
            renderRow(LanguageManager::GetText("HOTKEY_TOGGLE_ROTATION"), &MapRenderState::g_hotkeys.toggleMinimapRot,   defaults.toggleMinimapRot);
            renderRow(LanguageManager::GetText("HOTKEY_HOLD_ENTITIES"),   &MapRenderState::g_hotkeys.holdEntities,      defaults.holdEntities, true);
            renderRow(LanguageManager::GetText("HOTKEY_TOGGLE_SEEDMAP"),  &MapRenderState::g_hotkeys.toggleSeedMap,     defaults.toggleSeedMap, true);
            renderRow(LanguageManager::GetText("HOTKEY_ENLARGE_MINIMAP"), &MapRenderState::g_hotkeys.enlargeMinimap,    defaults.enlargeMinimap, true);
            renderRow(LanguageManager::GetText("HOTKEY_CENTER_CAMERA"),   &MapRenderState::g_hotkeys.centerCamera,       defaults.centerCamera, true);

            ImGui::Columns(1);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === 底部按钮栏：撤销 | 全部重置 ===
            float halfW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
            // 撤销按钮 (无可撤销时禁用)
            if (!s_undo.valid) ImGui::BeginDisabled();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.35f, 0.6f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.45f, 0.75f, 1.0f));
            if (ImGui::Button(LanguageManager::GetText("HOTKEY_UNDO"), ImVec2(halfW, 0))) {
                performUndo();
            }
            ImGui::PopStyleColor(2);
            if (!s_undo.valid) ImGui::EndDisabled();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("HOTKEY_STATUS_UNDONE"));
            ImGui::SameLine();

            // 全部重置 (推入批量撤销，支持 Ctrl+Z 恢复; 包含 openBigMap)
            if (ImGui::Button(LanguageManager::GetText("HOTKEY_RESET_ALL"), ImVec2(halfW, 0))) {
                bool anyChanged = (MapRenderState::g_hotkeys.openBigMap        != defaults.openBigMap ||
                                   MapRenderState::g_hotkeys.openWaypointMgr   != defaults.openWaypointMgr ||
                                   MapRenderState::g_hotkeys.openDeathPointMgr != defaults.openDeathPointMgr ||
                                   MapRenderState::g_hotkeys.toggleMinimap     != defaults.toggleMinimap ||
                                   MapRenderState::g_hotkeys.toggleMinimapShape!= defaults.toggleMinimapShape ||
                                   MapRenderState::g_hotkeys.toggleMinimapRot  != defaults.toggleMinimapRot ||
                                   MapRenderState::g_hotkeys.holdEntities      != defaults.holdEntities ||
                                   MapRenderState::g_hotkeys.toggleSeedMap     != defaults.toggleSeedMap ||
                                   MapRenderState::g_hotkeys.enlargeMinimap    != defaults.enlargeMinimap ||
                                   MapRenderState::g_hotkeys.centerCamera      != defaults.centerCamera);
                if (anyChanged) {
                    std::vector<UndoChange> changes = {
                        {&MapRenderState::g_hotkeys.openBigMap,        MapRenderState::g_hotkeys.openBigMap},
                        {&MapRenderState::g_hotkeys.openWaypointMgr,   MapRenderState::g_hotkeys.openWaypointMgr},
                        {&MapRenderState::g_hotkeys.openDeathPointMgr, MapRenderState::g_hotkeys.openDeathPointMgr},
                        {&MapRenderState::g_hotkeys.toggleMinimap,     MapRenderState::g_hotkeys.toggleMinimap},
                        {&MapRenderState::g_hotkeys.toggleMinimapShape,MapRenderState::g_hotkeys.toggleMinimapShape},
                        {&MapRenderState::g_hotkeys.toggleMinimapRot,  MapRenderState::g_hotkeys.toggleMinimapRot},
                        {&MapRenderState::g_hotkeys.holdEntities,      MapRenderState::g_hotkeys.holdEntities},
                        {&MapRenderState::g_hotkeys.toggleSeedMap,     MapRenderState::g_hotkeys.toggleSeedMap},
                        {&MapRenderState::g_hotkeys.enlargeMinimap,    MapRenderState::g_hotkeys.enlargeMinimap},
                        {&MapRenderState::g_hotkeys.centerCamera,      MapRenderState::g_hotkeys.centerCamera}
                    };
                    MapRenderState::g_hotkeys = defaults;
                    pushUndo(LanguageManager::GetText("HOTKEY_RESET_ALL"), std::move(changes));
                    // 全部绿色闪烁
                    addFlash(&MapRenderState::g_hotkeys.openBigMap,        ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.openWaypointMgr,   ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.openDeathPointMgr, ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.toggleMinimap,     ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.toggleMinimapShape,ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.toggleMinimapRot,  ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.holdEntities,      ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.toggleSeedMap,     ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.enlargeMinimap,    ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    addFlash(&MapRenderState::g_hotkeys.centerCamera,      ImVec4(0.2f, 0.7f, 0.3f, 1.0f));
                    setStatus(LanguageManager::GetText("HOTKEY_STATUS_RESET"), ImVec4(0.4f, 0.9f, 0.5f, 1.0f));
                }
                MapRenderState::g_listeningHotkey = nullptr;
                LanguageManager::SaveConfig();
            }

            // === 底部状态消息栏 (带淡出效果) ===
            if (s_status.timeLeft > 0.0f) {
                ImGui::Spacing();
                float alpha = (s_status.timeLeft > 0.5f) ? 1.0f : (s_status.timeLeft / 0.5f); // 最后 0.5s 淡出
                ImVec4 col = s_status.color;
                col.w *= alpha;
                ImGui::TextColored(col, "%s", s_status.text.c_str());
            }
        }
        ImGui::End();
    }

    // [UI辅助] 安全截断 UTF-8 文本并添加省略号，保证不超过指定像素宽度，不破坏多字节编码
    inline std::string TruncateTextToWidth(const std::string& text, float maxWidth, bool& wasTruncated) {
        wasTruncated = false;
        if (text.empty() || maxWidth <= 0.0f) return "";

        if (ImGui::CalcTextSize(text.c_str()).x <= maxWidth) {
            return text;
        }

        const char* ellipsis = "...";
        float ellipsisWidth = ImGui::CalcTextSize(ellipsis).x;
        float targetWidth = maxWidth - ellipsisWidth;
        if (targetWidth <= 0.0f) {
            wasTruncated = true;
            return ellipsis;
        }

        size_t lastValidBytePos = 0;
        size_t i = 0;
        while (i < text.size()) {
            size_t next = i + 1;
            while (next < text.size() && (static_cast<unsigned char>(text[next]) & 0xC0) == 0x80) {
                next++;
            }
            std::string sub = text.substr(0, next);
            if (ImGui::CalcTextSize(sub.c_str()).x > targetWidth) {
                break;
            }
            lastValidBytePos = next;
            i = next;
        }

        wasTruncated = true;
        if (lastValidBytePos == 0) return ellipsis;
        return text.substr(0, lastValidBytePos) + ellipsis;
    }

    // ==========================================
    // 路径点 ImGui 管理控制台 (添加搜索、排序、置顶、文件夹、手动排序、重命名与传送)
    // ==========================================
    inline void RenderImGuiWaypointUI() {
        float curScale = MapRenderState::globalUIScale;
        if (curScale < 0.1f) curScale = 1.0f;

        ImGui::SetNextWindowSize(ImVec2(840.0f * curScale, 520.0f * curScale), ImGuiCond_FirstUseEver); 
        ImGui::SetNextWindowSizeConstraints(ImVec2(720.0f * curScale, 360.0f * curScale), ImVec2(1920.0f * curScale, 1080.0f * curScale));
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x / 2 - 420.0f * curScale, ImGui::GetIO().DisplaySize.y / 2 - 260.0f * curScale), ImGuiCond_FirstUseEver);
        
        // 记录管理器面板当前帧的开启状态
        bool lastShowWPUI = MapRenderState::showWaypointUI;
        
        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin(LanguageManager::GetText("WP_MANAGER_TITLE"), &MapRenderState::showWaypointUI, winFlags)) {
            
            static bool showAddPopup = false;
            static bool showNewFolderPopup = false;
            static bool showRenameFolderPopup = false;
            static bool showDeleteFolderPopup = false;
            // 记录新建窗口当前帧的开启状态
            bool lastShowAddPopup = showAddPopup;
            static char searchBuf[256] = "";
            static int wpTab = -1; // 维度标签: -1=全部, 0=主世界, 1=下界, 2=末地
            
            // 顶栏第一排：搜索栏与新建按钮
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 190.0f * curScale);
            ImGui::InputTextWithHint("##WPSearch", LanguageManager::GetText("SEARCH_HINT"), searchBuf, sizeof(searchBuf));
            ImGui::PopItemWidth();
            
            ImGui::SameLine();
            if (ImGui::Button("\xe2\x9c\x8e##Search")) {
                NativeIME::Open(searchBuf, sizeof(searchBuf), LanguageManager::GetText("SEARCH_HINT"));
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("NATIVE_IME_TOOLTIP"));
            
            ImGui::SameLine();
            if (ImGui::Button(LanguageManager::GetText("NEW_WP_BUTTON"), ImVec2(140.0f * curScale, 0))) {
                NativeIME::Close();
                showAddPopup = true;
            }

            // 顶栏第二排：维度标签筛选与排序选择
            {
                if (!lastShowWPUI) {
                    if (MapRenderState::addWaypointDim >= 0 && MapRenderState::addWaypointDim < 3) {
                        wpTab = MapRenderState::addWaypointDim;
                        MapRenderState::addWaypointDim = -1;
                    } else {
                        wpTab = MapRenderState::currentDimensionId; // 打开管理器时默认选中当前维度
                    }
                }
                const char* tabLabels[4] = {
                    LanguageManager::GetText("WP_TAB_ALL"),
                    LanguageManager::GetText("WP_TAB_OVERWORLD"),
                    LanguageManager::GetText("WP_TAB_NETHER"),
                    LanguageManager::GetText("WP_TAB_END")
                };
                int tabValues[4] = { -1, 0, 1, 2 };
                for (int i = 0; i < 4; ++i) {
                    if (i > 0) ImGui::SameLine();
                    bool active = (wpTab == tabValues[i]);
                    ImGui::PushStyleColor(ImGuiCol_Button, active ? ImVec4(0.25f, 0.55f, 0.9f, 1.0f) : ImVec4(0.25f, 0.32f, 0.38f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.52f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.28f, 0.34f, 1.0f));
                    if (ImGui::Button(tabLabels[i], ImVec2(105.0f * curScale, 0))) wpTab = tabValues[i];
                    ImGui::PopStyleColor(3);
                }

                // 排序下拉选框 (包含 7 种模式：时间最近/最远、名称升降序、距离升降序、手动排序)
                ImGui::SameLine();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                ImGui::Text("%s:", LanguageManager::GetText("WP_SORT"));
                ImGui::SameLine();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
                const char* sortLabels[7] = {
                    LanguageManager::GetText("WP_SORT_TIME_DESC"),
                    LanguageManager::GetText("WP_SORT_TIME_ASC"),
                    LanguageManager::GetText("WP_SORT_NAME_ASC"),
                    LanguageManager::GetText("WP_SORT_NAME_DESC"),
                    LanguageManager::GetText("WP_SORT_DIST_ASC"),
                    LanguageManager::GetText("WP_SORT_DIST_DESC"),
                    LanguageManager::GetText("WP_SORT_MANUAL")
                };
                int curSort = MapRenderState::waypointSortMode;
                if (curSort < 0 || curSort >= 7) curSort = 0;
                ImGui::SetNextItemWidth(140.0f * curScale);
                if (ImGui::BeginCombo("##WPSortCombo", sortLabels[curSort])) {
                    for (int s = 0; s < 7; ++s) {
                        bool isSel = (curSort == s);
                        if (ImGui::Selectable(sortLabels[s], isSel)) {
                            MapRenderState::waypointSortMode = s;
                        }
                    }
                    ImGui::EndCombo();
                }
            }

            // 顶栏第三排：文件夹筛选栏与管理按钮
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);
                ImGui::Text("%s:", LanguageManager::GetText("WP_FOLDER"));
                ImGui::SameLine();
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);

                std::string currentFolderLabel = LanguageManager::GetText("WP_FOLDER_ALL");
                if (MapRenderState::waypointFolderFilter == "__ROOT__") currentFolderLabel = LanguageManager::GetText("WP_FOLDER_NONE");
                else if (!MapRenderState::waypointFolderFilter.empty()) currentFolderLabel = MapRenderState::waypointFolderFilter;

                ImGui::SetNextItemWidth(160.0f * curScale);
                if (ImGui::BeginCombo("##WPFolderCombo", currentFolderLabel.c_str())) {
                    if (ImGui::Selectable(LanguageManager::GetText("WP_FOLDER_ALL"), MapRenderState::waypointFolderFilter.empty())) {
                        MapRenderState::waypointFolderFilter = "";
                    }
                    if (ImGui::Selectable(LanguageManager::GetText("WP_FOLDER_NONE"), MapRenderState::waypointFolderFilter == "__ROOT__")) {
                        MapRenderState::waypointFolderFilter = "__ROOT__";
                    }
                    for (const auto& f : WaypointManager::GetFolders()) {
                        bool isSel = (MapRenderState::waypointFolderFilter == f);
                        if (ImGui::Selectable(f.c_str(), isSel)) {
                            MapRenderState::waypointFolderFilter = f;
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.28f, 0.48f, 0.35f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.38f, 0.58f, 0.45f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.40f, 0.28f, 1.0f));
                if (ImGui::Button(LanguageManager::GetText("WP_FOLDER_NEW"), ImVec2(80.0f * curScale, 0))) {
                    showNewFolderPopup = true;
                }
                ImGui::PopStyleColor(3);

                // 若当前选中了具体文件夹，支持重命名与删除该文件夹
                if (!MapRenderState::waypointFolderFilter.empty() && MapRenderState::waypointFolderFilter != "__ROOT__") {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.42f, 0.52f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.52f, 0.62f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.35f, 0.45f, 1.0f));
                    if (ImGui::Button(LanguageManager::GetText("WP_FOLDER_RENAME"))) {
                        showRenameFolderPopup = true;
                    }
                    ImGui::PopStyleColor(3);

                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.25f, 0.25f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.75f, 0.35f, 0.35f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.18f, 0.18f, 1.0f));
                    if (ImGui::Button(LanguageManager::GetText("WP_FOLDER_DELETE"))) {
                        showDeleteFolderPopup = true;
                    }
                    ImGui::PopStyleColor(3);
                }
            }

            ImGui::Separator();

            ImGui::BeginChild("WPList", ImVec2(0, 0), true);
            std::string toDelete = "";
            std::string toTogglePin = "";
            std::string toToggleEnabled = "";
            std::string toSwapId1 = "";
            std::string toSwapId2 = "";
            std::string toMoveSrc = "";
            std::string toMoveTgt = "";
            bool triggerTp = false;

            static std::string uiEditId = "";
            static bool uiTriggerEdit = false;

            // [新增] 路径点多选状态
            static std::set<std::string> selectedIds;
            bool bulkDeleteTriggered = false;

            // [新增] “在大地图上定位”触发状态（携带目标坐标与维度，循环结束后统一处理）
            bool triggerLocate = false;
            int locateX = 0, locateY = 0, locateZ = 0, locateDim = 0;

            std::string query = searchBuf;
            for (char& c : query) { if (c >= 'A' && c <= 'Z') c += 32; }

            // 筛选并提取要展示的路径点
            std::vector<Waypoint> displayList;
            {
                std::lock_guard<std::mutex> lock(WaypointManager::g_wpMutex);
                for (const auto& wp : WaypointManager::g_waypoints) {
                    if (wpTab != -1 && wp.dimId != wpTab) continue; // 维度标签筛选
                    if (!MapRenderState::waypointFolderFilter.empty()) {
                        if (MapRenderState::waypointFolderFilter == "__ROOT__") {
                            if (!wp.folder.empty()) continue; // 仅未分类
                        } else {
                            if (wp.folder != MapRenderState::waypointFolderFilter) continue; // 仅指定文件夹
                        }
                    }
                    if (!query.empty()) {
                        std::string lowerName = wp.name;
                        for (char& c : lowerName) { if (c >= 'A' && c <= 'Z') c += 32; }
                        if (lowerName.find(query) == std::string::npos) continue;
                    }
                    displayList.push_back(wp);
                }
            }

            // 排序计算：置顶绝对优先于所有其他规则
            std::sort(displayList.begin(), displayList.end(), [&](const Waypoint& a, const Waypoint& b) {
                if (a.pinned != b.pinned) return a.pinned > b.pinned; // 置顶项优先排前
                switch (MapRenderState::waypointSortMode) {
                    case 0: { // Time Desc: 时间 (最近)
                        uint64_t ca = a.createdAt ? a.createdAt : (uint64_t)a.order;
                        uint64_t cb = b.createdAt ? b.createdAt : (uint64_t)b.order;
                        if (ca != cb) return ca > cb;
                        return a.order < b.order;
                    }
                    case 1: { // Time Asc: 时间 (最远)
                        uint64_t ca = a.createdAt ? a.createdAt : (uint64_t)a.order;
                        uint64_t cb = b.createdAt ? b.createdAt : (uint64_t)b.order;
                        if (ca != cb) return ca < cb;
                        return a.order > b.order;
                    }
                    case 2: { // Name Asc: 名称 (A-Z)
                        std::string sa = a.name, sb = b.name;
                        for (char& c : sa) { if (c >= 'A' && c <= 'Z') c += 32; }
                        for (char& c : sb) { if (c >= 'A' && c <= 'Z') c += 32; }
                        return sa < sb;
                    }
                    case 3: { // Name Desc: 名称 (Z-A)
                        std::string sa = a.name, sb = b.name;
                        for (char& c : sa) { if (c >= 'A' && c <= 'Z') c += 32; }
                        for (char& c : sb) { if (c >= 'A' && c <= 'Z') c += 32; }
                        return sa > sb;
                    }
                    case 4:   // Dist Asc: 距离 (最近)
                    case 5: { // Dist Desc: 距离 (最远)
                        bool aSame = (a.dimId == MapRenderState::currentDimensionId);
                        bool bSame = (b.dimId == MapRenderState::currentDimensionId);
                        if (aSame != bSame) return aSame > bSame;
                        double da = (double)(a.x - g_playerBlockX) * (a.x - g_playerBlockX) + (double)(a.z - g_playerBlockZ) * (a.z - g_playerBlockZ);
                        double db = (double)(b.x - g_playerBlockX) * (b.x - g_playerBlockX) + (double)(b.z - g_playerBlockZ) * (b.z - g_playerBlockZ);
                        return (MapRenderState::waypointSortMode == 4) ? (da < db) : (da > db);
                    }
                    case 6: // Manual: 手动排序
                        return a.order < b.order;
                    default:
                        return a.createdAt > b.createdAt;
                }
            });

            // 当前筛选结果是否已全部选中（用于全选/取消全选切换按钮）
            bool allDisplaySelected = !displayList.empty();
            if (allDisplaySelected) {
                for (const auto& wp : displayList) {
                    if (selectedIds.find(wp.id) == selectedIds.end()) {
                        allDisplaySelected = false;
                        break;
                    }
                }
            }

            // 多选工具栏（固定在列表上方，不随滚动条消失）
            {
                if (displayList.empty()) ImGui::BeginDisabled();
                // 未全选时显示蓝色“全选”，全部选中后切换为灰色“取消全选”
                if (allDisplaySelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.40f, 0.40f, 0.40f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.32f, 0.38f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.45f, 0.52f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.28f, 0.34f, 1.0f));
                }
                const char* toggleSelectLabel = allDisplaySelected
                    ? LanguageManager::GetText("WP_DESELECT_ALL")
                    : LanguageManager::GetText("WP_SELECT_ALL");
                if (ImGui::Button(toggleSelectLabel, ImVec2(90.0f * curScale, 0))) {
                    if (allDisplaySelected) {
                        selectedIds.clear();
                    } else {
                        for (const auto& wp : displayList) {
                            selectedIds.insert(wp.id);
                        }
                    }
                }
                ImGui::PopStyleColor(3);
                if (displayList.empty()) ImGui::EndDisabled();

                if (!selectedIds.empty()) {
                    ImGui::SameLine();
                    char delBuf[128];
                    snprintf(delBuf, sizeof(delBuf), LanguageManager::GetText("WP_DELETE_SELECTED"), (int)selectedIds.size());
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.18f, 0.18f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.28f, 0.28f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.12f, 0.12f, 1.0f));
                    if (ImGui::Button(delBuf)) {
                        bulkDeleteTriggered = true;
                    }
                    ImGui::PopStyleColor(3);
                }
            }
            ImGui::Separator();

            // 可滚动列表区：工具栏固定在其上方，仅列表内容随滚动条滚动
            ImGui::BeginChild("WPListScroll", ImVec2(0, 0), false);

            // 渲染排序后的路径点列表
            for (size_t idx = 0; idx < displayList.size(); ++idx) {
                const auto& wp = displayList[idx];
                ImGui::PushID(wp.id.c_str());
                float rowBaseY = ImGui::GetCursorPosY();

                // 多选复选框
                bool isSelected = selectedIds.count(wp.id) > 0;
                if (ImGui::Checkbox("##msel", &isSelected)) {
                    if (isSelected) selectedIds.insert(wp.id);
                    else selectedIds.erase(wp.id);
                }
                ImGui::SameLine();

                // 上移/下移手动排序按钮 (▲ / ▼)
                {
                    bool canUp = (idx > 0 && displayList[idx - 1].pinned == wp.pinned);
                    if (!canUp) ImGui::BeginDisabled();
                    if (ImGui::Button("\xe2\x96\xb2##Up", ImVec2(20.0f * curScale, 24.0f * curScale))) { // ▲ U+25B2
                        toSwapId1 = wp.id;
                        toSwapId2 = displayList[idx - 1].id;
                        MapRenderState::waypointSortMode = 6; // 自动切入手动排序模式
                    }
                    if (!canUp) ImGui::EndDisabled();
                    else if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("WP_MOVE_UP"));
                    ImGui::SameLine();

                    bool canDown = (idx + 1 < displayList.size() && displayList[idx + 1].pinned == wp.pinned);
                    if (!canDown) ImGui::BeginDisabled();
                    if (ImGui::Button("\xe2\x96\xbc##Down", ImVec2(20.0f * curScale, 24.0f * curScale))) { // ▼ U+25BC
                        toSwapId1 = wp.id;
                        toSwapId2 = displayList[idx + 1].id;
                        MapRenderState::waypointSortMode = 6; // 自动切入手动排序模式
                    }
                    if (!canDown) ImGui::EndDisabled();
                    else if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("WP_MOVE_DOWN"));
                    ImGui::SameLine();
                }

                // 置顶切换按钮 (★ / ☆)
                if (wp.pinned) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.65f, 0.15f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.75f, 0.25f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.75f, 0.55f, 0.10f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    if (ImGui::Button("\xe2\x98\x85##Pin", ImVec2(24.0f * curScale, 24.0f * curScale))) { // ★ U+2605
                        toTogglePin = wp.id;
                    }
                    ImGui::PopStyleColor(4);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("WP_UNPIN"));
                } else {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.24f, 0.28f, 0.32f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.34f, 0.38f, 0.42f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.22f, 0.26f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                    if (ImGui::Button("\xe2\x98\x86##Pin", ImVec2(24.0f * curScale, 24.0f * curScale))) { // ☆ U+2606
                        toTogglePin = wp.id;
                    }
                    ImGui::PopStyleColor(4);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("WP_PIN"));
                }
                ImGui::SameLine();

                // 颜色预览块 (支持拖拽放置目标与拖拽源)
                ImGui::ColorButton("##color", ImVec4(wp.r, wp.g, wp.b, 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(24.0f * curScale, 24.0f * curScale));
                
                // 拖拽重排交互 (Drag & Drop)
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload("WP_DND_ROW", wp.id.c_str(), wp.id.size() + 1);
                    ImGui::Text("%s: %s", LanguageManager::GetText("WP_SORT_MANUAL"), wp.name.c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WP_DND_ROW")) {
                        const char* srcId = (const char*)payload->Data;
                        if (srcId && wp.id != srcId) {
                            toMoveSrc = srcId;
                            toMoveTgt = wp.id;
                            MapRenderState::waypointSortMode = 6; // 自动切入手动排序模式
                        }
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::SameLine();
                
                float winWidth = ImGui::GetWindowWidth();
                float buttonsStartX = winWidth - 260.0f * curScale;

                // 准备坐标与距离文本
                char coordBuf[128];
                if (wp.dimId == MapRenderState::currentDimensionId) {
                    double dDist = std::sqrt((double)(wp.x - g_playerBlockX) * (wp.x - g_playerBlockX) + (double)(wp.z - g_playerBlockZ) * (wp.z - g_playerBlockZ));
                    if (dDist >= 1000.0) {
                        snprintf(coordBuf, sizeof(coordBuf), "X:%d Y:%d Z:%d (%.1fkm)", wp.x, wp.y, wp.z, dDist / 1000.0);
                    } else {
                        snprintf(coordBuf, sizeof(coordBuf), "X:%d Y:%d Z:%d (%.0fm)", wp.x, wp.y, wp.z, dDist);
                    }
                } else {
                    snprintf(coordBuf, sizeof(coordBuf), "X:%d Y:%d Z:%d", wp.x, wp.y, wp.z);
                }
                float coordTextWidth = ImGui::CalcTextSize(coordBuf).x;
                float coordStartX = buttonsStartX - coordTextWidth - 14.0f * curScale;

                // 准备文件夹徽章与维度标签
                char folderBuf[128] = "";
                float folderWidth = 0.0f;
                bool isFolderTruncated = false;
                if (!wp.folder.empty() && MapRenderState::waypointFolderFilter.empty()) {
                    std::string dispFolder = TruncateTextToWidth(wp.folder, 80.0f * curScale, isFolderTruncated);
                    snprintf(folderBuf, sizeof(folderBuf), "[%s]", dispFolder.c_str());
                    folderWidth = ImGui::CalcTextSize(folderBuf).x + 8.0f * curScale;
                }

                const char* dimLabel = LanguageManager::GetText(
                    wp.dimId == 1 ? "WP_TAB_NETHER" :
                    wp.dimId == 2 ? "WP_TAB_END" : "WP_TAB_OVERWORLD");
                char dimBuf[64];
                snprintf(dimBuf, sizeof(dimBuf), "[%s]", dimLabel);
                float dimWidth = ImGui::CalcTextSize(dimBuf).x + 8.0f * curScale;

                char tempBuf[64] = "";
                float tempWidth = 0.0f;
                if (wp.isTemporary) {
                    snprintf(tempBuf, sizeof(tempBuf), "[%s]", LanguageManager::GetText("TEMP_WAYPOINT_NAME"));
                    tempWidth = ImGui::CalcTextSize(tempBuf).x + 8.0f * curScale;
                }

                // 动态计算名称区域可用最大像素宽度
                float nameStartX = ImGui::GetCursorPosX();
                float totalAvailForName = (coordStartX - 10.0f * curScale) - nameStartX - folderWidth - dimWidth - tempWidth;
                if (totalAvailForName < 50.0f * curScale) totalAvailForName = 50.0f * curScale;

                bool isNameTruncated = false;
                std::string displayName = TruncateTextToWidth(wp.name, totalAvailForName, isNameTruncated);

                // 名称展示（置顶项目带醒目微金色高亮，超长截断并悬停显示完整名称）
                ImGui::SetCursorPosY(rowBaseY + 4.0f * curScale);
                if (wp.pinned) {
                    ImGui::TextColored(ImVec4(1.0f, 0.88f, 0.35f, 1.0f), "%s", displayName.c_str());
                } else if (wp.isTemporary) {
                    ImGui::TextColored(ImVec4(0.2f, 0.95f, 1.0f, 1.0f), "%s", displayName.c_str());
                } else {
                    ImGui::Text("%s", displayName.c_str());
                }
                if (isNameTruncated && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", wp.name.c_str());
                }

                // 临时路径点归属徽章
                if (tempWidth > 0.0f) {
                    ImGui::SameLine();
                    ImGui::SetCursorPosY(rowBaseY + 4.0f * curScale);
                    ImGui::TextColored(ImVec4(0.0f, 0.90f, 1.0f, 1.0f), "%s", tempBuf);
                }

                // 文件夹归属徽章（在全文件夹视图或未分类视图时明确标注归属）
                if (folderWidth > 0.0f) {
                    ImGui::SameLine();
                    ImGui::SetCursorPosY(rowBaseY + 4.0f * curScale);
                    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "%s", folderBuf);
                    if (isFolderTruncated && ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s: %s", LanguageManager::GetText("WP_FOLDER"), wp.folder.c_str());
                    }
                }

                // 维度标注：显示主世界 / 下界 / 末地
                {
                    ImVec4 dimCol = wp.dimId == 1 ? ImVec4(0.90f, 0.45f, 0.40f, 1.0f) :
                                    wp.dimId == 2 ? ImVec4(0.70f, 0.55f, 0.90f, 1.0f) :
                                                    ImVec4(0.45f, 0.80f, 0.45f, 1.0f);
                    ImGui::SameLine();
                    ImGui::SetCursorPosY(rowBaseY + 4.0f * curScale);
                    ImGui::TextColored(dimCol, "%s", dimBuf);
                }

                // 坐标与距离展示（右对齐排在操作按钮左侧，单调递增绝不后退覆盖）
                if (coordStartX > ImGui::GetCursorPosX()) {
                    ImGui::SameLine(coordStartX);
                } else {
                    ImGui::SameLine(0.0f, 8.0f * curScale);
                }
                ImGui::SetCursorPosY(rowBaseY + 4.0f * curScale);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", coordBuf);

                // “显示”开关：眼睛图标按钮（开启=蓝底睁眼，关闭=灰底闭眼+红斜杠），
                // 悬停显示详细说明；右侧操作按钮区统一对齐行基线
                ImGui::SameLine(buttonsStartX);
                ImGui::SetCursorPosY(rowBaseY);
                {
                    const bool visOn = wp.enabled;
                    if (visOn) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.72f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.58f, 0.86f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.38f, 0.62f, 1.0f));
                    } else {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.23f, 0.26f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.33f, 0.33f, 0.37f, 1.0f));
                        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.18f, 0.18f, 0.21f, 1.0f));
                    }
                    if (ImGui::Button("##WPVisToggle", ImVec2(30.0f * curScale, 0))) {
                        toToggleEnabled = wp.id;
                    }
                    // 在按钮上绘制眼睛图标（不依赖字体字形，用 DrawList 直接绘制）
                    {
                        ImDrawList* btnDraw = ImGui::GetWindowDrawList();
                        ImVec2 eyeCtr;
                        eyeCtr.x = (ImGui::GetItemRectMin().x + ImGui::GetItemRectMax().x) * 0.5f;
                        eyeCtr.y = (ImGui::GetItemRectMin().y + ImGui::GetItemRectMax().y) * 0.5f;
                        ImU32 eyeCol = visOn ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 150, 255);
                        btnDraw->AddEllipse(eyeCtr, ImVec2(10.0f * curScale, 6.4f * curScale), eyeCol, 0.0f, 20, 1.6f * curScale);
                        btnDraw->AddCircleFilled(eyeCtr, 3.2f * curScale, eyeCol, 20);
                        if (!visOn) {
                            // 隐藏状态：红色斜杠划过眼睛
                            btnDraw->AddLine(ImVec2(eyeCtr.x - 8.5f * curScale, eyeCtr.y + 8.5f * curScale),
                                             ImVec2(eyeCtr.x + 8.5f * curScale, eyeCtr.y - 8.5f * curScale),
                                             IM_COL32(235, 95, 95, 255), 2.0f * curScale);
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("%s", LanguageManager::GetText("WP_VIS_TOGGLE_TIP"));
                    }
                    ImGui::PopStyleColor(3);
                }

                ImGui::SameLine(winWidth - 220.0f * curScale);
                // [新增] 在全屏大地图上定位到该路径点（⚑ 小按钮，悬停显示本地化提示）
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.35f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.60f, 0.45f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.38f, 0.28f, 1.0f));
                if (ImGui::Button("\u2691##WPLocate", ImVec2(30.0f * curScale, 0))) {
                    triggerLocate = true;
                    locateX = wp.x;
                    locateZ = wp.z;
                    locateDim = wp.dimId;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", LanguageManager::GetText("WP_LOCATE_ON_MAP"));
                }
                ImGui::PopStyleColor(3);

                ImGui::SameLine(winWidth - 185.0f * curScale);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.6f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.7f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.5f, 0.1f, 1.0f));
                if (ImGui::Button(LanguageManager::GetText("EDIT_WP"), ImVec2(55.0f * curScale, 0))) {
                    uiEditId = wp.id;
                    uiTriggerEdit = true;
                }
                ImGui::PopStyleColor(3);

                ImGui::SameLine(winWidth - 125.0f * curScale);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.8f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.1f, 0.5f, 0.7f, 1.0f));
                if (ImGui::Button(LanguageManager::GetText("WP_LIST_TELEPORT"), ImVec2(45.0f * curScale, 0))) {
                    MapRenderState::tpTargetX = (float)wp.x + 0.5f;
                    MapRenderState::tpTargetY = (float)wp.y; 
                    MapRenderState::tpTargetZ = (float)wp.z + 0.5f;
                    MapRenderState::tpTargetDim = wp.dimId;
                    MapRenderState::triggerTeleport.store(true);
                    triggerTp = true;
                }
                ImGui::PopStyleColor(3);

                ImGui::SameLine(winWidth - 75.0f * curScale);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
                if (ImGui::Button(LanguageManager::GetText("WP_LIST_DELETE"), ImVec2(45.0f * curScale, 0))) {
                    toDelete = wp.id;
                }
                ImGui::PopStyleColor(3);
                
                ImGui::PopID();
                ImGui::Separator();
            }
            ImGui::EndChild(); // WPListScroll：仅列表区滚动，工具栏保持固定

            // 交互处理：批量删除、单个删除、置顶切换、显示状态切换、手动排序交换与拖拽
            if (bulkDeleteTriggered && !selectedIds.empty()) {
                WaypointManager::RemoveWaypoints(selectedIds);
                selectedIds.clear();
            } else if (!toDelete.empty()) {
                WaypointManager::RemoveWaypoint(toDelete);
                selectedIds.erase(toDelete);  // 清理已删除路径点的选中状态
                WaypointManager::SaveWaypoints();
            }
            if (!toTogglePin.empty()) {
                WaypointManager::ToggleWaypointPin(toTogglePin);
            }
            if (!toToggleEnabled.empty()) {
                WaypointManager::ToggleWaypoint(toToggleEnabled);
                WaypointManager::SaveWaypoints();
            }
            if (!toSwapId1.empty() && !toSwapId2.empty()) {
                WaypointManager::SwapWaypointOrder(toSwapId1, toSwapId2);
            }
            if (!toMoveSrc.empty() && !toMoveTgt.empty()) {
                WaypointManager::MoveWaypointToIndex(toMoveSrc, toMoveTgt);
            }
            
            if (triggerTp) {
                MapRenderState::showWaypointUI = false;
                MapRenderState::showBigMap = false;
            }

            // [新增] 关闭管理器并打开全屏大地图，切到路径点所在维度并将视野居中到该点。
            // 偏移公式与种子地图 CenterSeedMapOnMarker 一致：offset = -(目标世界坐标 - 玩家平滑坐标) * zoom
            if (triggerLocate) {
                LocateBigMapOn(locateDim, (float)locateX + 0.5f, (float)locateZ + 0.5f, g_smoothPX, g_smoothPZ);
                MapRenderState::showWaypointUI = false;
            }
            
            ImGui::EndChild();

            // 调用 UI 列表专属重命名弹窗模块
            RenderEditModal((std::string(LanguageManager::GetText("EDIT_WP_TITLE")) + "##ModalUIEdit").c_str(), uiEditId, uiTriggerEdit);

            // 新建文件夹弹窗
            if (showNewFolderPopup) ImGui::OpenPopup(LanguageManager::GetText("WP_FOLDER_NEW"));
            if (ImGui::BeginPopupModal(LanguageManager::GetText("WP_FOLDER_NEW"), &showNewFolderPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                static char fNameBuf[128] = "";
                if (ImGui::IsWindowAppearing()) { fNameBuf[0] = '\0'; }
                ImGui::Text("%s:", LanguageManager::GetText("WP_FOLDER_NAME"));
                ImGui::PushItemWidth(180.0f * curScale);
                ImGui::InputText("##NewFolderName", fNameBuf, sizeof(fNameBuf));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("\xe2\x9c\x8e##NewFolderIME")) {
                    NativeIME::Open(fNameBuf, sizeof(fNameBuf), LanguageManager::GetText("WP_FOLDER_NAME"));
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("NATIVE_IME_TOOLTIP"));
                ImGui::Spacing();
                if (ImGui::Button(LanguageManager::GetText("WP_SAVE"), ImVec2(100.0f * curScale, 0))) {
                    if (fNameBuf[0] != '\0') {
                        WaypointManager::AddFolder(fNameBuf);
                        MapRenderState::waypointFolderFilter = fNameBuf;
                    }
                    showNewFolderPopup = false;
                    ImGui::CloseCurrentPopup();
                    NativeIME::Close();
                }
                ImGui::SameLine();
                if (ImGui::Button(LanguageManager::GetText("WP_CANCEL"), ImVec2(100.0f * curScale, 0))) {
                    showNewFolderPopup = false;
                    ImGui::CloseCurrentPopup();
                    NativeIME::Close();
                }
                ImGui::EndPopup();
            }

            // 重命名文件夹弹窗
            if (showRenameFolderPopup) ImGui::OpenPopup(LanguageManager::GetText("WP_FOLDER_RENAME"));
            if (ImGui::BeginPopupModal(LanguageManager::GetText("WP_FOLDER_RENAME"), &showRenameFolderPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                static char rNameBuf[128] = "";
                if (ImGui::IsWindowAppearing()) {
                    snprintf(rNameBuf, sizeof(rNameBuf), "%s", MapRenderState::waypointFolderFilter.c_str());
                }
                ImGui::Text("%s:", LanguageManager::GetText("WP_FOLDER_NAME"));
                ImGui::PushItemWidth(180.0f * curScale);
                ImGui::InputText("##RenameFolderName", rNameBuf, sizeof(rNameBuf));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("\xe2\x9c\x8e##RenameFolderIME")) {
                    NativeIME::Open(rNameBuf, sizeof(rNameBuf), LanguageManager::GetText("WP_FOLDER_NAME"));
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("NATIVE_IME_TOOLTIP"));
                ImGui::Spacing();
                if (ImGui::Button(LanguageManager::GetText("WP_SAVE"), ImVec2(100.0f * curScale, 0))) {
                    if (rNameBuf[0] != '\0' && MapRenderState::waypointFolderFilter != rNameBuf) {
                        WaypointManager::RenameFolder(MapRenderState::waypointFolderFilter, rNameBuf);
                        MapRenderState::waypointFolderFilter = rNameBuf;
                    }
                    showRenameFolderPopup = false;
                    ImGui::CloseCurrentPopup();
                    NativeIME::Close();
                }
                ImGui::SameLine();
                if (ImGui::Button(LanguageManager::GetText("WP_CANCEL"), ImVec2(100.0f * curScale, 0))) {
                    showRenameFolderPopup = false;
                    ImGui::CloseCurrentPopup();
                    NativeIME::Close();
                }
                ImGui::EndPopup();
            }

            // 删除文件夹弹窗
            if (showDeleteFolderPopup) ImGui::OpenPopup(LanguageManager::GetText("WP_FOLDER_DELETE"));
            if (ImGui::BeginPopupModal(LanguageManager::GetText("WP_FOLDER_DELETE"), &showDeleteFolderPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                char warnMsg[256];
                snprintf(warnMsg, sizeof(warnMsg), LanguageManager::GetText("WP_FOLDER_CONFIRM_DELETE"), MapRenderState::waypointFolderFilter.c_str());
                ImGui::TextWrapped("%s", warnMsg);
                ImGui::Spacing();
                if (ImGui::Button(LanguageManager::GetText("CONFIRM_DELETE"), ImVec2(120.0f * curScale, 0))) {
                    WaypointManager::DeleteFolder(MapRenderState::waypointFolderFilter);
                    MapRenderState::waypointFolderFilter = "";
                    showDeleteFolderPopup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(LanguageManager::GetText("WP_CANCEL"), ImVec2(120.0f * curScale, 0))) {
                    showDeleteFolderPopup = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (MapRenderState::triggerAddWaypoint) {
                showAddPopup = true;
                MapRenderState::triggerAddWaypoint = false;
            }

            if (showAddPopup) ImGui::OpenPopup(LanguageManager::GetText("NEW_WP_TITLE"));
            
            // 新建路径点弹窗（支持置顶与归属文件夹选择）
            if (ImGui::BeginPopupModal(LanguageManager::GetText("NEW_WP_TITLE"), &showAddPopup, ImGuiWindowFlags_AlwaysAutoResize)) {
                static char nameBuf[256] = "";
                static char folderBuf[128] = "";
                static int pos[3] = {0, 0, 0};
                static float col[3] = {1.0f, 0.3f, 0.3f};
                static int  rgb[3] = {255, 76, 76};
                static bool isPinned = false;
                
                if (ImGui::IsWindowAppearing()) {
                    nameBuf[0] = '\0'; 
                    isPinned = false;
                    if (!MapRenderState::waypointFolderFilter.empty() && MapRenderState::waypointFolderFilter != "__ROOT__") {
                        snprintf(folderBuf, sizeof(folderBuf), "%s", MapRenderState::waypointFolderFilter.c_str());
                    } else {
                        folderBuf[0] = '\0';
                    }
                    pos[0] = (MapRenderState::addWaypointX != -999999) ? MapRenderState::addWaypointX : g_playerBlockX;
                    pos[1] = (MapRenderState::addWaypointY != -999999) ? MapRenderState::addWaypointY : (int)std::floor(g_playerY);
                    pos[2] = (MapRenderState::addWaypointZ != -999999) ? MapRenderState::addWaypointZ : g_playerBlockZ;
                    
                    MapRenderState::addWaypointX = -999999;
                    MapRenderState::addWaypointY = -999999;
                    MapRenderState::addWaypointZ = -999999;

                    rgb[0] = (int)(col[0] * 255.0f);
                    rgb[1] = (int)(col[1] * 255.0f);
                    rgb[2] = (int)(col[2] * 255.0f);
                }

                ImGui::PushItemWidth(180.0f * curScale);
                ImGui::InputText("##NewWPInput", nameBuf, sizeof(nameBuf));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("\xe2\x9c\x8e##NewWP")) {
                    NativeIME::Open(nameBuf, sizeof(nameBuf), LanguageManager::GetText("WP_NAME"));
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("NATIVE_IME_TOOLTIP"));
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("WP_NAME"));
                
                ImGui::InputInt3("X / Y / Z", pos);
                if (ImGui::ColorEdit3(LanguageManager::GetText("WP_COLOR"), col)) {
                    rgb[0] = (int)(col[0] * 255.0f);
                    rgb[1] = (int)(col[1] * 255.0f);
                    rgb[2] = (int)(col[2] * 255.0f);
                }
                if (ImGui::InputInt3("R / G / B", rgb)) {
                    rgb[0] = rgb[0] < 0 ? 0 : (rgb[0] > 255 ? 255 : rgb[0]);
                    rgb[1] = rgb[1] < 0 ? 0 : (rgb[1] > 255 ? 255 : rgb[1]);
                    rgb[2] = rgb[2] < 0 ? 0 : (rgb[2] > 255 ? 255 : rgb[2]);
                    col[0] = (float)rgb[0] / 255.0f;
                    col[1] = (float)rgb[1] / 255.0f;
                    col[2] = (float)rgb[2] / 255.0f;
                }

                // 文件夹输入与已有文件夹快捷选择
                ImGui::PushItemWidth(180.0f * curScale);
                ImGui::InputText("##NewWPFolder", folderBuf, sizeof(folderBuf));
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("\xe2\x9c\x8e##NewWPFolderIME")) {
                    NativeIME::Open(folderBuf, sizeof(folderBuf), LanguageManager::GetText("WP_FOLDER"));
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", LanguageManager::GetText("NATIVE_IME_TOOLTIP"));
                ImGui::SameLine();
                ImGui::Text("%s", LanguageManager::GetText("WP_FOLDER"));

                auto existingFolders = WaypointManager::GetFolders();
                if (!existingFolders.empty()) {
                    std::string fPreview = folderBuf[0] ? folderBuf : LanguageManager::GetText("WP_FOLDER_NONE");
                    ImGui::PushItemWidth(180.0f * curScale);
                    if (ImGui::BeginCombo("##NewWPFolderCombo", fPreview.c_str())) {
                        if (ImGui::Selectable(LanguageManager::GetText("WP_FOLDER_NONE"), folderBuf[0] == '\0')) {
                            folderBuf[0] = '\0';
                        }
                        for (const auto& ef : existingFolders) {
                            bool isSel = (ef == folderBuf);
                            if (ImGui::Selectable(ef.c_str(), isSel)) {
                                snprintf(folderBuf, sizeof(folderBuf), "%s", ef.c_str());
                            }
                        }
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                }

                ImGui::Checkbox(LanguageManager::GetText("WP_PIN"), &isPinned);
                
                ImGui::Spacing();
                if (ImGui::Button(LanguageManager::GetText("WP_SAVE"), ImVec2(120.0f * curScale, 0))) {
                    WaypointManager::AddWaypoint(nameBuf, pos[0], pos[1], pos[2], col[0], col[1], col[2], wpTab, isPinned, folderBuf);
                    showAddPopup = false;
                    ImGui::CloseCurrentPopup();
                    NativeIME::Close();
                }
                ImGui::SameLine();
                if (ImGui::Button(LanguageManager::GetText("WP_CANCEL"), ImVec2(120.0f * curScale, 0))) {
                    showAddPopup = false;
                    ImGui::CloseCurrentPopup();
                    NativeIME::Close();
                }
                ImGui::EndPopup();
            }
            
            // 状态跳变检测：点击右上角 [X] 关闭新建窗口时的联动销毁
            if (lastShowAddPopup && !showAddPopup) {
                NativeIME::Close();
            }
            // 状态跳变检测：点击右上角 [X] 关闭整个管理器面板时的联动销毁
            if (lastShowWPUI && !MapRenderState::showWaypointUI) {
                NativeIME::Close();
            }
        }
        ImGui::End();
    }

    // ==========================================
    // [死亡记录] 死亡点管理控制台
    // ==========================================
    inline void RenderImGuiDeathPointUI() {
        float curScale = MapRenderState::globalUIScale;
        if (curScale < 0.1f) curScale = 1.0f;

        ImGui::SetNextWindowSize(ImVec2(760.0f * curScale, 520.0f * curScale), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSizeConstraints(ImVec2(680.0f * curScale, 300.0f * curScale), ImVec2(1920.0f * curScale, 1080.0f * curScale));
        ImVec2 center = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
        ImGui::SetNextWindowPos(center, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

        if (!ImGui::Begin(LanguageManager::GetText("DEATH_POINTS_TITLE"), &MapRenderState::showDeathPointUI, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::End();
            return;
        }

        std::vector<DeathPoint> points;
        {
            std::lock_guard<std::mutex> lock(DeathPointManager::g_deathMutex);
            points = DeathPointManager::g_deathPoints;
        }

        if (points.empty()) {
            ImGui::Dummy(ImVec2(1.0f, 120.0f * curScale));
            float textWidth = ImGui::CalcTextSize(LanguageManager::GetText("DEATH_POINTS_EMPTY")).x;
            ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
            ImGui::TextUnformatted(LanguageManager::GetText("DEATH_POINTS_EMPTY"));
            ImGui::End();
            return;
        }

        ImGui::TextDisabled("%s", LanguageManager::GetText("DEATH_POINTS_HINT"));
        ImGui::Separator();

        bool triggerTeleport = false;
        bool triggerLocate = false;
        int locateX = 0;
        int locateZ = 0;
        int locateDim = 0;
        std::string removeId;

        ImGui::BeginChild("DeathPointList", ImVec2(0, 0), false);
        for (const auto& point : points) {
            ImGui::PushID(point.id.c_str());

            ImVec4 rowBg = OreColor(35, 39, 42, 220);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, rowBg);
            ImGui::BeginChild("DeathRow", ImVec2(0, 80.0f * curScale), true, ImGuiWindowFlags_NoScrollbar);

            ImGui::BeginGroup();
            ImVec4 dimColor = (point.dimensionId == 0) ? OreColor(108, 214, 140) :
                              (point.dimensionId == 1) ? OreColor(235, 95, 95) :
                              (point.dimensionId == 2) ? OreColor(185, 120, 240) : OreColor(170, 174, 178);
            OreTag(DimensionText(point.dimensionId), dimColor);
            ImGui::SameLine();
            char dpCoordBuf[128];
            if (point.dimensionId == MapRenderState::currentDimensionId) {
                double dDist = std::sqrt((double)(point.x - g_playerBlockX) * (point.x - g_playerBlockX) + (double)(point.z - g_playerBlockZ) * (point.z - g_playerBlockZ));
                if (dDist >= 1000.0) {
                    snprintf(dpCoordBuf, sizeof(dpCoordBuf), "X: %d   Y: %d   Z: %d (%.1fkm)", point.x, point.y, point.z, dDist / 1000.0);
                } else {
                    snprintf(dpCoordBuf, sizeof(dpCoordBuf), "X: %d   Y: %d   Z: %d (%.0fm)", point.x, point.y, point.z, dDist);
                }
            } else {
                snprintf(dpCoordBuf, sizeof(dpCoordBuf), "X: %d   Y: %d   Z: %d", point.x, point.y, point.z);
            }
            ImGui::TextColored(OreColor(236, 238, 240), "%s", dpCoordBuf);
            ImGui::TextDisabled("%s", FormatDeathTime(point.timestamp).c_str());
            ImGui::EndGroup();

            const float btnWidthStd = 76.0f * curScale;
            const float btnWidthWp = 116.0f * curScale;
            const float btnSpacing = 8.0f * curScale;
            float totalButtonsWidth = btnWidthStd * 3 + btnWidthWp + btnSpacing * 3;
            float rightX = ImGui::GetWindowWidth() - totalButtonsWidth - 16.0f * curScale;
            if (rightX > ImGui::GetCursorPosX()) {
                ImGui::SameLine(rightX);
            } else {
                ImGui::SameLine();
            }

            ImGui::BeginGroup();

            // 1. 传送按钮 (支持跨维度传送)
            PushOreButtonStyle(OreButtonKind::Primary);
            if (ImGui::Button(LanguageManager::GetText("DEATH_POINT_TELEPORT"), ImVec2(btnWidthStd, 34.0f * curScale))) {
                MapRenderState::tpTargetX = (float)point.x + 0.5f;
                MapRenderState::tpTargetY = (float)point.y;
                MapRenderState::tpTargetZ = (float)point.z + 0.5f;
                MapRenderState::tpTargetDim = point.dimensionId;
                MapRenderState::triggerTeleport.store(true);
                triggerTeleport = true;
            }
            PopOreButtonStyle();

            ImGui::SameLine();

            // 2. 定位按钮 (在全屏大地图上居中定位该死亡点)
            PushOreButtonStyle(OreButtonKind::Warning);
            if (ImGui::Button(LanguageManager::GetText("DEATH_POINT_LOCATE"), ImVec2(btnWidthStd, 34.0f * curScale))) {
                triggerLocate = true;
                locateX = point.x;
                locateZ = point.z;
                locateDim = point.dimensionId;
            }
            PopOreButtonStyle();
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", LanguageManager::GetText("WP_LOCATE_ON_MAP"));
            }

            ImGui::SameLine();

            // 3. 转为路径点按钮 (对应路径点删除后可再次转换)
            bool isConverted = IsDeathPointConverted(point);
            if (isConverted) ImGui::BeginDisabled();
            PushOreButtonStyle(isConverted ? OreButtonKind::Default : OreButtonKind::Success);
            const char* btnLabel = isConverted ? LanguageManager::GetText("DEATH_POINT_ALREADY_CONVERTED") : LanguageManager::GetText("DEATH_POINT_CREATE_WP");
            if (ImGui::Button(btnLabel, ImVec2(btnWidthWp, 34.0f * curScale))) {
                if (!isConverted) {
                    std::string wpName = std::string(LanguageManager::GetText("DEATH_POINT_WP_PREFIX")) + " " + FormatDeathTime(point.timestamp);
                    std::string newWaypointId = WaypointManager::AddWaypoint(wpName, point.x, point.y, point.z, 0.95f, 0.25f, 0.25f, point.dimensionId);
                    WaypointManager::SaveWaypoints();
                    DeathPointManager::SetConverted(point.id, true, newWaypointId);
                }
            }
            PopOreButtonStyle();
            if (isConverted) ImGui::EndDisabled();

            ImGui::SameLine();

            // 4. 删除按钮
            PushOreButtonStyle(OreButtonKind::Danger);
            if (ImGui::Button(LanguageManager::GetText("DEATH_POINT_DELETE"), ImVec2(btnWidthStd, 34.0f * curScale))) {
                removeId = point.id;
            }
            PopOreButtonStyle();

            ImGui::EndGroup();

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::Spacing();
            ImGui::PopID();
        }

        if (!removeId.empty()) {
            DeathPointManager::RemoveDeathPoint(removeId);
        }

        ImGui::EndChild();

        if (triggerTeleport) {
            MapRenderState::showDeathPointUI = false;
            MapRenderState::showBigMap = false;
        }

        if (triggerLocate) {
            LocateBigMapOn(locateDim, (float)locateX + 0.5f, (float)locateZ + 0.5f, g_smoothPX, g_smoothPZ);
            MapRenderState::showDeathPointUI = false;
        }

        ImGui::End();
    }

    // ==========================================
    // [洞穴地图] 洞穴设置面板
    // ==========================================
    inline void RenderCaveSettings() {
        if (!MapRenderState::showCaveSettings) return;

        float curScale = MapRenderState::globalUIScale;
        if (curScale < 0.1f) curScale = 1.0f;

        ImGui::SetNextWindowSizeConstraints(ImVec2(460.0f * curScale, 100.0f * curScale), ImVec2(460.0f * curScale, 1000.0f * curScale));
        ImGui::SetNextWindowPos(ImVec2(50.0f * curScale, 50.0f * curScale), ImGuiCond_FirstUseEver);

        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin(LanguageManager::GetText("CAVE_SETTINGS"), &MapRenderState::showCaveSettings, winFlags)) {

            // === Cave Mode Type (下拉选择) ===
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("CAVE_MODE_TYPE"));
            ImGui::SameLine(200.0f * curScale);
            const char* caveTypeNames[] = {
                LanguageManager::GetText("CAVE_MODE_OFF"),
                LanguageManager::GetText("CAVE_MODE_LAYERED")
            };
            ImGui::SetNextItemWidth(220.0f * curScale);
            if (ImGui::Combo("##CaveModeType", &MapRenderState::g_caveModeType, caveTypeNames, 2)) {
                LanguageManager::SaveConfig();
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 440.0f * curScale);
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.55f, 1.0f), "%s", LanguageManager::GetText("CAVE_MODE_DESC"));
            ImGui::PopTextWrapPos();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === Cave Mode Top Y (auto / 手动) ===
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("CAVE_TOP_Y_MODE"));
            ImGui::SameLine(200.0f * curScale);
            const char* topYModes[] = {
                LanguageManager::GetText("CAVE_TOP_Y_AUTO"),
                LanguageManager::GetText("CAVE_TOP_Y_MANUAL")
            };
            int topYModeIdx = MapRenderState::g_caveTopYAuto ? 0 : 1;
            ImGui::SetNextItemWidth(220.0f * curScale);
            if (ImGui::Combo("##CaveTopYMode", &topYModeIdx, topYModes, 2)) {
                MapRenderState::g_caveTopYAuto = (topYModeIdx == 0);
                LanguageManager::SaveConfig();
            }

            ImGui::Spacing();

            // === Top Y: 滑块 + 输入框(带+-微调) + 重置 ===
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("CAVE_TOP_Y"));
            ImGui::SameLine(200.0f * curScale);
            int topY = MapRenderState::g_caveTopY;
            ImGui::SetNextItemWidth(140.0f * curScale);
            if (ImGui::SliderInt("##CaveTopY", &topY, -64, 320, "%d")) {
                MapRenderState::g_caveTopY = topY;
                LanguageManager::SaveConfig();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(55.0f * curScale);
            if (ImGui::InputInt("##CaveTopYIn", &topY, 1, 5, ImGuiInputTextFlags_EnterReturnsTrue)) {
                topY = (topY < -64) ? -64 : (topY > 320) ? 320 : topY;
                MapRenderState::g_caveTopY = topY;
                LanguageManager::SaveConfig();
            }
            ImGui::SameLine();
            if (ImGui::Button("\u21BA##ResetTopY", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
                MapRenderState::g_caveTopY = 64;  // 重置为海平面 (有效范围 [-64,320])
                LanguageManager::SaveConfig();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s (64)", LanguageManager::GetText("RESET"));
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 440.0f * curScale);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", LanguageManager::GetText("CAVE_TOP_Y_DESC"));
            ImGui::PopTextWrapPos();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === Cave Depth: 滑块 + 输入框(带+-微调) + 重置 ===
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", LanguageManager::GetText("CAVE_DEPTH"));
            ImGui::SameLine(200.0f * curScale);
            int depth = MapRenderState::g_caveDepth;
            ImGui::SetNextItemWidth(140.0f * curScale);
            if (ImGui::SliderInt("##CaveDepth", &depth, 1, 64, "%d")) {
                MapRenderState::g_caveDepth = depth;
                LanguageManager::SaveConfig();
            }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(55.0f * curScale);
            if (ImGui::InputInt("##CaveDepthIn", &depth, 1, 5, ImGuiInputTextFlags_EnterReturnsTrue)) {
                depth = (depth < 1) ? 1 : (depth > 64) ? 64 : depth;
                MapRenderState::g_caveDepth = depth;
                LanguageManager::SaveConfig();
            }
            ImGui::SameLine();
            if (ImGui::Button("\u21BA##ResetDepth", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
                MapRenderState::g_caveDepth = 30;
                LanguageManager::SaveConfig();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s (30)", LanguageManager::GetText("RESET"));
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 440.0f * curScale);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", LanguageManager::GetText("CAVE_DEPTH_DESC"));
            ImGui::PopTextWrapPos();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === Legible Cave Maps (复选框) ===
            if (ImGui::Checkbox(LanguageManager::GetText("CAVE_LEGIBLE"), &MapRenderState::g_legibleCaveMaps)) {
                LanguageManager::SaveConfig();
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 440.0f * curScale);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", LanguageManager::GetText("CAVE_LEGIBLE_DESC"));
            ImGui::PopTextWrapPos();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // === 当前状态 ===
            if (MapRenderState::g_caveModeActive) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s (Y=%d)",
                    LanguageManager::GetText("CAVE_ACTIVE"), MapRenderState::g_caveStartY);
            } else {
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", LanguageManager::GetText("CAVE_INACTIVE"));
            }
        }
        ImGui::End();
    }

    // ==========================================
    // [传送安全增强] 地形加载遮罩层 UI
    // 功能：
    //   - 在 pendingSurfaceProbe / TeleportState::Loading/Validating 期间显示
    //     半透明全屏遮罩 + 居中模态窗 + 旋转加载动画 + 状态消息
    //   - 在 TeleportState::Failed 期间显示失败提示（2.5 秒后自动重置）
    //   - 防止用户在加载未完成时触发其他传送或操作（模态拦截输入）
    // ==========================================
    inline void RenderTeleportLoadingOverlay() {
        int state = MapRenderState::teleportState.load();

        // 失败状态自动重置计时器（2.5 秒后回到 Idle，避免遮罩永久停留）
        static double failedShowStart = -1.0;
        if (state == (int)MapRenderState::TeleportState::Failed) {
            if (failedShowStart < 0.0) failedShowStart = ImGui::GetTime();
            if (ImGui::GetTime() - failedShowStart > 2.5) {
                MapRenderState::teleportState.store((int)MapRenderState::TeleportState::Idle);
                MapRenderState::teleportFailReason.clear();
                failedShowStart = -1.0;
                return;
            }
        } else if (state == (int)MapRenderState::TeleportState::Done) {
            MapRenderState::teleportState.store((int)MapRenderState::TeleportState::Idle);
            MapRenderState::teleportStatusMsg.clear();
            failedShowStart = -1.0;
            return;
        } else {
            failedShowStart = -1.0;
        }

        bool showLoading = MapRenderState::pendingSurfaceProbe.load() ||
                           state == (int)MapRenderState::TeleportState::Loading ||
                           state == (int)MapRenderState::TeleportState::Validating;
        bool showFailed = (state == (int)MapRenderState::TeleportState::Failed);

        if (!showLoading && !showFailed) return;

        float curScale = MapRenderState::globalUIScale;
        if (curScale < 0.1f) curScale = 1.0f;

        ImGuiIO& io = ImGui::GetIO();
        ImVec2 displaySize = io.DisplaySize;

        // 半透明黑色背景遮罩（仅在加载/失败状态下显示，避免遮挡画面其他时刻）
        if (showLoading) {
            ImGui::GetBackgroundDrawList()->AddRectFilled(
                ImVec2(0, 0), displaySize, IM_COL32(0, 0, 0, 140));
        } else if (showFailed) {
            ImGui::GetBackgroundDrawList()->AddRectFilled(
                ImVec2(0, 0), displaySize, IM_COL32(20, 0, 0, 130));
        }

        if (showLoading) {
            // 居中模态窗口（加载中）
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(380.0f * curScale, 0), ImGuiCond_Always);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoFocusOnAppearing;
            if (ImGui::Begin("##TeleportLoadingOverlay", nullptr, flags)) {
                // 旋转加载动画：8 个圆点围绕中心点旋转
                ImVec2 winPos = ImGui::GetWindowPos();
                float winW = ImGui::GetWindowWidth();
                ImVec2 center(winPos.x + winW * 0.5f, winPos.y + 50.0f * curScale);
                const int dotCount = 8;
                const float radius = 20.0f * curScale;
                float time = (float)ImGui::GetTime();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                for (int i = 0; i < dotCount; ++i) {
                    float angle = time * 3.5f + (float)i * (2.0f * 3.14159265f / dotCount);
                    float x = center.x + std::cos(angle) * radius;
                    float y = center.y + std::sin(angle) * radius;
                    // 透明度按 i 递减，营造旋转拖尾效果
                    int alpha = 255 - (i * 220 / dotCount);
                    if (alpha < 30) alpha = 30;
                    ImU32 dotCol = IM_COL32(120, 200, 255, alpha);
                    dl->AddCircleFilled(ImVec2(x, y), 5.0f * curScale, dotCol);
                }

                // 占位高度（为动画留出空间）
                ImGui::Dummy(ImVec2(0, 80.0f * curScale));

                // 主状态消息文本（居中）
                const char* msg = MapRenderState::teleportStatusMsg.empty() ?
                                  LanguageManager::GetText("TELEPORT_LOADING") :
                                  MapRenderState::teleportStatusMsg.c_str();
                float textWidth = ImGui::CalcTextSize(msg).x;
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - textWidth) * 0.5f);
                ImGui::Text("%s", msg);

                // 辅助提示文本（灰色）
                ImGui::Spacing();
                const char* hint = LanguageManager::GetText("TELEPORT_LOADING_HINT");
                float hintWidth = ImGui::CalcTextSize(hint).x;
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - hintWidth) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::Text("%s", hint);
                ImGui::PopStyleColor();
            }
            ImGui::End();
        } else if (showFailed) {
            // 居中模态窗口（失败提示）
            ImGui::SetNextWindowPos(ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f),
                                    ImGuiCond_Always, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(400.0f * curScale, 0), ImGuiCond_Always);
            ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar |
                                     ImGuiWindowFlags_NoFocusOnAppearing;
            if (ImGui::Begin("##TeleportFailedOverlay", nullptr, flags)) {
                // 红色失败标题（居中）
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.35f, 0.35f, 1.0f));
                const char* failTitle = LanguageManager::GetText("TELEPORT_FAILED");
                float titleWidth = ImGui::CalcTextSize(failTitle).x;
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - titleWidth) * 0.5f);
                ImGui::Text("%s", failTitle);
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // 失败详情消息（居中，灰白色）
                const char* failMsg = MapRenderState::teleportFailReason.empty() ?
                                      LanguageManager::GetText("TELEPORT_FAILED_MSG") :
                                      MapRenderState::teleportFailReason.c_str();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.85f, 0.85f, 1.0f));
                float msgWidth = ImGui::CalcTextSize(failMsg).x;
                float availW = ImGui::GetWindowWidth() - 2.0f * ImGui::GetStyle().WindowPadding.x;
                if (msgWidth > availW) {
                    // 长消息使用 TextWrapped 居中
                    ImGui::SetCursorPosX(ImGui::GetStyle().WindowPadding.x);
                    ImGui::TextWrapped("%s", failMsg);
                } else {
                    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - msgWidth) * 0.5f);
                    ImGui::Text("%s", failMsg);
                }
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // 自动消失提示
                const char* dismissHint = LanguageManager::GetText("TELEPORT_FAILED_DISMISS");
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                float hintWidth = ImGui::CalcTextSize(dismissHint).x;
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - hintWidth) * 0.5f);
                ImGui::Text("%s", dismissHint);
                ImGui::PopStyleColor();
            }
            ImGui::End();
        }
    }

    // ==========================================
    // [PNG导出] World Map PNG Export 原生 ImGui 窗口
    // ==========================================
    inline void RenderExportPNGScreen() {
        static bool s_wasWindowOpen = false;
        if (!MapRenderState::showExportPNGScreen) {
            s_wasWindowOpen = false;
            return;
        }
        if (!s_wasWindowOpen) {
            MapCacheManager::InvalidateExportPreview();
            s_wasWindowOpen = true;
        }

        float curScale = MapRenderState::globalUIScale;
        if (curScale < 0.1f) curScale = 1.0f;

        ImGuiIO& io = ImGui::GetIO();
        ImGui::SetNextWindowPos(
            ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
            ImGuiCond_Appearing,
            ImVec2(0.5f, 0.5f)
        );
        ImGui::SetNextWindowSizeConstraints(ImVec2(480.0f * curScale, 0.0f), ImVec2(680.0f * curScale, io.DisplaySize.y * 0.9f));

        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin(LanguageManager::GetText("EXPORT_SCREEN_TITLE"), &MapRenderState::showExportPNGScreen, winFlags)) {
            int stage = MapRenderState::exportStage.load();
            int resType = MapRenderState::exportResultType.load();
            bool isExporting = (stage == 1);

            ImGui::BeginDisabled(isExporting);

            // 1. 强制全图导出 (Force Full Map) - 持久保存
            if (ImGui::Checkbox(LanguageManager::GetText("EXPORT_OPT_FULL"), &MapRenderState::exportForceFullMap)) {
                LanguageManager::SaveConfig();
                MapCacheManager::InvalidateExportPreview();
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 480.0f * curScale);
            ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f), "%s", LanguageManager::GetText("EXPORT_OPT_FULL_DESC"));
            ImGui::PopTextWrapPos();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 2. 多张无缩放图像 (Multiple Unscaled Images) - 持久保存
            if (ImGui::Checkbox(LanguageManager::GetText("EXPORT_OPT_MULTI"), &MapRenderState::exportMultipleImages)) {
                LanguageManager::SaveConfig();
                MapCacheManager::InvalidateExportPreview();
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 480.0f * curScale);
            ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f), "%s", LanguageManager::GetText("EXPORT_OPT_MULTI_DESC"));
            ImGui::PopTextWrapPos();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 3. 导出后自动打开文件夹 (Open Folder After Export) - 持久保存
            if (ImGui::Checkbox(LanguageManager::GetText("EXPORT_OPT_OPEN_FOLDER"), &MapRenderState::exportOpenFolder)) {
                LanguageManager::SaveConfig();
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 480.0f * curScale);
            ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f), "%s", LanguageManager::GetText("EXPORT_OPT_OPEN_FOLDER_DESC"));
            ImGui::PopTextWrapPos();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 4. 单张图像最大尺寸 (Max Single Image Size: 0=不限制, 1..90 区域) - 持久保存
            ImGui::BeginDisabled(MapRenderState::exportMultipleImages);
            char maxSizeValBuf[64];
            if (MapRenderState::exportScaleDownSquare <= 0) {
                snprintf(maxSizeValBuf, sizeof(maxSizeValBuf), "%s", LanguageManager::GetText("EXPORT_OPT_MAX_SIZE_UNSCALED"));
            } else {
                snprintf(maxSizeValBuf, sizeof(maxSizeValBuf), LanguageManager::GetText("EXPORT_OPT_MAX_SIZE_VAL"),
                         MapRenderState::exportScaleDownSquare, MapRenderState::exportScaleDownSquare);
            }
            ImGui::Text("%s: %s", LanguageManager::GetText("EXPORT_OPT_MAX_SIZE"), maxSizeValBuf);
            ImGui::PushItemWidth(260.0f * curScale);
            if (ImGui::SliderInt("##export_max_size_slider", &MapRenderState::exportScaleDownSquare, 0, 90, maxSizeValBuf)) {
                MapRenderState::exportScaleDownSquare = std::clamp(MapRenderState::exportScaleDownSquare, 0, 90);
                MapCacheManager::InvalidateExportPreview();
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                LanguageManager::SaveConfig();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            ImGui::PushItemWidth(95.0f * curScale);
            if (ImGui::InputInt("##export_max_size_input", &MapRenderState::exportScaleDownSquare, 1, 10)) {
                MapRenderState::exportScaleDownSquare = std::clamp(MapRenderState::exportScaleDownSquare, 0, 90);
                MapCacheManager::InvalidateExportPreview();
            }
            if (ImGui::IsItemDeactivatedAfterEdit()) {
                LanguageManager::SaveConfig();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("\u21BA##reset_export_max_size", ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()))) {
                MapRenderState::exportScaleDownSquare = 20;
                LanguageManager::SaveConfig();
                MapCacheManager::InvalidateExportPreview();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s (20)", LanguageManager::GetText("RESET"));
            }
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 480.0f * curScale);
            ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.0f), "%s", LanguageManager::GetText("EXPORT_OPT_MAX_SIZE_DESC"));
            ImGui::PopTextWrapPos();
            ImGui::EndDisabled();

            ImGui::EndDisabled(); // isExporting

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 5. 实时无损导出规格预览 (ImGui 面板直接展示范围与分辨率)
            auto preview = MapCacheManager::GetExportPreviewInfo();
            if (preview.hasValidPixels) {
                ImGui::TextColored(
                    ImVec4(0.55f, 0.85f, 1.0f, 1.0f),
                    "X: %d .. %d,  Z: %d .. %d  (%d x %d Blocks)",
                    preview.minBlockX, preview.maxBlockX,
                    preview.minBlockZ, preview.maxBlockZ,
                    preview.areaW, preview.areaH
                );
                if (MapRenderState::exportMultipleImages) {
                    ImGui::TextColored(
                        ImVec4(0.55f, 1.0f, 0.65f, 1.0f),
                        "PNG Tiles: %d  |  Tile Size: %d x %d px (%dx Lossless)",
                        preview.tileCount, preview.outW, preview.outH, preview.effectivePixelScale
                    );
                } else {
                    ImGui::TextColored(
                        ImVec4(0.55f, 1.0f, 0.65f, 1.0f),
                        "Output PNG: %d x %d px  (%dx Lossless: 1 Block = %dx%d px)",
                        preview.outW, preview.outH,
                        preview.effectivePixelScale, preview.effectivePixelScale, preview.effectivePixelScale
                    );
                }
            }

            // 6. 导出进度与结果提示
            if (stage == 1) {
                ImGui::Spacing();
                if (MapRenderState::exportMultipleImages) {
                    int total = MapRenderState::exportTotalTiles.load();
                    int completed = MapRenderState::exportCompletedTiles.load();
                    float fraction = (total > 0) ? (static_cast<float>(completed) / static_cast<float>(total)) : 0.0f;
                    if (fraction > 1.0f) fraction = 1.0f;

                    char progressBuf[128];
                    snprintf(progressBuf, sizeof(progressBuf), "%s: %d / %d (%.0f%%)",
                             LanguageManager::GetText("EXPORT_PROGRESS"), completed, total, fraction * 100.0f);

                    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.7f, 0.35f, 1.0f));
                    ImGui::ProgressBar(fraction, ImVec2(-1.0f, 24.0f * curScale), progressBuf);
                    ImGui::PopStyleColor();

                    ImGui::Spacing();
                    bool cancelRequested = MapRenderState::exportCancelRequested.load();
                    ImGui::BeginDisabled(cancelRequested);
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.72f, 0.22f, 0.22f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.32f, 0.32f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.16f, 0.16f, 1.0f));
                    if (ImGui::Button(LanguageManager::GetText("EXPORT_CANCEL_AND_CLEAR"), ImVec2(-1.0f, 28.0f * curScale))) {
                        MapRenderState::exportCancelRequested.store(true);
                    }
                    ImGui::PopStyleColor(3);
                    ImGui::EndDisabled();
                } else {
                    ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.35f, 1.0f), "%s", LanguageManager::GetText("EXPORT_EXPORTING"));
                }
            } else if (stage == 2 && resType >= 0) {
                ImGui::Spacing();
                const char* resMsg = "";
                ImVec4 resCol = ImVec4(0.35f, 1.0f, 0.35f, 1.0f);
                switch (resType) {
                    case 0: resMsg = LanguageManager::GetText("EXPORT_RES_SUCCESS"); resCol = ImVec4(0.35f, 1.0f, 0.35f, 1.0f); break;
                    case 1: resMsg = LanguageManager::GetText("EXPORT_RES_EMPTY"); resCol = ImVec4(1.0f, 0.35f, 0.35f, 1.0f); break;
                    case 2: resMsg = LanguageManager::GetText("EXPORT_RES_NOT_PREPARED"); resCol = ImVec4(1.0f, 0.35f, 0.35f, 1.0f); break;
                    case 3: resMsg = LanguageManager::GetText("EXPORT_RES_TOO_BIG"); resCol = ImVec4(1.0f, 0.35f, 0.35f, 1.0f); break;
                    case 4: resMsg = LanguageManager::GetText("EXPORT_RES_OOM"); resCol = ImVec4(1.0f, 0.35f, 0.35f, 1.0f); break;
                    case 6: resMsg = LanguageManager::GetText("EXPORT_RES_CANCELED"); resCol = ImVec4(1.0f, 0.45f, 0.35f, 1.0f); break;
                    default: resMsg = LanguageManager::GetText("EXPORT_RES_IO"); resCol = ImVec4(1.0f, 0.35f, 0.35f, 1.0f); break;
                }
                ImGui::TextColored(resCol, "%s", resMsg);
                std::string pathCopy;
                {
                    std::lock_guard<std::mutex> lk(MapRenderState::exportResultMutex);
                    pathCopy = MapRenderState::exportResultPath;
                }
                if (!pathCopy.empty() && resType == 0) {
                    ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + 480.0f * curScale);
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", pathCopy.c_str());
                    ImGui::PopTextWrapPos();
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // 7. 底部操作按钮: 确认 (Confirm) / 返回 (Back)
            float availW = ImGui::GetContentRegionAvail().x;
            float btnW = std::max(140.0f * curScale, (availW - 12.0f * curScale) * 0.5f);

            ImGui::BeginDisabled(isExporting);
            if (ImGui::Button(LanguageManager::GetText("EXPORT_CONFIRM"), ImVec2(btnW, 32.0f * curScale))) {
                MapCacheManager::TriggerExportMapToPNG();
            }
            ImGui::EndDisabled();

            ImGui::SameLine(0.0f, 12.0f * curScale);
            if (ImGui::Button(LanguageManager::GetText("EXPORT_BACK"), ImVec2(btnW, 32.0f * curScale))) {
                MapRenderState::showExportPNGScreen = false;
            }
        }
        ImGui::End();
    }

    inline void RenderImGui(IDXGISwapChain* pSwapChain) {
        static std::atomic<bool> isRendering{false};
        if (isRendering.exchange(true)) return;

        // [关闭期安全] 进程退出阶段已开始时，所有 D3D/ImGui 资源可能在 ShutdownImGuiAndBuffers
        // 中被释放；此时进入渲染路径会访问悬空指针，导致 0xC0000005 退出崩溃。
        // 直接放行 Present 到原函数，由 MH_DisableHook 完成后此回调自然不再被调用。
        if (MapRenderState::g_isShuttingDown.load()) {
            isRendering = false;
            return;
        }

        if (g_clientInstance) {
            __try {
                if (g_clientInstance->isShowingLoadingScreen() ||
                    g_clientInstance->isShowingProgressScreen() ||
                    g_clientInstance->isShowingWorldProgressScreen() ||
                    g_clientInstance->isShowingDeathScreen() ||
                    (g_clientInstance->isShowingPauseScreen() && !MapRenderState::IsUIActive())) {
                    isRendering = false;
                    return;
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                isRendering = false;
                return;
            }
        }

        static bool initAttempted = false; 
        if (!g_imguiInitialized && !initAttempted) {
            HRESULT hr11 = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&g_pd3dDevice);
            if (SUCCEEDED(hr11)) {
                initAttempted = true;
                g_pd3dDevice->GetImmediateContext(&g_pd3dDeviceContext);
            } else {
                if (!g_pGameCommandQueue) { isRendering = false; return; }
                initAttempted = true;
                ID3D12Device* pD3D12Device = nullptr;
                if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D12Device), (void**)&pD3D12Device))) {
                    if (SUCCEEDED(D3D11On12CreateDevice(pD3D12Device, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0, (IUnknown**)&g_pGameCommandQueue, 1, 0, &g_pd3dDevice, &g_pd3dDeviceContext, nullptr))) {
                        g_pd3dDevice->QueryInterface(__uuidof(ID3D11On12Device), (void**)&g_d3d11On12Device);
                    }
                    pD3D12Device->Release();
                }
            }

            if (g_pd3dDevice) {
                DXGI_SWAP_CHAIN_DESC sd;
                pSwapChain->GetDesc(&sd);
                g_hWnd = sd.OutputWindow;
                if (!g_hWnd) g_hWnd = FindWindowW(L"Minecraft", NULL);

                oWndProc = (WNDPROC)SetWindowLongPtr(g_hWnd, GWLP_WNDPROC, (LONG_PTR)WndProcHook);
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();

                InitImGuiFonts(io);
                ImGui_ImplWin32_Init(g_hWnd);
                ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

                g_baseImGuiStyle = ImGui::GetStyle();
                g_baseStyleSaved = true;
                g_lastAppliedUIScale = -1.0f;

                InitMapTexture();
                g_imguiInitialized = true;
            }
        }

        if (g_imguiInitialized && g_hasPlayer) {
            // [性能·核心优化] 无 UI 时跳过整个 ImGui + D3D11On12 渲染管线
            // 原实现每帧无条件执行 ImGui NewFrame/Render + D3D11On12 Acquire/Release/Flush，
            // 即使小地图关闭、大地图未开、无任何面板可见。现改为仅在有 UI 需要渲染时才执行，
            // 无 UI 时仅调用 UpdateSmoothCamera (已改为不依赖 ImGui)，使模组开销趋近于零。

            // 【修复退出 UI 后原生界面光标消失/锁死】全屏大地图等 UI 激活期间，我们拦截了
            // SetCursorPos 以阻止光标被游戏底层锁回中心 (修横跳)，但这会打断游戏的指针锁定状态机，
            // 使其 ShowCursor 隐藏计数停留在负值、指针锁定未释放。一旦 Esc 关闭 UI，背包/容器/暂停
            // 等原生界面本应解锁并显示系统光标，却因状态错乱导致光标被隐藏并锁死在屏幕中心、
            // 看不见也动不了。故在 UI 由激活转为非激活的下降沿，强制把硬件光标可见计数拉回非负，
            // 并解除任何残留的光标裁剪，让原生界面恢复正常鼠标操作。
            {
                static bool s_wasUIActive = false;
                bool uiActiveNow = MapRenderState::IsUIActive();
                if (s_wasUIActive && !uiActiveNow) {
                    int cur = ShowCursor(TRUE);
                    while (cur < 0) { cur = ShowCursor(TRUE); }
                    ClipCursor(NULL);
                }
                s_wasUIActive = uiActiveNow;
            }

            UpdateSmoothCamera();

            bool isTeleportUI = (MapRenderState::teleportState.load() == (int)MapRenderState::TeleportState::Loading ||
                                 MapRenderState::teleportState.load() == (int)MapRenderState::TeleportState::Validating ||
                                 MapRenderState::teleportState.load() == (int)MapRenderState::TeleportState::Failed);
            bool needsRender = MapRenderState::showMiniMap || MapRenderState::IsUIActive() || isTeleportUI;
            if (!needsRender) { isRendering = false; return; }

            auto renderImGuiFrame = [&](ID3D11RenderTargetView* rtv) {
                ImGuiIO& io = ImGui::GetIO();
                if (MapRenderState::globalUIScale < 0.1f) {
                    MapRenderState::globalUIScale = MapRenderState::GetOptimalUIScale(io.DisplaySize.y);
                }
                float curScale = MapRenderState::globalUIScale;
                if (curScale < 0.1f) curScale = 1.0f;
                if (!g_baseStyleSaved) {
                    g_baseImGuiStyle = ImGui::GetStyle();
                    g_baseStyleSaved = true;
                }

                if (std::abs(curScale - g_lastAppliedUIScale) > 0.001f) {
                    g_lastAppliedUIScale = curScale;
                    io.FontGlobalScale = curScale;
                    ImGui::GetStyle() = g_baseImGuiStyle;
                    ImGui::GetStyle().ScaleAllSizes(curScale);
                    // 【修复鼠标指针消失】ImGui 的 ScaleAllSizes 内部使用 ImTrunc/ImFloor 对 MouseCursorScale 截断取整，
                    // 导致当 curScale < 1.0f 时，1.0f * curScale 截断后变为 0.0f，使软件光标尺寸缩放为 0x0 像素而完全消失！
                    // 此处确保 MouseCursorScale 始终维持不低于 1.0f 的正常标准尺寸，UI 放大时随 UI 等比放大
                    ImGui::GetStyle().MouseCursorScale = std::max(curScale, 1.0f);
                }
                // 每帧保底校验：确保软件光标缩放比例绝不低于 1.0f
                if (ImGui::GetStyle().MouseCursorScale < 1.0f) {
                    ImGui::GetStyle().MouseCursorScale = std::max(curScale, 1.0f);
                }

                g_pd3dDeviceContext->OMSetRenderTargets(1, &rtv, NULL);
                ImGui_ImplDX11_NewFrame(); ImGui_ImplWin32_NewFrame(); ImGui::NewFrame();

                bool isTextInputActive = NativeIME::isTyping.load() || (ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput);
                const auto& holdHk = MapRenderState::g_hotkeys.holdEntities;
                if (!isTextInputActive && !holdHk.IsEmpty() && MapRenderState::g_listeningHotkey == nullptr) {
                    bool keyState = (ReadPhysicalKeyState(holdHk.key) & 0x8000) != 0;
                    if (keyState) {
                        bool ctrlDown = ((ReadPhysicalKeyState(VK_CONTROL) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_LCONTROL) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_RCONTROL) & 0x8000) != 0) || (ImGui::GetCurrentContext() && ImGui::GetIO().KeyCtrl);
                        bool shiftDown = ((ReadPhysicalKeyState(VK_SHIFT) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_LSHIFT) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_RSHIFT) & 0x8000) != 0) || (ImGui::GetCurrentContext() && ImGui::GetIO().KeyShift);
                        bool altDown = ((ReadPhysicalKeyState(VK_MENU) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_LMENU) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_RMENU) & 0x8000) != 0) || (ImGui::GetCurrentContext() && ImGui::GetIO().KeyAlt);
                        bool ctrlOk  = ((holdHk.modifiers & MapRenderState::Hotkey::HK_MOD_CTRL) != 0) == ctrlDown;
                        bool shiftOk = ((holdHk.modifiers & MapRenderState::Hotkey::HK_MOD_SHIFT) != 0) == shiftDown;
                        bool altOk   = ((holdHk.modifiers & MapRenderState::Hotkey::HK_MOD_ALT) != 0) == altDown;
                        g_tabHeld = (ctrlOk && shiftOk && altOk);
                    } else {
                        g_tabHeld = false;
                    }
                } else {
                    g_tabHeld = false;
                }

                const auto& enlargeHk = MapRenderState::g_hotkeys.enlargeMinimap;
                if (!isTextInputActive && !enlargeHk.IsEmpty() && MapRenderState::g_listeningHotkey == nullptr && !MapRenderState::enlargeMinimapToggle) {
                    bool keyState = (ReadPhysicalKeyState(enlargeHk.key) & 0x8000) != 0;
                    if (keyState) {
                        bool ctrlDown = ((ReadPhysicalKeyState(VK_CONTROL) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_LCONTROL) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_RCONTROL) & 0x8000) != 0) || (ImGui::GetCurrentContext() && ImGui::GetIO().KeyCtrl);
                        bool shiftDown = ((ReadPhysicalKeyState(VK_SHIFT) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_LSHIFT) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_RSHIFT) & 0x8000) != 0) || (ImGui::GetCurrentContext() && ImGui::GetIO().KeyShift);
                        bool altDown = ((ReadPhysicalKeyState(VK_MENU) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_LMENU) & 0x8000) != 0) || ((ReadPhysicalKeyState(VK_RMENU) & 0x8000) != 0) || (ImGui::GetCurrentContext() && ImGui::GetIO().KeyAlt);
                        bool ctrlOk  = ((enlargeHk.modifiers & MapRenderState::Hotkey::HK_MOD_CTRL) != 0) == ctrlDown;
                        bool shiftOk = ((enlargeHk.modifiers & MapRenderState::Hotkey::HK_MOD_SHIFT) != 0) == shiftDown;
                        bool altOk   = ((enlargeHk.modifiers & MapRenderState::Hotkey::HK_MOD_ALT) != 0) == altDown;
                        MapRenderState::g_enlargeHeld.store(ctrlOk && shiftOk && altOk);
                    } else {
                        MapRenderState::g_enlargeHeld.store(false);
                    }
                } else {
                    if (!MapRenderState::enlargeMinimapToggle) {
                        MapRenderState::g_enlargeHeld.store(false);
                    }
                }

                // 【修复光标被覆盖】当原生文本框开启时，关闭 ImGui 的软件光标，将显示权交还给被我们强制唤醒的 Windows 硬件光标
                ImGui::GetIO().MouseDrawCursor = MapRenderState::IsUIActive() && !NativeIME::isTyping.load();

                static bool s_wasBigMapActive = false;
                if (MapRenderState::showBigMap) {
                    if (!s_wasBigMapActive) {
                        MapRenderState::bigMapOverworldCave = (MapRenderState::currentDimensionId == 0 && MapRenderState::g_caveModeActive);
                    }
                    s_wasBigMapActive = true;
                    RenderImGuiBigMap();
                } else {
                    if (s_wasBigMapActive) {
                        s_wasBigMapActive = false;
                        // 大地图关闭：保存关闭前所在维度的视角中心与缩放
                        int closingDim = MapRenderState::GetEffectiveViewDimensionId();
                        MapRenderState::SaveDimCamera(closingDim, g_smoothPX, g_smoothPZ);
                        MapRenderState::bigMapOverworldCave = false;

                        // 大地图关闭：若曾切换到其他维度浏览，立即切回玩家物理所在维度
                        // 注意：若正在进行跨维度传送，不要切回原物理维度，让传送流程接管维度切换
                        if (MapRenderState::triggerTeleport.load() && MapRenderState::tpTargetDim >= 0 && MapRenderState::tpTargetDim != MapRenderState::currentDimensionId) {
                            // 跨维度传送进行中，跳过强制切回
                        } else if (MapCacheManager::GetLoadedDimensionId() != MapRenderState::currentDimensionId && !MapRenderState::currentWorldId.empty()) {
                            MapCacheManager::SwitchWorld(MapRenderState::currentWorldId, MapRenderState::currentDimensionId);
                            MapRenderState::clearGPUCache.store(true);
                        }
                        MapRenderState::bigMapViewDimensionId = -999;
                        s_gotoTargetActive = false;
                        s_gotoTargetTimer = 0.0f;
                    }
                    RenderImGuiMiniMap();
                }

                if (MapRenderState::showWaypointUI) {
                    RenderImGuiWaypointUI();
                }

                if (MapRenderState::showDeathPointUI) {
                    RenderImGuiDeathPointUI();
                }

                // 无条件调用，内含状态检测，处理还原逻辑
                RenderMiniMapPosSettings();

                // [小地图设置] 面板 (在大地图内渲染)
                RenderMiniMapSettings();

                // [全屏大地图设置] 面板 (在大地图内渲染)
                RenderBigMapSettings();

                // [Task 3] 快捷键设置面板 (内含状态检测)
                RenderHotkeySettingsWindow();

                // [洞穴地图] 洞穴设置面板 (内含状态检测)
                RenderCaveSettings();

                // [PNG导出] World Map PNG Export 全屏界面
                RenderExportPNGScreen();

                // [传送安全增强] 地形加载遮罩层 (在所有 UI 之上，模态拦截输入)
                RenderTeleportLoadingOverlay();

                ImGui::Render();
                ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

                ID3D11RenderTargetView* nullRTV = nullptr;
                g_pd3dDeviceContext->OMSetRenderTargets(1, &nullRTV, NULL);
                // [性能] RTV 由调用方管理生命周期 (D3D11On12 路径缓存复用, D3D11 路径调用后释放)
            };

            if (g_d3d11On12Device) {
                // [全屏切换修复] 回退为每帧即时模式，对齐 0.3.4 不崩溃版本。
                // 原先跨帧缓存的 D3D11On12 包装资源 + RTV (g_cachedWrappedBuffers) 在全屏/窗口
                // 切换 (F11) 时交换链状态变化，缓存资源指向失效的 back buffer，复用写越界触发
                // 游戏栈溢出保护 (0xC0000409 fastfail)。每帧即时 GetBuffer + CreateWrappedResource
                // + CreateRenderTargetView + Release 虽略有开销，但始终绑定当前有效缓冲。
                UINT bufferIndex = 0;
                IDXGISwapChain3* pSwapChain3 = nullptr;
                if (SUCCEEDED(pSwapChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&pSwapChain3))) {
                    bufferIndex = pSwapChain3->GetCurrentBackBufferIndex();
                    pSwapChain3->Release();
                }

                ID3D12Resource* d3d12BackBuffer = nullptr;
                if (SUCCEEDED(pSwapChain->GetBuffer(bufferIndex, __uuidof(ID3D12Resource), (void**)&d3d12BackBuffer))) {
                    ID3D11Resource* wrappedBackBuffer = nullptr;
                    D3D11_RESOURCE_FLAGS d3d11Flags = {D3D11_BIND_RENDER_TARGET};

                    if (SUCCEEDED(g_d3d11On12Device->CreateWrappedResource(
                        d3d12BackBuffer, &d3d11Flags, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_PRESENT, __uuidof(ID3D11Resource), (void**)&wrappedBackBuffer)))
                    {
                        ID3D11RenderTargetView* rtv = nullptr;
                        g_pd3dDevice->CreateRenderTargetView(wrappedBackBuffer, NULL, &rtv);
                        g_d3d11On12Device->AcquireWrappedResources(&wrappedBackBuffer, 1);

                        if (rtv) renderImGuiFrame(rtv);

                        g_d3d11On12Device->ReleaseWrappedResources(&wrappedBackBuffer, 1);
                        wrappedBackBuffer->Release();
                        if (rtv) rtv->Release();
                        // Flush 确保 D3D11 命令在 D3D12 barrier 之前提交 (D3D11On12 同步要求)。
                        g_pd3dDeviceContext->Flush();
                    }
                    d3d12BackBuffer->Release();
                }
            } else {
                ID3D11Texture2D* pBackBuffer = nullptr;
                if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer))) {
                    ID3D11RenderTargetView* rtv = nullptr;
                    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
                    pBackBuffer->Release();
                    if (rtv) {
                        renderImGuiFrame(rtv);
                        rtv->Release();
                    }
                }
            }
        }
        isRendering = false;
    }

    inline HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oPresent) return oPresent(pSwapChain, SyncInterval, Flags);
            return S_OK;
        }
        RenderImGui(pSwapChain);
        return oPresent(pSwapChain, SyncInterval, Flags);
    }

    inline HRESULT __stdcall hkPresent1(IDXGISwapChain1* pSwapChain, UINT SyncInterval, UINT Flags, const DXGI_PRESENT_PARAMETERS* pParams) {
        if (MapRenderState::g_isShuttingDown.load()) {
            if (oPresent1) return oPresent1(pSwapChain, SyncInterval, Flags, pParams);
            return S_OK;
        }
        RenderImGui(pSwapChain);
        return oPresent1(pSwapChain, SyncInterval, Flags, pParams);
    }

    inline bool init() {
        static std::mutex s_initMutex;
        static std::atomic<bool> s_initialized{false};
        if (s_initialized.load()) return true;

        std::lock_guard<std::mutex> lock(s_initMutex);
        if (s_initialized.load()) return true;

        HWND hwnd = FindWindowW(L"Minecraft", NULL);
        if (!hwnd) hwnd = GetForegroundWindow();
        bool createdDummyHwnd = false;
        if (!hwnd) {
            hwnd = CreateWindowExW(0, L"STATIC", L"ChiyanDummy", WS_OVERLAPPED, 0, 0, 100, 100, NULL, NULL, GetModuleHandle(NULL), NULL);
            createdDummyHwnd = (hwnd != NULL);
        }
        if (!hwnd) return false;
        
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1; sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; sd.OutputWindow = hwnd;
        sd.SampleDesc.Count = 1; sd.Windowed = TRUE; sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11Device* dummyDevice = nullptr;
        IDXGISwapChain* dummySwapChain = nullptr;
        ID3D11DeviceContext* dummyContext = nullptr;

        MH_STATUS status = MH_Initialize();
        if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) {
            if (createdDummyHwnd && hwnd) DestroyWindow(hwnd);
            return false;
        }

        bool d3d11Success = false;
        if (SUCCEEDED(D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &featureLevel, 1, D3D11_SDK_VERSION, &sd, &dummySwapChain, &dummyDevice, NULL, &dummyContext))) {
            void** pVTable = *reinterpret_cast<void***>(dummySwapChain);
            
            if (MH_CreateHook(pVTable[8], (LPVOID)hkPresent, (void**)&oPresent) == MH_OK) {
                MH_QueueEnableHook(pVTable[8]);
            }
            if (MH_CreateHook(pVTable[13], (LPVOID)hkResizeBuffers, (void**)&oResizeBuffers) == MH_OK) {
                MH_QueueEnableHook(pVTable[13]);
            }

            IDXGISwapChain1* dummySwapChain1 = nullptr;
            if (SUCCEEDED(dummySwapChain->QueryInterface(__uuidof(IDXGISwapChain1), (void**)&dummySwapChain1))) {
                void** pVTable1 = *reinterpret_cast<void***>(dummySwapChain1);
                if (MH_CreateHook(pVTable1[22], (LPVOID)hkPresent1, (void**)&oPresent1) == MH_OK) {
                    MH_QueueEnableHook(pVTable1[22]);
                }
                dummySwapChain1->Release();
            }

            HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
            if (hUser32) {
                void* pGetRawInputData = (void*)GetProcAddress(hUser32, "GetRawInputData");
                if (pGetRawInputData && MH_CreateHook(pGetRawInputData, (LPVOID)hkGetRawInputData, (void**)&oGetRawInputData) == MH_OK) {
                    MH_QueueEnableHook(pGetRawInputData);
                }
                void* pGetRawInputBuffer = (void*)GetProcAddress(hUser32, "GetRawInputBuffer");
                if (pGetRawInputBuffer && MH_CreateHook(pGetRawInputBuffer, (LPVOID)hkGetRawInputBuffer, (void**)&oGetRawInputBuffer) == MH_OK) {
                    MH_QueueEnableHook(pGetRawInputBuffer);
                }
                void* pGetAsyncKeyState = (void*)GetProcAddress(hUser32, "GetAsyncKeyState");
                if (pGetAsyncKeyState && MH_CreateHook(pGetAsyncKeyState, (LPVOID)hkGetAsyncKeyState, (void**)&oGetAsyncKeyState) == MH_OK) {
                    MH_QueueEnableHook(pGetAsyncKeyState);
                }
                void* pGetKeyState = (void*)GetProcAddress(hUser32, "GetKeyState");
                if (pGetKeyState && MH_CreateHook(pGetKeyState, (LPVOID)hkGetKeyState, (void**)&oGetKeyState) == MH_OK) {
                    MH_QueueEnableHook(pGetKeyState);
                }
                void* pGetCursorPos = (void*)GetProcAddress(hUser32, "GetCursorPos");
                if (pGetCursorPos && MH_CreateHook(pGetCursorPos, (LPVOID)hkGetCursorPos, (void**)&oGetCursorPos) == MH_OK) {
                    MH_QueueEnableHook(pGetCursorPos);
                }
                void* pSetCursorPos = (void*)GetProcAddress(hUser32, "SetCursorPos");
                if (pSetCursorPos && MH_CreateHook(pSetCursorPos, (LPVOID)hkSetCursorPos, (void**)&oSetCursorPos) == MH_OK) {
                    MH_QueueEnableHook(pSetCursorPos);
                }
            }
            dummySwapChain->Release(); dummyDevice->Release(); dummyContext->Release();
            d3d11Success = true;
        }

        if (createdDummyHwnd && hwnd) {
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }

        if (!d3d11Success) {
            return false;
        }

        ID3D12Device* pDummyD12Device = nullptr;
        if (SUCCEEDED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), (void**)&pDummyD12Device))) {
            D3D12_COMMAND_QUEUE_DESC queueDesc = {};
            queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            ID3D12CommandQueue* pDummyQueue = nullptr;
            if (SUCCEEDED(pDummyD12Device->CreateCommandQueue(&queueDesc, __uuidof(ID3D12CommandQueue), (void**)&pDummyQueue))) {
                void** pVTable12 = *reinterpret_cast<void***>(pDummyQueue);
                if (MH_CreateHook(pVTable12[10], (LPVOID)hkExecuteCommandLists, (void**)&oExecuteCommandLists) == MH_OK) {
                    MH_QueueEnableHook(pVTable12[10]);
                }
                pDummyQueue->Release();
            }
            pDummyD12Device->Release();
        }

        // [性能关键修复] 一次性批量激活所有钩子 (仅执行单次全进程线程挂起与恢复)
        // 彻底杜绝原本逐个调用 MH_EnableHook 触发 10 次重复线程遍历/冻结所引起的 1~4 秒主菜单严重卡顿
        MH_ApplyQueued();

        s_initialized.store(true);
        return true;
    }
}
