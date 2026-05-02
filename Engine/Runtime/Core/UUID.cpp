#include "Runtime/Core/UUID.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>

DELTA_ENGINE_NS_BEGIN

namespace {

std::string stripDashes(std::string str)
{
    str.erase(std::remove(str.begin(), str.end(), '-'), str.end());
    return str;
}

bool isHexString(const std::string& s)
{
    return s.size() == 32 && std::all_of(s.begin(), s.end(), [](unsigned char c) {
        return std::isxdigit(static_cast<unsigned char>(c));
    });
}

}

UUID UUID::Generate()
{
    thread_local std::mt19937_64 engine{ std::random_device{}() };
    thread_local std::uniform_int_distribution<uint64_t> dist;
    UUID u;
    u.m_high = dist(engine);
    u.m_low  = dist(engine);
    u.m_high = (u.m_high & ~(0xFULL << 12)) | (0x4ULL << 12);
    u.m_low  = (u.m_low & ~(0x3ULL << 62)) | (0x2ULL << 62);
    return u;
}

UUID UUID::Null()
{
    return UUID{ 0, 0 };
}

UUID UUID::FromString(const std::string& str)
{
    std::string hex = stripDashes(str);
    if (!isHexString(hex))
    {
        DLOG(LogSerialization, ELogLevel::Verbose,
            "UUID::FromString: invalid hex payload strippedLen={}",
            hex.size());
        return Null();
    }
    try
    {
        UUID u;
        u.m_high = std::stoull(hex.substr(0, 16), nullptr, 16);
        u.m_low  = std::stoull(hex.substr(16, 16), nullptr, 16);
        return u;
    }
    catch (...)
    {
        DLOG(LogSerialization, ELogLevel::Verbose,
            "UUID::FromString: numeric parse threw strippedLen={}",
            hex.size());
        return Null();
    }
}

std::string UUID::ToString() const
{
    char buf[32 + 1];
    int n = std::snprintf(buf, sizeof(buf), "%016llx%016llx",
        static_cast<unsigned long long>(m_high),
        static_cast<unsigned long long>(m_low));
    if (n != 32)
        return {};
    std::string s(buf, 32);
    s.insert(20, 1, '-');
    s.insert(16, 1, '-');
    s.insert(12, 1, '-');
    s.insert(8, 1, '-');
    return s;
}

DELTA_ENGINE_NS_END
