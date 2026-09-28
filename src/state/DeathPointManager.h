#pragma once

#include <mutex>
#include <string>
#include <vector>

struct DeathPoint {
    std::string id;
    int x = 0;
    int y = 0;
    int z = 0;
    int dimensionId = 0;
    long long timestamp = 0;
    bool converted = false;
    std::string convertedWaypointId; // 转换生成的路径点 ID（用于删除后恢复按钮）
};

namespace DeathPointManager {
    extern std::vector<DeathPoint> g_deathPoints;
    extern std::mutex g_deathMutex;

    void Init();
    void SwitchWorld(const std::string& worldId);
    void AddDeathPoint(int x, int y, int z, int dimensionId);
    void RemoveDeathPoint(const std::string& id);
    void SetConverted(const std::string& id, bool converted = true, const std::string& waypointId = "");
    void SaveDeathPoints();
    void LoadDeathPoints();
}
