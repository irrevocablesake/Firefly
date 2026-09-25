#define VOLK_IMPLEMENTATION
#include<volk/volk.h>

#define VMA_IMPLEMENTATION
#include<vma/vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

#include "validation.h"
#include "renderer.h"

void Renderer::setupLibraries() {
	validationIF.validateResult(SDL_Init(SDL_INIT_VIDEO));
	validationIF.validateResult(SDL_Vulkan_LoadLibrary(NULL));
	validationIF.validateResult(volkInitialize());
}

void Renderer::setupInstance() {
	windowIF.getInstanceExtensionsIF().extensions = SDL_Vulkan_GetInstanceExtensions(&windowIF.getInstanceExtensionsIF().count);

	VkInstanceCreateInfo instanceCI{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &windowIF.applicationInfo,
		.enabledExtensionCount = windowIF.getInstanceExtensionsIF().count,
		.ppEnabledExtensionNames = windowIF.getInstanceExtensionsIF().extensions
	};

	validationIF.validateResult(vkCreateInstance(&instanceCI, nullptr, &windowIF.getInstanceIF().instance));
	volkLoadInstance(windowIF.getInstanceIF().instance);
}

//should also add API version here condition, some ICDs expose only 1.2
void Renderer::pickPhysicalDeviceAndQueue() {
	validationIF.validateResult(vkEnumeratePhysicalDevices(windowIF.getInstanceIF().instance, &windowIF.getPhysicalDeviceIF().count, nullptr));

	windowIF.getPhysicalDeviceIF().devices.resize(windowIF.getPhysicalDeviceIF().count);
	windowIF.getQueue().properties.resize(windowIF.getPhysicalDeviceIF().count);

	validationIF.validateResult(vkEnumeratePhysicalDevices(windowIF.getInstanceIF().instance, &windowIF.getPhysicalDeviceIF().count, windowIF.getPhysicalDeviceIF().devices.data()));

	for (uint32_t i = 0; i < windowIF.getPhysicalDeviceIF().count; i++) {
		VkPhysicalDeviceProperties2 properties{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2
		};

		vkGetPhysicalDeviceProperties2(windowIF.getPhysicalDeviceIF().devices[i], &properties);
		windowIF.getPhysicalDeviceIF().properties.push_back(properties);

		uint32_t queueFamiliesCount{};
		vkGetPhysicalDeviceQueueFamilyProperties2(windowIF.getPhysicalDeviceIF().devices[i], &queueFamiliesCount, nullptr);
		windowIF.getQueue().properties[i].resize(queueFamiliesCount);

		for (uint32_t j = 0; j < queueFamiliesCount; j++) {
			windowIF.getQueue().properties[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		}

		vkGetPhysicalDeviceQueueFamilyProperties2(windowIF.getPhysicalDeviceIF().devices[i], &queueFamiliesCount, windowIF.getQueue().properties[i].data());
	}

	for (uint32_t i = 0; i < windowIF.getPhysicalDeviceIF().count; i++) {
		bool found = false;

		for (uint32_t j = 0; j < windowIF.getQueue().properties[i].size(); j++) {
			bool cond1 = windowIF.getPhysicalDeviceIF().properties[i].properties.deviceType == windowIF.getPhysicalDeviceIF().deviceType;
			bool cond2 = windowIF.getQueue().properties[i][j].queueFamilyProperties.queueFlags & windowIF.getQueue().queueFlags;
			if (cond1 && cond2) {
				windowIF.getPhysicalDeviceIF().index = i;
				windowIF.getQueue().index = j;
				windowIF.getPhysicalDeviceIF().device = windowIF.getPhysicalDeviceIF().devices[i];

				found = true;
				break;
			}

		}
		if (found) {
			break;
		}
	}

	validationIF.validateResult(SDL_Vulkan_GetPresentationSupport(windowIF.getInstanceIF().instance, windowIF.getPhysicalDeviceIF().device, windowIF.getQueue().index));
}

void Renderer::setupLogicalDevice() {
	VkDeviceQueueCreateInfo queueCI{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = windowIF.getQueue().index,
		.queueCount = static_cast<uint32_t>( windowIF.getQueue().count ),
		.pQueuePriorities = &windowIF.getQueue().priorities
	};

	VkPhysicalDeviceVulkan11Features enabledVk11Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.shaderDrawParameters = VK_TRUE
	};

	VkPhysicalDeviceVulkan12Features enabledVk12Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES	,
		.pNext = &enabledVk11Features,
		.descriptorIndexing = true,
		.shaderSampledImageArrayNonUniformIndexing = true,
		.descriptorBindingVariableDescriptorCount = true,
		.runtimeDescriptorArray = true,
		.bufferDeviceAddress = true
	};

	VkPhysicalDeviceVulkan13Features enabledVk13Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &enabledVk12Features,
		.synchronization2 = true,
		.dynamicRendering = true
	};

	VkDeviceCreateInfo deviceCI{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &enabledVk13Features,
		.queueCreateInfoCount = static_cast< uint32_t>( windowIF.getQueue().count ),
		.pQueueCreateInfos = &queueCI,
		.enabledExtensionCount = static_cast<uint32_t>(windowIF.getDeviceExtensionsIF().extensions.size()),
		.ppEnabledExtensionNames = windowIF.getDeviceExtensionsIF().extensions.data(),
	};

	validationIF.validateResult(vkCreateDevice(windowIF.getPhysicalDeviceIF().device, &deviceCI, nullptr, &windowIF.getLogicalDeviceIF().handle));
	volkLoadDevice(windowIF.getLogicalDeviceIF().handle);

	vkGetDeviceQueue(windowIF.getLogicalDeviceIF().handle, windowIF.getQueue().index, 0, &windowIF.getQueue().handle);
}

