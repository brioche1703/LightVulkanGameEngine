#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>

#include "VulkanDevice.h"
#include "VulkanImageView.h"

namespace LightVulkan {

	class VulkanSwapChain {
	public:
		void create(VulkanDevice& device, Window window) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device.getPhysicalDevice(), device.getSurface());

			VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
			VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
			VkExtent2D extentIn = chooseSwapExtent(swapChainSupport.capabilities, window);

			uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
			if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
				imageCount = swapChainSupport.capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = device.getSurface();

			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = extentIn;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies(device.getPhysicalDevice(), device.getSurface());
			uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

			if (indices.graphicsFamily != indices.presentFamily) {
				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = queueFamilyIndices;
			}
			else {
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			}

			createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.presentMode = presentMode;
			createInfo.clipped = VK_TRUE;

			if (vkCreateSwapchainKHR(device.getLogicalDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
				throw std::runtime_error("failed to create swap chain!");
			}

			vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapChain, &imageCount, nullptr);
			images.resize(imageCount);
			vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapChain, &imageCount, images.data());

			imageFormat = surfaceFormat.format;
			extent = extentIn;
		}
        void destroy(VkDevice device) {
            vkDestroySwapchainKHR(device, swapChain, nullptr);
        }
        void destroyFrameBuffers(VkDevice device) {
            for (auto framebuffer : framebuffers) {
                vkDestroyFramebuffer(device, framebuffer, nullptr);
            }
        }
        void destroyImageViews(VkDevice device) {
            for (auto imageView : imageViews) {
                imageView.destroy(device);
            }
        }
		VkSwapchainKHR get() {
			return swapChain;
		}
		std::vector<VkImage>& getImages() {
			return images;
		}
		VkFormat getImageFormat() {
			return imageFormat;
		}
		VkExtent2D getExtent() {
			return extent;
		}
		std::vector<VulkanImageView>& getImageViews() {
			return imageViews;
		}
		std::vector<VkFramebuffer>& getFramebuffers() {
			return framebuffers;
		}
		void createImageViews(VkDevice device) {
			imageViews.resize(images.size());
			for (uint32_t i = 0; i < images.size(); i++) {
				imageViews[i].create(device, images[i], imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
			}
		}

	private:
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
			for (const auto& availableFormat : availableFormats) {
				if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
					return availableFormat;
				}
			}

			return availableFormats[0];
		}
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
			for (const auto& availablePresentMode : availablePresentModes) {
				if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
					return availablePresentMode;
				}
			}

			return VK_PRESENT_MODE_FIFO_KHR;
		}
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window window) {
			if (capabilities.currentExtent.width != UINT32_MAX) {
				return capabilities.currentExtent;
			}
			else {
				int width, height;
				glfwGetFramebufferSize(window.get(), &width, &height);

				VkExtent2D actualExtent = {
					static_cast<uint32_t>(width),
					static_cast<uint32_t>(height)
				};

				actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
				actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

				return actualExtent;
			}
		}

	private:
		VkSwapchainKHR swapChain;
		std::vector<VkImage> images;
		VkFormat imageFormat;
		VkExtent2D extent;
		std::vector<VulkanImageView> imageViews;
		std::vector<VkFramebuffer> framebuffers;
	};
}
