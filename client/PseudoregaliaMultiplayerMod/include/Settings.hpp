#pragma once

#include <string>

namespace Settings
{
    void Load();
    const std::string& GetAddress();
    const std::string& GetPort();
}
