#pragma once

#include "EditorIncludes.h"

#include "Editor/EditorWindows/EditorWindow_WorldOutliner.h"

#include <vector>

DELTA_ENGINE_NS_BEGIN

DELTAEDITOR_API void BuildOutlinerFilterMatches(const std::vector<OutlinerEntry>& entries, const char* filterBuf,
    std::vector<const OutlinerEntry*>& outFiltered);

DELTA_ENGINE_NS_END
