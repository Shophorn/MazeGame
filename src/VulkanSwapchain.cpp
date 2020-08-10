
// Todo(Leo): this is not nice yet :(
namespace vulkan
{
	/// INITIALIZATION CALLS
	internal void create_drawing_resources      (VulkanContext*, u32 width, u32 height);
	internal void destroy_drawing_resources     (VulkanContext*);                   
}


namespace vulkan_swapchain_internal_
{
	using namespace vulkan;
	internal void create_attachments(VulkanContext * context);
}


internal VulkanSwapchainSupportDetails fsvulkan_query_swap_chain_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	VulkanSwapchainSupportDetails result = {};

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &result.capabilities);

	u32 formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
	if (formatCount > 0)
	{
		result.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, result.formats.data());
	}

	u32 presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

	if (presentModeCount > 0)
	{
		result.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, result.presentModes.data());
	}

	return result;
}

internal VkSurfaceFormatKHR fsvulkan_choose_swapchain_surface_format(std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	constexpr VkFormat preferredFormat              = VK_FORMAT_R8G8B8A8_UNORM;
	constexpr VkColorSpaceKHR preferredColorSpace   = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	VkSurfaceFormatKHR result = availableFormats [0];
	for (s32 i = 0; i < availableFormats.size(); ++i)
	{
		if (availableFormats[i].format == preferredFormat && availableFormats[i].colorSpace == preferredColorSpace)
		{
			result = availableFormats [i];
		}   
	}
	return result;
}

internal VkPresentModeKHR fsvulkan_choose_surface_present_mode(std::vector<VkPresentModeKHR> & availablePresentModes)
{
	/*
	Note(Leo): mailbox goes as fast as possible, fifo will wait for vsync, right?
	Mailbox causes tearing, on my computer at least.
	*/
	// constexpr VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
	constexpr VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;
	for (s32 i = 0; i < availablePresentModes.size(); ++i)
	{
		if (availablePresentModes[i] == preferredPresentMode)
		{
			result = availablePresentModes[i];
		}
	}
	return result;
}