void Renderer::setupSynchronization() {
	VkSemaphoreCreateInfo semaphoreCI{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	VkFenceCreateInfo fenceCI{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};

	for (uint32_t i = 0; i < windowIF.getFramesIF().maxFramesInFlight; i++) {
		validationIF.validateResult(vkCreateFence(windowIF.getLogicalDeviceIF().handle, &fenceCI, nullptr, &windowIF.getFramesIF().fences[i]));
		validationIF.validateResult(vkCreateSemaphore(windowIF.getLogicalDeviceIF().handle, &semaphoreCI, nullptr, &windowIF.getFramesIF().presentSemaphores[i]));
	}

	windowIF.getSwapchain().renderSemaphores.resize(windowIF.getSwapchain().imageCount);
	for (VkSemaphore& semaphore : windowIF.getSwapchain().renderSemaphores) {
		validationIF.validateResult(vkCreateSemaphore(windowIF.getLogicalDeviceIF().handle, &semaphoreCI, nullptr, &semaphore));
	}
}

void Renderer::setupSwapchain() {
	windowIF.getSwapchain().swapchainCI = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = windowIF.getSurfaceIF().surface,
		.minImageCount = windowIF.getSurfaceIF().surfaceCapabilites.minImageCount,
		.imageFormat = windowIF.getSwapchain().imageFormat,
		.imageColorSpace = windowIF.getSwapchain().imageColorSpace,
		.imageExtent = {
			windowIF.getSurfaceIF().surfaceCapabilites.currentExtent.width,
			windowIF.getSurfaceIF().surfaceCapabilites.currentExtent.height
		},
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR
	};

	validationIF.validateResult(vkCreateSwapchainKHR(windowIF.getLogicalDeviceIF().handle, &windowIF.getSwapchain().swapchainCI, nullptr, &windowIF.getSwapchain().swapchain));

	validationIF.validateResult(vkGetSwapchainImagesKHR(windowIF.getLogicalDeviceIF().handle, windowIF.getSwapchain().swapchain, &windowIF.getSwapchain().imageCount, nullptr));
	windowIF.getSwapchain().images.resize(windowIF.getSwapchain().imageCount);
	validationIF.validateResult(vkGetSwapchainImagesKHR(windowIF.getLogicalDeviceIF().handle, windowIF.getSwapchain().swapchain, &windowIF.getSwapchain().imageCount, windowIF.getSwapchain().images.data()));
	windowIF.getSwapchain().imageViews.resize(windowIF.getSwapchain().imageCount);

	for (uint32_t i = 0; i < windowIF.getSwapchain().imageCount; i++) {
		VkImageViewCreateInfo imageViewCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = windowIF.getSwapchain().images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = windowIF.getSwapchain().imageFormat,
			.subresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1,
				.layerCount = 1
			}
		};

		validationIF.validateResult(vkCreateImageView(windowIF.getLogicalDeviceIF().handle, &imageViewCI, nullptr, &windowIF.getSwapchain().imageViews[i]));
	}
}

