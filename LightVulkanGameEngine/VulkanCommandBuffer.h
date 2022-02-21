#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanDevice.h"

namespace LightVulkan {
    class VulkanCommandBuffer {
    public:
        void create(VulkanDevice* deviceIn, VkCommandBufferLevel level) 
        {
            device = deviceIn;
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = level;
            allocInfo.commandPool = device->getCommandPool();
            allocInfo.commandBufferCount = 1;

            vkAllocateCommandBuffers(device->getLogicalDevice(), &allocInfo, &commandBuffer);
        }
        void createSingleTimeCommandBuffer(VulkanDevice* deviceIn) {
            device = deviceIn;
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = device->getCommandPool();
            allocInfo.commandBufferCount = 1;

            vkAllocateCommandBuffers(device->getLogicalDevice(), &allocInfo, &commandBuffer);
        }
        VkCommandBuffer get() {
            return commandBuffer;
        }
        VkCommandBuffer* getp() {
            return &commandBuffer;
        }
        void beginSingleTimeCommands() {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);
        }
        void endSingleTimeCommands() {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(device->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(device->getGraphicsQueue());

            vkFreeCommandBuffers(device->getLogicalDevice(), device->getCommandPool(), 1, &commandBuffer);
        }

    private:
        VkCommandBuffer commandBuffer;

        VulkanDevice* device;
    };
}
