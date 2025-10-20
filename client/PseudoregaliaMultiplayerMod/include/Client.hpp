#pragma once

#include <string>

#include "Unreal/TArray.hpp"

#include "ST_PlayerInfo.hpp"

#include <Unreal/Property/FArrayProperty.hpp>
#pragma warning(default : 4005)

namespace Client
{

    void OnSceneLoad(std::wstring);
    void Tick();
    uint32_t SetPlayerInfo(const FST_PlayerInfo&);
    void GetGhostInfo(const uint32_t&, RC::Unreal::FScriptArray&, RC::Unreal::TArray<uint8_t>&);
}
