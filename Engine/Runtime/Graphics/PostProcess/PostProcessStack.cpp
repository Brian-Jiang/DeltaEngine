#include "Runtime/Graphics/PostProcess/PostProcessStack.h"

#include "Runtime/Graphics/PostProcess/PostProcessPass.h"

DELTA_ENGINE_NS_BEGIN

PostProcessPass* PostProcessStack::GetPass(int index) const
{
    const int count = static_cast<int>(m_passes.size());
    if (!DELTA_ENSURE_MSG(index >= 0 && index < count,
            "PostProcessStack::GetPass index out of range (index={}, passCount={}, expected 0 <= index < passCount)",
            index, count))
        return nullptr;
    return m_passes[static_cast<size_t>(index)];
}

int PostProcessStack::GetPassCount() const
{
    return static_cast<int>(m_passes.size());
}

DELTA_ENGINE_NS_END
