#include "Shared/GpuD3D12Validation.h"
#include "Shared/GpuReadback.h"
#include "Shared/GpuSceneBuilder.h"
#include "Shared/GpuTestAssetFixture.h"

#include "Runtime/Core/Camera.h"
#include "Runtime/Graphics/DirectX/RenderTarget.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Graphics/RenderGraph/RenderGraph.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

using namespace DeltaEngine;
using namespace DeltaEngine::Tests;

namespace
{
constexpr uint32_t kHaltonPeriod = 8;
constexpr uint32_t kConvergenceFrames = 64;
constexpr uint32_t kWarmupFrames = 4;
/** Mean display-space |TAA - reference|; bilinear unjitter leaves the converged image slightly soft. */
constexpr double kMeanTolerance = 0.002;
/** Luma range across the jitter cycle above which a pixel counts as an aliased edge. */
constexpr float kEdgeFlickerThreshold = 0.02f;
constexpr uint32_t kMinEdgePixels = 10;
/** Converged TAA must flicker at most this fraction of the raw jittered frames on edges. */
constexpr double kFlickerRatio = 0.25;

float DisplayLuma(const GpuReadbackPixel& pixel)
{
    return 0.2126f * pixel.r + 0.7152f * pixel.g + 0.0722f * pixel.b;
}

/** Per-pixel mean and luma range over a sequence of images. */
struct ImageSequenceStats
{
    std::vector<GpuReadbackPixel> mean;
    std::vector<float> minLuma;
    std::vector<float> maxLuma;
    uint32_t count = 0;

    void Add(const GpuReadbackImage& image)
    {
        if (count == 0)
        {
            mean.assign(image.pixels.size(), GpuReadbackPixel{});
            minLuma.assign(image.pixels.size(), FLT_MAX);
            maxLuma.assign(image.pixels.size(), -FLT_MAX);
        }
        ++count;
        const float w = 1.0f / static_cast<float>(count);
        for (size_t p = 0; p < mean.size(); ++p)
        {
            const GpuReadbackPixel& px = image.pixels[p];
            mean[p].r += (px.r - mean[p].r) * w;
            mean[p].g += (px.g - mean[p].g) * w;
            mean[p].b += (px.b - mean[p].b) * w;
            const float luma = DisplayLuma(px);
            minLuma[p] = (std::min)(minLuma[p], luma);
            maxLuma[p] = (std::max)(maxLuma[p], luma);
        }
    }

    float Flicker(size_t p) const { return maxLuma[p] - minLuma[p]; }
};

/** Maps HDR colour to x / (1 + luma), the space TAA's brightness weighting averages in. */
void ToDisplaySpace(GpuReadbackImage& image)
{
    for (GpuReadbackPixel& pixel : image.pixels)
    {
        const float scale = 1.0f / (1.0f + 0.2126f * pixel.r + 0.7152f * pixel.g + 0.0722f * pixel.b);
        pixel.r *= scale;
        pixel.g *= scale;
        pixel.b *= scale;
    }
}

float PixelDifference(const GpuReadbackPixel& a, const GpuReadbackPixel& b)
{
    return (std::fabs(a.r - b.r) + std::fabs(a.g - b.g) + std::fabs(a.b - b.b)) / 3.0f;
}

std::optional<size_t> FindCompiledPass(const RenderGraph& graph, const std::string& name)
{
    for (size_t i = 0; i < graph.GetCompiledPassCount(); ++i)
    {
        if (name == graph.GetPass(graph.GetCompiledPass(i).passIndex).GetName())
            return i;
    }
    return std::nullopt;
}

void ExpectBitEqual(const GpuReadbackImage& expected, const GpuReadbackImage& actual)
{
    ASSERT_EQ(expected.width, actual.width);
    ASSERT_EQ(expected.height, actual.height);
    for (uint32_t y = 0; y < expected.height; ++y)
    {
        for (uint32_t x = 0; x < expected.width; ++x)
        {
            const GpuReadbackPixel& e = expected.At(x, y);
            const GpuReadbackPixel& a = actual.At(x, y);
            ASSERT_TRUE(e.r == a.r && e.g == a.g && e.b == a.b && e.a == a.a)
                << "x=" << x << " y=" << y << " expected (" << e.r << ", " << e.g << ", " << e.b << ", " << e.a
                << ") actual (" << a.r << ", " << a.g << ", " << a.b << ", " << a.a << ")";
        }
    }
}
} // namespace

