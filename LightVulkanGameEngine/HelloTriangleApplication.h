#pragma once

#include "VulkanApplication.h"

using namespace LightVulkan;

class HelloTriangleApplication : public VulkanApplication {
public:
    void run() {
        VulkanApplication::run("Hello Triangle Vulkan");
    }

private:
    void createVertexBuffer() override {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VulkanBuffer stagingBuffer;
        stagingBuffer.create(device, bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(device.getLogicalDevice(), stagingBuffer.getMemory(), 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t)bufferSize);
        vkUnmapMemory(device.getLogicalDevice(), stagingBuffer.getMemory());

        vertexBuffer.create(device, bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VulkanBuffer::copyBuffer(device, stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device.getLogicalDevice(), stagingBuffer.getBuffer(), nullptr);
        vkFreeMemory(device.getLogicalDevice(), stagingBuffer.getMemory(), nullptr);
    }
    void createIndexBuffer() override {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

        VulkanBuffer stagingBuffer;
        stagingBuffer.create(device, bufferSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* data;
        vkMapMemory(device.getLogicalDevice(), stagingBuffer.getMemory(), 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vkUnmapMemory(device.getLogicalDevice(), stagingBuffer.getMemory());

        indexBuffer.create(device, bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VulkanBuffer::copyBuffer(device, stagingBuffer, indexBuffer, bufferSize);

        stagingBuffer.destroy(device.getLogicalDevice());
    }
};
