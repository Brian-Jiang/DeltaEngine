#include "Runtime/Graphics/RenderGraph/RenderGraph.h"

#include <map>
#include <utility>

#include "Runtime/Graphics/DirectX/CommandList.h"
#include "Runtime/Graphics/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/Graphics/RenderGraph/TransientTexturePool.h"

DELTA_ENGINE_NS_BEGIN

void RenderGraph::AddPass(std::unique_ptr<RenderGraphPass> pass)
{
    DELTA_ASSERT(pass != nullptr);
    m_passes.push_back(std::move(pass));
}

RenderGraphTextureHandle RenderGraph::ImportTexture(std::string name, std::shared_ptr<DirectX12Texture> texture,
    RenderGraphTextureUsage usage)
{
    DELTA_ASSERT(FindImportedTexture(name).index == RenderGraphTextureHandle::kInvalid);

    RenderGraphTextureHandle handle;
    handle.index = static_cast<uint32_t>(m_importedTextures.size());

    RenderGraphTexture importedTexture;
    importedTexture.name = std::move(name);
    importedTexture.texture = std::move(texture);
    importedTexture.usage = usage;
    m_importedTextures.push_back(std::move(importedTexture));

    return handle;
}

RenderGraphTextureHandle RenderGraph::CreateTexture(std::string name, const D3D12_RESOURCE_DESC& desc,
    RenderGraphTextureUsage usage)
{
    DELTA_ASSERT(m_transientPool != nullptr);
    DELTA_ASSERT(FindImportedTexture(name).index == RenderGraphTextureHandle::kInvalid);
    if (!m_transientPool)
    {
        return {};
    }

    std::shared_ptr<DirectX12Texture> texture = m_transientPool->Acquire(desc, name);

    RenderGraphTextureHandle handle;
    handle.index = static_cast<uint32_t>(m_importedTextures.size());

    RenderGraphTexture transientTexture;
    transientTexture.name = std::move(name);
    transientTexture.texture = std::move(texture);
    transientTexture.usage = usage;
    transientTexture.transient = true;
    m_importedTextures.push_back(std::move(transientTexture));

    return handle;
}

void RenderGraph::Reset()
{
    m_passes.clear();
    m_importedTextures.clear();
    m_compiledPasses.clear();
}

void RenderGraph::Compile()
{
    m_compiledPasses.clear();

    const size_t passCount = m_passes.size();
    if (passCount == 0)
    {
        return;
    }

    // Gather declared reads/writes by replaying each pass's Setup.
    RenderGraphBuilder builder(*this);
    for (size_t i = 0; i < passCount; ++i)
    {
        builder.BeginPass(i);
        m_passes[i]->Setup(builder);
    }
    const std::vector<std::vector<RenderGraphResourceAccess>>& accesses = builder.GetPassAccesses();

    // Build producer -> consumer edges from reads. Each read depends on the latest
    // preceding writer for that texture (supports ping-pong multi-write). When a
    // consumer is declared before its producer, fall back to the earliest future
    // writer so topo-sort can still reorder out-of-order declarations.
    DELTA_ASSERT(accesses.size() == passCount);

    std::map<uint32_t, std::vector<size_t>> writersOf;
    for (size_t pass = 0; pass < passCount; ++pass)
    {
        for (const RenderGraphResourceAccess& access : accesses[pass])
        {
            if (access.type == RenderGraphAccessType::Write)
            {
                writersOf[access.texture.index].push_back(pass);
            }
        }
    }

    std::vector<std::vector<size_t>> adjacency(passCount);
    std::vector<uint32_t> inDegree(passCount, 0);
    for (size_t pass = 0; pass < passCount; ++pass)
    {
        for (const RenderGraphResourceAccess& access : accesses[pass])
        {
            if (access.type != RenderGraphAccessType::Read)
            {
                continue;
            }

            const auto writersIt = writersOf.find(access.texture.index);
            if (writersIt == writersOf.end())
            {
                continue;
            }

            const std::vector<size_t>& writers = writersIt->second;
            size_t producerPass = passCount;
            for (auto writerIt = writers.rbegin(); writerIt != writers.rend(); ++writerIt)
            {
                if (*writerIt < pass)
                {
                    producerPass = *writerIt;
                    break;
                }
            }

            if (producerPass == passCount)
            {
                for (size_t writerPass : writers)
                {
                    if (writerPass > pass)
                    {
                        producerPass = writerPass;
                        break;
                    }
                }
            }

            if (producerPass < passCount && producerPass != pass)
            {
                adjacency[producerPass].push_back(pass);
                ++inDegree[pass];
            }
        }
    }

    // Kahn's algorithm. Declaration order is preserved as the tiebreak by always
    // scanning candidate passes in ascending index order.
    std::vector<size_t> executionOrder;
    executionOrder.reserve(passCount);
    std::vector<bool> scheduled(passCount, false);

    while (executionOrder.size() < passCount)
    {
        bool progressed = false;
        for (size_t pass = 0; pass < passCount; ++pass)
        {
            if (!scheduled[pass] && inDegree[pass] == 0)
            {
                scheduled[pass] = true;
                executionOrder.push_back(pass);
                for (size_t consumer : adjacency[pass])
                {
                    --inDegree[consumer];
                }
                progressed = true;
            }
        }

        // No schedulable pass remaining => a dependency cycle exists.
        DELTA_ASSERT(progressed);
        if (!progressed)
        {
            break;
        }
    }

    // Emit a minimal per-pass transition list by tracking each resource's running
    // state (resources start in COMMON, matching engine resource creation).
    std::map<uint32_t, D3D12_RESOURCE_STATES> runningState;
    for (size_t pass : executionOrder)
    {
        RenderGraphCompiledPass compiled;
        compiled.passIndex = pass;

        for (const RenderGraphResourceAccess& access : accesses[pass])
        {
            auto stateIt = runningState.find(access.texture.index);
            const D3D12_RESOURCE_STATES current =
                stateIt != runningState.end() ? stateIt->second : D3D12_RESOURCE_STATE_COMMON;

            if (current != access.state)
            {
                compiled.transitions.push_back({ access.texture, access.state });
                runningState[access.texture.index] = access.state;
            }

            if (access.type == RenderGraphAccessType::Write &&
                access.clear.type != RenderGraphClearValue::Type::None)
            {
                compiled.clears.push_back({ access.texture, access.clear });
            }
        }

        m_compiledPasses.push_back(std::move(compiled));
    }
}

