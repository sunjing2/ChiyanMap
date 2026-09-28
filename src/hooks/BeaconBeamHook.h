#pragma once

// ====================================================================
// [死亡信标光束模块]
// 目标: 在每个死亡点位置渲染红色原版信标光束 (Beacon Beam)。
//
// 实现原理 (全程复用原版游戏内置渲染器, 不自己画光柱):
//   1. 用游戏自己的构造函数 BeaconBlockActor(pos, true) 创建纯客户端
//      的假信标方块实体, 并手动填入一段红色 BeaconBeamSection;
//   2. 自行实现 IVanillaRenderBlockActorComponent (16 个虚函数),
//      把假方块实体包装成渲染队列可接受的"渲染组件";
//   3. Hook LevelRendererCamera::renderBlockEntities, 在透明层渲染
//      Pass 把组件追加进原版的 mRenderComponentRenderAlphaQueue,
//      原版随后会经由 BlockActorRenderDispatcher 调用 BeaconRenderer,
//      使用原版材质 / 原版贴图 / 原版着色器绘制整条红色光束。
//
// 该方案不修改世界、不放真方块、不发包, 纯客户端视觉效果。
// ====================================================================

#include <new>
#include <string>
#include <unordered_map>
#include <vector>

#include "ll/api/memory/Hook.h"

#include "state/DeathPointManager.h"
#include "state/MapRenderState.h"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/renderer/BaseActorRenderContext.h"
#include "mc/client/renderer/game/LevelRendererCamera.h"
#include "mc/deps/core/math/Color.h"
#include "mc/deps/core/math/Vec3.h"
#include "mc/world/actor/ActorTerrainInterlockData.h"
#include "mc/world/level/BlockPos.h"
#include "mc/world/level/BlockSource.h"
#include "mc/world/level/block/actor/BeaconBeamSection.h"
#include "mc/world/level/block/actor/BeaconBlockActor.h"
#include "mc/world/level/block/actor/BlockActorRendererId.h"
#include "mc/world/level/block/actor/BlockActorType.h"
#include "mc/world/level/block/actor/component/IVanillaRenderBlockActorComponent.h"
#include "mc/world/phys/AABB.h"

class ClientInstance;
extern ClientInstance* g_clientInstance;

namespace DeathBeams {

// 红色信标光束颜色 (鲜艳正红, 乘到原版白色光束贴图上)
static constexpr float kBeamRedR = 1.00f;
static constexpr float kBeamRedG = 0.08f;
static constexpr float kBeamRedB = 0.08f;

// --------------------------------------------------------------------
// IVanillaRenderBlockActorComponent 的本地实现:
// 渲染调度器通过该接口查询方块实体信息, 我们把它桥接到假 BeaconBlockActor
// --------------------------------------------------------------------
class FakeBeaconComponent final : public IVanillaRenderBlockActorComponent {
public:
    BlockActor*             mActor = nullptr;
    AABB                    mBox{};
    ActorTerrainInterlockData mInterlock{};

    void bind(BlockActor* actor, AABB const& box) {
        mActor = actor;
        mBox   = box;
    }

    BlockActor&       getBlockActor() override { return *mActor; }
    BlockActor const& getBlockActor() const override { return *mActor; }

    BlockPos const& getBlockActorPosition() const override { return mActor->getPosition(); }

    BlockActorType       getBlockActorType() const override { return BlockActorType::Beacon; }
    BlockActorRendererId getRendererId() const override { return BlockActorRendererId::Beacon; }

    // 永久渲染标记: 跳过常规区块生命周期限制
    bool isPermanentlyRendered() const override { return true; }
    bool isWithinRenderDistance(::Vec3 const& cameraPosition) const override { return true; }

    // 信标光束带半透明外层, 必须进入 Alpha 渲染队列
    bool hasAlphaLayer() const override { return true; }

    float getShadowRadius(::BlockSource& region) const override { return 0.0f; }
    bool  isInWorld() const override { return true; }

    IVanillaRenderBlockActorComponent* getCrackEntity(::BlockSource& region, ::BlockPos const& pos) override {
        return nullptr;
    }

    ::AABB const& getAABB() const override { return mBox; }
    void          setAABB(::AABB const& value) override { mBox = value; }

    ActorTerrainInterlockData&       getEntityTerrainInterlockData() override { return mInterlock; }
    ActorTerrainInterlockData const& getEntityTerrainInterlockDataConst() const override { return mInterlock; }

