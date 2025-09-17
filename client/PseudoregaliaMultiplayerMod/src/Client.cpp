#pragma once

#include "Client.hpp"

#include <queue>

namespace
{
    namespace Internal
    {
        void UpdateInfo(const FST_PlayerInfo&);
        const FST_PlayerInfo& GetInfo();

        std::deque<FST_PlayerInfo> info_queue{};
        const size_t QUEUE_MAX = 60;
    }
}

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
    Internal::UpdateInfo(player_info);
    // TODO use player_info to update info to send to server
}

void Client::GetGhostInfo(RC::Unreal::TArray<FST_PlayerInfo>& ghost_info)
{
    // TODO fill out ghost_info
    ghost_info.Add(Internal::GetInfo());
}

void Internal::UpdateInfo(const FST_PlayerInfo& player_info)
{
    if (info_queue.size() == QUEUE_MAX)
    {
        info_queue.pop_front();
    }
    info_queue.push_back(player_info);
}

const FST_PlayerInfo& Internal::GetInfo()
{
    return info_queue.front();
}