void RenderGraph::Execute(const RenderGraphContext& context)
{
    DELTA_ASSERT(context.commandList != nullptr);

    for (const RenderGraphCompiledPass& compiled : m_compiledPasses)
    {
        for (const RenderGraphResourceTransition& transition : compiled.transitions)
        {
            const RenderGraphTexture& texture = GetImportedTexture(transition.texture);
            context.commandList->TransitionBarrier(texture.texture, transition.stateAfter);
        }

        // Flush before Execute: passes may record GPU work that bypasses the state
        // tracker (e.g. ResolveSubresourceNoBarrier), so queued transitions must be
        // on the command list before the pass runs.
        context.commandList->FlushResourceBarriers();

        if (!compiled.clears.empty())
        {
            for (const RenderGraphClearOp& clear : compiled.clears)
            {
                const RenderGraphTexture& texture = GetImportedTexture(clear.texture);
                if (!texture.texture)
                {
                    continue;
                }

                if (clear.value.type == RenderGraphClearValue::Type::Color)
                {
                    context.commandList->GetD3D12CommandList()->ClearRenderTargetView(
                        texture.texture->GetRenderTargetView(), clear.value.color, 0, nullptr);
                }
                else if (clear.value.type == RenderGraphClearValue::Type::DepthStencil)
                {
                    context.commandList->GetD3D12CommandList()->ClearDepthStencilView(
                        texture.texture->GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH,
                        clear.value.depth, clear.value.stencil, 0, nullptr);
                }
            }
        }

        m_passes[compiled.passIndex]->Execute(context);
    }
}

const RenderGraphCompiledPass& RenderGraph::GetCompiledPass(size_t index) const
{
    DELTA_ASSERT(index < m_compiledPasses.size());
    return m_compiledPasses[index];
}

const RenderGraphPass& RenderGraph::GetPass(size_t index) const
{
    DELTA_ASSERT(index < m_passes.size());
    return *m_passes[index];
}

RenderGraphTextureHandle RenderGraph::FindImportedTexture(std::string_view name) const
{
    for (size_t i = 0; i < m_importedTextures.size(); ++i)
    {
        if (m_importedTextures[i].name == name)
        {
            RenderGraphTextureHandle handle;
            handle.index = static_cast<uint32_t>(i);
            return handle;
        }
    }

    return {};
}

const RenderGraphTexture& RenderGraph::GetImportedTexture(RenderGraphTextureHandle handle) const
{
    DELTA_ASSERT(handle.IsValid());
    DELTA_ASSERT(handle.index < m_importedTextures.size());
    return m_importedTextures[handle.index];
}

DELTA_ENGINE_NS_END
