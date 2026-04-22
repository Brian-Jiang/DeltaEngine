#pragma once

#include "EngineIncludes.h"

#include <d3d12.h>
#include "Runtime/Core/DObject.h"
#include "Runtime/Serialization/ISerializationCallbackReceiver.h"
#include "Runtime/Serialization/TBulkData.h"

#include <dxcapi.h>
#include <string>
#include <vector>
#include <wrl/client.h>

#include "DShader.generated.h"

DELTA_ENGINE_NS_BEGIN

DCLASS()
class DShader : public DObject, public ISerializationCallbackReceiver
{
    DGENERATED_BODY(DShader)

public:
    DELTAENGINE_API DShader();
    DELTAENGINE_API ~DShader();

    /// Sets all shader source and entry-point metadata, then compiles.
    DELTAENGINE_API void Initialize(const std::wstring& sourcePath, const std::wstring& vertexShaderEntryPoint,
        const std::wstring& pixelShaderEntryPoint, const std::wstring& vertexShaderTargetProfile,
        const std::wstring& pixelShaderTargetProfile);

    /// Updates the shader source path and recompiles.
    DFUNCTION()
    DELTAENGINE_API void SetSourcePath(const std::wstring& sourcePath);
    /// Updates the vertex shader entry point and recompiles.
    DFUNCTION()
    DELTAENGINE_API void SetVertexShaderEntryPoint(const std::wstring& entryPoint);
    /// Updates the pixel shader entry point and recompiles.
    DFUNCTION()
    DELTAENGINE_API void SetPixelShaderEntryPoint(const std::wstring& entryPoint);
    /// Updates the vertex shader target profile and recompiles.
    DFUNCTION()
    DELTAENGINE_API void SetVertexShaderTargetProfile(const std::wstring& targetProfile);
    /// Updates the pixel shader target profile and recompiles.
    DFUNCTION()
    DELTAENGINE_API void SetPixelShaderTargetProfile(const std::wstring& targetProfile);

    /// Recompiles the shader from m_sourcePath and replaces the serialized bulk data.
    DFUNCTION(ShowAsButton)
    DELTAENGINE_API void Reimport();

    
    /// Copies the input layout and keeps semantic-name storage alive.
    DELTAENGINE_API void SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);

    /// Packs compiled shader blobs for serialization.
    void OnBeforeSerialize() override;
    /// Restores compiled shader blobs after deserialization.
    void OnAfterDeserialize() override;

    /// Returns the compiled vertex shader blob.
    inline IDxcBlob* GetVertexShaderBlob() const { return m_vertexShaderBlob.Get(); }
    /// Returns the compiled pixel shader blob.
    inline IDxcBlob* GetPixelShaderBlob() const { return m_pixelShaderBlob.Get(); }
    /// Returns the current input layout descriptors.
    inline const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputLayout() const { return m_inputLayout; }

private:
    void CompileShader();

private:
    Microsoft::WRL::ComPtr<IDxcBlob> m_vertexShaderBlob;
    Microsoft::WRL::ComPtr<IDxcBlob> m_pixelShaderBlob;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
    std::vector<std::string> m_inputLayoutSemanticNames;

    DPROPERTY()
    std::wstring m_sourcePath;
    DPROPERTY()
    std::wstring m_vertexShaderEntryPoint;
    DPROPERTY()
    std::wstring m_pixelShaderEntryPoint;
    DPROPERTY()
    std::wstring m_vertexShaderTargetProfile;
    DPROPERTY()
    std::wstring m_pixelShaderTargetProfile;
    DPROPERTY()
    TBulkData m_serializedShaderBlobs;
    DPROPERTY()
    TBulkData m_serializedInputLayout;
};

DELTA_ENGINE_NS_END
