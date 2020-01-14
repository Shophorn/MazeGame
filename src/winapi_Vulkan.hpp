/*=============================================================================
Leo Tamminen
shophorn @ internet


Windows-Vulkan interface. And lots of rubbish at the moment.

STUDY: https://devblogs.nvidia.com/vulkan-dos-donts/
=============================================================================*/

#ifndef WIN_VULKAN_HPP

// constexpr int32 MAX_FRAMES_IN_FLIGHT = 2;
// constexpr int32 VIRTUAL_FRAME_COUNT = 2;



constexpr uint64 VULKAN_NO_TIME_OUT	= MaxValue<uint64>;
constexpr real32 VULKAN_MAX_LOD_FLOAT = 100.0f;

constexpr int32 VULKAN_MAX_MODEL_COUNT = 2000;
constexpr int32 VULKAN_MAX_MATERIAL_COUNT = 100;


constexpr VkSampleCountFlagBits VULKAN_MAX_MSAA_SAMPLE_COUNT = VK_SAMPLE_COUNT_1_BIT;

// Note(Leo): these need to align properly
struct VulkanCameraUniformBufferObject
{
	alignas(16) Matrix44 view;
	alignas(16) Matrix44 perspective;
};

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

struct VulkanSwapchainItems
{
    VkSwapchainKHR swapchain;
    VkExtent2D extent;

    VkFormat imageFormat;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
};

struct VulkanDrawingResources
{
	VkDeviceMemory memory;

	VkImage colorImage;
	VkImageView colorImageView;

	VkImage depthImage;	
	VkImageView depthImageView;
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
    uint32          indexCount;
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
	platform::PipelineOptions options;
};

struct VulkanLoadedPipeline
{
	// Note(Leo): we need info for recreating pipelines after swapchain recreation
	VulkanPipelineLoadInfo 	info;
	VkPipeline 				pipeline;
	VkPipelineLayout 		layout;
	VkDescriptorSetLayout	materialLayout;
};

struct VulkanVirtualFrame
{
	VkCommandBuffer commandBuffer;

	VkSemaphore 	imageAvailableSemaphore;
	VkSemaphore 	renderFinishedSemaphore;
	VkFence 		inFlightFence;
	
	VkFramebuffer 	frameBuffer;
};

struct platform::GraphicsContext
{
	VkInstance 						instance;
	VkDevice 						device; // Note(Leo): this is Logical Device.
	VkPhysicalDevice 				physicalDevice;
	VkPhysicalDeviceProperties 		physicalDeviceProperties;

	VkSurfaceKHR 					surface;
	VkQueue 						graphicsQueue;
	VkQueue 						presentQueue;

	VkCommandPool 					commandPool;
    
    std::vector<VkFramebuffer>      frameBuffers;

    // std::vector<VkCommandBuffer>    frameCommandBuffers;
    // std::vector<VkSemaphore>    	imageAvailableSemaphores;
    // std::vector<VkSemaphore>    	renderFinishedSemaphores;
    // std::vector<VkFence>        	inFlightFences;

    std::vector<VulkanVirtualFrame> virtualFrames;

	/* Todo(Leo): There is one layout for each of [models, scene data, materials].*/
    // VkDescriptorSetLayout 	descriptorSetLayouts [3];

    struct
    {
    	VkDescriptorSetLayout scene;
    	VkDescriptorSetLayout model;
    } descriptorSetLayouts;

    struct
    {
	    VkDescriptorSet model;
   	 	VkDescriptorSet scene;
    } uniformDescriptorSets;


    VkRenderPass            renderPass;
    VulkanSwapchainItems 	swapchainItems;

    // MULTISAMPLING
    VkSampleCountFlagBits msaaSamples;

    /* Note(Leo): color and depth images for initial writing. These are
    afterwards resolved to actual framebufferimage */
    VulkanDrawingResources drawingResources;

    /* Note(Leo): image and sampler are now (as in Vulkan) unrelated, and same
    sampler can be used with multiple images, noice */
    VkSampler textureSampler;			

    /* Note(Leo): uniform descriptor pool os recreated on swapchain recreate,
	since they are written and read each frame and thus there needs to be one for
	every swapchain image. Materials are only read each frame so one set of those
	is sufficient. However there is one set per material, so the pool is bigger.*/
    VkDescriptorPool      		uniformDescriptorPool;
    VkDescriptorPool      		materialDescriptorPool;

    VulkanBufferResource       	stagingBufferPool;
    
    // Note(Leo): After this these need to be recreated on unload as empty containers
    VulkanBufferResource 		staticMeshPool;
    VulkanBufferResource 		modelUniformBuffer;
    VulkanBufferResource 		sceneUniformBuffer;
    VulkanBufferResource		guiUniformBuffer;
    

    // Todo(Leo): Use our own arena arrays for these.
    std::vector<VulkanMesh>  			loadedMeshes;
	std::vector<VulkanTexture> 			loadedTextures;
	std::vector<VulkanMaterial>			loadedMaterials;
    std::vector<VulkanLoadedPipeline> 	loadedPipelines;
	std::vector<VulkanModel>	loadedModels;

    VulkanLoadedPipeline 	lineDrawPipeline;
    VulkanMaterial 			lineDrawMaterial;

    // uint32 currentDrawImageIndex;
    uint32 currentDrawFrameIndex;
    bool32 canDraw = false;
    PipelineHandle currentBoundPipeline;
    uint32 currentUniformBufferOffset;

