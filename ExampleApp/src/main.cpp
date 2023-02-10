#include <retgui.hpp>

#include "retgui_impl_glfw.hpp"
#include "retgui_impl_vulkan.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VULKAN_HPP_NO_NODISCARD_WARNINGS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vma/vk_mem_alloc.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

static vk::DynamicLoader g_loader;
static vk::Instance g_instance;
static vk::DebugUtilsMessengerEXT g_debugCallback;
static vk::PhysicalDevice g_physicalDevice;
static vk::Device g_device;
static VmaAllocator g_allocator;
static uint32_t g_queueFamily = static_cast<uint32_t>(-1);
static vk::Queue g_queue;
static vk::DescriptorPool g_descriptorPool;
static vk::CommandPool g_cmdPool;

struct RenderFrame
{
    vk::CommandBuffer cmd;
    vk::Semaphore imageReadySemaphore;
    vk::Semaphore renderDoneSemaphore;
    vk::Fence cmdFence;
};
static std::array<RenderFrame, 2> g_frames;
static uint32_t g_frameIndex = 0;

static vk::SurfaceKHR g_surface;
static uint32_t g_minImageCount = 2;
static vk::SwapchainKHR g_swapchain;
static std::vector<vk::Image> g_swapchainImages;
static std::vector<vk::ImageView> g_swapchainViews;
static uint32_t g_imageIndex;
static bool g_rebuildSwapchain = false;

static std::array<float, 4> clear_color = { 0.45f, 0.55f, 0.60f, 1.00f };

static void check_vk_result(vk::Result result)
{
    if (result == vk::Result::eSuccess)
    {
        return;
    }
    fprintf(stderr, "[Vulkan] Error: vk::Result = %s\n", vk::to_string(result).c_str());
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                     VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* pUserData)
{
    (void)messageSeverity;
    (void)messageTypes;
    (void)pUserData;
    fprintf(stderr,
            "[vulkan] Debug callback from ObjectType: %i\nMessage: %s\n\n",
            pCallbackData->pObjects[0].objectType,
            pCallbackData->pMessage);
    return VK_FALSE;
}

static void setup_vulkan(std::vector<const char*> instance_extensions)
{
    // Create Vulkan instance
    {
        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = g_loader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
        VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

        vk::ApplicationInfo appInfo{};
        appInfo.apiVersion = VK_API_VERSION_1_3;

        vk::InstanceCreateInfo create_info{};
        create_info.setPApplicationInfo(&appInfo);
#if _DEBUG
        std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
        create_info.setPEnabledLayerNames(layers);

        // Enable debug_utils extension
        instance_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        vk::DebugUtilsMessengerCreateInfoEXT debug_utils_create_info{};
        debug_utils_create_info.setPfnUserCallback(&debug_callback);
        debug_utils_create_info.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eError |
                                                   vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning);
        debug_utils_create_info.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                               vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);

        create_info.setPNext(&debug_utils_create_info);
