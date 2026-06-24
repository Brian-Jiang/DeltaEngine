#include "Runtime/Graphics/TransparentDrawEntry.h"

#include <algorithm>

using namespace DeltaEngine;

void SortTransparentDrawEntriesDescending(std::vector<TransparentDrawEntry>& entries)
{
    std::sort(entries.begin(), entries.end(),
        [](const TransparentDrawEntry& a, const TransparentDrawEntry& b)
        {
            return a.sortDepth > b.sortDepth;
        });
}
