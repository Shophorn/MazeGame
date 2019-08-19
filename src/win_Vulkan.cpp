/*=============================================================================
Leo Tamminen

Implementations of vulkan related functions
=============================================================================*/
#include "win_Vulkan.hpp"

internal uint32
Vulkan::FindMemoryType (VkPhysicalDevice physicalDevice, uint32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties (physicalDevice, &memoryProperties);

    for (int i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool32 filterMatch = (typeFilter & (1 << i)) != 0;
        bool32 memoryTypeOk = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (filterMatch && memoryTypeOk)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}   

// Todo(Leo): see if we can change this to proper functional function
internal void
Vulkan::CreateBuffer(
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
    allocateInfo.memoryTypeIndex = Vulkan::FindMemoryType(
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

internal void
Vulkan::CreateBufferResource(
    VkDevice                logicalDevice,
    VkPhysicalDevice        physicalDevice,
    VkDeviceSize            size,
    VkBufferUsageFlags      usage,
    VkMemoryPropertyFlags   memoryProperties,
    VulkanBufferResource *  result
){

    if (result->created == true)
    {
        // TODO(Leo): ASSERT and log
        return;
    }
    
    // TODO(Leo): inline this
    Vulkan::CreateBuffer(   logicalDevice, physicalDevice, size,
                            usage, memoryProperties, &result->buffer, &result->memory);

    result->size = size;
    result->created = true;
}

internal void
Vulkan::DestroyBufferResource(VkDevice logicalDevice, VulkanBufferResource * resource)
{
    vkDestroyBuffer(logicalDevice, resource->buffer, nullptr);
    vkFreeMemory(logicalDevice, resource->memory, nullptr);

    resource->created = false;
}

internal VkVertexInputBindingDescription
Vulkan::GetVertexBindingDescription ()
{
	VkVertexInputBindingDescription value = {};

	value.binding = 0;
	value.stride = sizeof(Vertex);
	value.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Note(Leo): Other option here has to do with instancing

	return value;	
}

internal Array<VkVertexInputAttributeDescription, 4>
Vulkan::GetVertexAttributeDescriptions()
{
	Array<VkVertexInputAttributeDescription, 4> value = {};

	value[0].binding = 0;
	value[0].location = 0;
	value[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	value[0].offset = offsetof(Vertex, position);

	value[1].binding = 0;
	value[1].location = 1;
	value[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	value[1].offset = offsetof(Vertex, normal);

	value[2].binding = 0;
	value[2].location = 2;
	value[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	value[2].offset = offsetof(Vertex, color);	

	value[3].binding = 0;
	value[3].location = 3;
	value[3].format = VK_FORMAT_R32G32_SFLOAT;
	value[3].offset = offsetof(Vertex, texCoord);

	return value;
}	

internal VkIndexType
Vulkan::ConvertIndexType(IndexType type)
{
	switch (type)
	{
		case IndexType::UInt16:
			return VK_INDEX_TYPE_UINT16;

		case IndexType::UInt32:
			return VK_INDEX_TYPE_UINT32;

		default:
			return VK_INDEX_TYPE_NONE_NV;
	};
}

internal VulkanSwapchainSupportDetails
Vulkan::QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VulkanSwapchainSupportDetails result = {};

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

internal VkSurfaceFormatKHR
Vulkan::ChooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& availableFormats)
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

internal VkPresentModeKHR
Vulkan::ChooseSurfacePresentMode(std::vector<VkPresentModeKHR> & availablePresentModes)
{
    // Todo(Leo): Is it really preferred???
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


VulkanQueueFamilyIndices
Vulkan::FindQueueFamilies (VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // Note: A card must also have a graphics queue family available; We do want to draw after all
    VkQueueFamilyProperties queueFamilyProps [50];
    uint32 queueFamilyCount = ARRAY_COUNT(queueFamilyProps);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps);

    // std::cout << "queueFamilyCount: " << queueFamilyCount << "\n";

    bool32 properQueueFamilyFound = false;
    VulkanQueueFamilyIndices result = {};
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
Vulkan::CreateShaderModule(BinaryAsset code, VkDevice logicalDevice)
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


/******************************************************************************

Todo[Vulkan](Leo):
AFTER THIS THESE FUNCTIONS ARE JUST YEETED HERE, SET THEM PROPERLY IN 'Vulkan'
NAMESPACE AND SPLIT TO MULTIPLE FILES IF NECEESSARY


*****************************************************************************/

/* Todo(Leo): Define a proper struct (of which size is to be used) to
make it easier to change later. */
internal uint32
GetModelUniformBufferOffsetForSwapchainImages(VulkanContext * context, int32 imageIndex)
{
    uint32 memorySizePerModelMatrix = AlignUpTo(
        context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
        sizeof(Matrix44));

    uint32 result = imageIndex * memorySizePerModelMatrix * MAX_MODEL_COUNT;   
    return result;
}

internal uint32
GetSceneUniformBufferOffsetForSwapchainImages(VulkanContext * context, int32 imageIndex)
{
    uint32 memorySizePerObject = AlignUpTo(
        context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
        sizeof(VulkanCameraUniformBufferObject));

    // Note(Leo): so far we only have on of these
    uint32 result = imageIndex * memorySizePerObject;
    return result;
}

internal VkSampleCountFlagBits 
GetMaxUsableMsaaSampleCount (VkPhysicalDevice physicalDevice)
{
    // Todo(Leo): to be easier on machine when developing for 2 players at same time
    return VK_SAMPLE_COUNT_1_BIT;

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
}

internal VkInstance
CreateInstance()
{
    auto CheckValidationLayerSupport = [] () -> bool32
    {
        VkLayerProperties availableLayers [50];
        uint32 availableLayersCount = ARRAY_COUNT(availableLayers);

        bool32 result = true;
        if (vkEnumerateInstanceLayerProperties (&availableLayersCount, availableLayers) == VK_SUCCESS)
        {

            for (
                int validationLayerIndex = 0;
                validationLayerIndex < Vulkan::VALIDATION_LAYERS_COUNT;
                ++validationLayerIndex)
            {
                bool32 layerFound = false;
                for(
                    int availableLayerIndex = 0; 
                    availableLayerIndex < availableLayersCount; 
                    ++availableLayerIndex)
                {
                    if (strcmp (Vulkan::validationLayers[validationLayerIndex], availableLayers[availableLayerIndex].layerName) == 0)
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

    if (Vulkan::enableValidationLayers && CheckValidationLayerSupport() == false)
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

    if constexpr (Vulkan::enableValidationLayers)
    {
        instanceInfo.enabledLayerCount = Vulkan::VALIDATION_LAYERS_COUNT;
        instanceInfo.ppEnabledLayerNames = Vulkan::validationLayers;
    }
    else
    {
        instanceInfo.enabledLayerCount = 0;
    }

    VkInstance instance;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        // Todo: Bad stuff happened, abort
    }

    return instance;
    // Todo(Leo): check if glfw extensions are found
    // VkExtensionProperties extensions [50];
    // uint32 extensionCount = ARRAY_COUNT(extensions);
    // vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions); 
}

internal VkPhysicalDevice
PickPhysicalDevice(VkInstance vulkanInstance, VkSurfaceKHR surface)
{
    auto CheckDeviceExtensionSupport = [] (VkPhysicalDevice testDevice) -> bool32
    {
        VkExtensionProperties availableExtensions [100];
        uint32 availableExtensionsCount = ARRAY_COUNT(availableExtensions);
        vkEnumerateDeviceExtensionProperties (testDevice, nullptr, &availableExtensionsCount, availableExtensions);

        bool32 result = true;
        for (int requiredIndex = 0;
            requiredIndex < Vulkan::DEVICE_EXTENSION_COUNT;
            ++requiredIndex)
        {

            bool32 requiredExtensionFound = false;
            for (int availableIndex = 0;
                availableIndex < availableExtensionsCount;
                ++availableIndex)
            {
                if (strcmp(Vulkan::deviceExtensions[requiredIndex], availableExtensions[availableIndex].extensionName) == 0)
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

    auto IsPhysicalDeviceSuitable = [CheckDeviceExtensionSupport] (VkPhysicalDevice testDevice, VkSurfaceKHR surface) -> bool32
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(testDevice, &props);
        bool32 isDedicatedGPU = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(testDevice, &features);

        bool32 extensionsAreSupported = CheckDeviceExtensionSupport(testDevice);

        bool32 swapchainIsOk = false;
        if (extensionsAreSupported)
        {
            VulkanSwapchainSupportDetails swapchainSupport = Vulkan::QuerySwapChainSupport(testDevice, surface);
            swapchainIsOk = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
        }

        VulkanQueueFamilyIndices indices = Vulkan::FindQueueFamilies(testDevice, surface);
        return  isDedicatedGPU 
                && indices.hasAll()
                && extensionsAreSupported
                && swapchainIsOk
                && features.samplerAnisotropy;
    };

    VkPhysicalDevice resultPhysicalDevice;


    VkPhysicalDevice devices [10];
    uint32 deviceCount = ARRAY_COUNT(devices);
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices);

    if (deviceCount == 0)
    {
        throw std::runtime_error("No GPU devices found");
    }

    for (int i = 0; i < deviceCount; i++)
    {
        if (IsPhysicalDeviceSuitable(devices[i], surface))
        {
            resultPhysicalDevice = devices[i];
            break;
        }
    }

    if (resultPhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable GPU device found");
    }

    return resultPhysicalDevice;
}

internal VkDevice
CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VulkanQueueFamilyIndices queueIndices = Vulkan::FindQueueFamilies(physicalDevice, surface);
    
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
    deviceCreateInfo.enabledExtensionCount = Vulkan::DEVICE_EXTENSION_COUNT;
    deviceCreateInfo.ppEnabledExtensionNames = Vulkan::deviceExtensions;

    if constexpr (Vulkan::enableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount = Vulkan::VALIDATION_LAYERS_COUNT;
        deviceCreateInfo.ppEnabledLayerNames = Vulkan::validationLayers;
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    VkDevice resultLogicalDevice;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo,nullptr, &resultLogicalDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vulkan logical device");
    }

    return resultLogicalDevice;
}

internal VkImage
CreateImage(
    VkDevice logicalDevice, 
    VkPhysicalDevice physicalDevice,
    uint32 texWidth,
    uint32 texHeight,
    uint32 mipLevels,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkSampleCountFlagBits msaaSamples
){
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE; // note(leo): concerning queue families
    imageInfo.arrayLayers   = 1;

    imageInfo.extent        = { texWidth, texHeight, 1 };
    imageInfo.mipLevels     = mipLevels;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.usage         = usage;
    imageInfo.samples       = msaaSamples;

    imageInfo.flags         = 0;

    VkImage resultImage;
    if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &resultImage) != VK_SUCCESS)
    {
        throw std::runtime_error ("Failed to create image");
    }
    return resultImage;
}

internal void
CreateImageAndMemory(
    VkDevice logicalDevice, 
    VkPhysicalDevice physicalDevice,
    uint32 texWidth,
    uint32 texHeight,
    uint32 mipLevels,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    VkSampleCountFlagBits msaaSamples,
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
    imageInfo.samples = msaaSamples;
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
    allocateInfo.memoryTypeIndex = Vulkan::FindMemoryType (
                                    physicalDevice,
                                    memoryRequirements.memoryTypeBits,
                                    memoryFlags);

    if (vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, resultImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory");
    }

    vkBindImageMemory(logicalDevice, *resultImage, *resultImageMemory, 0);   
}


internal VkImageView
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


internal VulkanSwapchainItems
CreateSwapchainAndImages(VkPhysicalDevice physicalDevice, VkDevice logicalDevice,VkSurfaceKHR surface, GLFWwindow * window)
{
    VulkanSwapchainItems resultSwapchain = {};


    VulkanSwapchainSupportDetails swapchainSupport = Vulkan::QuerySwapChainSupport(physicalDevice, surface);

    VkSurfaceFormatKHR surfaceFormat = Vulkan::ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = Vulkan::ChooseSurfacePresentMode(swapchainSupport.presentModes);

    // Find extent ie. size of drawing window
    /* Note(Leo): max value is special value denoting that all are okay.
    Or something else, so we need to ask platform */
    if (swapchainSupport.capabilities.currentExtent.width != MaxValue<uint32>)
    {
        resultSwapchain.extent = swapchainSupport.capabilities.currentExtent;
    }
    else
    {   
        int32 width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D min = swapchainSupport.capabilities.minImageExtent;
        VkExtent2D max = swapchainSupport.capabilities.maxImageExtent;

        resultSwapchain.extent.width = Clamp(static_cast<uint32>(width), min.width, max.width);
        resultSwapchain.extent.height = Clamp(static_cast<uint32>(height), min.height, max.height);
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
    createInfo.imageExtent = resultSwapchain.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VulkanQueueFamilyIndices queueIndices = Vulkan::FindQueueFamilies(physicalDevice, surface);
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

    if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &resultSwapchain.swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain");
    }

    resultSwapchain.imageFormat = surfaceFormat.format;


    // Bug(Leo): following imageCount variable is reusing one from previous definition, maybe bug
    // Note(Leo): Swapchain images are not created, they are gotten from api
    vkGetSwapchainImagesKHR(logicalDevice, resultSwapchain.swapchain, &imageCount, nullptr);
    resultSwapchain.images.resize (imageCount);
    vkGetSwapchainImagesKHR(logicalDevice, resultSwapchain.swapchain, &imageCount, &resultSwapchain.images[0]);

    std::cout << "Created swapchain and images\n";
    
    resultSwapchain.imageViews.resize(imageCount);

    for (int i = 0; i < imageCount; ++i)
    {
        resultSwapchain.imageViews[i] = CreateImageView(
            logicalDevice, resultSwapchain.images[i], 1, 
            resultSwapchain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    std::cout << "Created image views\n";

    return resultSwapchain;
}

internal VkFormat
FindSupportedFormat(
    VkPhysicalDevice physicalDevice,
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

internal VkFormat
FindSupportedDepthFormat(VkPhysicalDevice physicalDevice)
{
    VkFormat formats [] = { VK_FORMAT_D32_SFLOAT,
                            VK_FORMAT_D32_SFLOAT_S8_UINT,
                            VK_FORMAT_D24_UNORM_S8_UINT };
    int32 formatCount = 3;


    VkFormat result = FindSupportedFormat(
                        physicalDevice, formatCount, formats, VK_IMAGE_TILING_OPTIMAL, 
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    return result;
}

internal VkRenderPass
CreateRenderPass(VkDevice logicalDevice, VkPhysicalDevice physicalDevice, VulkanSwapchainItems * swapchainItems, VkSampleCountFlagBits msaaSamples)
{
    constexpr int 
        color_attachment_id     = 0,
        depth_attachment_id     = 1,
        resolve_attachments_id  = 2,
        attachment_count        = 3;

    VkAttachmentDescription attachments[attachment_count] = {};

    /*
    Note(Leo): We render internally to color attachment and depth attachment
    using multisampling. After that final image is compiled to 'resolve'
    attachment that is image from swapchain and present that
    */
    attachments[color_attachment_id].format         = swapchainItems->imageFormat;
    attachments[color_attachment_id].samples        = msaaSamples;
    attachments[color_attachment_id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[color_attachment_id].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[color_attachment_id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[color_attachment_id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[color_attachment_id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[color_attachment_id].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[depth_attachment_id].format         = FindSupportedDepthFormat(physicalDevice);
    attachments[depth_attachment_id].samples        = msaaSamples;
    attachments[depth_attachment_id].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[depth_attachment_id].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[depth_attachment_id].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[depth_attachment_id].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[depth_attachment_id].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[depth_attachment_id].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachments[resolve_attachments_id].format         = swapchainItems->imageFormat;
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

    VkRenderPass resultRenderPass;
    if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &resultRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass");
    }

    return resultRenderPass;
}

/* Note(leo): these are fixed per available renderpipeline thing. There describe
'kinds' of resources or something, so their amount does not change */
constexpr static int32
    DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID = 0,
    DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID = 2,
    DESCRIPTOR_SAMPLER_ID              = 1;
constexpr static int32 DESCRIPTOR_SET_COUNT = 3;


internal VkDescriptorSetLayout
CreateDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding layoutBindings [DESCRIPTOR_SET_COUNT] = {};

    // UNIFORM BUFFER OBJECT, camera projection matrices for now
    layoutBindings[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].binding = DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID;
    layoutBindings[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].descriptorCount = 1;
    layoutBindings[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].pImmutableSamplers = nullptr; // Note(Leo): relevant for sampler stuff, like textures

    // SAMPLER
    layoutBindings[DESCRIPTOR_SAMPLER_ID].binding = DESCRIPTOR_SAMPLER_ID;
    layoutBindings[DESCRIPTOR_SAMPLER_ID].descriptorCount = 1;
    layoutBindings[DESCRIPTOR_SAMPLER_ID].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[DESCRIPTOR_SAMPLER_ID].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[DESCRIPTOR_SAMPLER_ID].pImmutableSamplers = nullptr;

    // MODEL TRANSFORM BUFFES
    layoutBindings[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].binding = DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID;
    layoutBindings[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBindings[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].descriptorCount = 1;
    layoutBindings[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutCreateInfo.bindingCount = DESCRIPTOR_SET_COUNT;
    layoutCreateInfo.pBindings = &layoutBindings[0];

    VkDescriptorSetLayout resultDescriptorSetLayout;

    if(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create the descriptor set layout");
    }

    return resultDescriptorSetLayout;
}

internal VulkanPipelineItems
CreateGraphicsPipeline(
    VkDevice device,
    VkDescriptorSetLayout descriptorSetLayout,
    VkRenderPass renderPass,
    VulkanSwapchainItems * swapchainItems,
    VkSampleCountFlagBits msaaSamples
){

    VulkanPipelineItems resultPipelineItems = {};

    // Todo(Leo): unhardcode these
    BinaryAsset vertexShaderCode = ReadBinaryFile("shaders/vert.spv");
    BinaryAsset fragmentShaderCode = ReadBinaryFile("shaders/frag.spv");

    VkShaderModule vertexShaderModule = Vulkan::CreateShaderModule(vertexShaderCode, device);
    VkShaderModule fragmentShaderModule = Vulkan::CreateShaderModule(fragmentShaderCode, device);

    constexpr uint32 
        VERTEX_STAGE_ID     = 0,
        FRAGMENT_STAGE_ID   = 1,
        SHADER_STAGE_COUNT  = 2;
    VkPipelineShaderStageCreateInfo shaderStages [SHADER_STAGE_COUNT] = {};

    shaderStages[VERTEX_STAGE_ID].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[VERTEX_STAGE_ID].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[VERTEX_STAGE_ID].module = vertexShaderModule;
    shaderStages[VERTEX_STAGE_ID].pName = "main";

    shaderStages[FRAGMENT_STAGE_ID].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[FRAGMENT_STAGE_ID].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[FRAGMENT_STAGE_ID].module = fragmentShaderModule;
    shaderStages[FRAGMENT_STAGE_ID].pName = "main";

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    auto bindingDescription = Vulkan::GetVertexBindingDescription();
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    
    auto attributeDescriptions = Vulkan::GetVertexAttributeDescriptions();
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.count();
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptions[0];

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchainItems->extent.width;
    viewport.height = (float) swapchainItems->extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainItems->extent;

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

    // Note: This attachment is per framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =   VK_COLOR_COMPONENT_R_BIT 
                                            | VK_COLOR_COMPONENT_G_BIT 
                                            | VK_COLOR_COMPONENT_B_BIT 
                                            | VK_COLOR_COMPONENT_A_BIT;
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

    // Todo(Leo): does this need to be returned
    if (vkCreatePipelineLayout (device, &pipelineLayoutInfo, nullptr, &resultPipelineItems.layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = SHADER_STAGE_COUNT;
    pipelineInfo.pStages = shaderStages;

    // Note: Fixed-function stages???
    pipelineInfo.pVertexInputState      = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState    = &inputAssembly;
    pipelineInfo.pViewportState         = &viewportState;
    pipelineInfo.pRasterizationState    = &rasterizer;
    pipelineInfo.pMultisampleState      = &multisampling;
    pipelineInfo.pDepthStencilState     = &depthStencil;
    pipelineInfo.pColorBlendState       = &colorBlending;
    pipelineInfo.pDynamicState          = nullptr;

    pipelineInfo.layout = resultPipelineItems.layout;

    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;

    // Note: These are for cheaper re-use of pipelines, not used right now
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;


    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &resultPipelineItems.pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    // Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);

    return resultPipelineItems;
}

internal VulkanSyncObjects
CreateSyncObjects(VkDevice device)
{
    VulkanSyncObjects resultSyncObjects = {};

    resultSyncObjects.imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    resultSyncObjects.renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    resultSyncObjects.inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i <MAX_FRAMES_IN_FLIGHT; ++i)
    {
        if (
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &resultSyncObjects.imageAvailableSemaphores[i]) != VK_SUCCESS
            || vkCreateSemaphore(device, &semaphoreInfo, nullptr, &resultSyncObjects.renderFinishedSemaphores[i]) != VK_SUCCESS
            || vkCreateFence(device, &fenceInfo, nullptr, &resultSyncObjects.inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create semaphores");
        }
    }

    std::cout << "Created semaphores\n";

    return resultSyncObjects;
}


// Change image layout from stored pixel array layout to device optimal layout
internal void
TransitionImageLayout(
    VkDevice device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VkImage image,
    VkFormat format,
    uint32 mipLevels,
    VkImageLayout oldLayout,
    VkImageLayout newLayout
){
    VkCommandBuffer commandBuffer = Vulkan::BeginOneTimeCommandBuffer(device, commandPool);

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
        if (Vulkan::FormatHasStencilComponent(format))
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

    Vulkan::EndOneTimeCommandBuffer (device, commandPool, graphicsQueue, commandBuffer);
}


internal VulkanDrawingResources
CreateDrawingResources(
    VkDevice device,
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    VulkanSwapchainItems * swapchainItems,
    VkSampleCountFlagBits msaaSamples)
{
    VulkanDrawingResources resultResources = {};

    VkFormat colorFormat = swapchainItems->imageFormat;
    resultResources.colorImage = CreateImage(  device, physicalDevice,
                                                swapchainItems->extent.width, swapchainItems->extent.height,
                                                1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                                                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                msaaSamples);

    VkFormat depthFormat = FindSupportedDepthFormat(physicalDevice);
    resultResources.depthImage = CreateImage(  device, physicalDevice,
                                                swapchainItems->extent.width, swapchainItems->extent.height,
                                                1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                msaaSamples);

    VkMemoryRequirements colorMemoryRequirements, depthMemoryRequirements;
    vkGetImageMemoryRequirements(device, resultResources.colorImage, &colorMemoryRequirements);
    vkGetImageMemoryRequirements(device, resultResources.depthImage, &depthMemoryRequirements);


    // Todo(Leo): Do these need to be aligned like below????
    uint64 colorMemorySize = colorMemoryRequirements.size;
    uint64 depthMemorySize = depthMemoryRequirements.size;
    // uint64 colorMemorySize = AlignUpTo(colorMemoryRequirements.alignment, colorMemoryRequirements.size);
    // uint64 depthMemorySize = AlignUpTo(depthMemoryRequirements.alignment, depthMemoryRequirements.size);

    uint32 memoryTypeIndex = Vulkan::FindMemoryType(
                                physicalDevice,
                                colorMemoryRequirements.memoryTypeBits | depthMemoryRequirements.memoryTypeBits,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = (colorMemorySize + depthMemorySize);
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &resultResources.memory))
    {
        throw std::runtime_error("Failed to allocate drawing resource memory");
    }            

    // Todo(Leo): Result check binding memories
    vkBindImageMemory(device, resultResources.colorImage, resultResources.memory, 0);
    vkBindImageMemory(device, resultResources.depthImage, resultResources.memory, colorMemorySize);

    resultResources.colorImageView = CreateImageView(  device, resultResources.colorImage,
                                                        1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    resultResources.depthImageView = CreateImageView(  device, resultResources.depthImage,
                                                        1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    TransitionImageLayout(  device, commandPool, graphicsQueue, resultResources.colorImage, colorFormat, 1,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    TransitionImageLayout(  device, commandPool, graphicsQueue, resultResources.depthImage, depthFormat, 1,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    return resultResources;
}

internal VkDescriptorPool
CreateDescriptorPool(VkDevice device, int32 swapchainImageCount)
{
    /*
    Note(Leo): 
    There needs to only one per type, not one per user

    We create a single big buffer and then use offsets to divide it to smaller chunks
    */

    // int32 imageCount = swapchainItems.images.size();

    VkDescriptorPoolSize poolSizes [DESCRIPTOR_SET_COUNT] = {};

    poolSizes[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].descriptorCount = swapchainImageCount;

    poolSizes[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].descriptorCount = swapchainImageCount; 

    poolSizes[DESCRIPTOR_SAMPLER_ID].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[DESCRIPTOR_SAMPLER_ID].descriptorCount = swapchainImageCount;

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = DESCRIPTOR_SET_COUNT;
    poolInfo.pPoolSizes = &poolSizes[0];
    poolInfo.maxSets = swapchainImageCount;

    MAZEGAME_NO_INIT VkDescriptorPool resultDescriptorPool;
    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &resultDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create descriptor pool");
    }
    return resultDescriptorPool;
}

    internal std::vector<VkDescriptorSet>
    CreateDescriptorSets(
        VkDevice device,
        VkDescriptorPool descriptorPool,
        VkDescriptorSetLayout descriptorSetLayout,
        VulkanSwapchainItems * swapchainItems,
        VulkanBufferResource * sceneUniformBuffer,
        VulkanBufferResource * modelUniformBuffer,
        VkImageView textureImageView,
        VkSampler textureSampler,
        VulkanContext * context)
    {
        /* Note(Leo): Create vector of [imageCount] copies from descriptorSetLayout
        for allocation */
        int imageCount = swapchainItems->images.size();
        std::vector<VkDescriptorSetLayout> layouts (imageCount, descriptorSetLayout);

        VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocateInfo.descriptorPool = descriptorPool;
        allocateInfo.descriptorSetCount = imageCount;
        allocateInfo.pSetLayouts = &layouts[0];

        // descriptorSets.resize (imageCount);
        std::vector<VkDescriptorSet> resultDescriptorSets(imageCount);
        if (vkAllocateDescriptorSets(device, &allocateInfo, &resultDescriptorSets[0]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
        }

        for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex)
        {
            VkWriteDescriptorSet descriptorWrites [DESCRIPTOR_SET_COUNT] = {};

            // SCENE UNIFORM BUFFER
            VkDescriptorBufferInfo sceneBufferInfo = {};
            sceneBufferInfo.buffer = sceneUniformBuffer->buffer;
            sceneBufferInfo.offset = GetSceneUniformBufferOffsetForSwapchainImages(context, imageIndex);
            sceneBufferInfo.range = sizeof(VulkanCameraUniformBufferObject);

            descriptorWrites[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].dstSet = resultDescriptorSets[imageIndex];
            descriptorWrites[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].dstBinding = DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID;
            descriptorWrites[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].dstArrayElement = 0;
            descriptorWrites[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].descriptorCount = 1;
            descriptorWrites[DESCRIPTOR_SCENE_UNIFORM_BUFFER_ID].pBufferInfo = &sceneBufferInfo;

            // MODEL UNIFORM BUFFERS
            VkDescriptorBufferInfo modelBufferInfo = {};
            modelBufferInfo.buffer = modelUniformBuffer->buffer;
            modelBufferInfo.offset = GetModelUniformBufferOffsetForSwapchainImages(context, imageIndex);
            modelBufferInfo.range = sizeof(Matrix44);

            descriptorWrites[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].dstSet = resultDescriptorSets[imageIndex];
            descriptorWrites[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].dstBinding = DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID;
            descriptorWrites[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].dstArrayElement = 0;
            descriptorWrites[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            descriptorWrites[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].descriptorCount = 1;
            descriptorWrites[DESCRIPTOR_MODEL_UNIFORM_BUFFER_ID].pBufferInfo = &modelBufferInfo;

            // MOCKUP: IMAGE DESCRIPTOR_SAMPLER_ID
            VkDescriptorImageInfo imageInfo = {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = textureImageView;
            imageInfo.sampler = textureSampler;

            descriptorWrites[DESCRIPTOR_SAMPLER_ID].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[DESCRIPTOR_SAMPLER_ID].dstSet = resultDescriptorSets[imageIndex];
            descriptorWrites[DESCRIPTOR_SAMPLER_ID].dstBinding = DESCRIPTOR_SAMPLER_ID;
            descriptorWrites[DESCRIPTOR_SAMPLER_ID].dstArrayElement = 0;
            descriptorWrites[DESCRIPTOR_SAMPLER_ID].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[DESCRIPTOR_SAMPLER_ID].descriptorCount = 1;
            descriptorWrites[DESCRIPTOR_SAMPLER_ID].pImageInfo = &imageInfo;

            // Note(Leo): Two first are write info, two latter are copy info
            vkUpdateDescriptorSets(device, DESCRIPTOR_SET_COUNT, &descriptorWrites[0], 0, nullptr);
        }

        return resultDescriptorSets;
    }
