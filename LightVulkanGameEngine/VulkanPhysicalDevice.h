#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanInstance.h"
#include "VulkanSurfaceKHR.h"
#include "VulkanQueueFamily.h"
#include "VulkanSwapChain.h"

namespace LightVulkan {
	class VulkanPhysicalDevice {
	public:
		void pick(VulkanInstance& instance, VkSampleCountFlagBits& msaaSamples, VulkanSurfaceKHR surface, const std::vector<const char*> deviceExtensions) {
			uint32_t deviceCount = 0;
			vkEnumeratePhysicalDevices(instance.get(), &deviceCount, nullptr);

			if (deviceCount == 0) {
				throw std::runtime_error("failed to find GPUs with Vulkan support!");
			}

			std::vector<VkPhysicalDevice> devices(deviceCount);
			vkEnumeratePhysicalDevices(instance.get(), &deviceCount, devices.data());

			for (const auto& device : devices) {
				if (isDeviceSuitable(device, surface.get(), deviceExtensions)) {
					physicalDevice = device;
					msaaSamples = getMaxUsableSampleCount();
					break;
				}
			}

			if (physicalDevice == VK_NULL_HANDLE) {
				throw std::runtime_error("failed to find a suitable GPU!");
			}
		}
		VkPhysicalDevice& get() {
			return physicalDevice;
		}
		bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface, const std::vector<const char*> deviceExtensions) {
			QueueFamilyIndices indices = findQueueFamilies(device, surface);

			bool extensionsSupported = checkDeviceExtensionSupport(device, deviceExtensions);

			bool swapChainAdequate = false;
			if (extensionsSupported) {
				SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
				swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
			}

			VkPhysicalDeviceFeatures supportedFeatures;
			vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

			return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
		}
		VkSampleCountFlagBits getMaxUsableSampleCount() {
			VkPhysicalDeviceProperties physicalDeviceProperties;
			vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

			VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
			if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
			if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
			if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
			if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
			if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
			if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

			return VK_SAMPLE_COUNT_1_BIT;
		}
		bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> deviceExtensions) {
			uint32_t extensionCount;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			std::vector<VkExtensionProperties> availableExtensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

			std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

			for (const auto& extension : availableExtensions) {
				requiredExtensions.erase(extension.extensionName);
			}

			return requiredExtensions.empty();
		}

	private:
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	};
}
