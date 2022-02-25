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

namespace LightVulkan {

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

        std::vector<VkCommandBuffer> commandBuffers;

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
            createUniformBuffers();
            createDescriptorPool();
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
        }
        virtual void cleanup() {
            cleanupSwapChain();
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

        virtual void createUniformBuffers() {};
        virtual void updateUniformBuffers(uint32_t currentImage) {};
        virtual void cleanUniformBuffers() {

        }
        virtual void createDescriptorSetLayout() {};
        virtual void createDescriptorPool() {};
        virtual void createDescriptorSets() {};

        virtual void createGraphicsPipeline() = 0;
        virtual void createCommandBuffers() = 0;

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

            updateUniformBuffers(imageIndex);

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
