#include "resourceManager.h"

#include<cstring>

void ResourceManager::copyDataIntoBuffer(void* destination, void* source, size_t size) {
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