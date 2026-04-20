#include "Graphics/PostProcess/PostProcessStack.h"

#include "Graphics/PostProcess/PostProcessPass.h"

using namespace DeltaEngine;

PostProcessPass* PostProcessStack::GetPass(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_passes.size()))
        return nullptr;
    return m_passes[static_cast<size_t>(index)];
}

int PostProcessStack::GetPassCount() const
{
    return static_cast<int>(m_passes.size());
}