void Renderer::setupUI() {
	windowIF.getWindowConfiguration().window = SDL_CreateWindow(
		windowIF.getWindowConfiguration().windowName.c_str(),
		windowIF.getWindowConfiguration().windowWidth,
		windowIF.getWindowConfiguration().windowHeight,
		windowIF.getWindowConfiguration().windowFlags
	);

	validationIF.validateResult(SDL_GetWindowSize(windowIF.getWindowConfiguration().window, &windowIF.getWindowConfiguration().windowSize.x, &windowIF.getWindowConfiguration().windowSize.y));
	validationIF.validateResult(SDL_Vulkan_CreateSurface(windowIF.getWindowConfiguration().window, windowIF.getInstanceIF().instance, nullptr, &windowIF.getSurfaceIF().surface));
	validationIF.validateResult(
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
			windowIF.getPhysicalDeviceIF().device,
			windowIF.getSurfaceIF().surface,
			&windowIF.getSurfaceIF().surfaceCapabilites
		)
	);
}

void Renderer::setupVMA() {
	VmaVulkanFunctions vulkanFns{
		.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
		.vkGetDeviceProcAddr = vkGetDeviceProcAddr
	};

	VmaAllocatorCreateInfo allocatorCI{
		.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
		.physicalDevice = windowIF.getPhysicalDeviceIF().device,
		.device = windowIF.getLogicalDeviceIF().handle,
		.pVulkanFunctions = &vulkanFns,
		.instance = windowIF.getInstanceIF().instance
	};

	validationIF.validateResult(vmaCreateAllocator(&allocatorCI, &windowIF.getInstanceIF().allocator));
}

void Renderer::setupDepthAttachment() {
	for (VkFormat& format : windowIF.getDepthAttachmentIF().formatList) {
		vkGetPhysicalDeviceFormatProperties2(windowIF.getPhysicalDeviceIF().device,format, &windowIF.getDepthAttachmentIF().formatProperties);
		if (windowIF.getDepthAttachmentIF().formatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
			windowIF.getDepthAttachmentIF().format = format;
			break;
		}
	}

	windowIF.getDepthAttachmentIF().imageCI = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = windowIF.getDepthAttachmentIF().format,
		.extent = {
			.width = static_cast<uint32_t> (windowIF.getWindowConfiguration().windowSize.x),
			.height = static_cast<uint32_t>(windowIF.getWindowConfiguration().windowSize.y),
			.depth = 1
		},
		.mipLevels = 1,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VmaAllocationCreateInfo allocationCreateInfo{
		.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
		.usage = VMA_MEMORY_USAGE_AUTO
	};

	validationIF.validateResult(vmaCreateImage(windowIF.getInstanceIF().allocator, &windowIF.getDepthAttachmentIF().imageCI, &allocationCreateInfo, &windowIF.getDepthAttachmentIF().image, &windowIF.getDepthAttachmentIF().allocation, nullptr));

	VkImageViewCreateInfo imageViewCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = windowIF.getDepthAttachmentIF().image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = windowIF.getDepthAttachmentIF().format,
		.subresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};

	validationIF.validateResult(vkCreateImageView(windowIF.getLogicalDeviceIF().handle, &imageViewCreateInfo, nullptr, &windowIF.getDepthAttachmentIF().imageView));
}

