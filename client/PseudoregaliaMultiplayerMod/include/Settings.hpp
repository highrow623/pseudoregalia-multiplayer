#pragma once

#include <array>
#include <string>

namespace Settings
{
    void Load();
    const std::string& GetAddress();
    const std::string& GetPort();
    const std::array<uint8_t, 3>& GetColor();
}
