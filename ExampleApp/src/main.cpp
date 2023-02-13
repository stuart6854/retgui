#include <retgui.hpp>

#include "retgui_impl_glfw.hpp"
#include "retgui_impl_vulkan.hpp"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define VULKAN_HPP_NO_NODISCARD_WARNINGS
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#define VMA_IMPLEMENTATION
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

static vk::PipelineLayout g_pipelineLayout;
static vk::Pipeline g_pipeline;

static vk::Buffer g_vertexBuffer;
static VmaAllocation g_vertexBufferAlloc;
static vk::Buffer g_indexBuffer;
static VmaAllocation g_indexBufferAlloc;

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

    // Create allocation
    {
        VmaAllocatorCreateInfo create_info{};
        create_info.instance = g_instance;
        create_info.physicalDevice = g_physicalDevice;
        create_info.device = g_device;
        create_info.vulkanApiVersion = VK_API_VERSION_1_3;
        vmaCreateAllocator(&create_info, &g_allocator);
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

static std::vector<uint32_t> __vk_shader_vert_spv = {
    0x07230203, 0x00010000, 0x0008000b, 0x0000002e, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e,
    0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x000b000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000000d,
    0x00000012, 0x00000027, 0x00000028, 0x0000002a, 0x0000002c, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
    0x00000000, 0x00060005, 0x0000000b, 0x505f6c67, 0x65567265, 0x78657472, 0x00000000, 0x00060006, 0x0000000b, 0x00000000, 0x505f6c67,
    0x7469736f, 0x006e6f69, 0x00070006, 0x0000000b, 0x00000001, 0x505f6c67, 0x746e696f, 0x657a6953, 0x00000000, 0x00070006, 0x0000000b,
    0x00000002, 0x435f6c67, 0x4470696c, 0x61747369, 0x0065636e, 0x00070006, 0x0000000b, 0x00000003, 0x435f6c67, 0x446c6c75, 0x61747369,
    0x0065636e, 0x00030005, 0x0000000d, 0x00000000, 0x00040005, 0x00000012, 0x6f705f61, 0x00000073, 0x00060005, 0x00000014, 0x68737550,
    0x736e6f43, 0x746e6174, 0x00000073, 0x00050006, 0x00000014, 0x00000000, 0x6c616373, 0x00000065, 0x00060006, 0x00000014, 0x00000001,
    0x6e617274, 0x74616c73, 0x00000065, 0x00050005, 0x00000016, 0x68737570, 0x6e6f635f, 0x00737473, 0x00040005, 0x00000027, 0x5f74756f,
    0x00007675, 0x00040005, 0x00000028, 0x76755f61, 0x00000000, 0x00050005, 0x0000002a, 0x5f74756f, 0x6f6c6f63, 0x00000072, 0x00040005,
    0x0000002c, 0x6f635f61, 0x00726f6c, 0x00050048, 0x0000000b, 0x00000000, 0x0000000b, 0x00000000, 0x00050048, 0x0000000b, 0x00000001,
    0x0000000b, 0x00000001, 0x00050048, 0x0000000b, 0x00000002, 0x0000000b, 0x00000003, 0x00050048, 0x0000000b, 0x00000003, 0x0000000b,
    0x00000004, 0x00030047, 0x0000000b, 0x00000002, 0x00040047, 0x00000012, 0x0000001e, 0x00000000, 0x00050048, 0x00000014, 0x00000000,
    0x00000023, 0x00000000, 0x00050048, 0x00000014, 0x00000001, 0x00000023, 0x00000008, 0x00030047, 0x00000014, 0x00000002, 0x00040047,
    0x00000027, 0x0000001e, 0x00000000, 0x00040047, 0x00000028, 0x0000001e, 0x00000001, 0x00040047, 0x0000002a, 0x0000001e, 0x00000001,
    0x00040047, 0x0000002c, 0x0000001e, 0x00000002, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006,
    0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040015, 0x00000008, 0x00000020, 0x00000000, 0x0004002b, 0x00000008,
    0x00000009, 0x00000001, 0x0004001c, 0x0000000a, 0x00000006, 0x00000009, 0x0006001e, 0x0000000b, 0x00000007, 0x00000006, 0x0000000a,
    0x0000000a, 0x00040020, 0x0000000c, 0x00000003, 0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000003, 0x00040015, 0x0000000e,
    0x00000020, 0x00000001, 0x0004002b, 0x0000000e, 0x0000000f, 0x00000000, 0x00040017, 0x00000010, 0x00000006, 0x00000002, 0x00040020,
    0x00000011, 0x00000001, 0x00000010, 0x0004003b, 0x00000011, 0x00000012, 0x00000001, 0x0004001e, 0x00000014, 0x00000010, 0x00000010,
    0x00040020, 0x00000015, 0x00000009, 0x00000014, 0x0004003b, 0x00000015, 0x00000016, 0x00000009, 0x00040020, 0x00000017, 0x00000009,
    0x00000010, 0x0004002b, 0x0000000e, 0x0000001b, 0x00000001, 0x0004002b, 0x00000006, 0x0000001f, 0x00000000, 0x0004002b, 0x00000006,
    0x00000020, 0x3f800000, 0x00040020, 0x00000024, 0x00000003, 0x00000007, 0x00040020, 0x00000026, 0x00000003, 0x00000010, 0x0004003b,
    0x00000026, 0x00000027, 0x00000003, 0x0004003b, 0x00000011, 0x00000028, 0x00000001, 0x0004003b, 0x00000024, 0x0000002a, 0x00000003,
    0x00040020, 0x0000002b, 0x00000001, 0x00000007, 0x0004003b, 0x0000002b, 0x0000002c, 0x00000001, 0x00050036, 0x00000002, 0x00000004,
    0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x00000010, 0x00000013, 0x00000012, 0x00050041, 0x00000017, 0x00000018,
    0x00000016, 0x0000000f, 0x0004003d, 0x00000010, 0x00000019, 0x00000018, 0x00050085, 0x00000010, 0x0000001a, 0x00000013, 0x00000019,
    0x00050041, 0x00000017, 0x0000001c, 0x00000016, 0x0000001b, 0x0004003d, 0x00000010, 0x0000001d, 0x0000001c, 0x00050081, 0x00000010,
    0x0000001e, 0x0000001a, 0x0000001d, 0x00050051, 0x00000006, 0x00000021, 0x0000001e, 0x00000000, 0x00050051, 0x00000006, 0x00000022,
    0x0000001e, 0x00000001, 0x00070050, 0x00000007, 0x00000023, 0x00000021, 0x00000022, 0x0000001f, 0x00000020, 0x00050041, 0x00000024,
    0x00000025, 0x0000000d, 0x0000000f, 0x0003003e, 0x00000025, 0x00000023, 0x0004003d, 0x00000010, 0x00000029, 0x00000028, 0x0003003e,
    0x00000027, 0x00000029, 0x0004003d, 0x00000007, 0x0000002d, 0x0000002c, 0x0003003e, 0x0000002a, 0x0000002d, 0x000100fd, 0x00010038
};

static std::vector<uint32_t> __vk_shader_frag_spv = {
    0x07230203, 0x00010000, 0x0008000b, 0x00000010, 0x00000000, 0x00020011, 0x00000001, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e,
    0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0008000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009,
    0x0000000b, 0x0000000f, 0x00030010, 0x00000004, 0x00000007, 0x00030003, 0x00000002, 0x000001c2, 0x00040005, 0x00000004, 0x6e69616d,
    0x00000000, 0x00050005, 0x00000009, 0x67617266, 0x6c6f635f, 0x0000726f, 0x00050005, 0x0000000b, 0x635f6e69, 0x726f6c6f, 0x00000000,
    0x00040005, 0x0000000f, 0x755f6e69, 0x00000076, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047, 0x0000000b, 0x0000001e,
    0x00000001, 0x00040047, 0x0000000f, 0x0000001e, 0x00000000, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016,
    0x00000006, 0x00000020, 0x00040017, 0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b,
    0x00000008, 0x00000009, 0x00000003, 0x00040020, 0x0000000a, 0x00000001, 0x00000007, 0x0004003b, 0x0000000a, 0x0000000b, 0x00000001,
    0x00040017, 0x0000000d, 0x00000006, 0x00000002, 0x00040020, 0x0000000e, 0x00000001, 0x0000000d, 0x0004003b, 0x0000000e, 0x0000000f,
    0x00000001, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x00000007, 0x0000000c,
    0x0000000b, 0x0003003e, 0x00000009, 0x0000000c, 0x000100fd, 0x00010038
};

static void setup_vulkan_pipeline(vk::Format attachment_format)
{
    vk::PushConstantRange push_range{};
    push_range.setStageFlags(vk::ShaderStageFlagBits::eVertex);
    push_range.setOffset(0);
    push_range.setSize(sizeof(RetVec2) * 2);

    vk::PipelineLayoutCreateInfo layout_info{};
    layout_info.setPushConstantRanges(push_range);
    g_pipelineLayout = g_device.createPipelineLayout(layout_info);

    vk::ShaderModuleCreateInfo vertex_module_info{};
    vertex_module_info.setCode(__vk_shader_vert_spv);
    auto vertex_module = g_device.createShaderModule(vertex_module_info);

    vk::ShaderModuleCreateInfo pixel_module_info{};
    pixel_module_info.setCode(__vk_shader_frag_spv);
    auto pixel_module = g_device.createShaderModule(pixel_module_info);

    std::array<vk::PipelineShaderStageCreateInfo, 2> stages{};
    stages[0].setStage(vk::ShaderStageFlagBits::eVertex);
    stages[0].setModule(vertex_module);
    stages[0].setPName("main");
    stages[1].setStage(vk::ShaderStageFlagBits::eFragment);
    stages[1].setModule(pixel_module);
    stages[1].setPName("main");

    vk::VertexInputBindingDescription binding_desc{};
    binding_desc.setStride(sizeof(RetDrawVert));
    binding_desc.setInputRate(vk::VertexInputRate::eVertex);

    std::array<vk::VertexInputAttributeDescription, 3> attribute_descs{};
    attribute_descs[0].setBinding(0);
    attribute_descs[0].setLocation(0);
    attribute_descs[0].setFormat(vk::Format::eR32G32Sfloat);
    attribute_descs[0].setOffset(offsetof(RetDrawVert, position));
    attribute_descs[1].setBinding(0);
    attribute_descs[1].setLocation(1);
    attribute_descs[1].setFormat(vk::Format::eR32G32Sfloat);
    attribute_descs[1].setOffset(offsetof(RetDrawVert, texCoord));
    attribute_descs[2].setBinding(0);
    attribute_descs[2].setLocation(2);
    attribute_descs[2].setFormat(vk::Format::eR32G32B32A32Sfloat);
    attribute_descs[2].setOffset(offsetof(RetDrawVert, color));

    vk::PipelineVertexInputStateCreateInfo vertex_info{};
    vertex_info.setVertexBindingDescriptions(binding_desc);
    vertex_info.setVertexAttributeDescriptions(attribute_descs);

    vk::PipelineInputAssemblyStateCreateInfo input_assembly_info{};
    input_assembly_info.setTopology(vk::PrimitiveTopology::eTriangleList);

    vk::PipelineViewportStateCreateInfo viewport_info{};
    viewport_info.setViewportCount(1);
    viewport_info.setScissorCount(1);

    vk::PipelineRasterizationStateCreateInfo rasterization_info{};
    rasterization_info.setPolygonMode(vk::PolygonMode::eFill);
    rasterization_info.setCullMode(vk::CullModeFlagBits::eNone);  // #TODO: Cull back face?
    rasterization_info.setFrontFace(vk::FrontFace::eCounterClockwise);
    rasterization_info.setLineWidth(1.0f);

    vk::PipelineMultisampleStateCreateInfo multisample_info{};
    multisample_info.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    vk::PipelineColorBlendAttachmentState color_attachment_state{};
    color_attachment_state.setBlendEnable(true);
    color_attachment_state.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha);
    color_attachment_state.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
    color_attachment_state.setColorBlendOp(vk::BlendOp::eAdd);
    color_attachment_state.setSrcAlphaBlendFactor(vk::BlendFactor::eOne);
    color_attachment_state.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha);
    color_attachment_state.setAlphaBlendOp(vk::BlendOp::eAdd);
    color_attachment_state.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                             vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);

    vk::PipelineColorBlendStateCreateInfo color_blend_info{};
    color_blend_info.setAttachments(color_attachment_state);

    vk::PipelineDepthStencilStateCreateInfo depth_info{};

    std::array<vk::DynamicState, 2> dynamic_states{};
    dynamic_states[0] = vk::DynamicState::eViewport;
    dynamic_states[1] = vk::DynamicState::eScissor;
    vk::PipelineDynamicStateCreateInfo dynamic_state_info{};
    dynamic_state_info.setDynamicStates(dynamic_states);

    vk::PipelineRenderingCreateInfo rendering_info{};
    rendering_info.setColorAttachmentFormats(attachment_format);

    vk::GraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.setLayout(g_pipelineLayout);
    pipeline_info.setStages(stages);
    pipeline_info.setPVertexInputState(&vertex_info);
    pipeline_info.setPInputAssemblyState(&input_assembly_info);
    pipeline_info.setPViewportState(&viewport_info);
    pipeline_info.setPRasterizationState(&rasterization_info);
    pipeline_info.setPMultisampleState(&multisample_info);
    pipeline_info.setPColorBlendState(&color_blend_info);
    pipeline_info.setPDepthStencilState(&depth_info);
    pipeline_info.setPDynamicState(&dynamic_state_info);
    pipeline_info.setPNext(&rendering_info);

    g_pipeline = g_device.createGraphicsPipeline({}, pipeline_info).value;

    g_device.destroy(vertex_module);
    g_device.destroy(pixel_module);
}