class GpuTemporalResolveTests : public GpuTestAssetFixture, public ::testing::WithParamInterface<RenderPath>
{
protected:
    RenderPath GetInitialRenderPath() const override { return GetParam(); }

    /** Two lit opaque spheres; deferred + skybox faults on WARP, so no skybox. */
    void BuildScene(GpuSceneBuilder& builder)
    {
        m_camera = builder.AddCamera();
        ASSERT_NE(m_camera, nullptr);
        ASSERT_NE(builder.AddDirectionalLightWithShadows(), nullptr);
        ASSERT_NE(builder.AddOpaqueUnitSphere("SphereA", -1.2f, 0.0f, 0.0f), nullptr);
        ASSERT_NE(builder.AddOpaqueUnitSphere("SphereB", 1.4f, 0.6f, 1.0f), nullptr);
    }

    void Read(const std::shared_ptr<DirectX12Texture>& texture, GpuReadbackImage& image)
    {
        ASSERT_NE(texture, nullptr);
        ASSERT_EQ(texture->GetD3D12ResourceDesc().SampleDesc.Count, 1u);
        ASSERT_TRUE(ReadTexturePixels(*GetDevice(), GetDirectQueue(), texture, image));
        ASSERT_GT(image.width, 0u);
    }

    /** TAA output of the last frame (read/write roles have already swapped). */
    void ReadTemporalOutput(GpuReadbackImage& image) { Read(GetRenderManager().GetTemporalHistory().GetReadTexture(), image); }

    void ReadSceneColor(GpuReadbackImage& image) { Read(GetRenderTarget()->GetTexture(AttachmentPoint::Color0), image); }

    bool FrameHasTemporalResolve() { return FindCompiledPass(GetRenderManager().GetFrameGraph(), "TemporalResolve").has_value(); }

    Camera* m_camera = nullptr;
};

TEST_P(GpuTemporalResolveTests, ResetFrame_OutputsCurrentFrameUnchanged)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NO_FATAL_FAILURE(BuildScene(builder));
    builder.InitGpuResources();

    DXRenderManager& renderManager = GetRenderManager();
    renderManager.SetTemporalEnabled(true);
    RenderSceneFrames(kWarmupFrames);
    ASSERT_TRUE(FrameHasTemporalResolve());

    renderManager.RequestTemporalHistoryReset();
    RenderSceneFrame();
    ASSERT_TRUE(renderManager.GetTemporalCameraState().historyResetThisFrame);
    ASSERT_TRUE(FrameHasTemporalResolve());

    GpuReadbackImage scene;
    GpuReadbackImage output;
    ASSERT_NO_FATAL_FAILURE(ReadSceneColor(scene));
    ASSERT_NO_FATAL_FAILURE(ReadTemporalOutput(output));
    ExpectBitEqual(scene, output);
    EXPECT_TRUE(renderManager.GetTemporalHistory().IsValid());
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_P(GpuTemporalResolveTests, StaticScene_ConvergesToEightSampleReference)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NO_FATAL_FAILURE(BuildScene(builder));
    builder.InitGpuResources();

    DXRenderManager& renderManager = GetRenderManager();
    renderManager.SetTemporalEnabled(true);
    RenderSceneFrames(kWarmupFrames);

    // Reference: the 8 Halton-jittered frames, each rendered as a reset frame (raw scene colour) and averaged.
    ImageSequenceStats reference;
    for (uint32_t i = 0; i < kHaltonPeriod; ++i)
    {
        renderManager.RequestTemporalHistoryReset();
        renderManager.GetTemporalCameraState().haltonIndex = i;
        RenderSceneFrame();

        GpuReadbackImage frame;
        ASSERT_NO_FATAL_FAILURE(ReadSceneColor(frame));
        ToDisplaySpace(frame);
        reference.Add(frame);
    }

    // Converge, then sample the TAA output over one full jitter cycle.
    renderManager.RequestTemporalHistoryReset();
    RenderSceneFrames(kConvergenceFrames - kHaltonPeriod);
    GpuReadbackImage output;
    ImageSequenceStats converged;
    for (uint32_t i = 0; i < kHaltonPeriod; ++i)
    {
        RenderSceneFrame();
        ASSERT_NO_FATAL_FAILURE(ReadTemporalOutput(output));
        ASSERT_EQ(output.pixels.size(), reference.mean.size());
        ToDisplaySpace(output);
        converged.Add(output);
    }

    double totalError = 0.0;
    double edgeRawFlicker = 0.0;
    double edgeTemporalFlicker = 0.0;
    uint32_t edgePixels = 0;
    for (size_t p = 0; p < reference.mean.size(); ++p)
    {
        totalError += PixelDifference(output.pixels[p], reference.mean[p]);
        if (reference.Flicker(p) > kEdgeFlickerThreshold)
        {
            edgeRawFlicker += reference.Flicker(p);
            edgeTemporalFlicker += converged.Flicker(p);
            ++edgePixels;
        }
    }

    const double meanError = totalError / static_cast<double>(reference.mean.size());
    ASSERT_GE(edgePixels, kMinEdgePixels) << "scene has too few aliased edges to measure convergence";
    EXPECT_LT(meanError, kMeanTolerance);
    EXPECT_LT(edgeTemporalFlicker, kFlickerRatio * edgeRawFlicker)
        << "edge pixels=" << edgePixels << " mean raw flicker=" << edgeRawFlicker / edgePixels
        << " mean TAA flicker=" << edgeTemporalFlicker / edgePixels;

    const RenderGraph& graph = renderManager.GetFrameGraph();
    const std::optional<size_t> transparent = FindCompiledPass(graph, "Transparent");
    const std::optional<size_t> resolve = FindCompiledPass(graph, "TemporalResolve");
    const std::optional<size_t> finalize = FindCompiledPass(graph, "PostProcessFinalize");
    ASSERT_TRUE(transparent && resolve && finalize);
    EXPECT_LT(*transparent, *resolve);
    EXPECT_LT(*resolve, *finalize);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