void Renderer::setupCommandBuffers() {
	VkCommandPoolCreateInfo commandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		.queueFamilyIndex = windowIF.getQueue().index
	};

	validationIF.validateResult(vkCreateCommandPool(windowIF.getLogicalDeviceIF().handle, &commandPoolCreateInfo, nullptr, &windowIF.getInstanceIF().commandPool));

	VkCommandBufferAllocateInfo commandBufferAllocateCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = windowIF.getInstanceIF().commandPool,
		.commandBufferCount = windowIF.getFramesIF().maxFramesInFlight
	};

	validationIF.validateResult(vkAllocateCommandBuffers(windowIF.getLogicalDeviceIF().handle, &commandBufferAllocateCreateInfo, windowIF.getFramesIF().commandBuffers.data()));
}

void Renderer::createComputePipeline( VkPipeline& pipeline, VkPipelineShaderStageCreateInfo& shaderStages, VkPipelineLayout& pipelineLayout ) {
	VkComputePipelineCreateInfo pipelineCI{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = shaderStages,
		.layout = pipelineLayout
	};

	vkCreateComputePipelines( windowIF.getLogicalDeviceIF().handle, nullptr, 1, &pipelineCI, nullptr, &pipeline );
}

Renderer::FieldState Renderer::generateField(VkFormat imageFormat, VkFilter filtering) {

	FieldState fieldState{};

	VkImageCreateInfo textureImageCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = imageFormat,
		.extent{
			.width = static_cast< uint32_t >( windowIF.getWindowConfiguration().windowSize.x ),
			.height = static_cast< uint32_t >(windowIF.getWindowConfiguration().windowSize.y ),
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
	validationIF.validateResult(vmaCreateImage( windowIF.getInstanceIF().allocator, &textureImageCreateInfo, &textureImageAllocationCreateInfo, &fieldState.image, &fieldState.allocation, nullptr));
	
	VkFenceCreateInfo fenceOneTimeCreateInfo{
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO
	};
	VkFence fenceOneTime;
	validationIF.validateResult(vkCreateFence( windowIF.getLogicalDeviceIF().handle, &fenceOneTimeCreateInfo, nullptr, &fenceOneTime));

	VkCommandBuffer commandBufferOneTime;
	VkCommandBufferAllocateInfo commandBufferOneTimeAllocationInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool = windowIF.getInstanceIF().commandPool,
		.commandBufferCount = 1
	};

	validationIF.validateResult(vkAllocateCommandBuffers( windowIF.getLogicalDeviceIF().handle, &commandBufferOneTimeAllocationInfo, &commandBufferOneTime));

	VkCommandBufferBeginInfo commandBufferOneTimeBeginInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
	};
	validationIF.validateResult(vkBeginCommandBuffer(commandBufferOneTime, &commandBufferOneTimeBeginInfo));

	barrier.transitionImageFromUndefinedToGeneral( commandBufferOneTime, fieldState.image );
	
	validationIF.validateResult(vkEndCommandBuffer(commandBufferOneTime));

	VkSubmitInfo oneTimeSubmitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBufferOneTime
	};

	validationIF.validateResult(vkQueueSubmit(windowIF.getQueue().handle, 1, &oneTimeSubmitInfo, fenceOneTime));
	validationIF.validateResult(vkWaitForFences(windowIF.getLogicalDeviceIF().handle, 1, &fenceOneTime, VK_TRUE, UINT64_MAX));
	
	VkImageViewCreateInfo textureImageViewCreateInfo{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = fieldState.image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = imageFormat,
		.subresourceRange = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};

	validationIF.validateResult(vkCreateImageView( windowIF.getLogicalDeviceIF().handle, &textureImageViewCreateInfo, nullptr, &fieldState.imageView));

	fieldState.descriptor = {
		.sampler = VK_NULL_HANDLE,
		.imageView = fieldState.imageView,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL
	};

	return fieldState;
}

