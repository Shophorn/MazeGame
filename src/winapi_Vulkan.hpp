/*=============================================================================
Leo Tamminen
shophorn @ internet


Windows-Vulkan interface. And lots of rubbish at the moment.

STUDY: https://devblogs.nvidia.com/vulkan-dos-donts/
=============================================================================*/

#ifndef WIN_VULKAN_HPP

using VulkanContext = PlatformGraphics;

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
		v4 cameraPosition;
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
    VkDeviceSize used;
    VkDeviceSize size;
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
    VkSurfaceCapabilitiesKHR 		capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> 	presentModes;
};


struct VulkanTexture
{
	VkImage 		image;
	VkImageView 	view;

	// TODO(Leo): totally not allocate like this, we need texture pool
	VkDeviceMemory memory;
};

struct VulkanGuiTexture
{
	VulkanTexture 	texture;
	VkDescriptorSet descriptorSet;
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

	using PostRenderFunc = void(VulkanContext*);
	PostRenderFunc * onPostRender;


    VulkanBufferResource stagingBufferPool;
    VulkanBufferResource staticMeshPool;
    VulkanBufferResource modelUniformBuffer;
    VulkanBufferResource sceneUniformBuffer;

    // Todo(Leo): Use our own arena arrays for these.
    // Todo(Leo): That requires access to persistent memory block at platform layer
    // Todo(Leo): Also make these per "scene" thing, so we can unload stuff per scene easily
    std::vector<VulkanMesh>  	loadedMeshes;
	std::vector<VulkanTexture> 	loadedTextures;
	std::vector<VulkanMaterial>	loadedMaterials;
	std::vector<VulkanModel>	loadedModels;

	std::vector<VulkanGuiTexture> 	loadedGuiTextures;
	std::vector<VulkanTexture> 		loadedGuiTexturesTextures;
	std::vector<VkDescriptorSet> 	loadedGuiTexturesDescriptorSets;

	VkDescriptorSet 		shadowMapTexture;

	FSVulkanPipeline pipelines [GRAPHICS_PIPELINE_COUNT];

	VkPipeline 				linePipeline;
	VkPipelineLayout 		linePipelineLayout;
	VkDescriptorSetLayout 	linePipelineDescriptorSetLayout;

	VkPipeline 			leavesShadowPipeline;
	VkPipelineLayout 	leavesShadowPipelineLayout;

	// Note(Leo): This is a list of functions to call when destroying this.
    using CleanupFunc = void (VulkanContext*);
	std::vector<CleanupFunc*> cleanups = {};

    // u32 currentDrawImageIndex;
    u32 currentDrawFrameIndex;
    bool32 canDraw = false;
    // u32 currentUniformBufferOffset;

    bool32 sceneUnloaded = false;

    VkBuffer 		leafBuffer;
    VkDeviceMemory 	leafBufferMemory;
    VkDeviceSize 	leafBufferCapacity;
    VkDeviceSize 	leafBufferUsed[VIRTUAL_FRAME_COUNT];
    void * 			persistentMappedLeafBufferMemory;

};



// Note(Leo): We are expecting to at some point need to get things from multiple different
// containers, which is easier with helper function.
internal VulkanGuiTexture & fsvulkan_get_loaded_gui_texture (VulkanContext & context, GuiTextureHandle id)
{
	return context.loadedGuiTextures[id];
}


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

    constexpr s32 MAX_LOADED_TEXTURES = 100;

    enum : u32
    {
    	SHADER_LIGHTING_SET = 3
    };


    // Todo(Leo): this is internal to the some place
	internal VkIndexType convert_index_type(IndexType);

	// HELPER FUNCTIONS
	internal VulkanQueueFamilyIndices find_queue_families (VkPhysicalDevice device, VkSurfaceKHR surface);

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

    internal VulkanTexture  make_texture(VulkanContext * context, TextureAsset * asset);
    internal VulkanTexture  make_cubemap(VulkanContext * context, StaticArray<TextureAsset, 6> * assets);        
}

internal void fsvulkan_recreate_drawing_resources 	(VulkanContext*, u32 width, u32 height);


internal VkDescriptorSet fsvulkan_make_texture_descriptor_set(	VulkanContext*,
																VkDescriptorSetLayout,
																VkDescriptorPool,
																s32 			textureCount,
																VkImageView * 	textures);

internal GuiTextureHandle fsvulkan_resources_push_gui_texture(VulkanContext * context, TextureAsset * asset);
internal MaterialHandle fsvulkan_resources_push_material(VulkanContext*, GraphicsPipeline, s32 textureCount, TextureHandle * textures);