#endif
        create_info.setPEnabledExtensionNames(instance_extensions);

        g_instance = vk::createInstance(create_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(g_instance);

#if _DEBUG
        g_debugCallback = g_instance.createDebugUtilsMessengerEXT(debug_utils_create_info);
#endif
    }

    // Select GPU
    {
        auto gpus = g_instance.enumeratePhysicalDevices();

        size_t gpu_index = 0;
        for (size_t i = 0; i < gpus.size(); ++i)
        {
            auto gpu_props = gpus[i].getProperties();
            if (gpu_props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
            {
                gpu_index = i;
                break;
            }
        }

        g_physicalDevice = gpus[gpu_index];
    }

    // Select graphics queue family
    {
        auto queues = g_physicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < queues.size(); ++i)
        {
            if (queues[i].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                g_queueFamily = i;
                break;
            }
        }
        assert(g_queueFamily != static_cast<uint32_t>(-1));
    }

    // Create logical device
    {
        const float queue_priority = 1.0f;
        vk::DeviceQueueCreateInfo queue_info{};
        queue_info.setQueueFamilyIndex(g_queueFamily);
        queue_info.setQueueCount(1);
        queue_info.setQueuePriorities(queue_priority);

        std::vector<const char*> device_extensions = { "VK_KHR_swapchain", VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME };

        vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
        dynamic_rendering_features.setDynamicRendering(true);

        vk::DeviceCreateInfo create_info{};
        create_info.setQueueCreateInfos(queue_info);
        create_info.setPEnabledExtensionNames(device_extensions);
        create_info.setPNext(&dynamic_rendering_features);
        g_device = g_physicalDevice.createDevice(create_info);
        VULKAN_HPP_DEFAULT_DISPATCHER.init(g_device);

        g_queue = g_device.getQueue(g_queueFamily, 0);
    }

    // Create descriptor pool
    {
        std::vector<vk::DescriptorPoolSize> pool_sizes = {
            { vk::DescriptorType::eCombinedImageSampler, 1000 },
        };

        vk::DescriptorPoolCreateInfo create_info{};
        create_info.setMaxSets(1000);
        create_info.setPoolSizes(pool_sizes);
        g_descriptorPool = g_device.createDescriptorPool(create_info);
    }

    // Create command pool
    {
        vk::CommandPoolCreateInfo create_info{};
        create_info.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        create_info.setQueueFamilyIndex(g_queueFamily);
        g_cmdPool = g_device.createCommandPool(create_info);
    }

    // Create frames
    {
        for (auto& frame : g_frames)
        {
            vk::CommandBufferAllocateInfo alloc_info{};
            alloc_info.setCommandPool(g_cmdPool);
            alloc_info.setCommandBufferCount(1);
            alloc_info.setLevel(vk::CommandBufferLevel::ePrimary);
            frame.cmd = g_device.allocateCommandBuffers(alloc_info)[0];

            frame.imageReadySemaphore = g_device.createSemaphore({});
            frame.renderDoneSemaphore = g_device.createSemaphore({});
            frame.cmdFence = g_device.createFence({ vk::FenceCreateFlagBits::eSignaled });
        }
    }
}

static void setup_vulkan_window(uint32_t width, uint32_t height)
{
    auto old_swapchain = g_swapchain;

    vk::SwapchainCreateInfoKHR create_info{};
    create_info.setSurface(g_surface);
    create_info.setMinImageCount(2);
    create_info.setImageFormat(vk::Format::eB8G8R8A8Srgb);
    create_info.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear);
    create_info.setImageArrayLayers(1);
    create_info.setImageExtent({ width, height });
    create_info.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);
    create_info.setPresentMode(vk::PresentModeKHR::eFifo);
    create_info.setOldSwapchain(old_swapchain);

    g_swapchain = g_device.createSwapchainKHR(create_info);

    g_device.destroy(old_swapchain);

    g_swapchainImages = g_device.getSwapchainImagesKHR(g_swapchain);
    for (auto& image : g_swapchainImages)
    {
        vk::ImageViewCreateInfo view_info{};
        view_info.setImage(image);
        view_info.setFormat(vk::Format::eB8G8R8A8Srgb);
        view_info.setViewType(vk::ImageViewType::e2D);
        view_info.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        g_swapchainViews.push_back(g_device.createImageView(view_info));
    }
}

static void cleanup_vulkan()
{
    for (auto& frame : g_frames)
    {
        g_device.destroy(frame.imageReadySemaphore);
        g_device.destroy(frame.renderDoneSemaphore);
        g_device.destroy(frame.cmdFence);
    }

    g_device.destroy(g_cmdPool);
    g_device.destroy(g_descriptorPool);

    g_device.destroy();
    g_instance.destroy(g_surface);
    g_instance.destroy(g_debugCallback);
    g_instance.destroy();
}

static void cleanup_vulkan_window()
{
    for (auto& view : g_swapchainViews)
    {
        g_device.destroy(view);
    }

    g_device.destroy(g_swapchain);
}

