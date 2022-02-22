#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "VulkanDevice.h"
#include "Utils.h"

namespace LightVulkan {
    class VulkanShaderModule {
    public:
        VulkanShaderModule(VulkanDevice& device, const char* filepath) {
            auto code = readFile(filepath);

            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            if (vkCreateShaderModule(device.getLogicalDevice(), &createInfo, nullptr, &module) != VK_SUCCESS) {
                throw std::runtime_error("failed to create shader module!");
            }
        }
        void destroy(VulkanDevice& device) {
            vkDestroyShaderModule(device.getLogicalDevice(), module, nullptr);
        }
        VkShaderModule get() {
            return module;
        }

private:
    VkShaderModule module;
};
}
