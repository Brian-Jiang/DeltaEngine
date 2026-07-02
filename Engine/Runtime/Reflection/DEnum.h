#pragma once

#include "EngineIncludes.h"

#include <cstdint>
#include <string>
#include <vector>

DELTA_ENGINE_NS_BEGIN

struct DEnumEntry
{
    std::string name;
    int64_t value;
};

class DELTAENGINE_API DEnum
{
public:
    DEnum(std::string name, std::string underlyingType);

    void AddEntry(std::string name, int64_t value);

    const std::string& GetName() const { return m_name; }
    const std::string& GetUnderlyingType() const { return m_underlyingType; }
    const std::vector<DEnumEntry>& GetEntries() const { return m_entries; }

    const DEnumEntry* FindEntryByValue(int64_t value) const;

private:
    std::string m_name;
    std::string m_underlyingType;
    std::vector<DEnumEntry> m_entries;
};

DELTA_ENGINE_NS_END
