#pragma once

#include "EngineIncludes.h"

#include <unknwn.h>
#include <dxcapi.h>
#include <slang.h>
#include <slang-com-ptr.h>
#include <string>
#include <wrl/client.h>

DELTA_ENGINE_NS_BEGIN

DELTAENGINE_API Microsoft::WRL::ComPtr<IDxcBlob> CompileHLSLStage(
    const std::wstring& engineRelativePath,
    const std::wstring& entryPoint,
    const std::wstring& targetProfile,
    const char* debugLabel = nullptr);

/** Compiles a single stage from a .slang file via the Slang C++ API.
 *  engineRelativePath: path relative to the engine source-asset root (e.g. "Shaders/StandardObject.slang").
 *  entryPoint:         stage entry-point name (e.g. "VSMain").
 *  targetProfile:      SM profile string (e.g. "sm_6_6", "vs_6_6", "ps_6_6" — stage prefixes are accepted and stripped).
 */
DELTAENGINE_API Slang::ComPtr<ISlangBlob> CompileSlangStage(
    const std::wstring& engineRelativePath,
    const std::wstring& entryPoint,
    const std::wstring& targetProfile,
    const char* debugLabel = nullptr);

DELTA_ENGINE_NS_END
