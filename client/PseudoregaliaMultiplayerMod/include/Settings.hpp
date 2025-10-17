#pragma once

#include <array>
#include <string>
#include <Unreal/FString.hpp>

namespace Settings
{
    void Load();
    const std::string& GetAddress();
    const std::string& GetPort();
    const std::array<uint8_t, 3>& GetColor();
    const RC::Unreal::FString& GetName();
    const std::string& GetNameStr();
}
