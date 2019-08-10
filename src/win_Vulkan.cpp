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