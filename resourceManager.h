#pragma once

#include<volk/volk.h>
#include<vma/vk_mem_alloc.h>

class ResourceManager {
	public:
		struct BufferHandle {

			size_t size;

			VkBuffer buffer{ VK_NULL_HANDLE };

			VmaAllocation allocation{ VK_NULL_HANDLE };
			VmaAllocationInfo allocationInfo{};

			VkDeviceAddress deviceAddress{};
			void* mapped{ nullptr };
		};
	
		void setupBuffer( VkDevice& device, VmaAllocator& allocator, BufferHandle& handle );
		void copyDataIntoBuffer( void* destination, void* source, size_t size );
};