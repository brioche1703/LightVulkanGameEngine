#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <glm/glm.hpp>

#include <stdexcept>
#include <array>
#include <vector>
#include <unordered_map>

#include "VulkanBuffer.h"

namespace LightVulkan {
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec2 texCoord;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, color);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

            return attributeDescriptions;
        }

        bool operator==(const Vertex& other) const {
            return pos == other.pos && color == other.color && texCoord == other.texCoord;
        }
    };

    struct VertexHash {
        size_t operator()(Vertex const& vertex) const noexcept {
            return ((std::hash<glm::vec3>()(vertex.pos) ^ (std::hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (std::hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };

    class Model {
    public:
        void load(VulkanDevice& device, const std::string& filepath) {
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str())) {
                throw std::runtime_error(warn + err);
            }

            std::unordered_map<Vertex, uint32_t, VertexHash> uniqueVertices{};

            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Vertex vertex{};

                    vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                    };

                    vertex.color = { 1.0f, 1.0f, 1.0f };

                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }

                    indices.push_back(uniqueVertices[vertex]);
                }
            }
            createVertexBuffer(device);
            createIndexBuffer(device);
        }
        void destroyBuffers(VulkanDevice& device) {
            indexBuffer.destroy(device.getLogicalDevice());
            vertexBuffer.destroy(device.getLogicalDevice());
        }
        std::vector<Vertex>& getVertices() {
            return vertices;
        }
        std::vector<uint32_t>& getIndices() {
            return indices;
        }
        VkBuffer getVertexBuffer() {
            return vertexBuffer.getBuffer();
        }
        VkBuffer getIndexBuffer() {
            return indexBuffer.getBuffer();
        }

    private:
        void createVertexBuffer(VulkanDevice& device) {
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
        void createIndexBuffer(VulkanDevice& device) {
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

    private:
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;
    };
}

