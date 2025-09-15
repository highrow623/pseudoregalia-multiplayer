#pragma once

#include <string>

namespace Logger
{
    enum class LogType
    {
        Default,
        Loud,
        Warning,
        Error,
    };

    void Log(std::wstring, LogType = LogType::Default);
}

using Logger::Log;
using Logger::LogType;
