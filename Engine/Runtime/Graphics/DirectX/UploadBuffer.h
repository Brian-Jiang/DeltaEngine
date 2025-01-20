#pragma once

#include "EngineIncludes.h"

#include <deque>
#include <d3d12.h>
#include <wrl.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

class Device;

class UploadBuffer {
public:
    // Use to upload data to the GPU
    struct Allocation {
        void* CPU;
        D3D12_GPU_VIRTUAL_ADDRESS GPU;
    };

    /**
     * The maximum size of an allocation is the size of a single page.
     */
    size_t GetPageSize() const {
        return m_PageSize;
    }

    /**
     * Allocate memory in an Upload heap.
     * An allocation must not exceed the size of a page.
     * Use a memcpy or similar method to copy the
     * buffer data to CPU pointer in the Allocation structure returned from
     * this function.
     */
    Allocation Allocate(size_t sizeInBytes, size_t alignment);

    /**
     * Release all allocated pages. This should only be done when the command list
     * is finished executing on the CommandQueue.
     */
    void Reset();

    friend struct std::default_delete<UploadBuffer>;

    /**
     * @param pageSize The size to use to allocate new pages in GPU memory.
     */
    explicit UploadBuffer(Device& device, size_t pageSize = 2 * 1024 * 1024);
    virtual ~UploadBuffer();

private:
    // A single page for the allocator.
    struct Page {
        Page(Device& device, size_t sizeInBytes);
        ~Page();

        // Check to see if the page has room to satisfy the requested
        // allocation.
        bool HasSpace(size_t sizeInBytes, size_t alignment) const;

        // Allocate memory from the page.
        // Throws std::bad_alloc if the the allocation size is larger
        // that the page size or the size of the allocation exceeds the
        // remaining space in the page.
        Allocation Allocate(size_t sizeInBytes, size_t alignment);

        // Reset the page for reuse.
        void Reset();

    private:
        Device& m_Device;
        Microsoft::WRL::ComPtr<ID3D12Resource> m_d3d12Resource;

        // Base pointer.
        void* m_CPUPtr;
        D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr;

        // Allocated page size.
        size_t m_PageSize;
        // Current allocation offset in bytes.
        size_t m_Offset;
    };

    // A pool of memory pages.
    using PagePool = std::deque<std::shared_ptr<Page>>;

    // The device that was used to create this upload buffer.
    Device& m_Device;

    // Request a page from the pool of available pages
    // or create a new page if there are no available pages.
    std::shared_ptr<Page> RequestPage();

    PagePool m_PagePool;
    PagePool m_AvailablePages;

    std::shared_ptr<Page> m_CurrentPage;

    // The size of each page of memory.
    size_t m_PageSize;
};

DELTA_ENGINE_NS_END