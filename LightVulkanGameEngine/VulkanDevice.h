#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanPhysicalDevice.h"
#include "VulkanLogicalDevice.h"
#include "Window.h"

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

namespace LightVulkan {
    class VulkanDevice {
    public:
        void setUp(VulkanInstance& instance, Window& window, VkSampleCountFlagBits& msaaSamples) {
            surface.setUp(instance, window);
            physicalDevice.pick(instance, msaaSamples, surface.get(), deviceExtensions);
            device.setUp(physicalDevice.get(), surface.get(), deviceExtensions);
        }
        void destroy(VkInstance& instance) {
            vkDestroyDevice(device.get(), nullptr);
            surface.destroy(instance);
        }
        VkPhysicalDevice getPhysicalDevice() {
            return physicalDevice.get();
        }
        VkDevice getLogicalDevice() {
            return device.get();
        }
        VkQueue getGraphicsQueue() {
            return device.getGraphicsQueue();
        }
        VkQueue getPresentQueue() {
            return device.getPresentQueue();
        }
        VkCommandPool getCommandPool() {
            return device.getCommandPool();
        }
        VkSurfaceKHR getSurface() {
            return surface.get();
        }
        void createCommandPool() {
            QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice.get(), surface.get());

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

            if (vkCreateCommandPool(device.get(), &poolInfo, nullptr, &device.getCommandPool()) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics command pool!");
            }
        }

    private:
        VulkanPhysicalDevice physicalDevice;
        VulkanLogicalDevice device;
        VulkanSurfaceKHR surface;
    };

}
