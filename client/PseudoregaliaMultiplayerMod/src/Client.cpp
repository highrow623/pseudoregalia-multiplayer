#pragma once

#include "Client.hpp"

#include <chrono>
#include <codecvt>
#include <queue>

#define WSWRAP_NO_SSL
#define WSWRAP_NO_COMPRESSION
#define WSWRAP_SEND_EXCEPTIONS
#define ASIO_STANDALONE
#define BOOST_ALL_NO_LIB
#include "wswrap.hpp"
#include "nlohmann/json.hpp"

#include "Logger.hpp"
#include "Settings.hpp"

namespace
{
    void OnOpen();
    void OnClose();
    void OnMessage(const std::string&);
    void OnError(const std::string&);
    std::wstring ToWide(const std::string&);
    std::string BuildUpdate();

    FST_PlayerInfo player_info = {};

    bool connected = false;
    bool queue_connect = false;
    bool queue_disconnect = false;
    wswrap::WS* ws = nullptr;

    std::optional<FST_PlayerInfo> last_sent = {};
    std::unordered_map<std::string, FST_PlayerInfo> ghost_data = {};

    // TODO make this configurable?
    const auto MS_PER_UPDATE = std::chrono::milliseconds(20);
    std::optional<std::chrono::steady_clock::time_point> last_send_time = {};
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
            last_sent.reset();
            last_send_time.reset();
            ghost_data.clear();
            connected = false;
        }
        queue_disconnect = false;
    }
    if (queue_connect)
    {
        if (!ws)
        {
            try
            {
                ws = new wswrap::WS(Settings::GetURI(), OnOpen, OnClose, OnMessage, OnError);
            }
            catch (const std::exception& ex)
            {
                ws = nullptr;
                Log(L"Error connecting: " + ToWide(ex.what()), LogType::Error);
            }
        }
        queue_connect = false;
    }
    if (ws && connected)
    {
        bool send_update = false;
        // This implementation calculates whether to send an update based on the timestamp of the last update sent,
        // which means update frequency depends on the rate at which this function is called. In other words, if
        // MS_PER_UPDATE is set to 50, an update won't happen sooner than 50ms, but the extra time above 50ms is not
        // taken into account for the next update.
        if (last_send_time)
        {
            auto now = std::chrono::steady_clock::now();
            std::chrono::duration<double> since_last = now - *last_send_time;
            auto ms_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(since_last);
            if (ms_since_last >= MS_PER_UPDATE)
            {
                send_update = true;
                last_send_time = now;
            }
        }
        else
        {
            send_update = true;
            last_send_time = std::chrono::steady_clock::now();
        }
        if (send_update)
        {
            std::string update = BuildUpdate();
            ws->send_text(update);
        }
    }
    if (ws)
    {
        ws->poll();
    }
}

void Client::SetPlayerInfo(const FST_PlayerInfo& info)
{
    player_info = info;
}

void Client::GetGhostInfo(RC::Unreal::TArray<FST_PlayerInfo>& ghost_info)
{
    for (const auto& [_, ghost] : ghost_data)
    {
        ghost_info.Add(ghost);
    }
}