static void create_buffers(const RetDrawList& draw_list)
{
    {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.setSize(sizeof(RetDrawVert) * draw_list.vertices.size());
        bufferInfo.setUsage(vk::BufferUsageFlagBits::eVertexBuffer);

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkBufferCreateInfo vkBufferInfo = bufferInfo;
        VkBuffer vkBuffer;
        vmaCreateBuffer(g_allocator, &vkBufferInfo, &allocInfo, &vkBuffer, &g_vertexBufferAlloc, nullptr);
        g_vertexBuffer = vkBuffer;
    }
    {
        vk::BufferCreateInfo bufferInfo{};
        bufferInfo.setSize(sizeof(RetDrawIdx) * draw_list.indices.size());
        bufferInfo.setUsage(vk::BufferUsageFlagBits::eIndexBuffer);

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        VkBufferCreateInfo vkBufferInfo = bufferInfo;
        VkBuffer vkBuffer;
        vmaCreateBuffer(g_allocator, &vkBufferInfo, &allocInfo, &vkBuffer, &g_indexBufferAlloc, nullptr);
        g_indexBuffer = vkBuffer;
    }
}

static void update_buffers(const RetDrawList& draw_list)
{
    {
        void* mapped = nullptr;
        vmaMapMemory(g_allocator, g_vertexBufferAlloc, &mapped);
        std::memcpy(mapped, draw_list.vertices.data(), sizeof(RetDrawVert) * draw_list.vertices.size());
        vmaUnmapMemory(g_allocator, g_vertexBufferAlloc);
    }
    {
        void* mapped = nullptr;
        vmaMapMemory(g_allocator, g_indexBufferAlloc, &mapped);
        std::memcpy(mapped, draw_list.indices.data(), sizeof(RetDrawIdx) * draw_list.indices.size());
        vmaUnmapMemory(g_allocator, g_indexBufferAlloc);
    }
}