internal void
vulkan::create_drawing_resources(VulkanContext * context, u32 width, u32 height)
{
	VulkanSwapchainSupportDetails swapchainSupport  = fsvulkan_query_swap_chain_support(context->physicalDevice, context->surface);
	VkSurfaceFormatKHR surfaceFormat                = fsvulkan_choose_swapchain_surface_format(swapchainSupport.formats);
	VkPresentModeKHR presentMode                    = fsvulkan_choose_surface_present_mode(swapchainSupport.presentModes);

	// Find extent ie. size of drawing window
	/* Note(Leo): max value is special value denoting that all are okay.
	Or something else, so we need to ask platform */
	if (swapchainSupport.capabilities.currentExtent.width != max_value_u32)
	{
		context->swapchainExtent = swapchainSupport.capabilities.currentExtent;
	}
	else
	{   
		VkExtent2D min = swapchainSupport.capabilities.minImageExtent;
		VkExtent2D max = swapchainSupport.capabilities.maxImageExtent;

		context->swapchainExtent.width = clamp_f32(width, min.width, max.width);
		context->swapchainExtent.height = clamp_f32(height, min.height, max.height);
	}

	u32 imageCount 		= swapchainSupport.capabilities.minImageCount + 1;
	u32 maxImageCount 	= swapchainSupport.capabilities.maxImageCount;
	if (maxImageCount > 0 && imageCount > maxImageCount)
	{
		imageCount = maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface            = context->surface,
		.minImageCount      = imageCount,
		.imageFormat        = surfaceFormat.format,
		.imageColorSpace    = surfaceFormat.colorSpace,
		.imageExtent        = context->swapchainExtent,
		.imageArrayLayers   = 1,
		.imageUsage         = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
	};
	VulkanQueueFamilyIndices queueIndices = vulkan::find_queue_families(context->physicalDevice, context->surface);
	u32 queueIndicesArray [2] = {queueIndices.graphics, queueIndices.present};

	if (queueIndices.graphics == queueIndices.present)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	else
	{
		createInfo.imageSharingMode 		= VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount 	= 2;
		createInfo.pQueueFamilyIndices 		= queueIndicesArray;
	}

	createInfo.preTransform 	= swapchainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha 	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode 		= presentMode;
	createInfo.clipped 			= VK_TRUE;
	createInfo.oldSwapchain 	= VK_NULL_HANDLE;

	VULKAN_CHECK(vkCreateSwapchainKHR(context->device, &createInfo, nullptr, &context->swapchain));


	context->swapchainImageFormat = surfaceFormat.format;


	// TODO(Leo): following imageCount variable is reusing one from previous definition, maybe bug
	// Note(Leo): Swapchain images are not created, they are gotten from api
	vkGetSwapchainImagesKHR(context->device, context->swapchain, &imageCount, nullptr);
	context->swapchainImages.resize (imageCount);
	vkGetSwapchainImagesKHR(context->device, context->swapchain, &imageCount, &context->swapchainImages[0]);

	context->swapchainImageViews.resize(imageCount);

	for (int i = 0; i < imageCount; ++i)
	{
		context->swapchainImageViews[i] = make_vk_image_view( context->device,
																context->swapchainImages[i],
																1, 
																context->swapchainImageFormat,
																VK_IMAGE_ASPECT_COLOR_BIT);
	}

	context->hdrFormat 	= VK_FORMAT_R32G32B32A32_SFLOAT;
	context->renderPass = make_vk_render_pass(context, context->hdrFormat, context->msaaSamples);
	
	/// PASS-THROUGH RENDER PASS FOR HDR-RENDERING
	#if 1
	{
		// Todo(Leo): Clean, 'context' is used here for logical and physical devices only
		// enum : s32
		// { 
		//     COLOR_ATTACHMENT_ID     = 0,
		//     DEPTH_ATTACHMENT_ID     = 1,
		//     RESOLVE_ATTACHMENT_ID   = 2,
		//     ATTACHMENT_COUNT        = 3,
		// };
		// VkAttachmentDescription attachments[ATTACHMENT_COUNT] = {};
		VkAttachmentDescription attachment = {};

		/*
		Note(Leo): We render internally to color attachment and depth attachment
		using multisampling. After that final image is compiled to 'resolve'
		attachment that is image from swapchain and present that
		*/

		// attachments[COLOR_ATTACHMENT_ID].format         = format;
		// attachments[COLOR_ATTACHMENT_ID].samples        = msaaSamples;
		// attachments[COLOR_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// attachments[COLOR_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		// attachments[COLOR_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		// attachments[COLOR_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// attachments[COLOR_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		// attachments[COLOR_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// attachments[DEPTH_ATTACHMENT_ID].format         = find_supported_depth_format(context->physicalDevice);
		// attachments[DEPTH_ATTACHMENT_ID].samples        = msaaSamples;
		// attachments[DEPTH_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
		// attachments[DEPTH_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// attachments[DEPTH_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		// attachments[DEPTH_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		// attachments[DEPTH_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		// attachments[DEPTH_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		attachment.format         = context->swapchainImageFormat;
		attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// constexpr s32 COLOR_ATTACHMENT_COUNT = 1;        
		// VkAttachmentReference colorAttachmentRefs[COLOR_ATTACHMENT_COUNT] = {};
		// colorAttachmentRefs[0].attachment = COLOR_ATTACHMENT_ID;
		// colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference attachmentRef = {};
		attachmentRef.attachment 			= 0;
		attachmentRef.layout 				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		// // Note(Leo): there can be only one depth attachment
		// VkAttachmentReference depthStencilAttachmentRef = {};
		// depthStencilAttachmentRef.attachment = DEPTH_ATTACHMENT_ID;
		// depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		// VkAttachmentReference resolveAttachmentRefs [COLOR_ATTACHMENT_COUNT] = {};
		// resolveAttachmentRefs[0].attachment = RESOLVE_ATTACHMENT_ID;
		// resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpasses[1]       = {};
		subpasses[0].pipelineBindPoint          = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpasses[0].colorAttachmentCount       = 1;
		subpasses[0].pColorAttachments          = &attachmentRef;
		subpasses[0].pResolveAttachments        = nullptr;//&resolveAttachmentRefs[0];
		subpasses[0].pDepthStencilAttachment    = nullptr;//&depthStencilAttachmentRef;

		// Note(Leo): subpass dependencies
		VkSubpassDependency dependencies[1] = {};
		dependencies[0].srcSubpass          = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass          = 0;
		dependencies[0].srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask       = 0;
		dependencies[0].dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo   = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		renderPassInfo.attachmentCount          = 1;
		renderPassInfo.pAttachments             = &attachment;
		renderPassInfo.subpassCount             = 1;
		renderPassInfo.pSubpasses               = &subpasses[0];
		renderPassInfo.dependencyCount          = array_count(dependencies);
		renderPassInfo.pDependencies            = &dependencies[0];

		// VkRenderPass resultRenderPass;
		VULKAN_CHECK (vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &context->passThroughRenderPass));
 
	}
	#endif		

	vulkan_swapchain_internal_::create_attachments(context);
}


