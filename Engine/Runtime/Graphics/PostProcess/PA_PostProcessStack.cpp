#include "Graphics/PostProcess/PA_PostProcessStack.h"

#include "Core/UUID.h"
#include "Graphics/PostProcess/PostProcessPass.h"
#include "Graphics/PostProcess/PostProcessStack.h"
#include "Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;

PA_PostProcessStack* PA_PostProcessStack::Create()
{
    PA_PostProcessStack* asset = CreateDObject<PA_PostProcessStack>();
    if (!asset)
        return nullptr;

    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className = "PA_PostProcessStack";

    PostProcessStack* stack = CreateDObject<PostProcessStack>();
    if (stack)
    {
        asset->AddObject(stack);
        asset->m_stack = stack;
    }

    return asset;
}

PostProcessPass* PA_PostProcessStack::AddPass(const std::string& className)
{
    if (!m_stack)
        return nullptr;

    DObject* obj = GetReflectionRegistry().CreateObject(className);
    if (!obj)
        return nullptr;

    PostProcessPass* pass = dynamic_cast<PostProcessPass*>(obj);
    if (!pass)
    {
        GetReflectionRegistry().DestroyObject(obj);
        return nullptr;
    }

    AddObject(pass);
    m_stack->m_passes.push_back(pass);
    return pass;
}
