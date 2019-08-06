// Note(Leo): this is only here to manifest in sublime text autocomplete
// VK_STRUCTURE_TYPE_

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <functional>
#include <vector>
#include <fstream>
#include <array>
#include <chrono>

/* Note(Leo): glm default behaviour creates projection matrix ranging -1.0 to 1.0
for opengl. Defining this we get 0.0 to 1.0 for vulkan. */ 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

// TODO(Leo): Make sure that arrays for getting extensions etc. counts are large enough
// TOdo(Leo): Combine to fewer functions and remove throws, instead returning specific enum value

/* TODO(Leo) extra: Use separate queuefamily thing for transfering between vertex
staging buffer and actual vertex buffer. https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer */

/* STUDY(Leo):
    http://asawicki.info/news_1698_vulkan_sparse_binding_-_a_quick_overview.html
*/

#define class_member static
#define local_persist static

using int32 = int32_t;
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using bool32 = int32;

template <typename Number>
static constexpr Number MinValue = std::numeric_limits<Number>::min();

template <typename Number>
static constexpr Number MaxValue = std::numeric_limits<Number>::max();

template<typename Number>
Number Clamp(Number value, Number min, Number max)
{
    if (value < min)
        value = min;
    if (value > max)
        value = max;
    return value;
}

using BinaryAsset = std::vector<uint8>;

constexpr char texture_path [] = "textures/chalet.jpg";

#include "Array.hpp"
    
// Note(Leo): make unity build
#include "vulkan_debug_strings.cpp"
#include "command_buffer.cpp"
#include "vertex_data.cpp"
#include "Camera.cpp"


BinaryAsset
ReadBinaryFile (const char * fileName)
{
    std::ifstream file (fileName, std::ios::ate | std::ios::binary);

    if (file.is_open() == false)
    {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = file.tellg();
    BinaryAsset result (fileSize);


    file.seekg(0);
    file.read(reinterpret_cast<char*>(result.data()), fileSize);

    file.close();
    return result;    
}

constexpr int32 max_frames_in_flight = 2;
constexpr int32 window_width = 800;
constexpr int32 window_height = 600;

#define ARRAY_COUNT(array) sizeof(array) / sizeof((array)[0])

constexpr const char * validationLayers[] = {
    "VK_LAYER_KHRONOS_validation"
};
constexpr int validationLayersCount = ARRAY_COUNT(validationLayers);

constexpr const char * deviceExtensions [] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
constexpr int deviceExtensionsCount = ARRAY_COUNT(deviceExtensions);

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif



/*
LoopCounter provides repeating incrementing loop up to range
*/
template<typename Number, int Range>
class LoopCounter
{
    Number value;

public:
    using type = Number;
    constexpr class_member int range = Range;

    LoopCounter () : value (static_cast<type>(0)) {}  
    LoopCounter (type value) : value (value % range) {}

    operator type() { return value; }

    // pre increment
    type operator ++ ()
    {
        value += 1;
        value %= range;
        return value;        
    }

    // post increment
    type operator ++ (int)
    {
        type result = value;
        value += 1;
        value %= range;
        return result;
    }
};

template<int Range>
using Loop32 = LoopCounter<uint32, Range>;

struct QueueFamilyIndices
{
    uint32 graphics;
    uint32 present;

    bool hasGraphics;
    bool hasPresent;

    uint32 getAt(int index)
    {
        if (index == 0) return graphics;
        if (index == 1) return present;
        return -1;
    }

    bool hasAll()
    {
        return hasGraphics && hasPresent;
    }
};


struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

SwapchainSupportDetails
QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    SwapchainSupportDetails result = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &result.capabilities);

    uint32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount > 0)
    {
        result.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, result.formats.data());
    }

    // std::cout << "physicalDevice surface formats count: " << formatCount << "\n";

    uint32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount > 0)
    {
        result.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, result.presentModes.data());
    }

    return result;
}

VkSurfaceFormatKHR
ChooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    constexpr VkFormat preferredFormat = VK_FORMAT_R8G8B8A8_UNORM;
    constexpr VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    VkSurfaceFormatKHR result = availableFormats [0];
    for (int i = 0; i < availableFormats.size(); ++i)
    {
        if (availableFormats[i].format == preferredFormat && availableFormats[i].colorSpace == preferredColorSpace)
        {
            result = availableFormats [i];
        }   
    }
    return result;
}

VkPresentModeKHR
ChooseSurfacePresentMode(std::vector<VkPresentModeKHR> & availablePresentModes)
{
    // Todo(Leo): Is it really
    constexpr VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;
    for (int i = 0; i < availablePresentModes.size(); ++i)
    {
        if (availablePresentModes[i] == preferredPresentMode)
        {
            result = availablePresentModes[i];
        }
    }
    return result;
}

QueueFamilyIndices
FindQueueFamilies (VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilyIndices result = {};
    // Note: A card must also have a graphics queue family available; We do want to draw after all
    VkQueueFamilyProperties queueFamilyProps [50];
    uint32 queueFamilyCount = ARRAY_COUNT(queueFamilyProps);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps);

    // std::cout << "queueFamilyCount: " << queueFamilyCount << "\n";

    bool properQueueFamilyFound = false;
    // uint32 queueFamilyIndex = -1;
    for (int i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilyProps[i].queueCount > 0 && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            result.graphics = i;
            result.hasGraphics = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (queueFamilyProps[i].queueCount > 0 && presentSupport)
        {
            result.present = i;
            result.hasPresent = true;
        }

        if (result.hasAll())
        {
            break;
        }
    }
    return result;
}


VkShaderModule
CreateShaderModule(BinaryAsset code, VkDevice logicalDevice)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<uint32 *>(code.data());

    VkShaderModule result;
    if (vkCreateShaderModule (logicalDevice, &createInfo, nullptr, &result) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module");
    }

    return result;
}

class MazegameApplication
{
public:
    
    // Todo(Leo): Maybe unwind this
    void Run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    GLFWwindow * window;
    VkInstance vulkanInstance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice logicalDevice;
    VkSurfaceKHR surface;  // This is where we draw?

    /* Note: We need both graphics and present queue, but they might be on
    separate devices (correct???), so we may need to end up with multiple queues */
    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapchain;

    /* Note(Leo): these images dont need explicit memory, it is managed by GPU
    We receive these from gpu for presenting. */
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    VkRenderPass renderPass;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    VkCommandPool commandPool;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    Loop32<max_frames_in_flight> currentFrameIndex = 0;
    bool framebufferResized = false;

    // MAP BUFFER Note(Leo): map is big and can have own memory and buffer
    VkBuffer generatedMapBuffer;
    VkDeviceMemory generatedMapBufferMemory;
    VkDeviceSize generatedMapBufferIndexOffset;
    VkDeviceSize generatedMapBufferVertexOffset;
    uint32 generatedMapUniformBufferOffset;

    // CHARACTER BUFFER Note(Leo): going to contain other things too, but later
    VkBuffer characterBuffer;
    VkDeviceMemory characterBufferMemory;
    VkDeviceSize characterBufferIndexOffset;
    VkDeviceSize characterBufferVertexOffset;
    uint32 characterUniformBufferOffset;

    VkBuffer sceneUniformBuffer;
    VkDeviceMemory sceneUniformBufferMemory;
    std::vector<uint32> sceneUniformBufferOffsets;

    VkBuffer modelUniformBuffer;
    VkDeviceMemory modelUniformBufferMemory;
    std::vector<uint32> modelUniformBufferOffsets;

