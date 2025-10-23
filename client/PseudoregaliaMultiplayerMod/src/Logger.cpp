#pragma once

#include "Logger.hpp"

#include <DynamicOutput/DynamicOutput.hpp>

void Logger::Log(std::wstring message, LogType log_level)
{
    auto full_message = L"[PseudoregaliaMultiplayerMod] " + message + L"\n";
    switch (log_level)
    {
    case LogType::Default:
        RC::Output::send<RC::LogLevel::Default>(full_message);
        break;
    case LogType::Loud:
        RC::Output::send<RC::LogLevel::Verbose>(full_message);
        break;
    case LogType::Warning:
        RC::Output::send<RC::LogLevel::Warning>(full_message);
        break;
    case LogType::Error:
        RC::Output::send<RC::LogLevel::Error>(full_message);
        break;
    }
}
