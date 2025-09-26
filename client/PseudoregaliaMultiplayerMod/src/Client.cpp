#pragma once

#include "Client.hpp"

#include <bit>
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
#include "UdpSocket.hpp"

namespace
{
    void OnOpen();
    void OnClose();
    void OnMessage(const std::string&);
    void OnError(const std::string&);

    void OnRecv(const boost::array<uint8_t, 512>&, size_t);
    void OnErr(const std::string&);

    std::wstring ToWide(const std::string&);
    uint32_t HashW(const std::wstring&);
    int64_t ToUnrealId(const uint32_t&);

    void SerializeU32(uint32_t, boost::array<uint8_t, 48>&, size_t&);
    void SerializeF32(float, boost::array<uint8_t, 48>&, size_t&);
    uint32_t DeserializeU32(const boost::array<uint8_t, 512>&, size_t&);
    float DeserializeF32(const boost::array<uint8_t, 512>&, size_t&);

    bool queue_connect = false;
    bool queue_disconnect = false;
    wswrap::WS* ws = nullptr;
    UdpSocket::UdpSocket<48, 512>* udp = nullptr;

    struct Ghost
    {
        FST_PlayerInfo info;
        uint32_t zone;
        uint32_t update_num;
        bool updated;
    };

    uint32_t current_zone;
    std::optional<FST_PlayerInfo> player_info = {};

    // the id given in the Connected message; this value being defined means a full connection has been established
    std::optional<uint32_t> id = {};
    uint32_t update_num = 0;
    std::unordered_map<uint32_t, Ghost> ghosts = {};
    std::unordered_set<uint32_t> spawned_ghosts = {};
}

void Client::OnSceneLoad(std::wstring level)
{
    // we clear spawned_ghosts here because being in a new scene means they're all gone anyway
    spawned_ghosts.clear();
    current_zone = HashW(level);
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
            delete udp;
            udp = nullptr;
            id.reset();
            update_num = 0;
            ghosts.clear();
            // don't clear spawned_ghosts because we need to tell the bp mod to delete the actors
        }
        queue_disconnect = false;
    }
    if (queue_connect)
    {
        if (!ws)
        {
            try
            {
                const auto& address = Settings::GetAddress();
                const auto& port = Settings::GetPort();
                auto uri = "ws://" + address + ":" + std::to_string(port);
                ws = new wswrap::WS(uri, OnOpen, OnClose, OnMessage, OnError);
                udp = new UdpSocket::UdpSocket<48, 512>(address, port, OnRecv, OnErr);
            }
            catch (const std::exception& ex)
            {
                ws = nullptr;
                udp = nullptr;
                Log(L"Error connecting: " + ToWide(ex.what()), LogType::Error);
            }
        }
        queue_connect = false;
    }
    if (id && player_info)
    {
        boost::array<uint8_t, 48> buf{};
        update_num++;
        size_t pos = 0;
        SerializeU32(update_num, buf, pos);
        SerializeU32(*id, buf, pos);
        SerializeU32(current_zone, buf, pos);
        SerializeF32(float(player_info->location_x), buf, pos);
        SerializeF32(float(player_info->location_y), buf, pos);
        SerializeF32(float(player_info->location_z), buf, pos);
        SerializeF32(float(player_info->rotation_x), buf, pos);
        SerializeF32(float(player_info->rotation_y), buf, pos);
        SerializeF32(float(player_info->rotation_z), buf, pos);
        SerializeF32(float(player_info->scale_x), buf, pos);
        SerializeF32(float(player_info->scale_y), buf, pos);
        SerializeF32(float(player_info->scale_z), buf, pos);
        udp->Send(buf);
    }
    if (ws)
    {
        ws->poll();
        udp->Poll();
    }
}

void Client::SetPlayerInfo(const FST_PlayerInfo& info)
{
    player_info = info;
}

