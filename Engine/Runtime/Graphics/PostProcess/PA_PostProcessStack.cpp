#include "Runtime/Graphics/PostProcess/PA_PostProcessStack.h"

#include "Runtime/Core/UUID.h"
#include "Runtime/Graphics/PostProcess/PostProcessPass.h"
#include "Runtime/Graphics/PostProcess/PostProcessStack.h"
#include "Runtime/Reflection/DClass.h"
#include "Runtime/Reflection/ReflectionRegistry.h"
#include "Runtime/Logging/LogChannels.h"

#include <algorithm>

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

PostProcessStack* PA_PostProcessStack::GetStack() const
{
    if (m_stack)
        return m_stack;

    for (DObject* object : GetObjects())
    {
        if (PostProcessStack* stack = dynamic_cast<PostProcessStack*>(object))
        {
            m_stack = stack;
            return m_stack;
        }
    }

    return nullptr;
}

PostProcessPass* PA_PostProcessStack::AddPass(const std::string& className)
{
    PostProcessStack* stack = GetStack();
    if (!DELTA_ENSURE_MSG(stack, "PA_PostProcessStack::AddPass called with null stack (className='{}', expected PostProcessStack on asset)",
            className))
        return nullptr;

    for (PostProcessPass* existing : stack->m_passes)
    {
        if (!existing || !existing->GetClass())
            continue;
        if (existing->GetClass()->GetName() == className)
        {
            DLOG(LogPostProcess, ELogLevel::Warning,
                "AddPass rejected: stack already contains a pass of class '{}' (at most one pass per class per stack)",
                className);
            return nullptr;
        }
    }

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
    stack->m_passes.push_back(pass);
    return pass;
}

bool PA_PostProcessStack::RemovePass(const std::string& className)
{
    PostProcessStack* stack = GetStack();
    if (!DELTA_ENSURE_MSG(stack, "PA_PostProcessStack::RemovePass called with null stack (className='{}', expected PostProcessStack on asset)",
            className))
        return false;

    auto& passes = stack->m_passes;
    auto it = std::find_if(passes.begin(), passes.end(),
        [&](PostProcessPass* p)
        {
            return p && p->GetClass() && p->GetClass()->GetName() == className;
        });

    if (it == passes.end())
    {
        DLOG(LogPostProcess, ELogLevel::Warning,
            "RemovePass failed: no pass of class '{}' found in stack",
            className);
        return false;
    }

    PostProcessPass* pass = *it;
    const ObjectId passId = pass->GetObjectId();
    passes.erase(it);
    RemoveObject(passId);
    GetReflectionRegistry().DestroyObject(pass);
    MarkDirty();
    return true;
}
