#pragma once

#include <android/native_window.h>
#include <vulkan/vulkan.h>

#include <vector>

namespace switchui::renderer {

struct VulkanContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue graphics_queue = VK_NULL_HANDLE;
    uint32_t graphics_queue_family = 0;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchain_images;
    VkFormat swapchain_format = VK_FORMAT_UNDEFINED;
    VkExtent2D extent{};
};

bool InitVulkanForAndroid(ANativeWindow* window, VulkanContext* out);
void DestroyVulkanContext(VulkanContext* ctx);

} // namespace switchui::renderer
