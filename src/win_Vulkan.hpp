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

struct VulkanQueueFamilyIndices
{
    uint32 graphics;
    uint32 present;

    bool32 hasGraphics;
    bool32 hasPresent;

    uint32 getAt(int index)
    {
        if (index == 0) return graphics;
        if (index == 1) return present;
        return -1;
    }

    bool32 hasAll()
    {
        return hasGraphics && hasPresent;
    }
};

struct VulkanSwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

constexpr uint64 VULKAN_NO_TIME_OUT	= MaxValue<uint64>;

namespace Vulkan
{
	// Todo(Leo): Probably going to roll custom for NDEBUG too
	#if MAZEGAME_DEVELOPMENT
	constexpr bool32 enableValidationLayers = false;
	#else
	constexpr bool32 enableValidationLayers = true;
	#endif

	constexpr const char * validationLayers[] = {
	    "VK_LAYER_KHRONOS_validation"
	};
	constexpr int VALIDATION_LAYERS_COUNT = ARRAY_COUNT(validationLayers);

	constexpr const char * deviceExtensions [] = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	constexpr int DEVICE_EXTENSION_COUNT = ARRAY_COUNT(deviceExtensions);


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

    internal inline bool32
    FormatHasStencilComponent(VkFormat format)
    {
        bool32 result = (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT);
        return result;
    }
	
	internal VulkanSwapchainSupportDetails
	QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);

	internal VkSurfaceFormatKHR
	ChooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& availableFormats);

	internal VkPresentModeKHR
	ChooseSurfacePresentMode(std::vector<VkPresentModeKHR> & availablePresentModes);

	VulkanQueueFamilyIndices
	FindQueueFamilies (VkPhysicalDevice device, VkSurfaceKHR surface);

	VkShaderModule
	CreateShaderModule(BinaryAsset code, VkDevice logicalDevice);
}

#define WIN_VULKAN_HPP
#endif