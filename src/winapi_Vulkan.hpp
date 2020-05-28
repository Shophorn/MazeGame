/*=============================================================================
Leo Tamminen
shophorn @ internet


Windows-Vulkan interface. And lots of rubbish at the moment.

STUDY: https://devblogs.nvidia.com/vulkan-dos-donts/
=============================================================================*/

#ifndef WIN_VULKAN_HPP

internal void 
print_vulkan_assert(LogInput::FileAddress address, VkResult result)
{
    logVulkan(0) << address << "Vulkan check failed " << vulkan::to_str(result) << "(" << result << ")\n";
}

#define VULKAN_CHECK(result) if (result != VK_SUCCESS) { print_vulkan_assert(FILE_ADDRESS, result); abort();}

static void check_vk_result(VkResult result)
{
	VULKAN_CHECK(result);
	    // if (err == 0) return;
	    // printf("VkResult %d\n", err);
	    // if (err < 0)
	    //     abort();
}

constexpr s32 VIRTUAL_FRAME_COUNT = 3;


constexpr u64 VULKAN_NO_TIME_OUT = math::highest_value<u64>;
constexpr f32 VULKAN_MAX_LOD_FLOAT = 100.0f;

constexpr s32 VULKAN_MAX_MODEL_COUNT = 2000;
constexpr s32 VULKAN_MAX_MATERIAL_COUNT = 100;


constexpr VkSampleCountFlagBits VULKAN_MAX_MSAA_SAMPLE_COUNT = VK_SAMPLE_COUNT_2_BIT;

// Note(Leo): these need to align properly

namespace vulkan
{
	struct CameraUniformBuffer
	{
		alignas(16) m44 view;
		alignas(16) m44 projection;
		alignas(16) m44 lightViewProjection;
		alignas(16) f32 shadowDistance;
		f32 shadowTransitionDistance;
	};

	struct LightingUniformBuffer
	{
		v4 direction;
		v4 color;
		v4 ambient;
	};
	
	struct ModelUniformBuffer
	{
		// Note(Leo): matrices must be aligned on 16 byte boundaries
		// Todo(Leo): Find the confirmation for this from Vulkan documentation
		alignas(16) m44 	localToWorld;
		alignas(16) float 	isAnimated;
		alignas(16) m44 	bonesToLocal [32];
	};
}

struct VulkanBufferResource
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    
    // Todo[memory, vulkan](Leo): IMPORTANT Enforce/track these
    u64 used;
    u64 size;
};

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
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};


struct VulkanTexture
{
	VkImage 		image;
	VkImageView 	view;

	// TODO(Leo): totally not allocate like this, we need texture pool
	VkDeviceMemory memory;
};

struct VulkanMesh
{
    VkBuffer        bufferReference;
    // VkDeviceMemory  memoryReference;

    VkDeviceSize    vertexOffset;
    VkDeviceSize    indexOffset;
    u32             indexCount;
    VkIndexType     indexType;
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

struct VulkanLoadedPipeline
{
	// Note(Leo): we need info for recreating pipelines after swapchain recreation
	VkPipeline 				pipeline;
	VkPipelineLayout 		layout;
	VkDescriptorSetLayout	materialLayout;
};

struct FSVulkanPipeline
{
	VkPipeline 				pipeline;
	VkPipelineLayout 		pipelineLayout;
	VkDescriptorSetLayout 	descriptorSetLayout;
	s32 					textureCount;
};

struct VulkanVirtualFrame
{
	struct
	{
		VkCommandBuffer master;
		VkCommandBuffer scene;
		VkCommandBuffer gui;

        VkCommandBuffer offscreen;

		// Todo(Leo): Do we want this too?
		// VkCommandBuffer debug;
	} commandBuffers;

	VkFramebuffer  framebuffer;
    VkFramebuffer  shadowFramebuffer;

    VkSemaphore 	shadowPassWaitSemaphore;
	VkSemaphore    imageAvailableSemaphore;
	VkSemaphore    renderFinishedSemaphore;
	VkFence        inFlightFence; // Todo(Leo): Change to queuesubmitfence or commandbufferfence etc..
};


using VulkanContext = platform::Graphics;

struct platform::Graphics
{
	VkInstance 						instance;
	VkDevice 						device; // Note(Leo): this is Logical Device.
	VkPhysicalDevice 				physicalDevice;
	VkPhysicalDeviceProperties 		physicalDeviceProperties;
	VkSurfaceKHR 					surface;

