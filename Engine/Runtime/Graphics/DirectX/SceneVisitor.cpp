#include "Runtime/Graphics/DirectX/SceneVisitor.h"

#include "Runtime/Graphics/DirectX/EffectPSO.h"
#include "Runtime/Core/Camera.h"

#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/DirectX/IndexBuffer.h"
#include "Runtime/Graphics/DirectX/Material.h"
#include "Runtime/Graphics/DirectX/Mesh.h"
#include "Runtime/Graphics/DirectX/EffectPSO.h"
//#include <dx12lib/SceneNode.h>

#include <DirectXMath.h>

using namespace DeltaEngine;
using namespace DirectX;

SceneVisitor::SceneVisitor( CommandList& commandList, const Camera& camera, EffectPSO& pso, bool transparent )
    : m_CommandList( commandList )
    , m_Camera( camera )
    , m_LightingPSO( pso )
    , m_TransparentPass(transparent)
{}

void SceneVisitor::Visit(std::weak_ptr<SceneComponent> sceneComponent)
{
    if (auto component = sceneComponent.lock())
    {
        auto world = component->GetWorldTransform();
        m_LightingPSO.SetWorldMatrix(world);
    }
    //m_LightingPSO.SetViewMatrix( m_Camera.GetViewMatrix() );
    //m_LightingPSO.SetProjectionMatrix( m_Camera.GetProjectionMatrix() );
}

//void SceneVisitor::Visit( SceneNode& sceneNode )
//{
//    auto world = sceneNode.GetWorldTransform();
//    m_LightingPSO.SetWorldMatrix( world );
//}

void SceneVisitor::Visit( Mesh& mesh )
{
    auto material = mesh.GetMaterial();
    if ( material->IsTransparent() == m_TransparentPass )
    {
        m_LightingPSO.SetMaterial( material );

        m_LightingPSO.Apply( m_CommandList );
        mesh.Draw( m_CommandList );
    }
}