void Renderer::setupDescriptorSet() {
	VkDescriptorPoolSize poolSize{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1,
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize
	};

	validationIF.validateResult(vkCreateDescriptorPool( windowIF.getLogicalDeviceIF().handle, &descriptorPoolCreateInfo, nullptr, &descriptorPool));
	
	VkDescriptorSetLayoutBinding descriptorSetLayoutBinding{
		.binding = 0,
		.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
		.descriptorCount = 1,
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1,
		.pBindings = &descriptorSetLayoutBinding
	};

	vkCreateDescriptorSetLayout( windowIF.getLogicalDeviceIF().handle, &descriptorSetLayoutCI, nullptr, &descriptorSetLayout );
	
	VkDescriptorSetAllocateInfo descriptorAllocateInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &descriptorSetLayout
	};

	validationIF.validateResult(vkAllocateDescriptorSets( windowIF.getLogicalDeviceIF().handle, &descriptorAllocateInfo, &descriptorSet));
}

void Renderer::updateDescriptors() {
		 rayTracedFrame = generateField(VK_FORMAT_R8G8B8A8_UNORM, VK_FILTER_LINEAR);

		VkWriteDescriptorSet imageSet = {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet,
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &rayTracedFrame.descriptor
		};
	

	vkUpdateDescriptorSets(windowIF.getLogicalDeviceIF().handle, 1, &imageSet, 0, nullptr);
}

void Renderer::setupPipeline() {
	setupDescriptorSet();
	updateDescriptors();

	//COMPUTE PIPELINE
	VkPushConstantRange generalPushConstant{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0,
		.size = sizeof( PushConstants )
	};

	VkPipelineLayoutCreateInfo rayTracerLayoutCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = &generalPushConstant
	};

	validationIF.validateResult(vkCreatePipelineLayout(windowIF.getLogicalDeviceIF().handle, &rayTracerLayoutCI, nullptr, &windowIF.getRayTracerPipeline().pipelineLayout));

	VkPipelineShaderStageCreateInfo rayTracerShaderStage{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = shaderIF.loadAndCompileShaders(windowIF.getLogicalDeviceIF().handle, "rayTracerShaderModule", "assets/shaders/rayTracerShader.slang"),
		.pName = "main"
	};

	createComputePipeline(windowIF.getRayTracerPipeline().pipeline, rayTracerShaderStage, windowIF.getRayTracerPipeline().pipelineLayout);
}

