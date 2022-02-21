#pragma once

#include "VulkanImage.h"
#include "VulkanImageView.h"
#include "VulkanDevice.h"

namespace LightVulkan {
    class VulkanResource {
    public:
        void create(VulkanDevice* device, uint32_t width, uint32_t height,
            VkSampleCountFlagBits msaaSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usages,
            VkMemoryPropertyFlags memProperties, VkImageAspectFlags aspects, uint32_t mipLevels) {
            image.createImage(device,
                width, height, 1, msaaSamples, format,
                tiling, usages, memProperties, memory);

            imageView.create(device->getLogicalDevice(), image.get(), format, aspects, mipLevels);
        }
        void destroy(VkDevice device) {
            imageView.destroy(device);
            image.destroy(device);
            vkFreeMemory(device, memory, nullptr);
        }
        VulkanImage getImage() {
            return image;
        }
        VulkanImageView getImageView() {
            return imageView;
        }
        VkDeviceMemory getMemory() {
            return memory;
        }

    private:
        VulkanImage image;
        VulkanImageView imageView;
        VkDeviceMemory memory;
    };

    class VulkanDepthResource : public VulkanResource {
    public:
        void create(VulkanDevice* device, uint32_t width, uint32_t height, VkSampleCountFlagBits msaaSamples) {
            VulkanResource::create(device,
                width, height,
                msaaSamples, Utils::findDepthFormat(device->getPhysicalDevice()), VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_IMAGE_ASPECT_DEPTH_BIT, 1);
        }
    };
}
