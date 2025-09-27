#pragma once

#include <string>

#include "Unreal/TArray.hpp"

#include "ST_PlayerInfo.hpp"

namespace Client
{
    void OnSceneLoad(std::wstring);
    void Tick();
    void SetPlayerInfo(const FST_PlayerInfo&);
    void GetGhostInfo(RC::Unreal::TArray<FST_PlayerInfo>&, RC::Unreal::TArray<uint8_t>&);
}