void Renderer::animate() {
	bool quit{ false };
	uint64_t startFrameTime = SDL_GetPerformanceCounter();
	uint64_t previousFrameTime = startFrameTime;
	uint64_t frequency = SDL_GetPerformanceFrequency();

	float elapsedTime = 0.0f;

	while (!quit) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				quit = true;
			}

			if (event.type == SDL_EVENT_WINDOW_RESIZED) {
				windowIF.getSwapchain().updateSwapchain = true;
			}
		}

		uint64_t currentFrameTime = SDL_GetPerformanceCounter();

		float dt = (float)(currentFrameTime - previousFrameTime) / (float)frequency;
		previousFrameTime = currentFrameTime;

		elapsedTime = (float)(currentFrameTime - startFrameTime) / (float)frequency;

		validationIF.validateResult(vkWaitForFences(windowIF.getLogicalDeviceIF().handle, 1, &windowIF.getFramesIF().fences[windowIF.getFrameIndex()], true, UINT64_MAX));
		validationIF.validateResult(vkResetFences(windowIF.getLogicalDeviceIF().handle, 1, &windowIF.getFramesIF().fences[windowIF.getFrameIndex()]));

		validationIF.validateSwapchain(vkAcquireNextImageKHR(windowIF.getLogicalDeviceIF().handle, windowIF.getSwapchain().swapchain, UINT64_MAX, windowIF.getFramesIF().presentSemaphores[windowIF.getFrameIndex()], VK_NULL_HANDLE, &windowIF.getImageIndex()), windowIF.getSwapchain().updateSwapchain);

		auto commandBuffer = windowIF.getFramesIF().commandBuffers[windowIF.getFrameIndex()];
		validationIF.validateResult(vkResetCommandBuffer(commandBuffer, 0));

		VkCommandBufferBeginInfo commandBufferBeginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
		};

		//later update the uniform data
		pushConstants.uniformBufferBDA = windowIF.getFramesIF().uniformData[windowIF.getFrameIndex()].deviceAddress;
		resourceManager.copyDataIntoBuffer(windowIF.getFramesIF().uniformData[ windowIF.getFrameIndex() ].allocationInfo.pMappedData, &uniformData, windowIF.getFramesIF().uniformData[windowIF.getFrameIndex()].size);

		validationIF.validateResult(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

		VkViewport viewport{
			.width = float(windowIF.getWindowConfiguration().windowSize.x),
			.height = float(windowIF.getWindowConfiguration().windowSize.y),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		};

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		VkRect2D scissor{
			.extent {
				.width = static_cast<uint32_t>(windowIF.getWindowConfiguration().windowSize.x),
				.height = static_cast<uint32_t>(windowIF.getWindowConfiguration().windowSize.y),
			}
		};
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		pushConstants.uniformBufferBDA = windowIF.getFramesIF().uniformData[windowIF.getFrameIndex()].deviceAddress;
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, windowIF.getRayTracerPipeline().pipeline );
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, windowIF.getRayTracerPipeline().pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		vkCmdPushConstants( commandBuffer, windowIF.getRayTracerPipeline().pipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof( PushConstants ), &pushConstants);
		vkCmdDispatch(commandBuffer, (windowIF.getWindowConfiguration().windowSize.x + 7) / 8, (windowIF.getWindowConfiguration().windowSize.y + 7) / 8, 1);

		barrier.transitionImageFromGeneralToTransferSrc( commandBuffer, rayTracedFrame.image );
		barrier.transitionImageFromUndefinedToTransferDst(commandBuffer, windowIF.getSwapchain().images[ windowIF.getImageIndex()]);

		VkImageCopy copyRegion{
			.srcSubresource{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1
			},
			.dstSubresource{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.layerCount = 1
			},
			.extent{
				.width = windowIF.getSurfaceIF().surfaceCapabilites.currentExtent.width,
				.height = windowIF.getSurfaceIF().surfaceCapabilites.currentExtent.height,
				.depth = 1
			}
		};

		vkCmdCopyImage(
			commandBuffer,
			rayTracedFrame.image,
			VK_IMAGE_LAYOUT_GENERAL,
			windowIF.getSwapchain().images[ windowIF.getImageIndex() ],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion
		);

		barrier.transitionImageTransferDstToPresent( commandBuffer, windowIF.getSwapchain().images[windowIF.getImageIndex()] );
		barrier.transitionImageFromTransferSrcToGeneral( commandBuffer, rayTracedFrame.image );

		vkEndCommandBuffer(commandBuffer);

		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &windowIF.getFramesIF().presentSemaphores[windowIF.getFrameIndex()],
			.pWaitDstStageMask = &waitStages,
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &windowIF.getSwapchain().renderSemaphores[windowIF.getImageIndex()],
		};
		validationIF.validateResult(vkQueueSubmit(windowIF.getQueue().handle, 1, &submitInfo, windowIF.getFramesIF().fences[windowIF.getFrameIndex()]), "Failed to Submit Queue");

		windowIF.getFrameIndex() = (windowIF.getFrameIndex() + 1) % windowIF.getFramesIF().maxFramesInFlight;

		VkPresentInfoKHR presentInfo{
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &windowIF.getSwapchain().renderSemaphores[windowIF.getImageIndex()],
			.swapchainCount = 1,
			.pSwapchains = &windowIF.getSwapchain().swapchain,
			.pImageIndices = &windowIF.getImageIndex()
		};
		validationIF.validateResult(vkQueuePresentKHR(windowIF.getQueue().handle, &presentInfo), "Failed To Present Queue");

		if (windowIF.getSwapchain().updateSwapchain) {
			recreateSwapchain();
		}

		//SDL_Delay(100);
	}
}

