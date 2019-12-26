/*=============================================================================
Leo Tamminen

Things to use with win api and vulkan.

Structs and functions in 'Vulkan' namespace are NOT actual vulkan functions,
but helpers and combos of things commonly used together in this game.

TODO(Leo): Maybe rename to communicate more clearly that these hide actual
vulkan api.

STUDY: https://devblogs.nvidia.com/vulkan-dos-donts/
=============================================================================*/

#ifndef WIN_VULKAN_HPP

constexpr int32 MAX_FRAMES_IN_FLIGHT = 2;
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


// TOdo[rendering] (Leo): Use this VulkanModelUniformBufferObject
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

struct VulkanPipelineItems
{
	VkPipeline pipeline;
	VkPipelineLayout layout;
};

struct VulkanSyncObjects
{
    std::vector<VkSemaphore>    imageAvailableSemaphores;
    std::vector<VkSemaphore>    renderFinishedSemaphores;
    std::vector<VkFence>        inFlightFences;
};

struct VulkanRenderInfo
{
    VkBuffer meshBuffer;

    VkDeviceSize vertexOffset;
    VkDeviceSize indexOffset;
    
    uint32 indexCount;
    VkIndexType indexType;
    
    uint32 uniformBufferOffset;

    uint32 materialIndex;
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
	VkSampler 		sampler;
	VkImage 		image;
	VkImageView 	view;
	uint32 			mipLevels;

	// TODO(Leo): totally not allocate like this, we need texture pool
	VkDeviceMemory memory;
};

struct VulkanModel
{
    VkBuffer        buffer;
    VkDeviceMemory  memory;

    VkDeviceSize    vertexOffset;
    VkDeviceSize    indexOffset;
    uint32          indexCount;
    VkIndexType     indexType;
};

struct VulkanRenderedObject
{
	MeshHandle 		mesh;
	MaterialHandle 	material;
    uint32         	uniformBufferOffset;
};

using VulkanMaterial = VkDescriptorSet;

struct VulkanContext : platform::IGraphicsContext
{
	VkInstance 						instance;
	VkDevice 						device; // Note(Leo): this is logical device.
	VkPhysicalDevice 				physicalDevice;
	VkPhysicalDeviceProperties 		physicalDeviceProperties;

	VkCommandPool 					commandPool;
    std::vector<VkCommandBuffer>    frameCommandBuffers;

	VkSurfaceKHR 					surface;
	VkQueue 						graphicsQueue;
	VkQueue 						presentQueue;
	
	/* Todo(Leo): There is one layout for each [models, scene data, materials].
	They were originally in array for ease of use, but this is no longer true.*/
    VkDescriptorSetLayout 	descriptorSetLayouts [3];
    VkDescriptorSetLayout 	guiDescriptorSetLayout;

    std::vector<VkFramebuffer>      frameBuffers;

    VkRenderPass            renderPass;
    VulkanSwapchainItems 	swapchainItems;
    VulkanPipelineItems     pipelineItems;
    VulkanPipelineItems     guiPipelineItems;
    VulkanSyncObjects       syncObjects;

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

    std::vector<VkDescriptorSet>    descriptorSets;
    std::vector<VkDescriptorSet>    sceneDescriptorSets;
    std::vector<VkDescriptorSet>	guiDescriptorSets;

    VulkanBufferResource       	stagingBufferPool;
    
    // Note(Leo): After this these need to be recreated on unload as empty containers
    VulkanBufferResource 		staticMeshPool;
    VulkanBufferResource 		modelUniformBuffer;
    VulkanBufferResource 		sceneUniformBuffer;
    VulkanBufferResource		guiUniformBuffer;
    

    // Todo(Leo): Use our own arena arrays for these.
    std::vector<VulkanModel>  			loadedModels;
	std::vector<VulkanTexture> 			loadedTextures;
	std::vector<VulkanMaterial>			loadedMaterials;  // Note(Leo): this is secretly just a VkDescriptorSet
	
	std::vector<VulkanRenderedObject>	loadedRenderedObjects;
	std::vector<VulkanRenderedObject>	loadedGuiObjects;


    /// ----- IGraphicsContext interface implementation -----
	/* Todo(Leo): Making this struct have these and others up is kinda bad mix
	of different styles, so make that better */
    MeshHandle 		PushMesh(MeshAsset * mesh);
    TextureHandle 	PushTexture (TextureAsset * texture);
    MaterialHandle 	PushMaterial (MaterialAsset * material);
    
    RenderedObjectHandle 	PushRenderedObject(MeshHandle mesh, MaterialHandle material);
    GuiHandle 				PushGui(MeshHandle mesh, MaterialHandle material);

    void Apply();
    void UnloadAll();
};

struct VulkanDescriptorLayouts
{
	VkDescriptorSetLayout modelUniform;
	VkDescriptorSetLayout sceneUniform;
	VkDescriptorSetLayout material;
};


namespace vulkan
{
	struct RenderInfo
	{
		Matrix44 cameraView;
		Matrix44 cameraPerspective;

		// Todo(Leo): Are these cool, since they kinda are just pointers to data somewhere else....?? How to know???
		ArenaArray<Matrix44, RenderedObjectHandle> renderedObjects;
		ArenaArray<Matrix44, GuiHandle> guiObjects;
	};		


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

    constexpr int32 TEXTURES_PER_MATERIAL = 3;
    constexpr int32 MAX_LOADED_TEXTURES = 100;

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
		VulkanContext *			context,
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

    /* Todo(Leo): Define a proper struct (of which size is to be used) to
	make it easier to change later. */
	internal inline uint32
	GetModelUniformBufferOffsetForSwapchainImages(VulkanContext * context, int32 imageIndex)
	{
	    uint32 memorySizePerModelMatrix = align_up_to(
	        context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
	        sizeof(Matrix44));

	    uint32 result = imageIndex * memorySizePerModelMatrix * VULKAN_MAX_MODEL_COUNT;   
	    return result;
	}

	internal inline uint32
	GetSceneUniformBufferOffsetForSwapchainImages(VulkanContext * context, int32 imageIndex)
	{
	    uint32 memorySizePerObject = align_up_to(
	        context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
	        sizeof(VulkanCameraUniformBufferObject));

	    // Note(Leo): so far we only have one of these
	    uint32 result = imageIndex * memorySizePerObject;
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

	internal void
	UpdateUniformBuffer(VulkanContext * context, uint32 imageIndex, vulkan::RenderInfo * renderInfo);

	internal void
	DrawFrame(VulkanContext * context, uint32 imageIndex, uint32 frameIndex);

	internal void
	RecreateSwapchain(VulkanContext * context, VkExtent2D frameBufferSize);

	internal void
	RefreshCommandBuffers(VulkanContext * context);

	internal VulkanTexture
	CreateImageTexture(TextureAsset * asset, VulkanContext * context);

	internal VkDescriptorSet
	CreateMaterialDescriptorSets(	VulkanContext * context,
									TextureHandle albedoHandle,
									TextureHandle metallicHandle,
									TextureHandle testMaskHandle);

	internal void
	DestroyImageTexture(VulkanContext * context, VulkanTexture * texture);

}

#define WIN_VULKAN_HPP
#endif