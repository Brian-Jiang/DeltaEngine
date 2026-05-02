#include "Runtime/Core/DShader.h"
#include "Runtime/Core/DMesh.h"
#include "Runtime/Core/DMaterial.h"
#include "Runtime/Core/DTexture.h"
#include "Runtime/Graphics/MaterialConstants.h"

#include <gtest/gtest.h>
#include <stdexcept>

using namespace DeltaEngine;

TEST(CoreAssetData, DShader_Initialize_InvalidSource_YieldsEmptyBlobs)
{
    DShader shader;
    shader.Initialize(
        L"ThisPathDoesNotExistDeltaEngine_missing_xyz.slang",
        L"VSMain",
        L"PSMain",
        L"vs_6_6",
        L"ps_6_6");

    EXPECT_EQ(shader.GetVertexShaderBlob(), nullptr);
    EXPECT_EQ(shader.GetPixelShaderBlob(), nullptr);
}

TEST(CoreAssetData, DMesh_ImportFromAbsolutePath_MissingFile_KeepsZeroSubmeshes)
{
    DMesh mesh;
    mesh.ImportFromAbsolutePath(L"C:\\DoesNotExistDeltaEngine\\missing_mesh_xyz.obj");

    EXPECT_EQ(mesh.GetSubMeshCount(), 0);
}

TEST(CoreAssetData, DTexture_Initialize_MissingFile_ThrowsRuntimeError)
{
    DTexture tex;
    EXPECT_THROW(tex.Initialize(L"C:\\DoesNotExistDeltaEngine\\missing_tex_xyz.dds"), std::runtime_error);
}

TEST(CoreAssetData, DMaterial_GetTexture_InvalidSlot_ReturnsNull)
{
    DMaterial mat;

    EXPECT_EQ(mat.GetTexture(-1), nullptr);
    EXPECT_EQ(mat.GetTexture(99), nullptr);
}

TEST(CoreAssetData, DMaterial_FillMaterialCB_DefaultScalarsMatchConstruction)
{
    DMaterial mat;
    MaterialCB cb {};
    mat.FillMaterialCB(cb);

    EXPECT_FLOAT_EQ(cb.metallic, 0.f);
    EXPECT_FLOAT_EQ(cb.roughness, 0.5f);
    EXPECT_FLOAT_EQ(cb.emissiveIntensity, 1.f);
}
