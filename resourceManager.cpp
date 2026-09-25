#include "resourceManager.h"

#include<cstring>

void ResourceManager::copyDataIntoBuffer(void* destination, const void* source, size_t size) {
	memcpy(destination, source, size);
}

void ResourceManager::setupBuffer(VkDevice& device, VmaAllocator& allocator, BufferHandle& handle) {
	VkBufferCreateInfo uniformBufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = handle.size,
		.usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
	};

	VmaAllocationCreateInfo uniformBufferAllocationCreateInfo{
		.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO
	};

	//validation later
	vmaCreateBuffer(allocator, &uniformBufferCreateInfo, &uniformBufferAllocationCreateInfo, &handle.buffer, &handle.allocation, &handle.allocationInfo);

	VkBufferDeviceAddressInfo uniformBDAInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
		.buffer = handle.buffer
	};

	handle.deviceAddress = vkGetBufferDeviceAddress(device, &uniformBDAInfo);
}

void ResourceManager::generateTexture(VkDevice& device, VmaAllocator& allocator, VkCommandPool& commandPool, TextureHandle& handle) {
	VkImageCreateInfo textureImageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = handle.imageFormat,
		.extent{
			.width = static_cast<uint32_t>( handle.width ),
			.height = static_cast<uint32_t>( handle.height ),
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo textureImageAllocationCreateInfo{
		.usage = VMA_MEMORY_USAGE_AUTO
	};

	//add validation later
	vmaCreateImage( allocator, &textureImageCreateInfo, &textureImageAllocationCreateInfo, &handle.image, &handle.allocation, nullptr);

	VkImageViewCreateInfo textureImageViewCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = handle.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = handle.imageFormat,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};

	vkCreateImageView(device, &textureImageViewCreateInfo, nullptr, &handle.imageView);

	handle.descriptor = {
		.sampler = VK_NULL_HANDLE,
		.imageView = handle.imageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};
}