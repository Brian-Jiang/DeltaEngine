#include "Runtime/Reflection/DFunction.h"

using namespace DeltaEngine;

DFunction::DFunction(std::string name,
                     NativeFn nativeFn,
                     uint32_t numParams,
                     uint32_t totalSize,
                     uint32_t returnValueOffset)
    : m_name(std::move(name))
    , m_nativeFn(nativeFn)
    , m_numParams(numParams)
    , m_totalSize(totalSize)
    , m_returnValueOffset(returnValueOffset)
{
}

void DFunction::Invoke(DObject* context, void* params) const
{
    m_nativeFn(context, params);
}

void DFunction::AddParam(DProperty* param)
{
    m_params.push_back(param);
}

void DFunction::SetReturnProperty(DProperty* prop)
{
    m_returnProperty = prop;
}

const std::string& DFunction::GetName() const { return m_name; }
DClass* DFunction::GetDeclaringClass() const { return m_declaringClass; }
uint32_t DFunction::GetNumParams() const { return m_numParams; }
uint32_t DFunction::GetTotalSize() const { return m_totalSize; }
uint32_t DFunction::GetReturnValueOffset() const { return m_returnValueOffset; }
const std::vector<DProperty*>& DFunction::GetParams() const { return m_params; }
DProperty* DFunction::GetReturnProperty() const { return m_returnProperty; }
bool DFunction::HasReturnValue() const { return m_returnProperty != nullptr; }
