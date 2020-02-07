/*=============================================================================
Leo Tamminen
shophorn @ internet


Windows-Vulkan interface. And lots of rubbish at the moment.

STUDY: https://devblogs.nvidia.com/vulkan-dos-donts/
=============================================================================*/

#ifndef WIN_VULKAN_HPP

internal void 
print_vulkan_assert(char const * file, s32 line, VkResult result)
{
    std::cout << "Vulkan check failed [" << file << ":" << line << "]: " << vulkan::to_str(result) << "(" << result << ")\n";
}

#define VULKAN_CHECK(result) if (result != VK_SUCCESS) { print_vulkan_assert(__FILE__, __LINE__, result); abort();}


constexpr s32 VIRTUAL_FRAME_COUNT = 3;


constexpr u64 VULKAN_NO_TIME_OUT	= math::highest_value<u64>;
constexpr real32 VULKAN_MAX_LOD_FLOAT = 100.0f;

constexpr s32 VULKAN_MAX_MODEL_COUNT = 2000;
constexpr s32 VULKAN_MAX_MATERIAL_COUNT = 100;


constexpr VkSampleCountFlagBits VULKAN_MAX_MSAA_SAMPLE_COUNT = VK_SAMPLE_COUNT_1_BIT;

// Note(Leo): these need to align properly
struct VulkanCameraUniformBufferObject
{
	alignas(16) Matrix44 view;
	alignas(16) Matrix44 projection;
	Matrix44 lightViewProjection;
};

namespace vulkan
{
	struct LightingUniformBufferObject
	{
		vector4 direction;
		float4 color;
		float4 ambient;
	};
}

// Todo[rendering] (Leo): Use this VulkanModelUniformBufferObject
// struct VulkanModelUniformBufferObject
// {
// 	alignas(16) Matrix44 modelMatrix;
// };



// Todo (Leo): important this is used like an memory arena somewhere, and like somthing else elsewhere.
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
    VkBuffer        buffer;
    VkDeviceMemory  memory;

    VkDeviceSize    vertexOffset;
    VkDeviceSize    indexOffset;
    u32             indexCount;
    VkIndexType     indexType;
};

struct VulkanMaterial
{
	PipelineHandle pipeline;
	VkDescriptorSet descriptorSet;
};

/* Todo(Leo): this is now redundant it can go away.
Then we can simply draw by mesh and material anytime */
struct VulkanModel
{
	MeshHandle 		mesh;
	MaterialHandle 	material;
};

struct VulkanPipelineLoadInfo
{
	std::string vertexShaderPath;
	std::string fragmentShaderPath;
	platform::RenderingOptions options;
};

struct VulkanLoadedPipeline
{
	// Note(Leo): we need info for recreating pipelines after swapchain recreation
	VulkanPipelineLoadInfo 	info;
	VkPipeline 				pipeline;
	VkPipelineLayout 		layout;
	VkDescriptorSetLayout	materialLayout;

	// Todo(Leo): use these
	u32 sceneSetIndex;
	u32 modelSetIndex;
	u32 materialSetIndex;
	u32 lightingSetIndex;
};

struct VulkanVirtualFrame
{
	struct
	{
		VkCommandBuffer primary;
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
	} queues;

    VulkanVirtualFrame virtualFrames [VIRTUAL_FRAME_COUNT];
    u32 virtualFrameIndex = 0;

    struct
    {
    	VkDescriptorPool uniform;
    	VkDescriptorPool material;

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

		VulkanTexture attachment;
		VkSampler sampler; 

		VkRenderPass renderPass;
		VkFramebuffer framebuffer;

		VkPipelineLayout 	layout;
		VkPipeline 			pipeline;

		VulkanLoadedPipeline debugView;
	} shadowPass;

    VulkanBufferResource stagingBufferPool;
    VulkanBufferResource staticMeshPool;
    VulkanBufferResource modelUniformBuffer;
    VulkanBufferResource sceneUniformBuffer;
   
    // Todo(Leo): Use our own arena arrays for these.
    // Todo(Leo): Expose a function to game layer to reserve memory for these
    std::vector<VulkanMesh>  			loadedMeshes;
	std::vector<VulkanTexture> 			loadedTextures;
	std::vector<VulkanMaterial>			loadedMaterials;
    std::vector<VulkanLoadedPipeline> 	loadedPipelines;
	std::vector<VulkanModel>			loadedModels;

    VulkanLoadedPipeline 	lineDrawPipeline;
    VulkanMaterial 			lineDrawMaterial;

    VulkanLoadedPipeline		guiDrawPipeline;	
	std::vector<VulkanMaterial> loadedGuiMaterials;
	VulkanMaterial 				defaultGuiMaterial;

	// Note(Leo): This is a list of functions to call when destroying this.
    using CleanupFunc = void (platform::Graphics*);
	std::vector<CleanupFunc*> cleanups = {};

	struct {
		VulkanTexture white;
	} debugTextures;

    // u32 currentDrawImageIndex;
    u32 currentDrawFrameIndex;
    bool32 canDraw = false;
    PipelineHandle currentBoundPipeline;
    u32 currentUniformBufferOffset;