    bool32 abortFrameDrawing = false;
};
using VulkanContext = platform::GraphicsContext;

namespace vulkan
{
	#if MAZEGAME_DEVELOPMENT
	constexpr bool32 enableValidationLayers = true;
	#else
	constexpr bool32 enableValidationLayers = false;
	#endif

	constexpr const char * validationLayers[] = {
	    "VK_LAYER_KHRONOS_validation"
	};
	constexpr int VALIDATION_LAYERS_COUNT = ARRAY_COUNT(validationLayers);

	constexpr const char * deviceExtensions [] = {
	    VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	constexpr int DEVICE_EXTENSION_COUNT = ARRAY_COUNT(deviceExtensions);

    constexpr int32 MAX_LOADED_TEXTURES = 100;

	internal VkVertexInputBindingDescription get_vertex_binding_description ();
	// Todo(Leo): Remove this 'Array', use somthing else
	internal Array<VkVertexInputAttributeDescription, 4> get_vertex_attribute_description();

	internal VkIndexType convert_index_type(IndexType);

	internal uint32	find_memory_type ( 	VkPhysicalDevice physicalDevice,
										uint32 typeFilter,
										VkMemoryPropertyFlags properties);

	internal void
	CreateBuffer(
	    VkDevice                logicalDevice,
	    VkPhysicalDevice        physicalDevice,
	    VkDeviceSize            bufferSize,
	    VkBufferUsageFlags      usage,
	    VkMemoryPropertyFlags   memoryProperties,
	    VkBuffer *              resultBuffer,
	    VkDeviceMemory *        resultBufferMemory);

	internal VulkanBufferResource make_buffer_resource(	VulkanContext * context,
														VkDeviceSize size,
														VkBufferUsageFlags usage,
														VkMemoryPropertyFlags memoryProperties);

	internal void destroy_buffer_resource(VkDevice logicalDevice, VulkanBufferResource * resource);

    internal inline bool32
    FormatHasStencilComponent(VkFormat format)
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
	
	#pragma message ("Organize here before too late")

	internal VulkanSwapchainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	internal VkSurfaceFormatKHR ChooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& availableFormats);
	internal VkPresentModeKHR ChooseSurfacePresentMode(std::vector<VkPresentModeKHR> & availablePresentModes);
	internal VulkanQueueFamilyIndices FindQueueFamilies (VkPhysicalDevice device, VkSurfaceKHR surface);
	internal VkShaderModule CreateShaderModule(BinaryAsset code, VkDevice logicalDevice);
	internal void recreate_swapchain(VulkanContext * context, VkExtent2D frameBufferSize);


	internal VulkanLoadedPipeline make_pipeline(VulkanContext * context, VulkanPipelineLoadInfo loadInfo);
	internal VulkanLoadedPipeline make_line_pipeline(VulkanContext * context, VulkanPipelineLoadInfo loadInfo);
	internal void destroy_pipeline(VulkanContext * context, VulkanLoadedPipeline * pipeline);

	internal VulkanTexture make_texture(TextureAsset * asset, VulkanContext * context);
	// Todo(Leo): Use some structure with fixed size of six TextureAssets in place of 'assets'
	internal VulkanTexture make_cubemap(VulkanContext * context, TextureAsset * assets);
	internal void destroy_texture(VulkanContext * context, VulkanTexture * texture);

	internal VkDescriptorSetLayout create_material_descriptor_set_layout(VkDevice device, uint32 textureCount);
	internal VkDescriptorSet make_material_descriptor_set(	VulkanContext * context,
															PipelineHandle pipeline,
															ArenaArray<TextureHandle> textures);
	
	internal void create_drawing_resources(VulkanContext * context);
	internal void create_material_descriptor_pool(VulkanContext * context);
	internal void create_uniform_descriptor_pool(VulkanContext * context);
	internal void create_model_descriptor_sets(VulkanContext * context);
	internal void create_scene_descriptor_sets(VulkanContext * context);
	internal void create_texture_sampler(VulkanContext * context);


	// Note(Leo): SCENE and DRAWING functions are passed as pointers to game layer.
	/// SCENE, VulkanScene.cpp
    internal TextureHandle 	push_texture (VulkanContext * context, TextureAsset * texture);
    internal MaterialHandle push_material (VulkanContext * context, MaterialAsset * asset);
    internal MeshHandle 	push_mesh(VulkanContext * context, MeshAsset * mesh);
    internal ModelHandle 	push_model (VulkanContext * context, MeshHandle mesh, MaterialHandle material);
    internal TextureHandle 	push_cubemap(VulkanContext * context, TextureAsset * assets);
    internal PipelineHandle push_pipeline(VulkanContext * context, const char * vertexShaderPath, const char * fragmentShaderPath, platform::PipelineOptions options);
    internal void 			unload_scene(VulkanContext * context);

	/// DRAWING, VulkanDrawing.cpp
	internal void draw_frame(VulkanContext * context, uint32 imageIndex, uint32 frameIndex);
    internal void update_camera(VulkanContext * context, Matrix44 view, Matrix44 perspective);
	internal void prepare_drawing(VulkanContext * context);
	internal void finish_drawing(VulkanContext * context);
	internal void record_draw_command(VulkanContext * context, ModelHandle handle, Matrix44 transform);
	internal void record_line_draw_command(VulkanContext * context, Vector3 start, Vector3 end, float4 color);
}


#define WIN_VULKAN_HPP
#endif