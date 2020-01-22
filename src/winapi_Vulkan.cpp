/*=============================================================================
Leo Tamminen

Implementations of vulkan related functions
=============================================================================*/
#include "winapi_Vulkan.hpp"

#include "VulkanCommandBuffers.cpp"
#include "VulkanScene.cpp"
#include "VulkanDrawing.cpp"
#include "VulkanPipelines.cpp"
#include "VulkanSwapchain.cpp"

/* Note(leo): these are fixed per available renderpipeline thing. They describe
'kinds' of resources or something, so their amount does not change

IMPORTANT(Leo): These must be same in shaders
Todo(Leo): this is stupid and dangerous, please remove.
*/
enum : u32
{
    DESCRIPTOR_LAYOUT_SCENE_BINDING     = 0,
    DESCRIPTOR_LAYOUT_MODEL_BINDING     = 0,
    DESCRIPTOR_LAYOUT_SAMPLER_BINDING   = 0,
};

internal VkFormat
vulkan::find_supported_format(
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
    DEBUG_ASSERT(false, "Failed to find supported format");
}

internal VkFormat
vulkan::find_supported_depth_format(VkPhysicalDevice physicalDevice)   
{
    VkFormat formats [] = { VK_FORMAT_D32_SFLOAT,
                            VK_FORMAT_D32_SFLOAT_S8_UINT,
                            VK_FORMAT_D24_UNORM_S8_UINT };
    s32 formatCount = 3;
    VkFormat result = find_supported_format(
                        physicalDevice, formatCount, formats, VK_IMAGE_TILING_OPTIMAL, 
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    return result;
}

internal u32
vulkan::find_memory_type (VkPhysicalDevice physicalDevice, u32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties (physicalDevice, &memoryProperties);

    for (int i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool32 filterMatch = (typeFilter & (1 << i)) != 0;
        bool32 memoryTypeOk = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (filterMatch && memoryTypeOk)
        {
            return i;
        }
    }
    DEBUG_ASSERT(false, "Failed to find suitable memory type.");
}   


internal VulkanBufferResource
vulkan::make_buffer_resource(
    VulkanContext *         context,
    VkDeviceSize            size,
    VkBufferUsageFlags      usage,
    VkMemoryPropertyFlags   memoryProperties)
{
    VkBufferCreateInfo bufferInfo =
    { 
        .sType          = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size           = size,
        .usage          = usage,
        .sharingMode    = VK_SHARING_MODE_EXCLUSIVE,
        .flags          = 0,
    };

    VkBuffer buffer;
    VULKAN_CHECK(vkCreateBuffer(context->device, &bufferInfo, nullptr, &buffer));

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(context->device, buffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = find_memory_type(context->physicalDevice,
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
vulkan::destroy_buffer_resource(VkDevice logicalDevice, VulkanBufferResource * resource)
{
    vkDestroyBuffer(logicalDevice, resource->buffer, nullptr);
    vkFreeMemory(logicalDevice, resource->memory, nullptr);
}


internal VkIndexType
vulkan::convert_index_type(IndexType type)
{
	switch (type)
	{
		case IndexType::UInt16:
			return VK_INDEX_TYPE_UINT16;

		case IndexType::UInt32:
			return VK_INDEX_TYPE_UINT32;

		default:
			return VK_INDEX_TYPE_NONE_NV;
	};
}

internal VulkanSwapchainSupportDetails
vulkan::query_swap_chain_support(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
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

    // std::cout << "physicalDevice surface formats count: " << formatCount << "\n";

    u32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount > 0)
    {
        result.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, result.presentModes.data());
    }

    return result;
}

internal VkSurfaceFormatKHR
vulkan::choose_swapchain_surface_format(std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    constexpr VkFormat preferredFormat = VK_FORMAT_R8G8B8A8_UNORM;
    constexpr VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    VkSurfaceFormatKHR result = availableFormats [0];
    for (int i = 0; i < availableFormats.size(); ++i)
    {
        if (availableFormats[i].format == preferredFormat && availableFormats[i].colorSpace == preferredColorSpace)
        {
            result = availableFormats [i];
        }   
    }
    return result;
}

internal VkPresentModeKHR
vulkan::choose_surface_present_mode(std::vector<VkPresentModeKHR> & availablePresentModes)
{
    // Todo(Leo): Is it really preferred???
    constexpr VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;
    for (int i = 0; i < availablePresentModes.size(); ++i)
    {
        if (availablePresentModes[i] == preferredPresentMode)
        {
            result = availablePresentModes[i];
        }
    }
    return result;
}


VulkanQueueFamilyIndices
vulkan::find_queue_families (VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // Note: A card must also have a graphics queue family available; We do want to draw after all
    VkQueueFamilyProperties queueFamilyProps [50];
    u32 queueFamilyCount = get_array_count(queueFamilyProps);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps);

    // std::cout << "queueFamilyCount: " << queueFamilyCount << "\n";

    bool32 properQueueFamilyFound = false;
    VulkanQueueFamilyIndices result = {};
    for (int i = 0; i < queueFamilyCount; ++i)
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


VkShaderModule
vulkan::make_vk_shader_module(BinaryAsset code, VkDevice logicalDevice)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<u32 *>(code.data());

    VkShaderModule result;
    if (vkCreateShaderModule (logicalDevice, &createInfo, nullptr, &result) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module");
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
    attachments[RESOLVE_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
    dependencies[0].dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo   = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.attachmentCount          = ATTACHMENT_COUNT;
    renderPassInfo.pAttachments             = &attachments[0];
    renderPassInfo.subpassCount             = get_array_count(subpasses);
    renderPassInfo.pSubpasses               = &subpasses[0];
    renderPassInfo.dependencyCount          = get_array_count(dependencies);
    renderPassInfo.pDependencies            = &dependencies[0];

    VkRenderPass resultRenderPass;
    VULKAN_CHECK (vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &resultRenderPass));

    return resultRenderPass;
}





internal VkDescriptorSetLayout
vulkan::make_material_vk_descriptor_set_layout(VkDevice device, u32 textureCount)
{
    VkDescriptorSetLayoutBinding binding = 
    {
        .binding             = DESCRIPTOR_LAYOUT_SAMPLER_BINDING,
        .descriptorCount     = textureCount,
        .descriptorType      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers  = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo =
    { 
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount   = 1,
        .pBindings      = &binding,
    };

    VkDescriptorSetLayout resultDescriptorSetLayout;
    VULKAN_CHECK(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout));
    
    return resultDescriptorSetLayout;
}


internal VkDescriptorSet
vulkan::make_material_vk_descriptor_set(
    VulkanContext * context,
    PipelineHandle pipelineHandle,
    ArenaArray<TextureHandle> textures)
{
    u32 textureCount = get_loaded_pipeline(context, pipelineHandle)->info.options.textureCount;
    constexpr u32 maxTextures = 10;
    
    DEBUG_ASSERT(textureCount < maxTextures, "Too many textures on material");
    DEBUG_ASSERT(textureCount == textures.count(), "Wrong number of textures in ArenaArray 'textures'");

    VkDescriptorSetAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = context->descriptorPools.material,
        .descriptorSetCount = 1,
        .pSetLayouts        = &get_loaded_pipeline(context, pipelineHandle)->materialLayout,
    };

    VkDescriptorSet resultSet;
    VULKAN_CHECK(vkAllocateDescriptorSets(context->device, &allocateInfo, &resultSet));

    VkDescriptorImageInfo samplerInfos [maxTextures];
    for (int i = 0; i < textureCount; ++i)
    {
        samplerInfos[i] = 
        {
            .sampler        = context->textureSampler,
            .imageView      = get_loaded_texture(context, textures[i])->view,
            .imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    VkWriteDescriptorSet writing =
    {  
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_SAMPLER_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = textureCount,
        .pImageInfo         = samplerInfos,
    };

    // Note(Leo): Two first are write info, two latter are copy info
    vkUpdateDescriptorSets(context->device, 1, &writing, 0, nullptr);

    return resultSet;
}

internal VkDescriptorSet
vulkan::make_material_vk_descriptor_set(
    VulkanContext * context,
    VulkanLoadedPipeline * pipeline,
    VulkanTexture * texture,
    VkDescriptorPool pool)
{
    VkDescriptorSetAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &pipeline->materialLayout,
    };

    VkDescriptorSet resultSet;
    VULKAN_CHECK(vkAllocateDescriptorSets(context->device, &allocateInfo, &resultSet));

    VkDescriptorImageInfo samplerInfo = 
    {
        .sampler        = context->textureSampler,
        .imageView      = texture->view,
        .imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writing =
    {  
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_SAMPLER_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = 1,
        .pImageInfo         = &samplerInfo,
    };

    // Note(Leo): Two first are write info, two latter are copy info
    vkUpdateDescriptorSets(context->device, 1, &writing, 0, nullptr);

    return resultSet;
}

internal VkDescriptorSet
vulkan::make_material_vk_descriptor_set(
    VulkanContext * context,
    VulkanLoadedPipeline * pipeline,
    VkImageView imageView,
    VkDescriptorPool pool)
{
    VkDescriptorSetAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = pool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &pipeline->materialLayout,
    };

    VkDescriptorSet resultSet;
    VULKAN_CHECK(vkAllocateDescriptorSets(context->device, &allocateInfo, &resultSet));

    VkDescriptorImageInfo samplerInfo = 
    {
        .sampler        = context->textureSampler,
        .imageView      = imageView,
        .imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writing =
    {  
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_SAMPLER_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = 1,
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

    VkFramebuffer result;
    DEBUG_ASSERT(
        vkCreateFramebuffer(device, &createInfo, nullptr, &result) == VK_SUCCESS,
        "Failed to create framebuffer");

    return result;
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

        .imageType      = VK_IMAGE_TYPE_2D,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .sharingMode    = VK_SHARING_MODE_EXCLUSIVE, // note(leo): concerning queue families
        .arrayLayers    = 1,

        .extent         = { texWidth, texHeight, 1 },
        .mipLevels      = mipLevels,
        .format         = format,
        .tiling         = tiling,
        .usage          = usage,
        .samples        = msaaSamples,

        .flags          = 0,
    };

    VkImage resultImage;
    VULKAN_CHECK(vkCreateImage(context->device, &imageInfo, nullptr, &resultImage));
    return resultImage;
}

// Change image layout from stored pixel array layout to device optimal layout
internal void
vulkan::cmd_transition_image_layout(
    VkCommandBuffer commandBuffer,
    VkDevice        device,
    VkQueue         graphicsQueue,
    VkImage         image,
    VkFormat        format,
    u32             mipLevels,
    VkImageLayout   oldLayout,
    VkImageLayout   newLayout,
    u32             layerCount)
{
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout; // Note(Leo): Can be VK_IMAGE_LAYOUT_UNDEFINED if we dont care??
    barrier.newLayout = newLayout;

    /* Note(Leo): these are used if we are to change queuefamily ownership.
    Otherwise must be set to 'VK_QUEUE_FAMILY_IGNORED' */
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (vulkan::has_stencil_component(format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    /* Todo(Leo): This is ultimate stupid, we rely on knowing all this based
    on two variables, instead rather pass more arguments, or struct, or a 
    index to lookup table or a struct from that lookup table.

    This function should at most handle the command buffer part instead of
    quessing omitted values.
    */

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
            && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    // DEPTH IMAGE
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
            && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT 
                                | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    // COLOR IMAGE
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
            && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                                | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }


    else
    {
        DEBUG_ASSERT(false, "This layout transition is not supported!");
    }

    VkDependencyFlags dependencyFlags = 0;
    vkCmdPipelineBarrier(   commandBuffer,
                            sourceStage, destinationStage,
                            dependencyFlags,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
}

/* Todo(Leo): this belongs to winapi namespace because it is definetly windows specific.
Make better separation between windows part of this and vulkan part of this. */
namespace winapi
{
    internal VulkanContext create_vulkan_context  (WinAPIWindow*);
    internal void destroy_vulkan_context        (VulkanContext*);
}


namespace winapi_vulkan_internal_
{   
    internal void add_cleanup(VulkanContext*, VulkanContext::CleanupFunc * cleanupFunc);


    internal VkInstance             create_vk_instance();
    internal VkSurfaceKHR           create_vk_surface(VkInstance, WinAPIWindow*);
    internal VkPhysicalDevice       create_vk_physical_device(VkInstance, VkSurfaceKHR);
    internal VkDevice               create_vk_device(VkPhysicalDevice, VkSurfaceKHR);
    internal VkSampleCountFlagBits  get_max_usable_msaa_samplecount(VkPhysicalDevice);

    internal void init_memory (VulkanContext*);

    internal void init_model_descriptor_set_layout(VulkanContext*);
    internal void init_scene_descriptor_set_layout(VulkanContext*);

    internal void init_material_descriptor_pool   (VulkanContext*);
    internal void init_uniform_descriptor_pool    (VulkanContext*);
    internal void init_persistent_descriptor_pool (VulkanContext*, u32 descriptorCount, u32 maxSets);

    internal void init_model_descriptor_sets      (VulkanContext*);
    internal void init_scene_descriptor_sets      (VulkanContext*);
    internal void init_texture_sampler            (VulkanContext*);
    internal void init_virtual_frames             (VulkanContext*);
}

internal VulkanContext
winapi::create_vulkan_context(WinAPIWindow * window)
{
    // Note(Leo): This is actual winapi part of vulkan context
    VulkanContext context = {};
    {
        using namespace winapi_vulkan_internal_;

        context.instance          = create_vk_instance();
        // TODO(Leo): (if necessary, but at this point) Setup debug callbacks, look vulkan-tutorial.com
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
        vkGetDeviceQueue(context.device, queueFamilyIndices.graphics, 0, &context.queues.graphics);
        vkGetDeviceQueue(context.device, queueFamilyIndices.present, 0, &context.queues.present);

        /// START OF RESOURCES SECTION ////////////////////
        VkCommandPoolCreateInfo poolInfo =
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex   = queueFamilyIndices.graphics,
            .flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };
        RELEASE_ASSERT(vkCreateCommandPool(context.device, &poolInfo, nullptr, &context.commandPool) == VK_SUCCESS, "");

        add_cleanup(&context, [](VulkanContext * context)
        {
            vkDestroyCommandPool(context->device, context->commandPool, nullptr);
        });

        // Note(Leo): these are expected to add_cleanup any functionality required
        init_memory(&context);
        init_scene_descriptor_set_layout (&context);
        init_model_descriptor_set_layout (&context);
        init_virtual_frames (&context);
        init_uniform_descriptor_pool (&context);
        init_material_descriptor_pool (&context);
        init_persistent_descriptor_pool (&context, 20, 20);
        init_model_descriptor_sets (&context);
        init_scene_descriptor_sets (&context);
        init_texture_sampler (&context);
    
        vulkan::create_drawing_resources(&context, window->width, window->height);
        add_cleanup(&context, [](VulkanContext * context)
        {
            vulkan::destroy_drawing_resources(context);
        });
        
        context.debugTextures.white = vulkan::make_texture(&context, 512, 512, {1,1,1,1}, VK_FORMAT_R8G8B8A8_UNORM);
        add_cleanup(&context, [](VulkanContext * context)
        {
            vulkan::destroy_texture(context, &context->debugTextures.white);
        });
    }


    // PLATFORM defined resources that GAME uses
    {
        VulkanPipelineLoadInfo linePipelineInfo = 
        {
            .vertexShaderPath           = "shaders/line_vert.spv",
            .fragmentShaderPath         = "shaders/line_frag.spv",

            .options.primitiveType      = platform::RenderingOptions::PRIMITIVE_LINE,
            .options.lineWidth          = 2.0f,
            .options.pushConstantSize   = sizeof(float4) * 3,

            .options.useVertexInput      = false,
            .options.useSceneLayoutSet      = true,
            .options.useMaterialLayoutSet   = false,
            .options.useModelLayoutSet      = false,
        };
        context.lineDrawPipeline = vulkan::make_pipeline(&context, linePipelineInfo);

        // ---------------------------------------------------------------------

        VulkanPipelineLoadInfo guiPipelineInfo =
        {
            .vertexShaderPath    = "shaders/gui_vert2.spv",
            .fragmentShaderPath    = "shaders/gui_frag2.spv",

            .options.primitiveType = platform::RenderingOptions::PRIMITIVE_TRIANGLE_STRIP,
            .options.cullMode = platform::RenderingOptions::CULL_NONE,
            .options.pushConstantSize = sizeof(vector2) * 4 + sizeof(float4),
            .options.textureCount = 1,
            .options.useVertexInput = false,

            .options.useSceneLayoutSet      = false,
            .options.useMaterialLayoutSet   = true,
            .options.useModelLayoutSet      = false,
        };
        context.guiDrawPipeline = vulkan::make_pipeline(&context, guiPipelineInfo);

        winapi_vulkan_internal_::add_cleanup(&context, [](VulkanContext * context)
        {
            vulkan::destroy_loaded_pipeline(context, &context->lineDrawPipeline);
            vulkan::destroy_loaded_pipeline(context, &context->guiDrawPipeline);
        });
    }

    // PLATFORM defined debug resources that PLATFORM uses
    {
        context.defaultGuiMaterial = 
        {
            .pipeline = PipelineHandle::Null,
            .descriptorSet = vulkan::make_material_vk_descriptor_set(   &context,
                                                                        &context.guiDrawPipeline,
                                                                        &context.debugTextures.white,
                                                                        context.descriptorPools.persistent),
        };

    }

    std::cout << "\n[VULKAN]: Initialized succesfully\n\n";

    return context;
}

internal void
winapi::destroy_vulkan_context(VulkanContext * context)
{
    // Note(Leo): All draw frame operations are asynchronous, must wait for them to finish
    vkDeviceWaitIdle(context->device);

    vulkan::unload_scene(context);

    while(context->cleanups.size() > 0)
    {
        context->cleanups.back()(context);
        context->cleanups.pop_back();
    }

    vkDestroyDevice     (context->device, nullptr);
    vkDestroySurfaceKHR (context->instance, context->surface, nullptr);
    vkDestroyInstance   (context->instance, nullptr);
    
    std::cout << "[VULKAN]: shut down\n";
}

internal void 
winapi_vulkan_internal_::add_cleanup(VulkanContext * context, VulkanContext::CleanupFunc * cleanup)
{
    context->cleanups.push_back(cleanup);
}


internal VkInstance
winapi_vulkan_internal_::create_vk_instance()
{
    auto CheckValidationLayerSupport = [] () -> bool32
    {
        VkLayerProperties availableLayers [50];
        u32 availableLayersCount = get_array_count(availableLayers);

        bool32 result = true;
        if (vkEnumerateInstanceLayerProperties (&availableLayersCount, availableLayers) == VK_SUCCESS)
        {

            for (
                int validationLayerIndex = 0;
                validationLayerIndex < vulkan::VALIDATION_LAYERS_COUNT;
                ++validationLayerIndex)
            {
                bool32 layerFound = false;
                for(
                    int availableLayerIndex = 0; 
                    availableLayerIndex < availableLayersCount; 
                    ++availableLayerIndex)
                {
                    if (strcmp (vulkan::validationLayers[validationLayerIndex], availableLayers[availableLayerIndex].layerName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }

                if (layerFound == false)
                {
                    result = false;
                    break;
                }
            }
        }

        return result;
    };

    // Note(Leo): If validation is enable, check support
    assert (!vulkan::enableValidationLayers || CheckValidationLayerSupport());

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan practice";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    instanceInfo.enabledExtensionCount = 2;
    const char * extensions [] = {"VK_KHR_surface", "VK_KHR_win32_surface"}; 
    instanceInfo.ppEnabledExtensionNames = extensions;

    if constexpr (vulkan::enableValidationLayers)
    {
        instanceInfo.enabledLayerCount = vulkan::VALIDATION_LAYERS_COUNT;
        instanceInfo.ppEnabledLayerNames = vulkan::validationLayers;
    }
    else
    {
        instanceInfo.enabledLayerCount = 0;
    }

    VkInstance instance;
    VULKAN_CHECK(vkCreateInstance(&instanceInfo, nullptr, &instance));
    

    return instance;
}

internal VkSurfaceKHR
winapi_vulkan_internal_::create_vk_surface(VkInstance instance, WinAPIWindow * window)
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
        u32 availableExtensionsCount = get_array_count(availableExtensions);
        vkEnumerateDeviceExtensionProperties (testDevice, nullptr, &availableExtensionsCount, availableExtensions);

        bool32 result = true;
        for (int requiredIndex = 0;
            requiredIndex < vulkan::DEVICE_EXTENSION_COUNT;
            ++requiredIndex)
        {

            bool32 requiredExtensionFound = false;
            for (int availableIndex = 0;
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
            VulkanSwapchainSupportDetails swapchainSupport = vulkan::query_swap_chain_support(testDevice, surface);
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
    u32 deviceCount = get_array_count(devices);
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices);

    // Note(Leo): No gpu found at all, or no vulkan supporting gpu
    RELEASE_ASSERT(deviceCount != 0, "");

    for (int i = 0; i < deviceCount; i++)
    {
        if (IsPhysicalDeviceSuitable(devices[i], surface))
        {
            resultPhysicalDevice = devices[i];
            break;
        }
    }

    // Note(Leo): no suitable GPU device found
    RELEASE_ASSERT(resultPhysicalDevice != VK_NULL_HANDLE, "");
    return resultPhysicalDevice;
}

internal VkDevice
winapi_vulkan_internal_::create_vk_device(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VulkanQueueFamilyIndices queueIndices = vulkan::find_queue_families(physicalDevice, surface);
    
    /* Note: We need both graphics and present queue, but they might be on
    separate devices (correct???), so we may need to end up with multiple queues */
    int uniqueQueueFamilyCount = queueIndices.graphics == queueIndices.present ? 1 : 2;
    VkDeviceQueueCreateInfo queueCreateInfos [2] = {};
    float queuePriorities[/*queueCount*/] = { 1.0f };
    for (int i = 0; i <uniqueQueueFamilyCount; ++i)
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

    VkSampleCountFlags counts = std::min(
            physicalDeviceProperties.limits.framebufferColorSampleCounts,
            physicalDeviceProperties.limits.framebufferDepthSampleCounts
        );

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

internal void
winapi_vulkan_internal_::init_memory(VulkanContext * context)
{
    // TODO [MEMORY] (Leo): Properly measure required amount
    // TODO[memory] (Leo): Log usage
    u64 staticMeshPoolSize       = megabytes(500);
    u64 stagingBufferPoolSize    = megabytes(100);
    u64 modelUniformBufferSize   = megabytes(100);
    u64 sceneUniformBufferSize   = megabytes(100);
    u64 guiUniformBufferSize     = megabytes(100);

    // TODO[MEMORY] (Leo): This will need guarding against multithreads once we get there
    context->staticMeshPool = vulkan::make_buffer_resource(  
                                    context, staticMeshPoolSize,
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    context->stagingBufferPool = vulkan::make_buffer_resource(  
                                    context, stagingBufferPoolSize,
                                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    context->modelUniformBuffer = vulkan::make_buffer_resource(  
                                    context, modelUniformBufferSize,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    context->sceneUniformBuffer = vulkan::make_buffer_resource(
                                    context, sceneUniformBufferSize,
                                    VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);


    add_cleanup(context, [](VulkanContext * context){
        vulkan::destroy_buffer_resource(context->device, &context->staticMeshPool);
        vulkan::destroy_buffer_resource(context->device, &context->stagingBufferPool);
        vulkan::destroy_buffer_resource(context->device, &context->modelUniformBuffer);
        vulkan::destroy_buffer_resource(context->device, &context->sceneUniformBuffer);
    });
};

internal void
winapi_vulkan_internal_::init_persistent_descriptor_pool(VulkanContext * context, u32 descriptorCount, u32 maxSets)
{
    // Note(Leo): this posr sheds soma light on max set vs descriptorCount confusion.
    // https://www.reddit.com/r/vulkan/comments/8u9zqr/having_trouble_understanding_descriptor_pool/
    VkDescriptorPoolSize poolSize =
    {
        .type               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = descriptorCount,
    };

    VkDescriptorPoolCreateInfo poolCreateInfo =
    {
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount  = 1,
        .pPoolSizes     = &poolSize,
        .maxSets        = maxSets,
    };

    VULKAN_CHECK(vkCreateDescriptorPool(context->device, &poolCreateInfo, nullptr, &context->descriptorPools.persistent));

    add_cleanup(context, [](VulkanContext * context)
    {
        vkDestroyDescriptorPool(context->device, context->descriptorPools.persistent, nullptr);
    });
}

internal void
winapi_vulkan_internal_::init_uniform_descriptor_pool(VulkanContext * context)
{
    /*
    Note(Leo): 
    There needs to only one per type, not one per user

    We create a single big buffer and then use offsets to divide it to smaller chunks

    'count' is one for actor (characters, animated scenery) uniform buffers which are dynamic and 
    one for static scenery.
    */
    constexpr s32 count = 2;
    VkDescriptorPoolSize poolSizes [count] = {};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[0].descriptorCount = 1; // Note(Leo): 2 = 1 for 3d models and 1 for gui 

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = count;
    poolInfo.pPoolSizes = &poolSizes[0];
    poolInfo.maxSets = 20;

    VULKAN_CHECK( vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &context->descriptorPools.uniform));

    add_cleanup(context, [](VulkanContext * context)
    {
        vkDestroyDescriptorPool(context->device, context->descriptorPools.uniform, nullptr);
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
        .poolSizeCount  = 1,
        .pPoolSizes     = &poolSize,
        .maxSets        = vulkan::MAX_LOADED_TEXTURES,
    };

    VULKAN_CHECK(vkCreateDescriptorPool(context->device, &poolCreateInfo, nullptr, &context->descriptorPools.material));

    add_cleanup(context, [](VulkanContext * context)
    {
        vkDestroyDescriptorPool(context->device, context->descriptorPools.material, nullptr);
    });
}

internal void
winapi_vulkan_internal_::init_model_descriptor_sets(VulkanContext * context)
{
    VkDescriptorSetAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = context->descriptorPools.uniform,
        .descriptorSetCount = 1,
        .pSetLayouts        = &context->descriptorSetLayouts.model,
    };

    VkDescriptorSet resultDescriptorSet;
    VULKAN_CHECK (vkAllocateDescriptorSets(context->device, &allocateInfo, &resultDescriptorSet));

    VkDescriptorBufferInfo modelBufferInfo =
    {
        .buffer = context->modelUniformBuffer.buffer,
        .offset = 0,

        // Todo(Leo): Align Align, this works now because matrix44 happens to fit 64 bytes.
        .range = sizeof(Matrix44),
    };

    VkWriteDescriptorSet descriptorWrite =
    {
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultDescriptorSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_MODEL_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount    = 1,
        .pBufferInfo        = &modelBufferInfo,
    };

    vkUpdateDescriptorSets(context->device, 1, &descriptorWrite, 0, nullptr);
    context->uniformDescriptorSets.model = resultDescriptorSet;
}

internal void
winapi_vulkan_internal_::init_scene_descriptor_sets(VulkanContext * context)
{
    VkDescriptorSetAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = context->descriptorPools.uniform,
        .descriptorSetCount = 1,
        .pSetLayouts        = &context->descriptorSetLayouts.scene,
    };

    VkDescriptorSet resultDescriptorSet;
    VULKAN_CHECK(vkAllocateDescriptorSets(context->device, &allocateInfo, &resultDescriptorSet));
    
    VkDescriptorBufferInfo sceneBufferInfo =
    {
        .buffer = context->sceneUniformBuffer.buffer,
        .offset = 0,
        .range = sizeof(VulkanCameraUniformBufferObject),
    };

    VkWriteDescriptorSet descriptorWrite =
    {
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultDescriptorSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_SCENE_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .pBufferInfo        = &sceneBufferInfo,
    };

    vkUpdateDescriptorSets(context->device, 1, &descriptorWrite, 0, nullptr);
    context->uniformDescriptorSets.scene = resultDescriptorSet;
}

internal void
winapi_vulkan_internal_::init_texture_sampler(VulkanContext * context)
{
    VkSamplerCreateInfo samplerInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter          = VK_FILTER_LINEAR,
        .minFilter          = VK_FILTER_LINEAR,

        .addressModeU       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW       = VK_SAMPLER_ADDRESS_MODE_REPEAT,

        .anisotropyEnable   = VK_TRUE,
        .maxAnisotropy      = 16,

        .borderColor                 = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates     = VK_FALSE,
        .compareEnable               = VK_FALSE,
        .compareOp                   = VK_COMPARE_OP_ALWAYS,

        .mipmapMode         = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .minLod             = 0.0f,
        .maxLod             = VULKAN_MAX_LOD_FLOAT,
        .mipLodBias         = 0.0f,
    };

    VULKAN_CHECK(vkCreateSampler(context->device, &samplerInfo, nullptr, &context->textureSampler));

    add_cleanup(context, [](VulkanContext * context)
    {
        vkDestroySampler(context->device, context->textureSampler, nullptr);
    });
}


internal void
winapi_vulkan_internal_::init_virtual_frames(VulkanContext * context)
{
    VkCommandBufferAllocateInfo primaryCmdAllocateInfo =
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
        bool32 success = vkAllocateCommandBuffers(context->device, &primaryCmdAllocateInfo, &frame.commandBuffers.primary) == VK_SUCCESS;
        success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.commandBuffers.scene) == VK_SUCCESS;
        success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.commandBuffers.gui) == VK_SUCCESS;

        success = success && vkAllocateCommandBuffers(context->device, &primaryCmdAllocateInfo, &frame.commandBuffers.offscreen) == VK_SUCCESS;

        // Synchronization stuff
        success = success && vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) == VK_SUCCESS;
        success = success && vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) == VK_SUCCESS;
        success = success && vkCreateFence(context->device, &fenceInfo, nullptr, &frame.inFlightFence) == VK_SUCCESS;

        DEBUG_ASSERT(success, "Failed to create VulkanVirtualFrame");
    }

    add_cleanup(context, [](VulkanContext * context)
    {
        for (auto & frame : context->virtualFrames)
        {
            /* Note(Leo): command buffers are destroyed with command pool, but we need to destroy
            framebuffers here, since they are always recreated immediately right after destroying
            them in drawing procedure */
            vkDestroyFramebuffer(context->device, frame.framebuffer, nullptr);
            
            vkDestroySemaphore(context->device, frame.renderFinishedSemaphore, nullptr);
            vkDestroySemaphore(context->device, frame.imageAvailableSemaphore, nullptr);
            vkDestroyFence(context->device, frame.inFlightFence, nullptr);
        }
    });
}

internal void
winapi_vulkan_internal_::init_model_descriptor_set_layout(VulkanContext * context)
{
    VkDescriptorSetLayoutBinding binding =
    {
        .binding            = DESCRIPTOR_LAYOUT_MODEL_BINDING,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo =
    { 
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount   = 1,
        .pBindings      = &binding,
    };

    VULKAN_CHECK(vkCreateDescriptorSetLayout(context->device, &layoutCreateInfo, nullptr, &context->descriptorSetLayouts.model));

    add_cleanup(context, [](VulkanContext * context)
    {
        vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayouts.model, nullptr);
    });
}

internal void
winapi_vulkan_internal_::init_scene_descriptor_set_layout(VulkanContext * context)
{
    VkDescriptorSetLayoutBinding binding =
    {
        .binding            = DESCRIPTOR_LAYOUT_SCENE_BINDING,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo =
    { 
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount   = 1,
        .pBindings      = &binding,
    };

    VULKAN_CHECK(vkCreateDescriptorSetLayout(   context->device,
                                                &layoutCreateInfo,
                                                nullptr,
                                                &context->descriptorSetLayouts.scene));

    add_cleanup(context, [](VulkanContext * context)
    {
        vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayouts.scene, nullptr);
    });
}