#pragma once
#include <ll/api/memory/Hook.h>
#include <mc/client/renderer/screen/MinecraftUIRenderContext.h>
#include <mc/client/game/ClientInstance.h>
#include <optional>

#include "mod/ChiyanMap.h"
#include "state/MapRenderState.h"
#include "hooks/PlayerHook.h"
#include "hooks/DX11Hook.h" 

LL_TYPE_INSTANCE_HOOK(
    UIRenderContextFlushTextHook,
    ll::memory::HookPriority::Normal,
    MinecraftUIRenderContext,
    &MinecraftUIRenderContext::$flushText,
    void,
    float deltaTime,
    std::optional<float> obfuscateSwitchTime
) {
    if (MapRenderState::g_isShuttingDown.load()) {
        origin(deltaTime, obfuscateSwitchTime);
        return;
    }
    origin(deltaTime, obfuscateSwitchTime);

    // 兜底机制：若模组启用阶段尚未完成挂钩，则在首次 UI 绘制时补齐挂钩
    static bool s_dxgiHooked = false;
    if (!s_dxgiHooked) { 
        if (DX11Hook::init()) {
            s_dxgiHooked = true; 
        }
    }

    if (!g_clientInstance || !g_hasPlayer) return;

    // 更新帧计数，协助 PlayerHook 进行平滑扫描
    int callIndex = ++MapRenderState::frameCallCount;
    if (callIndex < MapRenderState::lastFrameTotalCalls) return;
}