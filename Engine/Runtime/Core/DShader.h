#pragma once

#include "EngineIncludes.h"

#include <memory>
#include <wrl/client.h>
#include <string>
#include <d3d12.h>
#include <vector>

#include "Runtime/Core/DObject.h"

struct IDxcBlob;

DELTA_ENGINE_NS_BEGIN

class DShader : public DObject
{

public:
    DShader();
    DShader(const std::wstring& sourcePath, const std::wstring& vertexShaderEntryPoint,
            const std::wstring& pixelShaderEntryPoint, const std::wstring& vertexShaderTargetProfile,
            const std::wstring& pixelShaderTargetProfile);
    ~DShader();

    void SetSourcePath(const std::wstring& sourcePath);
    void SetVertexShaderEntryPoint(const std::wstring& entryPoint);
    void SetPixelShaderEntryPoint(const std::wstring& entryPoint);
    void SetVertexShaderTargetProfile(const std::wstring& targetProfile);
    void SetPixelShaderTargetProfile(const std::wstring& targetProfile);

    inline IDxcBlob* GetVertexShaderBlob() const { return m_vertexShaderBlob.Get(); }
    inline IDxcBlob* GetPixelShaderBlob() const { return m_pixelShaderBlob.Get(); }

private:
    void CompileShader();

private:
    Microsoft::WRL::ComPtr<IDxcBlob> m_vertexShaderBlob;
    Microsoft::WRL::ComPtr<IDxcBlob> m_pixelShaderBlob;
    std::wstring m_sourcePath;
    std::wstring m_vertexShaderEntryPoint;
    std::wstring m_pixelShaderEntryPoint;
    std::wstring m_vertexShaderTargetProfile;
    std::wstring m_pixelShaderTargetProfile;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
};

DELTA_ENGINE_NS_END
