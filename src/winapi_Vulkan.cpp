/*=============================================================================
Leo Tamminen

Implementations of vulkan related functions
=============================================================================*/
#include "winapi_Vulkan.hpp"
#include "winapi_VulkanIGraphicsContext.cpp"

internal uint32
vulkan::FindMemoryType (VkPhysicalDevice physicalDevice, uint32 typeFilter, VkMemoryPropertyFlags properties)
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
vulkan::CreateBuffer(
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
    allocateInfo.memoryTypeIndex = vulkan::FindMemoryType(
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
vulkan::CreateBufferResource(
    VulkanContext *         context,
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
    vulkan::CreateBuffer(   context->device, context->physicalDevice, size,
                            usage, memoryProperties, &result->buffer, &result->memory);

    result->size = size;
    result->created = true;
}

internal void
vulkan::DestroyBufferResource(VkDevice logicalDevice, VulkanBufferResource * resource)
{
    vkDestroyBuffer(logicalDevice, resource->buffer, nullptr);
    vkFreeMemory(logicalDevice, resource->memory, nullptr);

    resource->created = false;
}

internal VkVertexInputBindingDescription
vulkan::GetVertexBindingDescription ()
{
	VkVertexInputBindingDescription value = {};

	value.binding = 0;
	value.stride = sizeof(Vertex);
	value.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // Note(Leo): Other option here has to do with instancing

	return value;	
}

internal Array<VkVertexInputAttributeDescription, 4>
vulkan::GetVertexAttributeDescriptions()
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
vulkan::ConvertIndexType(IndexType type)
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
vulkan::QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
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
vulkan::ChooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& availableFormats)
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
vulkan::ChooseSurfacePresentMode(std::vector<VkPresentModeKHR> & availablePresentModes)
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
vulkan::FindQueueFamilies (VkPhysicalDevice device, VkSurfaceKHR surface)
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
vulkan::CreateShaderModule(BinaryAsset code, VkDevice logicalDevice)
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
AFTER THIS THESE FUNCTIONS ARE JUST YEETED HERE, SET THEM PROPERLY IN 'vulkan'
NAMESPACE AND SPLIT TO MULTIPLE FILES IF APPROPRIATE


*****************************************************************************/


internal VkSampleCountFlagBits 
GetMaxUsableMsaaSampleCount (VkPhysicalDevice physicalDevice)
{
    // Todo(Leo): to be easier on machine when developing for 2 players at same time
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = std::min(
            physicalDeviceProperties.limits.framebufferColorSampleCounts,
            physicalDeviceProperties.limits.framebufferDepthSampleCounts
        );

    VkSampleCountFlagBits result;

    if (counts & VK_SAMPLE_COUNT_64_BIT) { result = VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { result = VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { result = VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { result = VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { result = VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { result = VK_SAMPLE_COUNT_2_BIT; }

    if (result > VULKAN_MAX_MSAA_SAMPLE_COUNT)
    {
        result = VULKAN_MAX_MSAA_SAMPLE_COUNT;
    }

    return result;
}

// Todo(Leo): put into namespace, I already forgot where this was :)
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
                validationLayerIndex < vulkan::VALIDATION_LAYERS_COUNT;
                ++validationLayerIndex)
            {
                bool32 layerFound = false;
                for(
                    int availableLayerIndex = 0; 
                    availableLayerIndex < availableLayersCount; 
                    ++availableLayerIndex)
                {
                    if (strcmp (vulkan::validationLayers[validationLayerIndex], availableLayers[availableLayerIndex].layerName) == 0)
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

    if (vulkan::enableValidationLayers && CheckValidationLayerSupport() == false)
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

    instanceInfo.enabledExtensionCount = 2;
    const char * extensions [] = {"VK_KHR_surface", "VK_KHR_win32_surface"}; 
    instanceInfo.ppEnabledExtensionNames = extensions;

    if constexpr (vulkan::enableValidationLayers)
    {
        instanceInfo.enabledLayerCount = vulkan::VALIDATION_LAYERS_COUNT;
        instanceInfo.ppEnabledLayerNames = vulkan::validationLayers;
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
            requiredIndex < vulkan::DEVICE_EXTENSION_COUNT;
            ++requiredIndex)
        {

            bool32 requiredExtensionFound = false;
            for (int availableIndex = 0;
                availableIndex < availableExtensionsCount;
                ++availableIndex)
            {
                if (strcmp(vulkan::deviceExtensions[requiredIndex], availableExtensions[availableIndex].extensionName) == 0)
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
            VulkanSwapchainSupportDetails swapchainSupport = vulkan::QuerySwapChainSupport(testDevice, surface);
            swapchainIsOk = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
        }

        VulkanQueueFamilyIndices indices = vulkan::FindQueueFamilies(testDevice, surface);
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
    VulkanQueueFamilyIndices queueIndices = vulkan::FindQueueFamilies(physicalDevice, surface);
    
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
    deviceCreateInfo.enabledExtensionCount = vulkan::DEVICE_EXTENSION_COUNT;
    deviceCreateInfo.ppEnabledExtensionNames = vulkan::deviceExtensions;

    if constexpr (vulkan::enableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount = vulkan::VALIDATION_LAYERS_COUNT;
        deviceCreateInfo.ppEnabledLayerNames = vulkan::validationLayers;
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
    allocateInfo.memoryTypeIndex = vulkan::FindMemoryType (
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
CreateSwapchainAndImages(VulkanContext * context, VkExtent2D frameBufferSize)
{
    VulkanSwapchainItems resultSwapchain = {};

    VulkanSwapchainSupportDetails swapchainSupport = vulkan::QuerySwapChainSupport(context->physicalDevice, context->surface);

    VkSurfaceFormatKHR surfaceFormat = vulkan::ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = vulkan::ChooseSurfacePresentMode(swapchainSupport.presentModes);

    // Find extent ie. size of drawing window
    /* Note(Leo): max value is special value denoting that all are okay.
    Or something else, so we need to ask platform */
    if (swapchainSupport.capabilities.currentExtent.width != MaxValue<uint32>)
    {
        resultSwapchain.extent = swapchainSupport.capabilities.currentExtent;
    }
    else
    {   
        VkExtent2D min = swapchainSupport.capabilities.minImageExtent;
        VkExtent2D max = swapchainSupport.capabilities.maxImageExtent;

        resultSwapchain.extent.width = Clamp(static_cast<uint32>(frameBufferSize.width), min.width, max.width);
        resultSwapchain.extent.height = Clamp(static_cast<uint32>(frameBufferSize.height), min.height, max.height);
    }

    std::cout << "Creating swapchain actually, width: " << resultSwapchain.extent.width << ", height: " << resultSwapchain.extent.height << "\n";

    uint32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    uint32 maxImageCount = swapchainSupport.capabilities.maxImageCount;
    if (maxImageCount > 0 && imageCount > maxImageCount)
    {
        imageCount = maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = context->surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = resultSwapchain.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VulkanQueueFamilyIndices queueIndices = vulkan::FindQueueFamilies(context->physicalDevice, context->surface);
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

    if (vkCreateSwapchainKHR(context->device, &createInfo, nullptr, &resultSwapchain.swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain");
    }

    resultSwapchain.imageFormat = surfaceFormat.format;


    // Bug(Leo): following imageCount variable is reusing one from previous definition, maybe bug
    // Note(Leo): Swapchain images are not created, they are gotten from api
    vkGetSwapchainImagesKHR(context->device, resultSwapchain.swapchain, &imageCount, nullptr);
    resultSwapchain.images.resize (imageCount);
    vkGetSwapchainImagesKHR(context->device, resultSwapchain.swapchain, &imageCount, &resultSwapchain.images[0]);

    std::cout << "Created swapchain and images\n";
    
    resultSwapchain.imageViews.resize(imageCount);

    for (int i = 0; i < imageCount; ++i)
    {
        resultSwapchain.imageViews[i] = CreateImageView(
            context->device, resultSwapchain.images[i], 1, 
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
CreateRenderPass(VulkanContext * context, VulkanSwapchainItems * swapchainItems, VkSampleCountFlagBits msaaSamples)
{
    constexpr int 
        COLOR_ATTACHMENT_ID     = 0,
        DEPTH_ATTACHMENT_ID     = 1,
        RESOLVE_ATTACHMENT_ID   = 2,
        ATTACHMENT_COUNT        = 3;

    VkAttachmentDescription attachments[ATTACHMENT_COUNT] = {};

    /*
    Note(Leo): We render internally to color attachment and depth attachment
    using multisampling. After that final image is compiled to 'resolve'
    attachment that is image from swapchain and present that
    */
    attachments[COLOR_ATTACHMENT_ID].format         = swapchainItems->imageFormat;
    attachments[COLOR_ATTACHMENT_ID].samples        = msaaSamples;
    attachments[COLOR_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[COLOR_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[COLOR_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[COLOR_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[COLOR_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[COLOR_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[DEPTH_ATTACHMENT_ID].format         = FindSupportedDepthFormat(context->physicalDevice);
    attachments[DEPTH_ATTACHMENT_ID].samples        = msaaSamples;
    attachments[DEPTH_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[DEPTH_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[DEPTH_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[DEPTH_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[DEPTH_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[DEPTH_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachments[RESOLVE_ATTACHMENT_ID].format         = swapchainItems->imageFormat;
    attachments[RESOLVE_ATTACHMENT_ID].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[RESOLVE_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[RESOLVE_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[RESOLVE_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[RESOLVE_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[RESOLVE_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[RESOLVE_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    constexpr int32 COLOR_ATTACHMENT_COUNT = 1;        
    VkAttachmentReference colorAttachmentRefs[COLOR_ATTACHMENT_COUNT] = {};
    colorAttachmentRefs[0].attachment = COLOR_ATTACHMENT_ID;
    colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Note(Leo): there can be only one depth attachment
    VkAttachmentReference depthStencilAttachmentRef = {};
    depthStencilAttachmentRef.attachment = DEPTH_ATTACHMENT_ID;
    depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveAttachmentRefs [COLOR_ATTACHMENT_COUNT] = {};
    resolveAttachmentRefs[0].attachment = RESOLVE_ATTACHMENT_ID;
    resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpasses[1] = {};
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = COLOR_ATTACHMENT_COUNT;
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
    renderPassInfo.attachmentCount = ATTACHMENT_COUNT;
    renderPassInfo.pAttachments = &attachments[0];
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpasses[0];
    renderPassInfo.dependencyCount = ARRAY_COUNT(dependencies);
    renderPassInfo.pDependencies = &dependencies[0];

    VkRenderPass resultRenderPass;
    if (vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &resultRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass");
    }

    return resultRenderPass;
}

/* Note(leo): these are fixed per available renderpipeline thing. They describe
'kinds' of resources or something, so their amount does not change

IMPORTANT(Leo): These must be same in shaders
*/

constexpr static int32 DESCRIPTOR_LAYOUT_SCENE_BINDING = 0;
constexpr static int32 DESCRIPTOR_LAYOUT_MODEL_BINDING = 0;
constexpr static int32 DESCRIPTOR_LAYOUT_SAMPLER_BINDING = 0;

internal VkDescriptorSetLayout
CreateModelDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding layoutBindings [1] = {};

    layoutBindings[0].binding = DESCRIPTOR_LAYOUT_MODEL_BINDING;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutCreateInfo.bindingCount = 1;
    layoutCreateInfo.pBindings = &layoutBindings[0];

    VkDescriptorSetLayout resultDescriptorSetLayout;

    if(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create the descriptor set layout");
    }

    return resultDescriptorSetLayout;
}

internal VkDescriptorSetLayout
CreateMaterialDescriptorSetLayout(VkDevice device)
{

    MAZEGAME_NO_INIT VkDescriptorSetLayoutBinding binding;
    binding.binding             = DESCRIPTOR_LAYOUT_SAMPLER_BINDING;
    binding.descriptorCount     = vulkan::TEXTURES_PER_MATERIAL;
    binding.descriptorType      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers  = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutCreateInfo.bindingCount   = 1;
    layoutCreateInfo.pBindings      = &binding;

    MAZEGAME_NO_INIT VkDescriptorSetLayout resultDescriptorSetLayout;
    if(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create the descriptor set layout");
    }
    return resultDescriptorSetLayout;
}

internal VkDescriptorSetLayout
CreateSceneDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding layoutBindings [1] = {};

    // UNIFORM BUFFER OBJECT, camera projection matrices for now
    layoutBindings[0].binding = DESCRIPTOR_LAYOUT_SCENE_BINDING;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr; // Note(Leo): relevant for sampler stuff, like textures

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layoutCreateInfo.bindingCount = 1;
    layoutCreateInfo.pBindings = &layoutBindings[0];

    VkDescriptorSetLayout resultDescriptorSetLayout;

    if(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create the descriptor set layout");
    }

    return resultDescriptorSetLayout;
}

internal VulkanPipelineItems
make_pipeline(
    VulkanContext * context,
    int32 descriptorSetLayoutCount,
    const char * vertexShaderPath,
    const char * fragmentShaderPath,
    bool32 enableDepth)
{

    // Todo(Leo): unhardcode these
    BinaryAsset vertexShaderCode = ReadBinaryFile(vertexShaderPath);
    BinaryAsset fragmentShaderCode = ReadBinaryFile(fragmentShaderPath);

    VkShaderModule vertexShaderModule = vulkan::CreateShaderModule(vertexShaderCode, context->device);
    VkShaderModule fragmentShaderModule = vulkan::CreateShaderModule(fragmentShaderCode, context->device);

    enum : uint32
    {
        VERTEX_STAGE_ID     = 0,
        FRAGMENT_STAGE_ID   = 1,
        SHADER_STAGE_COUNT  = 2
    };

    VkPipelineShaderStageCreateInfo shaderStages [SHADER_STAGE_COUNT] =
    {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertexShaderModule,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragmentShaderModule,
            .pName = "main",
        }
    };

    auto bindingDescription     = vulkan::GetVertexBindingDescription();
    auto attributeDescriptions  = vulkan::GetVertexAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo =
    {
        .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount      = 1,
        .pVertexBindingDescriptions         = &bindingDescription,
        .vertexAttributeDescriptionCount    = attributeDescriptions.count(),
        .pVertexAttributeDescriptions       = &attributeDescriptions[0],
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport =
    {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = (float) context->swapchainItems.extent.width,
        .height     = (float) context->swapchainItems.extent.height,
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    VkRect2D scissor =
    {
        .offset = {0, 0},
        .extent = context->swapchainItems.extent,
    };

    VkPipelineViewportStateCreateInfo viewportState =
    {
        .sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount  = 1,
        .pViewports     = &viewport,
        .scissorCount   = 1,
        .pScissors      = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer =
    {
        .sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable           = VK_FALSE,
        .rasterizerDiscardEnable    = VK_FALSE,
        .polygonMode                = VK_POLYGON_MODE_FILL,
        .lineWidth                  = 1.0f,
        .cullMode                   = VK_CULL_MODE_BACK_BIT,
        .frontFace                  = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable            = VK_FALSE,
        .depthBiasConstantFactor    = 0.0f,
        .depthBiasClamp             = 0.0f,
        .depthBiasSlopeFactor       = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable    = VK_FALSE,
        .rasterizationSamples   = context->msaaSamples,
        .minSampleShading       = 1.0f,
        .pSampleMask            = nullptr,
        .alphaToCoverageEnable  = VK_FALSE,
        .alphaToOneEnable       = VK_FALSE,
    };

    // Note: This attachment is per framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment =
    {
        .colorWriteMask         =   VK_COLOR_COMPONENT_R_BIT 
                                    | VK_COLOR_COMPONENT_G_BIT 
                                    | VK_COLOR_COMPONENT_B_BIT 
                                    | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable            = VK_FALSE,
        .srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp           = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp           = VK_BLEND_OP_ADD, 
    };

    // Note: this is global
    VkPipelineColorBlendStateCreateInfo colorBlending =
    {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable      = VK_FALSE, // Note: enabling this disables per framebuffer method above,
        .logicOp            = VK_LOGIC_OP_COPY,
        .attachmentCount    = 1,
        .pAttachments       = &colorBlendAttachment,
        .blendConstants[0]  = 0.0f,
        .blendConstants[1]  = 0.0f,
        .blendConstants[2]  = 0.0f,
        .blendConstants[3]  = 0.0f,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil =
    { 
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

        .depthTestEnable    = VK_TRUE,
        .depthWriteEnable   = (VkBool32)enableDepth,
        .depthCompareOp     = enableDepth ? VK_COMPARE_OP_LESS : VK_COMPARE_OP_LESS,

        // Note(Leo): These further limit depth range for this render pass
        .depthBoundsTestEnable  = VK_FALSE,
        .minDepthBounds         = 0.0f,
        .maxDepthBounds         = 1.0f,

        // Note(Leo): Configure stencil tests
        .stencilTestEnable  = VK_FALSE,
        .front              = {},
        .back               = {},
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = (uint32)descriptorSetLayoutCount,
        .pSetLayouts            = context->descriptorSetLayouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    };

    VkPipelineLayout layout;
    if (vkCreatePipelineLayout (context->device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount             = SHADER_STAGE_COUNT,
        .pStages                = shaderStages,

        // Note(Leo): Are these The Fixed-Function Stages?
        .pVertexInputState      = &vertexInputInfo,
        .pInputAssemblyState    = &inputAssembly,
        .pViewportState         = &viewportState,
        .pRasterizationState    = &rasterizer,
        .pMultisampleState      = &multisampling,
        .pDepthStencilState     = &depthStencil,
        .pColorBlendState       = &colorBlending,
        .pDynamicState          = nullptr,
        .layout                 = layout,
        .renderPass             = context->renderPass,
        .subpass                = 0,

        // Note(Leo): These are for cheaper re-use of pipelines, not used right now.
        // Study(Leo): Like inheritance??
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };


    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    // Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
    vkDestroyShaderModule(context->device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(context->device, vertexShaderModule, nullptr);

    return {
        .pipeline   = pipeline,
        .layout     = layout
    };
}


// internal VulkanPipelineItems
// CreateGraphicsPipeline(
//     VulkanContext * context,
//     int32 descriptorSetLayoutCount)
// {
//     auto result = make_pipeline(context, descriptorSetLayoutCount, "shaders/vert.spv", "shaders/frag.spv");
//     return result;
// }

internal VulkanPipelineItems
CreateGuiPipeline(VulkanContext * context, int32 descriptorSetLayoutCount)
{
    auto result = make_pipeline(context, descriptorSetLayoutCount, "shaders/vert_gui.spv", "shaders/frag_gui.spv");
    return result;
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
    VkCommandBuffer commandBuffer = vulkan::BeginOneTimeCommandBuffer(device, commandPool);

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
        if (vulkan::FormatHasStencilComponent(format))
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

    vulkan::EndOneTimeCommandBuffer (device, commandPool, graphicsQueue, commandBuffer);
}


internal VulkanDrawingResources
CreateDrawingResources(VulkanContext * context)
{
    VulkanDrawingResources resultResources = {};

    VkFormat colorFormat = context->swapchainItems.imageFormat;
    resultResources.colorImage = CreateImage(  context->device, context->physicalDevice,
                                                context->swapchainItems.extent.width, context->swapchainItems.extent.height,
                                                1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                                                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                context->msaaSamples);

    VkFormat depthFormat = FindSupportedDepthFormat(context->physicalDevice);
    resultResources.depthImage = CreateImage(  context->device, context->physicalDevice,
                                                context->swapchainItems.extent.width, context->swapchainItems.extent.height,
                                                1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                context->msaaSamples);

    VkMemoryRequirements colorMemoryRequirements, depthMemoryRequirements;
    vkGetImageMemoryRequirements(context->device, resultResources.colorImage, &colorMemoryRequirements);
    vkGetImageMemoryRequirements(context->device, resultResources.depthImage, &depthMemoryRequirements);


    // Todo(Leo): Do these need to be aligned like below????
    uint64 colorMemorySize = colorMemoryRequirements.size;
    uint64 depthMemorySize = depthMemoryRequirements.size;
    // uint64 colorMemorySize = align_up_to(colorMemoryRequirements.alignment, colorMemoryRequirements.size);
    // uint64 depthMemorySize = align_up_to(depthMemoryRequirements.alignment, depthMemoryRequirements.size);

    uint32 memoryTypeIndex = vulkan::FindMemoryType(
                                context->physicalDevice,
                                colorMemoryRequirements.memoryTypeBits | depthMemoryRequirements.memoryTypeBits,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = (colorMemorySize + depthMemorySize);
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(context->device, &allocateInfo, nullptr, &resultResources.memory))
    {
        throw std::runtime_error("Failed to allocate drawing resource memory");
    }            

    // Todo(Leo): Result check binding memories
    vkBindImageMemory(context->device, resultResources.colorImage, resultResources.memory, 0);
    vkBindImageMemory(context->device, resultResources.depthImage, resultResources.memory, colorMemorySize);

    resultResources.colorImageView = CreateImageView(  context->device, resultResources.colorImage,
                                                        1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    resultResources.depthImageView = CreateImageView(  context->device, resultResources.depthImage,
                                                        1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    TransitionImageLayout(  context->device, context->commandPool, context->graphicsQueue, resultResources.colorImage, colorFormat, 1,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    TransitionImageLayout(  context->device, context->commandPool, context->graphicsQueue, resultResources.depthImage, depthFormat, 1,
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

    'count' is one for actor (characters, animated scenery) uniform buffers which are dynamic and 
    one for static scenery.
    */

    constexpr int32 count = 2;
    VkDescriptorPoolSize poolSizes [count] = {};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[0].descriptorCount = swapchainImageCount * 2; // Note(Leo): 2 = 1 for 3d models and 1 for gui 

    // TODO(Leo): Hack from past, REMOVE
    int randomMultiplierForTesting = 20;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = swapchainImageCount * randomMultiplierForTesting;

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = count;
    poolInfo.pPoolSizes = &poolSizes[0];
    poolInfo.maxSets = swapchainImageCount * randomMultiplierForTesting;

    MAZEGAME_NO_INIT VkDescriptorPool resultDescriptorPool;
    MAZEGAME_ASSERT(
        vkCreateDescriptorPool(device, &poolInfo, nullptr, &resultDescriptorPool) == VK_SUCCESS,
        "Failed to create descriptor pool"
    );
    return resultDescriptorPool;
}

internal VkDescriptorPool
CreateMaterialDescriptorPool(VkDevice device)
{
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = vulkan::MAX_LOADED_TEXTURES;

    VkDescriptorPoolCreateInfo poolCreateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolCreateInfo.poolSizeCount = 1;
    poolCreateInfo.pPoolSizes = &poolSize;
    poolCreateInfo.maxSets = vulkan::MAX_LOADED_TEXTURES;

    MAZEGAME_NO_INIT VkDescriptorPool result;
    MAZEGAME_ASSERT(
        vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &result) == VK_SUCCESS,
        "Failed to create descriptor pool for materials!");
    return result;

}

internal std::vector<VkDescriptorSet>
CreateGuiDescriptorSets(VulkanContext * context)
{
    /* Note(Leo): Create vector of [imageCount] copies from descriptorSetLayout
    for allocation */
    int imageCount = context->swapchainItems.images.size();
    std::vector<VkDescriptorSetLayout> layouts (imageCount, context->guiDescriptorSetLayout);

    VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocateInfo.descriptorPool = context->uniformDescriptorPool;
    allocateInfo.descriptorSetCount = imageCount;
    allocateInfo.pSetLayouts = &layouts[0];

    std::vector<VkDescriptorSet> resultDescriptorSets(imageCount);
    if (vkAllocateDescriptorSets(context->device, &allocateInfo, &resultDescriptorSets[0]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
    }

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex)
    {
        VkWriteDescriptorSet descriptorWrites [1] = {};

        // MODEL UNIFORM BUFFERS
        VkDescriptorBufferInfo guiBufferInfo = {};
        guiBufferInfo.buffer = context->guiUniformBuffer.buffer;
        guiBufferInfo.offset = vulkan::GetModelUniformBufferOffsetForSwapchainImages(context, imageIndex);
        guiBufferInfo.range = sizeof(Matrix44);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = resultDescriptorSets[imageIndex];
        descriptorWrites[0].dstBinding = DESCRIPTOR_LAYOUT_MODEL_BINDING;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &guiBufferInfo;

        // Note(Leo): Two first are write info, two latter are copy info
        vkUpdateDescriptorSets(context->device, 1, &descriptorWrites[0], 0, nullptr);
    }

    return resultDescriptorSets;
}

internal std::vector<VkDescriptorSet>
CreateModelDescriptorSets(VulkanContext * context)
{
    /* Note(Leo): Create vector of [imageCount] copies from descriptorSetLayout
    for allocation */
    int imageCount = context->swapchainItems.images.size();
    std::vector<VkDescriptorSetLayout> layouts (imageCount, context->descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM]);

    VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocateInfo.descriptorPool = context->uniformDescriptorPool;
    allocateInfo.descriptorSetCount = imageCount;
    allocateInfo.pSetLayouts = &layouts[0];

    std::vector<VkDescriptorSet> resultDescriptorSets(imageCount);
    if (vkAllocateDescriptorSets(context->device, &allocateInfo, &resultDescriptorSets[0]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
    }

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex)
    {
        VkWriteDescriptorSet descriptorWrites [1] = {};

        // MODEL UNIFORM BUFFERS
        VkDescriptorBufferInfo modelBufferInfo = {};
        modelBufferInfo.buffer = context->modelUniformBuffer.buffer;
        modelBufferInfo.offset = vulkan::GetModelUniformBufferOffsetForSwapchainImages(context, imageIndex);
        modelBufferInfo.range = sizeof(Matrix44);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = resultDescriptorSets[imageIndex];
        descriptorWrites[0].dstBinding = DESCRIPTOR_LAYOUT_MODEL_BINDING;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &modelBufferInfo;

        // Note(Leo): Two first are write info, two latter are copy info
        vkUpdateDescriptorSets(context->device, 1, &descriptorWrites[0], 0, nullptr);
    }

    return resultDescriptorSets;
}

internal VkDescriptorSet
vulkan::CreateMaterialDescriptorSets(
    VulkanContext * context,
    TextureHandle albedoHandle,
    TextureHandle metallicHandle,
    TextureHandle testMaskHandle)
{
    VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocateInfo.descriptorPool = context->materialDescriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &context->descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL];

    VkDescriptorSet resultSet;
    if (vkAllocateDescriptorSets(context->device, &allocateInfo, &resultSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
    }

    VkDescriptorImageInfo samplerInfos [vulkan::TEXTURES_PER_MATERIAL]
    {
        {
            .sampler        = context->textureSampler,
            .imageView      = context->loadedTextures[albedoHandle].view,
            .imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
        {
            .sampler        = context->textureSampler,
            .imageView      = context->loadedTextures[metallicHandle].view,
            .imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        },
        {
            .sampler        = context->textureSampler,
            .imageView      = context->loadedTextures[testMaskHandle].view,
            .imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        }
    };

    VkWriteDescriptorSet writing =
    {  
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_SAMPLER_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = vulkan::TEXTURES_PER_MATERIAL,
        .pImageInfo         = samplerInfos,
    };

    // Note(Leo): Two first are write info, two latter are copy info
    vkUpdateDescriptorSets(context->device, 1, &writing, 0, nullptr);

    return resultSet;
}

internal std::vector<VkDescriptorSet>
CreateSceneDescriptorSets(VulkanContext * context)
{
    /* Note(Leo): Create vector of [imageCount] copies from layout for allocation */
    int imageCount = context->swapchainItems.images.size();
    std::vector<VkDescriptorSetLayout> layouts (imageCount, context->descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM]);

    VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    allocateInfo.descriptorPool = context->uniformDescriptorPool;
    allocateInfo.descriptorSetCount = imageCount;
    allocateInfo.pSetLayouts = &layouts[0];

    std::vector<VkDescriptorSet> resultDescriptorSets(imageCount);
    if (vkAllocateDescriptorSets(context->device, &allocateInfo, &resultDescriptorSets[0]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
    }

    for (int imageIndex = 0; imageIndex < imageCount; ++imageIndex)
    {
        VkWriteDescriptorSet descriptorWrites [1] = {};

        // SCENE UNIFORM BUFFER
        VkDescriptorBufferInfo sceneBufferInfo = {};
        sceneBufferInfo.buffer = context->sceneUniformBuffer.buffer;
        sceneBufferInfo.offset = vulkan::GetSceneUniformBufferOffsetForSwapchainImages(context, imageIndex);
        sceneBufferInfo.range = sizeof(VulkanCameraUniformBufferObject);

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = resultDescriptorSets[imageIndex];
        descriptorWrites[0].dstBinding = DESCRIPTOR_LAYOUT_SCENE_BINDING;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &sceneBufferInfo;

        // Note(Leo): Two first are write info, two latter are copy info
        vkUpdateDescriptorSets(context->device, 1, &descriptorWrites[0], 0, nullptr);
    }

    return resultDescriptorSets;
}

internal std::vector<VkFramebuffer>
CreateFrameBuffers(VulkanContext * context)
{ 
    /* Note(Leo): This is basially allocating right, there seems to be no
    need for VkDeviceMemory for swapchainimages??? */
    
    int imageCount = context->swapchainItems.imageViews.size();
    // frameBuffers.resize(imageCount);
    std::vector<VkFramebuffer> resultFrameBuffers (imageCount);

    for (int i = 0; i < imageCount; ++i)
    {
        constexpr int ATTACHMENT_COUNT = 3;
        VkImageView attachments[ATTACHMENT_COUNT] = {
            context->drawingResources.colorImageView,
            context->drawingResources.depthImageView,
            context->swapchainItems.imageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        framebufferInfo.renderPass      = context->renderPass;
        framebufferInfo.attachmentCount = ATTACHMENT_COUNT;
        framebufferInfo.pAttachments    = &attachments[0];
        framebufferInfo.width           = context->swapchainItems.extent.width;
        framebufferInfo.height          = context->swapchainItems.extent.height;
        framebufferInfo.layers          = 1;

        if (vkCreateFramebuffer(context->device, &framebufferInfo, nullptr, &resultFrameBuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer");
        }
    }

    return resultFrameBuffers;
}

internal void
CopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue submitQueue, VkBuffer srcBuffer, VkImage dstImage, uint32 width, uint32 height)
{
    VkCommandBuffer commandBuffer = vulkan::BeginOneTimeCommandBuffer(device, commandPool);

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

    vulkan::EndOneTimeCommandBuffer (device, commandPool, submitQueue, commandBuffer);
}

internal uint32
ComputeMipmapLevels(uint32 texWidth, uint32 texHeight)
{
   uint32 result = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;
   return result;
}

internal void
GenerateMipMaps(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue submitQueue, VkImage image, VkFormat imageFormat, uint32 texWidth, uint32 texHeight, uint32 mipLevels)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == false)
    {
        throw std::runtime_error("Texture image format does not support blitting!");
    }


    VkCommandBuffer commandBuffer = vulkan::BeginOneTimeCommandBuffer(device, commandPool);

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

    vulkan::EndOneTimeCommandBuffer (device, commandPool, submitQueue, commandBuffer);
}

internal VulkanTexture
vulkan::CreateImageTexture(TextureAsset * asset, VulkanContext * context)
{
    VulkanTexture resultTexture = {};
    resultTexture.mipLevels = ComputeMipmapLevels(asset->width, asset->height);
    VkDeviceSize imageSize = asset->width * asset->height * asset->channels;

    void * data;
    vkMapMemory(context->device, context->stagingBufferPool.memory, 0, imageSize, 0, &data);
    memcpy (data, (void*)asset->pixels.begin(), imageSize);
    vkUnmapMemory(context->device, context->stagingBufferPool.memory);

    CreateImageAndMemory(context->device, context->physicalDevice,
                asset->width, asset->height, resultTexture.mipLevels,
                VK_FORMAT_R8G8B8A8_UNORM,
                VK_IMAGE_TILING_OPTIMAL, 
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                VK_SAMPLE_COUNT_1_BIT,
                &resultTexture.image, &resultTexture.memory);

    /* Note(Leo):
        1. change layout to copy optimal
        2. copy contents
        3. change layout to read optimal

        This last is good to use after generated textures, finally some 
        benefits from this verbose nonsense :D
    */
    // Todo(Leo): begin and end command buffers once only and then just add commands from inside these
    TransitionImageLayout(  context->device, context->commandPool, context->graphicsQueue,
                            resultTexture.image, VK_FORMAT_R8G8B8A8_UNORM, resultTexture.mipLevels,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    CopyBufferToImage(  context->device, context->commandPool, context->graphicsQueue,
                        context->stagingBufferPool.buffer, resultTexture.image,
                        asset->width, asset->height);

    GenerateMipMaps(    context->device, context->physicalDevice, context->commandPool,
                        context->graphicsQueue, resultTexture.image, VK_FORMAT_R8G8B8A8_UNORM,
                        asset->width, asset->height, resultTexture.mipLevels);

    resultTexture.view = CreateImageView(   context->device, resultTexture.image,
                                            resultTexture.mipLevels,
                                            VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);

    return resultTexture;
}

internal void
vulkan::DestroyImageTexture(VulkanContext * context, VulkanTexture * texture)
{
    vkDestroyImage(context->device, texture->image, nullptr);
    vkFreeMemory(context->device, texture->memory, nullptr);
    vkDestroyImageView(context->device, texture->view, nullptr);
}

internal VkSampler
CreateTextureSampler(VulkanContext * context)
{
    VkSamplerCreateInfo samplerInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter          = VK_FILTER_LINEAR,
        .minFilter          = VK_FILTER_LINEAR,

        .addressModeU       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW       = VK_SAMPLER_ADDRESS_MODE_REPEAT,

        .anisotropyEnable   = VK_TRUE,
        .maxAnisotropy      = 16,

        .borderColor                 = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates     = VK_FALSE,
        .compareEnable               = VK_FALSE,
        .compareOp                   = VK_COMPARE_OP_ALWAYS,

        .mipmapMode         = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .minLod             = 0.0f,
        .maxLod             = VULKAN_MAX_LOD_FLOAT,
        .mipLodBias         = 0.0f,
    };

    VkSampler resultSampler;
    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &resultSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }
    return resultSampler;
}

internal void
CleanupSwapchain(VulkanContext * context)
{
    vkDestroyImage (context->device, context->drawingResources.colorImage, nullptr);
    vkDestroyImageView(context->device, context->drawingResources.colorImageView, nullptr);
    vkDestroyImage (context->device, context->drawingResources.depthImage, nullptr);
    vkDestroyImageView (context->device, context->drawingResources.depthImageView, nullptr);
    vkFreeMemory(context->device, context->drawingResources.memory, nullptr);

    vkDestroyDescriptorPool(context->device, context->uniformDescriptorPool, nullptr);

    for (auto framebuffer : context->frameBuffers)
    {
        vkDestroyFramebuffer(context->device, framebuffer, nullptr);
    }

    // Normal graphics
    // TODO(Leo): clean pipeline things in context for game

    // vkDestroyPipeline(context->device, context->pipelineItems.pipeline, nullptr);
    // vkDestroyPipelineLayout(context->device, context->pipelineItems.layout, nullptr);

    // Gui graphics
    vkDestroyPipeline(context->device, context->guiPipelineItems.pipeline, nullptr);
    vkDestroyPipelineLayout(context->device, context->guiPipelineItems.layout, nullptr);    

    vkDestroyRenderPass(context->device, context->renderPass, nullptr);

    for (auto imageView : context->swapchainItems.imageViews)
    {
        vkDestroyImageView(context->device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(context->device, context->swapchainItems.swapchain, nullptr);
}

internal void
Cleanup(VulkanContext * context)
{
    CleanupSwapchain(context);
    vkDestroyDescriptorPool(context->device, context->materialDescriptorPool, nullptr);

    for (int i = 0; i < context->loadedTextures.size(); ++i)
    {
        vulkan::DestroyImageTexture(context, &context->loadedTextures[i]);
    }

    vkDestroySampler(context->device, context->textureSampler, nullptr);

    vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MODEL_UNIFORM], nullptr);  
    vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_SCENE_UNIFORM], nullptr);  
    vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayouts[DESCRIPTOR_SET_LAYOUT_MATERIAL], nullptr);  
    vkDestroyDescriptorSetLayout(context->device, context->guiDescriptorSetLayout, nullptr);  

    vulkan::DestroyBufferResource(context->device, &context->staticMeshPool);
    vulkan::DestroyBufferResource(context->device, &context->stagingBufferPool);
    vulkan::DestroyBufferResource(context->device, &context->modelUniformBuffer);
    vulkan::DestroyBufferResource(context->device, &context->sceneUniformBuffer);
    vulkan::DestroyBufferResource(context->device, &context->guiUniformBuffer);

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroySemaphore(context->device, context->syncObjects.renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(context->device, context->syncObjects.imageAvailableSemaphores[i], nullptr);

        vkDestroyFence(context->device, context->syncObjects.inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(context->device, context->commandPool, nullptr);

    vkDestroyDevice(context->device, nullptr);
    vkDestroySurfaceKHR(context->instance, context->surface, nullptr);
    vkDestroyInstance(context->instance, nullptr);
}

internal void
CreateCommandBuffers(VulkanContext * context)
{
    int32 framebufferCount = context->frameBuffers.size();
    context->frameCommandBuffers.resize(framebufferCount);

    VkCommandBufferAllocateInfo allocateInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = context->commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = (uint32)context->frameCommandBuffers.size(),
    };

    if (vkAllocateCommandBuffers(context->device, &allocateInfo, &context->frameCommandBuffers[0]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate command buffers");
    }


    int32 objectCount = context->loadedRenderedObjects.size();
    std::vector<VulkanRenderInfo> renderInfos (objectCount);

    for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
    {
        MeshHandle meshHandle           = context->loadedRenderedObjects[objectIndex].mesh;
        MaterialHandle materialHandle   = context->loadedRenderedObjects[objectIndex].material;

        renderInfos[objectIndex].meshBuffer             = context->loadedModels[meshHandle].buffer; 
        renderInfos[objectIndex].vertexOffset           = context->loadedModels[meshHandle].vertexOffset;
        renderInfos[objectIndex].indexOffset            = context->loadedModels[meshHandle].indexOffset;
        renderInfos[objectIndex].indexCount             = context->loadedModels[meshHandle].indexCount;
        renderInfos[objectIndex].indexType              = context->loadedModels[meshHandle].indexType;

        renderInfos[objectIndex].uniformBufferOffset    = context->loadedRenderedObjects[objectIndex].uniformBufferOffset;
        
        renderInfos[objectIndex].materialIndex          = materialHandle;
    }

    int32 guiCount = context->loadedGuiObjects.size();
    std::vector<VulkanRenderInfo> guiInfos (guiCount);

    for (int32 guiIndex = 0; guiIndex < guiCount; ++guiIndex)
    {
        MeshHandle meshHandle           = context->loadedGuiObjects[guiIndex].mesh;
        MaterialHandle materialHandle   = context->loadedGuiObjects[guiIndex].material;

        guiInfos[guiIndex].meshBuffer             = context->loadedModels[meshHandle].buffer; 
        guiInfos[guiIndex].vertexOffset           = context->loadedModels[meshHandle].vertexOffset;
        guiInfos[guiIndex].indexOffset            = context->loadedModels[meshHandle].indexOffset;
        guiInfos[guiIndex].indexCount             = context->loadedModels[meshHandle].indexCount;
        guiInfos[guiIndex].indexType              = context->loadedModels[meshHandle].indexType;

        guiInfos[guiIndex].uniformBufferOffset    = context->loadedGuiObjects[guiIndex].uniformBufferOffset;
        
        guiInfos[guiIndex].materialIndex          = materialHandle;   
    }

    for (int frameIndex = 0; frameIndex < context->frameCommandBuffers.size(); ++frameIndex)
    {
        VkCommandBuffer commandBuffer = context->frameCommandBuffers[frameIndex];

        VkCommandBufferBeginInfo beginInfo =
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags              = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            .pInheritanceInfo   = nullptr,
        };

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to begin recording command buffer");
        }

        // Todo(Leo): These should also use constexpr ids or unscoped enums
        VkClearValue clearValues [2] =
        {
            { .color = {0.0f, 0.0f, 0.0f, 1.0f} },
            { .depthStencil = {1.0f, 0} }
        };

        VkRenderPassBeginInfo renderPassInfo =
        {
            .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass         = context->renderPass,
            .framebuffer        = context->frameBuffers[frameIndex],
            
            .renderArea.offset  = {0, 0},
            .renderArea.extent  = context->swapchainItems.extent,

            .clearValueCount    = 2,
            .pClearValues       = &clearValues [0],
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        
        // struct ShaderThing
        // {
        //     VkPipeline pipeline;
        //     VkPipelineLayout layout;  
        // };

        // struct MaterialThing
        // {
        //     ShaderThing shader;

        //     int textureCount = 3;
        //     int uniformCount = 3;

        //     TextureHandle textures[3];
        //     float uniforms [3];
        // };

        enum : uint32
        {
            LAYOUT_SCENE    = 0,
            LAYOUT_MATERIAL = 1,
            LAYOUT_MODEL    = 2,
        };

        // Todo(Leo): Not really 'shader' although it is, but actually a pipeline...

        // Normal 3d graphics
        {
            ShaderHandle        currentShader = {};
            VkPipeline          currentPipeline;
            VkPipelineLayout    currentLayout;

            for (int objectIndex = 0; objectIndex < objectCount; ++objectIndex)
            {
                auto materialIndex = renderInfos[objectIndex].materialIndex;
                ShaderHandle newShader = context->loadedMaterials[materialIndex].pipeline;
                if (newShader != currentShader)
                {
                    currentShader   = newShader;
                    currentPipeline = context->loadedPipelines[currentShader].pipeline;
                    currentLayout   = context->loadedPipelines[currentShader].layout;

                    vkCmdBindPipeline(      commandBuffer,
                                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            currentPipeline);

                    vkCmdBindDescriptorSets(commandBuffer,
                                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            currentLayout,
                                            LAYOUT_SCENE,
                                            1, 
                                            &context->sceneDescriptorSets[frameIndex],
                                            0,
                                            nullptr);
                }

                // Bind material
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        currentLayout,
                                        LAYOUT_MATERIAL,
                                        1, 
                                        &context->loadedMaterials[materialIndex].descriptorSet,
                                        0,
                                        nullptr);
                // Bind model info
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, &renderInfos[objectIndex].meshBuffer, &renderInfos[objectIndex].vertexOffset);
                vkCmdBindIndexBuffer(commandBuffer, renderInfos[objectIndex].meshBuffer, renderInfos[objectIndex].indexOffset, renderInfos[objectIndex].indexType);

                // Bind entity transform
                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        currentLayout,
                                        LAYOUT_MODEL,
                                        1,
                                        &context->descriptorSets[frameIndex],
                                        1,
                                        &renderInfos[objectIndex].uniformBufferOffset);

                vkCmdDrawIndexed(commandBuffer, renderInfos[objectIndex].indexCount, 1, 0, 0, 0);
            }
        }

        // 2d gui pipeline
        {
            vkCmdBindPipeline(      commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    context->guiPipelineItems.pipeline);

            vkCmdBindDescriptorSets(commandBuffer,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    context->guiPipelineItems.layout,
                                    0, 1,
                                    &context->sceneDescriptorSets[frameIndex],
                                    0, nullptr);

            for (int guiIndex = 0; guiIndex < guiCount; ++guiIndex)
            {
                // Bind material
                auto materialIndex = guiInfos[guiIndex].materialIndex;

                vkCmdBindDescriptorSets(commandBuffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        context->guiPipelineItems.layout,
                                        1,
                                        1,
                                        &context->loadedMaterials[materialIndex].descriptorSet,
                                        0,
                                        nullptr);
                // Bind model info
                vkCmdBindVertexBuffers( commandBuffer, 0, 1, &guiInfos[guiIndex].meshBuffer, &guiInfos[guiIndex].vertexOffset);
                vkCmdBindIndexBuffer(   commandBuffer, guiInfos[guiIndex].meshBuffer, guiInfos[guiIndex].indexOffset, guiInfos[guiIndex].indexType);

                // Bind entity transform
                vkCmdBindDescriptorSets(    commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                            context->guiPipelineItems.layout,
                                            2, 1, 
                                            &context->guiDescriptorSets[frameIndex], 1,
                                            &guiInfos[guiIndex].uniformBufferOffset);

                vkCmdDrawIndexed(commandBuffer, guiInfos[guiIndex].indexCount, 1, 0, 0, 0);
            }
        }

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to record command buffer");
        }
    }

}

internal void
vulkan::RefreshCommandBuffers(VulkanContext * context)
{
    vkFreeCommandBuffers(context->device, context->commandPool, context->frameCommandBuffers.size(), &context->frameCommandBuffers[0]);        
    CreateCommandBuffers(context);
}



internal void
vulkan::UpdateUniformBuffer(VulkanContext * context, uint32 imageIndex, vulkan::RenderInfo * renderInfo)
{
    // Todo(Leo): Single mapping is really enough, offsets can be used here too
    uint32 startUniformBufferOffset = vulkan::GetModelUniformBufferOffsetForSwapchainImages(context, imageIndex);

    int32 modelCount = context->loadedRenderedObjects.size();
    for (RenderedObjectHandle handle = {0}; handle < modelCount; ++handle.index)
    {
        Matrix44 * pModelMatrix;
        vkMapMemory(context->device, context->modelUniformBuffer.memory, 
                    startUniformBufferOffset + context->loadedRenderedObjects[handle].uniformBufferOffset,
                    sizeof(pModelMatrix), 0, (void**)&pModelMatrix);

        *pModelMatrix = renderInfo->renderedObjects[handle];
        vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

    }

    int guiCount = context->loadedGuiObjects.size();
    for (GuiHandle handle = {0}; handle < guiCount; ++handle.index)
    {
        Matrix44 * pGuiMatrix;
        vkMapMemory(context->device, context->guiUniformBuffer.memory,
                    startUniformBufferOffset + context->loadedGuiObjects[handle].uniformBufferOffset,
                    sizeof(pGuiMatrix), 0, (void**)&pGuiMatrix);

        *pGuiMatrix = renderInfo->guiObjects[handle];
        vkUnmapMemory(context->device, context->guiUniformBuffer.memory);
    }

    // Note (Leo): map vulkan memory directly to right type so we can easily avoid one (small) memcpy per frame
    VulkanCameraUniformBufferObject * pUbo;
    vkMapMemory(context->device, context->sceneUniformBuffer.memory,
                vulkan::GetSceneUniformBufferOffsetForSwapchainImages(context, imageIndex),
                sizeof(VulkanCameraUniformBufferObject), 0, (void**)&pUbo);

    pUbo->view          = renderInfo->cameraView;
    pUbo->perspective   = renderInfo->cameraPerspective;

    vkUnmapMemory(context->device, context->sceneUniformBuffer.memory);
}

internal void
vulkan::RecreateSwapchain(VulkanContext * context, VkExtent2D frameBufferSize)
{
    vkDeviceWaitIdle(context->device);

    CleanupSwapchain(context);

    std::cout << "Recreating swapchain, width: " << frameBufferSize.width << ", height: " << frameBufferSize.height <<"\n";

    context->swapchainItems      = CreateSwapchainAndImages(context, frameBufferSize);
    context->renderPass          = CreateRenderPass(context, &context->swapchainItems, context->msaaSamples);
    // context->pipelineItems       = make_pipeline(context, 3, "shaders/vert.spv", "shaders/frag.spv");
    context->guiPipelineItems    = make_pipeline(context, 3, "shaders/vert.spv", "shaders/frag.spv");
    context->drawingResources    = CreateDrawingResources(context);
    context->frameBuffers        = CreateFrameBuffers(context);
    context->uniformDescriptorPool      = CreateDescriptorPool(context->device, context->swapchainItems.images.size());


    context->descriptorSets = CreateModelDescriptorSets(context);
    context->guiDescriptorSets = CreateGuiDescriptorSets(context);

    context->sceneDescriptorSets = CreateSceneDescriptorSets(context);

    RefreshCommandBuffers(context);
}

internal void
vulkan::DrawFrame(VulkanContext * context, uint32 imageIndex, uint32 frameIndex)
{
    VkSubmitInfo submitInfo[1] = {};
    submitInfo[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // Note(Leo): We wait for these BEFORE drawing
    VkSemaphore waitSemaphores [] = { context->syncObjects.imageAvailableSemaphores[frameIndex] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo[0].waitSemaphoreCount = 1;
    submitInfo[0].pWaitSemaphores = waitSemaphores;
    submitInfo[0].pWaitDstStageMask = waitStages;

    submitInfo[0].commandBufferCount = 1;
    submitInfo[0].pCommandBuffers = &context->frameCommandBuffers[imageIndex];

    // Note(Leo): We signal these AFTER drawing
    VkSemaphore signalSemaphores[] = { context->syncObjects.renderFinishedSemaphores[frameIndex] };
    submitInfo[0].signalSemaphoreCount = ARRAY_COUNT(signalSemaphores);
    submitInfo[0].pSignalSemaphores = signalSemaphores;

    /* Note(Leo): reset here, so that if we have recreated swapchain above,
    our fences will be left in signaled state */
    vkResetFences(context->device, 1, &context->syncObjects.inFlightFences[frameIndex]);

    if (vkQueueSubmit(context->graphicsQueue, 1, submitInfo, context->syncObjects.inFlightFences[frameIndex]) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = ARRAY_COUNT(signalSemaphores);
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.pResults = nullptr;

    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &context->swapchainItems.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    // Todo(Leo): Should we do something about this??
    VkResult result = vkQueuePresentKHR(context->presentQueue, &presentInfo);

    // // Todo, Study(Leo): Somebody on interenet told us to do this. No Idea why???
    // // Note(Leo): Do after presenting to not interrupt semaphores at whrong time
    // if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context->framebufferResized)
    // {
    //     context->framebufferResized = false;
    //     RecreateSwapchain(context);
    // }
    // else if (result != VK_SUCCESS)
    // {
    //     throw std::runtime_error("Failed to present swapchain");
    // }
}