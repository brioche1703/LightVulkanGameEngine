#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace LightVulkan {
    class Window {
    public:
        void setUp(const uint32_t width, const uint32_t height, const std::string title) {
            glfwInit();

            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

            window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
            glfwSetWindowUserPointer(window, this);
            glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        }
        void destroy() {
            glfwDestroyWindow(window);
        }
        GLFWwindow* get() {
            return window;
        }
        void setFrameBufferResized(bool value) {
            framebufferResized = value;
        }
        bool isFrameBufferResized() const {
            return framebufferResized;
        }

    private:
        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
            framebufferResized = true;
        }

    private:
        static bool framebufferResized;
        GLFWwindow* window;
    };
}
