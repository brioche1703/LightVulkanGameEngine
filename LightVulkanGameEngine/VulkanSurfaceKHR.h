#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

#include "Window.h"
#include "VulkanInstance.h"

namespace LightVulkan {
	class VulkanSurfaceKHR {
	public:
		void setUp(VulkanInstance& instance, Window& window) {
			if (glfwCreateWindowSurface(instance.get(), window.get(), nullptr, &surface) != VK_SUCCESS) {
				throw std::runtime_error("failed to create window surface!");
			}
		}
		VkSurfaceKHR get() {
			return surface;
		}

	private:
		VkSurfaceKHR surface;
	};
}
