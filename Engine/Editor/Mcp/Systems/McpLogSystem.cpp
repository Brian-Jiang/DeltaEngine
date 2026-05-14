#include "McpLogSystem.h"

#include "Editor/EditorCore.h"
#include "Mcp/McpRegistry.h"
#include "Runtime/Logging/LogCategory.h"
#include "Runtime/Logging/LoggingManager.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace DeltaEngine;

namespace
{

nlohmann::json MakeError(const std::string& msg)
{
    return { {"ok", false}, {"error", msg} };
}

std::string ToLower(std::string s)
{
    for (char& c : s)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

bool ContainsCI(std::string_view haystack, std::string_view needle)
{
    if (needle.empty())
        return true;
    if (needle.size() > haystack.size())
        return false;
    const std::string h = ToLower(std::string{haystack});
    const std::string n = ToLower(std::string{needle});
    return h.find(n) != std::string::npos;
}

std::optional<ELogLevel> LevelFromString(std::string_view name)
{
    const std::string lower = ToLower(std::string{name});
    if (lower == "veryverbose" || lower == "trace")    return ELogLevel::VeryVerbose;
    if (lower == "verbose"     || lower == "debug")    return ELogLevel::Verbose;
    if (lower == "log"         || lower == "info")     return ELogLevel::Log;
    if (lower == "display")                            return ELogLevel::Display;
    if (lower == "warning"     || lower == "warn")     return ELogLevel::Warning;
    if (lower == "error"       || lower == "err")      return ELogLevel::Error;
    if (lower == "fatal"       || lower == "critical") return ELogLevel::Fatal;
    return std::nullopt;
}

const char* LevelToString(ELogLevel level)
{
    switch (level)
    {
    case ELogLevel::VeryVerbose: return "VeryVerbose";
    case ELogLevel::Verbose:     return "Verbose";
    case ELogLevel::Log:         return "Log";
    case ELogLevel::Display:     return "Display";
    case ELogLevel::Warning:     return "Warning";
    case ELogLevel::Error:       return "Error";
    case ELogLevel::Fatal:       return "Fatal";
    }
    return "Unknown";
}

std::optional<ELogLevel> LevelFromSpdlogToken(std::string_view token)
{
    const std::string lower = ToLower(std::string{token});
    if (lower == "trace")    return ELogLevel::VeryVerbose;
    if (lower == "debug")    return ELogLevel::Verbose;
    if (lower == "info")     return ELogLevel::Log;
    if (lower == "warning")  return ELogLevel::Warning;
    if (lower == "error")    return ELogLevel::Error;
    if (lower == "critical") return ELogLevel::Fatal;
    return std::nullopt;
}

struct ParsedLine
{
    std::string              raw;
    std::string              time;
    std::string              category;
    std::string              message;
    std::optional<ELogLevel> level;
};

bool ExtractBracket(std::string_view line, size_t& cursor, std::string& outContent)
{
    while (cursor < line.size() && std::isspace(static_cast<unsigned char>(line[cursor])))
        ++cursor;
    if (cursor >= line.size() || line[cursor] != '[')
        return false;
    const size_t close = line.find(']', cursor + 1);
    if (close == std::string_view::npos)
        return false;
    outContent.assign(line.data() + cursor + 1, close - cursor - 1);
    cursor = close + 1;
    return true;
}

ParsedLine ParseLine(std::string raw)
{
    ParsedLine out;
    out.raw = std::move(raw);

    std::string_view view = out.raw;
    size_t cursor = 0;

    std::string timeTok, catTok, lvlTok;
    if (!ExtractBracket(view, cursor, timeTok))
    {
        out.message = out.raw;
        return out;
    }
    if (!ExtractBracket(view, cursor, catTok))
    {
        out.message = out.raw;
        return out;
    }
    if (!ExtractBracket(view, cursor, lvlTok))
    {
        out.message = out.raw;
        return out;
    }

    while (cursor < view.size() && std::isspace(static_cast<unsigned char>(view[cursor])))
        ++cursor;

    out.time     = std::move(timeTok);
    out.category = std::move(catTok);
    out.level    = LevelFromSpdlogToken(lvlTok);
    out.message.assign(view.begin() + static_cast<ptrdiff_t>(cursor), view.end());
    return out;
}

std::optional<std::chrono::seconds> TimeOfDayDeltaSeconds(std::string_view hhmmss)
{
    int h = 0, m = 0, s = 0;
    if (hhmmss.size() < 8)
        return std::nullopt;
    auto digit = [](char c) { return c >= '0' && c <= '9'; };
    if (!digit(hhmmss[0]) || !digit(hhmmss[1]) || hhmmss[2] != ':'
        || !digit(hhmmss[3]) || !digit(hhmmss[4]) || hhmmss[5] != ':'
        || !digit(hhmmss[6]) || !digit(hhmmss[7]))
        return std::nullopt;
    h = (hhmmss[0] - '0') * 10 + (hhmmss[1] - '0');
    m = (hhmmss[3] - '0') * 10 + (hhmmss[4] - '0');
    s = (hhmmss[6] - '0') * 10 + (hhmmss[7] - '0');

    const std::time_t now = std::time(nullptr);
    std::tm tmNow{};
#if defined(_WIN32)
    localtime_s(&tmNow, &now);
#else
    localtime_r(&now, &tmNow);
#endif
    const int nowSec = tmNow.tm_hour * 3600 + tmNow.tm_min * 60 + tmNow.tm_sec;
    const int lineSec = h * 3600 + m * 60 + s;
    int delta = nowSec - lineSec;
    if (delta < 0)
        delta += 24 * 3600;
    return std::chrono::seconds{delta};
}

} // namespace

void McpLogSystem::RegisterTools(McpRegistry& registry)
{
    registry.RegisterOperation("log", "file_location",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryFileLocation(c, p); });
    registry.RegisterOperation("log", "read",
        [this](EditorCore& c, const nlohmann::json& p) { return QueryRead(c, p); });
}

