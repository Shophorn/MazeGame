/*=============================================================================
Leo Tamminen

Implementations of vulkan related functions

Todo(Leo): this is horrible file :)
=============================================================================*/
#include "fsvulkan.hpp"

internal VkDescriptorSetLayoutCreateInfo
fsvulkan_descriptor_set_layout_create_info(u32 bindingCount, VkDescriptorSetLayoutBinding const * bindings)
{
	VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCreateInfo = 
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		nullptr,
		0,
		bindingCount,
		bindings
	};

	return descriptorSetlayoutCreateInfo;
}

internal VkDescriptorSetAllocateInfo
fsvulkan_descriptor_set_allocate_info(VkDescriptorPool descriptorPool, u32 descriptorSetCount, VkDescriptorSetLayout const * setLayouts)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
	{ 
		.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext 				= nullptr,
		.descriptorPool     = descriptorPool,
		.descriptorSetCount = descriptorSetCount,
		.pSetLayouts        = setLayouts,
	};	

	return descriptorSetAllocateInfo;
}

internal VkWriteDescriptorSet
fsvulkan_write_descriptor_set_buffer (VkDescriptorSet dstSet, u32 descriptorCount, VkDescriptorType descriptorType, VkDescriptorBufferInfo const * bufferInfo)
{
	VkWriteDescriptorSet writeDescriptorSet =
	{
		.sType 				= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.pNext 				= nullptr,
		.dstSet 			= dstSet,
		.dstBinding 		= 0,
		.dstArrayElement	= 0,
		.descriptorCount 	= descriptorCount,
		.descriptorType 	= descriptorType,
		.pImageInfo 		= nullptr,
		.pBufferInfo		= bufferInfo,
		.pTexelBufferView 	= nullptr,
	};

	return writeDescriptorSet;
}


// Todo(Leo): these are getting old..
#include "VulkanCommandBuffers.cpp"

#include "fsvulkan_pipelines.cpp"
#include "fsvulkan_resources.cpp"
#include "fsvulkan_drawing.cpp"

// Todo(Leo): these are getting old..
#include "VulkanSwapchain.cpp"


// Todo(Leo): this is weird, it doesnt feel at home here.
internal void fsvulkan_prepare_frame(VulkanContext * context)
{
	VulkanVirtualFrame * frame = fsvulkan_get_current_virtual_frame(context);

    VULKAN_CHECK(vkWaitForFences(context->device, 1, &frame->frameInUseFence, VK_TRUE, VULKAN_NO_TIME_OUT));

    VkResult result = vkAcquireNextImageKHR(context->device,
                                            context->swapchain,
                                            VULKAN_NO_TIME_OUT,
                                            frame->imageAvailableSemaphore,
                                            VK_NULL_HANDLE,
                                            &context->currentDrawFrameIndex);

    // switch(result)
    // {
    // 	case VK_SUCCESS:
    // 		return PGFR_FRAME_OK;

	   //  case VK_SUBOPTIMAL_KHR:
	   //  case VK_ERROR_OUT_OF_DATE_KHR:
	   //  	return PGFR_FRAME_RECREATE;

	   //  default:
	   //  	return PGFR_FRAME_BAD_PROBLEM;
    // }
}

internal void graphics_development_reload_shaders(VulkanContext * context)
{
	context->postRenderEvents.reloadShaders = true;

	// context->onPostRender = [](VulkanContext * context)
	// {
	// 	// char const * command = "\"../compile_shaders.py\"";
	// 	// log_graphics(0, "Reloading shaders, command: '", command, "'");

	// 	// system(command);
	

	// };
}

internal VkFormat
BAD_VULKAN_find_supported_format(
	VkPhysicalDevice physicalDevice,
	s32 candidateCount,
	VkFormat * pCandidates,
	VkImageTiling requestedTiling,
	VkFormatFeatureFlags requestedFeatures
){
	bool32 requestOptimalTiling = requestedTiling == VK_IMAGE_TILING_OPTIMAL;

	for (VkFormat * pFormat = pCandidates; pFormat != pCandidates + candidateCount; ++pFormat)
	{
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, *pFormat, &properties);

		VkFormatFeatureFlags features = requestOptimalTiling ? 
			properties.optimalTilingFeatures : properties.linearTilingFeatures;    

		if ((features & requestedFeatures) == requestedFeatures)
		{
			return *pFormat;
		}
	}
	AssertMsg(false, "Failed to find supported format");
	return (VkFormat)-1;
}

