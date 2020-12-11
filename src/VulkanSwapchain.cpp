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
	// Todo(Leo): make these paramterized
	constexpr VkFormat preferredFormat              = VK_FORMAT_B8G8R8A8_SRGB;
	constexpr VkColorSpaceKHR preferredColorSpace   = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

	VkSurfaceFormatKHR result = availableFormats [0];
	s32 resultFormatIndex;
	for (s32 i = 0; i < availableFormats.size(); ++i)
	{	
		if (availableFormats[i].format == preferredFormat && availableFormats[i].colorSpace == preferredColorSpace)
		{
			resultFormatIndex 	= i;
			result 				= availableFormats [i];
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

internal void vulkan_create_scene_render_targets(VulkanContext * context, u32 width, u32 height)
{
	using namespace vulkan;

	width = context->swapchainExtent.width;
	height = context->swapchainExtent.height;

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
		context->sceneRenderTargets[i].colorAttachmentImage = make_vk_image(   context, width, height,
																1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
																VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
																context->msaaSamples);

		context->sceneRenderTargets[i].depthAttachmentImage = make_vk_image(   context, width, height,
																1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
																VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
																context->msaaSamples);

		context->sceneRenderTargets[i].resolveAttachmentImage = make_vk_image(context, width, height,
																1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
																VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
																VK_SAMPLE_COUNT_1_BIT);
	}

	/// 2. ALLOCATE MEMORY
	VkMemoryRequirements colorMemReqs;
	vkGetImageMemoryRequirements(context->device, context->sceneRenderTargets[0].colorAttachmentImage, &colorMemReqs);
	u64 colorMemorySize = memory_align_up(colorMemReqs.size, colorMemReqs.alignment);

	VkMemoryRequirements depthMemReqs;
	vkGetImageMemoryRequirements(context->device, context->sceneRenderTargets[0].depthAttachmentImage, &depthMemReqs);
	u64 depthMemorySize = memory_align_up(depthMemReqs.size, depthMemReqs.alignment);

	VkMemoryRequirements resolveMemoryRequirements;
	vkGetImageMemoryRequirements(context->device, context->sceneRenderTargets[0].resolveAttachmentImage, &resolveMemoryRequirements);
	u64 resolveMemorySize = memory_align_up(resolveMemoryRequirements.size, resolveMemoryRequirements.alignment);


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

	VULKAN_CHECK(vkAllocateMemory(context->device, &allocateInfo, nullptr, &context->sceneRenderTargetsAttachmentMemory));

	/// 3. BIND MEMORY
	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		VULKAN_CHECK(vkBindImageMemory(	context->device,
										context->sceneRenderTargets[i].colorAttachmentImage,
										context->sceneRenderTargetsAttachmentMemory,
										i * colorMemorySize));

		VULKAN_CHECK(vkBindImageMemory(	context->device,
										context->sceneRenderTargets[i].depthAttachmentImage,
										context->sceneRenderTargetsAttachmentMemory,
										totalColourMemorySize + i * depthMemorySize));

		VULKAN_CHECK(vkBindImageMemory(	context->device,
										context->sceneRenderTargets[i].resolveAttachmentImage,
										context->sceneRenderTargetsAttachmentMemory,
										totalColourMemorySize + totalDepthMemorySize + i * resolveMemorySize));

	}


	/// 4. CREATE IMAGE VIEWS
	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		context->sceneRenderTargets[i].colorAttachment = make_vk_image_view(context->device, context->sceneRenderTargets[i].colorAttachmentImage, 1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
		context->sceneRenderTargets[i].depthAttachment = make_vk_image_view(context->device, context->sceneRenderTargets[i].depthAttachmentImage, 1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
		context->sceneRenderTargets[i].resolveAttachment = make_vk_image_view(context->device, context->sceneRenderTargets[i].resolveAttachmentImage, 1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	/// 5. TRANSITION LAYOUTS
	VkCommandBuffer cmd = begin_command_buffer(context->device, context->commandPool);

	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		// Todo(Leo): vulkan-tutorials proposed layout transition function is bad, just do like resolve attachment :)

		cmd_transition_image_layout(cmd, context->device, context->graphicsQueue, context->sceneRenderTargets[i].colorAttachmentImage, colorFormat, 1,
									VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		cmd_transition_image_layout(cmd, context->device, context->graphicsQueue, context->sceneRenderTargets[i].depthAttachmentImage, depthFormat, 1,
									VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		/// RESOLVE ATTACHMENT
		{
		    VkImageMemoryBarrier barrier 	= { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		    barrier.srcAccessMask 			= 0;
		    barrier.dstAccessMask 			= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		    barrier.oldLayout 				= VK_IMAGE_LAYOUT_UNDEFINED;
		    // Todo(Leo): see if could be more specific
		    barrier.newLayout 				= VK_IMAGE_LAYOUT_GENERAL;
		    barrier.srcQueueFamilyIndex 	= VK_QUEUE_FAMILY_IGNORED;
		    barrier.dstQueueFamilyIndex 	= VK_QUEUE_FAMILY_IGNORED;

		    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
		    barrier.subresourceRange.baseMipLevel   = 0;
		    barrier.subresourceRange.levelCount     = 1;
		    barrier.subresourceRange.baseArrayLayer = 0;
		    barrier.subresourceRange.layerCount     = 1;

		    barrier.image = context->sceneRenderTargets[i].resolveAttachmentImage;

		    vkCmdPipelineBarrier(   cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0,
		                            0, nullptr, 0, nullptr, 1, &barrier);
		}

	}

	execute_command_buffer (cmd, context->device, context->commandPool, context->graphicsQueue);

	for (s32 i = 0; i <VIRTUAL_FRAME_COUNT; ++i)
	{
		auto & drawing = context->sceneRenderTargets[i];

		VkImageView attachments [] =
		{
			drawing.colorAttachment,
			drawing.depthAttachment,
			drawing.resolveAttachment,
		};

		context->sceneRenderTargets[i].framebuffer = vulkan::make_vk_framebuffer(  context->device,
																context->sceneRenderPass,
																array_count(attachments),
																attachments,
																width,
																height);
	}
}


internal void
vulkan_create_drawing_resources(VulkanContext * context, u32 width, u32 height)
{
	using namespace vulkan;

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

		context->swapchainExtent.width = f32_clamp(width, min.width, max.width);
		context->swapchainExtent.height = f32_clamp(height, min.height, max.height);
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
	log_graphics(1, "Swapchain format is ", fsvulkan_format_string(surfaceFormat.format));


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
	context->sceneRenderPass = make_vk_render_pass(context, context->hdrFormat, context->msaaSamples);
	
	/// PASS-THROUGH RENDER PASS FOR HDR-RENDERING
	{
		VkAttachmentDescription attachment = {};

		attachment.format         = context->swapchainImageFormat;
		attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
		attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
		attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference attachmentRef = {};
		attachmentRef.attachment 			= 0;
		attachmentRef.layout 				= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


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

		VULKAN_CHECK (vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &context->screenSpaceRenderPass));
	}
}


static void vulkan_destroy_scene_render_targets(VulkanContext * context)
{	

	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		vkDestroyFramebuffer(context->device, context->sceneRenderTargets[i].framebuffer, nullptr);
	
		vkDestroyImage(context->device, context->sceneRenderTargets[i].colorAttachmentImage, nullptr);
		vkDestroyImageView(context->device, context->sceneRenderTargets[i].colorAttachment, nullptr);

		vkDestroyImage(context->device, context->sceneRenderTargets[i].depthAttachmentImage, nullptr);
		vkDestroyImageView(context->device, context->sceneRenderTargets[i].depthAttachment, nullptr);

		vkDestroyImage(context->device, context->sceneRenderTargets[i].resolveAttachmentImage, nullptr);
		vkDestroyImageView(context->device, context->sceneRenderTargets[i].resolveAttachment, nullptr);

		context->sceneRenderTargets[i].colorAttachmentImage 		= VK_NULL_HANDLE;
		context->sceneRenderTargets[i].colorAttachment 			= VK_NULL_HANDLE;
		context->sceneRenderTargets[i].depthAttachmentImage 		= VK_NULL_HANDLE;
		context->sceneRenderTargets[i].depthAttachment 			= VK_NULL_HANDLE;
		context->sceneRenderTargets[i].resolveAttachmentImage 	= VK_NULL_HANDLE;
		context->sceneRenderTargets[i].resolveAttachment 		= VK_NULL_HANDLE;
	}

	vkFreeMemory (context->device, context->sceneRenderTargetsAttachmentMemory, nullptr);
}

internal void
vulkan_destroy_drawing_resources(VulkanContext * context)
{

	vkDestroyRenderPass(context->device, context->sceneRenderPass, nullptr);
	vkDestroyRenderPass(context->device, context->screenSpaceRenderPass, nullptr);

	for (auto imageView : context->swapchainImageViews)
	{
		vkDestroyImageView(context->device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(context->device, context->swapchain, nullptr);
}



internal void vulkan_recreate_drawing_resources(VulkanContext * context, u32 width, u32 height)
{
	vkDeviceWaitIdle(context->device);

	vulkan_destroy_scene_render_targets(context);
	vulkan_destroy_drawing_resources(context);

	vulkan_create_drawing_resources(context, width, height);
	vulkan_create_scene_render_targets(context, context->sceneRenderTargetWidth, context->sceneRenderTargetHeight);

	vkResetDescriptorPool(context->device, context->drawingResourceDescriptorPool, 0);

	fsvulkan_cleanup_pipelines(context);
	fsvulkan_initialize_pipelines(*context, width, height);

	for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
	{
		context->shadowMapTextureDescriptorSet[i] = make_material_vk_descriptor_set_2( context,
																					context->shadowMapTextureDescriptorSetLayout,
																					context->shadowAttachment[i].view,
																					context->drawingResourceDescriptorPool,
																					context->shadowTextureSampler,
																					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}
}

