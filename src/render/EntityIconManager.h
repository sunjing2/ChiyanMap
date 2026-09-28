#pragma once
#include <d3d11.h>
#include <wincodec.h>
#include <unordered_map>
#include <string>
#include <filesystem>
#include <vector>
#include <mutex>
#include <windows.h>
#include <algorithm>
#include <cctype>
#include "state/MapRenderState.h"

namespace EntityIconManager {

    inline std::filesystem::path GetHeadsDirectory() {
        static std::filesystem::path s_headsDir;
        static bool s_resolved = false;
        if (s_resolved) return s_headsDir;

        // 1. 优先通过模块句柄获取 ChiyanMap.dll 所在的绝对路径下的 heads 目录
        HMODULE hMod = NULL;
        if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCWSTR)&GetHeadsDirectory, &hMod)) {
            wchar_t modPath[MAX_PATH];
            if (GetModuleFileNameW(hMod, modPath, MAX_PATH)) {
                std::filesystem::path dllDir = std::filesystem::path(modPath).parent_path();
                auto headsDir = dllDir / "heads";
                std::error_code ec;
                if (std::filesystem::exists(headsDir, ec) && std::filesystem::is_directory(headsDir, ec)) {
                    s_headsDir = headsDir;
                    s_resolved = true;
                    return s_headsDir;
                }
            }
        }

        // 2. 常见 Fallbacks
        const char* fallbacks[] = {
            "mods/ChiyanMap/heads",
            "plugins/ChiyanMap/heads",
            "bin/ChiyanMap/heads",
            "heads"
        };
        for (const char* fb : fallbacks) {
            std::error_code ec;
            if (std::filesystem::exists(fb, ec) && std::filesystem::is_directory(fb, ec)) {
                s_headsDir = std::filesystem::path(fb);
                s_resolved = true;
                return s_headsDir;
            }
        }

        s_resolved = true;
        return "";
    }

    inline const std::unordered_map<std::string, std::string>& GetEntityIconMap() {
        static std::unordered_map<std::string, std::string> s_map;
        static bool s_inited = false;
        if (!s_inited) {
            static const std::pair<const char*, const char*> rawPairs[] = {
                // 敌对生物
                {"minecraft:zombie", "ZombieFace.png"},
                {"minecraft:skeleton", "SkeletonFace.png"},
                {"minecraft:creeper", "CreeperFace.png"},
                {"minecraft:spider", "SpiderFace.png"},
                {"minecraft:cave_spider", "CaveSpiderFace.png"},
                {"minecraft:enderman", "EndermanFace.png"},
                {"minecraft:blaze", "BlazeFace.png"},
                {"minecraft:ghast", "GhastFace.png"},
                {"minecraft:slime", "SlimeFace.png"},
                {"minecraft:magma_cube", "MagmaCubeFace.png"},
                {"minecraft:witch", "WitchFace.png"},
                {"minecraft:guardian", "GuardianFace.png"},
                {"minecraft:elder_guardian", "ElderGuardianFace.png"},
                {"minecraft:silverfish", "SilverfishFace.png"},
                {"minecraft:endermite", "EndermiteFace.png"},
                {"minecraft:shulker", "ShulkerFace.png"},
                {"minecraft:phantom", "PhantomFace.png"},
                {"minecraft:drowned", "DrownedFace.png"},
                {"minecraft:husk", "HuskFace.png"},
                {"minecraft:stray", "StrayFace.png"},
                {"minecraft:wither_skeleton", "WitherSkeletonFace.png"},
                {"minecraft:zombie_villager", "ZombieVillagerFace.png"},
                {"minecraft:zombified_piglin", "ZombiePigmanFace.png"},
                {"minecraft:zombie_pigman", "ZombiePigmanFace.png"},
                {"minecraft:piglin", "PiglinFace.png"},
                {"minecraft:piglin_brute", "PiglinFace.png"},
                {"minecraft:hoglin", "HoglinFace.png"},
                {"minecraft:zoglin", "ZoglinFace.png"},
                {"minecraft:ravager", "RavagerFace.png"},
                {"minecraft:wither", "WitherFace.png"},
                {"minecraft:ender_dragon", "EnderdragonFace.png"},
                {"minecraft:vex", "VexFace.png"},
                {"minecraft:evoker", "EvokerFace.png"},
                {"minecraft:vindicator", "VindicatorFace.png"},
                {"minecraft:pillager", "PillagerFace.png"},
                {"minecraft:warden", "WardenFace.png"},
                {"minecraft:bogged", "BoggedPotatoFace.png"},
                {"minecraft:breeze", "BreezeFace.png"},
                {"minecraft:happy_ghast", "HappyGhastFace.png"},
                {"minecraft:creaking", "CreakingFace.png"},
                {"minecraft:illusioner", "IllusionerFace.png"},
                // 被动与中立生物
                {"minecraft:cow", "CowFace.png"},
                {"minecraft:pig", "PigFace.png"},
                {"minecraft:sheep", "SheepFace.png"},
                {"minecraft:chicken", "ChickenFace.png"},
                {"minecraft:rabbit", "RabbitFace.png"},
                {"minecraft:wolf", "WolfFace.png"},
                {"minecraft:fox", "FoxFace.png"},
                {"minecraft:cat", "CatFace.png"},
                {"minecraft:ocelot", "OcelotFace.png"},
                {"minecraft:horse", "HorseFace.png"},
                {"minecraft:donkey", "DonkeyFace.png"},
                {"minecraft:mule", "MuleFace.png"},
                {"minecraft:skeleton_horse", "SkeletonHorseFace.png"},
                {"minecraft:zombie_horse", "ZombieHorseFace.png"},
                {"minecraft:bee", "BeeFace.png"},
                {"minecraft:dolphin", "DolphinFace.png"},
                {"minecraft:squid", "SquidFace.png"},
                {"minecraft:glow_squid", "GlowSquidFace.png"},
                {"minecraft:turtle", "TurtleFace.png"},
                {"minecraft:panda", "PandaFace.png"},
                {"minecraft:polar_bear", "PolarBearFace.png"},
                {"minecraft:goat", "GoatFace.png"},
                {"minecraft:frog", "FrogFace.png"},
                {"minecraft:tadpole", "TadpoleFace.png"},
                {"minecraft:parrot", "ParrotFace.png"},
                {"minecraft:llama", "LlamaFace.png"},
                {"minecraft:trader_llama", "TraderLlamaFace.png"},
                {"minecraft:wandering_trader", "WanderingTraderFace.png"},
                {"minecraft:strider", "StriderFace.png"},
                {"minecraft:iron_golem", "IronGolemFace.png"},
                {"minecraft:snow_golem", "SnowGolemFace.png"},
                {"minecraft:villager", "VillagerFace.png"},
                {"minecraft:villager_v2", "VillagerFace.png"},
                {"villager", "VillagerFace.png"},
                {"minecraft:mooshroom", "MooshroomFace.png"},
                {"minecraft:sniffer", "SnifferFace.png"},
                {"minecraft:camel", "CamelFace.png"},
                {"minecraft:armadillo", "ArmadilloFace.png"},
                {"minecraft:armor_stand", "ArmorStandFace.png"},
                {"minecraft:allay", "AllayFace.png"},
                {"minecraft:axolotl", "AxolotlFace.png"},
                {"minecraft:bat", "BatFace.png"},
                {"minecraft:pufferfish", "PufferfishFace.png"},
                // 村民职业 & 扩展
                {"minecraft:armorer", "ArmorerFace.png"},
                {"minecraft:barnacle", "BarnacleFace.png"},
                {"minecraft:butcher", "ButcherFace.png"},
                {"minecraft:cartographer", "CartographerFace.png"},
                {"minecraft:cleric", "ClericFace.png"},
                {"minecraft:cluckshroom", "CluckshroomFace.png"},
                {"minecraft:copper_golem", "CopperGolemFace.png"},
                {"minecraft:farmer", "FarmerFace.png"},
                {"minecraft:firefly", "FireflyFace.png"},
                {"minecraft:fisherman", "FishermanFace.png"},
                {"minecraft:fletcher", "FletcherFace.png"},
                {"minecraft:herobrine", "HerobrineFace.png"},
                {"minecraft:iceologer", "IceologerFace.png"},
                {"minecraft:leatherworker", "LeatherworkerFace.png"},
                {"minecraft:librarian", "LibrarianFace.png"},
                {"minecraft:moobloom", "MoobloomFace.png"},
                {"minecraft:moolip", "MoolipFace.png"},
                {"minecraft:ostrich", "OstrichFace.png"},
                {"minecraft:rascal", "RascalFace.png"},
                {"minecraft:shepherd", "ShepherdFace.png"},
                {"minecraft:vulture", "VultureFace.png"},
                {"minecraft:cod", "CodBody.png"},
                {"minecraft:salmon", "SalmonBody.png"},
                {"minecraft:tropical_fish", "TropicalFishFace.png"},
                {"minecraft:tropicalfish", "TropicalFishFace.png"},
            };

            for (const auto& item : rawPairs) {
                s_map[item.first] = item.second;
                // 同时插入去除 "minecraft:" 前缀后的纯名称
                std::string k = item.first;
                if (k.rfind("minecraft:", 0) == 0) {
                    s_map[k.substr(10)] = item.second;
                }
            }
            s_inited = true;
        }
        return s_map;
    }

    inline ID3D11ShaderResourceView* LoadTextureFromFileWIC(ID3D11Device* pDevice, const std::filesystem::path& filePath) {
        if (!pDevice || filePath.empty()) return nullptr;

        std::error_code ec;
        if (!std::filesystem::exists(filePath, ec)) return nullptr;

        HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        bool needUninit = SUCCEEDED(hrCo);

        IWICImagingFactory* pFactory = nullptr;
        HRESULT hr = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(&pFactory)
        );
        if (FAILED(hr) || !pFactory) {
            if (needUninit) CoUninitialize();
            return nullptr;
        }

        IWICBitmapDecoder* pDecoder = nullptr;
        hr = pFactory->CreateDecoderFromFilename(
            filePath.wstring().c_str(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnDemand,
            &pDecoder
        );
        if (FAILED(hr) || !pDecoder) {
            pFactory->Release();
            if (needUninit) CoUninitialize();
            return nullptr;
        }

        IWICBitmapFrameDecode* pFrame = nullptr;
        hr = pDecoder->GetFrame(0, &pFrame);
        if (FAILED(hr) || !pFrame) {
            pDecoder->Release();
            pFactory->Release();
            if (needUninit) CoUninitialize();
            return nullptr;
        }

        IWICFormatConverter* pConverter = nullptr;
        hr = pFactory->CreateFormatConverter(&pConverter);
        if (FAILED(hr) || !pConverter) {
            pFrame->Release();
            pDecoder->Release();
            pFactory->Release();
            if (needUninit) CoUninitialize();
            return nullptr;
        }

        hr = pConverter->Initialize(
            pFrame,
            GUID_WICPixelFormat32bppRGBA,
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeCustom
        );
        if (FAILED(hr)) {
            pConverter->Release();
            pFrame->Release();
            pDecoder->Release();
            pFactory->Release();
            if (needUninit) CoUninitialize();
            return nullptr;
        }

        UINT width = 0, height = 0;
        pConverter->GetSize(&width, &height);
        if (width == 0 || height == 0) {
            pConverter->Release();
            pFrame->Release();
            pDecoder->Release();
            pFactory->Release();
            if (needUninit) CoUninitialize();
            return nullptr;
        }

        UINT stride = width * 4;
        UINT bufferSize = stride * height;
        std::vector<BYTE> pixelBuffer(bufferSize);

        hr = pConverter->CopyPixels(nullptr, stride, bufferSize, pixelBuffer.data());
        pConverter->Release();
        pFrame->Release();
        pDecoder->Release();
        pFactory->Release();
        if (needUninit) CoUninitialize();

        if (FAILED(hr)) return nullptr;

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixelBuffer.data();
        initData.SysMemPitch = stride;

        ID3D11Texture2D* pTexture = nullptr;
        ID3D11ShaderResourceView* pSRV = nullptr;
        hr = pDevice->CreateTexture2D(&desc, &initData, &pTexture);
        if (SUCCEEDED(hr) && pTexture) {
            hr = pDevice->CreateShaderResourceView(pTexture, nullptr, &pSRV);
            pTexture->Release();
        }

        return pSRV;
    }

    inline std::string ToUpperCamelCase(std::string s) {
        if (s.empty()) return s;
        std::string res;
        bool capNext = true;
        for (char c : s) {
            if (c == '_' || c == ' ') {
                capNext = true;
            } else {
                if (capNext) {
                    res += (char)std::toupper((unsigned char)c);
                    capNext = false;
                } else {
                    res += (char)std::tolower((unsigned char)c);
                }
            }
        }
        return res;
    }

    // 面孔颜色方案（用于程序化 12x12 回退头像）
    struct MobFace {
        uint8_t skin[3];
        uint8_t eye[3];
        uint8_t eyePupil[3];
        uint8_t mouth[3];
        uint8_t hair[3];
        uint8_t accent[3];
    };

    inline const std::unordered_map<std::string, MobFace>& GetTypeToFaceMap() {
        static const std::unordered_map<std::string, MobFace> s_faces = {
            // 敌对生物
            {"minecraft:zombie",            {{80,130,50},{200,200,200},{0,0,0},{60,30,10},{50,70,30},{0,0,0}}},
            {"minecraft:skeleton",          {{190,190,190},{0,0,0},{0,0,0},{80,80,80},{190,190,190},{80,80,80}}},
            {"minecraft:creeper",           {{100,180,100},{255,255,255},{0,0,0},{0,0,0},{100,180,100},{0,0,0}}},
            {"minecraft:spider",            {{80,50,40},{200,50,50},{0,0,0},{30,20,15},{50,30,20},{0,0,0}}},
            {"minecraft:cave_spider",       {{70,100,120},{200,50,50},{0,0,0},{30,40,50},{50,70,90},{0,0,0}}},
            {"minecraft:enderman",          {{30,20,40},{200,50,200},{0,0,0},{10,10,20},{10,5,20},{200,50,200}}},
            {"minecraft:blaze",             {{220,180,50},{255,200,50},{0,0,0},{180,100,20},{200,150,30},{0,0,0}}},
            {"minecraft:ghast",             {{220,220,220},{200,50,50},{0,0,0},{150,150,150},{220,220,220},{0,0,0}}},
            {"minecraft:slime",             {{80,200,80},{255,255,255},{0,0,0},{40,100,40},{60,150,60},{0,0,0}}},
            {"minecraft:magma_cube",        {{200,120,40},{255,200,50},{0,0,0},{150,80,20},{180,100,30},{0,0,0}}},
            {"minecraft:witch",             {{80,60,50},{200,50,50},{0,0,0},{50,30,20},{20,20,30},{100,100,30}}},
            {"minecraft:guardian",          {{150,180,200},{255,255,200},{0,0,0},{100,120,140},{80,120,150},{200,200,0}}},
            {"minecraft:elder_guardian",    {{120,150,180},{255,255,200},{0,0,0},{80,100,120},{60,100,130},{200,200,0}}},
            {"minecraft:silverfish",        {{120,120,120},{0,0,0},{0,0,0},{80,80,80},{100,100,100},{0,0,0}}},
            {"minecraft:endermite",         {{60,40,50},{200,50,200},{0,0,0},{40,25,35},{40,25,35},{0,0,0}}},
            {"minecraft:shulker",           {{140,100,160},{200,200,200},{0,0,0},{100,70,120},{120,80,140},{0,0,0}}},
            {"minecraft:phantom",           {{60,80,100},{200,50,50},{0,0,0},{40,50,70},{40,60,80},{0,0,0}}},
            {"minecraft:drowned",           {{60,100,80},{150,200,150},{0,0,0},{30,60,40},{30,80,50},{0,150,150}}},
            {"minecraft:husk",              {{150,140,100},{200,200,150},{0,0,0},{100,90,60},{120,110,70},{0,0,0}}},
            {"minecraft:stray",             {{160,170,180},{200,100,200},{0,0,0},{100,110,120},{120,130,140},{150,150,255}}},
            {"minecraft:wither_skeleton",   {{40,40,40},{200,50,50},{0,0,0},{20,20,20},{20,20,20},{0,0,0}}},
            {"minecraft:zombie_villager",   {{80,130,50},{200,200,200},{0,0,0},{60,30,10},{50,70,30},{150,100,50}}},
            {"minecraft:zombified_piglin",  {{120,100,80},{200,50,50},{0,0,0},{80,60,40},{100,80,60},{255,150,50}}},
            {"minecraft:piglin",            {{200,140,100},{200,100,50},{0,0,0},{150,100,60},{180,120,80},{255,150,50}}},
            {"minecraft:piglin_brute",      {{200,130,90},{200,50,50},{0,0,0},{150,90,50},{180,110,70},{200,100,0}}},
            {"minecraft:hoglin",            {{150,100,80},{200,150,100},{0,0,0},{100,60,40},{120,80,60},{100,60,40}}},
            {"minecraft:ravager",           {{80,80,80},{200,50,50},{0,0,0},{50,50,50},{60,60,60},{0,0,0}}},
            {"minecraft:wither",            {{30,30,30},{200,50,50},{0,0,0},{15,15,15},{20,20,20},{0,0,0}}},
            {"minecraft:ender_dragon",      {{30,20,40},{200,50,200},{0,0,0},{10,10,20},{10,5,20},{150,0,200}}},
            {"minecraft:zoglin",            {{90,110,70},{200,50,50},{0,0,0},{60,80,40},{70,90,50},{100,60,40}}},
            {"minecraft:vex",               {{160,160,180},{200,50,50},{0,0,0},{120,120,140},{140,140,160},{0,0,0}}},
            {"minecraft:evoker",            {{140,120,100},{200,200,200},{0,0,0},{100,80,60},{120,100,80},{150,150,150}}},
            {"minecraft:vindicator",        {{140,120,100},{200,200,200},{0,0,0},{100,80,60},{120,100,80},{0,0,0}}},
            {"minecraft:pillager",          {{140,120,100},{200,200,200},{0,0,0},{100,80,60},{120,100,80},{80,80,80}}},
            {"minecraft:warden",            {{20,40,80},{200,200,255},{0,0,0},{10,20,50},{10,20,50},{0,200,255}}},
            {"minecraft:bogged",            {{100,120,80},{200,50,200},{0,0,0},{60,80,40},{80,100,60},{0,0,0}}},
            {"minecraft:breeze",            {{150,180,200},{255,255,255},{0,0,0},{100,130,150},{120,150,170},{200,220,255}}},
            {"minecraft:creaking",          {{100,80,60},{200,200,100},{0,0,0},{60,40,20},{80,60,40},{0,0,0}}},
            {"minecraft:happy_ghast",       {{255,200,200},{255,255,255},{0,0,0},{100,50,50},{255,150,150},{255,100,100}}},
            {"minecraft:illusioner",        {{140,120,100},{200,200,200},{0,0,0},{100,80,60},{120,100,80},{100,200,100}}},
            {"minecraft:armor_stand",       {{160,140,120},{0,0,0},{0,0,0},{100,90,80},{130,115,100},{0,0,0}}},
            // 友好/中立生物
            {"minecraft:cow",               {{150,100,80},{255,255,255},{0,0,0},{80,50,30},{120,80,60},{200,200,200}}},
            {"minecraft:pig",               {{200,150,150},{255,255,255},{0,0,0},{150,100,100},{180,130,130},{0,0,0}}},
            {"minecraft:sheep",             {{200,200,200},{255,255,255},{0,0,0},{150,150,150},{180,180,180},{0,0,0}}},
            {"minecraft:chicken",           {{220,200,180},{255,150,50},{0,0,0},{180,160,140},{200,180,160},{200,50,50}}},
            {"minecraft:rabbit",            {{180,150,140},{255,255,255},{0,0,0},{130,100,90},{150,120,110},{0,0,0}}},
            {"minecraft:wolf",              {{150,150,150},{255,255,255},{0,0,0},{100,100,100},{130,130,130},{0,0,0}}},
            {"minecraft:fox",               {{200,120,60},{255,255,255},{0,0,0},{150,80,30},{180,100,40},{255,255,255}}},
            {"minecraft:cat",               {{200,150,100},{100,200,100},{0,0,0},{150,100,60},{180,130,80},{0,0,0}}},
            {"minecraft:ocelot",            {{200,180,100},{100,200,100},{0,0,0},{150,130,60},{180,160,80},{0,0,0}}},
            {"minecraft:horse",             {{120,80,60},{255,255,255},{0,0,0},{80,50,30},{100,60,40},{0,0,0}}},
            {"minecraft:donkey",            {{100,100,100},{255,255,255},{0,0,0},{60,60,60},{80,80,80},{0,0,0}}},
            {"minecraft:mule",              {{120,100,80},{255,255,255},{0,0,0},{80,60,40},{100,80,60},{0,0,0}}},
            {"minecraft:skeleton_horse",    {{150,160,170},{200,50,200},{0,0,0},{100,110,120},{130,140,150},{0,0,0}}},
            {"minecraft:zombie_horse",      {{80,120,60},{200,50,50},{0,0,0},{50,80,30},{60,100,40},{0,0,0}}},
            {"minecraft:bee",               {{220,180,50},{255,255,255},{0,0,0},{180,140,20},{200,160,30},{50,50,50}}},
            {"minecraft:dolphin",           {{100,140,180},{255,255,255},{0,0,0},{60,100,140},{80,120,160},{0,0,0}}},
            {"minecraft:squid",             {{100,120,150},{255,255,255},{0,0,0},{60,80,110},{80,100,130},{0,0,0}}},
            {"minecraft:glow_squid",        {{80,180,200},{0,200,200},{0,0,0},{40,140,160},{60,160,180},{0,255,200}}},
            {"minecraft:turtle",            {{80,140,80},{255,255,255},{0,0,0},{40,100,40},{60,120,60},{0,0,0}}},
            {"minecraft:panda",             {{200,200,200},{255,255,255},{0,0,0},{150,150,150},{180,180,180},{50,50,50}}},
            {"minecraft:polar_bear",        {{220,210,190},{255,255,255},{0,0,0},{180,170,150},{200,190,170},{0,0,0}}},
            {"minecraft:goat",              {{180,170,160},{255,200,100},{0,0,0},{140,130,120},{160,150,140},{0,0,0}}},
            {"minecraft:frog",              {{80,180,80},{255,255,200},{0,0,0},{40,140,40},{60,160,60},{0,0,0}}},
            {"minecraft:tadpole",           {{60,80,60},{0,0,0},{0,0,0},{30,50,30},{40,60,40},{0,0,0}}},
            {"minecraft:parrot",            {{50,180,50},{255,255,255},{0,0,0},{30,140,30},{40,160,40},{200,200,50}}},
            {"minecraft:llama",             {{180,160,130},{255,255,255},{0,0,0},{140,120,90},{160,140,110},{0,0,0}}},
            {"minecraft:trader_llama",      {{160,140,110},{255,255,255},{0,0,0},{120,100,70},{140,120,90},{100,200,100}}},
            {"minecraft:wandering_trader",  {{150,120,100},{255,255,255},{0,0,0},{110,80,60},{130,100,80},{0,150,200}}},
            {"minecraft:strider",           {{200,100,100},{255,200,200},{0,0,0},{150,60,60},{180,80,80},{0,0,0}}},
            {"minecraft:iron_golem",        {{150,120,100},{255,255,255},{0,0,0},{100,80,60},{120,100,80},{80,80,100}}},
            {"minecraft:snow_golem",        {{220,220,240},{255,255,255},{0,0,0},{180,180,200},{200,200,220},{255,150,50}}},
            {"minecraft:villager",          {{140,110,80},{255,255,255},{0,0,0},{100,70,40},{120,90,60},{150,100,50}}},
            {"minecraft:villager_v2",       {{140,110,80},{255,255,255},{0,0,0},{100,70,40},{120,90,60},{150,100,50}}},
            {"villager",                    {{140,110,80},{255,255,255},{0,0,0},{100,70,40},{120,90,60},{150,100,50}}},
            {"minecraft:mooshroom",         {{200,100,100},{255,255,255},{0,0,0},{150,60,60},{180,80,80},{200,50,50}}},
            {"minecraft:sniffer",           {{80,120,160},{255,255,200},{0,0,0},{40,80,120},{60,100,140},{0,0,0}}},
            {"minecraft:camel",             {{180,160,130},{255,255,255},{0,0,0},{140,120,90},{160,140,110},{100,80,50}}},
            {"minecraft:armadillo",         {{150,120,80},{255,255,255},{0,0,0},{110,80,40},{130,100,60},{0,0,0}}},
            {"minecraft:allay",             {{150,180,220},{255,255,200},{0,0,0},{100,130,170},{120,150,190},{200,200,255}}},
            {"minecraft:axolotl",           {{200,120,180},{255,200,200},{0,0,0},{150,80,130},{180,100,150},{0,0,0}}},
            {"minecraft:bat",               {{80,60,40},{255,255,255},{0,0,0},{50,30,20},{60,40,30},{0,0,0}}},
            {"minecraft:pufferfish",        {{200,200,100},{255,255,255},{0,0,0},{150,150,50},{180,180,80},{0,0,0}}},
            {"minecraft:tropical_fish",     {{255,150,50},{0,0,0},{255,255,255},{200,80,0},{255,200,100},{100,200,255}}},
        };
        return s_faces;
    }

    // 程序化生成 12x12 RGBA 面孔贴图 (回退保障)
    inline ID3D11ShaderResourceView* CreateProgrammaticFaceTexture(ID3D11Device* pDevice, const MobFace& face) {
        if (!pDevice) return nullptr;
        constexpr int S = 12;
        uint8_t pixels[S * S * 4];
        const uint8_t* cols[6] = {face.hair, face.skin, face.eye, face.eyePupil, face.mouth, face.accent};
        const int tmpl[12][12] = {
            {0,0,0,0,0,0,0,0,0,0,0,0},
            {0,1,1,1,1,1,1,1,1,1,1,0},
            {0,1,1,1,1,1,1,1,1,1,1,0},
            {0,1,1,1,1,1,1,1,1,1,1,0},
            {0,1,1,2,2,1,1,2,2,1,1,0},
            {0,1,1,3,3,1,1,3,3,1,1,0},
            {0,1,1,1,1,1,1,1,1,1,1,0},
            {0,1,1,1,1,4,4,1,1,1,1,0},
            {0,1,1,1,4,4,4,4,1,1,1,0},
            {0,1,1,1,1,1,1,1,1,1,1,0},
            {0,1,1,1,1,1,1,1,1,1,1,0},
            {0,0,0,0,0,0,0,0,0,0,0,0},
        };
        for (int y = 0; y < S; y++) {
            for (int x = 0; x < S; x++) {
                int ci = tmpl[y][x];
                int di = (y * S + x) * 4;
                pixels[di+0] = cols[ci][0];
                pixels[di+1] = cols[ci][1];
                pixels[di+2] = cols[ci][2];
                pixels[di+3] = 255;
            }
        }
        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = S; desc.Height = S;
        desc.MipLevels = 1; desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1; desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA init = {};
        init.pSysMem = pixels; init.SysMemPitch = S * 4;
        ID3D11Texture2D* tex = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        if (SUCCEEDED(pDevice->CreateTexture2D(&desc, &init, &tex)) && tex) {
            pDevice->CreateShaderResourceView(tex, nullptr, &srv);
            tex->Release();
        }
        return srv;
    }

    struct PlayerHeadTexture {
        ID3D11ShaderResourceView* srv = nullptr;
        uint64_t revision = 0;
    };
    inline std::unordered_map<std::string, PlayerHeadTexture> g_playerHeadTextures;
    inline std::mutex g_playerHeadTexturesMutex;

    inline void ClearPlayerHeadTexturesIfRequested() {
        if (!clearPlayerHeadTextures.exchange(false)) return;
        std::lock_guard<std::mutex> lock(g_playerHeadTexturesMutex);
        for (auto& [uuid, texture] : g_playerHeadTextures) {
            if (texture.srv) texture.srv->Release();
        }
        g_playerHeadTextures.clear();
    }

    // 获取玩家皮肤头部贴图（从抓取合成的 8x8 RGBA 像素最近邻放大）
    inline ID3D11ShaderResourceView* GetOrCreatePlayerHeadTexture(ID3D11Device* pDevice, const std::string& uuid) {
        if (!pDevice || uuid.empty()) return nullptr;
        ClearPlayerHeadTexturesIfRequested();

        PlayerSkinHead head;
        {
            std::lock_guard<std::mutex> lock(g_playerSkinMutex);
            auto hit = g_playerSkinHeads.find(uuid);
            if (hit == g_playerSkinHeads.end() || !hit->second.valid) {
                std::lock_guard<std::mutex> pLock(g_playerHeadTexturesMutex);
                auto texture = g_playerHeadTextures.find(uuid);
                if (texture != g_playerHeadTextures.end()) {
                    if (texture->second.srv) texture->second.srv->Release();
                    g_playerHeadTextures.erase(texture);
                }
                return nullptr;
            }
            head = hit->second;
        }

        {
            std::lock_guard<std::mutex> pLock(g_playerHeadTexturesMutex);
            auto texture = g_playerHeadTextures.find(uuid);
            if (texture != g_playerHeadTextures.end()) {
                if (texture->second.revision == head.revision && texture->second.srv) {
                    return texture->second.srv;
                }
                if (texture->second.srv) texture->second.srv->Release();
                g_playerHeadTextures.erase(texture);
            }
        }

        constexpr int FACE_SIZE = 16;
        std::vector<uint8_t> pixels(FACE_SIZE * FACE_SIZE * 4);
        const float scale = (float)FACE_SIZE / 8.0f;
        for (int y = 0; y < FACE_SIZE; ++y) {
            for (int x = 0; x < FACE_SIZE; ++x) {
                int sx = static_cast<int>(x / scale);
                int sy = static_cast<int>(y / scale);
                if (sx > 7) sx = 7;
                if (sy > 7) sy = 7;
                int si = (sy * 8 + sx) * 4;
                int di = (y * FACE_SIZE + x) * 4;
                pixels[di + 0] = head.pixels[si + 0];
                pixels[di + 1] = head.pixels[si + 1];
                pixels[di + 2] = head.pixels[si + 2];
                pixels[di + 3] = head.pixels[si + 3];
            }
        }

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = FACE_SIZE;
        desc.Height = FACE_SIZE;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = pixels.data();
        initData.SysMemPitch = FACE_SIZE * 4;

        ID3D11Texture2D* pTexture = nullptr;
        ID3D11ShaderResourceView* pSRV = nullptr;
        if (SUCCEEDED(pDevice->CreateTexture2D(&desc, &initData, &pTexture)) && pTexture) {
            pDevice->CreateShaderResourceView(pTexture, nullptr, &pSRV);
            pTexture->Release();
        }

        if (!pSRV) return nullptr;

        {
            std::lock_guard<std::mutex> pLock(g_playerHeadTexturesMutex);
            g_playerHeadTextures.emplace(uuid, PlayerHeadTexture{pSRV, head.revision});
        }
        return pSRV;
    }

    inline std::unordered_map<std::string, ID3D11ShaderResourceView*> s_cache;
    inline std::mutex s_cacheMutex;

    inline ID3D11ShaderResourceView* GetEntityHeadTexture(ID3D11Device* pDevice, const std::string& typeName, const std::string& nameTag = "") {
        if (!pDevice || typeName.empty()) return nullptr;
        if (typeName == "player" || typeName == "minecraft:player") return nullptr;

        // 生成唯一缓存键（若有彩蛋命名则包含 nameTag）
        std::string cacheKey = typeName;
        if (!nameTag.empty()) {
            if (nameTag == "jeb_" || nameTag == "Toast" || nameTag == "Johnny" || nameTag == "Jellie") {
                cacheKey = typeName + "#" + nameTag;
            }
        }

        {
            std::lock_guard<std::mutex> lock(s_cacheMutex);
            auto it = s_cache.find(cacheKey);
            if (it != s_cache.end()) {
                return it->second;
            }
        }

        // 查找对应的 PNG 文件名
        std::string fileName;
        if (!nameTag.empty()) {
            if (nameTag == "jeb_") fileName = "JebFace.png";
            else if (nameTag == "Toast") fileName = "ToastFace.png";
            else if (nameTag == "Johnny") fileName = "JohnnyFace.png";
            else if (nameTag == "Jellie") fileName = "JellieFace.png";
        }

        if (fileName.empty()) {
            const auto& iconMap = GetEntityIconMap();
            auto it = iconMap.find(typeName);
            if (it != iconMap.end()) {
                fileName = it->second;
            } else {
                // 剥离 minecraft: 再次查找
                std::string clean = typeName;
                if (clean.rfind("minecraft:", 0) == 0) {
                    clean = clean.substr(10);
                }
                auto itClean = iconMap.find(clean);
                if (itClean != iconMap.end()) {
                    fileName = itClean->second;
                } else {
                    // Fallback: 尝试转换为驼峰格式，如 "zombie" -> "ZombieFace.png"
                    std::string camel = ToUpperCamelCase(clean);
                    auto headsDir = GetHeadsDirectory();
                    if (!headsDir.empty()) {
                        std::error_code ec;
                        if (std::filesystem::exists(headsDir / (camel + "Face.png"), ec)) {
                            fileName = camel + "Face.png";
                        } else if (std::filesystem::exists(headsDir / (camel + "Body.png"), ec)) {
                            fileName = camel + "Body.png";
                        }
                    }
                }
            }
        }

        ID3D11ShaderResourceView* srv = nullptr;
        if (!fileName.empty()) {
            auto headsDir = GetHeadsDirectory();
            if (!headsDir.empty()) {
                auto filePath = headsDir / fileName;
                srv = LoadTextureFromFileWIC(pDevice, filePath);
            }
        }

        // PNG 不存在或加载失败时，回退到程序化面孔
        if (!srv) {
            const auto& faces = GetTypeToFaceMap();
            auto faceIt = faces.find(typeName);
            if (faceIt == faces.end() && typeName.size() > 10 && typeName.rfind("minecraft:", 0) == 0) {
                faceIt = faces.find(typeName.substr(10));
            }
            if (faceIt == faces.end()) {
                faceIt = faces.find("minecraft:" + typeName);
            }
            if (faceIt != faces.end()) {
                srv = CreateProgrammaticFaceTexture(pDevice, faceIt->second);
            }
        }

        {
            std::lock_guard<std::mutex> lock(s_cacheMutex);
            s_cache[cacheKey] = srv;
        }

        return srv;
    }

    inline void ReleaseAll() {
        {
            std::lock_guard<std::mutex> lock(s_cacheMutex);
            for (auto& pair : s_cache) {
                if (pair.second) {
                    pair.second->Release();
                    pair.second = nullptr;
                }
            }
            s_cache.clear();
        }
        {
            std::lock_guard<std::mutex> lock(g_playerHeadTexturesMutex);
            for (auto& pair : g_playerHeadTextures) {
                if (pair.second.srv) {
                    pair.second.srv->Release();
                    pair.second.srv = nullptr;
                }
            }
            g_playerHeadTextures.clear();
        }
    }

} // namespace EntityIconManager
