#include "state/MapRenderState.h"
#include <windows.h>
#include <imgui.h>
#include <algorithm>

namespace MapRenderState {
    float CalculateOptimalUIScale(float displayH) {
        if (displayH <= 0.0f) {
            if (ImGui::GetCurrentContext()) {
                float imguiH = ImGui::GetIO().DisplaySize.y;
                if (imguiH > 100.0f) {
                    displayH = imguiH;
                }
            }
        }
        if (displayH <= 0.0f) {
            displayH = (float)GetSystemMetrics(SM_CYSCREEN);
        }
        if (displayH <= 0.0f) {
            displayH = 1080.0f;
        }

        if (displayH >= 2800.0f) {
            return 2.50f; // 5K (2880p) 及以上
        } else if (displayH >= 2000.0f) {
            return 2.00f; // 4K / UHD (2160p)
        } else if (displayH >= 1700.0f) {
            return 1.65f; // 3K (1800p)
        } else if (displayH >= 1500.0f) {
            return 1.50f; // 16:10 QHD+ (1600p)
        } else if (displayH >= 1300.0f) {
            return 1.25f; // 2K / QHD (1440p)
        } else if (displayH >= 1150.0f) {
            return 1.10f; // 1200p
        } else {
            return 1.00f; // 1080p 及以下
        }
    }

    float GetOptimalUIScale(float displayH) {
        return CalculateOptimalUIScale(displayH);
    }
}