    void _resetAABB() override {}
};

struct FakeBeamEntry {
    BeaconBlockActor* actor = nullptr;
    FakeBeaconComponent component;
};

// --------------------------------------------------------------------
// 通过原版构造函数创建/销毁假信标方块实体
// --------------------------------------------------------------------
inline BeaconBlockActor* CreateFakeBeacon(int x, int y, int z, int height) {
    void* mem = ::operator new(sizeof(BeaconBlockActor), std::align_val_t{alignof(BeaconBlockActor)});
    auto* actor = new (mem) BeaconBlockActor(BlockPos{x, y, z}, true);

    // 填入唯一一段红色光束段 (从死亡点位置一直延伸到维度顶部)
    BeaconBeamSection section;
    section.mHeight = height;
    section.mColor  = mce::Color(kBeamRedR, kBeamRedG, kBeamRedB, 1.0f);

    actor->mBeamSections.get().clear();
    actor->mBeamSections.get().push_back(section);
    return actor;
}

inline void DestroyFakeBeacon(BeaconBlockActor* actor) {
    if (!actor) return;
    actor->~BeaconBlockActor();
    ::operator delete(actor, std::align_val_t{alignof(BeaconBlockActor)});
}

// --------------------------------------------------------------------
// 假信标集合: 与当前维度的死亡点列表保持同步
// --------------------------------------------------------------------
inline std::unordered_map<std::string, FakeBeamEntry>& Entries() {
    static std::unordered_map<std::string, FakeBeamEntry> sEntries;
    return sEntries;
}

inline int& TrackedDimension() {
    static int sDim = -9999;
    return sDim;
}

inline void ClearAll() {
    auto& entries = Entries();
    for (auto& kv : entries) {
        DestroyFakeBeacon(kv.second.actor);
    }
    entries.clear();
    TrackedDimension() = -9999;
}

inline void Sync(BlockSource& region, int dimensionId) {
    if (dimensionId != TrackedDimension()) {
        ClearAll();
        TrackedDimension() = dimensionId;
    }

    const short maxY = region.getMaxHeight();

    std::vector<DeathPoint> points;
    {
        std::lock_guard<std::mutex> lock(DeathPointManager::g_deathMutex);
        points = DeathPointManager::g_deathPoints;
    }

    auto& entries = Entries();

    // 移除已删除/失效的假信标
    for (auto it = entries.begin(); it != entries.end();) {
        bool keep = false;
        for (const auto& p : points) {
            if (p.id == it->first && p.dimensionId == dimensionId) {
                keep = true;
                break;
            }
        }
        if (!keep) {
            DestroyFakeBeacon(it->second.actor);
            it = entries.erase(it);
        } else {
            ++it;
        }
    }

    // 为新死亡点创建假信标
    for (const auto& p : points) {
        if (p.dimensionId != dimensionId) continue;
        if (entries.find(p.id) != entries.end()) continue;

        int height = static_cast<int>(maxY) - p.y;
        if (height < 1) height = 1;
        if (height > 2048) height = 2048;

        BeaconBlockActor* actor = CreateFakeBeacon(p.x, p.y, p.z, height);

        FakeBeamEntry entry;
        entry.actor = actor;
        entry.component.bind(
            actor,
            AABB{
                Vec3{static_cast<float>(p.x), static_cast<float>(p.y), static_cast<float>(p.z)},
                Vec3{static_cast<float>(p.x) + 1.0f,
                     static_cast<float>(p.y) + static_cast<float>(height),
                     static_cast<float>(p.z) + 1.0f}
            }
        );
        entries.emplace(p.id, std::move(entry));
    }
}

} // namespace DeathBeams

// ====================================================================
// Hook: LevelRendererCamera::renderBlockEntities
// 信标属于"带 Alpha 层"的方块实体, 原版只在 renderAlphaLayer == true
// 的 Pass 通过 mRenderComponentRenderAlphaQueue 渲染。我们在该 Pass
// 把假信标组件追加进队列, 后续完全由原版 BeaconRenderer 绘制。
// ====================================================================
LL_TYPE_INSTANCE_HOOK(
    DeathBeamRenderBlockEntitiesHook,
    ll::memory::HookPriority::Normal,
    LevelRendererCamera,
    &LevelRendererCamera::$renderBlockEntities,
    void,
    ::BaseActorRenderContext& renderContext,
    bool                      renderAlphaLayer
) {
    if (renderAlphaLayer && !MapRenderState::g_isShuttingDown.load() && g_clientInstance) {
        BlockSource* region = g_clientInstance->getRegion();
        if (region) {
            const int dimensionId = static_cast<int>(region->getDimensionId());
            DeathBeams::Sync(*region, dimensionId);

            auto& alphaQueue = this->mRenderComponentRenderAlphaQueue.get();
            for (auto& kv : DeathBeams::Entries()) {
                BlockPos const& bp = kv.second.component.getBlockActorPosition();
                // 视锥剔除: 只把视锥体附近的光束提交给原版 (与原版队列筛选行为一致)
                Vec3 base{static_cast<float>(bp.x) + 0.5f, static_cast<float>(bp.y), static_cast<float>(bp.z) + 0.5f};
                if (this->cullerIsVisible(base, 4.0f)) {
                    alphaQueue.emplace_back(&kv.second.component);
                }
            }
        }
    }

    origin(renderContext, renderAlphaLayer);
}

// 卸载/关档时释放全部假信标对象
inline void ShutdownAllDeathBeams() { DeathBeams::ClearAll(); }