internal void
vulkan::destroy_drawing_resources(VulkanContext * context)
{
	// Todo(Leo): these should be decoupled from swapchain later in production, when user can manually select render size
	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		vkDestroyImage(context->device, context->virtualFrames[i].colorImage, nullptr);
		vkDestroyImageView(context->device, context->virtualFrames[i].colorImageView, nullptr);

		vkDestroyImage(context->device, context->virtualFrames[i].depthImage, nullptr);
		vkDestroyImageView(context->device, context->virtualFrames[i].depthImageView, nullptr);

		vkDestroyImage(context->device, context->virtualFrames[i].resolveImage, nullptr);
		vkDestroyImageView(context->device, context->virtualFrames[i].resolveImageView, nullptr);

		vkDestroyFramebuffer(context->device, context->virtualFrames[i].passThroughFramebuffer, nullptr);

		context->virtualFrames[i].colorImage 				= VK_NULL_HANDLE;
		context->virtualFrames[i].colorImageView 			= VK_NULL_HANDLE;
		context->virtualFrames[i].depthImage 				= VK_NULL_HANDLE;
		context->virtualFrames[i].depthImageView 			= VK_NULL_HANDLE;
		context->virtualFrames[i].resolveImage 				= VK_NULL_HANDLE;
		context->virtualFrames[i].resolveImageView 			= VK_NULL_HANDLE;
		context->virtualFrames[i].passThroughFramebuffer 	= VK_NULL_HANDLE;
	}

	vkFreeMemory        (context->device, context->virtualFrameAttachmentMemory, nullptr);

	vkDestroyRenderPass(context->device, context->renderPass, nullptr);
	vkDestroyRenderPass(context->device, context->passThroughRenderPass, nullptr);

	for (auto imageView : context->swapchainImageViews)
	{
		vkDestroyImageView(context->device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(context->device, context->swapchain, nullptr);
}

internal void fsvulkan_recreate_drawing_resources(VulkanContext * context, u32 width, u32 height)
{
	vkDeviceWaitIdle(context->device);

	vulkan::destroy_drawing_resources(context);
	vulkan::create_drawing_resources(context, width, height);

	vkResetDescriptorPool(context->device, context->persistentDescriptorPool, 0);

	fsvulkan_cleanup_pipelines(context);
	fsvulkan_initialize_pipelines(*context);

	context->shadowMapTexture = make_material_vk_descriptor_set_2( 	context,
																context->pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].descriptorSetLayout,
																context->shadowPass.attachment.view,
																context->persistentDescriptorPool,
																context->shadowPass.sampler,
																VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	/* Todo(Leo, 28.5.): We currently have viewport and scissor sizes as dynamic
	things in all pipelines, so we do not need to recreate those. But if we change that,
	then we will need to. */
}

internal void
vulkan_swapchain_internal_::create_attachments(VulkanContext * context)
{
	/*
	1. Create images
	2. Allocate single memory for both
	3. Bind images to memory
	4. Create image views
	5. Transition image layouts to attachments optimal
	*/

	/// 1. CREATE IMAGES
	VkFormat colorFormat = context->hdrFormat;
	VkFormat depthFormat = find_supported_depth_format(context->physicalDevice);
	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		context->virtualFrames[i].colorImage = make_vk_image(   context, context->swapchainExtent.width, context->swapchainExtent.height,
																1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
																VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
																context->msaaSamples);

		context->virtualFrames[i].depthImage = make_vk_image(   context, context->swapchainExtent.width, context->swapchainExtent.height,
																1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
																VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
																context->msaaSamples);

		context->virtualFrames[i].resolveImage = make_vk_image(context, context->swapchainExtent.width, context->swapchainExtent.height,
																1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
																VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
																VK_SAMPLE_COUNT_1_BIT);
	}

	/// 2. ALLOCATE MEMORY
	VkMemoryRequirements colorMemReqs;
	vkGetImageMemoryRequirements(context->device, context->virtualFrames[0].colorImage, &colorMemReqs);
	u64 colorMemorySize = align_up(colorMemReqs.size, colorMemReqs.alignment);

	VkMemoryRequirements depthMemReqs;
	vkGetImageMemoryRequirements(context->device, context->virtualFrames[0].depthImage, &depthMemReqs);
	u64 depthMemorySize = align_up(depthMemReqs.size, depthMemReqs.alignment);

	VkMemoryRequirements resolveMemoryRequirements;
	vkGetImageMemoryRequirements(context->device, context->virtualFrames[0].resolveImage, &resolveMemoryRequirements);
	u64 resolveMemorySize = align_up(resolveMemoryRequirements.size, resolveMemoryRequirements.alignment);


	// Todo(Leo): this is required for naive alignment stuff below, it should be changed
	Assert(colorMemReqs.alignment == depthMemReqs.alignment);
	Assert(colorMemReqs.alignment == resolveMemoryRequirements.alignment);

	u32 memoryTypeIndex = find_memory_type( context->physicalDevice,
											colorMemReqs.memoryTypeBits | depthMemReqs.memoryTypeBits | resolveMemoryRequirements.memoryTypeBits,
											VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	u64 totalColourMemorySize   = colorMemorySize * VIRTUAL_FRAME_COUNT;
	u64 totalDepthMemorySize    = depthMemorySize * VIRTUAL_FRAME_COUNT;
	u64 totalResolveMemorySize  = resolveMemorySize * VIRTUAL_FRAME_COUNT;

	VkMemoryAllocateInfo allocateInfo   = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize         = totalColourMemorySize + totalDepthMemorySize + totalResolveMemorySize;
	allocateInfo.memoryTypeIndex        = memoryTypeIndex;

	VULKAN_CHECK(vkAllocateMemory(context->device, &allocateInfo, nullptr, &context->virtualFrameAttachmentMemory));

	/// 3. BIND MEMORY
	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		VULKAN_CHECK(vkBindImageMemory(context->device, context->virtualFrames[i].colorImage, context->virtualFrameAttachmentMemory, i * colorMemorySize));
		VULKAN_CHECK(vkBindImageMemory(context->device, context->virtualFrames[i].depthImage, context->virtualFrameAttachmentMemory, totalColourMemorySize + i * depthMemorySize));
		VULKAN_CHECK(vkBindImageMemory(context->device, context->virtualFrames[i].resolveImage, context->virtualFrameAttachmentMemory, totalColourMemorySize + totalDepthMemorySize + i * resolveMemorySize));
	}


	/// 4. CREATE IMAGE VIEWS
	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		context->virtualFrames[i].colorImageView = make_vk_image_view(context->device, context->virtualFrames[i].colorImage, 1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		context->virtualFrames[i].depthImageView = make_vk_image_view(context->device, context->virtualFrames[i].depthImage, 1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		context->virtualFrames[i].resolveImageView = make_vk_image_view(context->device, context->virtualFrames[i].resolveImage, 1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	/// 5. TRANSITION LAYOUTS
	VkCommandBuffer cmd = begin_command_buffer(context->device, context->commandPool);

	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		cmd_transition_image_layout(cmd, context->device, context->graphicsQueue, context->virtualFrames[i].colorImage, colorFormat, 1,
									VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		cmd_transition_image_layout(cmd, context->device, context->graphicsQueue, context->virtualFrames[i].depthImage, depthFormat, 1,
									VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		// cmd_transition_image_layout(cmd, context->device, context->graphicsQueue, context->virtualFrames[i].resolveImage, colorFormat, 1,
		// 							VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		{
		    VkImageMemoryBarrier barrier 	= { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		    barrier.srcAccessMask 			= 0;
		    barrier.dstAccessMask 			= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		    barrier.oldLayout 				= VK_IMAGE_LAYOUT_UNDEFINED;
		    barrier.newLayout 				= VK_IMAGE_LAYOUT_GENERAL;
		    barrier.srcQueueFamilyIndex 	= VK_QUEUE_FAMILY_IGNORED;
		    barrier.dstQueueFamilyIndex 	= VK_QUEUE_FAMILY_IGNORED;

		    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		    barrier.subresourceRange.baseMipLevel   = 0;
		    barrier.subresourceRange.levelCount     = 1;
		    barrier.subresourceRange.baseArrayLayer = 0;
		    barrier.subresourceRange.layerCount     = 1;

		    barrier.image = context->virtualFrames[i].resolveImage;

		    vkCmdPipelineBarrier(   cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
		                            0, nullptr, 0, nullptr, 1, &barrier);
		}

	}

	execute_command_buffer (cmd, context->device, context->commandPool, context->graphicsQueue);
}

