#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Reflection/ReflectionRegistry.h"

using namespace DeltaEngine;

PA_PostProcessStack* PA_PostProcessStack::Create()
{
    PA_PostProcessStack* asset = CreateDObject<PA_PostProcessStack>();
    if (!asset)
        return nullptr;

    asset->GetHeader().m_persistentId = UUID::Generate();
    asset->GetHeader().m_className = "PA_PostProcessStack";

    PostProcessStack* stack = CreateDObject<PostProcessStack>();
    if (!stack)
    {
        DLOG(LogPostProcess, ELogLevel::Error,
            "PA_PostProcessStack::Create failed: CreateDObject<PostProcessStack> returned nullptr (expected embedded stack)");
        GetReflectionRegistry().DestroyObject(asset);
        return nullptr;
    }

    asset->AddObject(stack);
    asset->m_stack = stack;
    return asset;
}

PostProcessPass* PA_PostProcessStack::AddPass(const std::string& className)
{
    if (!DELTA_ENSURE_MSG(m_stack, "PA_PostProcessStack::AddPass called with null m_stack (className='{}', expected PostProcessStack on asset)",
            className))
        return nullptr;

    DObject* obj = GetReflectionRegistry().CreateObject(className);
    if (!obj)
    {
        DLOG(LogPostProcess, ELogLevel::Warning,
            "AddPass failed: reflection CreateObject returned nullptr (className='{}', expected registered DCLASS name)",
            className);
        return nullptr;
    }

    PostProcessPass* pass = dynamic_cast<PostProcessPass*>(obj);
    if (!pass)
    {
        DLOG(LogPostProcess, ELogLevel::Warning,
            "AddPass failed: '{}' is not a PostProcessPass subclass (expected type derived from PostProcessPass)",
            className);
        GetReflectionRegistry().DestroyObject(obj);
        return nullptr;
    }

    AddObject(pass);
    m_stack->m_passes.push_back(pass);
    return pass;
}