static void cleanup_buffers()
{
    vmaDestroyBuffer(g_allocator, g_vertexBuffer, g_vertexBufferAlloc);
    vmaDestroyBuffer(g_allocator, g_indexBuffer, g_indexBufferAlloc);
}

static void cleanup_vulkan()
{
    cleanup_buffers();

    g_device.destroy(g_pipeline);
    g_device.destroy(g_pipelineLayout);

    for (auto& frame : g_frames)
    {
        g_device.destroy(frame.imageReadySemaphore);
        g_device.destroy(frame.renderDoneSemaphore);
        g_device.destroy(frame.cmdFence);
    }

    g_device.destroy(g_cmdPool);
    g_device.destroy(g_descriptorPool);

    vmaDestroyAllocator(g_allocator);

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

static void render_retgui(RetDrawData* draw_data)
{
    auto& frame = g_frames[g_frameIndex];
    auto& cmd = frame.cmd;

    vk::Viewport viewport(0, 0, 1600, 900, 0.0f, 1.0f);
    cmd.setViewport(0, viewport);

    vk::Rect2D scissor({ 0, 0 }, { 1600, 900 });
    cmd.setScissor(0, scissor);

    cmd.bindVertexBuffers(0, g_vertexBuffer, { 0 });
    cmd.bindIndexBuffer(g_indexBuffer, 0, vk::IndexType::eUint16);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, g_pipeline);

    RetVec2 scale{
        2.0f / 1600,
        2.0f / 900,
    };
    RetVec2 translate{
        -1.0f,  // * scale.x,
        -1.0f,  // * scale.y,
    };
    cmd.pushConstants(g_pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(RetVec2) * 0, sizeof(RetVec2), &scale.x);
    cmd.pushConstants(g_pipelineLayout, vk::ShaderStageFlagBits::eVertex, sizeof(RetVec2) * 1, sizeof(RetVec2), &translate.x);

    for (auto& draw_list : draw_data->draw_lists)
    {
        cmd.drawIndexed(draw_list.index_count(), 1, 0, 0, 0);
    }
}

