#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

#include "VulkanDevice.h"
#include "VulkanUtils.h"

namespace LightVulkan {
    class VulkanBuffer {
    public:
        void create(VulkanDevice& device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device.getLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
                throw std::runtime_error("failed to create buffer!");
            }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device.getLogicalDevice(), buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = Utils::findMemoryType(device.getPhysicalDevice(), memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(device.getLogicalDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate buffer memory!");
            }

            vkBindBufferMemory(device.getLogicalDevice(), buffer, memory, 0);
        }
        void destroy(VkDevice device) {
            vkDestroyBuffer(device, buffer, nullptr);
            vkFreeMemory(device, memory, nullptr);
        }
        VkBuffer getBuffer() {
            return buffer;
        }
        VkDeviceMemory getMemory() {
            return memory;
        }
        static void copyBuffer(VulkanDevice& device, VulkanBuffer srcBuffer, VulkanBuffer dstBuffer, VkDeviceSize size) {
            VulkanCommandBuffer commandBuffer;
            commandBuffer.createSingleTimeCommandBuffer(device);
            commandBuffer.beginSingleTimeCommands();

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer.get(), srcBuffer.getBuffer(), dstBuffer.getBuffer(), 1, &copyRegion);

            commandBuffer.endSingleTimeCommands();
        }

    private:
        VkBuffer buffer;
        VkDeviceMemory memory;
    };
}
