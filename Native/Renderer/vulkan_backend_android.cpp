#include "vulkan_backend_android.h"

#include <android/log.h>

#include <array>

namespace switchui::renderer {
namespace {

constexpr const char* kTag = "SwitchUI-Vulkan";

#define VK_CHECK(expr)                                                                     \
    do {                                                                                    \
        const VkResult result = (expr);                                                    \
        if (result != VK_SUCCESS) {                                                        \
            __android_log_print(ANDROID_LOG_ERROR, kTag, "%s failed (%d)", #expr, result); \
            return false;                                                                   \
        }                                                                                   \
    } while (0)

// Metal -> Vulkan translation anchors used by emulator renderer integration:
// [Metal] MTLDevice                     -> [Vulkan] VkPhysicalDevice + VkDevice
// [Metal] CAMetalLayer drawable         -> [Vulkan] VkSurfaceKHR + VkSwapchainKHR image
// [Metal] MTLCommandQueue               -> [Vulkan] VkQueue
// [Metal] MTLCommandBuffer              -> [Vulkan] VkCommandBuffer
// [Metal] MTLRenderPassDescriptor       -> [Vulkan] VkRenderPass + VkFramebuffer
// [Metal] setVertexBytes/setFragment... -> [Vulkan] vkCmdPushConstants / UBO/SSBO

bool PickPhysicalDevice(VulkanContext* out) {
    uint32_t gpu_count = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(out->instance, &gpu_count, nullptr));
    if (gpu_count == 0) {
        return false;
    }

    std::vector<VkPhysicalDevice> gpus(gpu_count);
    VK_CHECK(vkEnumeratePhysicalDevices(out->instance, &gpu_count, gpus.data()));

    for (auto gpu : gpus) {
        uint32_t queue_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, nullptr);
        std::vector<VkQueueFamilyProperties> queue_props(queue_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_count, queue_props.data());

        for (uint32_t i = 0; i < queue_count; ++i) {
            VkBool32 present_supported = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, out->surface, &present_supported);
            if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && present_supported) {
                out->physical_device = gpu;
                out->graphics_queue_family = i;
                return true;
            }
        }
    }
    return false;
}

} // namespace

bool InitVulkanForAndroid(ANativeWindow* window, VulkanContext* out) {
    if (!window || !out) {
        return false;
    }

    std::array<const char*, 2> instance_extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
    };

    VkApplicationInfo app_info{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app_info.pApplicationName = "SwitchUI";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "SwitchUIRenderer";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
    instance_info.ppEnabledExtensionNames = instance_extensions.data();
    VK_CHECK(vkCreateInstance(&instance_info, nullptr, &out->instance));

    VkAndroidSurfaceCreateInfoKHR surface_info{VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR};
    surface_info.window = window;
    VK_CHECK(vkCreateAndroidSurfaceKHR(out->instance, &surface_info, nullptr, &out->surface));

    if (!PickPhysicalDevice(out)) {
        DestroyVulkanContext(out);
        return false;
    }

    float prio = 1.0f;
    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = out->graphics_queue_family;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &prio;

    const char* device_exts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    device_info.enabledExtensionCount = 1;
    device_info.ppEnabledExtensionNames = device_exts;
    VK_CHECK(vkCreateDevice(out->physical_device, &device_info, nullptr, &out->device));

    vkGetDeviceQueue(out->device, out->graphics_queue_family, 0, &out->graphics_queue);

    VkSurfaceCapabilitiesKHR caps{};
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(out->physical_device, out->surface, &caps));

    uint32_t format_count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(out->physical_device, out->surface, &format_count, nullptr));
    if (format_count == 0) {
        DestroyVulkanContext(out);
        return false;
    }

    std::vector<VkSurfaceFormatKHR> formats(format_count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(out->physical_device, out->surface, &format_count, formats.data()));

    VkSurfaceFormatKHR chosen = formats.front();
    for (const auto& fmt : formats) {
        if (fmt.format == VK_FORMAT_R8G8B8A8_UNORM || fmt.format == VK_FORMAT_B8G8R8A8_UNORM) {
            chosen = fmt;
            break;
        }
    }

    out->swapchain_format = chosen.format;
    out->extent = caps.currentExtent;

    uint32_t min_image_count = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && min_image_count > caps.maxImageCount) {
        min_image_count = caps.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    swapchain_info.surface = out->surface;
    swapchain_info.minImageCount = min_image_count;
    swapchain_info.imageFormat = out->swapchain_format;
    swapchain_info.imageColorSpace = chosen.colorSpace;
    swapchain_info.imageExtent = out->extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = caps.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_info.clipped = VK_TRUE;
    VK_CHECK(vkCreateSwapchainKHR(out->device, &swapchain_info, nullptr, &out->swapchain));

    uint32_t swap_count = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(out->device, out->swapchain, &swap_count, nullptr));
    out->swapchain_images.resize(swap_count);
    VK_CHECK(vkGetSwapchainImagesKHR(out->device, out->swapchain, &swap_count, out->swapchain_images.data()));

    return true;
}

void DestroyVulkanContext(VulkanContext* ctx) {
    if (!ctx) {
        return;
    }

    if (ctx->device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx->device);
    }

    if (ctx->swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ctx->device, ctx->swapchain, nullptr);
    }
    if (ctx->device != VK_NULL_HANDLE) {
        vkDestroyDevice(ctx->device, nullptr);
    }
    if (ctx->surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(ctx->instance, ctx->surface, nullptr);
    }
    if (ctx->instance != VK_NULL_HANDLE) {
        vkDestroyInstance(ctx->instance, nullptr);
    }

    *ctx = {};
}

} // namespace switchui::renderer