	struct {
		VkQueue graphics;
		VkQueue present;

		u32 graphicsIndex;
		u32 presentIndex;
	} queues;

    VulkanVirtualFrame virtualFrames [VIRTUAL_FRAME_COUNT];
    u32 virtualFrameIndex = 0;

    struct
    {
    	VkDescriptorPool uniform;
    	VkDescriptorPool material;

    	// Note(Leo): these are not cleared on unload
    	VkDescriptorPool persistent;
    } descriptorPools;

    struct
    {
    	VkDescriptorSetLayout camera;
    	VkDescriptorSetLayout model;
    	VkDescriptorSetLayout lighting;
    	VkDescriptorSetLayout shadowMap;
    } descriptorSetLayouts;

    struct
    {
   	 	VkDescriptorSet camera;
	    VkDescriptorSet model;
   	 	VkDescriptorSet lighting;
   	 	VkDescriptorSet lightCamera;
    } uniformDescriptorSets;

	VkDescriptorSet shadowMapDescriptorSet;

    // Todo(Leo): set these dynamically
    u32 cameraUniformOffset = 0;
    u32 lightingUniformOffset = 256;
    u32 lightCameraUniformOffset = 512;

    // Uncategorized
	VkCommandPool 			commandPool;
    VkSampleCountFlagBits 	msaaSamples;
    VkSampler 				textureSampler;			

    /* Note(Leo): color and depth images for initial writing. These are
    afterwards resolved to actual framebufferimage */

	struct {
	    VkSwapchainKHR 	swapchain;
	    VkExtent2D 		extent;

	    VkFormat 					imageFormat;
	    std::vector<VkImage> 		images;
	    std::vector<VkImageView> 	imageViews;

	    VkRenderPass renderPass;

	    // Note(Leo): these are attchaments.
		VkDeviceMemory memory;

		VkImage 	colorImage;
		VkImageView colorImageView;

		VkImage 	depthImage;	
		VkImageView depthImageView;
	} drawingResources = {};


	struct
	{
		u32 width;
		u32 height;

		VulkanTexture 		attachment;
		VkSampler 			sampler; 

		VkRenderPass 		renderPass;
		VkFramebuffer 		framebuffer;

		VkPipelineLayout 	layout;
		VkPipeline 			pipeline;

	} shadowPass;

	VkDescriptorSet 		shadowMapTexture;

    VulkanBufferResource stagingBufferPool;
    VulkanBufferResource staticMeshPool;
    VulkanBufferResource modelUniformBuffer;
    VulkanBufferResource sceneUniformBuffer;
   
    // Todo(Leo): Use our own arena arrays for these.
    // Todo(Leo): Expose a function to game layer to reserve memory for these
    std::vector<VulkanMesh>  			loadedMeshes;
	std::vector<VulkanTexture> 			loadedTextures;
	std::vector<VulkanMaterial>			loadedMaterials;
	std::vector<VulkanModel>			loadedModels;


	FSVulkanPipeline pipelines [GRAPHICS_PIPELINE_COUNT];

	VkPipeline 				linePipeline;
	VkPipelineLayout 		linePipelineLayout;
	VkDescriptorSetLayout 	linePipelineDescriptorSetLayout;

	// Note(Leo): This is a list of functions to call when destroying this.
    using CleanupFunc = void (VulkanContext*);
	std::vector<CleanupFunc*> cleanups = {};

    // u32 currentDrawImageIndex;
    u32 currentDrawFrameIndex;
    bool32 canDraw = false;
    u32 currentUniformBufferOffset;

    bool32 sceneUnloaded = false;
};


namespace vulkan
{
	internal void
	advance_virtual_frame(VulkanContext * context)
	{
		context->virtualFrameIndex += 1;
		context->virtualFrameIndex %= VIRTUAL_FRAME_COUNT;
	}

	internal inline VulkanVirtualFrame *
	get_current_virtual_frame(VulkanContext * context)
	{
		return &context->virtualFrames[context->virtualFrameIndex];
	}

	internal inline VulkanTexture *
	get_loaded_texture(VulkanContext * context, TextureHandle handle)
	{
	    return &context->loadedTextures[handle];
	}

	internal inline VulkanMaterial *
	get_loaded_material(VulkanContext * context, MaterialHandle handle)
	{
		return &context->loadedMaterials[handle];
	}

