#define VOLK_IMPLEMENTATION
#include<volk/volk.h>

#define VMA_IMPLEMENTATION
#include<vma/vk_mem_alloc.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include<glm/glm.hpp>

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

	VkPhysicalDeviceVulkan13Features enabledVk13Features{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.pNext = &enabledVk11Features,
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
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
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

void Renderer::setupSLANG() {
	slang::createGlobalSession(slangGlobalSession.writeRef());

	auto slangTargets{
		std::to_array< slang::TargetDesc >({{
			.format{SLANG_SPIRV},
			.profile{slangGlobalSession->findProfile("spirv_1_4")}
		}})
	};

	auto slangOptions{
		std::to_array < slang::CompilerOptionEntry>({{
			slang::CompilerOptionName::EmitSpirvDirectly,
			{
				slang::CompilerOptionValueKind::Int, 1
			}
		}})
	};

	slang::SessionDesc slangSessionDesc{
		.targets{
			slangTargets.data()
		},
		.targetCount{
			SlangInt(slangTargets.size())
		},
		.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
		.compilerOptionEntries{
			slangOptions.data()
		},
		.compilerOptionEntryCount{
			uint32_t(slangOptions.size())
		}
	};

	slangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());
}

VkShaderModule Renderer::loadAndCompileShaders(const char* shaderName, const char* filePath) {
	Slang::ComPtr< slang::IModule > slangModule{
		slangSession->loadModuleFromSource(shaderName, filePath, nullptr, nullptr)
	};

	Slang::ComPtr< ISlangBlob > spirv;
	slangModule->getTargetCode(0, spirv.writeRef());

	VkShaderModuleCreateInfo shaderModuleCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = spirv->getBufferSize(),
		.pCode = (uint32_t*)spirv->getBufferPointer()
	};

	VkShaderModule shaderModule{};
	validationIF.validateResult(vkCreateShaderModule(windowIF.getLogicalDeviceIF().handle, &shaderModuleCreateInfo, nullptr, &shaderModule));

	return shaderModule;
}

void Renderer::createPipeline(VkPipeline& pipeline, std::vector< VkPipelineShaderStageCreateInfo >& shaderStages, const std::vector<VkDescriptorSetLayout>& layout, VkPipelineLayout& pipelineLayout, VkFormat format) {

	VkPipelineVertexInputStateCreateInfo vertexInputState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 0,
		.pVertexBindingDescriptions = nullptr,
		.vertexAttributeDescriptionCount = 0,
		.pVertexAttributeDescriptions = nullptr,
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
	};

	std::vector< VkDynamicState > dynamicStates{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = 2,
		.pDynamicStates = dynamicStates.data()
	};

	VkPipelineViewportStateCreateInfo viewportState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1
	};

	VkPipelineDepthStencilStateCreateInfo depthStencilState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_FALSE,
		.depthWriteEnable = VK_FALSE
	};

	VkPipelineRenderingCreateInfo renderingCreateInfo{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
		.colorAttachmentCount = 1,
		.pColorAttachmentFormats = &format,
	};

	VkPipelineColorBlendAttachmentState blendAttachment{
		.colorWriteMask = 0xF
	};

	VkPipelineColorBlendStateCreateInfo colorBlendState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.attachmentCount = 1,
		.pAttachments = &blendAttachment
	};

	VkPipelineRasterizationStateCreateInfo rasterizationState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisampleState{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
	};

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext = &renderingCreateInfo,
		.stageCount = 2,
		.pStages = shaderStages.data(),
		.pVertexInputState = &vertexInputState,
		.pInputAssemblyState = &inputAssemblyState,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizationState,
		.pMultisampleState = &multisampleState,
		.pDepthStencilState = &depthStencilState,
		.pColorBlendState = &colorBlendState,
		.pDynamicState = &dynamicState,
		.layout = pipelineLayout
	};

	validationIF.validateResult(vkCreateGraphicsPipelines(windowIF.getLogicalDeviceIF().handle, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));
}

void Renderer::setupPipeline() {
	//GRAPHICS PIPELINE
	VkPipelineLayoutCreateInfo graphicsPipelineLayoutCI{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO
	};

	validationIF.validateResult(vkCreatePipelineLayout(windowIF.getLogicalDeviceIF().handle, &graphicsPipelineLayoutCI, nullptr, &windowIF.getGraphicsPipeline().pipelineLayout));

	std::vector< VkPipelineShaderStageCreateInfo > graphicsPipelineShaderStages{
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = loadAndCompileShaders("vertexShaderModule", "assets/shaders/vertexShader.slang"),
			.pName = "main"
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = loadAndCompileShaders("fragmentShaderModule", "assets/shaders/fragmentShader.slang"),
			.pName = "main"
		}
	};

	createPipeline(windowIF.getGraphicsPipeline().pipeline, graphicsPipelineShaderStages, {}, windowIF.getGraphicsPipeline().pipelineLayout, VK_FORMAT_B8G8R8A8_SRGB);
}

Renderer::RenderingAttachment Renderer::setRenderingAttachment(VkImageView& imageView) {
	RenderingAttachment attachment{};
	attachment.attachmentInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		.imageView = imageView,
		.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.clearValue{
			windowIF.getWindowConfiguration().clearColor
		}
	};

	attachment.renderingInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		.renderArea{
			.extent{
				.width = static_cast<uint32_t>(windowIF.getWindowConfiguration().windowSize.x),
				.height = static_cast<uint32_t>(windowIF.getWindowConfiguration().windowSize.y)
			},
		},
		.layerCount = 1,
		.colorAttachmentCount = 1,
		.pColorAttachments = &attachment.attachmentInfo
	};

	return attachment;
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

		transitionBarriersIF.transitionImageUndefinedToAttachment(commandBuffer, windowIF.getSwapchain().images[windowIF.getImageIndex()]);
		RenderingAttachment renderingAttachment = setRenderingAttachment(windowIF.getSwapchain().imageViews[windowIF.getImageIndex()]);
		vkCmdBeginRendering(commandBuffer, &renderingAttachment.renderingInfo);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, windowIF.getInstanceIF().graphicsPipeline.pipeline);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);
		vkCmdEndRendering(commandBuffer);
		transitionBarriersIF.transitionImageAttachmentToPresent(commandBuffer, windowIF.getSwapchain().images[windowIF.getImageIndex()]);

		vkEndCommandBuffer(commandBuffer);

		VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

void Renderer::TransitionBarriers::transitionImageUndefinedToAttachment(VkCommandBuffer& commandBuffer, VkImage& image) {
	VkImageMemoryBarrier2 outputBarrier{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
		.image = image,
		.subresourceRange {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.levelCount = 1,
			.layerCount = 1
		}
	};

	VkDependencyInfo barrierDependencyInfo{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &outputBarrier
	};

	vkCmdPipelineBarrier2(commandBuffer, &barrierDependencyInfo);
}

void Renderer::TransitionBarriers::transitionImageAttachmentToPresent(VkCommandBuffer& commandBuffer, VkImage& image) {
	VkImageMemoryBarrier2 barrierPresent{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
		.dstAccessMask = 0,
		.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		.image = image,
		.subresourceRange{.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 }
	};
	VkDependencyInfo barrierPresentDependencyInfo{
		.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
		.imageMemoryBarrierCount = 1,
		.pImageMemoryBarriers = &barrierPresent
	};
	vkCmdPipelineBarrier2(commandBuffer, &barrierPresentDependencyInfo);
}


void Renderer::simulate() {
	setup();
	animate();
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
	setupSLANG();
	setupPipeline();
}