static void render_frame(/* RetDrawData* draw_data */)
{
    g_frameIndex = (g_frameIndex + 1) % g_frames.size();
    auto& frame = g_frames[g_frameIndex];

    auto result = g_device.acquireNextImageKHR(g_swapchain, UINT64_MAX, frame.imageReadySemaphore);
    if (result.result == vk::Result::eErrorOutOfDateKHR || result.result == vk::Result::eSuboptimalKHR)
    {
        g_rebuildSwapchain = true;
    }
    g_imageIndex = result.value;

    g_device.waitForFences(frame.cmdFence, true, UINT64_MAX);
    g_device.resetFences(frame.cmdFence);

    vk::CommandBufferBeginInfo begin_info{};
    frame.cmd.begin(begin_info);

    // Transition: ColorAttachment
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.setSrcAccessMask({});
        barrier.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        barrier.setOldLayout(vk::ImageLayout::eUndefined);
        barrier.setNewLayout(vk::ImageLayout::eColorAttachmentOptimal);
        barrier.setImage(g_swapchainImages[g_imageIndex]);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        frame.cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eColorAttachmentOutput, {}, {}, {}, barrier);
    }

    vk::RenderingAttachmentInfo attachment_info{};
    attachment_info.setImageView(g_swapchainViews[g_imageIndex]);
    attachment_info.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal);
    attachment_info.setLoadOp(vk::AttachmentLoadOp::eClear);
    attachment_info.setStoreOp(vk::AttachmentStoreOp::eStore);
    attachment_info.setClearValue(vk::ClearColorValue(clear_color));

    vk::RenderingInfo pass_info{};
    pass_info.setColorAttachments(attachment_info);
    pass_info.setRenderArea(vk::Rect2D({ 0, 0 }, { 1600, 900 }));
    pass_info.setLayerCount(1);
    frame.cmd.beginRendering(pass_info);

    frame.cmd.endRendering();

    // Transition: PresentSrc
    {
        vk::ImageMemoryBarrier barrier{};
        barrier.setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);
        barrier.setDstAccessMask({});
        barrier.setOldLayout(vk::ImageLayout::eColorAttachmentOptimal);
        barrier.setNewLayout(vk::ImageLayout::ePresentSrcKHR);
        barrier.setImage(g_swapchainImages[g_imageIndex]);
        barrier.setSubresourceRange(vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        frame.cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {}, barrier);
    }

    frame.cmd.end();

    vk::PipelineStageFlags wait_stages = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submit_info{};
    submit_info.setWaitDstStageMask(wait_stages);
    submit_info.setWaitSemaphores(frame.imageReadySemaphore);
    submit_info.setSignalSemaphores(frame.renderDoneSemaphore);
    submit_info.setCommandBuffers(frame.cmd);
    g_queue.submit(submit_info, frame.cmdFence);
}

static void present_frame()
{
    auto& frame = g_frames[g_frameIndex];

    vk::PresentInfoKHR present_info{};
    present_info.setWaitSemaphores(frame.renderDoneSemaphore);
    present_info.setSwapchains(g_swapchain);
    present_info.setImageIndices(g_imageIndex);
    auto result = g_queue.presentKHR(present_info);
    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR)
    {
        g_rebuildSwapchain = true;
    }
}

int main(int /*argc*/, char** /*argv*/)
{
    if (!glfwInit())
    {
        return 1;
    }

    // Create window with Vulkan context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1600, 900, "RetGui GLFW+Vulkan Example", nullptr, nullptr);
    if (!glfwVulkanSupported())
    {
        printf("GLFW: Vulkan not supported!");
        return 1;
    }

    uint32_t extensions_count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
    setup_vulkan({ extensions, extensions + extensions_count });

    // Create window surface
    VkSurfaceKHR surface;
    auto result = vk::Result(glfwCreateWindowSurface(g_instance, window, nullptr, &surface));
    check_vk_result(result);
    g_surface = surface;

    // Create swapchain
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    setup_vulkan_window(w, h);

    // #TODO: Setup RetGui context

    // #TODO: Setup Platform/Renderer backends

    // #TODO: Load fonts
    // #TODO: Upload fonts

    // RetVec4 clear_color = RetVec4(0.45f, 0.55f, 0.6f, 1.0f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll & Handle events (inputs, window resize, etc.)
        glfwPollEvents();

        // #TODO: Render RetGui - retgui::render();
        // #TODO: Get RetGui draw data - retgui::get_draw_data();
        // #TODO: Draw retgui draw data

        render_frame();
        present_frame();
    }

    // Cleanup
    g_device.waitIdle();

    // #TODO: Shutdown RetGui context - retgui::destroy_context();

    cleanup_vulkan_window();
    cleanup_vulkan();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}