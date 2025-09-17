#pragma once

#include "Client.hpp"

#include <queue>

#define WSWRAP_NO_SSL
#define WSWRAP_NO_COMPRESSION
#define WSWRAP_SEND_EXCEPTIONS
#define ASIO_STANDALONE
#define BOOST_ALL_NO_LIB
#include "wswrap.hpp"

#include "Logger.hpp"

namespace
{
    namespace Internal
    {
        void UpdateInfo(const FST_PlayerInfo&);
        const FST_PlayerInfo& GetInfo();
        void OnOpen();
        void OnClose();
        void OnMessage(const std::string&);
        void OnError(const std::string&);
        std::wstring ToWide(const std::string&);
    }

    std::deque<FST_PlayerInfo> info_queue{};
    const size_t QUEUE_MAX = 60;

    bool queue_connect = false;
    bool queue_disconnect = false;
    wswrap::WS* ws;
    // TODO make this configurable
    const std::string URI = "ws://127.0.0.1:8080";
}

void Client::OnSceneLoad(std::wstring level)
{
    if (level == L"TitleScreen" || level == L"EndScreen")
    {
        queue_disconnect = true;
    }
    else
    {
        queue_connect = true;
    }
}

void Client::Tick()
{
    if (queue_disconnect)
    {
        if (ws)
        {
            delete ws;
            ws = nullptr;
            // TODO reset ghost info from server
        }
        queue_disconnect = false;
    }
    if (queue_connect)
    {
        if (!ws)
        {
            try
            {
                ws = new wswrap::WS(URI, Internal::OnOpen, Internal::OnClose, Internal::OnMessage, Internal::OnError);
            }
            catch (const std::exception& ex)
            {
                std::wstring what = Internal::ToWide(ex.what());
                Log(L"Error connecting: " + what, LogType::Error);
            }
        }
        queue_connect = false;
    }
    if (ws)
    {
        // TODO send messages based on current player info? maybe on an interval
        ws->poll();
    }
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

void Internal::OnOpen()
{
    Log(L"Connected to server", LogType::Loud);
}

void Internal::OnClose()
{
    Log(L"Disconnected from server", LogType::Loud);
    queue_disconnect = true;
}

void Internal::OnMessage(const std::string& message)
{
    // TODO remove log and parse message to update ghost info
    Log(L"Received message: " + Internal::ToWide(message));
}

void Internal::OnError(const std::string& error_message)
{
    Log(L"Error: " + Internal::ToWide(error_message), LogType::Error);
}

std::wstring Internal::ToWide(const std::string& input)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(input);
}
