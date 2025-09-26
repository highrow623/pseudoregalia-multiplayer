#pragma once

#include <string>

namespace Settings
{
    void Load();
    const std::string& GetAddress();
    const uint16_t& GetPort();
}
