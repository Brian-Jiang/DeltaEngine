#include "Runtime/Graphics/ShaderCompile.h"
#include "Runtime/IO/IOManager.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>

using namespace DeltaEngine;

namespace
{
struct GpuShaderStageSpec
{
    const char* engineRelativePath;
    const char* entryPoint;
    const char* targetProfile;
};

constexpr GpuShaderStageSpec kAllRuntimeShaderStages[] = {
    { "Shaders/StandardObject.slang", "VSMain", "vs_6_6" },
    { "Shaders/StandardObject.slang", "PSMain", "ps_6_6" },
    { "Shaders/PBRObject.slang", "VSMain", "vs_6_6" },
    { "Shaders/PBRObject.slang", "PSMain", "ps_6_6" },
    { "Shaders/Skybox.slang", "VSMain", "vs_6_6" },
    { "Shaders/Skybox.slang", "PSMain", "ps_6_6" },
    { "Shaders/PostProcess_Passthrough.slang", "VSMain", "vs_6_6" },
    { "Shaders/PostProcess_Passthrough.slang", "PSMain", "ps_6_6" },
    { "Shaders/PostProcess_Tonemap.slang", "VSMain", "vs_6_6" },
    { "Shaders/PostProcess_Tonemap.slang", "PSMain", "ps_6_6" },
    { "Shaders/PostProcess_Bloom.slang", "VSMain", "vs_6_6" },
    { "Shaders/PostProcess_Bloom.slang", "PSExtract", "ps_6_6" },
    { "Shaders/PostProcess_Bloom.slang", "PSBlur", "ps_6_6" },
    { "Shaders/PostProcess_Bloom.slang", "PSComposite", "ps_6_6" },
    { "Shaders/ShadowDepth.slang", "VSMain", "vs_6_6" },
    { "Shaders/IBL_IrradianceConvolve.slang", "CSMain", "cs_6_6" },
    { "Shaders/IBL_SpecularPrefilter.slang", "CSMain", "cs_6_6" },
    { "Shaders/IBL_BrdfLut.slang", "CSMain", "cs_6_6" },
};

std::string GpuShaderStageTestName(const ::testing::TestParamInfo<GpuShaderStageSpec>& info)
{
    const std::filesystem::path path(info.param.engineRelativePath);
    return std::string(info.param.entryPoint) + "_" + path.stem().string();
}
}

class GpuShaderCompileTests : public ::testing::TestWithParam<GpuShaderStageSpec>
{
};

TEST_P(GpuShaderCompileTests, CompilesRuntimeShaderStage)
{
    const GpuShaderStageSpec& spec = GetParam();
    const std::filesystem::path fullPath = IOManager::GetEngineSourceAssetFullPath(spec.engineRelativePath);
    if (!std::filesystem::exists(fullPath))
        GTEST_SKIP() << "Shader source missing: " << spec.engineRelativePath;

    const Slang::ComPtr<ISlangBlob> blob = CompileSlangStage(
        std::filesystem::path(spec.engineRelativePath),
        spec.entryPoint,
        spec.targetProfile,
        "GpuShaderCompileTests");

    ASSERT_TRUE(blob);
    EXPECT_GT(blob->getBufferSize(), 0u);
}

INSTANTIATE_TEST_SUITE_P(
    RuntimeShaders,
    GpuShaderCompileTests,
    ::testing::ValuesIn(kAllRuntimeShaderStages),
    GpuShaderStageTestName);
