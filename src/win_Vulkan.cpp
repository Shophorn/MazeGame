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