internal VkFormat
vulkan::find_supported_depth_format(VkPhysicalDevice physicalDevice)   
{
	VkFormat formats [] = { VK_FORMAT_D32_SFLOAT,
							VK_FORMAT_D32_SFLOAT_S8_UINT,
							VK_FORMAT_D24_UNORM_S8_UINT };
	s32 formatCount = 3;
	VkFormat result = BAD_VULKAN_find_supported_format(
						physicalDevice, formatCount, formats, VK_IMAGE_TILING_OPTIMAL, 
						VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	return result;
}

internal u32
vulkan::find_memory_type (VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties (physicalDevice, &memoryProperties);

	for (s32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
	{
		bool32 filterMatch = (typeFilter & (1 << i)) != 0;
		bool32 memoryTypeOk = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
		if (filterMatch && memoryTypeOk)
		{
			return i;
		}
	}
	AssertMsg(false, "Failed to find suitable memory type.");
	return -1;
}   

internal VkBufferCreateInfo fsvulkan_buffer_create_info(VkDeviceSize size, VkBufferUsageFlags usage)
{
	VkBufferCreateInfo info =
	{
	    .sType 					= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext 					= nullptr,
	    .flags 					= 0, // Todo(Leo): add if necessary
	    .size 					= size,
	    .usage 					= usage,
	    .sharingMode 			= VK_SHARING_MODE_EXCLUSIVE, // Todo(Leo): Expose if necessary, also maybe queue stuff
	    .queueFamilyIndexCount 	= 0,
	    .pQueueFamilyIndices 	= nullptr
	};
	return info;
};

internal VkMemoryAllocateInfo fsvulkan_memory_allocate_info(VkDeviceSize allocationSize, u32 memoryTypeIndex)
{
	VkMemoryAllocateInfo allocateInfo = 
	{
		.sType 				= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext 				= nullptr,
		.allocationSize 	= allocationSize,
		.memoryTypeIndex 	= memoryTypeIndex
	};
	return allocateInfo;
}

internal VkMemoryRequirements fsvulkan_get_buffer_memory_requirements(VkDevice device, VkBuffer buffer)
{
	VkMemoryRequirements requirements;
	vkGetBufferMemoryRequirements(device, buffer, &requirements);
	return requirements;
}

internal void fsvulkan_create_and_allocate_buffer(	VkDevice 				device,
													VkPhysicalDevice 		physicalDevice,
													VkBuffer * 				buffer,
													VkDeviceMemory * 		memory,
													VkDeviceSize 			size,
													VkBufferUsageFlags 		usage,
													VkMemoryPropertyFlags 	memoryProperties)
{
	auto createInfo = fsvulkan_buffer_create_info(size, usage);
	VULKAN_CHECK(vkCreateBuffer(device, &createInfo, nullptr, buffer));
	
	VkMemoryRequirements memoryRequirements = fsvulkan_get_buffer_memory_requirements(device, *buffer);

	auto allocateInfo = fsvulkan_memory_allocate_info(memoryRequirements.size, vulkan::find_memory_type(physicalDevice, memoryRequirements.memoryTypeBits, memoryProperties));

	VULKAN_CHECK(vkAllocateMemory(device, &allocateInfo, nullptr, memory));

	vkBindBufferMemory(device, *buffer, *memory, 0);
}

internal VulkanBufferResource
BAD_VULKAN_make_buffer_resource(
	VulkanContext *         context,
	VkDeviceSize            size,
	VkBufferUsageFlags      usage,
	VkMemoryPropertyFlags   memoryProperties)
{
	VkBufferCreateInfo bufferInfo =
	{ 
		.sType          = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.flags          = 0,
		.size           = size,
		.usage          = usage,
		.sharingMode    = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkBuffer buffer;
	VULKAN_CHECK(vkCreateBuffer(context->device, &bufferInfo, nullptr, &buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(context->device, buffer, &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = vulkan::find_memory_type(context->physicalDevice,
											memoryRequirements.memoryTypeBits,
											memoryProperties),
	};

	/* Todo(Leo): do not actually always use new allocate, but instead allocate
	once and create allocator to handle multiple items */
	VkDeviceMemory memory;
	VULKAN_CHECK(vkAllocateMemory(context->device, &allocateInfo, nullptr, &memory));

	vkBindBufferMemory(context->device, buffer, memory, 0); 
	
	return {
		.buffer = buffer,
		.memory = memory,
		.used   = 0,
		.size   = size,
	};
}

internal void
BAD_VULKAN_destroy_buffer_resource(VkDevice logicalDevice, VulkanBufferResource * resource)
{
	vkDestroyBuffer(logicalDevice, resource->buffer, nullptr);
	vkFreeMemory(logicalDevice, resource->memory, nullptr);
}

VulkanQueueFamilyIndices
vulkan::find_queue_families (VkPhysicalDevice device, VkSurfaceKHR surface)
{
	// Note: A card must also have a graphics queue family available; We do want to draw after all
	VkQueueFamilyProperties queueFamilyProps [50];
	u32 queueFamilyCount = array_count(queueFamilyProps);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps);

	bool32 properQueueFamilyFound = false;
	VulkanQueueFamilyIndices result = {};
	for (s32 i = 0; i < queueFamilyCount; ++i)
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

internal VkImageView
vulkan::make_vk_image_view(
	VkDevice logicalDevice,
	VkImage image,
	u32 mipLevels,
	VkFormat format,
	VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewInfo.image = image;
	imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewInfo.format = format;

	imageViewInfo.subresourceRange.aspectMask = aspectFlags;
	imageViewInfo.subresourceRange.baseMipLevel = 0;
	imageViewInfo.subresourceRange.levelCount = mipLevels;
	imageViewInfo.subresourceRange.baseArrayLayer = 0;
	imageViewInfo.subresourceRange.layerCount = 1;

	imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

	VkImageView result;
	VULKAN_CHECK(vkCreateImageView(logicalDevice, &imageViewInfo, nullptr, &result));

	return result;
}


// Todo(Leo): Clean, 'context' is used here for logical and physical devices only
internal VkRenderPass
vulkan::make_vk_render_pass(VulkanContext * context, VkFormat format, VkSampleCountFlagBits msaaSamples)
{
	enum : s32
	{ 
		COLOR_ATTACHMENT_ID     = 0,
		DEPTH_ATTACHMENT_ID     = 1,
		RESOLVE_ATTACHMENT_ID   = 2,
		ATTACHMENT_COUNT        = 3,
	};
	VkAttachmentDescription attachments[ATTACHMENT_COUNT] = {};

	/*
	Note(Leo): We render internally to color attachment and depth attachment
	using multisampling. After that final image is compiled to 'resolve'
	attachment that is image from swapchain and present that
	*/

	attachments[COLOR_ATTACHMENT_ID].format         = format;
	attachments[COLOR_ATTACHMENT_ID].samples        = msaaSamples;
	attachments[COLOR_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[COLOR_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[COLOR_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[COLOR_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[COLOR_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[COLOR_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[DEPTH_ATTACHMENT_ID].format         = find_supported_depth_format(context->physicalDevice);
	attachments[DEPTH_ATTACHMENT_ID].samples        = msaaSamples;
	attachments[DEPTH_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[DEPTH_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[DEPTH_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[DEPTH_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[DEPTH_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[DEPTH_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	attachments[RESOLVE_ATTACHMENT_ID].format         = format;
	attachments[RESOLVE_ATTACHMENT_ID].samples        = VK_SAMPLE_COUNT_1_BIT;
	attachments[RESOLVE_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[RESOLVE_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[RESOLVE_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[RESOLVE_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[RESOLVE_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[RESOLVE_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_GENERAL;
	// attachments[RESOLVE_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	constexpr s32 COLOR_ATTACHMENT_COUNT = 1;        
	VkAttachmentReference colorAttachmentRefs[COLOR_ATTACHMENT_COUNT] = {};
	colorAttachmentRefs[0].attachment = COLOR_ATTACHMENT_ID;
	colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Note(Leo): there can be only one depth attachment
	VkAttachmentReference depthStencilAttachmentRef = {};
	depthStencilAttachmentRef.attachment = DEPTH_ATTACHMENT_ID;
	depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference resolveAttachmentRefs [COLOR_ATTACHMENT_COUNT] = {};
	resolveAttachmentRefs[0].attachment = RESOLVE_ATTACHMENT_ID;
	resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpasses[1]       = {};
	subpasses[0].pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount       = COLOR_ATTACHMENT_COUNT;
	subpasses[0].pColorAttachments          = &colorAttachmentRefs[0];
	subpasses[0].pResolveAttachments        = &resolveAttachmentRefs[0];
	subpasses[0].pDepthStencilAttachment    = &depthStencilAttachmentRef;

	// Note(Leo): subpass dependencies
	VkSubpassDependency dependencies[1] = {};
	dependencies[0].srcSubpass          = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass          = 0;
	dependencies[0].srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask       = 0;
	// dependencies[0].dstStageMask        = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo   = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount          = ATTACHMENT_COUNT;
	renderPassInfo.pAttachments             = &attachments[0];
	renderPassInfo.subpassCount             = array_count(subpasses);
	renderPassInfo.pSubpasses               = &subpasses[0];
	renderPassInfo.dependencyCount          = array_count(dependencies);
	renderPassInfo.pDependencies            = &dependencies[0];

	VkRenderPass resultRenderPass;
	VULKAN_CHECK (vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &resultRenderPass));

	return resultRenderPass;
}

internal VkDescriptorSet
make_material_vk_descriptor_set_2(
	VulkanContext *			context,
	VkDescriptorSetLayout 	descriptorSetLayout,
	VkImageView 			imageView,
	VkDescriptorPool 		pool,
	VkSampler 				sampler,
	VkImageLayout 			layout)
{
	VkDescriptorSetAllocateInfo allocateInfo =
	{ 
		.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = pool,
		.descriptorSetCount = 1,
		.pSetLayouts        = &descriptorSetLayout,
	};

	VkDescriptorSet resultSet;
	VULKAN_CHECK(vkAllocateDescriptorSets(context->device, &allocateInfo, &resultSet));

	VkDescriptorImageInfo samplerInfo = 
	{
		.sampler        = sampler,
		.imageView      = imageView,
		.imageLayout    = layout,
	};

	VkWriteDescriptorSet writing =
	{  
		.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet             = resultSet,
		.dstBinding         = 0,
		.dstArrayElement    = 0,
		.descriptorCount    = 1,
		.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.pImageInfo         = &samplerInfo,
	};

	// Note(Leo): Two first are write info, two latter are copy info
	vkUpdateDescriptorSets(context->device, 1, &writing, 0, nullptr);

	return resultSet;
}


internal VkFramebuffer
vulkan::make_vk_framebuffer(VkDevice        device,
							VkRenderPass    renderPass,
							u32             attachmentCount,
							VkImageView *   attachments,
							u32             width,
							u32             height)
{
	VkFramebufferCreateInfo createInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass         = renderPass,
		.attachmentCount    = attachmentCount,
		.pAttachments       = attachments,
		.width              = width,
		.height             = height,
		.layers             = 1,
	};

	VkFramebuffer framebuffer;
	VULKAN_CHECK(vkCreateFramebuffer(device, &createInfo, nullptr, &framebuffer));
	return framebuffer;
}

internal void
vulkan::destroy_texture(VulkanContext * context, VulkanTexture * texture)
{
	vkDestroyImage(context->device, texture->image, nullptr);
	vkFreeMemory(context->device, texture->memory, nullptr);
	vkDestroyImageView(context->device, texture->view, nullptr);
}

internal VkImage
vulkan::make_vk_image( VulkanContext * context,
					u32 texWidth,
					u32 texHeight,
					u32 mipLevels,
					VkFormat format,
					VkImageTiling tiling,
					VkImageUsageFlags usage,
					VkSampleCountFlagBits msaaSamples)
{
	VkImageCreateInfo imageInfo = {
		.sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags          = 0,

		.imageType      = VK_IMAGE_TYPE_2D,
		.format         = format,
		.extent         = { texWidth, texHeight, 1 },
		.mipLevels      = mipLevels,
		.arrayLayers    = 1,
		
		.samples        = msaaSamples,
		.tiling         = tiling,
		.usage          = usage,

		.sharingMode    = VK_SHARING_MODE_EXCLUSIVE, // note(leo): concerning queue families
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,

	};

	VkImage resultImage;
	VULKAN_CHECK(vkCreateImage(context->device, &imageInfo, nullptr, &resultImage));
	return resultImage;
}


VkSampler
BAD_VULKAN_make_vk_sampler(VkDevice device, VkSamplerAddressMode addressMode, VkFilter filter = VK_FILTER_LINEAR)
{
	VkSamplerCreateInfo samplerInfo =
	{ 
		.sType              = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter          = filter,
		.minFilter          = filter,
		.mipmapMode         = VK_SAMPLER_MIPMAP_MODE_LINEAR,

		.addressModeU       = addressMode,
		.addressModeV       = addressMode,
		.addressModeW       = addressMode,

		.mipLodBias         = 0.0f,
		.anisotropyEnable   = VK_TRUE,
		.maxAnisotropy      = 16,

		.compareEnable      = VK_FALSE,
		.compareOp          = VK_COMPARE_OP_ALWAYS,
		
		.minLod             = 0.0f,
		.maxLod             = VULKAN_MAX_LOD_FLOAT,
		
		.borderColor                 = VK_BORDER_COLOR_INT_OPAQUE_WHITE,
		.unnormalizedCoordinates     = VK_FALSE,

	};
	VkSampler result;
	VULKAN_CHECK(vkCreateSampler(device, &samplerInfo, nullptr, &result));
	return result;
}

#if FS_VULKAN_USE_VALIDATION

VKAPI_ATTR VkBool32 VKAPI_CALL fsvulkan_debug_callback (VkDebugUtilsMessageSeverityFlagBitsEXT 			severity,
														VkDebugUtilsMessageTypeFlagsEXT					type,
														const VkDebugUtilsMessengerCallbackDataEXT * 	data,
														void * 											userData)
{
	log_graphics(0, data->pMessage);
	return VK_FALSE;
}

#endif

/* Todo(Leo): this belongs to winapi namespace because it is definetly windows specific.
Make better separation between windows part of this and vulkan part of this. */
namespace winapi
{
	internal VulkanContext create_vulkan_context  (Win32PlatformWindow*);
	internal void destroy_vulkan_context        (VulkanContext*);
}

namespace winapi_vulkan_internal_
{   
	using namespace vulkan;

	internal void add_cleanup(VulkanContext*, VulkanContext::CleanupFunc * cleanupFunc);

	internal VkInstance             create_vk_instance();
	internal VkSurfaceKHR           create_vk_surface(VkInstance, Win32PlatformWindow*);
	internal VkPhysicalDevice       create_vk_physical_device(VkInstance, VkSurfaceKHR);
	internal VkDevice               create_vk_device(VkPhysicalDevice, VkSurfaceKHR);
	internal VkSampleCountFlagBits  get_max_usable_msaa_samplecount(VkPhysicalDevice);

	// Todo(Leo): These are not winapi specific, so they could move to universal vulkan layer
	internal void init_uniform_buffers  (VulkanContext*);

	internal void init_material_descriptor_pool     (VulkanContext*);
	internal void init_persistent_descriptor_pool   (VulkanContext*, u32 descriptorCount, u32 maxSets);
	internal void init_virtual_frames               (VulkanContext*);
	internal void init_shadow_pass                  (VulkanContext*, u32 width, u32 height);
}


// Todo(Leo): define proper structs for these and move to other file 
internal void fsvulkan_create_memory(VulkanContext *);
internal void fsvulkan_destroy_memory(VulkanContext *);


internal VulkanContext
winapi::create_vulkan_context(Win32PlatformWindow * window)
{
	// Note(Leo): This is actual winapi part of vulkan context
	VulkanContext context = {};
	{
		using namespace winapi_vulkan_internal_;

		context.instance          = create_vk_instance();

		#if FS_VULKAN_USE_VALIDATION
		{

			VkDebugUtilsMessengerCreateInfoEXT createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity 		= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
											// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
											| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
											| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
									| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
									| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = fsvulkan_debug_callback;

			createInfo.pUserData = nullptr;

			auto debugUtilsCreateFunc = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(context.instance, "vkCreateDebugUtilsMessengerEXT");
			debugUtilsCreateFunc(context.instance, &createInfo, nullptr, &context.debugMessenger);

			add_cleanup(&context, [](VulkanContext * context)
			{
				auto debugUtilsDestroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(context->instance, "vkDestroyDebugUtilsMessengerEXT");
				debugUtilsDestroyFunc(context->instance, context->debugMessenger, nullptr);
			});

		}
		#endif





		context.surface           = create_vk_surface(context.instance, window);
		context.physicalDevice    = create_vk_physical_device(context.instance, context.surface);
		context.device            = create_vk_device(context.physicalDevice, context.surface);

		// Get physical device properties
		context.msaaSamples = get_max_usable_msaa_samplecount(context.physicalDevice);
		vkGetPhysicalDeviceProperties(context.physicalDevice, &context.physicalDeviceProperties);

	}

	/// END OF PLATFORM SECTION ////////////////////

	/* Todo(Leo): this probably means that we can end winapi section here,
	and move rest to platform independent section. */

	{
		using namespace winapi_vulkan_internal_;

		VulkanQueueFamilyIndices queueFamilyIndices = vulkan::find_queue_families(context.physicalDevice, context.surface);
		vkGetDeviceQueue(context.device, queueFamilyIndices.graphics, 0, &context.graphicsQueue);
		vkGetDeviceQueue(context.device, queueFamilyIndices.present, 0, &context.presentQueue);

		context.graphicsQueueFamily = queueFamilyIndices.graphics;
		context.presentQueueFamily = queueFamilyIndices.present;

		/// START OF RESOURCES SECTION ////////////////////
		VkCommandPoolCreateInfo poolInfo =
		{
			.sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex   = queueFamilyIndices.graphics,
		};
		VULKAN_CHECK(vkCreateCommandPool(context.device, &poolInfo, nullptr, &context.commandPool));

		add_cleanup(&context, [](VulkanContext * context)
		{
			vkDestroyCommandPool(context->device, context->commandPool, nullptr);
		});

		// Note(Leo): these are expected to add_cleanup any functionality required
		fsvulkan_create_memory(&context);

		init_uniform_buffers (&context);

		init_material_descriptor_pool (&context);
		init_persistent_descriptor_pool (&context, 20, 20);
		init_virtual_frames (&context);
	
		context.linearRepeatSampler = BAD_VULKAN_make_vk_sampler(context.device, VK_SAMPLER_ADDRESS_MODE_REPEAT);
		context.nearestRepeatSampler = BAD_VULKAN_make_vk_sampler(context.device, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_NEAREST);
		context.clampOnEdgeSampler = BAD_VULKAN_make_vk_sampler(context.device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
		add_cleanup(&context, [](VulkanContext * context)
		{
			vkDestroySampler(context->device, context->linearRepeatSampler, nullptr);
			vkDestroySampler(context->device, context->nearestRepeatSampler, nullptr);
			vkDestroySampler(context->device, context->clampOnEdgeSampler, nullptr);
		});

		vulkan_create_drawing_resources(&context, window->width, window->height);
		vulkan_create_scene_render_targets(&context, context.sceneRenderTargetWidth, context.sceneRenderTargetHeight);


		add_cleanup(&context, [](VulkanContext * context)
		{
			vulkan_destroy_scene_render_targets(context);
			vulkan_destroy_drawing_resources(context);
		});
		
		init_shadow_pass(&context, 1024 * 8, 1024 * 8);

		/// PIPELINES
		{
			// fsvulkan_initialize_pipelines(context, context.swapchainExtent.width, context.swapchainExtent.height);
			fsvulkan_initialize_pipelines(context, context.sceneRenderTargetWidth, context.sceneRenderTargetHeight);
			add_cleanup(&context, [](VulkanContext * context)
			{
				// Todo(Leo): this kinda goes to show that this add_cleanup is stupid, we should just create
				// pairs of initialize and cleanup functions and call those
				fsvulkan_cleanup_pipelines(context);
			});
		}

		for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
		{
			context.shadowMapTextureDescriptorSet[i] = make_material_vk_descriptor_set_2( 	&context,
																					context.shadowMapTextureDescriptorSetLayout,
																					// context.pipelines[GraphicsPipeline_SCREEN_GUI].descriptorSetLayout,
																					context.shadowAttachment[i].view,
																					context.drawingResourceDescriptorPool,
																					context.shadowTextureSampler,
																					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);    
		}
	}


	log_graphics(1, "Initialized succesfully");

	return context;
}


internal void
winapi::destroy_vulkan_context(VulkanContext * context)
{
	// Note(Leo): All draw frame operations are asynchronous, must wait for them to finish
	vkDeviceWaitIdle(context->device);

	BAD_BUT_ACTUAL_graphics_memory_unload(context);

	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		vkDestroyFramebuffer(context->device, context->presentFramebuffers[i], nullptr);
	}

	fsvulkan_destroy_memory(context);

	while(context->cleanups.size() > 0)
	{
		context->cleanups.back()(context);
		context->cleanups.pop_back();
	}

	vkDestroyDevice     (context->device, nullptr);
	vkDestroySurfaceKHR (context->instance, context->surface, nullptr);
	vkDestroyInstance   (context->instance, nullptr);
	
	log_graphics(1, "Shut down");
}

internal void 
winapi_vulkan_internal_::add_cleanup(VulkanContext * context, VulkanContext::CleanupFunc * cleanup)
{
	context->cleanups.push_back(cleanup);
}


internal VkInstance
winapi_vulkan_internal_::create_vk_instance()
{
	if constexpr (vulkan::enableValidationLayers)
	{
		VkLayerProperties availableLayers [50];
		u32 availableCount = array_count(availableLayers);

		VULKAN_CHECK(vkEnumerateInstanceLayerProperties(&availableCount, availableLayers));

		for (s32 layerIndex = 0; layerIndex < vulkan::VALIDATION_LAYERS_COUNT; ++layerIndex)
		{
			bool32 layerFound = false;
			char const * layerName = vulkan::validationLayers[layerIndex];

			for(s32 availableIndex = 0; availableIndex < availableCount; ++availableIndex)
			{
				if (cstring_equals(layerName, availableLayers[availableIndex].layerName))
				{
					layerFound = true;
					break;
				}
			}

			Assert(layerFound);
		}
		
		log_graphics(0, "Validation layers ok!");
	}


	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan practice";
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &appInfo;

	const char * const extensions [] =
	{
		"VK_KHR_surface",
		"VK_KHR_win32_surface",

		#if FS_VULKAN_USE_VALIDATION

		VK_EXT_DEBUG_UTILS_EXTENSION_NAME,

		#endif
	}; 
	instanceInfo.enabledExtensionCount = array_count(extensions);
	instanceInfo.ppEnabledExtensionNames = extensions;

	if constexpr (vulkan::enableValidationLayers)
	{
		instanceInfo.enabledLayerCount = vulkan::VALIDATION_LAYERS_COUNT;
		instanceInfo.ppEnabledLayerNames = vulkan::validationLayers;
	}
	else
	{
		instanceInfo.enabledLayerCount = 0;
		instanceInfo.ppEnabledLayerNames = nullptr;
	}

	VkInstance instance;
	VULKAN_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));
	

	return instance;
}

internal VkSurfaceKHR
winapi_vulkan_internal_::create_vk_surface(VkInstance instance, Win32PlatformWindow * window)
{
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo =
	{
		.sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance  = window->hInstance,
		.hwnd       = window->hwnd, 
	};
	VkSurfaceKHR result;
	VULKAN_CHECK(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &result));
	return result;
}

internal VkPhysicalDevice
winapi_vulkan_internal_::create_vk_physical_device(VkInstance vulkanInstance, VkSurfaceKHR surface)
{
	auto CheckDeviceExtensionSupport = [] (VkPhysicalDevice testDevice) -> bool32
	{
		VkExtensionProperties availableExtensions [100];
		u32 availableExtensionsCount = array_count(availableExtensions);
		vkEnumerateDeviceExtensionProperties (testDevice, nullptr, &availableExtensionsCount, availableExtensions);

		bool32 result = true;
		for (s32 requiredIndex = 0;
			requiredIndex < vulkan::DEVICE_EXTENSION_COUNT;
			++requiredIndex)
		{

			bool32 requiredExtensionFound = false;
			for (s32 availableIndex = 0;
				availableIndex < availableExtensionsCount;
				++availableIndex)
			{
				if (strcmp(vulkan::deviceExtensions[requiredIndex], availableExtensions[availableIndex].extensionName) == 0)
				{
					requiredExtensionFound = true;
					break;
				}
			}

			result = requiredExtensionFound;
			if (result == false)
			{
				break;
			}
		}

		return result;
	};

	auto IsPhysicalDeviceSuitable = [CheckDeviceExtensionSupport] (VkPhysicalDevice testDevice, VkSurfaceKHR surface) -> bool32
	{
		VkPhysicalDeviceProperties props;
		vkGetPhysicalDeviceProperties(testDevice, &props);
		bool32 isDedicatedGPU = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(testDevice, &features);

		bool32 extensionsAreSupported = CheckDeviceExtensionSupport(testDevice);

		bool32 swapchainIsOk = false;
		if (extensionsAreSupported)
		{
			VulkanSwapchainSupportDetails swapchainSupport = fsvulkan_query_swap_chain_support(testDevice, surface);
			swapchainIsOk = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
		}

		VulkanQueueFamilyIndices indices = vulkan::find_queue_families(testDevice, surface);
		return  isDedicatedGPU 
				&& indices.hasAll()
				&& extensionsAreSupported
				&& swapchainIsOk
				&& features.samplerAnisotropy
				&& features.wideLines;
	};

	VkPhysicalDevice resultPhysicalDevice;


	VkPhysicalDevice devices [10];
	u32 deviceCount = array_count(devices);
	vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices);

	// Note(Leo): No gpu found at all, or no vulkan supporting gpu
	AssertRelease(deviceCount != 0, "");

	for (s32 i = 0; i < deviceCount; i++)
	{
		if (IsPhysicalDeviceSuitable(devices[i], surface))
		{
			resultPhysicalDevice = devices[i];
			break;
		}
	}

	// Note(Leo): no suitable GPU device found
	AssertRelease(resultPhysicalDevice != VK_NULL_HANDLE, "");
	return resultPhysicalDevice;
}

internal VkDevice
winapi_vulkan_internal_::create_vk_device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	VulkanQueueFamilyIndices queueIndices = vulkan::find_queue_families(physicalDevice, surface);
	
	/* Note: We need both graphics and present queue, but they might be on
	separate devices (correct???), so we may need to end up with multiple queues */
	s32 uniqueQueueFamilyCount = queueIndices.graphics == queueIndices.present ? 1 : 2;
	VkDeviceQueueCreateInfo queueCreateInfos [2] = {};
	float queuePriorities[/*queueCount*/] = { 1.0f };
	for (s32 i = 0; i < uniqueQueueFamilyCount; ++i)
	{
		// VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[i].queueFamilyIndex = queueIndices.getAt(i);
		queueCreateInfos[i].queueCount = 1;

		queueCreateInfos[i].pQueuePriorities = queuePriorities;
	}

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
	physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;
	physicalDeviceFeatures.wideLines = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
	deviceCreateInfo.enabledExtensionCount = vulkan::DEVICE_EXTENSION_COUNT;
	deviceCreateInfo.ppEnabledExtensionNames = vulkan::deviceExtensions;

	if constexpr (vulkan::enableValidationLayers)
	{
		deviceCreateInfo.enabledLayerCount = vulkan::VALIDATION_LAYERS_COUNT;
		deviceCreateInfo.ppEnabledLayerNames = vulkan::validationLayers;
	}
	else
	{
		deviceCreateInfo.enabledLayerCount = 0;
		deviceCreateInfo.ppEnabledLayerNames = nullptr;
	}

	VkDevice resultLogicalDevice;

	VULKAN_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo,nullptr, &resultLogicalDevice));

	return resultLogicalDevice;
}

internal VkSampleCountFlagBits 
winapi_vulkan_internal_::get_max_usable_msaa_samplecount(VkPhysicalDevice physicalDevice)
{
	// Todo(Leo): to be easier on machine when developing for 2 players at same time
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	auto colorSampleCounts = physicalDeviceProperties.limits.framebufferColorSampleCounts;
	auto depthSampleCounts = physicalDeviceProperties.limits.framebufferDepthSampleCounts;

	VkSampleCountFlags counts = colorSampleCounts < depthSampleCounts ?
								colorSampleCounts :
								depthSampleCounts;

	VkSampleCountFlagBits result;

	if (counts & VK_SAMPLE_COUNT_64_BIT) { result = VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { result = VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { result = VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { result = VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { result = VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { result = VK_SAMPLE_COUNT_2_BIT; }

	if (result > VULKAN_MAX_MSAA_SAMPLE_COUNT)
	{
		result = VULKAN_MAX_MSAA_SAMPLE_COUNT;
	}

	return result;
}

internal void fsvulkan_create_memory(VulkanContext * context)
{
	// TODO [MEMORY] (Leo): Properly measure required amount
	// TODO [MEMORY] (Leo): Measure something at all...
	// TODO[memory] (Leo): Log usage
	u64 staticMeshPoolSize       				= megabytes(500);
	context->stagingBufferCapacity 				= megabytes(100);
	context->modelUniformBufferFrameCapacity 	= megabytes(100);
	context->dynamicMeshFrameCapacity 			= megabytes(100);
	u64 guiUniformBufferSize     				= megabytes(100);

	// TODO[MEMORY] (Leo): These will need guarding against multithreads once we get there
	context->staticMeshPool = BAD_VULKAN_make_buffer_resource(  
									context, staticMeshPoolSize,
									VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
									VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	fsvulkan_create_and_allocate_buffer(context->device,
										context->physicalDevice,
										&context->stagingBuffer,
										&context->stagingBufferDeviceMemory,
										context->stagingBufferCapacity,
										VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkMapMemory(context->device, context->stagingBufferDeviceMemory, 0, VK_WHOLE_SIZE, 0, (void**)&context->persistentMappedStagingBufferMemory);

	fsvulkan_create_and_allocate_buffer(context->device,
										context->physicalDevice,
										&context->modelUniformBufferBuffer,
										&context->modelUniformBufferMemory,
										VIRTUAL_FRAME_COUNT * context->modelUniformBufferFrameCapacity,
										VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkMapMemory(context->device, context->modelUniformBufferMemory, 0, VK_WHOLE_SIZE, 0, (void**)&context->persistentMappedModelUniformBufferMemory);
	
	fsvulkan_create_and_allocate_buffer(context->device,
										context->physicalDevice,
										&context->dynamicMeshBuffer,
										&context->dynamicMeshDeviceMemory,
										VIRTUAL_FRAME_COUNT * context->dynamicMeshFrameCapacity,
										VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	vkMapMemory(context->device, context->dynamicMeshDeviceMemory, 0, VK_WHOLE_SIZE, 0, (void**)&context->persistentMappedDynamicMeshMemory);

	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		context->modelUniformBufferFrameStart[i] 	= i * context->modelUniformBufferFrameCapacity;
		context->modelUniformBufferFrameUsed[i] 	= 0;

		context->dynamicMeshFrameStart[i] 	= i * context->dynamicMeshFrameCapacity;
		context->dynamicMeshFrameUsed[i] 	= 0;
	}

	{
		s64 leafBufferSize = megabytes(200);

		VkBufferCreateInfo leafBufferInfo =
		{ 
			.sType          = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.flags          = 0,
			.size           = (VkDeviceSize)(leafBufferSize * VIRTUAL_FRAME_COUNT),
			.usage          = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			.sharingMode    = VK_SHARING_MODE_EXCLUSIVE,
		};

		VULKAN_CHECK(vkCreateBuffer(context->device, &leafBufferInfo, nullptr, &context->leafBuffer));

		VkMemoryRequirements memoryRequirements;
		vkGetBufferMemoryRequirements(context->device, context->leafBuffer, &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo =
		{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = memoryRequirements.size,
			.memoryTypeIndex = vulkan::find_memory_type(context->physicalDevice,
												memoryRequirements.memoryTypeBits,
												VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		};

		/* Todo(Leo): do not actually always use new allocate, but instead allocate
		once and create allocator to handle multiple items */
		// VkDeviceMemory memory;
		VULKAN_CHECK(vkAllocateMemory(context->device, &allocateInfo, nullptr, &context->leafBufferMemory));

		vkBindBufferMemory(context->device, context->leafBuffer, context->leafBufferMemory, 0); 

		context->leafBufferCapacity = leafBufferSize;
		vkMapMemory(context->device, context->leafBufferMemory, 0, VK_WHOLE_SIZE, 0, (void**)&context->persistentMappedLeafBufferMemory);
	}
};

internal void fsvulkan_destroy_memory(VulkanContext * context)
{
	BAD_VULKAN_destroy_buffer_resource(context->device, &context->staticMeshPool);

	vkDestroyBuffer(context->device, context->dynamicMeshBuffer, nullptr);
	vkFreeMemory(context->device, context->dynamicMeshDeviceMemory, nullptr);

	vkDestroyBuffer(context->device, context->modelUniformBufferBuffer, nullptr);
	vkFreeMemory(context->device, context->modelUniformBufferMemory, nullptr);

	vkDestroyBuffer(context->device, context->stagingBuffer, nullptr);
	vkFreeMemory(context->device, context->stagingBufferDeviceMemory, nullptr);

	vkDestroyBuffer(context->device, context->leafBuffer, nullptr);
	// Todo(Leo): Is it required to free memory on application exit?
	vkFreeMemory(context->device, context->leafBufferMemory, nullptr);	
}

internal void
winapi_vulkan_internal_::init_persistent_descriptor_pool(VulkanContext * context, u32 descriptorCount, u32 maxSets)
{
	// Note(Leo): this post sheds some light on max set vs descriptorCount confusion.
	// https://www.reddit.com/r/vulkan/comments/8u9zqr/having_trouble_understanding_descriptor_pool/
	VkDescriptorPoolSize poolSize =
	{
		.type               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount    = descriptorCount,
	};

	VkDescriptorPoolCreateInfo poolCreateInfo =
	{
		.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets        = maxSets,
		.poolSizeCount  = 1,
		.pPoolSizes     = &poolSize,
	};

	VULKAN_CHECK(vkCreateDescriptorPool(context->device, &poolCreateInfo, nullptr, &context->drawingResourceDescriptorPool));
	VULKAN_CHECK(vkCreateDescriptorPool(context->device, &poolCreateInfo, nullptr, &context->persistentDescriptorPool));


	add_cleanup(context, [](VulkanContext * context)
	{
		vkDestroyDescriptorPool(context->device, context->drawingResourceDescriptorPool, nullptr);
		vkDestroyDescriptorPool(context->device, context->persistentDescriptorPool, nullptr);
	});
}

internal void
winapi_vulkan_internal_::init_uniform_buffers(VulkanContext * context)
{	
	// Todo(Leo): there is quite a lot going on here, rather messily even. Pls cleanup

	static_assert(VIRTUAL_FRAME_COUNT == 3, "There is hardcoding here");

	VkDescriptorPoolSize poolSizes [] =
	{
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 	1},
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 			3 * VIRTUAL_FRAME_COUNT}
	};

	VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	poolInfo.poolSizeCount  			= array_count(poolSizes);
	poolInfo.pPoolSizes     			= poolSizes;
	poolInfo.maxSets        			= 20;
	VULKAN_CHECK( vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &context->uniformDescriptorPool));

	VkDescriptorSetLayoutBinding modelBinding 	= { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr };
	auto modelDescriptorSetLayoutCreateInfo 	= fsvulkan_descriptor_set_layout_create_info(1, &modelBinding);
	VULKAN_CHECK(vkCreateDescriptorSetLayout(context->device, &modelDescriptorSetLayoutCreateInfo, nullptr, &context->modelDescriptorSetLayout));

	// Note(Leo): Camera needs to accessible from fragment shader for specular etc. stuff
	VkDescriptorSetLayoutBinding cameraBinding 	= { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	auto cameraDescriptorSetLayoutCreateInfo	= fsvulkan_descriptor_set_layout_create_info(1, &cameraBinding);
	VULKAN_CHECK(vkCreateDescriptorSetLayout(context->device, &cameraDescriptorSetLayoutCreateInfo, nullptr, &context->cameraDescriptorSetLayout));

	VkDescriptorSetLayoutBinding lightBinding 	= { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	auto lightDescriptorSetLayoutCreateInfo 	= fsvulkan_descriptor_set_layout_create_info(1, &cameraBinding);
	VULKAN_CHECK(vkCreateDescriptorSetLayout(context->device, &lightDescriptorSetLayoutCreateInfo, nullptr, &context->lightingDescriptorSetLayout));

	// VkDescriptorSetLayoutBinding hdrBinding = {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr};
	// auto hdrDescriptorSetLayoutCreateInfo 	= fsvulkan_descriptor_set_layout_create_info(1, & hdrBinding);
	// VULKAN_CHECK(vkCreateDescriptorSetLayout(context->device, &hdrDescriptorSetLayoutCreateInfo, nullptr, &context->hdrSettingsDescriptorSetLayout));

	// Todo(Leo): Separate to separate calls per type, so we don't need to do remap below
	VkDescriptorSetLayout layouts [] =
	{
		context->modelDescriptorSetLayout,
		context->modelDescriptorSetLayout,
		context->modelDescriptorSetLayout,

		context->cameraDescriptorSetLayout,
		context->cameraDescriptorSetLayout,
		context->cameraDescriptorSetLayout,
		
		context->lightingDescriptorSetLayout,
		context->lightingDescriptorSetLayout,
		context->lightingDescriptorSetLayout,

		// context->hdrSettingsDescriptorSetLayout,
		// context->hdrSettingsDescriptorSetLayout,
		// context->hdrSettingsDescriptorSetLayout,
	};
	VkDescriptorSet resultSets[array_count(layouts)];

	auto allocateInfo = fsvulkan_descriptor_set_allocate_info(context->uniformDescriptorPool, array_count(layouts), layouts);
	VULKAN_CHECK(vkAllocateDescriptorSets(context->device, &allocateInfo, resultSets));

	context->modelDescriptorSet[0] = resultSets[0];
	context->modelDescriptorSet[1] = resultSets[1];
	context->modelDescriptorSet[2] = resultSets[2];

	context->cameraDescriptorSet[0] = resultSets[3];
	context->cameraDescriptorSet[1] = resultSets[4];
	context->cameraDescriptorSet[2] = resultSets[5];
	
	context->lightingDescriptorSet[0] = resultSets[6];
	context->lightingDescriptorSet[1] = resultSets[7];
	context->lightingDescriptorSet[2] = resultSets[8];

	// context->hdrSettingsDescriptorSet[0] = resultSets[7];
	// context->hdrSettingsDescriptorSet[1] = resultSets[8];
	// context->hdrSettingsDescriptorSet[2] = resultSets[9];


	/// Note(Leo): Allocation done
	// ------------------------------------------------------------------------------------

    /* Note(Leo): third is FRAME offset, so it is added per virtual frame */
    constexpr VkDeviceSize cameraUniformOffset 			= 0;
    constexpr VkDeviceSize lightingUniformOffset 		= 256;
    constexpr VkDeviceSize hdrOffset 					= 512;
    constexpr VkDeviceSize sceneUniformBufferFrameOffset = 1024;

    static_assert(sizeof(FSVulkanCameraUniformBuffer) <= 256);
    static_assert(sizeof(FSVulkanLightingUniformBuffer) <= 256);
    static_assert(sizeof(FSVulkanHdrSettings) <= 256);

	context->sceneUniformBufferCapacity = VIRTUAL_FRAME_COUNT * sceneUniformBufferFrameOffset;
	fsvulkan_create_and_allocate_buffer(context->device,
										context->physicalDevice,
										&context->sceneUniformBuffer,
										&context->sceneUniformBufferMemory,
										context->sceneUniformBufferCapacity,
										VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
										VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	// ------------------------------------------------------------------------------------

    // Todo(Leo): This would seem not to belong here
	VkDescriptorBufferInfo modelBufferInfo_0 = { context->modelUniformBufferBuffer, 0, sizeof(m44) };
	VkDescriptorBufferInfo modelBufferInfo_1 = { context->modelUniformBufferBuffer, 0, sizeof(m44) };
	VkDescriptorBufferInfo modelBufferInfo_2 = { context->modelUniformBufferBuffer, 0, sizeof(m44) };

	VkDescriptorBufferInfo cameraBufferInfo [] =
	{
		{context->sceneUniformBuffer, cameraUniformOffset + 0 * sceneUniformBufferFrameOffset, sizeof(FSVulkanCameraUniformBuffer)},
		{context->sceneUniformBuffer, cameraUniformOffset + 1 * sceneUniformBufferFrameOffset, sizeof(FSVulkanCameraUniformBuffer)},
		{context->sceneUniformBuffer, cameraUniformOffset + 2 * sceneUniformBufferFrameOffset, sizeof(FSVulkanCameraUniformBuffer)},
	};

	VkDescriptorBufferInfo lightingBufferInfo [] =
	{
		{context->sceneUniformBuffer, lightingUniformOffset + 0 * sceneUniformBufferFrameOffset, sizeof(FSVulkanLightingUniformBuffer)},
		{context->sceneUniformBuffer, lightingUniformOffset + 1 * sceneUniformBufferFrameOffset, sizeof(FSVulkanLightingUniformBuffer)},
		{context->sceneUniformBuffer, lightingUniformOffset + 2 * sceneUniformBufferFrameOffset, sizeof(FSVulkanLightingUniformBuffer)},
	};

	// VkDescriptorBufferInfo hdrBufferInfo [] =
	// {
	// 	{context->sceneUniformBuffer, hdrOffset + 0 * sceneUniformBufferFrameOffset, sizeof(FSVulkanHdrSettings)},
	// 	{context->sceneUniformBuffer, hdrOffset + 1 * sceneUniformBufferFrameOffset, sizeof(FSVulkanHdrSettings)},
	// 	{context->sceneUniformBuffer, hdrOffset + 2 * sceneUniformBufferFrameOffset, sizeof(FSVulkanHdrSettings)},
	// };

	// Todo(Leo): Why is this 'lIGHTINGbufferwrite' ??????
	VkWriteDescriptorSet lightingBufferWrite [] = 
	{
		fsvulkan_write_descriptor_set_buffer(context->modelDescriptorSet[0], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, &modelBufferInfo_0),
		fsvulkan_write_descriptor_set_buffer(context->modelDescriptorSet[1], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, &modelBufferInfo_1),
		fsvulkan_write_descriptor_set_buffer(context->modelDescriptorSet[2], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, &modelBufferInfo_2),
		
		fsvulkan_write_descriptor_set_buffer(context->cameraDescriptorSet[0], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &cameraBufferInfo[0]),		
		fsvulkan_write_descriptor_set_buffer(context->cameraDescriptorSet[1], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &cameraBufferInfo[1]),		
		fsvulkan_write_descriptor_set_buffer(context->cameraDescriptorSet[2], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &cameraBufferInfo[2]),		

		fsvulkan_write_descriptor_set_buffer(context->lightingDescriptorSet[0], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &lightingBufferInfo[0]),
		fsvulkan_write_descriptor_set_buffer(context->lightingDescriptorSet[1], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &lightingBufferInfo[1]),
		fsvulkan_write_descriptor_set_buffer(context->lightingDescriptorSet[2], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &lightingBufferInfo[2]),

		// fsvulkan_write_descriptor_set_buffer(context->hdrSettingDescriptorSet[0], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &hdrBufferInfo[0]),
		// fsvulkan_write_descriptor_set_buffer(context->hdrSettingDescriptorSet[1], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &hdrBufferInfo[1]),
		// fsvulkan_write_descriptor_set_buffer(context->hdrSettingDescriptorSet[2], 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &hdrBufferInfo[2]),
	};
	vkUpdateDescriptorSets(context->device, array_count(lightingBufferWrite), lightingBufferWrite, 0, nullptr);

	// Note(Leo): we don't need to save this pointer, vulkan memory is unmapped with VkDeviceMemory only.
	u8 * persistentMappedSceneUniformBufferMemory;
	vkMapMemory(context->device, context->sceneUniformBufferMemory, 0, VK_WHOLE_SIZE, 0, (void**)&persistentMappedSceneUniformBufferMemory);

	context->persistentMappedCameraUniformBufferMemory[0] = (FSVulkanCameraUniformBuffer*)(persistentMappedSceneUniformBufferMemory + cameraUniformOffset + 0 * sceneUniformBufferFrameOffset);
	context->persistentMappedCameraUniformBufferMemory[1] = (FSVulkanCameraUniformBuffer*)(persistentMappedSceneUniformBufferMemory + cameraUniformOffset + 1 * sceneUniformBufferFrameOffset);
	context->persistentMappedCameraUniformBufferMemory[2] = (FSVulkanCameraUniformBuffer*)(persistentMappedSceneUniformBufferMemory + cameraUniformOffset + 2 * sceneUniformBufferFrameOffset);

	context->persistentMappedLightingUniformBufferMemory[0] = (FSVulkanLightingUniformBuffer*)(persistentMappedSceneUniformBufferMemory + lightingUniformOffset + 0 * sceneUniformBufferFrameOffset);
	context->persistentMappedLightingUniformBufferMemory[1] = (FSVulkanLightingUniformBuffer*)(persistentMappedSceneUniformBufferMemory + lightingUniformOffset + 1 * sceneUniformBufferFrameOffset);
	context->persistentMappedLightingUniformBufferMemory[2] = (FSVulkanLightingUniformBuffer*)(persistentMappedSceneUniformBufferMemory + lightingUniformOffset + 2 * sceneUniformBufferFrameOffset);

	// context->frameHdrSettings[0] = (FSVulkanHdrSettings*)(persistentMappedSceneUniformBufferMemory + hdrOffset + 0 * sceneUniformBufferFrameOffset);
	// context->frameHdrSettings[1] = (FSVulkanHdrSettings*)(persistentMappedSceneUniformBufferMemory + hdrOffset + 1 * sceneUniformBufferFrameOffset);
	// context->frameHdrSettings[2] = (FSVulkanHdrSettings*)(persistentMappedSceneUniformBufferMemory + hdrOffset + 2 * sceneUniformBufferFrameOffset);

	///////////////////////////////
	///         CLEANUP         ///
	///////////////////////////////
	add_cleanup(context, [](VulkanContext * context)
	{
		vkDestroyDescriptorSetLayout(context->device, context->cameraDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(context->device, context->lightingDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(context->device, context->modelDescriptorSetLayout, nullptr);

		// Notice(Leo): this must be last
		vkDestroyDescriptorPool(context->device, context->uniformDescriptorPool, nullptr);

		vkDestroyBuffer(context->device, context->sceneUniformBuffer, nullptr);
		vkFreeMemory(context->device, context->sceneUniformBufferMemory, nullptr);
	});

}

internal void
winapi_vulkan_internal_::init_material_descriptor_pool(VulkanContext * context)
{
	VkDescriptorPoolSize poolSize =
	{
		.type               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount    = vulkan::MAX_LOADED_TEXTURES,
	};

	VkDescriptorPoolCreateInfo poolCreateInfo = 
	{
		.sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets        = vulkan::MAX_LOADED_TEXTURES,
		.poolSizeCount  = 1,
		.pPoolSizes     = &poolSize,
	};

	VULKAN_CHECK(vkCreateDescriptorPool(context->device, &poolCreateInfo, nullptr, &context->materialDescriptorPool));

	add_cleanup(context, [](VulkanContext * context)
	{
		vkDestroyDescriptorPool(context->device, context->materialDescriptorPool, nullptr);
	});
}

internal void
winapi_vulkan_internal_::init_virtual_frames(VulkanContext * context)
{
	VkCommandBufferAllocateInfo masterCmdAllocateInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = context->commandPool,
		.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1,
	};

	VkCommandBufferAllocateInfo secondaryCmdAllocateInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.commandPool        = context->commandPool,
		.level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
		.commandBufferCount = 1,
	};

	VkEventCreateInfo eventCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
	};

	VkSemaphoreCreateInfo semaphoreInfo = 
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
	};

	VkFenceCreateInfo fenceInfo =
	{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT,
	};

	for (auto & frame : context->virtualFrames)
	{
		// Command buffers
		bool32 success = vkAllocateCommandBuffers(context->device, &masterCmdAllocateInfo, &frame.mainCommandBuffer) 				== VK_SUCCESS;
		success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.sceneCommandBuffer) 		== VK_SUCCESS;
		success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.debugCommandBuffer) 		== VK_SUCCESS;
		success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.guiCommandBuffer) 			== VK_SUCCESS;
		success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.postProcessCommandBuffer) 	== VK_SUCCESS;
		success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.shadowCommandBuffer) 		== VK_SUCCESS;

		// Synchronization stuff
		success = success && vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) 	== VK_SUCCESS;
		success = success && vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) 	== VK_SUCCESS;
		success = success && vkCreateFence(context->device, &fenceInfo, nullptr, &frame.frameInUseFence) 					== VK_SUCCESS;

		Assert(success && "Failed to create VulkanVirtualFrame");
	}

	add_cleanup(context, [](VulkanContext * context)
	{
		for (auto & frame : context->virtualFrames)
		{
			vkDestroySemaphore(context->device, frame.renderFinishedSemaphore, nullptr);
			vkDestroySemaphore(context->device, frame.imageAvailableSemaphore, nullptr);
			vkDestroyFence(context->device, frame.frameInUseFence, nullptr);

			frame.renderFinishedSemaphore 	= VK_NULL_HANDLE;
			frame.imageAvailableSemaphore 	= VK_NULL_HANDLE;
			frame.frameInUseFence 			= VK_NULL_HANDLE;
		}
	});
}



void winapi_vulkan_internal_::init_shadow_pass(VulkanContext * context, u32 width, u32 height)
{
	using namespace vulkan;

	// Todo(Leo): This may not be a valid format, query support or check if vulkan spec requires this.
	VkFormat format = VK_FORMAT_D32_SFLOAT;

	context->shadowTextureWidth = width;
	context->shadowTextureHeight = height;

	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		context->shadowAttachment[i]  	= make_shadow_texture(context, context->shadowTextureWidth, context->shadowTextureHeight, format);
	}
	context->shadowTextureSampler     = BAD_VULKAN_make_vk_sampler(context->device, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	VkAttachmentDescription attachment =
	{
		.format         = format,
		.samples        = VK_SAMPLE_COUNT_1_BIT,
		.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};


	// Note(Leo): there can be only one depth attachment
	VkAttachmentReference depthStencilAttachmentRef =
	{
		.attachment = 0,
		.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	};

	VkSubpassDescription subpasses[1]       = {};
	subpasses[0].pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpasses[0].colorAttachmentCount       = 0;
	subpasses[0].pColorAttachments          = nullptr;
	subpasses[0].pResolveAttachments        = nullptr;
	subpasses[0].pDepthStencilAttachment    = &depthStencilAttachmentRef;

	// Note(Leo): subpass dependencies
	VkSubpassDependency dependencies[2]     = {};
	dependencies[0].srcSubpass              = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass              = 0;
	
	dependencies[0].srcStageMask            = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask            = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

	dependencies[0].srcAccessMask           = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask           = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags         = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass              = 0;
	dependencies[1].dstSubpass              = VK_SUBPASS_EXTERNAL;
	
	dependencies[1].srcStageMask            = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask            = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

	dependencies[1].srcAccessMask           = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask           = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags         = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo   = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassInfo.attachmentCount          = 1;
	renderPassInfo.pAttachments             = &attachment;
	renderPassInfo.subpassCount             = 1;
	renderPassInfo.pSubpasses               = subpasses;
	renderPassInfo.dependencyCount          = array_count(dependencies);
	renderPassInfo.pDependencies            = &dependencies[0];

	VULKAN_CHECK (vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &context->shadowRenderPass));

	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		context->shadowFramebuffer[i] = make_vk_framebuffer(  	context->device,
															context->shadowRenderPass,
															1,
															&context->shadowAttachment[i].view,
															context->shadowTextureWidth,
															context->shadowTextureHeight);    
	}

	fsvulkan_initialize_shadow_pipeline(*context);
	fsvulkan_initialize_leaves_shadow_pipeline(*context);

	VkDescriptorSetLayoutBinding shadowMapBinding 	= { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr };
	auto shadowMapLayoutCreateInfo					= fsvulkan_descriptor_set_layout_create_info(1, &shadowMapBinding);
	VULKAN_CHECK(vkCreateDescriptorSetLayout(context->device, &shadowMapLayoutCreateInfo, nullptr, &context->shadowMapTextureDescriptorSetLayout));

	// auto shadowMapAllocateInfo = fsvulkan_descriptor_set_allocate_info(context->materialDescriptorPool, 1, &context->shadowMapTextureDescriptorSetLayout);
	// VULKAN_CHECK (vkAllocateDescriptorSets(context->device, &shadowMapAllocateInfo, &context->shadowMapDescriptorSet));  

	// VkDescriptorImageInfo info =
	// {
	// 	context->shadowTextureSampler,
	// 	context->shadowAttachment.view,
	// 	VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
	// };

	// VkWriteDescriptorSet write = 
	// {
	// 	.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
	// 	.dstSet             = context->shadowMapDescriptorSet,
	// 	.dstBinding         = 0,
	// 	.dstArrayElement    = 0,
	// 	.descriptorCount    = 1,
	// 	.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
	// 	.pImageInfo         = &info,
	// };
	// vkUpdateDescriptorSets(context->device, 1, &write, 0, nullptr);

	add_cleanup(context, [](VulkanContext * context)
	{
		VkDevice device = context->device;

		for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
		{
			destroy_texture(context, &context->shadowAttachment[i]);
			vkDestroyFramebuffer(context->device, context->shadowFramebuffer[i], nullptr);
		}

		vkDestroySampler(context->device, context->shadowTextureSampler, nullptr);
		vkDestroyRenderPass(context->device, context->shadowRenderPass, nullptr);
		vkDestroyPipeline(context->device, context->shadowPipeline, nullptr);
		vkDestroyPipelineLayout(context->device, context->shadowPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(context->device, context->shadowMapTextureDescriptorSetLayout, nullptr);

		vkDestroyDescriptorSetLayout(context->device, context->leavesShadowMaskDescriptorSetLayout, nullptr);
		vkDestroyPipeline(device, context->leavesShadowPipeline, nullptr);
		vkDestroyPipelineLayout(device, context->leavesShadowPipelineLayout, nullptr);

	});
}