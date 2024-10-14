#pragma once

#include "EngineIncludes.h"

#include <deque>
#include <d3d12.h>
#include <wrl.h>
#include <memory>

DELTA_ENGINE_NS_BEGIN

//class EngineMain;

class DXUploadBuffer {
public:
	struct AddrPair {
		void* m_cpuAddr;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddr;
	};

	DXUploadBuffer(size_t pageSize);
	DXUploadBuffer(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t pageSize);

	void SetDevice(const Microsoft::WRL::ComPtr<ID3D12Device>& device);
	AddrPair Allocate(size_t sizeInBytes, size_t alignment);
	void Reset();
private:


	struct Page {
		Page(Microsoft::WRL::ComPtr<ID3D12Device> device, size_t size);
		~Page();

		bool HasSpace(size_t sizeInBytes, size_t alignment) const;
		AddrPair Allocate(size_t sizeInBytes, size_t alignment);
		void Reset();

	private:
		size_t m_offset;
		size_t m_maxSize;
		void* m_cpuAddr;
		D3D12_GPU_VIRTUAL_ADDRESS m_gpuAddr;
		Microsoft::WRL::ComPtr<ID3D12Device> m_device;
		Microsoft::WRL::ComPtr<ID3D12Resource> m_dxResource;
	};

	std::deque<std::shared_ptr<Page>> m_pagePool;
	std::deque<std::shared_ptr<Page>> m_freePagePool;
	size_t m_pageSize;
	std::shared_ptr<Page> m_currentPage;
	Microsoft::WRL::ComPtr<ID3D12Device> m_device;
	
};

DELTA_ENGINE_NS_END