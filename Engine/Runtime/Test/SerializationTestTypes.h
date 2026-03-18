#pragma once

#include "EngineIncludes.h"

#include <string>
#include <vector>

#include "SimpleMath.h"
#include "Runtime/Core/DObject.h"
#include "Runtime/Assets/DPrimaryAsset.h"
#include "Runtime/Serialization/TBulkData.h"

#include "SerializationTestTypes.generated.h"

DELTA_ENGINE_NS_BEGIN

DCLASS()
class DTestObjectA : public DObject
{
    DGENERATED_BODY(DTestObjectA)

    DPROPERTY()
    float m_health = 100.0f;

    DPROPERTY()
    int m_level = 1;

    DPROPERTY()
    bool m_isActive = false;

    DPROPERTY()
    double m_stamina = 50.0;

    DPROPERTY()
    std::string m_name = "DefaultName";

    DPROPERTY()
    DirectX::SimpleMath::Vector3 m_position = {};

    DPROPERTY()
    DirectX::SimpleMath::Quaternion m_rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    DPROPERTY()
    std::vector<float> m_weights;

    DPROPERTY()
    std::vector<std::vector<float>> m_weightMatrix;
};


DCLASS()
class DTestObjectB : public DObject
{
    DGENERATED_BODY(DTestObjectB)

    DPROPERTY()
    std::string m_label = "Ref";

    DPROPERTY()
    float m_weight = 1.0f;

    DPROPERTY()
    DTestObjectA* m_targetRef = nullptr;

    DPROPERTY()
    std::vector<DTestObjectA*> m_refs;

    DPROPERTY()
    std::vector<std::shared_ptr<DTestObjectA>> m_sharedRefs;
};


DCLASS()
class PA_TestAsset : public DPrimaryAsset
{
    DGENERATED_BODY(PA_TestAsset)
};


DCLASS()
class DTestMeshData : public DObject
{
    DGENERATED_BODY(DTestMeshData)

public:
    DTestMeshData() = default;
    DTestMeshData(const DTestMeshData& other)
    {
        m_vertexCount = other.m_vertexCount;
        m_indexCount  = other.m_indexCount;
        m_vertexBuffer.Set(other.m_vertexBuffer.m_data, other.m_vertexBuffer.m_size);
        m_indexBuffer.Set(other.m_indexBuffer.m_data, other.m_indexBuffer.m_size);
    }

    DPROPERTY()
    int m_vertexCount = 0;

    DPROPERTY()
    int m_indexCount = 0;

    DPROPERTY()
    TBulkData m_vertexBuffer;

    DPROPERTY()
    TBulkData m_indexBuffer;
};


DCLASS()
class PA_TestMesh : public DPrimaryAsset
{
    DGENERATED_BODY(PA_TestMesh)
};

DELTA_ENGINE_NS_END