namespace
{

void OnOpen()
{
    Log(L"Connected to server", LogType::Loud);
    connected = true;
}

void OnClose()
{
    Log(L"Disconnected from server", LogType::Loud);
    queue_disconnect = true;
}

void OnMessage(const std::string& message)
{
    nlohmann::json j = nlohmann::json::parse(message);
    if (!j.is_object())
    {
        Log(L"Received non-object message: " + ToWide(message), LogType::Warning);
        return;
    }

    std::unordered_set<std::string> keys = {};
    for (nlohmann::json::const_iterator iter = j.begin(); iter != j.end(); iter++)
    {
        const auto& key = iter.key();
        const auto& value = iter.value();
        if (!value.is_object())
        {
            Log(L"Received non-object value for ghost " + ToWide(key), LogType::Warning);
            continue;
        }
        keys.insert(key);
        if (ghost_data.contains(key))
        {
            auto& ghost = ghost_data.at(key);
            if (value.contains("zo"))
            {
                ghost.zone = RC::Unreal::FString(ToWide(value["zo"]).c_str());
            }
            if (value.contains("lx"))
            {
                ghost.location_x = value["lx"];
            }
            if (value.contains("ly"))
            {
                ghost.location_y = value["ly"];
            }
            if (value.contains("lz"))
            {
                ghost.location_z = value["lz"];
            }
            if (value.contains("rx"))
            {
                ghost.rotation_x = value["rx"];
            }
            if (value.contains("ry"))
            {
                ghost.rotation_y = value["ry"];
            }
            if (value.contains("rz"))
            {
                ghost.rotation_z = value["rz"];
            }
            if (value.contains("sx"))
            {
                ghost.scale_x = value["sx"];
            }
            if (value.contains("sy"))
            {
                ghost.scale_y = value["sy"];
            }
            if (value.contains("sz"))
            {
                ghost.scale_z = value["sz"];
            }
        }
        else
        {
            if (!value.contains("zo")
                || !value.contains("lx")
                || !value.contains("ly")
                || !value.contains("lz")
                || !value.contains("rx")
                || !value.contains("ry")
                || !value.contains("rz")
                || !value.contains("sx")
                || !value.contains("sy")
                || !value.contains("sz"))
            {
                Log(L"Received initial data with missing fields for ghost " + ToWide(key), LogType::Warning);
                continue;
            }
            ghost_data[key] = FST_PlayerInfo{
                .zone = RC::Unreal::FString(ToWide(value["zo"]).c_str()),
                .location_x = value["lx"],
                .location_y = value["ly"],
                .location_z = value["lz"],
                .rotation_x = value["rx"],
                .rotation_y = value["ry"],
                .rotation_z = value["rz"],
                .scale_x = value["sx"],
                .scale_y = value["sy"],
                .scale_z = value["sz"],
            };
        }
    }
    for (auto iter = ghost_data.cbegin(); iter != ghost_data.cend();)
    {
        if (!keys.contains(iter->first))
        {
            iter = ghost_data.erase(iter);
        }
        else {
            iter++;
        }
    }
}

void OnError(const std::string& error_message)
{
    Log(L"Error: " + ToWide(error_message), LogType::Error);
}

std::wstring ToWide(const std::string& input)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(input);
}

std::string BuildUpdate()
{
    nlohmann::json j = nlohmann::json::object();
    if (last_sent)
    {
        if (last_sent->zone != player_info.zone)
        {
            j["zo"] = player_info.zone.GetCharArray();
            last_sent->zone = player_info.zone;
        }
        if (last_sent->location_x != player_info.location_x)
        {
            j["lx"] = player_info.location_x;
            last_sent->location_x = player_info.location_x;
        }
        if (last_sent->location_y != player_info.location_y)
        {
            j["ly"] = player_info.location_y;
            last_sent->location_y = player_info.location_y;
        }
        if (last_sent->location_z != player_info.location_z)
        {
            j["lz"] = player_info.location_z;
            last_sent->location_z = player_info.location_z;
        }
        if (last_sent->rotation_x != player_info.rotation_x)
        {
            j["rx"] = player_info.rotation_x;
            last_sent->rotation_x = player_info.rotation_x;
        }
        if (last_sent->rotation_y != player_info.rotation_y)
        {
            j["ry"] = player_info.rotation_y;
            last_sent->rotation_y = player_info.rotation_y;
        }
        if (last_sent->rotation_z != player_info.rotation_z)
        {
            j["rz"] = player_info.rotation_z;
            last_sent->rotation_z = player_info.rotation_z;
        }
        if (last_sent->scale_x != player_info.scale_x)
        {
            j["sx"] = player_info.scale_x;
            last_sent->scale_x = player_info.scale_x;
        }
        if (last_sent->scale_y != player_info.scale_y)
        {
            j["sy"] = player_info.scale_y;
            last_sent->scale_y = player_info.scale_y;
        }
        if (last_sent->scale_z != player_info.scale_z)
        {
            j["sz"] = player_info.scale_z;
            last_sent->scale_z = player_info.scale_z;
        }
    }
    else
    {
        j["zo"] = player_info.zone.GetCharArray();
        j["lx"] = player_info.location_x;
        j["ly"] = player_info.location_y;
        j["lz"] = player_info.location_z;
        j["rx"] = player_info.rotation_x;
        j["ry"] = player_info.rotation_y;
        j["rz"] = player_info.rotation_z;
        j["sx"] = player_info.scale_x;
        j["sy"] = player_info.scale_y;
        j["sz"] = player_info.scale_z;
        last_sent = player_info;
    }
    return j.dump();
}

} // namespace