    // TODO(Leo): Do these and combine to same memory as scene buffer maybe
    std::vector<VkBuffer> uniformModelBuffers;
    std::vector<VkDeviceMemory> uniformModelBufferMemories;

    // IMAGE TEXTURE, Todo(Leo): these can be abstracted away, yay! :D
    uint32 textureMipLevels = 1;
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    /* Note(Leo): image and sampler are now (as in Vulkan) unrelated, and same
    sampler can be used with multiple images, noice */
    VkSampler textureSampler;
    
    // MULTISAMPLING
    VkSampleCountFlagBits msaaSamples;

    // DEPTH RESOURCES
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    // COLOR RESOURCES
    VkImage colorImage;
    VkDeviceMemory colorImageMemory;
    VkImageView colorImageView;

    Camera worldCamera;

    constexpr static int32
        DESC_scene_uniform_buffer_id = 0,
        DESC_model_uniform_buffer_id = 2,
        DESC_sampler_id = 1;
    constexpr static int32 descriptor_set_count = 3;

    constexpr static int32 MODEL_COUNT = 2;

    void InitWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, true);

        window = glfwCreateWindow(window_width, window_height, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);

        auto FramebufferResizeCallback = [] (GLFWwindow * window, int width, int height)
        {
            auto application = reinterpret_cast<MazegameApplication*>(glfwGetWindowUserPointer(window));
            application->framebufferResized = true;
        };

        glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
    }

    void 
    InitVulkan()
    {
        CreateInstance();
        // Setup debug callbacks
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();

        CreateSwapchainAndImages();
        CreateSwapchainImageViews();
        CreateRenderPass();
        CreateDescriptorSetLayout();
        CreateGraphicsPipeline();
        CreateCommandPool();
        CreateCamera();

        CreateColorResources();
        CreateDepthResources();
        CreateFrameBuffers();

        CreateImageTexture();
        CreateTextureImageView();
        CreateTextureSampler();

        generatedMap = GenerateMap();
        CreateMeshBufferAndMemory(
            &generatedMapBuffer, &generatedMapBufferMemory, &generatedMapBufferIndexOffset,
            &generatedMapBufferVertexOffset, &generatedMap);

        characterModel = LoadModel("models/character.obj");
        CreateMeshBufferAndMemory(
            &characterBuffer, &characterBufferMemory, &characterBufferIndexOffset,
            &characterBufferVertexOffset, &characterModel);

        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();

        CreateCommandBuffers();
        CreateSyncObjects();

        std::cout << "\nVulkan Initialized succesfully\n";
    }

    void
    MainLoop()
    {
        while (glfwWindowShouldClose(window) == false)
        {
            glfwPollEvents();
            DrawFrame();
        }

        // Note(Leo): All draw frame operations are asynchronous, must wait for them to finish
        vkDeviceWaitIdle(logicalDevice);
    }

    void
    CleanupSwapchain()
    {
        vkDestroyImage (logicalDevice, colorImage, nullptr);
        vkFreeMemory(logicalDevice, colorImageMemory, nullptr);
        vkDestroyImageView(logicalDevice, colorImageView, nullptr);

        vkDestroyImage (logicalDevice, depthImage, nullptr);
        vkFreeMemory (logicalDevice, depthImageMemory, nullptr);
        vkDestroyImageView (logicalDevice, depthImageView, nullptr);

        vkDestroyBuffer(logicalDevice, sceneUniformBuffer, nullptr);
        vkFreeMemory(logicalDevice, sceneUniformBufferMemory, nullptr);

        vkDestroyBuffer(logicalDevice, modelUniformBuffer, nullptr);
        vkFreeMemory(logicalDevice, modelUniformBufferMemory, nullptr);


        vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

        for (auto framebuffer : swapchainFramebuffers)
        {
            vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
        }

        vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(), &commandBuffers[0]);        

        vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
        vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

        for (auto imageView : swapchainImageViews)
        {
            vkDestroyImageView(logicalDevice, imageView, nullptr);
        }

        vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
    }

    void
    Cleanup()
    {
        CleanupSwapchain();

        vkDestroyImage(logicalDevice, textureImage, nullptr);
        vkFreeMemory(logicalDevice, textureImageMemory, nullptr);
        vkDestroyImageView(logicalDevice, textureImageView, nullptr);

        vkDestroySampler(logicalDevice, textureSampler, nullptr);

        vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);  

        vkDestroyBuffer(logicalDevice, generatedMapBuffer, nullptr);
        vkFreeMemory(logicalDevice, generatedMapBufferMemory, nullptr);

        vkDestroyBuffer(logicalDevice, characterBuffer, nullptr);
        vkFreeMemory(logicalDevice, characterBufferMemory, nullptr);

        for (int i = 0; i < max_frames_in_flight; ++i)
        {
            vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);

            vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

        vkDestroyDevice(logicalDevice, nullptr);
        vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
        vkDestroyInstance(vulkanInstance, nullptr);

        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void
    CreateInstance()
    {
        auto CheckValidationLayerSupport = [] () -> bool
        {
            VkLayerProperties availableLayers [50];
            uint32 availableLayersCount = ARRAY_COUNT(availableLayers);

            bool result = true;
            if (vkEnumerateInstanceLayerProperties (&availableLayersCount, availableLayers) == VK_SUCCESS)
            {

                // for (int i = 0; i < availableLayersCount; ++i)
                // {
                //     std::cout << availableLayers[i].layerName << "\n";
                // }

                // std::cout << "\n";

                // for (int i = 0; i < validationLayersCount; ++i)
                // {
                //     std::cout << validationLayers[i] << "\n";
                // }

                for (
                    int validationLayerIndex = 0;
                    validationLayerIndex < validationLayersCount;
                    ++validationLayerIndex)
                {
                    bool layerFound = false;
                    for(
                        int availableLayerIndex = 0; 
                        availableLayerIndex < availableLayersCount; 
                        ++availableLayerIndex)
                    {
                        if (strcmp (validationLayers[validationLayerIndex], availableLayers[availableLayerIndex].layerName) == 0)
                        {
                            layerFound = true;
                            break;
                        }
                    }

                    if (layerFound == false)
                    {
                        result = false;
                        break;
                    }
                }
            }

            return result;
        };

        if (enableValidationLayers && CheckValidationLayerSupport() == false)
        {
            throw std::runtime_error("Validation layers required but not present");
        }

        VkApplicationInfo appInfo = {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Vulkan practice";
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pApplicationInfo = &appInfo;

        uint32 glfwExtensionCount = 0;
        const char ** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        instanceInfo.enabledExtensionCount = glfwExtensionCount;
        instanceInfo.ppEnabledExtensionNames = glfwExtensions;

        if constexpr (enableValidationLayers)
        {
            instanceInfo.enabledLayerCount = validationLayersCount;
            instanceInfo.ppEnabledLayerNames = validationLayers;
        }
        else
        {
            instanceInfo.enabledLayerCount = 0;
        }

        if (vkCreateInstance(&instanceInfo, nullptr, &vulkanInstance) != VK_SUCCESS)
        {
            // Todo: Bad stuff happened, abort
        }


        // Todo(Leo): check if glfw extensions are found
        // VkExtensionProperties extensions [50];
        // uint32 extensionCount = ARRAY_COUNT(extensions);
        // vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions); 


        // for (int i = 0; i < glfwExtensionCount; ++i)
        // {
        //     std::cout << "\t" << glfwExtensions [i] << "\n";
        // }

        // std::cout << "\n";

        // for (int i = 0; i < extensionCount; ++i)
        // {
        //     std::cout << "\t" << extensions[i].extensionName << "\n";   
        // }

        // std::cout << "extensionCount: " << extensionCount << "\n";
    }

    void
    CreateSurface()
    {
        if (glfwCreateWindowSurface(vulkanInstance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface");
        }
    }

    void
    PickPhysicalDevice()
    {
        auto CheckDeviceExtensionSupport = [] (VkPhysicalDevice testDevice) -> bool
        {
            VkExtensionProperties availableExtensions [100];
            uint32 availableExtensionsCount = ARRAY_COUNT(availableExtensions);
            vkEnumerateDeviceExtensionProperties (testDevice, nullptr, &availableExtensionsCount, availableExtensions);

            // std::cout << "Device extensions found: " << availableExtensionsCount << "\n";
            // for (int i = 0; i < availableExtensionsCount; ++i)
            // {
            //     std::cout << "\t" << availableExtensions[i].extensionName << "\n";
            // }

            bool result = true;
            for (int requiredIndex = 0;
                requiredIndex < deviceExtensionsCount;
                ++requiredIndex)
            {

                bool requiredExtensionFound = false;
                for (int availableIndex = 0;
                    availableIndex < availableExtensionsCount;
                    ++availableIndex)
                {
                    if (strcmp(deviceExtensions[requiredIndex], availableExtensions[availableIndex].extensionName) == 0)
                    {
                        requiredExtensionFound = true;
                        break;
                    }
                }

                result = requiredExtensionFound;
                if (result == false)
                {
                    break;
                }
            }

            return result;
        };

        auto IsPhysicalDeviceSuitable = [this, CheckDeviceExtensionSupport] (VkPhysicalDevice testDevice) -> bool
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(testDevice, &props);
            bool isDedicatedGPU = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures(testDevice, &features);

            bool extensionsAreSupported = CheckDeviceExtensionSupport(testDevice);

            bool swapchainIsOk = false;
            if (extensionsAreSupported)
            {
                SwapchainSupportDetails swapchainSupport = QuerySwapChainSupport(testDevice, surface);
                swapchainIsOk = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
            }

            QueueFamilyIndices indices = FindQueueFamilies(testDevice, surface);
            return  isDedicatedGPU 
                    && indices.hasAll()
                    && extensionsAreSupported
                    && swapchainIsOk
                    && features.samplerAnisotropy;
        };

        auto GetMaxUsableMsaaSampleCount = [] (VkPhysicalDevice physicalDevice) -> VkSampleCountFlagBits
        {
            VkPhysicalDeviceProperties physicalDeviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

            VkSampleCountFlags counts = std::min(
                    physicalDeviceProperties.limits.framebufferColorSampleCounts,
                    physicalDeviceProperties.limits.framebufferDepthSampleCounts
                );

            if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
            if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
            if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
            if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
            if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
            if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

            return VK_SAMPLE_COUNT_1_BIT;
        };

        VkPhysicalDevice devices [10];
        uint32 deviceCount = ARRAY_COUNT(devices);
        vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices);

        if (deviceCount == 0)
        {
            throw std::runtime_error("No GPU devices found");
        }

        for (int i = 0; i < deviceCount; i++)
        {
            if (IsPhysicalDeviceSuitable(devices[i]))
            {
                physicalDevice = devices[i];
                msaaSamples = GetMaxUsableMsaaSampleCount(physicalDevice);
                std::cout << "Sample count: " << VulkanSampleCountFlagBitsString(msaaSamples) << "\n";
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("No suitable GPU device found");
        }
    }

    void
    CreateLogicalDevice()
    {
        QueueFamilyIndices queueIndices = FindQueueFamilies(physicalDevice, surface);


        /* Note: We need both graphics and present queue, but they might be on
        separate devices (correct???), so we may need to end up with multiple queues */
        int uniqueQueueFamilyCount = queueIndices.graphics == queueIndices.present ? 1 : 2;
        VkDeviceQueueCreateInfo queueCreateInfos [2] = {};
        float queuePriorities[/*queueCount*/] = { 1.0f };
        for (int i = 0; i <uniqueQueueFamilyCount; ++i)
        {
            // VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfos[i].queueFamilyIndex = queueIndices.getAt(i);
            queueCreateInfos[i].queueCount = 1;

            queueCreateInfos[i].pQueuePriorities = queuePriorities;
        }

        VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
        physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
        deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
        deviceCreateInfo.enabledExtensionCount = deviceExtensionsCount;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

        if constexpr (enableValidationLayers)
        {
            deviceCreateInfo.enabledLayerCount = validationLayersCount;
            deviceCreateInfo.ppEnabledLayerNames = validationLayers;
        }
        else
        {
            deviceCreateInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &deviceCreateInfo,nullptr, &logicalDevice) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create vulkan logical device");
        }

        vkGetDeviceQueue(logicalDevice, queueIndices.graphics, 0, &graphicsQueue);
        vkGetDeviceQueue(logicalDevice, queueIndices.present, 0, &presentQueue);
    }

    void
    CreateSwapchainAndImages()
    {
        SwapchainSupportDetails swapchainSupport = QuerySwapChainSupport(physicalDevice, surface);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.formats);
        VkPresentModeKHR presentMode = ChooseSurfacePresentMode(swapchainSupport.presentModes);

        // Find extent ie. size of drawing window
        /* Note(Leo): max value is special value denoting that all are okay.
        Or something else, so we need to ask platform */
        if (swapchainSupport.capabilities.currentExtent.width != MaxValue<uint32>)
        {
            swapchainExtent = swapchainSupport.capabilities.currentExtent;
        }
        else
        {   
            int32 width, height;
            glfwGetFramebufferSize(window, &width, &height);

            VkExtent2D min = swapchainSupport.capabilities.minImageExtent;
            VkExtent2D max = swapchainSupport.capabilities.maxImageExtent;

            swapchainExtent.width = Clamp(static_cast<uint32>(width), min.width, max.width);
            swapchainExtent.height = Clamp(static_cast<uint32>(height), min.height, max.height);
        }

        uint32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
        uint32 maxImageCount = swapchainSupport.capabilities.maxImageCount;
        if (maxImageCount > 0 && imageCount > maxImageCount)
        {
            imageCount = maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = swapchainExtent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices queueIndices = FindQueueFamilies(physicalDevice, surface);
        uint32 queueIndicesArray [2] = {queueIndices.graphics, queueIndices.present};

        if (queueIndices.graphics == queueIndices.present)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueIndicesArray;
        }

        createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapchain) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create swap chain");
        }

        swapchainImageFormat = surfaceFormat.format;

        // Note(Leo): Swapchain images are not created, they are gotten from api
        uint32 imgeCount;
        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, nullptr);
        swapchainImages.resize (imageCount);
        vkGetSwapchainImagesKHR(logicalDevice, swapchain, &imageCount, &swapchainImages[0]);
    
        std::cout << "Created swapchain and images\n";
    }

    void
    CreateSwapchainImageViews()
    {
        int imageCount = swapchainImages.size();
        swapchainImageViews.resize(imageCount);

        for (int i = 0; i < imageCount; ++i)
        {
            swapchainImageViews[i] = CreateImageView(
                logicalDevice, swapchainImages[i], 1, 
                swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        }

        std::cout << "Created image views\n";
    }

    void 
    CreateRenderPass()
    {
        constexpr int 
            color_attachment_id     = 0,
            depth_attachment_id     = 1,
            resolve_attachments_id  = 2,
            attachment_count        = 3;

        VkAttachmentDescription attachments[attachment_count] = {};

        attachments[color_attachment_id].format         = swapchainImageFormat;
        attachments[color_attachment_id].samples        = msaaSamples;
        attachments[color_attachment_id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[color_attachment_id].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[color_attachment_id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[color_attachment_id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[color_attachment_id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[color_attachment_id].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments[depth_attachment_id].format         = FindSupportedDepthFormat();
        attachments[depth_attachment_id].samples        = msaaSamples;
        attachments[depth_attachment_id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[depth_attachment_id].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[depth_attachment_id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[depth_attachment_id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[depth_attachment_id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[depth_attachment_id].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments[resolve_attachments_id].format         = swapchainImageFormat;
        attachments[resolve_attachments_id].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[resolve_attachments_id].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[resolve_attachments_id].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[resolve_attachments_id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[resolve_attachments_id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[resolve_attachments_id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[resolve_attachments_id].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        constexpr int32 colorAttachmentCount = 1;        
        VkAttachmentReference colorAttachmentRefs[colorAttachmentCount] = {};
        colorAttachmentRefs[0].attachment = color_attachment_id;
        colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        // Note(Leo): there can be only one depth attachment
        VkAttachmentReference depthStencilAttachmentRef = {};
        depthStencilAttachmentRef.attachment = depth_attachment_id;
        depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference resolveAttachmentRefs [colorAttachmentCount] = {};
        resolveAttachmentRefs[0].attachment = resolve_attachments_id;
        resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpasses[1] = {};
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].colorAttachmentCount = colorAttachmentCount;
        subpasses[0].pColorAttachments = &colorAttachmentRefs[0];
        subpasses[0].pResolveAttachments = &resolveAttachmentRefs[0];
        subpasses[0].pDepthStencilAttachment = &depthStencilAttachmentRef;

        // Note(Leo): subpass dependencies
        VkSubpassDependency dependencies[1] = {};
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
        renderPassInfo.attachmentCount = attachment_count;
        renderPassInfo.pAttachments = &attachments[0];
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpasses[0];
        renderPassInfo.dependencyCount = ARRAY_COUNT(dependencies);
        renderPassInfo.pDependencies = &dependencies[0];

        if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass");
        }
    }

    void
    CreateDescriptorSetLayout()
    {
        Array<VkDescriptorSetLayoutBinding, descriptor_set_count> layoutBindings = {};

        // UNIFORM BUFFER OBJECT, camera projection matrices for now
        layoutBindings[DESC_scene_uniform_buffer_id].binding = DESC_scene_uniform_buffer_id;
        layoutBindings[DESC_scene_uniform_buffer_id].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBindings[DESC_scene_uniform_buffer_id].descriptorCount = 1;
        layoutBindings[DESC_scene_uniform_buffer_id].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBindings[DESC_scene_uniform_buffer_id].pImmutableSamplers = nullptr; // Note(Leo): relevant for sampler stuff, like textures

        // SAMPLER
        layoutBindings[DESC_sampler_id].binding = DESC_sampler_id;
        layoutBindings[DESC_sampler_id].descriptorCount = 1;
        layoutBindings[DESC_sampler_id].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        layoutBindings[DESC_sampler_id].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        layoutBindings[DESC_sampler_id].pImmutableSamplers = nullptr;

        // MODEL TRANSFORM BUFFES
        layoutBindings[DESC_model_uniform_buffer_id].binding = DESC_model_uniform_buffer_id;
        layoutBindings[DESC_model_uniform_buffer_id].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        layoutBindings[DESC_model_uniform_buffer_id].descriptorCount = 1;
        layoutBindings[DESC_model_uniform_buffer_id].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        layoutBindings[DESC_model_uniform_buffer_id].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutCreateInfo.bindingCount = layoutBindings.count();
        layoutCreateInfo.pBindings = &layoutBindings[0];

        if(vkCreateDescriptorSetLayout(logicalDevice, &layoutCreateInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create the descriptor set layout");
        }
    }
    
    void
    CreateGraphicsPipeline()
    {
        // Todo: these can be removed totally from here, just call inline in next functions
        BinaryAsset vertexShaderCode = ReadBinaryFile("shaders/vert.spv");
        BinaryAsset fragmentShaderCode = ReadBinaryFile("shaders/frag.spv");

        VkShaderModule vertexShaderModule = CreateShaderModule(vertexShaderCode, logicalDevice);
        VkShaderModule fragmentShaderModule = CreateShaderModule(fragmentShaderCode, logicalDevice);

        VkPipelineShaderStageCreateInfo shaderStages [2] = {};
        constexpr uint32 
            vertex_stage_id     = 0,
            fragment_stage_id   = 1;

        shaderStages[vertex_stage_id].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[vertex_stage_id].stage = VK_SHADER_STAGE_VERTEX_BIT;
        shaderStages[vertex_stage_id].module = vertexShaderModule;
        shaderStages[vertex_stage_id].pName = "main";

        shaderStages[fragment_stage_id].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shaderStages[fragment_stage_id].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        shaderStages[fragment_stage_id].module = fragmentShaderModule;
        shaderStages[fragment_stage_id].pName = "main";

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        
        auto bindingDescription = Vertex::GetBindingDescription();
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        
        auto attributeDescriptions = Vertex::GetAttributeDescriptions();
        vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.count();
        vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptions[0];

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapchainExtent.width;
        viewport.height = (float) swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = swapchainExtent;

        VkPipelineViewportStateCreateInfo viewportState = {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer = {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f;
        rasterizer.depthBiasClamp = 0.0f;
        rasterizer.depthBiasSlopeFactor = 0.0f;

        VkPipelineMultisampleStateCreateInfo multisampling = {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = msaaSamples;
        multisampling.minSampleShading = 1.0f;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = VK_FALSE;
        multisampling.alphaToOneEnable = VK_FALSE;

        // Todo: Depth and stencil

        // Note: This attachment is per framebuffer
        VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor =  VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstColorBlendFactor =  VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.colorBlendOp =         VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor =  VK_BLEND_FACTOR_ONE;
        colorBlendAttachment.dstAlphaBlendFactor =  VK_BLEND_FACTOR_ZERO;
        colorBlendAttachment.alphaBlendOp =         VK_BLEND_OP_ADD; 

        // Note: this is global
        VkPipelineColorBlendStateCreateInfo colorBlending = {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE; // Note: enabling this disables per framebuffer method above
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;       
        colorBlending.blendConstants[1] = 0.0f;       
        colorBlending.blendConstants[2] = 0.0f;       
        colorBlending.blendConstants[3] = 0.0f;       

        VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

        // Note(Leo): These further limit depth range for this render pass
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;

        // Note(Leo): Configure stencil tests
        depthStencil.stencilTestEnable = VK_FALSE;
        depthStencil.front = {};
        depthStencil.back = {};

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;

        if (vkCreatePipelineLayout (logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo = {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;

        // Note: Fixed-function stages
        pipelineInfo.pVertexInputState      = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState    = &inputAssembly;
        pipelineInfo.pViewportState         = &viewportState;
        pipelineInfo.pRasterizationState    = &rasterizer;
        pipelineInfo.pMultisampleState      = &multisampling;
        pipelineInfo.pDepthStencilState     = &depthStencil;
        pipelineInfo.pColorBlendState       = &colorBlending;
        pipelineInfo.pDynamicState          = nullptr;

        pipelineInfo.layout = pipelineLayout;

        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;

        // Note: These are for cheaper re-use of pipelines, not used right now
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline");
        }

        // Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
        vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(logicalDevice, vertexShaderModule, nullptr);
    }

    void
    CreateFrameBuffers()
    {
        
        /* Note(Leo): This is basially allocating right, there seems to be no
         need for VkDeviceMemory for swapchainimages??? */
        
        int count = swapchainImageViews.size();
        swapchainFramebuffers.resize(count);

        for (int i = 0; i < count; ++i)
        {
            constexpr int attachment_count = 3;
            VkImageView attachments[attachment_count] = {
                colorImageView,
                depthImageView,
                swapchainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo = {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = attachment_count;
            framebufferInfo.pAttachments = &attachments[0];
            framebufferInfo.width = swapchainExtent.width;
            framebufferInfo.height = swapchainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer");
            }
        }
    }

    void
    CreateCommandPool()
    {
        QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice, surface);

        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphics;
        poolInfo.flags = 0;

        if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool");
        }
        
        std::cout << "Created command pool\n";
    }

    void
    CreateCamera()
    {
        worldCamera = {};

        worldCamera.target = vector3_zero;
        worldCamera.distance = 10;
        worldCamera.rotation = 0;

        worldCamera.fieldOfView = 45;
        worldCamera.nearClipPlane = 0.1f;
        worldCamera.farClipPlane = 100.0f;
        worldCamera.aspectRatio = (float)swapchainExtent.width / (float)swapchainExtent.height;
    }

    VkFormat
    FindSupportedFormat(
        int32 candidateCount,
        VkFormat * pCandidates,
        VkImageTiling requestedTiling,
        VkFormatFeatureFlags requestedFeatures
    ){
        bool32 requestOptimalTiling = requestedTiling == VK_IMAGE_TILING_OPTIMAL;

        for (VkFormat * pFormat = pCandidates; pFormat != pCandidates + candidateCount; ++pFormat)
        {
            VkFormatProperties properties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, *pFormat, &properties);

            VkFormatFeatureFlags features = requestOptimalTiling ? 
                properties.optimalTilingFeatures : properties.linearTilingFeatures;    

            if ((features & requestedFeatures) == requestedFeatures)
            {
                return *pFormat;
            }
        }

        throw std::runtime_error("Failed to find supported format");
    }


    void
    CreateColorResources()
    {
        VkFormat colorFormat = swapchainImageFormat;

        CreateImage(logicalDevice, physicalDevice,
                    swapchainExtent.width, swapchainExtent.height,
                    1, colorFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    msaaSamples,
                    &colorImage, &colorImageMemory);

        colorImageView = CreateImageView(logicalDevice, colorImage, 1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

        TransitionImageLayout(  colorImage, colorFormat, 1, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    VkFormat
    FindSupportedDepthFormat()
    {
        VkFormat formats [] = { VK_FORMAT_D32_SFLOAT,
                                VK_FORMAT_D32_SFLOAT_S8_UINT,
                                VK_FORMAT_D24_UNORM_S8_UINT };

        VkFormat result = FindSupportedFormat(
                            ARRAY_COUNT(formats), formats, VK_IMAGE_TILING_OPTIMAL, 
                            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
        return result;
    }

    void
    CreateDepthResources()
    {

        VkFormat depthFormat = FindSupportedDepthFormat();

        CreateImage(logicalDevice, physicalDevice,
                    swapchainExtent.width, swapchainExtent.height,
                    1, depthFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    msaaSamples,
                    &depthImage, &depthImageMemory);

        depthImageView = CreateImageView(logicalDevice, depthImage, 1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

        TransitionImageLayout(  depthImage, depthFormat, 1,VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }


    // Todo(Leo): FindMemoryType and CreateBuffer could be free functions, se if we want to do that
    // Todo(Leo): On the other hand, see if these should be proper member functions...
    class_member uint32
    FindMemoryType (VkPhysicalDevice physicalDevice, uint32 typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties (physicalDevice, &memoryProperties);

        for (int i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            bool filterMatch = (typeFilter & (1 << i)) != 0;
            bool memoryTypeOk = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
            if (filterMatch && memoryTypeOk)
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type.");
    }   

    // Todo(Leo): see if we can change this to proper functional function
    class_member void
    CreateBuffer(
        VkDevice                logicalDevice,
        VkPhysicalDevice        physicalDevice,
        VkDeviceSize            bufferSize,
        VkBufferUsageFlags      usage,
        VkMemoryPropertyFlags   memoryProperties,
        VkBuffer *              resultBuffer,
        VkDeviceMemory *        resultBufferMemory
    ){
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = bufferSize;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.flags = 0;

        if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, resultBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(logicalDevice, *resultBuffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryType(
                                            physicalDevice,
                                            memoryRequirements.memoryTypeBits,
                                            memoryProperties);

        std::cout << "memoryTypeIndex : " << allocateInfo.memoryTypeIndex << "\n";

        /* Todo(Leo): do not actually always use new allocate, but instead allocate
        once and create allocator to handle multiple items */
        if (vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, resultBufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate buffer memory");
        }

        vkBindBufferMemory(logicalDevice, *resultBuffer, *resultBufferMemory, 0); 
    }

    class_member void
    CopyBuffer(
        VkDevice logicalDevice,
        VkCommandPool commandPool,
        VkQueue transferQueue,
        VkBuffer sourceBuffer, 
        VkBuffer destinationBuffer, 
        VkDeviceSize size)
    {
        VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(logicalDevice, commandPool);

        VkBufferCopy copyRegion = {0, 0, size};
        vkCmdCopyBuffer(commandBuffer, sourceBuffer, destinationBuffer, 1, &copyRegion);

        EndOneTimeCommandBuffer(logicalDevice, commandPool, transferQueue, commandBuffer);
    }

    // Todo(Leo): This 'class_member' thing starts to make no sense, nice experiment though :))))
    class_member void
    CreateImage(
        VkDevice logicalDevice, 
        VkPhysicalDevice physicalDevice,
        uint32 texWidth,
        uint32 texHeight,
        uint32 mipLevels,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags memoryFlags,
        VkSampleCountFlagBits sampleCount,
        VkImage * resultImage,
        VkDeviceMemory * resultImageMemory
    ){
        VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent = { texWidth, texHeight, 1 };
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;

        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // note(leo): concerning queue families
        imageInfo.samples = sampleCount;
        imageInfo.flags = 0;

        // Note(Leo): some image formats may not be supported
        if (vkCreateImage(logicalDevice, &imageInfo, nullptr, resultImage) != VK_SUCCESS)
        {
            throw std::runtime_error ("Failed to create image");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements (logicalDevice, *resultImage, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryType (
                                        physicalDevice,
                                        memoryRequirements.memoryTypeBits,
                                        memoryFlags);

        if (vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, resultImageMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate image memory");
        }

        vkBindImageMemory(logicalDevice, *resultImage, *resultImageMemory, 0);   
    }

    bool32
    FormatHasStencilComponent(VkFormat format)
    {
        bool32 result = (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT);
        return result;
    }


    // Change image layout from stored pixel array layout to device optimal layout
    void
    TransitionImageLayout(
        VkImage image,
        VkFormat format,
        uint32 mipLevels,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    ){
        VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(logicalDevice, commandPool);

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.oldLayout = oldLayout; // Note(Leo): Can be VK_IMAGE_LAYOUT_UNDEFINED if we dont care??
        barrier.newLayout = newLayout;

        /* Note(Leo): these are used if we are to change queuefamily ownership.
        Otherwise must be set to 'VK_QUEUE_FAMILY_IGNORED' */
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.image = image;

        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            if (FormatHasStencilComponent(format))
            {
                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        /* Todo(Leo): This is ultimate stupid, we rely on knowing all this based
        on two variables, instead rather pass more arguments, or struct, or a 
        index to lookup table or a struct from that lookup table.
    
        This function should at most handle the command buffer part instead of
        quessing omitted values.
        */

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
            && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
                && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        // DEPTH IMAGE
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
                && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask =  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT 
                                    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        }
        // COLOR IMAGE
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
                && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                                    | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }


        else
        {
            throw std::runtime_error("This layout transition is not supported!");
        }

        VkDependencyFlags dependencyFlags = 0;
        vkCmdPipelineBarrier(   commandBuffer,
                                sourceStage, destinationStage,
                                dependencyFlags,
                                0, nullptr,
                                0, nullptr,
                                1, &barrier);

        EndOneTimeCommandBuffer (logicalDevice, commandPool, graphicsQueue, commandBuffer);
    }

    // Note(Leo): expect image layout be VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    void
    CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, uint32 width, uint32 height)
    {
        VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(logicalDevice, commandPool);

        VkBufferImageCopy region = {};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;

        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = { width, height, 1 };

        vkCmdCopyBufferToImage (commandBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        EndOneTimeCommandBuffer (logicalDevice, commandPool, graphicsQueue, commandBuffer);
    }

    uint32
    ComputeMipmapLevels(uint32 texWidth, uint32 texHeight)
    {
       uint32 result = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;
       return result;
    }

    void
    GenerateMipMaps(VkImage image, VkFormat imageFormat, uint32 texWidth, uint32 texHeight, uint32 mipLevels)
    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

        if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == false)
        {
            throw std::runtime_error("Texture image format does not support blitting!");
        }


        VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(logicalDevice, commandPool);

        VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int mipWidth = texWidth;
        int mipHeight = texHeight;

        for (int i = 1; i <mipLevels; ++i)
        {
            int newMipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
            int newMipHeight = mipHeight > 1 ? mipHeight / 2 : 1;

            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);


            VkImageBlit blit = {};
            
            blit.srcOffsets [0] = {0, 0, 0};
            blit.srcOffsets [1] = {mipWidth, mipHeight, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1; 

            blit.dstOffsets [0] = {0, 0, 0};
            blit.dstOffsets [1] = {newMipWidth, newMipHeight, 1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier (commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                1, &barrier);

            mipWidth = newMipWidth;
            mipHeight = newMipHeight;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
            1, &barrier);

        EndOneTimeCommandBuffer (logicalDevice, commandPool, graphicsQueue, commandBuffer);
    }

    void
    CreateImageTexture()
    {
        int32 texWidth, texHeight, texChannels;
        stbi_uc * pixels = stbi_load(texture_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        textureMipLevels = ComputeMipmapLevels(texWidth, texHeight);

        std::cout << "Mip levels: " << textureMipLevels << "\n";

        // Note(Leo): 4 applies for rgba images, duh
        VkDeviceSize imageSize = texWidth * texHeight * 4;

        if(pixels == nullptr)
        {
            throw std::runtime_error("Failed to load image");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(   logicalDevice, physicalDevice, imageSize,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &stagingBuffer, &stagingBufferMemory);

        void * data;
        vkMapMemory(logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy (data, pixels, imageSize);
        vkUnmapMemory(logicalDevice, stagingBufferMemory);
        stbi_image_free(pixels);

        CreateImage(logicalDevice, physicalDevice,
                    texWidth, texHeight, textureMipLevels,
                    VK_FORMAT_R8G8B8A8_UNORM,
                    VK_IMAGE_TILING_OPTIMAL, 
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    VK_SAMPLE_COUNT_1_BIT,
                    &textureImage, &textureImageMemory);

        /* Note(Leo):
            1. change layout to copy optimal
            2. copy contents
            3. change layout to read optimal

            This last is good to use after generated textures, finally some 
            benefits from this verbose nonsense :D
        */
        // Todo(Leo): begin and end command buffers once only and then just add commands from inside these
        TransitionImageLayout(  textureImage, VK_FORMAT_R8G8B8A8_UNORM, textureMipLevels,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        CopyBufferToImage(stagingBuffer, textureImage, texWidth, texHeight);

        /* Note(Leo): mipmap generator will be doing the transition top proper layout for now
        Eventually miplevels will be stored in file instead and we need to transition manually */
        // TransitionImageLayout(  textureImage, VK_FORMAT_R8G8B8A8_UNORM, textureMipLevels,
        //                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        //                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);        

        GenerateMipMaps(textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, textureMipLevels);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }

    class_member VkImageView
    CreateImageView(
        VkDevice logicalDevice,
        VkImage image,
        uint32 mipLevels,
        VkFormat format,
        VkImageAspectFlags aspectFlags
    ){
        VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewInfo.image = image;
        imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewInfo.format = format;

        imageViewInfo.subresourceRange.aspectMask = aspectFlags;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = mipLevels;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = 1;

        imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        VkImageView result;
        if (vkCreateImageView(logicalDevice, &imageViewInfo, nullptr, &result) != VK_SUCCESS)
        {
            throw std::runtime_error ("Failed to create image view");
        }

        return result;
    }

    void
    CreateTextureImageView()
    {
        textureImageView = CreateImageView(logicalDevice, textureImage, textureMipLevels, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    void
    CreateTextureSampler()
    {
        VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = 16;

        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(textureMipLevels);
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create texture sampler!");
        }
    }

    uint64
    AlignUpTo(uint64 alignment, uint64 size)
    {
        /*
        Divide to get number of full multiples of alignment
        Add one so we end up bigger
        Multiply to get actual size, that is both bigger AND a multiple of alignment
        */

        size /= alignment;
        size += 1;
        size *= alignment;
        return size;
    }

    void
    CreateMeshBufferAndMemory(
        VkBuffer *          resultBuffer,
        VkDeviceMemory *    resultBufferMemory,
        VkDeviceSize *      resultIndexOffset,
        VkDeviceSize *      resultVertexOffset,
        Mesh *              mesh
    ){
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        
        VkDeviceSize vertexBufferSize       = mesh->vertices.size() * sizeof(mesh->vertices[0]);
        VkDeviceSize indexBufferSize        = mesh->indices.size() * sizeof(mesh->indices[0]);
        VkDeviceSize totalBufferSize        = vertexBufferSize + indexBufferSize;// + modelProjectionSize;
        
        VkDeviceSize
            indexBufferOffset = 0,
            vertexBufferOffset = indexBufferSize;

        // WRITE RESULTS NOW
        *resultIndexOffset = indexBufferOffset;
        *resultVertexOffset = vertexBufferOffset;

        VkBufferCreateInfo stagingBufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        stagingBufferInfo.size = totalBufferSize;        
        stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        stagingBufferInfo.flags = 0;

        if (vkCreateBuffer (logicalDevice, &stagingBufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create staging buffer for vertices and indices");
        }

        VkMemoryRequirements memoryRequirements;
        vkGetBufferMemoryRequirements(logicalDevice, stagingBuffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = FindMemoryType(
                                            physicalDevice,
                                            memoryRequirements.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate staging buffer memory for vertex and index buffer");
        }

        vkBindBufferMemory(logicalDevice, stagingBuffer, stagingBufferMemory, 0);

        // COPY MESH INFO TO STAGING BUFFER, Todo(Leo): we could also load data directly here, avoiding memcpy        
        uint8 * data;
        vkMapMemory(logicalDevice, stagingBufferMemory, 0, totalBufferSize, 0, (void**)&data);
        memcpy (data, &mesh->indices[0], indexBufferSize);
        data += indexBufferSize;
        memcpy (data, &mesh->vertices[0], vertexBufferSize);
        vkUnmapMemory(logicalDevice, stagingBufferMemory);

        // Todo(Leo): We can move these to command buffer below
        CreateBuffer(   logicalDevice, physicalDevice, totalBufferSize,
                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        resultBuffer, resultBufferMemory);

        VkCommandBuffer commandBuffer = BeginOneTimeCommandBuffer(logicalDevice, commandPool);

        VkBufferCopy combinedCopyRegion = { 0, 0, totalBufferSize };
        vkCmdCopyBuffer(commandBuffer, stagingBuffer, *resultBuffer, 1, &combinedCopyRegion);

        EndOneTimeCommandBuffer(logicalDevice, commandPool, graphicsQueue, commandBuffer);

        vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
        vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
    }


    void 
    CreateUniformBuffers()
    {
        /* Todo(Leo): Uniform model buffer, first normal to get things running 
        and then dynamic to enable separate values for each object */

        /* Note(Leo):
        Create single uniform buffer for scene info such as camera projection and lights.
        Store offsets and use them to differentiate between regions.
        */

        int32 imageCount = swapchainImages.size();
        
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        VkDeviceSize minUniformBufferOffsetAlignment = physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;

        uint64 memorySizePerObject = AlignUpTo(minUniformBufferOffsetAlignment, sizeof(CameraProjectionsUniformBufferObject));
        VkDeviceSize fullSceneUniformBufferSize = imageCount * memorySizePerObject;

        CreateBuffer(   logicalDevice, physicalDevice, fullSceneUniformBufferSize,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                        &sceneUniformBuffer, &sceneUniformBufferMemory);

        uint64 memorySizePerModelMatrix = AlignUpTo(minUniformBufferOffsetAlignment, sizeof(glm::mat4));
        VkDeviceSize fullModelUniformBufferSize = imageCount * memorySizePerModelMatrix * MODEL_COUNT;


        CreateBuffer(   logicalDevice, physicalDevice, fullModelUniformBufferSize,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        &modelUniformBuffer, &modelUniformBufferMemory);

        generatedMapUniformBufferOffset = 0;
        characterUniformBufferOffset = memorySizePerModelMatrix;

        sceneUniformBufferOffsets.resize(imageCount);
        modelUniformBufferOffsets.resize(imageCount);

        for(int i = 0; i < imageCount; ++i)
        {
            sceneUniformBufferOffsets[i] = i * memorySizePerObject;
            modelUniformBufferOffsets[i] = i * memorySizePerModelMatrix * MODEL_COUNT;
        }
    }

    void
    CreateDescriptorPool()
    {
        int32 swapchainImageCount = swapchainImages.size();

        constexpr int32
            kUniformDescriptorPool  = 0,
            DYNAMIC_UNIFORM_BUFFER_POOL = 1,
            kSamplerDescriptorPool  = 2,
            kDescriptorPoolCount    = 3;

        // Note(Leo): there needs to only one per type, not one per user
        VkDescriptorPoolSize poolSizes [kDescriptorPoolCount] = {};

        uint32 uniformBufferDescriptorCount = swapchainImageCount * 2;
        poolSizes[kUniformDescriptorPool].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

        // Note(Leo): multiply by many to surely get enough
        // Todo(Leo): make a lot of more sensible number than arbitrary 10!!!!
        poolSizes[kUniformDescriptorPool].descriptorCount = uniformBufferDescriptorCount * 10;

        poolSizes[DYNAMIC_UNIFORM_BUFFER_POOL].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        poolSizes[DYNAMIC_UNIFORM_BUFFER_POOL].descriptorCount = uniformBufferDescriptorCount * 10; 


        uint32 samplerDescriptorCount = swapchainImageCount;
        poolSizes[kSamplerDescriptorPool].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[kSamplerDescriptorPool].descriptorCount = samplerDescriptorCount;

        VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = kDescriptorPoolCount;
        poolInfo.pPoolSizes = &poolSizes[0];
        poolInfo.maxSets = swapchainImages.size();

        if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor pool");
        }
    }

    void
    CreateDescriptorSets()
    {
        /* Note(Leo): Create vector of [swapchainImageCount] copies from descriptorSetLayout
        for allocation */
        int swapchainImageCount = swapchainImages.size();
        std::vector<VkDescriptorSetLayout> layouts (swapchainImageCount, descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = swapchainImageCount;
        allocateInfo.pSetLayouts = &layouts[0];

        descriptorSets.resize (swapchainImageCount);
        if (vkAllocateDescriptorSets(logicalDevice, &allocateInfo, &descriptorSets[0]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
        }

        for (int i = 0; i < swapchainImageCount; ++i)
        {
            Array<VkWriteDescriptorSet, descriptor_set_count> descriptorWrites = {};

            // SCENE UNIFORM BUFFER
            VkDescriptorBufferInfo sceneBufferInfo = {};
            sceneBufferInfo.buffer = sceneUniformBuffer;
            sceneBufferInfo.offset = sceneUniformBufferOffsets[i];
            sceneBufferInfo.range = sizeof(CameraProjectionsUniformBufferObject);

            descriptorWrites[DESC_scene_uniform_buffer_id].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[DESC_scene_uniform_buffer_id].dstSet = descriptorSets[i];
            descriptorWrites[DESC_scene_uniform_buffer_id].dstBinding = DESC_scene_uniform_buffer_id;
            descriptorWrites[DESC_scene_uniform_buffer_id].dstArrayElement = 0;
            descriptorWrites[DESC_scene_uniform_buffer_id].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[DESC_scene_uniform_buffer_id].descriptorCount = 1;
            descriptorWrites[DESC_scene_uniform_buffer_id].pBufferInfo = &sceneBufferInfo;

            // IMAGE DESC_sampler_id
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            descriptorWrites[DESC_sampler_id].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[DESC_sampler_id].dstSet = descriptorSets[i];
            descriptorWrites[DESC_sampler_id].dstBinding = DESC_sampler_id;
            descriptorWrites[DESC_sampler_id].dstArrayElement = 0;
            descriptorWrites[DESC_sampler_id].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[DESC_sampler_id].descriptorCount = 1;
            descriptorWrites[DESC_sampler_id].pImageInfo = &imageInfo;

            // MODEL UNIFORM BUFFERS
            VkDescriptorBufferInfo modelBufferInfo = {};
            modelBufferInfo.buffer = modelUniformBuffer;
            modelBufferInfo.offset = modelUniformBufferOffsets[i];
            modelBufferInfo.range = sizeof(glm::mat4);

            descriptorWrites[DESC_model_uniform_buffer_id].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[DESC_model_uniform_buffer_id].dstSet = descriptorSets[i];
            descriptorWrites[DESC_model_uniform_buffer_id].dstBinding = DESC_model_uniform_buffer_id;
            descriptorWrites[DESC_model_uniform_buffer_id].dstArrayElement = 0;
            descriptorWrites[DESC_model_uniform_buffer_id].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            descriptorWrites[DESC_model_uniform_buffer_id].descriptorCount = 1;
            descriptorWrites[DESC_model_uniform_buffer_id].pBufferInfo = &modelBufferInfo;

            // Note(Leo): Two first are write info, two latter are copy info
            vkUpdateDescriptorSets(logicalDevice, descriptorWrites.count(), &descriptorWrites[0], 0, nullptr);
        }
    }

    void
    CreateCommandBuffers()
    {
        commandBuffers.resize(swapchainFramebuffers.size());

        VkCommandBufferAllocateInfo allocateInfo = {};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = commandPool;
        allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocateInfo.commandBufferCount = commandBuffers.size();

        if (vkAllocateCommandBuffers(logicalDevice, &allocateInfo, &commandBuffers[0]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffers");
        }

        std::cout << "Allocated command buffers\n";

        for (int i = 0; i < commandBuffers.size(); ++i)
        {
            VkCommandBufferBeginInfo beginInfo = {};

            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            beginInfo.pInheritanceInfo = nullptr;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to begin recording command buffer");
            }

            std::cout << "Started recording command buffers\n";
    
            VkRenderPassBeginInfo renderPassInfo = {};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapchainFramebuffers[i];
            
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapchainExtent;

            // Todo(Leo): These should also use constexpr ids or unscoped enums
            VkClearValue clearValues [2] = {};
            clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
            clearValues[1].depthStencil = {1.0f, 0};

            renderPassInfo.clearValueCount = 2;
            renderPassInfo.pClearValues = &clearValues [0];

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

            // Draw map           
            // uint32 mapOffset = 0;
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                                    0, 1, &descriptorSets[i], 1, &generatedMapUniformBufferOffset);
            VkDeviceSize offsets [] = {generatedMapBufferVertexOffset};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &generatedMapBuffer, offsets);
            vkCmdBindIndexBuffer(commandBuffers[i], generatedMapBuffer, 0, generatedMap.indexType);

            vkCmdDrawIndexed(commandBuffers[i], generatedMap.indices.size(), 1, 0, 0, 0);
            
            // Draw character
            // uint32 characterOffset = 256;
            vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                        0, 1, &descriptorSets[i], 1, &characterUniformBufferOffset);
            VkDeviceSize characterMeshOffsets [] = {characterBufferVertexOffset};
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, &characterBuffer, characterMeshOffsets);
            vkCmdBindIndexBuffer(commandBuffers[i], characterBuffer, 0, characterModel.indexType);

            vkCmdDrawIndexed(commandBuffers[i], characterModel.indices.size(), 1, 0, 0, 0);

            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to record command buffer");
            }
        }
    }

    void
    CreateSyncObjects()
    {
        imageAvailableSemaphores.resize(max_frames_in_flight);
        renderFinishedSemaphores.resize(max_frames_in_flight);
        inFlightFences.resize(max_frames_in_flight);

        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i <max_frames_in_flight; ++i)
        {
            if (
                vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
                || vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
                || vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create semaphores");
            }
        }

        std::cout << "Created semaphores\n";
    }

    void
    UpdateUniformBuffer(uint32 imageIndex)
    {
        // Note(Leo): mockup update logic to see uniformbuffers updating
        local_persist auto startTimeMark = std::chrono::high_resolution_clock::now();
        local_persist float lastTime = 0;
        local_persist float zPosition = 0;

        auto currentTimeMark = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();

        float dt = time - lastTime;
        lastTime = time;
        // worldCamera.target.x += dt;
        zPosition -= dt;
        // </mockup>

        // Todo(Leo): Single mapping is really enough, offsets can be used here too
        glm::mat4 * pModelMatrix;
        vkMapMemory(logicalDevice, modelUniformBufferMemory,
                    modelUniformBufferOffsets[imageIndex],
                    sizeof(pModelMatrix), 0, (void**)&pModelMatrix);

        *pModelMatrix = glm::translate(glm::mat4(1), glm::vec3(0, 0, zPosition));

        vkUnmapMemory(logicalDevice, modelUniformBufferMemory);

        vkMapMemory(logicalDevice, modelUniformBufferMemory,
                    modelUniformBufferOffsets[imageIndex] + characterUniformBufferOffset,
                    sizeof(pModelMatrix), 0, (void**)&pModelMatrix);
        *pModelMatrix = glm::translate(glm::mat4(1), glm::vec3(0, 0, -zPosition));
        vkUnmapMemory(logicalDevice, modelUniformBufferMemory); 

        // Note (Leo): map vulkan memory directly to right type so we can easily avoid one (small) memcpy per frame
        CameraProjectionsUniformBufferObject * pUbo;
        vkMapMemory(logicalDevice, sceneUniformBufferMemory,
                    sceneUniformBufferOffsets[imageIndex],
                    sizeof(CameraProjectionsUniformBufferObject), 0, (void**)&pUbo);

        pUbo->view          = worldCamera.GetViewProjection();
        pUbo->perspective   = worldCamera.GetPerspectiveProjection();
        
        vkUnmapMemory(logicalDevice, sceneUniformBufferMemory);
    }

    void
    DrawFrame()
    {
        vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrameIndex], VK_TRUE, std::numeric_limits<uint64>::max());

        uint32 imageIndex;
        VkResult result = vkAcquireNextImageKHR(
            logicalDevice,
            swapchain,
            std::numeric_limits<uint64>::max(),
            imageAvailableSemaphores[currentFrameIndex],
            VK_NULL_HANDLE, // Note(Leo): here would be fence
            &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            // Note(Leo): While VK_SUBOPTIMAL_KHR is success, it could be put here
            RecreateSwapchain();
            return;
        }
        else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swap chain image");
        }

        UpdateUniformBuffer(imageIndex);

        VkSubmitInfo submitInfo[1] = {};
        submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores [] = { imageAvailableSemaphores[currentFrameIndex] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo[0].waitSemaphoreCount = 1;
        submitInfo[0].pWaitSemaphores = waitSemaphores;
        submitInfo[0].pWaitDstStageMask = waitStages;

        submitInfo[0].commandBufferCount = 1;
        submitInfo[0].pCommandBuffers = &commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrameIndex] };
        submitInfo[0].signalSemaphoreCount = ARRAY_COUNT(signalSemaphores);
        submitInfo[0].pSignalSemaphores = signalSemaphores;

        /* Note(Leo): reset here, so that if we have recreated swapchain above,
        our fences will be left in signaled state */
        vkResetFences(logicalDevice, 1, &inFlightFences[currentFrameIndex]);

        if (vkQueueSubmit(graphicsQueue, 1, submitInfo, inFlightFences[currentFrameIndex]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to submit draw command buffer");
        }

        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = ARRAY_COUNT(signalSemaphores);
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapchains [] = { swapchain };
        presentInfo.swapchainCount = ARRAY_COUNT(swapchains);
        presentInfo.pSwapchains = swapchains;
        presentInfo.pImageIndices = &imageIndex; // Todo(Leo): also an array, one for each swapchain

        presentInfo.pResults = nullptr;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        // Note(Leo): Do after present to make sure semaphores behave correctly
        if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
        {
            framebufferResized = false;
            RecreateSwapchain();
        }
        else if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to present swapchain");
        }
        currentFrameIndex++;
    }

    void
    RecreateSwapchain()
    {
        int width = 0;
        int height = 0;
        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(window, &width, &height);
            glfwWaitEvents();
        }


        vkDeviceWaitIdle(logicalDevice);

        CleanupSwapchain();

        CreateSwapchainAndImages();
        CreateSwapchainImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateColorResources();
        CreateDepthResources();
        CreateFrameBuffers();
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();
        CreateCommandBuffers();

        worldCamera.aspectRatio = (float)swapchainExtent.width / (float)swapchainExtent.height;
    }
};

int main()
{
    MazegameApplication app;

    try {
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Vulkan shut down succesfully\n";
    return EXIT_SUCCESS;
}