void Renderer::recreateSwapchain() {
	windowIF.getSwapchain().updateSwapchain = false;
	vkDeviceWaitIdle(windowIF.getLogicalDeviceIF().handle);
	validationIF.validateResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(windowIF.getPhysicalDeviceIF().device, windowIF.getSurfaceIF().surface, &windowIF.getSurfaceIF().surfaceCapabilites), "Failed to Get Surface Capabilities");
	windowIF.getSwapchain().swapchainCI.oldSwapchain = windowIF.getSwapchain().swapchain;
	windowIF.getSwapchain().swapchainCI.imageExtent = { .width = static_cast<uint32_t>(windowIF.getWindowConfiguration().windowSize.x), .height = static_cast<uint32_t>(windowIF.getWindowConfiguration().windowSize.y) };
	validationIF.validateResult(vkCreateSwapchainKHR(windowIF.getLogicalDeviceIF().handle, &windowIF.getSwapchain().swapchainCI, nullptr, &windowIF.getSwapchain().swapchain), "Failed to Create Swap chain");
	for (uint32_t i = 0; i < windowIF.getSwapchain().imageCount; i++) {
		vkDestroyImageView(windowIF.getLogicalDeviceIF().handle, windowIF.getSwapchain().imageViews[i], nullptr);
	}
	validationIF.validateResult(vkGetSwapchainImagesKHR(windowIF.getLogicalDeviceIF().handle, windowIF.getSwapchain().swapchain, &windowIF.getSwapchain().imageCount, nullptr), "Failed To Create Swap Chain Images");
	windowIF.getSwapchain().images.resize(windowIF.getSwapchain().imageCount);
	validationIF.validateResult(vkGetSwapchainImagesKHR(windowIF.getLogicalDeviceIF().handle, windowIF.getSwapchain().swapchain, &windowIF.getSwapchain().imageCount, windowIF.getSwapchain().images.data()), "Failed To Get Swap Chain Images");
	windowIF.getSwapchain().imageViews.resize(windowIF.getSwapchain().imageCount);
	for (uint32_t i = 0; i < windowIF.getSwapchain().imageCount; i++) {
		VkImageViewCreateInfo viewCI{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = windowIF.getSwapchain().images[i],
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = windowIF.getSwapchain().imageFormat,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1}
		};
		validationIF.validateResult(vkCreateImageView(windowIF.getLogicalDeviceIF().handle, &viewCI, nullptr, &windowIF.getSwapchain().imageViews[i]), "Failed To Create Image View");
	}
	vkDestroySwapchainKHR(windowIF.getLogicalDeviceIF().handle, windowIF.getSwapchain().swapchainCI.oldSwapchain, nullptr);
}

void Renderer::simulate() {
	setup();
	animate();
}

void Renderer::setupData() {
	for (int i = 0; i < windowIF.getFramesIF().maxFramesInFlight; i++) {
		windowIF.getFramesIF().uniformData[i].size = sizeof( UniformData );

		resourceManager.setupBuffer(windowIF.getLogicalDeviceIF().handle, windowIF.getInstanceIF().allocator, windowIF.getFramesIF().uniformData[i] );
		resourceManager.copyDataIntoBuffer(windowIF.getFramesIF().uniformData[i].allocationInfo.pMappedData, &uniformData, windowIF.getFramesIF().uniformData[i].size );
	}
}

void Renderer::setup() {
	setupLibraries();
	setupInstance();
	pickPhysicalDeviceAndQueue();
	setupUI();
	setupLogicalDevice();
	setupVMA();
	setupSwapchain();
	setupSynchronization();
	setupDepthAttachment();
	setupCommandBuffers();
	shaderIF.setupSLANG();
	setupPipeline();
	setupData();
}