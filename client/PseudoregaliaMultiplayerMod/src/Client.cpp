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
    namespace I
    {
        void OnOpen();
        void OnClose();
        void OnMessage(const std::string&);
        void OnError(const std::string&);
        std::wstring ToWide(const std::string&);
        std::string BuildUpdate();
    }

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
                ws = new wswrap::WS(Settings::GetURI(), I::OnOpen, I::OnClose, I::OnMessage, I::OnError);
            }
            catch (const std::exception& ex)
            {
                ws = nullptr;
                Log(L"Error connecting: " + I::ToWide(ex.what()), LogType::Error);
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
            std::string update = I::BuildUpdate();
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

void I::OnOpen()
{
    Log(L"Connected to server", LogType::Loud);
    connected = true;
}

void I::OnClose()
{
    Log(L"Disconnected from server", LogType::Loud);
    queue_disconnect = true;
}

void I::OnMessage(const std::string& message)
{
    nlohmann::json j = nlohmann::json::parse(message);
    if (!j.is_object())
    {
        Log(L"Received non-object message: " + I::ToWide(message), LogType::Warning);
        return;
    }

    std::unordered_set<std::string> keys = {};
    for (nlohmann::json::const_iterator iter = j.begin(); iter != j.end(); iter++)
    {
        const auto& key = iter.key();
        const auto& value = iter.value();
        if (!value.is_object())
        {
            Log(L"Received non-object value for ghost " + I::ToWide(key), LogType::Warning);
            continue;
        }
        keys.insert(key);
        if (ghost_data.contains(key))
        {
            auto& ghost = ghost_data.at(key);
            if (value.contains("zo"))
            {
                ghost.zone_2_7A1ED7FA4276A703DD7058A44D2AF8DA = RC::Unreal::FString(I::ToWide(value["zo"]).c_str());
            }
            if (value.contains("lx"))
            {
                ghost.location_x_24_A99B33C64A7E3258D121F292594BE28D = value["lx"];
            }
            if (value.contains("ly"))
            {
                ghost.location_y_25_F92DA72B4567F3AC671650A509495255 = value["ly"];
            }
            if (value.contains("lz"))
            {
                ghost.location_z_26_656F85D54404D4616F640EB43F326BC1 = value["lz"];
            }
            if (value.contains("rx"))
            {
                ghost.rotation_x_27_5CCE14E84D032F45F387C7BAA7E80C63 = value["rx"];
            }
            if (value.contains("ry"))
            {
                ghost.rotation_y_28_C810F6EF4BA528713F570EA57644882C = value["ry"];
            }
            if (value.contains("rz"))
            {
                ghost.rotation_z_29_CBFC70324216054968EAA7B1D823249C = value["rz"];
            }
            if (value.contains("sx"))
            {
                ghost.scale_x_30_55A9DE384646D5352090B3BF5BADC83E = value["sx"];
            }
            if (value.contains("sy"))
            {
                ghost.scale_y_31_8C45682E4A4B1A074EC88AB59FA732E2 = value["sy"];
            }
            if (value.contains("sz"))
            {
                ghost.scale_z_32_9D900BCF48CBE018B7FE6D942156FDA6 = value["sz"];
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
                Log(L"Received initial data with missing fields for ghost " + I::ToWide(key), LogType::Warning);
                continue;
            }
            ghost_data[key] = FST_PlayerInfo{
                .zone_2_7A1ED7FA4276A703DD7058A44D2AF8DA = RC::Unreal::FString(I::ToWide(value["zo"]).c_str()),
                .location_x_24_A99B33C64A7E3258D121F292594BE28D = value["lx"],
                .location_y_25_F92DA72B4567F3AC671650A509495255 = value["ly"],
                .location_z_26_656F85D54404D4616F640EB43F326BC1 = value["lz"],
                .rotation_x_27_5CCE14E84D032F45F387C7BAA7E80C63 = value["rx"],
                .rotation_y_28_C810F6EF4BA528713F570EA57644882C = value["ry"],
                .rotation_z_29_CBFC70324216054968EAA7B1D823249C = value["rz"],
                .scale_x_30_55A9DE384646D5352090B3BF5BADC83E = value["sx"],
                .scale_y_31_8C45682E4A4B1A074EC88AB59FA732E2 = value["sy"],
                .scale_z_32_9D900BCF48CBE018B7FE6D942156FDA6 = value["sz"],
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

void I::OnError(const std::string& error_message)
{
    Log(L"Error: " + I::ToWide(error_message), LogType::Error);
}

std::wstring I::ToWide(const std::string& input)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(input);
}

std::string I::BuildUpdate()
{
    nlohmann::json j = nlohmann::json::object();
    if (last_sent)
    {
        if (last_sent->zone_2_7A1ED7FA4276A703DD7058A44D2AF8DA!= player_info.zone_2_7A1ED7FA4276A703DD7058A44D2AF8DA)
        {
            j["zo"] = player_info.zone_2_7A1ED7FA4276A703DD7058A44D2AF8DA.GetCharArray();
            last_sent->zone_2_7A1ED7FA4276A703DD7058A44D2AF8DA = player_info.zone_2_7A1ED7FA4276A703DD7058A44D2AF8DA;
        }
        if (last_sent->location_x_24_A99B33C64A7E3258D121F292594BE28D !=
            player_info.location_x_24_A99B33C64A7E3258D121F292594BE28D)
        {
            j["lx"] = player_info.location_x_24_A99B33C64A7E3258D121F292594BE28D;
            last_sent->location_x_24_A99B33C64A7E3258D121F292594BE28D =
                player_info.location_x_24_A99B33C64A7E3258D121F292594BE28D;
        }
        if (last_sent->location_y_25_F92DA72B4567F3AC671650A509495255 !=
            player_info.location_y_25_F92DA72B4567F3AC671650A509495255)
        {
            j["ly"] = player_info.location_y_25_F92DA72B4567F3AC671650A509495255;
            last_sent->location_y_25_F92DA72B4567F3AC671650A509495255 =
                player_info.location_y_25_F92DA72B4567F3AC671650A509495255;
        }
        if (last_sent->location_z_26_656F85D54404D4616F640EB43F326BC1 !=
            player_info.location_z_26_656F85D54404D4616F640EB43F326BC1)
        {
            j["lz"] = player_info.location_z_26_656F85D54404D4616F640EB43F326BC1;
            last_sent->location_z_26_656F85D54404D4616F640EB43F326BC1 =
                player_info.location_z_26_656F85D54404D4616F640EB43F326BC1;
        }
        if (last_sent->rotation_x_27_5CCE14E84D032F45F387C7BAA7E80C63 !=
            player_info.rotation_x_27_5CCE14E84D032F45F387C7BAA7E80C63)
        {
            j["rx"] = player_info.rotation_x_27_5CCE14E84D032F45F387C7BAA7E80C63;
            last_sent->rotation_x_27_5CCE14E84D032F45F387C7BAA7E80C63 =
                player_info.rotation_x_27_5CCE14E84D032F45F387C7BAA7E80C63;
        }
        if (last_sent->rotation_y_28_C810F6EF4BA528713F570EA57644882C !=
            player_info.rotation_y_28_C810F6EF4BA528713F570EA57644882C)
        {
            j["ry"] = player_info.rotation_y_28_C810F6EF4BA528713F570EA57644882C;
            last_sent->rotation_y_28_C810F6EF4BA528713F570EA57644882C =
                player_info.rotation_y_28_C810F6EF4BA528713F570EA57644882C;
        }
        if (last_sent->rotation_z_29_CBFC70324216054968EAA7B1D823249C !=
            player_info.rotation_z_29_CBFC70324216054968EAA7B1D823249C)
        {
            j["rz"] = player_info.rotation_z_29_CBFC70324216054968EAA7B1D823249C;
            last_sent->rotation_z_29_CBFC70324216054968EAA7B1D823249C =
                player_info.rotation_z_29_CBFC70324216054968EAA7B1D823249C;
        }
        if (last_sent->scale_x_30_55A9DE384646D5352090B3BF5BADC83E !=
            player_info.scale_x_30_55A9DE384646D5352090B3BF5BADC83E)
        {
            j["sx"] = player_info.scale_x_30_55A9DE384646D5352090B3BF5BADC83E;
            last_sent->scale_x_30_55A9DE384646D5352090B3BF5BADC83E =
                player_info.scale_x_30_55A9DE384646D5352090B3BF5BADC83E;
        }
        if (last_sent->scale_y_31_8C45682E4A4B1A074EC88AB59FA732E2 !=
            player_info.scale_y_31_8C45682E4A4B1A074EC88AB59FA732E2)
        {
            j["sy"] = player_info.scale_y_31_8C45682E4A4B1A074EC88AB59FA732E2;
            last_sent->scale_y_31_8C45682E4A4B1A074EC88AB59FA732E2 =
                player_info.scale_y_31_8C45682E4A4B1A074EC88AB59FA732E2;
        }
        if (last_sent->scale_z_32_9D900BCF48CBE018B7FE6D942156FDA6 !=
            player_info.scale_z_32_9D900BCF48CBE018B7FE6D942156FDA6)
        {
            j["sz"] = player_info.scale_z_32_9D900BCF48CBE018B7FE6D942156FDA6;
            last_sent->scale_z_32_9D900BCF48CBE018B7FE6D942156FDA6 =
                player_info.scale_z_32_9D900BCF48CBE018B7FE6D942156FDA6;
        }
    }
    else
    {
        j["zo"] = player_info.zone_2_7A1ED7FA4276A703DD7058A44D2AF8DA.GetCharArray();
        j["lx"] = player_info.location_x_24_A99B33C64A7E3258D121F292594BE28D;
        j["ly"] = player_info.location_y_25_F92DA72B4567F3AC671650A509495255;
        j["lz"] = player_info.location_z_26_656F85D54404D4616F640EB43F326BC1;
        j["rx"] = player_info.rotation_x_27_5CCE14E84D032F45F387C7BAA7E80C63;
        j["ry"] = player_info.rotation_y_28_C810F6EF4BA528713F570EA57644882C;
        j["rz"] = player_info.rotation_z_29_CBFC70324216054968EAA7B1D823249C;
        j["sx"] = player_info.scale_x_30_55A9DE384646D5352090B3BF5BADC83E;
        j["sy"] = player_info.scale_y_31_8C45682E4A4B1A074EC88AB59FA732E2;
        j["sz"] = player_info.scale_z_32_9D900BCF48CBE018B7FE6D942156FDA6;
        last_sent = player_info;
    }
    return j.dump();
}
