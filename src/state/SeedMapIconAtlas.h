#pragma once

#include "worldgen/WorldGenTypes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace ChiyanMap::SeedMapIcons {

inline constexpr std::uint8_t kIconSizePixels = 20;

struct IconColor final {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
};

struct IconPixel final {
    std::uint8_t x = 0;
    std::uint8_t y = 0;
    std::uint8_t width = 0;
    std::uint8_t height = 0;
    IconColor    color{};
};

struct IconDefinition final {
    WorldGen::LayerId            layer{};
    std::span<const IconPixel>   pixels;
    IconColor                    accent{};
    // Figma review card containing the authoritative 20px map-icon grid.
    // The in-game rectangles are a direct 1:1 transcription of that grid.
    std::string_view             figmaCardId;
};

namespace Detail {

inline constexpr IconColor kOutline{15, 18, 20};
inline constexpr IconColor kStone{99, 110, 112};
inline constexpr IconColor kDarkOak{54, 40, 29};
inline constexpr IconColor kWood{124, 74, 38};
inline constexpr IconColor kSand{209, 166, 84};
inline constexpr IconColor kTerracotta{210, 92, 33};
inline constexpr IconColor kMoss{76, 119, 70};
inline constexpr IconColor kPrismarine{56, 163, 148};
inline constexpr IconColor kWater{33, 92, 135};
inline constexpr IconColor kCopper{195, 119, 57};
inline constexpr IconColor kSculk{26, 153, 153};
inline constexpr IconColor kObsidian{57, 42, 76};
inline constexpr IconColor kNetherrack{136, 50, 43};
inline constexpr IconColor kNetherBrick{88, 35, 36};
inline constexpr IconColor kPurpur{174, 115, 191};
inline constexpr IconColor kEndStone{219, 216, 151};
inline constexpr IconColor kGold{232, 177, 61};
inline constexpr IconColor kSnow{212, 232, 240};
inline constexpr IconColor kCherry{235, 135, 173};
inline constexpr IconColor kBadlandsTerracotta{184, 75, 43};
inline constexpr IconColor kIce{123, 210, 235};
inline constexpr IconColor kMangrove{54, 128, 91};
inline constexpr IconColor kPale{174, 192, 160};
inline constexpr IconColor kSulfur{200, 210, 50};
inline constexpr IconColor kDappled{85, 140, 60};
inline constexpr IconColor kSoul{93, 137, 158};
inline constexpr IconColor kCrimson{177, 55, 70};
inline constexpr IconColor kWarped{46, 157, 150};
inline constexpr IconColor kBasalt{92, 88, 94};
inline constexpr IconColor kSlimeDark{45, 122, 54};
inline constexpr IconColor kSlime{105, 214, 99};
inline constexpr IconColor kSlimeHighlight{170, 238, 129};

inline constexpr IconPixel kVillage[] = {
    {2, 14, 16, 3, kStone}, {4, 7, 12, 7, kDarkOak}, {2, 5, 16, 2, kTerracotta},
    {4, 3, 12, 2, kTerracotta}, {6, 1, 8, 2, kTerracotta}, {4, 10, 2, 4, kWood}, {14, 10, 2, 4, kWood},
};
inline constexpr IconPixel kOutpost[] = {
    {4, 16, 12, 3, kStone}, {6, 7, 8, 9, kDarkOak}, {3, 5, 14, 2, kDarkOak},
    {5, 3, 10, 2, kDarkOak}, {7, 1, 6, 2, kDarkOak}, {4, 8, 12, 2, kWood}, {12, 7, 2, 5, kTerracotta},
};
inline constexpr IconPixel kDesertPyramid[] = {
    {1, 15, 18, 3, kSand}, {3, 10, 14, 5, kSand}, {5, 7, 10, 3, kSand}, {7, 5, 6, 2, kSand},
    {1, 4, 3, 11, kSand}, {16, 4, 3, 11, kSand}, {1, 7, 3, 2, kTerracotta}, {16, 7, 3, 2, kTerracotta},
    {8, 12, 4, 3, kTerracotta},
};
inline constexpr IconPixel kJungleTemple[] = {
    {2, 15, 16, 3, kMoss}, {4, 11, 12, 4, kStone}, {6, 8, 8, 3, kMoss}, {8, 5, 4, 3, kStone},
    {5, 12, 2, 2, kDarkOak}, {13, 12, 2, 2, kDarkOak}, {3, 9, 2, 3, kMoss}, {15, 9, 2, 3, kMoss},
};
inline constexpr IconPixel kSwampHut[] = {
    {3, 15, 14, 2, kWater}, {5, 8, 10, 7, kDarkOak}, {3, 6, 14, 2, kDarkOak}, {5, 4, 10, 2, kDarkOak},
    {6, 15, 2, 4, kWood}, {12, 15, 2, 4, kWood}, {13, 3, 2, 3, kStone},
};
inline constexpr IconPixel kIgloo[] = {
    {3, 15, 14, 3, kSnow}, {4, 11, 12, 4, kSnow}, {6, 7, 8, 4, kSnow}, {8, 4, 4, 3, kSnow},
    {13, 13, 4, 2, kStone}, {14, 15, 3, 2, kSnow},
};
inline constexpr IconPixel kMansion[] = {
    {1, 15, 18, 3, kStone}, {3, 8, 14, 7, kDarkOak}, {1, 6, 18, 2, kDarkOak}, {4, 4, 12, 2, kDarkOak},
    {6, 2, 8, 2, kDarkOak}, {4, 10, 3, 2, kWood}, {9, 10, 2, 5, kWood}, {13, 10, 3, 2, kWood},
};
inline constexpr IconPixel kMonument[] = {
    {1, 16, 18, 2, kWater}, {2, 13, 16, 3, kPrismarine}, {4, 10, 3, 3, kPrismarine}, {13, 10, 3, 3, kPrismarine},
    {8, 4, 4, 9, kPrismarine}, {7, 7, 6, 2, kPrismarine}, {9, 5, 2, 2, kSnow},
};
inline constexpr IconPixel kOceanRuin[] = {
    {4, 14, 12, 2, kWater}, {6, 12, 8, 2, kStone}, {7, 9, 2, 3, kStone}, {11, 8, 2, 4, kStone},
    {13, 11, 2, 1, kMoss},
};
inline constexpr IconPixel kShipwreck[] = {
    {1, 16, 18, 2, kWater}, {3, 13, 13, 3, kWood}, {5, 11, 11, 2, kWood}, {9, 3, 2, 10, kDarkOak},
    {11, 5, 5, 4, kSnow}, {3, 15, 2, 2, kDarkOak}, {15, 14, 2, 2, kDarkOak},
};
inline constexpr IconPixel kTreasure[] = {
    {4, 14, 12, 2, kSand}, {6, 11, 8, 3, kWood}, {6, 10, 8, 1, kGold}, {9, 12, 2, 2, kGold},
};
inline constexpr IconPixel kRuinedPortal[] = {
    {4, 14, 12, 2, kStone}, {6, 8, 2, 6, kObsidian}, {12, 6, 2, 8, kObsidian}, {8, 6, 4, 2, kObsidian},
    {8, 10, 4, 4, kNetherrack},
};
inline constexpr IconPixel kRuinedPortalNether[] = {
    {4, 14, 12, 2, kNetherrack}, {6, 9, 2, 5, kObsidian}, {12, 7, 2, 7, kObsidian}, {8, 7, 4, 2, kObsidian},
    {8, 10, 4, 3, kNetherrack},
};
inline constexpr IconPixel kStronghold[] = {
    {4, 8, 12, 4, kSnow}, {6, 6, 8, 8, kSnow}, {7, 7, 6, 6, kSculk}, {8, 8, 4, 4, kOutline},
};
inline constexpr IconPixel kMineshaft[] = {
    {4, 14, 12, 2, kWood}, {6, 9, 2, 5, kWood}, {12, 9, 2, 5, kWood}, {8, 10, 4, 2, kWood},
    {9, 7, 2, 3, kWood},
};
inline constexpr IconPixel kAncientCity[] = {
    {1, 16, 18, 2, kStone}, {3, 13, 14, 3, kStone}, {4, 7, 3, 6, kStone}, {13, 7, 3, 6, kStone},
    {7, 4, 6, 3, kStone}, {8, 7, 4, 6, kSculk}, {9, 9, 2, 2, kOutline},
};
inline constexpr IconPixel kTrailRuins[] = {
    {1, 15, 18, 3, kSand}, {3, 12, 5, 3, kTerracotta}, {12, 11, 5, 4, kTerracotta}, {8, 8, 3, 7, kMoss},
    {5, 10, 2, 2, kStone}, {14, 8, 2, 3, kStone}, {9, 6, 2, 2, kTerracotta},
};
inline constexpr IconPixel kTrialChambers[] = {
    {2, 16, 16, 2, kStone}, {4, 8, 3, 8, kStone}, {13, 8, 3, 8, kStone}, {7, 5, 6, 3, kStone},
    {8, 8, 4, 8, kCopper}, {9, 10, 2, 3, kOutline}, {3, 13, 2, 2, kCopper}, {15, 13, 2, 2, kCopper},
};
inline constexpr IconPixel kFortress[] = {
    {4, 14, 12, 2, kNetherBrick}, {6, 11, 8, 3, kNetherBrick}, {7, 8, 2, 3, kNetherBrick}, {11, 8, 2, 3, kNetherBrick},
    {9, 10, 2, 1, kNetherBrick},
};
inline constexpr IconPixel kBastion[] = {
    {4, 14, 12, 2, kNetherrack}, {6, 12, 8, 2, kStone}, {8, 9, 3, 3, kStone}, {11, 10, 3, 2, kStone},
    {10, 8, 2, 4, kStone}, {10, 9, 1, 1, kGold},
};
inline constexpr IconPixel kEndCity[] = {
    {4, 14, 12, 2, kEndStone}, {7, 11, 6, 3, kPurpur}, {9, 7, 2, 4, kPurpur}, {8, 9, 4, 2, kPurpur},
    {8, 12, 1, 2, kPurpur}, {12, 12, 1, 2, kPurpur},
};
inline constexpr IconPixel kMushroomFields[] = {
    {2, 15, 16, 2, kMoss}, {5, 10, 10, 5, kSnow}, {3, 7, 14, 3, kTerracotta},
    {5, 5, 10, 2, kTerracotta}, {6, 10, 2, 2, kSnow}, {12, 10, 2, 2, kSnow},
};
inline constexpr IconPixel kCherryGrove[] = {
    {2, 16, 16, 2, kMoss}, {9, 8, 2, 8, kDarkOak}, {4, 5, 12, 4, kCherry},
    {2, 8, 16, 3, kCherry}, {5, 3, 10, 2, kCherry}, {8, 1, 4, 2, kCherry},
};
inline constexpr IconPixel kBadlands[] = {
    {2, 15, 16, 3, kBadlandsTerracotta}, {4, 11, 12, 4, kTerracotta}, {6, 8, 8, 3, kBadlandsTerracotta},
    {8, 5, 4, 3, kSand}, {5, 13, 10, 1, kSand},
};
inline constexpr IconPixel kIceSpikes[] = {
    {2, 16, 16, 2, kSnow}, {4, 11, 3, 5, kIce}, {8, 5, 4, 11, kIce},
    {13, 9, 3, 7, kIce}, {9, 2, 2, 3, kSnow},
};
inline constexpr IconPixel kMangroveSwamp[] = {
    {1, 16, 18, 2, kWater}, {5, 10, 2, 6, kWood}, {13, 10, 2, 6, kWood},
    {8, 7, 4, 9, kWood}, {3, 5, 14, 4, kMangrove}, {5, 3, 10, 2, kMangrove},
    {7, 1, 6, 2, kMangrove},
};
inline constexpr IconPixel kPaleGarden[] = {
    {2, 16, 16, 2, kPale}, {9, 8, 2, 8, kDarkOak}, {3, 6, 14, 3, kPale},
    {5, 4, 10, 2, kPale}, {8, 1, 4, 3, kSnow}, {4, 11, 3, 2, kPale}, {13, 11, 3, 2, kPale},
};
inline constexpr IconPixel kEndGateway[] = {
    {7, 14, 6, 2, kEndStone}, {5, 11, 10, 3, kEndStone}, {7, 8, 6, 3, kPurpur},
    {9, 4, 2, 4, kPurpur}, {8, 2, 4, 2, kSnow},
};
inline constexpr IconPixel kSoulSandValley[] = {
    {1, 16, 18, 2, kSand}, {3, 12, 14, 4, kSoul}, {5, 9, 10, 3, kSand},
    {7, 5, 6, 4, kSoul}, {9, 2, 2, 3, kSnow},
};
inline constexpr IconPixel kCrimsonForest[] = {
    {2, 16, 16, 2, kNetherrack}, {9, 8, 2, 8, kDarkOak}, {3, 6, 14, 3, kCrimson},
    {5, 4, 10, 2, kCrimson}, {8, 1, 4, 3, kCrimson},
};
inline constexpr IconPixel kWarpedForest[] = {
    {2, 16, 16, 2, kNetherrack}, {9, 8, 2, 8, kDarkOak}, {3, 6, 14, 3, kWarped},
    {5, 4, 10, 2, kWarped}, {8, 1, 4, 3, kWarped},
};
inline constexpr IconPixel kBasaltDeltas[] = {
    {1, 16, 18, 2, kNetherrack}, {3, 13, 14, 3, kBasalt}, {5, 9, 10, 4, kBasalt},
    {7, 5, 6, 4, kBasalt}, {9, 2, 2, 3, kNetherrack},
};
inline constexpr IconPixel kEndSmallIslands[] = {
    {3, 14, 5, 2, kEndStone}, {11, 12, 6, 2, kEndStone}, {7, 9, 4, 2, kEndStone},
    {8, 7, 2, 2, kPurpur},
};
inline constexpr IconPixel kEndMidlands[] = {
    {2, 15, 16, 3, kEndStone}, {4, 11, 12, 4, kEndStone}, {7, 7, 6, 4, kEndStone},
    {9, 4, 2, 3, kPurpur},
};
inline constexpr IconPixel kEndHighlands[] = {
    {1, 16, 18, 2, kEndStone}, {3, 12, 14, 4, kEndStone}, {5, 8, 10, 4, kEndStone},
    {7, 4, 6, 4, kEndStone}, {9, 1, 2, 3, kPurpur},
};
inline constexpr IconPixel kEndBarrens[] = {
    {2, 16, 16, 2, kEndStone}, {4, 13, 12, 3, kEndStone}, {7, 11, 6, 2, kEndStone},
    {9, 8, 2, 3, kPurpur},
};
inline constexpr IconPixel kSlimeChunks[] = {
    {4, 4, 12, 12, kSlimeDark}, {5, 3, 10, 1, kSlimeHighlight}, {3, 5, 1, 10, kSlimeHighlight},
    {5, 5, 10, 9, kSlime}, {6, 6, 2, 2, kSlimeHighlight}, {12, 6, 2, 2, kSlimeHighlight},
    {7, 9, 2, 2, kOutline}, {11, 9, 2, 2, kOutline}, {7, 13, 6, 1, kSlimeDark},
};
inline constexpr IconPixel kAbandonedCamp[] = {
    {2, 16, 16, 2, kDarkOak}, {4, 13, 12, 3, kWood}, {6, 9, 8, 4, kWood},
    {8, 5, 4, 4, kWood}, {9, 2, 2, 3, kDarkOak}, {9, 13, 2, 3, kGold},
    {8, 11, 4, 2, kTerracotta},
};
inline constexpr IconPixel kSulfurCaves[] = {
    {2, 16, 16, 2, kStone}, {4, 12, 12, 4, kStone}, {7, 8, 6, 4, kSulfur},
    {9, 4, 2, 4, kSulfur}, {6, 13, 3, 3, kSulfur}, {12, 11, 3, 4, kSulfur},
};
inline constexpr IconPixel kDappledForest[] = {
    {3, 14, 14, 3, kDappled}, {5, 9, 10, 5, kDappled}, {7, 5, 6, 4, kDappled},
    {9, 2, 2, 3, kDappled}, {9, 14, 2, 4, kWood}, {7, 10, 2, 2, kSand},
    {11, 8, 2, 2, kSand},
};

inline constexpr std::array kIcons{
    IconDefinition{WorldGen::LayerId::Village, kVillage, kTerracotta, "44:608"},
    IconDefinition{WorldGen::LayerId::PillagerOutpost, kOutpost, kDarkOak, "44:609"},
    IconDefinition{WorldGen::LayerId::DesertPyramid, kDesertPyramid, kSand, "44:610"},
    IconDefinition{WorldGen::LayerId::JungleTemple, kJungleTemple, kMoss, "44:611"},
    IconDefinition{WorldGen::LayerId::SwampHut, kSwampHut, kMoss, "44:612"},
    IconDefinition{WorldGen::LayerId::Igloo, kIgloo, kSnow, "44:613"},
    IconDefinition{WorldGen::LayerId::WoodlandMansion, kMansion, kDarkOak, "44:614"},
    IconDefinition{WorldGen::LayerId::OceanMonument, kMonument, kPrismarine, "44:615"},
    IconDefinition{WorldGen::LayerId::OceanRuin, kOceanRuin, kPrismarine, "46:1024"},
    IconDefinition{WorldGen::LayerId::Shipwreck, kShipwreck, kWood, "44:616"},
    IconDefinition{WorldGen::LayerId::BuriedTreasure, kTreasure, kGold, "46:1025"},
    IconDefinition{WorldGen::LayerId::RuinedPortal, kRuinedPortal, kObsidian, "46:1026"},
    IconDefinition{WorldGen::LayerId::RuinedPortalNether, kRuinedPortalNether, kNetherrack, "46:1027"},
    IconDefinition{WorldGen::LayerId::Stronghold, kStronghold, kObsidian, "46:1028"},
    IconDefinition{WorldGen::LayerId::Mineshaft, kMineshaft, kWood, "46:1029"},
    IconDefinition{WorldGen::LayerId::AncientCity, kAncientCity, kSculk, "44:617"},
    IconDefinition{WorldGen::LayerId::TrailRuins, kTrailRuins, kTerracotta, "44:618"},
    IconDefinition{WorldGen::LayerId::TrialChambers, kTrialChambers, kCopper, "44:619"},
    IconDefinition{WorldGen::LayerId::AbandonedCamp, kAbandonedCamp, kWood, "51:111"},
    IconDefinition{WorldGen::LayerId::NetherFortress, kFortress, kNetherBrick, "46:1030"},
    IconDefinition{WorldGen::LayerId::BastionRemnant, kBastion, kGold, "46:1031"},
    IconDefinition{WorldGen::LayerId::EndCity, kEndCity, kPurpur, "46:1032"},
    IconDefinition{WorldGen::LayerId::EndGateway, kEndGateway, kPurpur, "46:1033"},
    IconDefinition{WorldGen::LayerId::MushroomFields, kMushroomFields, kTerracotta, "44:620"},
    IconDefinition{WorldGen::LayerId::CherryGrove, kCherryGrove, kCherry, "44:621"},
    IconDefinition{WorldGen::LayerId::Badlands, kBadlands, kBadlandsTerracotta, "44:622"},
    IconDefinition{WorldGen::LayerId::IceSpikes, kIceSpikes, kIce, "44:623"},
    IconDefinition{WorldGen::LayerId::MangroveSwamp, kMangroveSwamp, kMangrove, "44:624"},
    IconDefinition{WorldGen::LayerId::PaleGarden, kPaleGarden, kPale, "44:625"},
    IconDefinition{WorldGen::LayerId::SulfurCaves, kSulfurCaves, kSulfur, "51:112"},
    IconDefinition{WorldGen::LayerId::DappledForest, kDappledForest, kDappled, "51:113"},
    IconDefinition{WorldGen::LayerId::SoulSandValley, kSoulSandValley, kSoul, "50:41"},
    IconDefinition{WorldGen::LayerId::CrimsonForest, kCrimsonForest, kCrimson, "50:42"},
    IconDefinition{WorldGen::LayerId::WarpedForest, kWarpedForest, kWarped, "50:43"},
    IconDefinition{WorldGen::LayerId::BasaltDeltas, kBasaltDeltas, kBasalt, "50:44"},
    IconDefinition{WorldGen::LayerId::EndSmallIslands, kEndSmallIslands, kEndStone, "50:45"},
    IconDefinition{WorldGen::LayerId::EndMidlands, kEndMidlands, kEndStone, "50:46"},
    IconDefinition{WorldGen::LayerId::EndHighlands, kEndHighlands, kEndStone, "50:47"},
    IconDefinition{WorldGen::LayerId::EndBarrens, kEndBarrens, kEndStone, "50:48"},
    IconDefinition{WorldGen::LayerId::SlimeChunks, kSlimeChunks, kSlime, "51:110"},
};

} // namespace Detail

[[nodiscard]] inline const IconDefinition* FindIcon(WorldGen::LayerId layer) noexcept {
    for (const IconDefinition& icon : Detail::kIcons) {
        if (icon.layer == layer) return &icon;
    }
    return nullptr;
}

[[nodiscard]] inline bool ValidateAtlas() noexcept {
    for (std::size_t index = 0; index < Detail::kIcons.size(); ++index) {
        const IconDefinition& icon = Detail::kIcons[index];
        if (icon.pixels.empty() || icon.figmaCardId.empty()) return false;
        for (const IconPixel& pixel : icon.pixels) {
            if (pixel.width == 0 || pixel.height == 0 || pixel.x + pixel.width > kIconSizePixels
                || pixel.y + pixel.height > kIconSizePixels) {
                return false;
            }
        }
        for (std::size_t other = index + 1; other < Detail::kIcons.size(); ++other) {
            if (icon.layer == Detail::kIcons[other].layer || icon.pixels.data() == Detail::kIcons[other].pixels.data()) {
                return false;
            }
        }
    }
    return true;
}

} // namespace ChiyanMap::SeedMapIcons
