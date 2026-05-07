#include "Runtime/Utils/StringUtils.h"

#include <windows.h>

using namespace DeltaEngine;

std::string StringUtils::WStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty())
        return {};

    const int size = ::WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.data(), static_cast<int>(wstr.size()),
        nullptr, 0,
        nullptr, nullptr);
    if (size <= 0)
    {
        DLOG(LogCore, ELogLevel::Error,
            "StringUtils::WStringToUtf8 sizing failed: input length={}, GetLastError={}",
            wstr.size(), ::GetLastError());
        return {};
    }

    std::string result(static_cast<size_t>(size), '\0');
    const int written = ::WideCharToMultiByte(
        CP_UTF8, 0,
        wstr.data(), static_cast<int>(wstr.size()),
        result.data(), size,
        nullptr, nullptr);
    if (written <= 0)
    {
        DLOG(LogCore, ELogLevel::Error,
            "StringUtils::WStringToUtf8 conversion failed: input length={}, GetLastError={}",
            wstr.size(), ::GetLastError());
        return {};
    }
    return result;
}

std::wstring StringUtils::Utf8ToWString(const std::string& str)
{
    if (str.empty())
        return {};

    const int size = ::MultiByteToWideChar(
        CP_UTF8, 0,
        str.data(), static_cast<int>(str.size()),
        nullptr, 0);
    if (size <= 0)
    {
        DLOG(LogCore, ELogLevel::Error,
            "StringUtils::Utf8ToWString sizing failed: input length={}, GetLastError={}",
            str.size(), ::GetLastError());
        return {};
    }

    std::wstring result(static_cast<size_t>(size), L'\0');
    const int written = ::MultiByteToWideChar(
        CP_UTF8, 0,
        str.data(), static_cast<int>(str.size()),
        result.data(), size);
    if (written <= 0)
    {
        DLOG(LogCore, ELogLevel::Error,
            "StringUtils::Utf8ToWString conversion failed: input length={}, GetLastError={}",
            str.size(), ::GetLastError());
        return {};
    }
    return result;
}

std::filesystem::path StringUtils::Utf8ToPath(const std::string& utf8)
{
    return std::filesystem::path(reinterpret_cast<const char8_t*>(utf8.data()),
                                 reinterpret_cast<const char8_t*>(utf8.data() + utf8.size()));
}

std::string StringUtils::PathToUtf8(const std::filesystem::path& p)
{
    const std::u8string u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}
