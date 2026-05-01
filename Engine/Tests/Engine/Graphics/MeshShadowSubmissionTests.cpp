#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DShader.h"
#include "Runtime/Graphics/MaterialConstants.h"
#include "Runtime/Graphics/RenderProxy/MeshRenderProxy.h"
#include "Runtime/Graphics/Renderer/MeshRenderer.h"
#include "Runtime/Graphics/Shadow/ShadowView.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/DProperty.h"

#include <d3dx12.h>
#include <gtest/gtest.h>

using namespace DeltaEngine;

namespace
{
void SetBoolProperty(DObject& obj, const char* name, bool value)
{
    DClass* cls = obj.GetClass();
    ASSERT_NE(cls, nullptr);
    DProperty* prop = cls->FindPropertyByName(name);
    ASSERT_NE(prop, nullptr);
    prop->SetValue(&obj, &value);
}
}

class MeshRendererShadowHarness : public MeshRenderer
{
public:
    using MeshRenderer::GatherShadowDrawCalls;
};

TEST(MeshShadowSubmissionTests, OpaqueSubmeshContributesToShadowMap)
{
    DShader shader;
    DMaterial mat;
    mat.Initialize(&shader);
    EXPECT_TRUE(MeshRenderProxy::SubmeshContributesToShadowMap(&mat));
    EXPECT_FALSE(HasAny(mat.GetFlags(), MaterialFlags::AlphaBlend));
}

TEST(MeshShadowSubmissionTests, AlphaBlendSubmeshExcludedFromShadowMap)
{
    DShader shader;
    DMaterial mat;
    mat.Initialize(&shader);

    CD3DX12_BLEND_DESC blend(D3D12_DEFAULT);
    blend.RenderTarget[0].BlendEnable = TRUE;
    blend.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
    blend.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    blend.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    blend.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
    blend.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    blend.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    mat.SetBlendState(CD3DX12_PIPELINE_STATE_STREAM_BLEND_DESC(blend));

    EXPECT_TRUE(HasAny(mat.GetFlags(), MaterialFlags::AlphaBlend));
    EXPECT_FALSE(MeshRenderProxy::SubmeshContributesToShadowMap(&mat));
}

TEST(MeshShadowSubmissionTests, MeshRendererSkipsShadowDispatchWhenCastShadowOff)
{
    MeshRendererShadowHarness renderer;
    ShadowView view {};

    bool castOff = false;
    SetBoolProperty(renderer, "m_castShadow", castOff);
    renderer.GatherShadowDrawCalls(nullptr, view);

    bool castOn = true;
    SetBoolProperty(renderer, "m_castShadow", castOn);
    renderer.GatherShadowDrawCalls(nullptr, view);
}
