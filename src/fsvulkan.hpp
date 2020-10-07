/*
Leo Tamminen

Windows-Vulkan interface. And lots of rubbish at the moment.

// TODO(Leo): Make sure that arrays for getting extensions ana layers are large enough

TODO(Leo) extra: Use separate queuefamily thing for transfering between vertex
staging buffer and actual vertex buffer. https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer

STUDY(Leo):
	http://asawicki.info/news_1698_vulkan_sparse_binding_-_a_quick_overview.html


STUDY: https://devblogs.nvidia.com/vulkan-dos-donts/
*/

#ifndef WIN_VULKAN_HPP

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "fsvulkan_debug_strings.cpp"


using VulkanContext = PlatformGraphics;

internal void 
print_vulkan_assert(LogFileAddress address, VkResult result)
{
    log_graphics(0, address, "Vulkan check failed ", fsvulkan_result_string(result), "(", result, ")");
}

#define VULKAN_CHECK(result) if (result != VK_SUCCESS) { print_vulkan_assert(FILE_ADDRESS, result); abort();}

// Todo(Leo): Play with these later in development, when we have closer to final contents
// constexpr s32 VIRTUAL_FRAME_COUNT = 2;
constexpr s32 VIRTUAL_FRAME_COUNT = 3;


constexpr u64 VULKAN_NO_TIME_OUT = max_value_u64;
constexpr f32 VULKAN_MAX_LOD_FLOAT = 100.0f;

constexpr s32 VULKAN_MAX_MODEL_COUNT = 2000;
constexpr s32 VULKAN_MAX_MATERIAL_COUNT = 100;


constexpr VkSampleCountFlagBits VULKAN_MAX_MSAA_SAMPLE_COUNT = VK_SAMPLE_COUNT_2_BIT;


// Todo(Leo): these need to align properly
// Note(Leo): these need to align properly
// Study(Leo): these need to align properly
struct FSVulkanCameraUniformBuffer
{
	alignas(16) m44 view;
	alignas(16) m44 projection;
	alignas(16) m44 lightViewProjection;
	alignas(16) f32 shadowDistance;
	f32 shadowTransitionDistance;
};

struct FSVulkanLightingUniformBuffer
{
	v4 direction;
	v4 color;
	v4 ambient;
	v4 cameraPosition;

	v4 skyBottomColor;
	v4 skyTopColor;
	v4 skyGroundColor;

	v4 horizonHaloColorAndFalloff;
	v4 sunHaloColorAndFalloff;

	v4 sunDiscColor;
	v4 sunDiscSizeAndFalloff;

	f32 skyColor;
};

struct VulkanModelUniformBuffer
{
	// Note(Leo): matrices must be aligned on 16 byte boundaries
	// Todo(Leo): Find the confirmation for this from Vulkan documentation
	alignas(16) m44 	localToWorld;
	alignas(16) float 	isAnimated;
	alignas(16) m44 	bonesToLocal [32];
};

// Note(Leo): this is currently same as platform version HdrSettings, and we could just use that, 
// but if we add more or more complex variables, we may need to care about alignment.
struct FSVulkanHdrSettings
{
	f32 exposure;
	f32 contrast;
};

struct VulkanBufferResource
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    
    // Todo[memory, vulkan](Leo): IMPORTANT Enforce/track these
    VkDeviceSize used;
    VkDeviceSize size;
};

// Todo(Leo): these should be local variables in initialization function
// struct VulkanQueueFamilyIndices
// struct VulkanSwapchainSupportDetails
struct VulkanQueueFamilyIndices
{
    u32 graphics;
    u32 present;

    bool32 hasGraphics;
    bool32 hasPresent;

    u32 getAt(int index)
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
    VkSurfaceCapabilitiesKHR 		capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> 	presentModes;
};


struct VulkanTexture
{
	VkImage 		image;
	VkImageView 	view;
	VkSampler 		sampler;
	
	VkFormat 		format;

	// TODO(Leo): totally not allocate like this, we need texture pool
	VkDeviceMemory memory;

	u32 width;
	u32 height;
	u32 mipLevels;
};

struct VulkanGuiTexture
{
	VulkanTexture 	texture;
	VkDescriptorSet descriptorSet;
};

struct VulkanMesh
{
    VkBuffer        bufferReference;

    u32             indexCount;
    VkIndexType     indexType;

    VkDeviceSize    indexOffset;
    VkDeviceSize    vertexOffset;
    VkDeviceSize 	skinningOffset;

    bool 			hasSkinning;
};

