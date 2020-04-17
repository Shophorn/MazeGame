/*=============================================================================
Leo Tamminen

This file implements common Vulkan command buffer operations
=============================================================================*/

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