static void render_frame(RetDrawData* draw_data)
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

    render_retgui(draw_data);

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

    setup_vulkan_pipeline(vk::Format::eB8G8R8A8Srgb);

    retgui::create_context();

    // #TODO: Setup Platform/Renderer backends

    // #TODO: Load fonts
    // #TODO: Upload fonts

    // RetVec4 clear_color = RetVec4(0.45f, 0.55f, 0.6f, 1.0f);

    RetDrawData drawData{};
    drawData.screen_width = 1600;
    drawData.screen_height = 900;
    auto& drawList = drawData.draw_lists.emplace_back();
    drawList.add_rect({ 10, 10 }, { 200, 200 }, { 1.0f, 0.0f, 0.0f, 1.0f });
    drawList.add_rect({ 1600 - 210, 10 }, { 1600 - 10, 210 }, { 0.0f, 1.0f, 0.0f, 1.0f });
    drawList.add_rect({ 1600 * 0.5f - 100, 900 * 0.5f - 100 }, { 1600 * 0.5f + 100, 900 * 0.5f + 100 }, { 0.0f, 0.0f, 1.0f, 1.0f });

    drawList.add_line({ 10, 10 }, { 1600 * 0.5f, 900 * 0.5f }, { 1, 0, 1, 1 }, 1.0f);

    create_buffers(drawList);
    update_buffers(drawList);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll & Handle events (inputs, window resize, etc.)
        glfwPollEvents();

        render_frame(&drawData);
        present_frame();
    }

    // Cleanup
    g_device.waitIdle();

    retgui::destroy_context();

    cleanup_vulkan_window();
    cleanup_vulkan();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}