#pragma once

#include "EngineIncludes.h"

#include <unknwn.h>
#include <dxcapi.h>
#include <string>
#include <wrl/client.h>

DELTA_ENGINE_NS_BEGIN

DELTAENGINE_API Microsoft::WRL::ComPtr<IDxcBlob> CompileHLSLStage(
    const std::wstring& engineRelativePath,
    const std::wstring& entryPoint,
    const std::wstring& targetProfile,
    const char* debugLabel = nullptr);

DELTA_ENGINE_NS_END