internal TextureHandle fsvulkan_init_shadow_pass (VulkanContext*);

internal void fsvulkan_initialize_normal_pipeline(VulkanContext & context);
internal void fsvulkan_initialize_animated_pipeline(VulkanContext & context);
internal void fsvulkan_initialize_skybox_pipeline(VulkanContext & context);
internal void fsvulkan_initialize_screen_gui_pipeline(VulkanContext & context);
internal void fsvulkan_initialize_line_pipeline(VulkanContext & context);
internal void fsvulkan_initialize_leaves_pipeline(VulkanContext & context);

internal VkDescriptorSet make_material_vk_descriptor_set_2(	VulkanContext *			context,
															VkDescriptorSetLayout 	descriptorSetLayout,
															VkImageView 			imageView,
															VkDescriptorPool 		pool,
															VkSampler 				sampler,
															VkImageLayout 			layout);

internal void fsvulkan_reload_shaders(VulkanContext * context)
{
	system("compile-shaders.py");

	context->onPostRender = [](VulkanContext * context)
	{
		VkDevice device = context->device;

		for (s32 i = 0; i < GRAPHICS_PIPELINE_COUNT; ++i)
		{
			vkDestroyDescriptorSetLayout(device, context->pipelines[i].descriptorSetLayout, nullptr);
			vkDestroyPipelineLayout(device, context->pipelines[i].pipelineLayout, nullptr);
			vkDestroyPipeline(device, context->pipelines[i].pipeline, nullptr);				
		}

		vkDestroyDescriptorSetLayout(device, context->linePipelineDescriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(device, context->linePipelineLayout, nullptr);
		vkDestroyPipeline(device, context->linePipeline, nullptr);

		fsvulkan_initialize_normal_pipeline(*context);
		fsvulkan_initialize_animated_pipeline(*context);
		fsvulkan_initialize_skybox_pipeline(*context);
		fsvulkan_initialize_screen_gui_pipeline(*context);
		fsvulkan_initialize_line_pipeline(*context);
		fsvulkan_initialize_leaves_pipeline(*context);

		context->shadowMapTexture = make_material_vk_descriptor_set_2( 	context,
																		context->pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].descriptorSetLayout,
																		context->shadowPass.attachment.view,
																		context->descriptorPools.persistent,
																		context->shadowPass.sampler,
																		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	};
}

internal PlatformGraphicsFrameResult fsvulkan_prepare_frame(VulkanContext * context)
{
	VulkanVirtualFrame * frame = vulkan::get_current_virtual_frame(context);

    VULKAN_CHECK(vkWaitForFences(context->device, 1, &frame->inFlightFence, VK_TRUE, VULKAN_NO_TIME_OUT));

    VkResult result = vkAcquireNextImageKHR(context->device,
                                            context->drawingResources.swapchain,
                                            VULKAN_NO_TIME_OUT,//MaxValue<u64>,
                                            frame->imageAvailableSemaphore,
                                            VK_NULL_HANDLE,
                                            &context->currentDrawFrameIndex);
    switch(result)
    {
    	case VK_SUCCESS:
    		return PGFR_FRAME_OK;

	    case VK_SUBOPTIMAL_KHR:
	    case VK_ERROR_OUT_OF_DATE_KHR:
	    	return PGFR_FRAME_RECREATE;

	    default:
	    	return PGFR_FRAME_BAD_PROBLEM;
    }
}

internal VkDeviceSize fsvulkan_get_uniform_memory(VulkanContext & context, VkDeviceSize size)
{
	// Note(Leo): offset data for different frames, so we do not overwrite previous frames' data before they are done.
	// Todo(Leo): make separate buffers for each frame
	VkDeviceSize frameSize = vulkan::get_aligned_uniform_buffer_size(&context, megabytes(150));
	VkDeviceSize frameOffset = frameSize * context.virtualFrameIndex;

	auto alignment 	= context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	size 			= align_up(size, alignment);

	VkDeviceSize currentOffset = context.modelUniformBuffer.used;

	Assert(currentOffset + size <= frameSize && "Trying to use too much memory.");

	context.modelUniformBuffer.used += size;


	return currentOffset + frameOffset;
};

internal void fsvulkan_reset_uniform_buffer(VulkanContext & context)
{
	/// TODO(Leo): This is used incorrectly. We should zero some other value, since this buffer is in 
	// practice split to three or two parts, but currently we use this same 'used' value to track each.
	// It luckily works because they are not accessed at same times.
	context.modelUniformBuffer.used = 0;
}

#define WIN_VULKAN_HPP
#endif