struct VulkanMaterial
{
	GraphicsPipeline 	pipeline;
	VkDescriptorSet 	descriptorSet;
};

/* Todo(Leo): this is now redundant it can go away.
Then we can simply draw by mesh and material anytime */
struct VulkanModel
{
	MeshHandle 		mesh;
	MaterialHandle 	material;
};

struct VulkanPipeline
{
	VkPipeline 				pipeline;
	VkPipelineLayout 		pipelineLayout;
	VkDescriptorSetLayout 	descriptorSetLayout;
	s32 					textureCount;
};

struct VulkanVirtualFrame
{
	VkCommandBuffer masterCommandBuffer;
	VkCommandBuffer sceneCommandBuffer;
	VkCommandBuffer guiCommandBuffer;
	VkCommandBuffer offscreenCommandBuffer;

	VkFramebuffer  	framebuffer;

	// Todo(Leo): more like 'final framebuffer' or 'present framebuffer'
	VkFramebuffer 	passThroughFramebuffer;

    // Note(Leo): these are attchaments.
	// VkDeviceMemory attachmentMemory;

	VkImage 	colorImage;
	VkImageView colorImageView;

	VkImage 	depthImage;	
	VkImageView depthImageView;

	VkImage 	resolveImage;
	VkImageView resolveImageView;

	// Note(Leo): This is used as input to hdr->ldr tonemap shader
	VkDescriptorSet resolveImageDescriptor;

	// Todo(Leo): this is not enough, the complete shadow texture mess needs to be per virtual frame
	// Extre Todo(Leo): this is not even used even though it should be instead of static single one
    // VkFramebuffer  	shadowFramebuffer;

    VkSemaphore 	shadowPassWaitSemaphore;
	VkSemaphore    	imageAvailableSemaphore;
	VkSemaphore    	renderFinishedSemaphore;
	VkFence        	queueSubmitFence;
};

struct PlatformGraphics
{
	VkInstance 						instance;
	VkDevice 						device; // Note(Leo): this is Logical Device.
	VkPhysicalDevice 				physicalDevice;
	VkPhysicalDeviceProperties 		physicalDeviceProperties;
	VkSurfaceKHR 					surface;

#if FS_VULKAN_USE_VALIDATION
	VkDebugUtilsMessengerEXT debugMessenger;
#endif

	/// QUEUES
	VkQueue graphicsQueue;
	VkQueue presentQueue;

    VulkanVirtualFrame 	virtualFrames [VIRTUAL_FRAME_COUNT];
    u32 				virtualFrameIndex = 0;
	VkDeviceMemory 		virtualFrameAttachmentMemory;

    VkDescriptorPool 		uniformDescriptorPool;
    VkDescriptorPool 		materialDescriptorPool;

	// Note(Leo): these are not cleared on unload
    VkDescriptorPool		persistentDescriptorPool;

    VkDescriptorSetLayout 	modelDescriptorSetLayout;
    VkDescriptorSet 		modelDescriptorSet[VIRTUAL_FRAME_COUNT];

	VkDescriptorSetLayout 	cameraDescriptorSetLayout;
    VkDescriptorSet 		cameraDescriptorSet[VIRTUAL_FRAME_COUNT];

	VkDescriptorSetLayout 	lightingDescriptorSetLayout;
    VkDescriptorSet 		lightingDescriptorSet[VIRTUAL_FRAME_COUNT];

	VkDescriptorSetLayout 	hdrSettingsDescriptorSetLayout;
	VkDescriptorSet 		hdrSettingsDescriptorSet[VIRTUAL_FRAME_COUNT];

    // VulkanBufferResource sceneUniformBuffer;
    VkDeviceSize 	sceneUniformBufferCapacity;
    VkBuffer 		sceneUniformBuffer;
    VkDeviceMemory 	sceneUniformBufferMemory;

    FSVulkanCameraUniformBuffer * 	persistentMappedCameraUniformBufferMemory[VIRTUAL_FRAME_COUNT];
    FSVulkanLightingUniformBuffer * persistentMappedLightingUniformBufferMemory[VIRTUAL_FRAME_COUNT];
    FSVulkanHdrSettings * 			frameHdrSettings[VIRTUAL_FRAME_COUNT];

    // Uncategorized
	VkCommandPool 			commandPool;
    VkSampleCountFlagBits 	msaaSamples;

    /* Note(Leo): we need a separate sampler for each address mode (and other properties).
	'textureSampler' has REPEAT address mode*/
    VkSampler 				textureSampler;			
    VkSampler 				clampSampler;			
	
