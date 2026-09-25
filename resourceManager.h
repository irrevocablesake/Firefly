#pragma once

#include<volk/volk.h>
#include<vma/vk_mem_alloc.h>

#define KHRONOS_STATIC 
#include<ktx.h>
#include<ktxvulkan.h>

#include "barriers.h"

class ResourceManager {
	Barriers barrier;

	public:
		struct BufferHandle {

			size_t size;

			VkBuffer buffer{ VK_NULL_HANDLE };

			VmaAllocation allocation{ VK_NULL_HANDLE };
			VmaAllocationInfo allocationInfo{};

			VkDeviceAddress deviceAddress{};
			void* mapped{ nullptr };
		};

		struct TextureHandle {
			VkImage image;
			VkImageView imageView;
			VmaAllocation allocation;
			VkSampler sampler;

			VkDescriptorImageInfo descriptor;

			VkFormat imageFormat;
			VkFilter imageFilter;

			uint32_t width;
			uint32_t height;
		};

	public:
		void setupBuffer( VkDevice& device, VmaAllocator& allocator, BufferHandle& handle );
		void copyDataIntoBuffer( void* destination, const void* source, size_t size );

		void generateTexture(VkDevice& device, VmaAllocator& allocator, VkCommandPool& commandPool, TextureHandle& handle );
};