	internal inline VulkanMesh *
	get_loaded_mesh(VulkanContext * context, MeshHandle handle)
	{
		return &context->loadedMeshes[handle];
	}

    internal VkFormat find_supported_format(    VkPhysicalDevice physicalDevice,
                                                s32 candidateCount,
                                                VkFormat * pCandidates,
                                                VkImageTiling requestedTiling,
                                                VkFormatFeatureFlags requestedFeatures);
    internal VkFormat find_supported_depth_format(VkPhysicalDevice physicalDevice);
	internal u32 find_memory_type ( VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties);

	#if MAZEGAME_DEVELOPMENT
	constexpr bool32 enableValidationLayers = true;
	#else
	constexpr bool32 enableValidationLayers = false;
	#endif

	constexpr char const * validationLayers[] = {
	    "VK_LAYER_KHRONOS_validation"
	};
	constexpr int VALIDATION_LAYERS_COUNT = array_count(validationLayers);

	constexpr char const * deviceExtensions [] = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	constexpr int DEVICE_EXTENSION_COUNT = array_count(deviceExtensions);

    constexpr s32 MAX_LOADED_TEXTURES = 100;

    enum : u32
    {
    	SHADER_LIGHTING_SET = 3
    };


    // Todo(Leo): this is internal to the some place
	internal VkIndexType convert_index_type(IndexType);

	// HELPER FUNCTIONS
	internal VulkanQueueFamilyIndices find_queue_families (VkPhysicalDevice device, VkSurfaceKHR surface);
	internal VkPresentModeKHR choose_surface_present_mode(std::vector<VkPresentModeKHR> & availablePresentModes);
	internal VulkanSwapchainSupportDetails query_swap_chain_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	internal VkSurfaceFormatKHR choose_swapchain_surface_format(std::vector<VkSurfaceFormatKHR>& availableFormats);

    internal inline bool32
    has_stencil_component(VkFormat format)
    {
        bool32 result = (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT);
        return result;
    }

    internal inline VkDeviceSize
    get_aligned_uniform_buffer_size(VulkanContext * context, VkDeviceSize size)
    {
    	auto alignment = context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
    	return align_up(size, alignment);
    } 

    internal VkCommandBuffer begin_command_buffer(VkDevice, VkCommandPool);
    internal void execute_command_buffer(VkCommandBuffer, VkDevice, VkCommandPool, VkQueue);
	internal void cmd_transition_image_layout(	VkCommandBuffer commandBuffer,
											    VkDevice        device,
											    VkQueue         graphicsQueue,
											    VkImage         image,
											    VkFormat        format,
											    u32             mipLevels,
											    VkImageLayout   oldLayout,
											    VkImageLayout   newLayout,
											    u32             layerCount = 1);


    /// INITIALIZATION CALLS
	internal void create_drawing_resources		(VulkanContext*, u32 width, u32 height);
	internal void recreate_drawing_resources 	(VulkanContext*, u32 width, u32 height);
	internal void destroy_drawing_resources 	(VulkanContext*);					

	/// INTERNAL RESOURCES, CUSTOM
	internal void destroy_texture 		(VulkanContext*, VulkanTexture * texture);

	internal VulkanBufferResource make_buffer_resource(	VulkanContext*,
														VkDeviceSize size,
														VkBufferUsageFlags usage,
														VkMemoryPropertyFlags memoryProperties);
	internal void destroy_buffer_resource(VkDevice device, VulkanBufferResource * resource);

	/// INTERNAL RESOURCES, VULKAN TYPES
    internal VkRenderPass 			make_vk_render_pass(VulkanContext*, VkFormat format, VkSampleCountFlagBits msaaSamples);
	internal VkDescriptorSetLayout 	make_material_vk_descriptor_set_layout(VkDevice device, u32 textureCount);

	internal VkFramebuffer 			make_vk_framebuffer(VkDevice, VkRenderPass, u32 attachmentCount, VkImageView * attachments, u32 width, u32 height);
	internal VkShaderModule 		make_vk_shader_module(BinaryAsset file, VkDevice logicalDevice);
	internal VkImage 				make_vk_image(	VulkanContext*, u32 width, u32 height, u32 mipLevels, VkFormat format,
											    	VkImageTiling tiling, VkImageUsageFlags usage,
											    	VkSampleCountFlagBits msaaSamples);
	internal VkImageView 			make_vk_image_view(VkDevice device, VkImage image, u32 mipLevels, VkFormat format, VkImageAspectFlags aspectFlags);
	internal VkSampler				make_vk_sampler(VkDevice, VkSamplerAddressMode);

