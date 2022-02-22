#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <stdexcept>

#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

namespace LightVulkan {
    class VulkanSyncObjects {
    public:
        void create(VulkanDevice& device, VulkanSwapChain& swapChain, int maxFramesInFlight) {
            imageAvailableSemaphores.resize(maxFramesInFlight);
            renderFinishedSemaphores.resize(maxFramesInFlight);
            inFlightFences.resize(maxFramesInFlight);
            imagesInFlight.resize(swapChain.getImages().size(), VK_NULL_HANDLE);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < maxFramesInFlight; i++) {
                if (vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(device.getLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                    vkCreateFence(device.getLogicalDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                    throw std::runtime_error("failed to create synchronization objects for a frame!");
                }
            }
        }
        void destroy(VulkanDevice& device, int maxFramesInFlight) {
            for (size_t i = 0; i < maxFramesInFlight; i++) {
                vkDestroySemaphore(device.getLogicalDevice(), renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(device.getLogicalDevice(), imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(device.getLogicalDevice(), inFlightFences[i], nullptr);
            }
        }
        void resize(VulkanSwapChain& swapChain) {
            imagesInFlight.resize(swapChain.getImages().size(), VK_NULL_HANDLE);
        }
        std::vector<VkSemaphore>& getImageAvailableSemaphores() {
            return imageAvailableSemaphores;
        }
        std::vector<VkSemaphore>& getRenderFinishedSemaphores() {
            return renderFinishedSemaphores;
        }
        std::vector<VkFence>& getInFlightFences() {
            return inFlightFences;
        }
        std::vector<VkFence>& getImagesInFlight() {
            return imagesInFlight;
        }

    private:
        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
    };
}
