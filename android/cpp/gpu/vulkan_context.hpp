#pragma once

#include <vulkan/vulkan.h>
#include <android/native_window.h>
#include <vector>

namespace SwitchUI {

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();

    bool Initialize(ANativeWindow* window);
    void Shutdown();

private:
    bool CreateInstance();
    bool CreateDevice();
    bool CreateSwapChain(ANativeWindow* window);

    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;

    uint32_t graphicsQueueFamilyIndex = 0;
};

} // namespace SwitchUI
