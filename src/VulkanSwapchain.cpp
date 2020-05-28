namespace vulkan_swapchain_internal_
{
    using namespace vulkan;
    internal void create_attachments(VulkanContext * context);
}

internal void
vulkan::create_drawing_resources(VulkanContext * context, u32 width, u32 height)
{
    auto * resources = &context->drawingResources;

    VulkanSwapchainSupportDetails swapchainSupport  = vulkan::query_swap_chain_support(context->physicalDevice, context->surface);
    VkSurfaceFormatKHR surfaceFormat                = vulkan::choose_swapchain_surface_format(swapchainSupport.formats);
    VkPresentModeKHR presentMode                    = vulkan::choose_surface_present_mode(swapchainSupport.presentModes);

    // Find extent ie. size of drawing window
    /* Note(Leo): max value is special value denoting that all are okay.
    Or something else, so we need to ask platform */
    if (swapchainSupport.capabilities.currentExtent.width != math::highest_value<u32>)
    {
        resources->extent = swapchainSupport.capabilities.currentExtent;
    }
    else
    {   
        VkExtent2D min = swapchainSupport.capabilities.minImageExtent;
        VkExtent2D max = swapchainSupport.capabilities.maxImageExtent;

        resources->extent.width = math::clamp(width, min.width, max.width);
        resources->extent.height = math::clamp(height, min.height, max.height);
    }

    u32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    u32 maxImageCount = swapchainSupport.capabilities.maxImageCount;
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
        .imageExtent        = resources->extent,
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
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueIndicesArray;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VULKAN_CHECK(vkCreateSwapchainKHR(context->device, &createInfo, nullptr, &resources->swapchain));


    resources->imageFormat = surfaceFormat.format;


    // Bug(Leo): following imageCount variable is reusing one from previous definition, maybe bug
    // Note(Leo): Swapchain images are not created, they are gotten from api
    vkGetSwapchainImagesKHR(context->device, resources->swapchain, &imageCount, nullptr);
    resources->images.resize (imageCount);
    vkGetSwapchainImagesKHR(context->device, resources->swapchain, &imageCount, &resources->images[0]);

    resources->imageViews.resize(imageCount);

    for (int i = 0; i < imageCount; ++i)
    {
        resources->imageViews[i] = make_vk_image_view(
            context->device, resources->images[i], 1, 
            resources->imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    context->drawingResources.renderPass = make_vk_render_pass( context,
                                                                context->drawingResources.imageFormat,
                                                                context->msaaSamples);

    vulkan_swapchain_internal_::create_attachments(context);
}


internal void
vulkan::destroy_drawing_resources(VulkanContext * context)
{
    vkDestroyImage      (context->device, context->drawingResources.colorImage, nullptr);
    vkDestroyImageView  (context->device, context->drawingResources.colorImageView, nullptr);
    vkDestroyImage      (context->device, context->drawingResources.depthImage, nullptr);
    vkDestroyImageView  (context->device, context->drawingResources.depthImageView, nullptr);
    vkFreeMemory        (context->device, context->drawingResources.memory, nullptr);

    vkDestroyRenderPass(context->device, context->drawingResources.renderPass, nullptr);

    for (auto imageView : context->drawingResources.imageViews)
    {
        vkDestroyImageView(context->device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(context->device, context->drawingResources.swapchain, nullptr);
}

internal void
vulkan::recreate_drawing_resources(VulkanContext * context, u32 width, u32 height)
{
    vkDeviceWaitIdle(context->device);

    destroy_drawing_resources(context);
    create_drawing_resources(context, width, height);
    
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
    auto * resources = &context->drawingResources;

    /// 1. CREATE IMAGES
    VkFormat colorFormat = context->drawingResources.imageFormat;
    resources->colorImage = make_vk_image(  context, context->drawingResources.extent.width, context->drawingResources.extent.height,
                                            1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                                            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            context->msaaSamples);


    VkFormat depthFormat = find_supported_depth_format(context->physicalDevice);
    resources->depthImage = make_vk_image(  context, context->drawingResources.extent.width, context->drawingResources.extent.height,
                                            1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                            context->msaaSamples);

    /// 2. ALLOCATE MEMORY
    VkMemoryRequirements colorMemReqs;
    VkMemoryRequirements depthMemReqs;
    vkGetImageMemoryRequirements(context->device, resources->colorImage, &colorMemReqs);
    vkGetImageMemoryRequirements(context->device, resources->depthImage, &depthMemReqs);

    // Todo(Leo): Do these need to be aligned like below????
    u64 colorMemorySize = colorMemReqs.size;
    u64 depthMemorySize = depthMemReqs.size;
    // u64 colorMemorySize = align_up_to(colorMemReqs.alignment, colorMemReqs.size);
    // u64 depthMemorySize = align_up_to(depthMemReqs.alignment, depthMemReqs.size);

    u32 memoryTypeIndex = find_memory_type( context->physicalDevice,
                                            colorMemReqs.memoryTypeBits | depthMemReqs.memoryTypeBits,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = (colorMemorySize + depthMemorySize);
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    VULKAN_CHECK(vkAllocateMemory(context->device, &allocateInfo, nullptr, &resources->memory));

    /// 3. BIND MEMORY
    VULKAN_CHECK(vkBindImageMemory(context->device, resources->colorImage, resources->memory, 0));
    VULKAN_CHECK(vkBindImageMemory(context->device, resources->depthImage, resources->memory, colorMemorySize));


    /// 4. CREATE IMAGE VIEWS
    resources->colorImageView = make_vk_image_view( context->device, resources->colorImage,
                                                    1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    resources->depthImageView = make_vk_image_view( context->device, resources->depthImage,
                                                    1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    /// 5. TRANSITION LAYOUTS
    VkCommandBuffer cmd = begin_command_buffer(context->device, context->commandPool);

    cmd_transition_image_layout(cmd, context->device, context->queues.graphics, resources->colorImage, colorFormat, 1,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    cmd_transition_image_layout(cmd, context->device, context->queues.graphics, resources->depthImage, depthFormat, 1,
                                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    execute_command_buffer (cmd, context->device, context->commandPool, context->queues.graphics);
}