TEST_P(GpuTemporalResolveTests, TemporalOff_OutputByteIdenticalAfterToggle)
{
    GpuSceneBuilder builder(GetEngine(), GetAssetDatabase());
    ASSERT_NO_FATAL_FAILURE(BuildScene(builder));
    PostProcessStack* stack = builder.CreatePassthroughTonemapStack();
    ASSERT_NE(stack, nullptr);
    ASSERT_GT(stack->GetPassCount(), 0);
    builder.AttachPostProcessStack(m_camera, stack);
    builder.InitGpuResources();

    DXRenderManager& renderManager = GetRenderManager();
    ASSERT_FALSE(renderManager.IsTemporalEnabled());
    const UINT sampleCount = GetRenderTarget()->GetSampleDesc().Count;

    RenderSceneFrame();
    EXPECT_FALSE(FrameHasTemporalResolve());
    GpuReadbackImage before;
    ASSERT_NO_FATAL_FAILURE(Read(GetFinalColorTexture(), before));

    renderManager.SetTemporalEnabled(true);
    EXPECT_EQ(GetRenderTarget()->GetSampleDesc().Count, 1u);
    RenderSceneFrames(kWarmupFrames);

    // TAA output feeds the post-process chain.
    const RenderGraph& graph = renderManager.GetFrameGraph();
    const std::optional<size_t> resolve = FindCompiledPass(graph, "TemporalResolve");
    const std::string& firstPostName = stack->GetPass(0)->m_passName;
    const std::optional<size_t> firstPost =
        FindCompiledPass(graph, firstPostName.empty() ? std::string("PostProcess") : firstPostName);
    ASSERT_TRUE(resolve && firstPost);
    EXPECT_LT(*resolve, *firstPost);

    renderManager.SetTemporalEnabled(false);
    EXPECT_EQ(GetRenderTarget()->GetSampleDesc().Count, sampleCount);
    RenderSceneFrame();
    EXPECT_FALSE(FrameHasTemporalResolve());
    GpuReadbackImage after;
    ASSERT_NO_FATAL_FAILURE(Read(GetFinalColorTexture(), after));

    ExpectBitEqual(before, after);
    EXPECT_GPU_VALIDATION_CLEAN(GetDevice()->GetD3D12Device().Get());
}

INSTANTIATE_TEST_SUITE_P(RenderPaths, GpuTemporalResolveTests,
    ::testing::Values(RenderPath::Forward, RenderPath::Deferred),
    [](const ::testing::TestParamInfo<RenderPath>& info)
    {
        return info.param == RenderPath::Forward ? std::string("Forward") : std::string("Deferred");
    });
