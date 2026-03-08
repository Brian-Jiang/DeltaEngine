#include "Runtime/Core/DObject.h"
#include "Assets/DPrimaryAsset.h"

using namespace DeltaEngine;

DObject::DObject()
{
}

DObject::~DObject()
{
}

void DObject::MarkDirty()
{
    if (m_owningAsset)
        m_owningAsset->MarkDirty();
}