    bool32 sceneUnloaded = false;
};

using VulkanContext = platform::Graphics;

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

	internal inline VulkanLoadedPipeline *
	get_loaded_pipeline(VulkanContext * context, PipelineHandle handle)
	{
		return &context->loadedPipelines[handle];
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

	internal inline VulkanMaterial *
	get_loaded_gui_material(VulkanContext * context, MaterialHandle handle)
	{
		return &context->loadedGuiMaterials[handle];
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
	constexpr int VALIDATION_LAYERS_COUNT = get_array_count(validationLayers);

	constexpr char const * deviceExtensions [] = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	constexpr int DEVICE_EXTENSION_COUNT = get_array_count(deviceExtensions);

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
    	return align_up_to(alignment, size);
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
	internal VulkanLoadedPipeline make_pipeline 		(VulkanContext*, VulkanPipelineLoadInfo);
	internal void recreate_loaded_pipeline				(VulkanContext*, VulkanLoadedPipeline*);
	internal void destroy_loaded_pipeline 				(VulkanContext*, VulkanLoadedPipeline*);

	internal void destroy_texture 		(VulkanContext*, VulkanTexture * texture);

	internal VulkanBufferResource make_buffer_resource(	VulkanContext*,
														VkDeviceSize size,
														VkBufferUsageFlags usage,
														VkMemoryPropertyFlags memoryProperties);
	internal void destroy_buffer_resource(VkDevice device, VulkanBufferResource * resource);

	/// INTERNAL RESOURCES, VULKAN TYPES
    internal VkRenderPass 			make_vk_render_pass(VulkanContext*, VkFormat format, VkSampleCountFlagBits msaaSamples);
	internal VkDescriptorSetLayout 	make_material_vk_descriptor_set_layout(VkDevice device, u32 textureCount);
	internal VkDescriptorSet 		make_material_vk_descriptor_set(VulkanContext*, PipelineHandle pipeline, ArenaArray<TextureHandle> textures);
	internal VkDescriptorSet 		make_material_vk_descriptor_set(VulkanContext*, 
																	VulkanLoadedPipeline * pipeline,
																	VulkanTexture * texture,
																	VkDescriptorPool pool);
	internal VkDescriptorSet 		make_material_vk_descriptor_set(VulkanContext*, 
																	VulkanLoadedPipeline * pipeline,
																	VkImageView imageView,
																	VkDescriptorPool pool);
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
    internal void update_lighting			(VulkanContext*, Light const *, Camera const *, float3 ambient);
	internal void record_draw_command 		(VulkanContext*, ModelHandle handle, Matrix44 transform, bool32 castShadow);
	internal void record_line_draw_command	(VulkanContext*, vector3 start, vector3 end, float width, float4 color);
	internal void record_gui_draw_command	(VulkanContext*, vector2 position, vector2 size, MaterialHandle material, float4 color);

    internal TextureHandle  push_texture (VulkanContext * context, TextureAsset * texture);
    internal MaterialHandle push_material (VulkanContext * context, MaterialAsset * asset);
    internal MaterialHandle push_gui_material (VulkanContext * context, TextureHandle texture);
    internal MeshHandle     push_mesh(VulkanContext * context, MeshAsset * mesh);
    internal ModelHandle    push_model (VulkanContext * context, MeshHandle mesh, MaterialHandle material);
    internal TextureHandle  push_cubemap(VulkanContext * context, StaticArray<TextureAsset, 6> * assets);
    internal PipelineHandle push_pipeline(VulkanContext * context, const char * vertexShaderPath, const char * fragmentShaderPath, platform::RenderingOptions options);
    internal void           unload_scene(VulkanContext * context);
    
    internal VulkanTexture  make_texture(VulkanContext * context, TextureAsset * asset);
    internal VulkanTexture  make_texture(VulkanContext * context, u32 width, u32 height, float4 color, VkFormat format);
    internal VulkanTexture  make_cubemap(VulkanContext * context, StaticArray<TextureAsset, 6> * assets);        

}



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
platform::set_functions(VulkanContext * context, platform::Functions * functions)
{
 	functions->push_mesh           	= vulkan::push_mesh;
 	functions->push_texture        	= vulkan::push_texture;
 	functions->push_cubemap        	= vulkan::push_cubemap;
 	functions->push_material       	= vulkan::push_material;
 	functions->push_gui_material   	= vulkan::push_gui_material;
 	functions->push_model          	= vulkan::push_model;
 	functions->push_pipeline       	= vulkan::push_pipeline;
 	functions->unload_scene        	= vulkan::unload_scene;
    
 	functions->prepare_frame       	= vulkan::prepare_drawing;
 	functions->finish_frame        	= vulkan::finish_drawing;
 	functions->update_camera       	= vulkan::update_camera;
 	functions->update_lighting		= vulkan::update_lighting;
 	functions->draw_model         	= vulkan::record_draw_command;
 	functions->draw_line          	= vulkan::record_line_draw_command;
 	functions->draw_gui           	= vulkan::record_gui_draw_command;
}

#define WIN_VULKAN_HPP
#endif