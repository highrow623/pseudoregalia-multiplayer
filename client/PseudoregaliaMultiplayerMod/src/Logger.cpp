#pragma once

#include "Logger.hpp"

#include <DynamicOutput/DynamicOutput.hpp>

using namespace RC;

void Logger::Log(std::wstring message, LogType log_level)
{
    auto full_message = L"[PseudoregaliaMultiplayerMod] " + message + L"\n";
    switch (log_level)
    {
    case LogType::Default:
        Output::send<LogLevel::Default>(full_message);
        break;
    case LogType::Loud:
        Output::send<LogLevel::Verbose>(full_message);
        break;
    case LogType::Warning:
        Output::send<LogLevel::Warning>(full_message);
        break;
    case LogType::Error:
        Output::send<LogLevel::Error>(full_message);
        break;
    }
}