nlohmann::json McpLogSystem::QueryFileLocation(EditorCore&, const nlohmann::json&)
{
    const std::filesystem::path path = LoggingManager::GetCurrentLogFilePath();
    if (path.empty())
        return MakeError("logging not initialized");

    nlohmann::json result;
    result["ok"]   = true;
    result["path"] = path.generic_string();

    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    result["exists"] = exists && !ec;
    if (result["exists"].get<bool>())
    {
        const auto sz = std::filesystem::file_size(path, ec);
        result["size_bytes"] = ec ? 0ull : static_cast<uint64_t>(sz);
    }
    else
    {
        result["size_bytes"] = 0ull;
    }
    return result;
}

nlohmann::json McpLogSystem::QueryRead(EditorCore&, const nlohmann::json& params)
{
    const std::filesystem::path path = LoggingManager::GetCurrentLogFilePath();
    if (path.empty())
        return MakeError("logging not initialized");

    ELogLevel minSeverity = ELogLevel::Log;
    if (params.contains("min_severity") && params["min_severity"].is_string())
    {
        auto parsed = LevelFromString(params["min_severity"].get<std::string>());
        if (!parsed.has_value())
            return MakeError("invalid min_severity (expected VeryVerbose|Verbose|Log|Display|Warning|Error|Fatal)");
        minSeverity = *parsed;
    }

    int count = 200;
    if (params.contains("count") && params["count"].is_number_integer())
        count = params["count"].get<int>();
    count = std::clamp(count, 1, 5000);

    std::string textSearch;
    if (params.contains("text_search") && params["text_search"].is_string())
        textSearch = params["text_search"].get<std::string>();

    std::string categoryFilter;
    if (params.contains("category") && params["category"].is_string())
        categoryFilter = params["category"].get<std::string>();

    std::optional<std::chrono::seconds> sinceLimit;
    if (params.contains("since_seconds") && params["since_seconds"].is_number_integer())
    {
        const int secs = params["since_seconds"].get<int>();
        if (secs >= 0)
            sinceLimit = std::chrono::seconds{secs};
    }

    spdlog::apply_all([](std::shared_ptr<spdlog::logger> logger) { logger->flush(); });

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open())
        return MakeError("failed to open log file: " + path.generic_string());

    std::vector<std::string> allLines;
    allLines.reserve(1024);
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        allLines.push_back(std::move(line));
    }

    std::vector<nlohmann::json> picked;
    picked.reserve(static_cast<size_t>(count));

    for (auto it = allLines.rbegin(); it != allLines.rend(); ++it)
    {
        if (static_cast<int>(picked.size()) >= count)
            break;

        ParsedLine pl = ParseLine(*it);

        if (pl.level.has_value() && static_cast<uint8_t>(*pl.level) < static_cast<uint8_t>(minSeverity))
            continue;

        if (!categoryFilter.empty() && !ContainsCI(pl.category, categoryFilter))
            continue;

        if (!textSearch.empty() && !ContainsCI(pl.raw, textSearch))
            continue;

        if (sinceLimit.has_value())
        {
            auto delta = TimeOfDayDeltaSeconds(pl.time);
            if (!delta.has_value() || *delta > *sinceLimit)
                continue;
        }

        nlohmann::json entry;
        entry["time"]     = pl.time;
        entry["category"] = pl.category;
        entry["level"]    = pl.level.has_value() ? LevelToString(*pl.level) : "Unknown";
        entry["message"]  = pl.message;
        picked.push_back(std::move(entry));
    }

    std::reverse(picked.begin(), picked.end());

    nlohmann::json result;
    result["ok"]      = true;
    result["path"]    = path.generic_string();
    result["entries"] = std::move(picked);
    return result;
}
