#pragma once

#include<volk/volk.h>

class Barriers {
	public:
		void transitionImageFromUndefinedToGeneral(VkCommandBuffer& commandBuffer, VkImage& image);
		void transitionImageFromGeneralToTransferSrc(VkCommandBuffer& commandBuffer, VkImage& image);
		void transitionImageFromTransferSrcToGeneral(VkCommandBuffer& commandBuffer, VkImage& image);
		void transitionImageTransferDstToPresent(VkCommandBuffer& commandBuffer, VkImage& image);
		void transitionImageFromUndefinedToTransferDst(VkCommandBuffer& commandBuffer, VkImage& image);
};