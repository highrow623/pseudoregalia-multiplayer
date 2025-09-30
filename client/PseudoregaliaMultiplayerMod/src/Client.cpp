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
    const size_t STATE_LEN = 24;
    const size_t MAX_STATES_PER_PACKET = 21;
    const size_t MIN_SERVER_PACKET_LEN = STATE_LEN;
    const size_t MAX_SERVER_PACKET_LEN = MAX_STATES_PER_PACKET * STATE_LEN;

    const size_t SEND = MIN_SERVER_PACKET_LEN;
    const size_t RECV = MAX_SERVER_PACKET_LEN;

    void OnOpen();
    void OnClose();
    void OnMessage(const std::string&);
    void OnError(const std::string&);

    void OnRecv(const boost::array<uint8_t, RECV>&, size_t);
    void OnErr(const std::string&);

    std::wstring ToWide(const std::string&);
    uint32_t HashW(const std::wstring&);
    std::chrono::steady_clock::time_point AdvanceNanos();
    bool TrySendUpdate(const FST_PlayerInfo&);
    void SendUpdate(const FST_PlayerInfo&);

    void SerializeU8(uint8_t, boost::array<uint8_t, SEND>&, size_t&);
    void SerializeU32(uint32_t, boost::array<uint8_t, SEND>&, size_t&);
    void SerializeF32(float, boost::array<uint8_t, SEND>&, size_t&);
    void SerializeLocator(double, boost::array<uint8_t, SEND>&, size_t&);
    void SerializeRotator(double, boost::array<uint8_t, SEND>&, size_t&);

    uint8_t DeserializeU8(const boost::array<uint8_t, RECV>&, size_t&);
    uint32_t DeserializeU32(const boost::array<uint8_t, RECV>&, size_t&);
    float DeserializeF32(const boost::array<uint8_t, RECV>&, size_t&);
    double DeserializeLocator(const boost::array<uint8_t, RECV>&, size_t&);
    double DeserializeRotator(const boost::array<uint8_t, RECV>&, size_t&);

    bool queue_connect = false;
    bool queue_disconnect = false;
    wswrap::WS* ws = nullptr;
    UdpSocket::UdpSocket<SEND, RECV>* udp = nullptr;

    const size_t MAX_STATES = 20;
    const size_t MAX_OFFSETS = 100;

    // some value in milliseconds to buffer when calculating update number to use for ghosts; causes delay, which can
    // allow more time for packets to arrive
    // TODO make this configurable, or auto calculate per ghost?
    const int64_t UPDATE_NUM_BUFFER = 100;

    struct State
    {
        FST_PlayerInfo info;
        uint32_t zone;
        uint32_t update_num;
    };

    struct Ghost
    {
        uint8_t id = 0;
        std::list<State> states;

        // offsets provide a way to figure out syncing. the offset is meant to describe how far off a player's update
        // number is from our own. these two fields let us easily check the average offset over the last MAX_OFFSETS
        // messages received
        int64_t total_offset = 0;
        std::deque<uint64_t> offsets;

        State cached_state{};

        bool can_insert(uint32_t update_num) const
        {
            auto eq = [&](const State& state) { return state.update_num == update_num; };
            return std::find_if(states.begin(), states.end(), eq) == states.end()
                && (states.size() < MAX_STATES || states.front().update_num < update_num);
        }

        // should only be called if can_insert returns true; otherwise states can include duplicates or this function
        // can be unnecessarily called with a state that would be dropped anyway
        void insert(State& s, const uint32_t& update_num)
        {
            // this is a new latest state, so update offset calculation
            if (states.size() == 0 || s.update_num > states.back().update_num)
            {
                int64_t offset = int64_t(s.update_num) - int64_t(update_num);
                total_offset += offset;
                offsets.push_back(offset);
                if (offsets.size() > MAX_OFFSETS)
                {
                    total_offset -= offsets.front();
                    offsets.pop_front();
                }
            }

            s.info.id = id;

            // insert after the first element that has a lower update_num to keep the list sorted
            // reverse find because we're more likely to be inserting towards the back of the list
            auto less = [&](const State& state) { return state.update_num < s.update_num; };
            auto it = std::find_if(states.rbegin(), states.rend(), less);
            states.insert(it.base(), s);

            if (states.size() > MAX_STATES)
            {
                states.pop_front();
            }
        }

        const State& get_state() const
        {
            return cached_state;
        }

        std::optional<State> refresh_state(const uint32_t& update_num)
        {
            if (states.size() == 0 || offsets.size() == 0)
            {
                return {};
            }

            int64_t average_offset = total_offset / int64_t(offsets.size());
            uint32_t ghost_update_num = uint32_t(int64_t(update_num) + average_offset - UPDATE_NUM_BUFFER);
            cached_state = get_closest(ghost_update_num);
            return cached_state;
        }

        State get_closest(const uint32_t& update_num) const
        {
            if (update_num <= states.front().update_num)
            {
                return states.front();
            }
            if (update_num >= states.back().update_num)
            {
                return states.back();
            }

            auto ge = [&](const State& state) { return state.update_num >= update_num; };
            auto it = std::find_if(states.cbegin(), states.cend(), ge);
            const State& higher = *it;
            --it;
            const State& lower = *it;
            // TODO interpolate higher and lower based on distance to update_num
            return (update_num - lower.update_num < higher.update_num - update_num) ? lower : higher;
        }
    };

    uint32_t current_zone;
    std::optional<FST_PlayerInfo> player_info = {};

    // the id given in the Connected message; this value being defined means a full connection has been established
    std::optional<uint8_t> id = {};
    uint32_t update_num = 0;
    std::unordered_map<uint8_t, Ghost> ghosts = {};
    std::unordered_set<uint8_t> spawned_ghosts = {};

    // about 1/60 seconds, in nanoseconds because that's what steady_clock uses
    const int64_t NANOS_PER_UPDATE = 16666667;
    // marks the time the first update was sent after connecting
    std::optional<std::chrono::steady_clock::time_point> start = {};
    // marks the last time the client checked if it could send an update; used to increment nanos
    std::optional<std::chrono::steady_clock::time_point> time_stamp = {};
    // keeps track of nanoseconds accrued for updates, an update can only be only fired if it exceeds NANOS_PER_UPDATE
    int64_t nanos = 0;
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

            time_stamp.reset();
            start.reset();
            nanos = 0;
        }
        queue_disconnect = false;
    }
    if (queue_connect)
    {
        if (!ws)
        {
            const auto& address = Settings::GetAddress();
            const auto& port = Settings::GetPort();
            auto uri = "ws://" + address + ":" + std::to_string(port);
            try
            {
                ws = new wswrap::WS(uri, OnOpen, OnClose, OnMessage, OnError);
            }
            catch (const std::exception& ex)
            {
                ws = nullptr;
                Log(L"Error creating WebSocket: " + ToWide(ex.what()), LogType::Error);
            }
            try
            {
                udp = new UdpSocket::UdpSocket<SEND, RECV>(address, port, OnRecv, OnErr);
            }
            catch (const std::exception& ex)
            {
                delete ws;
                ws = nullptr;
                udp = nullptr;
                Log(L"Error creating UDP socket: " + ToWide(ex.what()), LogType::Error);
            }
        }
        queue_connect = false;
    }
    if (id && time_stamp)
    {
        AdvanceNanos();
        if (player_info)
        {
            bool sent = TrySendUpdate(*player_info);
            if (sent)
            {
                player_info.reset();
            }
        }
    }
    if (ws)
    {
        ws->poll();
        udp->Poll();
    }
}

