#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>

#include "Window.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanResource.h"
#include "VulkanTexture.h"
#include "VulkanSampler.h"
#include "VulkanSyncObjects.h"
#include "VulkanShaderModule.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

namespace LightVulkan {
    struct BasicVertex {
        glm::vec2 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription() {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(BasicVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(BasicVertex, pos);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(BasicVertex, color);

            return attributeDescriptions;
        }
    };

    const std::vector<BasicVertex> vertices = {
        {{-0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}},
        {{ 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
    };

    const std::vector<uint32_t> indices = {
        0, 1, 2 
    };

    class VulkanApplication {
    public:
        void run(std::string title) {
            window.setUp(WIDTH, HEIGHT, title.c_str());
            initVulkan();
            mainLoop();
            cleanup();
        }

    protected:
        Window window;

        VulkanInstance instance;
        VulkanDebugMessenger debugMessenger;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VulkanDevice device;
        VulkanSwapChain swapChain;

        VkRenderPass renderPass;
        VkPipelineLayout pipelineLayout;
        VkPipeline graphicsPipeline;

        VulkanResource colorResource;
        VulkanDepthResource depthResource;

        VulkanBuffer vertexBuffer;
        VulkanBuffer indexBuffer;

        std::vector<VulkanBuffer> uniformBuffers;
        std::vector<VkCommandBuffer> commandBuffers;

        VkDescriptorSetLayout descriptorSetLayout;
        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        VulkanSyncObjects syncObjects;
        size_t currentFrame = 0;

        void mainLoop() {
            while (!glfwWindowShouldClose(window.get())) {
                glfwPollEvents();
                drawFrame();
            }

            vkDeviceWaitIdle(device.getLogicalDevice());
        }

        virtual void initVulkan() {
            instance.setUp(debugMessenger);
            debugMessenger.setUp(instance.get());
            device.setUp(instance, window, msaaSamples);
            swapChain.create(device, window);
            swapChain.createImageViews(device.getLogicalDevice());
            createRenderPass();
            createDescriptorSetLayout();
            createGraphicsPipeline();
            createCommandPool();
            createColorResources();
            createDepthResources();
            createFramebuffers();
            createVertexBuffer();
            createIndexBuffer();
            createUniformBuffers();
            createDescriptorPool();
            createDescriptorSets();
            createCommandBuffers();
            syncObjects.create(device, swapChain, MAX_FRAMES_IN_FLIGHT);
        }
        virtual void cleanupSwapChain() {
            depthResource.destroy(device.getLogicalDevice());
            colorResource.destroy(device.getLogicalDevice());

            swapChain.destroyFrameBuffers(device.getLogicalDevice());

            vkFreeCommandBuffers(device.getLogicalDevice(), device.getCommandPool(), static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

            vkDestroyPipeline(device.getLogicalDevice(), graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(device.getLogicalDevice(), pipelineLayout, nullptr);
            vkDestroyRenderPass(device.getLogicalDevice(), renderPass, nullptr);

            swapChain.destroyImageViews(device.getLogicalDevice());
            swapChain.destroy(device.getLogicalDevice());

            for (size_t i = 0; i < swapChain.getImages().size(); i++) {
                uniformBuffers[i].destroy(device.getLogicalDevice());
            }
            vkDestroyDescriptorPool(device.getLogicalDevice(), descriptorPool, nullptr);
        }
        virtual void cleanup() {
            cleanupSwapChain();
            vkDestroyDescriptorSetLayout(device.getLogicalDevice(), descriptorSetLayout, nullptr);
            vertexBuffer.destroy(device.getLogicalDevice());
            indexBuffer.destroy(device.getLogicalDevice());
            syncObjects.destroy(device, MAX_FRAMES_IN_FLIGHT);
            vkDestroyCommandPool(device.getLogicalDevice(), device.getCommandPool(), nullptr);
            device.destroy(instance.get());
            debugMessenger.destroy(instance.get());
            instance.destroy();
            window.destroy();
            glfwTerminate();
        }
        virtual void recreateSwapChain() {
            int width = 0, height = 0;
            glfwGetFramebufferSize(window.get(), &width, &height);
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(window.get(), &width, &height);
                glfwWaitEvents();
            }

            vkDeviceWaitIdle(device.getLogicalDevice());

            cleanupSwapChain();
            swapChain.create(device, window);
            swapChain.createImageViews(device.getLogicalDevice());
            createRenderPass();
            createGraphicsPipeline();
            createColorResources();
            createDepthResources();
            createFramebuffers();
            createUniformBuffers();
            createDescriptorPool();
            createDescriptorSets();
            createCommandBuffers();
            syncObjects.resize(swapChain);
        }
        virtual void createRenderPass() {
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = swapChain.getImageFormat();
            colorAttachment.samples = msaaSamples;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = LightVulkan::Utils::findDepthFormat(device.getPhysicalDevice());
            depthAttachment.samples = msaaSamples;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentDescription colorAttachmentResolve{};
            colorAttachmentResolve.format = swapChain.getImageFormat();
            colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            VkAttachmentReference colorAttachmentResolveRef{};
            colorAttachmentResolveRef.attachment = 2;
            colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpass.colorAttachmentCount = 1;
            subpass.pColorAttachments = &colorAttachmentRef;
            subpass.pDepthStencilAttachment = &depthAttachmentRef;
            subpass.pResolveAttachments = &colorAttachmentResolveRef;

            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            renderPassInfo.pAttachments = attachments.data();
            renderPassInfo.subpassCount = 1;
            renderPassInfo.pSubpasses = &subpass;
            renderPassInfo.dependencyCount = 1;
            renderPassInfo.pDependencies = &dependency;

            if (vkCreateRenderPass(device.getLogicalDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
                throw std::runtime_error("failed to create render pass!");
            }
        }
        virtual void createGraphicsPipeline() {
            VulkanShaderModule vertShaderModule(device, "shaders/helloTriangleVert.spv");
            VulkanShaderModule fragShaderModule(device, "shaders/helloTriangleFrag.spv");

            VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
            vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertShaderStageInfo.module = vertShaderModule.get();
            vertShaderStageInfo.pName = "main";

            VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
            fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = fragShaderModule.get();
            fragShaderStageInfo.pName = "main";

            VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

            VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
            vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

            auto bindingDescription = BasicVertex::getBindingDescription();
            auto attributeDescriptions = BasicVertex::getAttributeDescriptions();

            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
            vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
            vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

            VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
            inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            inputAssembly.primitiveRestartEnable = VK_FALSE;

            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)swapChain.getExtent().width;
            viewport.height = (float)swapChain.getExtent().height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;

            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = swapChain.getExtent();

            VkPipelineViewportStateCreateInfo viewportState{};
            viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            viewportState.viewportCount = 1;
            viewportState.pViewports = &viewport;
            viewportState.scissorCount = 1;
            viewportState.pScissors = &scissor;

            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE;
            rasterizer.rasterizerDiscardEnable = VK_FALSE;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
            rasterizer.lineWidth = 1.0f;
            rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
            rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
            rasterizer.depthBiasEnable = VK_FALSE;

            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_FALSE;
            multisampling.rasterizationSamples = msaaSamples;

            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.stencilTestEnable = VK_FALSE;

            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f;
            colorBlending.blendConstants[1] = 0.0f;
            colorBlending.blendConstants[2] = 0.0f;
            colorBlending.blendConstants[3] = 0.0f;

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1;
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

            if (vkCreatePipelineLayout(device.getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create pipeline layout!");
            }

            VkGraphicsPipelineCreateInfo pipelineInfo{};
            pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pInputAssemblyState = &inputAssembly;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.renderPass = renderPass;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

            if (vkCreateGraphicsPipelines(device.getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }

            vkDestroyShaderModule(device.getLogicalDevice(), fragShaderModule.get(), nullptr);
            vkDestroyShaderModule(device.getLogicalDevice(), vertShaderModule.get(), nullptr);
        }
        virtual void createFramebuffers() {
            swapChain.getFramebuffers().resize(swapChain.getImageViews().size());

            for (size_t i = 0; i < swapChain.getImageViews().size(); i++) {
                std::array<VkImageView, 3> attachments = {
                    colorResource.getImageView().get(),
                    depthResource.getImageView().get(),
                    swapChain.getImageViews()[i].get()
                };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = swapChain.getExtent().width;
                framebufferInfo.height = swapChain.getExtent().height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(device.getLogicalDevice(), &framebufferInfo, nullptr, &swapChain.getFramebuffers()[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create framebuffer!");
                }
            }
        }
        virtual void createCommandPool() {
            device.createCommandPool();
        }
        virtual void createColorResources() {
            colorResource.create(device,
                swapChain.getExtent().width, swapChain.getExtent().height,
                msaaSamples, swapChain.getImageFormat(), VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }
        virtual void createDepthResources() {
            depthResource.create(device,
                swapChain.getExtent().width, swapChain.getExtent().height,
                msaaSamples);
        }
        virtual void createCommandBuffers() {
            commandBuffers.resize(swapChain.getFramebuffers().size());

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = device.getCommandPool();
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

            if (vkAllocateCommandBuffers(device.getLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate command buffers!");
            }

            for (size_t i = 0; i < commandBuffers.size(); i++) {
                VkCommandBufferBeginInfo beginInfo{};
                beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

                if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
                    throw std::runtime_error("failed to begin recording command buffer!");
                }

                VkRenderPassBeginInfo renderPassInfo{};
                renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                renderPassInfo.renderPass = renderPass;
                renderPassInfo.framebuffer = swapChain.getFramebuffers()[i];
                renderPassInfo.renderArea.offset = { 0, 0 };
                renderPassInfo.renderArea.extent = swapChain.getExtent();

                std::array<VkClearValue, 2> clearValues{};
                clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
                clearValues[1].depthStencil = { 1.0f, 0 };

                renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
                renderPassInfo.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

                vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

                VkBuffer vertexBuffers[] = { vertexBuffer.getBuffer() };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

                vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, & descriptorSets[currentFrame], 0, nullptr);

                vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

                vkCmdEndRenderPass(commandBuffers[i]);

                if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to record command buffer!");
                }
            }
        }
        virtual void createUniformBuffers() {
            VkDeviceSize bufferSize = sizeof(UniformBufferObject);

            uniformBuffers.resize(swapChain.getImages().size());

            for (size_t i = 0; i < swapChain.getImages().size(); i++) {
                uniformBuffers[i].create(device,
                    bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            }
        }
        virtual void updateUniformBuffers(uint32_t currentImage) {
            static auto startTime = std::chrono::high_resolution_clock::now();

            auto currentTime = std::chrono::high_resolution_clock::now();
            float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

            UniformBufferObject ubo{};
            ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            ubo.proj = glm::perspective(glm::radians(45.0f), swapChain.getExtent().width / (float)swapChain.getExtent().height, 0.1f, 10.0f);
            ubo.proj[1][1] *= -1;

            void* data;
            vkMapMemory(device.getLogicalDevice(), uniformBuffers[currentImage].getMemory(), 0, sizeof(ubo), 0, &data);
            memcpy(data, &ubo, sizeof(ubo));
            vkUnmapMemory(device.getLogicalDevice(), uniformBuffers[currentImage].getMemory());
        }
        virtual void createVertexBuffer() {}
        virtual void createIndexBuffer() {}
        virtual void createDescriptorSetLayout() {
            VkDescriptorSetLayoutBinding uboLayoutBinding{};
            uboLayoutBinding.binding = 0;
            uboLayoutBinding.descriptorCount = 1;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.pImmutableSamplers = nullptr;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &uboLayoutBinding;

            if (vkCreateDescriptorSetLayout(device.getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor set layout!");
            }
        }
        virtual void createDescriptorPool() {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = 1;
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            if (vkCreateDescriptorPool(device.getLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
                throw std::runtime_error("failed to create descriptor pool!");
            }
        }
        virtual void createDescriptorSets() {
            std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPool;
            allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            allocInfo.pSetLayouts = layouts.data();

            descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
            if (vkAllocateDescriptorSets(device.getLogicalDevice(), &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
                throw std::runtime_error("failed to allocate descriptor sets!");
            }

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                VkDescriptorBufferInfo bufferInfo{};
                bufferInfo.buffer = uniformBuffers[i].getBuffer();
                bufferInfo.offset = 0;
                bufferInfo.range = sizeof(UniformBufferObject);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = descriptorSets[i];
                descriptorWrite.dstBinding = 0;
                descriptorWrite.dstArrayElement = 0;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pBufferInfo = &bufferInfo;

                vkUpdateDescriptorSets(device.getLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
            }
        }

        virtual void drawFrame() {
            vkWaitForFences(device.getLogicalDevice(), 1, &syncObjects.getInFlightFences()[currentFrame], VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(device.getLogicalDevice(), swapChain.get(), UINT64_MAX, syncObjects.getImageAvailableSemaphores()[currentFrame], VK_NULL_HANDLE, &imageIndex);

            if (result == VK_ERROR_OUT_OF_DATE_KHR) {
                recreateSwapChain();
                return;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
                throw std::runtime_error("failed to acquire swap chain image!");
            }

            updateUniformBuffers(imageIndex);

            if (syncObjects.getImagesInFlight()[imageIndex] != VK_NULL_HANDLE) {
                vkWaitForFences(device.getLogicalDevice(), 1, &syncObjects.getImagesInFlight()[imageIndex], VK_TRUE, UINT64_MAX);
            }
            syncObjects.getImagesInFlight()[imageIndex] = syncObjects.getInFlightFences()[currentFrame];

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            VkSemaphore waitSemaphores[] = { syncObjects.getImageAvailableSemaphores()[currentFrame] };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;

            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

            VkSemaphore signalSemaphores[] = { syncObjects.getRenderFinishedSemaphores()[currentFrame] };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            vkResetFences(device.getLogicalDevice(), 1, &syncObjects.getInFlightFences()[currentFrame]);

            if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, syncObjects.getInFlightFences()[currentFrame]) != VK_SUCCESS) {
                throw std::runtime_error("failed to submit draw command buffer!");
            }

            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = { swapChain.get() };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;

            presentInfo.pImageIndices = &imageIndex;

            result = vkQueuePresentKHR(device.getPresentQueue(), &presentInfo);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.isFrameBufferResized()) {
                window.setFrameBufferResized(false);
                recreateSwapChain();
            }
            else if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to present swap chain image!");
            }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }
    };
}