	/// DRAWING, VulkanDrawing.cpp
	internal void prepare_drawing			(VulkanContext*);
	internal void finish_drawing 			(VulkanContext*);
    internal void update_camera				(VulkanContext*, Camera const *);
    internal void update_lighting			(VulkanContext*, Light const *, Camera const *, v3 ambient);
	internal void record_draw_command 		(VulkanContext*, ModelHandle handle, m44 transform, bool32 castShadow, m44 const * bones, u32 bonesCount);

    internal TextureHandle  push_texture (VulkanContext * context, TextureAsset * texture);
    internal MaterialHandle push_material (VulkanContext * context, MaterialAsset * asset);
    internal MeshHandle     push_mesh(VulkanContext * context, MeshAsset * mesh);
    internal ModelHandle    push_model (VulkanContext * context, MeshHandle mesh, MaterialHandle material);
    internal TextureHandle  push_cubemap(VulkanContext * context, StaticArray<TextureAsset, 6> * assets);
    internal void           unload_scene(VulkanContext * context);
    
    internal VulkanTexture  make_texture(VulkanContext * context, TextureAsset * asset);
    internal VulkanTexture  make_texture(VulkanContext * context, u32 width, u32 height, v4 color, VkFormat format);
    internal VulkanTexture  make_cubemap(VulkanContext * context, StaticArray<TextureAsset, 6> * assets);        
}

internal VkDescriptorSet fsvulkan_make_texture_descriptor_set(	VulkanContext*,
														VkDescriptorSetLayout,
														VkDescriptorPool,
														s32 			textureCount,
														TextureHandle * textures);

internal MaterialHandle fsvulkan_push_material(VulkanContext*, GraphicsPipeline, s32 textureCount, TextureHandle * textures);

internal void fsvulkan_draw_meshes			(VulkanContext * context, s32 count, m44 const * transforms, MeshHandle, MaterialHandle);
internal void fsvulkan_draw_screen_rects	(VulkanContext * context, s32 count, ScreenRect const * rects, MaterialHandle material, v4 color);
internal void fsvulkan_draw_lines			(VulkanContext*, s32 count, v3 const * points, v4 color);

internal TextureHandle fsvulkan_init_shadow_pass (VulkanContext*);

platform::FrameResult
platform::prepare_frame(VulkanContext * context)
{
	VulkanVirtualFrame * frame = vulkan::get_current_virtual_frame(context);

    vkWaitForFences(context->device, 1, &frame->inFlightFence, VK_TRUE, VULKAN_NO_TIME_OUT);

    VkResult result = vkAcquireNextImageKHR(context->device,
                                            context->drawingResources.swapchain,
                                            VULKAN_NO_TIME_OUT,//MaxValue<u64>,
                                            frame->imageAvailableSemaphore,
                                            VK_NULL_HANDLE,
                                            &context->currentDrawFrameIndex);
    switch(result)
    {
    	case VK_SUCCESS:
    		return platform::FRAME_OK;

	    case VK_SUBOPTIMAL_KHR:
	    case VK_ERROR_OUT_OF_DATE_KHR:
	    	return platform::FRAME_RECREATE;

	    default:
	    	return platform::FRAME_BAD_PROBLEM;
    }
}

void
platform::set_functions(VulkanContext * context, platform::Functions * api)
{
 	api->push_mesh          = vulkan::push_mesh;
 	api->push_texture       = vulkan::push_texture;
 	api->push_cubemap       = vulkan::push_cubemap;
 	api->push_material      = fsvulkan_push_material;

 	api->push_model         = vulkan::push_model;
 	api->unload_scene       = vulkan::unload_scene;
 
 	api->init_shadow_pass 	= fsvulkan_init_shadow_pass;

 	api->prepare_frame      = vulkan::prepare_drawing;
 	api->finish_frame       = vulkan::finish_drawing;
 	api->update_camera      = vulkan::update_camera;
 	api->update_lighting	= vulkan::update_lighting;
 	api->draw_model         = vulkan::record_draw_command;

 	api->draw_meshes 			= fsvulkan_draw_meshes;
 	api->draw_screen_rects 		= fsvulkan_draw_screen_rects;
 	api->draw_lines 			= fsvulkan_draw_lines;
}

#define WIN_VULKAN_HPP
#endif