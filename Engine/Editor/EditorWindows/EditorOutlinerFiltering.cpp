#include "Editor/EditorWindows/EditorOutlinerFiltering.h"

#include <cctype>
#include <cstring>
#include <string>

DELTA_ENGINE_NS_BEGIN

void BuildOutlinerFilterMatches(const std::vector<OutlinerEntry>& entries, const char* filterBuf,
    std::vector<const OutlinerEntry*>& outFiltered)
{
    outFiltered.clear();
    outFiltered.reserve(entries.size());

    if (!filterBuf || filterBuf[0] == '\0')
    {
        for (const auto& e : entries)
            outFiltered.push_back(&e);
        return;
    }

    auto toLower = [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); };
    std::string needle;
    needle.reserve(std::strlen(filterBuf));
    for (const char* p = filterBuf; *p; ++p)
        needle += toLower(static_cast<unsigned char>(*p));

    for (const auto& e : entries)
    {
        std::string haystack;
        haystack.reserve(e.name.size());
        for (unsigned char ch : e.name)
            haystack += toLower(ch);

        if (haystack.find(needle) != std::string::npos)
            outFiltered.push_back(&e);
    }
}

DELTA_ENGINE_NS_END
