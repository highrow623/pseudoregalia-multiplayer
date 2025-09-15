#pragma once

#include "Client.hpp"

void Client::OnSceneLoad(std::wstring level)
{
    // TODO handle connection based on level? if non-gameplay level, close connection if one exists; if gameplay level,
    // establish connection if one doesn't exist
}

void Client::Tick()
{
    // TODO poll server and potentially send messages based on current player info? maybe on an interval
}

void Client::SetPlayerInfo(const FST_PlayerInfo& player_info)
{
    // TODO use player_info to update info to send to server
}

void Client::GetGhostInfo(RC::Unreal::TArray<FST_PlayerInfo>& ghost_info)
{
    // TODO fill out ghost_info
}
