#include "engine.h"

class Firefly {
	Engine engine;
	Engine::TextureHandle rayTracedFrame;

	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	std::vector< VkDescriptorSet > descriptorSet;

	void setup() {
		engine.configure("Firefly", VK_API_VERSION_1_4);

		//Generate Texture
		rayTracedFrame = engine.generateTextureHandle();
		rayTracedFrame.imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
		rayTracedFrame.imageFilter = VK_FILTER_LINEAR;
		rayTracedFrame.width = engine.windowManager.windowSize.x;
		rayTracedFrame.height = engine.windowManager.windowSize.y;
		engine.generateTexture( rayTracedFrame);

		engine.resourceManager.setupDescriptorPool({
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 }
		});

		VkDescriptorSetLayoutBinding binding{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT
		};

		VkDescriptorSetLayout layout = engine.resourceManager.createDescriptorSetLayout({
			binding
		});

		descriptorSet = engine.resourceManager.allocateDescriptorSets({
			layout
		});

		std::vector< VkWriteDescriptorSet > imageSet = { {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = descriptorSet[0],
			.dstBinding = 0,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.pImageInfo = &rayTracedFrame.descriptor
		} };

		engine.resourceManager.updateDescriptorSet( imageSet );


		//COMPUTE PIPELINE
		VkPipelineLayoutCreateInfo rayTracerLayoutCI{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &layout,
		};

		vkCreatePipelineLayout(engine.vulkanContext.logicalDeviceIF.handle, &rayTracerLayoutCI, nullptr, &pipelineLayout);

		VkPipelineShaderStageCreateInfo rayTracerShaderStage{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = engine.compileShader("rayTracerShaderModule", "assets/shaders/rayTracerShader.slang"),
			.pName = "main"
		};

		engine.pipelineManager.createComputePipeline(pipeline, rayTracerShaderStage, pipelineLayout);
	}

	void animate() {
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
					engine.updateSwapchain();
				}
			}

			uint64_t currentFrameTime = SDL_GetPerformanceCounter();

			float dt = (float)(currentFrameTime - previousFrameTime) / (float)frequency;
			previousFrameTime = currentFrameTime;

			elapsedTime = (float)(currentFrameTime - startFrameTime) / (float)frequency;

			vkWaitForFences( engine.vulkanContext.logicalDeviceIF.handle, 1, &engine.getCurrentFrame().frameFence, true, UINT64_MAX);
			vkResetFences(engine.vulkanContext.logicalDeviceIF.handle, 1, &engine.getCurrentFrame().frameFence);

			vkAcquireNextImageKHR(engine.vulkanContext.logicalDeviceIF.handle, engine.swapchainManager.swapchain, UINT64_MAX, engine.getCurrentFrame().imageAvailable, VK_NULL_HANDLE, &engine.swapchainManager.imageIndex);

			auto commandBuffer = engine.vulkanContext.getCurrentFrame().commandBuffer;
			vkResetCommandBuffer(commandBuffer, 0);

			VkCommandBufferBeginInfo commandBufferBeginInfo{
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
			};

			vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

			VkViewport viewport{
				.width = float(engine.windowManager.windowSize.x),
				.height = float(engine.windowManager.windowSize.y),
				.minDepth = 0.0f,
				.maxDepth = 1.0f
			};

			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			VkRect2D scissor{
				.extent {
					.width = static_cast<uint32_t>(engine.windowManager.windowSize.x),
					.height = static_cast<uint32_t>(engine.windowManager.windowSize.y),
				}
			};
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSet[0], 0, nullptr);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			vkCmdDispatch(commandBuffer, ( engine.windowManager.windowSize.x + 7) / 8, (engine.windowManager.windowSize.y + 7) / 8, 1);

			engine.barrier.transitionImageFromGeneralToTransferSrc(commandBuffer, rayTracedFrame.image);
			engine.barrier.transitionImageFromUndefinedToTransferDst(commandBuffer, engine.swapchainManager.images[engine.swapchainManager.imageIndex]);

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
					.width = engine.windowManager.surfaceCapabilites.currentExtent.width,
					.height = engine.windowManager.surfaceCapabilites.currentExtent.height,
					.depth = 1
				}
			};

			vkCmdCopyImage(
				commandBuffer,
				rayTracedFrame.image,
				VK_IMAGE_LAYOUT_GENERAL,
				engine.swapchainManager.images[ engine.swapchainManager.imageIndex ],
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion
			);

			engine.barrier.transitionImageTransferDstToPresent(commandBuffer, engine.swapchainManager.images[engine.swapchainManager.imageIndex]);
			engine.barrier.transitionImageFromTransferSrcToGeneral(commandBuffer, rayTracedFrame.image);

			vkEndCommandBuffer(commandBuffer);

			VkPipelineStageFlags waitStages = VK_PIPELINE_STAGE_TRANSFER_BIT;
			VkSubmitInfo submitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &engine.getCurrentFrame().imageAvailable,
				.pWaitDstStageMask = &waitStages,
				.commandBufferCount = 1,
				.pCommandBuffers = &commandBuffer,
				.signalSemaphoreCount = 1,
				.pSignalSemaphores = &engine.swapchainManager.renderSemaphores[engine.swapchainManager.imageIndex],
			};
			vkQueueSubmit( engine.vulkanContext.queueIF.handle, 1, &submitInfo, engine.getCurrentFrame().frameFence );

			engine.vulkanContext.currentFrame = (engine.vulkanContext.currentFrame + 1) % engine.vulkanContext.MAX_FRAMES_IN_FLIGHT;

			VkPresentInfoKHR presentInfo{
				.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
				.waitSemaphoreCount = 1,
				.pWaitSemaphores = &engine.swapchainManager.renderSemaphores[engine.swapchainManager.imageIndex],
				.swapchainCount = 1,
				.pSwapchains = &engine.swapchainManager.swapchain,
				.pImageIndices = &engine.swapchainManager.imageIndex
			};
			vkQueuePresentKHR(engine.vulkanContext.queueIF.handle, &presentInfo);

			engine.updateSwapchain();
		}
	}

public:
	void simulate() {
		setup();
		animate();
	}
};

int main() {
	Firefly firefly;
	firefly.simulate();

	return 0;
}