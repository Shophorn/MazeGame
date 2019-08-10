/*=============================================================================
Leo Tamminen

Things to use with win api and vulkan.

Structs and functions in 'Vulkan' namespace are NOT actual vulkan functions,
but helpers and combos of things commonly used together in this game.

TODO(Leo): Maybe rename to communicate more clearly that these hide actual
vulkan api.
=============================================================================*/

#ifndef WIN_VULKAN_HPP

// Note(Leo): these need to align properly
struct VulkanCameraUniformBufferObject
{
	alignas(16) Matrix44 view;
	alignas(16) Matrix44 perspective;
};

struct VulkanLoadedModel
{
    VkBuffer        buffer;
    VkDeviceMemory  memory;
    VkDeviceSize    vertexOffset;
    VkDeviceSize    indexOffset;
    uint32          uniformBufferOffset;
    uint32          indexCount;
    VkIndexType     indexType;
};

struct VulkanBufferResource
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    
    uint64 used;
    uint64 size;

    bool32 created;
};

namespace Vulkan
{
	internal VkVertexInputBindingDescription
	GetVertexBindingDescription ();

	internal Array<VkVertexInputAttributeDescription, 4>
	GetVertexAttributeDescriptions();

	internal VkIndexType
	ConvertIndexType(IndexType);

	internal uint32
	FindMemoryType (
		VkPhysicalDevice 		physicalDevice,
		uint32 					typeFilter,
		VkMemoryPropertyFlags 	properties);

	internal void
	CreateBuffer(
	    VkDevice                logicalDevice,
	    VkPhysicalDevice        physicalDevice,
	    VkDeviceSize            bufferSize,
	    VkBufferUsageFlags      usage,
	    VkMemoryPropertyFlags   memoryProperties,
	    VkBuffer *              resultBuffer,
	    VkDeviceMemory *        resultBufferMemory);

	internal void
	CreateBufferResource(
		VkDevice 				logicalDevice,
		VkPhysicalDevice 		physicalDevice,
		VkDeviceSize			size,
		VkBufferUsageFlags		usage,
		VkMemoryPropertyFlags	memoryProperties,
		VulkanBufferResource *  result);

	internal void
	DestroyBufferResource(VkDevice logicalDevice, VulkanBufferResource * resource);
}

#define WIN_VULKAN_HPP
#endif