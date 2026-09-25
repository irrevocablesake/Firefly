#pragma once

#include<SDL3/SDL.h>
#include<SDL3/SDL_vulkan.h>

#include<volk/volk.h>
#include<glm/glm.hpp>
#include<vma/vk_mem_alloc.h>

#include<iostream>
#include<cstdlib>
#include<array>
#include<vector>

#include "barriers.h"
#include "shaders.h"
#include "resourceManager.h"

using namespace std;

class Renderer {
	struct WindowIF {
		VkApplicationInfo applicationInfo{
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "Firefly",
			.apiVersion = VK_API_VERSION_1_4
		};

		struct SurfaceIF {
			VkSurfaceKHR surface{};
			VkSurfaceCapabilitiesKHR surfaceCapabilites{};
		} surfaceIF;

		struct windowConfiguration {
			SDL_Window *window;
			string windowName{ "Path Tracer" };
			int windowWidth{ 400u };
			int windowHeight{ 400u };
			SDL_WindowFlags windowFlags{
				SDL_WINDOW_VULKAN | SDL_WINDOW_FULLSCREEN
			};

			glm::ivec2 windowSize{};

			VkClearColorValue clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
		} windowConfiguration;

		struct DepthAttachmentIF {
			VkImageCreateInfo imageCI;

			vector< VkFormat > formatList{
				VK_FORMAT_D32_SFLOAT_S8_UINT,
				VK_FORMAT_D24_UNORM_S8_UINT
			};

			VkFormat format{ VK_FORMAT_UNDEFINED };
			VkFormatProperties2 formatProperties{
				.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2
			};

			VkImage image;
			VkImageView imageView;

			VmaAllocation allocation{};
		} depthAttachmentIF;

		struct Instance {
			VkInstance instance;
			VmaAllocator allocator;
			VkCommandPool commandPool{};

			struct Extensions {
				struct Instance {
					uint32_t count;
					char const* const* extensions;
				} instanceIF;

				struct Device {
					const vector< const char* > extensions{
						VK_KHR_SWAPCHAIN_EXTENSION_NAME
					};
				} deviceIF;
			} extensionsIF;
			struct DeviceInterface {
				struct Physical {
					uint32_t count;
					uint32_t index;

					vector<VkPhysicalDevice> devices;
					vector<VkPhysicalDeviceProperties2> properties;
					VkPhysicalDevice device;

					VkPhysicalDeviceType deviceType = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
				} physicalIF;

				struct Logical {
					VkDevice handle;
					struct Queue {
						const int count = 1;
						const float priorities{ 1.0f };
						vector< vector<VkQueueFamilyProperties2> > properties{};
						uint32_t index;

						VkQueue handle;
						VkQueueFlags queueFlags = VK_QUEUE_GRAPHICS_BIT;
					} queueIF;

					struct Features {

					} featuresIF;

					struct SwapchainConfiguration {
						VkSwapchainCreateInfoKHR swapchainCI{};

						const VkFormat imageFormat{ VK_FORMAT_B8G8R8A8_SRGB };
						const VkColorSpaceKHR imageColorSpace{ VK_COLORSPACE_SRGB_NONLINEAR_KHR };
						VkSwapchainKHR swapchain;
						uint32_t imageCount{ 0 };
						vector<VkImage> images;
						vector< VkImageView> imageViews;

						vector< VkSemaphore > renderSemaphores;

						uint32_t imageIndex{ 0 };

						bool updateSwapchain{ false };
					} swapchainIF;
				} logicalIF;
			} deviceIF;

			struct Pipeline {
				VkPipelineLayout pipelineLayout;
				VkPipeline pipeline;
			} graphicsPipeline, rayTracerPipeline;
		} instanceIF;

		struct FramesConfiguration {
			static constexpr uint32_t maxFramesInFlight{ 2 };

			array< VkFence, maxFramesInFlight > fences;
			array< VkSemaphore, maxFramesInFlight > presentSemaphores;
			array< VkCommandBuffer, maxFramesInFlight > commandBuffers{};
			array< ResourceManager::BufferHandle, maxFramesInFlight > uniformData;

			int frameIndex{ 0 };
		} framesIF;

		//Accessors
		WindowIF::Instance& getInstanceIF() {
			return instanceIF;
		}

		auto& getPhysicalDeviceIF() {
			return instanceIF.deviceIF.physicalIF;
		}

		auto& getLogicalDeviceIF() {
			return instanceIF.deviceIF.logicalIF;
		}

		auto& getExtensionsIF() {
			return instanceIF.extensionsIF;
		}

		auto& getInstanceExtensionsIF(){
			return instanceIF.extensionsIF.instanceIF;
		}

		auto& getDeviceExtensionsIF() {
			return instanceIF.extensionsIF.deviceIF;
		}

		auto& getSwapchain() {
			return instanceIF.deviceIF.logicalIF.swapchainIF;
		}

		auto& getQueue() {
			return instanceIF.deviceIF.logicalIF.queueIF;
		}

		auto& getFeatures() {
			return instanceIF.deviceIF.logicalIF.featuresIF;
		}

		auto& getApplicationInfo() {
			return applicationInfo;
		}

		auto& getFramesIF() {
			return framesIF;
		}

		auto& getWindowConfiguration(){
			return windowConfiguration;
		}

		auto& getSurfaceIF() {
			return surfaceIF;
		}

		auto& getDepthAttachmentIF() {
			return depthAttachmentIF;
		}

		auto& getGraphicsPipeline() {
			return instanceIF.graphicsPipeline;
		}

		auto& getRayTracerPipeline() {
			return instanceIF.rayTracerPipeline;
		}

		auto& getFrameIndex() {
			return framesIF.frameIndex;
		}

		auto& getImageIndex() {
			return instanceIF.deviceIF.logicalIF.swapchainIF.imageIndex;
		}
	} windowIF;

	struct UniformData {
		float aspectRatio = 1.0;
	} uniformData;

	struct PushConstants {
		VkDeviceAddress uniformBufferBDA;
	} pushConstants;

	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;

	void setupLibraries();
	
	void setupInstance();
	
	void pickPhysicalDeviceAndQueue();
	
	void setupLogicalDevice();
	
	void setupSynchronization();
	
	void setupUI();
	
	void setupDepthAttachment();
	
	void setupVMA();
	
	void setupCommandBuffers();
	
	void setupDescriptorSet();
	void updateDescriptors();
	
	void setupSwapchain();
	void recreateSwapchain();

	void createComputePipeline(VkPipeline& pipeline, VkPipelineShaderStageCreateInfo& shaderStages, VkPipelineLayout& pipelineLayout);
	void setupPipeline();

	void setupUniformBuffer();

	void animate();
	
	void setup();
	void setupData();

	ResourceManager::TextureHandle rayTracedFrame;

	Barriers barrier;
	Shaders shaderIF;
	ResourceManager resourceManager;
	
	public:
		void simulate();
};