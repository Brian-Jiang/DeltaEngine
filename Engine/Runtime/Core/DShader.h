#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <wrl/client.h>
#include <string>
#include <d3d12.h>
#include <vector>

#include "Runtime/Core/DObject.h"

#include <dxcapi.h>

#include "DShader.generated.h"

DELTA_ENGINE_NS_BEGIN

DCLASS()
class DShader : public DObject
{
    DGENERATED_BODY(DShader)

public:
    DShader();
    DShader(const std::wstring& sourcePath, const std::wstring& vertexShaderEntryPoint,
            const std::wstring& pixelShaderEntryPoint, const std::wstring& vertexShaderTargetProfile,
            const std::wstring& pixelShaderTargetProfile);
    ~DShader();

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
    
    void SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);

    
    inline IDxcBlob* GetVertexShaderBlob() const { return m_vertexShaderBlob.Get(); }
    
    inline IDxcBlob* GetPixelShaderBlob() const { return m_pixelShaderBlob.Get(); }

    inline const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputLayout() const { return m_inputLayout; }

private:
    void CompileShader();

private:
    Microsoft::WRL::ComPtr<IDxcBlob> m_vertexShaderBlob;
    Microsoft::WRL::ComPtr<IDxcBlob> m_pixelShaderBlob;

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
    
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
};

DELTA_ENGINE_NS_END
