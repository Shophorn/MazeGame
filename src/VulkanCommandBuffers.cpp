/*=============================================================================
Leo Tamminen

This file implements common Vulkan command buffer operations
=============================================================================*/

namespace vulkan
{
    internal VkCommandBuffer begin_command_buffer(VkDevice, VkCommandPool);
    internal void execute_command_buffer(VkCommandBuffer, VkDevice, VkCommandPool, VkQueue);
    internal void cmd_transition_image_layout(  VkCommandBuffer commandBuffer,
                                                VkDevice        device,
                                                VkQueue         graphicsQueue,
                                                VkImage         image,
                                                VkFormat        format,
                                                u32             mipLevels,
                                                VkImageLayout   oldLayout,
                                                VkImageLayout   newLayout,
                                                u32             layerCount = 1);

    internal inline bool32
    has_stencil_component(VkFormat format)
    {
        bool32 result = (format == VK_FORMAT_D32_SFLOAT_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT);
        return result;
    }

}

internal VkCommandBuffer
vulkan::begin_command_buffer(VkDevice logicalDevice, VkCommandPool commandPool)
{
    VkCommandBufferAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(logicalDevice, &allocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo =
    { 
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

internal void
vulkan::execute_command_buffer(
    VkCommandBuffer commandBuffer,
    VkDevice logicalDevice,
    VkCommandPool commandPool,
    VkQueue submitQueue)
{
    VULKAN_CHECK(vkEndCommandBuffer(commandBuffer));

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VULKAN_CHECK(vkQueueSubmit(submitQueue, 1,  &submitInfo, VK_NULL_HANDLE));
    
    // Todo(Leo): Mayube we should use VkFence and vkWaitForFences here instead of wait idle
    // Note(Leo): graphics and compute queues support transfer operations implicitly
    VULKAN_CHECK(vkQueueWaitIdle(submitQueue));

    vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}


// Todo(Leo): shitty function, it relies so much on some implicitness
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

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL || newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
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

    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
            && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
            && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
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
        AssertMsg(false, "This layout transition is not supported!");
    }

    VkDependencyFlags dependencyFlags = 0;
    vkCmdPipelineBarrier(   commandBuffer,
                            sourceStage, destinationStage,
                            dependencyFlags,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
}