void Client::GetGhostInfo(RC::Unreal::TArray<FST_PlayerInfo>& ghost_info, RC::Unreal::TArray<int64_t>& to_remove)
{
    for (auto& [id, ghost] : ghosts)
    {
        if (!ghost.updated || ghost.zone != current_zone)
        {
            continue;
        }

        ghost_info.Add(ghost.info);
        spawned_ghosts.insert(id);
        ghost.updated = false;
    }

    for (auto it = spawned_ghosts.begin(); it != spawned_ghosts.end(); )
    {
        if (!ghosts.contains(*it) || ghosts.at(*it).zone != current_zone)
        {
            to_remove.Add(ToUnrealId(*it));
            it = spawned_ghosts.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

namespace
{

void OnOpen()
{
    Log(L"WebSocket connection established", LogType::Loud);
    nlohmann::json j = {
        {"type", "Connect"},
    };
    ws->send_text(j.dump());
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

    if (!j.contains("type"))
    {
        Log(L"Received message with no type field: " + ToWide(message), LogType::Warning);
        return;
    }

    if (!j["type"].is_string())
    {
        Log(L"Received message with non-string type field: " + ToWide(message), LogType::Warning);
        return;
    }

    if (j["type"] == "Connected")
    {
        // set this here in case we return early
        queue_disconnect = true;

        if (id)
        {
            Log(L"Received Connected message after connection was already established");
            return;
        }

        if (!j.contains("id"))
        {
            Log(L"Received Connected message with no id field: " + ToWide(message), LogType::Warning);
            return;
        }

        if (!j["id"].is_number_unsigned())
        {
            Log(L"Received Connected message with invalid id field: " + ToWide(message), LogType::Warning);
            return;
        }

        if (!j.contains("players"))
        {
            Log(L"Received Connected message with no players field: " + ToWide(message), LogType::Warning);
            return;
        }

        if (!j["players"].is_array())
        {
            Log(L"Received Connected message with non-array players field: " + ToWide(message), LogType::Warning);
            return;
        }

        for (auto it = j["players"].begin(); it != j["players"].end(); ++it)
        {
            if (!it->is_number_unsigned())
            {
                Log(L"Received Connected message with invalid player id: " + ToWide(message), LogType::Warning);
                return;
            }

            auto player_id = it->template get<uint32_t>();
            ghosts[player_id] = Ghost{ .info = FST_PlayerInfo{ .id = ToUnrealId(player_id) } };
        }
 
        id = j["id"].template get<uint32_t>();
        queue_disconnect = false;
        Log(L"Received Connected message with player id " + std::to_wstring(*id), LogType::Loud);
    }
    else if (j["type"] == "PlayerJoined")
    {
        if (!j.contains("id"))
        {
            Log(L"Received PlayerJoined message with no id field: " + ToWide(message), LogType::Warning);
            return;
        }

        if (!j["id"].is_number_unsigned())
        {
            Log(L"Received PlayerJoined message with invalid id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto player_id = j["id"].template get<uint32_t>();
        ghosts[player_id] = Ghost{ .info = FST_PlayerInfo{ .id = ToUnrealId(player_id) } };
        Log(L"Received PlayerJoined message with id " + std::to_wstring(player_id), LogType::Loud);
    }
    else if (j["type"] == "PlayerLeft")
    {
        if (!j.contains("id"))
        {
            Log(L"Received PlayerLeft message with no id field: " + ToWide(message), LogType::Warning);
            return;
        }

        if (!j["id"].is_number_unsigned())
        {
            Log(L"Received PlayerLeft message with invalid id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto player_id = j["id"].template get<uint32_t>();
        ghosts.erase(player_id);
        Log(L"Received PlayerLeft message with id " + std::to_wstring(player_id), LogType::Loud);
    }
    else
    {
        Log(L"Received message with unknown type: " + ToWide(j["type"]), LogType::Warning);
    }
}

void OnError(const std::string& error_message)
{
    Log(L"WebSocket error: " + ToWide(error_message), LogType::Error);
}

void OnRecv(const boost::array<uint8_t, 512>& buf, size_t len)
{
    if (len < 48 || len > 488 || len % 44 != 4)
    {
        Log(L"Received packet of invalid size " + std::to_wstring(len), LogType::Warning);
        return;
    }

    size_t pos = 0;
    uint32_t update_num = DeserializeU32(buf, pos);
    size_t num_updates = (len - 4) / 44;
    for (size_t i = 0; i < num_updates; i++)
    {
        pos = 4 + i * 44;
        uint32_t player_id = DeserializeU32(buf, pos);
        if (!ghosts.contains(player_id))
        {
            continue;
        }

        auto& ghost = ghosts.at(player_id);
        if (update_num <= ghost.update_num)
        {
            continue;
        }

        ghost.zone = DeserializeU32(buf, pos);
        ghost.info.location_x = double(DeserializeF32(buf, pos));
        ghost.info.location_y = double(DeserializeF32(buf, pos));
        ghost.info.location_z = double(DeserializeF32(buf, pos));
        ghost.info.rotation_x = double(DeserializeF32(buf, pos));
        ghost.info.rotation_y = double(DeserializeF32(buf, pos));
        ghost.info.rotation_z = double(DeserializeF32(buf, pos));
        ghost.info.scale_x = double(DeserializeF32(buf, pos));
        ghost.info.scale_y = double(DeserializeF32(buf, pos));
        ghost.info.scale_z = double(DeserializeF32(buf, pos));
        ghost.update_num = update_num;
        ghost.updated = true;
    }
}

void OnErr(const std::string& error_message)
{
    Log(L"UDP error: " + ToWide(error_message), LogType::Error);
}

std::wstring ToWide(const std::string& input)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(input);
}

// Performs the 32-bit FNV-1a hash function on the input wstring.
uint32_t HashW(const std::wstring& str)
{
    static_assert(sizeof(wchar_t) == 2);

    uint32_t result = 0x911c9dc5; // 32-bit FNV offset basis
    for (wchar_t wc : str)
    {
        // since wchar_t is 2 bytes wide, we must put each byte into the hash individually
        auto b1 = uint8_t(wc >> 8);
        result ^= b1;
        result *= 0x01000193; // 32-bit FNV prime

        auto b2 = uint8_t(wc);
        result ^= b2;
        result *= 0x01000193; // 32-bit FNV prime
    }
    return result;
}

// converts a uint32_t into an int64_t with the same value
int64_t ToUnrealId(const uint32_t& id)
{
    return std::bit_cast<int64_t>(uint64_t(id));
}

// Serializes src into 4 bytes of buf starting at pos and increments pos by 4.
void SerializeU32(uint32_t src, boost::array<uint8_t, 48>& buf, size_t& pos)
{
    buf[pos + 3] = uint8_t(src);
    for (int i = 2; i >= 0; i--)
    {
        src >>= 8;
        buf[pos + i] = uint8_t(src);
    }
    pos += 4;
}

// Serializes src into 4 bytes of buf starting at pos and increments pos by 4.
void SerializeF32(float src, boost::array<uint8_t, 48>& buf, size_t& pos)
{
    uint32_t src_bits = std::bit_cast<uint32_t>(src);
    SerializeU32(src_bits, buf, pos);
}

// Derializes 4 bytes of buf into a uint32_t starting at pos and increments pos by 4.
uint32_t DeserializeU32(const boost::array<uint8_t, 512>& buf, size_t& pos)
{
    uint32_t result = buf[pos];
    for (int i = 1; i < 4; i++)
    {
        result <<= 8;
        result |= buf[pos + i];
    }
    pos += 4;
    return result;
}

// Derializes 4 bytes of buf into a float starting at pos and increments pos by 4.
float DeserializeF32(const boost::array<uint8_t, 512>& buf, size_t& pos)
{
    uint32_t bits = DeserializeU32(buf, pos);
    return std::bit_cast<float>(bits);
}

} // namespace
