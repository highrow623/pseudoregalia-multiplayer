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
    std::wstring ToWide(const std::string&);

    // if you run from the executable directory
    const std::string settings_filename1 = "Mods/PseudoregaliaMultiplayerMod/settings.toml";
    // if you run from the game directory
    const std::string settings_filename2 = "pseudoregalia/Binaries/Win64/" + settings_filename1;

    std::string uri = "ws://127.0.0.1:8080";
}

void Settings::Load()
{
    std::ifstream settings_file(settings_filename1);
    if (!settings_file.good())
    {
        settings_file = std::ifstream(settings_filename2);
        if (!settings_file.good())
        {
            Log(L"Settings file not found, using default settings");
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
        Log(L"Failed to parse settings: " + ToWide(err.what()));
        Log(L"Using default settings");
        return;
    }

    Log(L"Loading settings");
    ParseSetting(uri, settings_table, "uri");
}

const std::string& Settings::GetURI()
{
    return uri;
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

    Log(ToWide(setting_path + " = " + *option));
    setting = *option;
}

std::wstring ToWide(const std::string& input)
{
    static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(input);
}

} // namespace
