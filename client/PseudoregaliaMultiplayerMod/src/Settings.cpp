#pragma once

#include "Settings.hpp"

#include <codecvt>
#include <fstream>
#include <iostream>

#include "toml++/toml.hpp"

#include "Logger.hpp"

namespace
{
    void ParseSetting(std::string&, toml::table, const std::string&);
    void ParseSetting(std::array<uint8_t, 3>&, toml::table, const std::string&);
    std::wstring ToWide(const std::string&);

    // if you run from the executable directory
    const std::string settings_filename1 = "Mods/PseudoregaliaMultiplayerMod/settings.toml";
    // if you run from the game directory
    const std::string settings_filename2 = "pseudoregalia/Binaries/Win64/" + settings_filename1;

    std::string address = "127.0.0.1";
    std::string port = "23432";
    std::array<uint8_t, 3> color = { 0x00, 0x7f, 0xff };
	std::string name = "Sybil";
}

void Settings::Load()
{
    std::ifstream settings_file(settings_filename1);
    if (!settings_file.good())
    {
        settings_file = std::ifstream(settings_filename2);
        if (!settings_file.good())
        {
            Log(L"Settings file not found, using default settings", LogType::Warning);
            return;
        }
    }

    toml::table settings_table;
    try
    {
        settings_table = toml::parse(settings_file);
    }
    catch (const toml::parse_error& err)
    {
        Log(L"Failed to parse settings: " + ToWide(err.what()) + L"; using default settings", LogType::Warning);
        return;
    }

    Log(L"Loading settings", LogType::Loud);
    ParseSetting(address, settings_table, "server.address");
    ParseSetting(port, settings_table, "server.port");
    ParseSetting(color, settings_table, "sybil.color");
    ParseSetting(name, settings_table, "sybil.name");
}

const std::string& Settings::GetAddress()
{
    return address;
}

const std::string& Settings::GetPort()
{
    return port;
}

const std::array<uint8_t, 3>& Settings::GetColor()
{
    return color;
}

const std::string & Settings::GetName()
{
    return name;
}

namespace
{

void ParseSetting(std::string& setting, toml::table settings_table, const std::string& setting_path)
{
    std::optional<std::string> option = settings_table.at_path(setting_path).value<std::string>();
    if (!option)
    {
        Log(ToWide(setting_path) + L" = default (setting missing or not a string)");
        return;
    }

    Log(ToWide(setting_path + " = \"" + *option + "\""));
    setting = *option;
}

// parses the setting as an rgb hex code
void ParseSetting(std::array<uint8_t, 3>& setting, toml::table settings_table, const std::string& setting_path)
{
    std::optional<std::string> option = settings_table.at_path(setting_path).value<std::string>();
    if (!option)
    {
        Log(ToWide(setting_path) + L" = default (setting missing or not a string)");
        return;
    }

    if (option->size() != 6)
    {
        Log(ToWide(setting_path) + L" = default (ill-formed hex code)");
        return;
    }

    uint8_t red;
    uint8_t green;
    uint8_t blue;

    try
    {
        red = uint8_t(std::stoi(option->substr(0, 2), nullptr, 16));
        green = uint8_t(std::stoi(option->substr(2, 2), nullptr, 16));
        blue = uint8_t(std::stoi(option->substr(4, 2), nullptr, 16));
    }
    catch (const std::invalid_argument&)
    {
        Log(ToWide(setting_path) + L" = default (ill-formed hex code)");
        return;
    }

    Log(ToWide(setting_path + " = #" + *option));
    setting = { red, green, blue };
}

std::wstring ToWide(const std::string& input)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(input);
}

} // namespace