void Client::SetPlayerInfo(const FST_PlayerInfo& info)
{
    if (!id)
    {
        return;
    }
    if (!start)
    {
        start = std::chrono::steady_clock::now();
        time_stamp = start;
        SendUpdate(info);
    }
    else
    {
        auto now = AdvanceNanos();
        // convert nanos since start to millis since start
        update_num = uint32_t((now - *start).count() / 1000000);

        bool sent = TrySendUpdate(info);
        if (!sent)
        {
            player_info = info;
        }
    }
}

void Client::GetGhostInfo(RC::Unreal::TArray<FST_PlayerInfo>& ghost_info, RC::Unreal::TArray<uint8_t>& to_remove)
{
    for (auto& [id, ghost] : ghosts)
    {
        const auto& state = ghost.refresh_state(update_num);
        if (!state || state->zone != current_zone)
        {
            continue;
        }

        ghost_info.Add(state->info);
        spawned_ghosts.insert(id);
    }

    for (auto it = spawned_ghosts.begin(); it != spawned_ghosts.end(); )
    {
        if (!ghosts.contains(*it) || ghosts.at(*it).get_state().zone != current_zone)
        {
            to_remove.Add(*it);
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
    // set this here in case we return early
    queue_disconnect = true;

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

    auto& field_type = j["type"];
    if (!field_type.is_string())
    {
        Log(L"Received message with non-string type field: " + ToWide(message), LogType::Warning);
        return;
    }

    if (field_type == "Connected")
    {
        if (id)
        {
            Log(L"Received Connected message after connection was already established", LogType::Warning);
            return;
        }

        if (!j.contains("id"))
        {
            Log(L"Received Connected message with no id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto& field_id = j["id"];
        if (!field_id.is_number_unsigned())
        {
            Log(L"Received Connected message with invalid id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto long_id = field_id.template get<uint64_t>();
        if (long_id > 0xFF)
        {
            Log(L"Received Connected message with out-of-range id field: " + ToWide(message), LogType::Warning);
            return;
        }
        id = uint8_t(long_id);

        if (!j.contains("players"))
        {
            Log(L"Received Connected message with no players field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto& field_players = j["players"];
        if (!field_players.is_array())
        {
            Log(L"Received Connected message with non-array players field: " + ToWide(message), LogType::Warning);
            return;
        }

        for (auto it = field_players.begin(); it != field_players.end(); ++it)
        {
            if (!it->is_number_unsigned())
            {
                Log(L"Received Connected message with invalid player id: " + ToWide(message), LogType::Warning);
                return;
            }

            auto long_player_id = it->template get<uint64_t>();
            if (long_player_id > 0xFF)
            {
                Log(L"Received Connected message with out-of-range player id: " + ToWide(message), LogType::Warning);
                return;
            }

            auto player_id = uint8_t(long_player_id);
            ghosts[player_id] = Ghost{ .id = player_id };
        }
 
        Log(L"Received Connected message with player id " + std::to_wstring(*id), LogType::Loud);
    }
    else if (field_type == "PlayerJoined")
    {
        if (!id)
        {
            Log(L"Received PlayerJoined message before Connected message", LogType::Warning);
            return;
        }

        if (!j.contains("id"))
        {
            Log(L"Received PlayerJoined message with no id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto& field_id = j["id"];
        if (!field_id.is_number_unsigned())
        {
            Log(L"Received PlayerJoined message with invalid id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto long_player_id = field_id.template get<uint64_t>();
        if (long_player_id > 0xFF)
        {
            Log(L"Received PlayerJoined message with out-of-range id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto player_id = uint8_t(long_player_id);
        ghosts[player_id] = Ghost{ .id = player_id };
        Log(L"Received PlayerJoined message with id " + std::to_wstring(player_id), LogType::Loud);
    }
    else if (field_type == "PlayerLeft")
    {
        if (!id)
        {
            Log(L"Received PlayerLeft message before Connected message", LogType::Warning);
            return;
        }

        if (!j.contains("id"))
        {
            Log(L"Received PlayerLeft message with no id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto& field_id = j["id"];
        if (!field_id.is_number_unsigned())
        {
            Log(L"Received PlayerLeft message with invalid id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto long_player_id = field_id.template get<uint64_t>();
        if (long_player_id > 0xFF)
        {
            Log(L"Received PlayerLeft message with out-of-range id field: " + ToWide(message), LogType::Warning);
            return;
        }

        auto player_id = uint8_t(long_player_id);
        ghosts.erase(player_id);
        Log(L"Received PlayerLeft message with id " + std::to_wstring(player_id), LogType::Loud);
    }
    else
    {
        Log(L"Received message with unknown type: " + ToWide(field_type), LogType::Warning);
        return;
    }

    queue_disconnect = false;
}

void OnError(const std::string& error_message)
{
    Log(L"WebSocket error: " + ToWide(error_message), LogType::Error);
}

void OnRecv(const boost::array<uint8_t, RECV>& buf, size_t len)
{
    if (len < MIN_SERVER_PACKET_LEN || len > MAX_SERVER_PACKET_LEN || len % STATE_LEN != 0)
    {
        Log(L"Received packet of invalid size " + std::to_wstring(len), LogType::Warning);
        return;
    }

    size_t pos = 0;
    size_t num_updates = len / STATE_LEN;
    for (size_t i = 0; i < num_updates; i++)
    {
        uint8_t player_id = DeserializeU8(buf, pos);
        if (!ghosts.contains(player_id))
        {
            // skip pos ahead the bytes it would have read for this player
            pos += 19;
            continue;
        }
        auto& ghost = ghosts.at(player_id);

        uint32_t ghost_update_num = DeserializeU32(buf, pos);
        if (!ghost.can_insert(ghost_update_num))
        {
            pos += 15;
            continue;
        }

        State state{};
        state.update_num = ghost_update_num;
        state.zone = DeserializeU32(buf, pos);
        state.info.location_x = DeserializeLocator(buf, pos);
        state.info.location_y = DeserializeLocator(buf, pos);
        state.info.location_z = DeserializeLocator(buf, pos);
        state.info.rotation_x = DeserializeRotator(buf, pos);
        state.info.rotation_y = DeserializeRotator(buf, pos);
        state.info.rotation_z = DeserializeRotator(buf, pos);
        ghost.insert(state, update_num);
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

// Serializes src into 1 byte of buf starting at pos and increments pos by 1.
void SerializeU8(uint8_t src, boost::array<uint8_t, SEND>& buf, size_t& pos)
{
    buf[pos] = src;
    pos += 1;
}

// Serializes src into 4 bytes of buf starting at pos and increments pos by 4.
void SerializeU32(uint32_t src, boost::array<uint8_t, SEND>& buf, size_t& pos)
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
void SerializeF32(float src, boost::array<uint8_t, SEND>& buf, size_t& pos)
{
    uint32_t src_bits = std::bit_cast<uint32_t>(src);
    SerializeU32(src_bits, buf, pos);
}

// Casts src to a float, then serializes that into 4 bytes of buf starting at pos and increments pos by 4.
void SerializeLocator(double src, boost::array<uint8_t, SEND>& buf, size_t& pos)
{
    SerializeF32(float(src), buf, pos);
}

// Maps src from the range [-180.0, 180.0] to [0, 255], serializes that into 1 byte of buf starting at pos, and
// increments pos by 1.
void SerializeRotator(double src, boost::array<uint8_t, SEND>& buf, size_t& pos)
{
    double scaled = (src + 180.0) * 256.0 / 360.0;
    SerializeU8(uint8_t(scaled), buf, pos);
}

// Deserializes 1 byte of buf into a uint8_t starting at pos and increments pos by 1.
uint8_t DeserializeU8(const boost::array<uint8_t, RECV>& buf, size_t& pos)
{
    uint8_t result = buf[pos];
    pos += 1;
    return result;
}

// Deserializes 4 bytes of buf into a uint32_t starting at pos and increments pos by 4.
uint32_t DeserializeU32(const boost::array<uint8_t, RECV>& buf, size_t& pos)
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

// Deserializes 4 bytes of buf into a float starting at pos and increments pos by 4.
float DeserializeF32(const boost::array<uint8_t, RECV>& buf, size_t& pos)
{
    uint32_t bits = DeserializeU32(buf, pos);
    return std::bit_cast<float>(bits);
}

// Deserializes 4 bytes of buf into a float starting at pos and increments pos by 4, then casts the float to a double.
double DeserializeLocator(const boost::array<uint8_t, RECV>& buf, size_t& pos)
{
    return double(DeserializeF32(buf, pos));
}

// Deserializes 1 byte of buf starting at pos and increments pos by 1, then maps that byte to the range [-180.0, 180.0].
double DeserializeRotator(const boost::array<uint8_t, RECV>& buf, size_t& pos)
{
    uint8_t byte = DeserializeU8(buf, pos);
    return double(byte) * 360.0 / 256.0 - 180.0;
}

std::chrono::steady_clock::time_point AdvanceNanos()
{
    auto now = std::chrono::steady_clock::now();
    nanos += (now - *time_stamp).count();
    time_stamp = now;
    return now;
}

bool TrySendUpdate(const FST_PlayerInfo& info)
{
    if (nanos / NANOS_PER_UPDATE)
    {
        nanos = nanos % NANOS_PER_UPDATE;
        SendUpdate(info);
        return true;
    }
    return false;
}

void SendUpdate(const FST_PlayerInfo& info)
{
    boost::array<uint8_t, SEND> buf{};
    size_t pos = 0;
    SerializeU8(*id, buf, pos);
    SerializeU32(update_num, buf, pos);
    SerializeU32(current_zone, buf, pos);
    SerializeLocator(info.location_x, buf, pos);
    SerializeLocator(info.location_y, buf, pos);
    SerializeLocator(info.location_z, buf, pos);
    SerializeRotator(info.rotation_x, buf, pos);
    SerializeRotator(info.rotation_y, buf, pos);
    SerializeRotator(info.rotation_z, buf, pos);
    udp->Send(buf);
}

} // namespace
