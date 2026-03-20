#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <wrl/client.h>
#include <string>
#include <d3d12.h>
#include <vector>

#include "Runtime/Core/DObject.h"
#include "Runtime/Serialization/TBulkData.h"
#include "Runtime/Serialization/ISerializationCallbackReceiver.h"

#include <dxcapi.h>

#include "DShader.generated.h"

DELTA_ENGINE_NS_BEGIN

DCLASS()
class DShader : public DObject, public ISerializationCallbackReceiver
{
    DGENERATED_BODY(DShader)

public:
    DShader();
    //DShader(const std::wstring& sourcePath, const std::wstring& vertexShaderEntryPoint,
    //        const std::wstring& pixelShaderEntryPoint, const std::wstring& vertexShaderTargetProfile,
    //        const std::wstring& pixelShaderTargetProfile);
    ~DShader();

    DELTAENGINE_API void Initialize(const std::wstring& sourcePath, const std::wstring& vertexShaderEntryPoint,
                    const std::wstring& pixelShaderEntryPoint, const std::wstring& vertexShaderTargetProfile,
                    const std::wstring& pixelShaderTargetProfile);

    DFUNCTION()
    void SetSourcePath(const std::wstring& sourcePath);
    DFUNCTION()
    void SetVertexShaderEntryPoint(const std::wstring& entryPoint);
    DFUNCTION()
    void SetPixelShaderEntryPoint(const std::wstring& entryPoint);
    DFUNCTION()
    void SetVertexShaderTargetProfile(const std::wstring& targetProfile);
    DFUNCTION()
    void SetPixelShaderTargetProfile(const std::wstring& targetProfile);
    
    DELTAENGINE_API void SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);

    void OnBeforeSerialize() override;
    void OnAfterDeserialize() override;

    
    inline IDxcBlob* GetVertexShaderBlob() const { return m_vertexShaderBlob.Get(); }
    
    inline IDxcBlob* GetPixelShaderBlob() const { return m_pixelShaderBlob.Get(); }

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