	VkFormat 				hdrFormat;
	FSVulkanHdrSettings 	hdrSettings;

    VkSwapchainKHR 	swapchain;
	VkExtent2D 		swapchainExtent;
    VkRenderPass 	renderPass;
    VkRenderPass 	passThroughRenderPass;

    // Note(Leo): these are images from gpu for presentation, we use them to form final framebuffers
    // Todo(Leo): get rid of std::vector...
    VkFormat 					swapchainImageFormat;
    std::vector<VkImage> 		swapchainImages;
    std::vector<VkImageView> 	swapchainImageViews;

	// ----------------------------------------------------------------------------------

    u32 		shadowTextureWidth;
    u32 		shadowTextureHeight;
	VkSampler 	shadowTextureSampler; 

	VkRenderPass 		shadowRenderPass;
	
	VkPipeline 			shadowPipeline;
	VkPipelineLayout 	shadowPipelineLayout;

	VkPipeline 				leavesShadowPipeline;
	VkPipelineLayout 		leavesShadowPipelineLayout;
	VkDescriptorSetLayout 	leavesShadowMaskDescriptorSetLayout;

	VulkanTexture 			shadowAttachment[VIRTUAL_FRAME_COUNT];
	VkFramebuffer 			shadowFramebuffer[VIRTUAL_FRAME_COUNT];

	VkDescriptorSetLayout 	shadowMapTextureDescriptorSetLayout;
	VkDescriptorSet 		shadowMapTextureDescriptorSet[VIRTUAL_FRAME_COUNT];
	

	// ----------------------------------------------------------------------------------

	// Todo(Leo): This is stupid, just use bool, we only have one use case, probably never have a ton, so just add more bools
	using PostRenderFunc = void(VulkanContext*);
	PostRenderFunc * onPostRender;

	// Todo(Leo): Guard against multithreaded race condition once we have that
	VkDeviceSize	stagingBufferCapacity;
	VkBuffer 		stagingBuffer;
	VkDeviceMemory 	stagingBufferDeviceMemory;
	u8 * 			persistentMappedStagingBufferMemory;

    VulkanBufferResource staticMeshPool;

    // VulkanBufferResource modelUniformBuffer[VIRTUAL_FRAME_COUNT];

    VkBuffer 		modelUniformBufferBuffer;
    VkDeviceMemory 	modelUniformBufferMemory;
    u8 * 			persistentMappedModelUniformBufferMemory;

    VkDeviceSize	modelUniformBufferFrameCapacity;
    VkDeviceSize	modelUniformBufferFrameStart[VIRTUAL_FRAME_COUNT];
    VkDeviceSize	modelUniformBufferFrameUsed[VIRTUAL_FRAME_COUNT];

    VkBuffer  		dynamicMeshBuffer;
    VkDeviceMemory	dynamicMeshDeviceMemory;
    u8 *			persistentMappedDynamicMeshMemory;

    VkDeviceSize	dynamicMeshFrameCapacity;
    VkDeviceSize	dynamicMeshFrameStart[VIRTUAL_FRAME_COUNT];
    VkDeviceSize	dynamicMeshFrameUsed[VIRTUAL_FRAME_COUNT];

    // Todo(Leo): Use our own arena arrays for these.
    // Todo(Leo): That requires access to persistent memory block at platform layer
    // Todo(Leo): Also make these per "scene" thing, so we can unload stuff per scene easily
    std::vector<VulkanMesh>  	loadedMeshes;
	std::vector<VulkanTexture> 	loadedTextures;
	std::vector<VulkanMaterial>	loadedMaterials;
	std::vector<VulkanModel>	loadedModels;

	// Note(Leo): Guitextures are separeate becauses they are essentially texture AND material
	// since they are used alone. We should maybe just use normal materials. I don't know yet.
	std::vector<VulkanGuiTexture> 	loadedGuiTextures;


	VulkanPipeline pipelines [GraphicsPipelineCount];

	VkPipeline 				linePipeline;
	VkPipelineLayout 		linePipelineLayout;

	// Todo(Leo): these refer to hdr tonemap/post process pipeline
	VkPipeline 				passThroughPipeline;
	VkPipelineLayout 		passThroughPipelineLayout;
	VkDescriptorSetLayout 	passThroughDescriptorSetLayout; // Todo(Leo): make better name; what descriptor set this is

	// Note(Leo): This is a list of functions to call when destroying this.
	// Todo(Leo): Do not use std::vector; we know explicitly how many we have at compile time
    using CleanupFunc 					= void (VulkanContext*);
	std::vector<CleanupFunc*> cleanups 	= {};

