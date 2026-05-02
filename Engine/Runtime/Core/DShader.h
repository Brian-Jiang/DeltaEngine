#pragma once

#include "EngineIncludes.h"

#include "Runtime/Core/DObject.h"
#include "Runtime/Serialization/ISerializationCallbackReceiver.h"
#include "Runtime/Serialization/TBulkData.h"

#include <d3d12.h>
#include <slang.h>
#include <slang-com-ptr.h>
#include <wrl/client.h>

#include <filesystem>
#include <string>
#include <vector>

#include "DShader.generated.h"

DELTA_ENGINE_NS_BEGIN

DCLASS()
class DShader : public DObject, public ISerializationCallbackReceiver
{
    DGENERATED_BODY(DShader)

public:
    DELTAENGINE_API DShader();
    DELTAENGINE_API ~DShader();

    DELTAENGINE_API void Initialize(const std::filesystem::path& sourcePath, const std::string& vertexShaderEntryPoint,
        const std::string& pixelShaderEntryPoint, const std::string& vertexShaderTargetProfile,
        const std::string& pixelShaderTargetProfile);

    DFUNCTION()
    DELTAENGINE_API void SetSourcePath(const std::filesystem::path& sourcePath);
    DFUNCTION()
    DELTAENGINE_API void SetVertexShaderEntryPoint(const std::string& entryPoint);
    DFUNCTION()
    DELTAENGINE_API void SetPixelShaderEntryPoint(const std::string& entryPoint);
    DFUNCTION()
    DELTAENGINE_API void SetVertexShaderTargetProfile(const std::string& targetProfile);
    DFUNCTION()
    DELTAENGINE_API void SetPixelShaderTargetProfile(const std::string& targetProfile);

    DFUNCTION(ShowAsButton)
    DELTAENGINE_API void Reimport();

    DELTAENGINE_API void SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);

    void OnBeforeSerialize() override;
    void OnAfterDeserialize() override;

    inline ISlangBlob* GetVertexShaderBlob() const { return m_vertexShaderBlob.get(); }
    inline ISlangBlob* GetPixelShaderBlob() const { return m_pixelShaderBlob.get(); }
    inline const std::vector<D3D12_INPUT_ELEMENT_DESC>& GetInputLayout() const { return m_inputLayout; }

private:
    void CompileShader();

private:
    Slang::ComPtr<ISlangBlob> m_vertexShaderBlob;
    Slang::ComPtr<ISlangBlob> m_pixelShaderBlob;
    std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputLayout;
    std::vector<std::string> m_inputLayoutSemanticNames;

    DPROPERTY()
    std::filesystem::path m_sourcePath;
    DPROPERTY()
    std::string m_vertexShaderEntryPoint;
    DPROPERTY()
    std::string m_pixelShaderEntryPoint;
    DPROPERTY()
    std::string m_vertexShaderTargetProfile;
    DPROPERTY()
    std::string m_pixelShaderTargetProfile;
    DPROPERTY()
    TBulkData m_serializedShaderBlobs;
    DPROPERTY()
    TBulkData m_serializedInputLayout;
};

DELTA_ENGINE_NS_END