    u32 currentDrawFrameIndex;
    bool32 canDraw = false;

    bool32 sceneUnloaded = false;

    VkBuffer 		leafBuffer;
    VkDeviceMemory 	leafBufferMemory;
    VkDeviceSize 	leafBufferCapacity;
    VkDeviceSize 	leafBufferUsed[VIRTUAL_FRAME_COUNT];
    u8 * 			persistentMappedLeafBufferMemory;

    // HÄXÖR SKY
    // Todo(Leo): make smarter
    VkDescriptorSetLayout 	skyGradientDescriptorSetLayout;
    VkDescriptorSet 		skyGradientDescriptorSet;
};

// Note(Leo): We are expecting to at some point need to get things from multiple different
// containers, which is easier with helper function.
// Todo(Leo): any case, think through these at some point

internal VulkanGuiTexture & fsvulkan_get_loaded_gui_texture (VulkanContext & context, GuiTextureHandle id)
{
	return context.loadedGuiTextures[id];
}


// TODo(Leo): inline in render function
internal inline VulkanVirtualFrame *
fsvulkan_get_current_virtual_frame(VulkanContext * context)
{
	return &context->virtualFrames[context->virtualFrameIndex];
}

internal inline VulkanTexture *
fsvulkan_get_loaded_texture(VulkanContext * context, TextureHandle handle)
{
    return &context->loadedTextures[handle];
}

internal inline VulkanMaterial *
fsvulkan_get_loaded_material(VulkanContext * context, MaterialHandle handle)
{
	return &context->loadedMaterials[handle];
}

internal inline VulkanMesh *
fsvulkan_get_loaded_mesh(VulkanContext * context, MeshHandle handle)
{
	return &context->loadedMeshes[handle];
}

namespace vulkan
{
	// Todo(Leo): inline in init function
    internal VkFormat find_supported_depth_format(VkPhysicalDevice physicalDevice);
	internal u32 find_memory_type ( VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties);

	internal VulkanQueueFamilyIndices find_queue_families (VkPhysicalDevice device, VkSurfaceKHR surface);


#if FS_VULKAN_USE_VALIDATION
	constexpr bool32 enableValidationLayers = true;

	constexpr char const * const validationLayers[] =
	{
	    "VK_LAYER_KHRONOS_validation"
	};
	constexpr int VALIDATION_LAYERS_COUNT = array_count(validationLayers);

#else
	constexpr bool32 enableValidationLayers 	= false;

	constexpr char const ** validationLayers 	= nullptr;
	constexpr int VALIDATION_LAYERS_COUNT 		= 0;


#endif

	constexpr char const * const deviceExtensions [] =
	{
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	constexpr int DEVICE_EXTENSION_COUNT = array_count(deviceExtensions);


	// Todo(Leo): This needs to be enforced
    constexpr s32 MAX_LOADED_TEXTURES = 100;

	/// INTERNAL RESOURCES, CUSTOM
	internal void destroy_texture 		(VulkanContext*, VulkanTexture * texture);

	/// INTERNAL RESOURCES, VULKAN TYPES
    internal VkRenderPass 			make_vk_render_pass(VulkanContext*, VkFormat format, VkSampleCountFlagBits msaaSamples);

	internal VkFramebuffer 			make_vk_framebuffer(VkDevice, VkRenderPass, u32 attachmentCount, VkImageView * attachments, u32 width, u32 height);
	internal VkImage 				make_vk_image(	VulkanContext*, u32 width, u32 height, u32 mipLevels, VkFormat format,
											    	VkImageTiling tiling, VkImageUsageFlags usage,
											    	VkSampleCountFlagBits msaaSamples);
	internal VkImageView 			make_vk_image_view(VkDevice device, VkImage image, u32 mipLevels, VkFormat format, VkImageAspectFlags aspectFlags);
	// internal VkSampler				make_vk_sampler(VkDevice, VkSamplerAddressMode);   
}

internal VkDescriptorSetAllocateInfo
fsvulkan_descriptor_set_allocate_info(VkDescriptorPool descriptorPool, u32 descriptorSetCount, VkDescriptorSetLayout const * setLayouts);

internal VkDescriptorSet make_material_vk_descriptor_set_2(	VulkanContext *			context,
															VkDescriptorSetLayout 	descriptorSetLayout,
															VkImageView 			imageView,
															VkDescriptorPool 		pool,
															VkSampler 				sampler,
															VkImageLayout 			layout);






#define WIN_VULKAN_HPP
#endif