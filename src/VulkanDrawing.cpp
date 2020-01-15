/*=============================================================================
Leo Tamminen
shophorn @ internet

Vulkan drawing functions' definitions.
=============================================================================*/
void 
vulkan::update_camera(VulkanContext * context, Matrix44 view, Matrix44 perspective)
{
    // Todo(Leo): alignment

    // Note (Leo): map vulkan memory directly to right type so we can easily avoid one (small) memcpy per frame
    VulkanCameraUniformBufferObject * pUbo;
    vkMapMemory(context->device, context->sceneUniformBuffer.memory,
                0, sizeof(VulkanCameraUniformBufferObject), 0, (void**)&pUbo);

    pUbo->view          = view;
    pUbo->perspective   = perspective;

    vkUnmapMemory(context->device, context->sceneUniformBuffer.memory);
}

void
vulkan::prepare_drawing(VulkanContext * context)
{
 
    // std::cout << "[vulkan::prepare_drawing()]\n";
    uint32 frameIndex = context->currentDrawFrameIndex;
    context->currentUniformBufferOffset = 0;
    context->currentBoundPipeline = PipelineHandle::Null;

    DEVELOPMENT_ASSERT((context->canDraw == false), "Invalid call to prepare_drawing() when finish_drawing() has not been called.")
    context->canDraw = true;

    // Note(Leo): We recreate these everytime we are here.
    // https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-4#inpage-nav-5
    vkDestroyFramebuffer(context->device, get_current_virtual_frame(context)->framebuffer, nullptr);

    uint32 const attachmentCount = 3;
    VkImageView attachments [attachmentCount] =
    {
        context->drawingResources.colorImageView,
        context->drawingResources.depthImageView,
        context->swapchainItems.imageViews[frameIndex],
    };
    get_current_virtual_frame(context)->framebuffer = make_framebuffer(   context,
                                                                                context->renderPass,
                                                                                attachmentCount,
                                                                                attachments,
                                                                                context->swapchainItems.extent.width,
                                                                                context->swapchainItems.extent.height);    

    VkCommandBuffer commandBuffer = get_current_virtual_frame(context)->commandBuffer;

    /* Note (Leo): beginning command buffer implicitly resets it, if we have specified
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT in command pool creation */

    VkCommandBufferBeginInfo beginInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags              = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
        .pInheritanceInfo   = nullptr,
    };

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to begin recording command buffer");
    }

    VkClearValue clearValues [] =
    {
        { .color = {0.0f, 0.0f, 0.0f, 1.0f} },
        { .depthStencil = {1.0f, 0} }
    };

    VkRenderPassBeginInfo renderPassInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass         = context->renderPass,
        .framebuffer        = get_current_virtual_frame(context)->framebuffer,
        
        .renderArea.offset  = {0, 0},
        .renderArea.extent  = context->swapchainItems.extent,

        .clearValueCount    = 2,
        .pClearValues       = clearValues,
    };
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void
vulkan::finish_drawing(VulkanContext * context)
{
    DEVELOPMENT_ASSERT(context->canDraw, "Invalid call to finish_drawing() when prepare_drawing() has not been called.")
    context->canDraw = false;

    VkCommandBuffer commandBuffer = get_current_virtual_frame(context)->commandBuffer;

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer");
    }
}


internal void
vulkan::draw_frame(VulkanContext * context, uint32 imageIndex)
{
    /* Note(Leo): these have to do with sceneloading in game layer. We are then unloading
    all resources associated with command buffer, which makes it invalid to submit to queue */
    bool32 skipFrame            = context->sceneUnloaded;
    context->sceneUnloaded      = false;

    uint32 frameIndex = context->virtualFrameIndex;
    auto * frame = get_current_virtual_frame(context);


    /* Note(Leo): reset here, so that if we have recreated swapchain above,
    our fences will be left in signaled state */
    vkResetFences(context->device, 1, &frame->inFlightFence);

    // Note(Leo): We wait for these BEFORE drawing
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    VkSubmitInfo submitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .waitSemaphoreCount     = 1,
        .pWaitSemaphores        = &frame->imageAvailableSemaphore,
        .pWaitDstStageMask      = &waitStage,

        .commandBufferCount     = (uint32)(skipFrame ? 0 : 1),
        .pCommandBuffers        = &get_current_virtual_frame(context)->commandBuffer,

        // Note(Leo): We signal these AFTER drawing
        .signalSemaphoreCount   = 1,
        .pSignalSemaphores      = &frame->renderFinishedSemaphore,
    };

    if (vkQueueSubmit(context->graphicsQueue, 1, &submitInfo, frame->inFlightFence) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    VkPresentInfoKHR presentInfo =
    {
        .sType               = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount  = 1,
        .pWaitSemaphores     = &frame->renderFinishedSemaphore,
        .pResults            = nullptr,

        .swapchainCount  = 1,
        .pSwapchains     = &context->swapchainItems.swapchain,
        .pImageIndices   = &imageIndex,
    };

    // Todo(Leo): Should we do something with this??
    VkResult result = vkQueuePresentKHR(context->presentQueue, &presentInfo);

    #pragma message ("Is this correct place to advance??????")
    // Should we have a proper "all rendering done"-function?
    advance_virtual_frame(context);

    // // Todo, Study(Leo): Somebody on interenet told us to do this. No Idea why???
    // // Note(Leo): Do after presenting to not interrupt semaphores at whrong time
    // if(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context->framebufferResized)
    // {
    //     context->framebufferResized = false;
    //     recreate_swapchain(context);
    // }
    // else if (result != VK_SUCCESS)
    // {
    //     throw std::runtime_error("Failed to present swapchain");
    // }
}


void
vulkan::record_draw_command(VulkanContext * context, ModelHandle model, Matrix44 transform)
{
    // // Todo(Leo): Single mapping is really enough, offsets can be used here too
    // Note(Leo): using this we can use separate buffer sections for each framebuffer, not sure if necessary though
    uint32 modelUniformBufferOffset = context->currentUniformBufferOffset;
    uint32 modelMatrixSize          = get_aligned_uniform_buffer_size(context, sizeof(transform));

    context->currentUniformBufferOffset += modelMatrixSize;

    Matrix44 * pModelMatrix;
    vkMapMemory(context->device, context->modelUniformBuffer.memory, modelUniformBufferOffset, modelMatrixSize, 0, (void**)&pModelMatrix);
    *pModelMatrix = transform;
    vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

    // std::cout << "[vulkan::record_draw_command()]\n";
    DEVELOPMENT_ASSERT(context->canDraw, "Invalid call to record_draw_command() when prepare_drawing() has not been called.")

    VkCommandBuffer commandBuffer   = get_current_virtual_frame(context)->commandBuffer;
 
    MeshHandle meshHandle           = context->loadedModels[model].mesh;
    MaterialHandle materialHandle   = context->loadedModels[model].material;

    // Todo(Leo): This was not needed elsewhere so I brought it here, it can be removed
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


    VulkanRenderInfo renderInfo =
    {
        .meshBuffer             = context->loadedMeshes[meshHandle].buffer, 
        .vertexOffset           = context->loadedMeshes[meshHandle].vertexOffset,
        .indexOffset            = context->loadedMeshes[meshHandle].indexOffset,
        .indexCount             = context->loadedMeshes[meshHandle].indexCount,
        .indexType              = context->loadedMeshes[meshHandle].indexType,

        .uniformBufferOffset    = modelUniformBufferOffset,//context->loadedModels[model].uniformBufferOffset,
        
        .materialIndex          = (uint32)materialHandle,
    };

    // Todo(Leo): This is bad, these will be about to change
    enum : uint32
    {
        LAYOUT_SCENE    = 0,
        LAYOUT_MATERIAL = 1,
        LAYOUT_MODEL    = 2,
    };


    auto materialIndex          = renderInfo.materialIndex;
    PipelineHandle newPipeline  = context->loadedMaterials[materialIndex].pipeline;
    if (newPipeline != context->currentBoundPipeline)
    {
        context->currentBoundPipeline   = newPipeline;
    
        // Material type
        vkCmdBindPipeline(      commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                context->loadedPipelines[context->currentBoundPipeline].pipeline);
        // Scene
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                context->loadedPipelines[context->currentBoundPipeline].layout,
                                LAYOUT_SCENE,
                                1, 
                                &context->uniformDescriptorSets.scene,
                                0,
                                nullptr);
    }

    // Bind material
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            context->loadedPipelines[context->currentBoundPipeline].layout,
                            LAYOUT_MATERIAL,
                            1, 
                            &context->loadedMaterials[materialIndex].descriptorSet,
                            0,
                            nullptr);
    // Bind model info
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &renderInfo.meshBuffer, &renderInfo.vertexOffset);
    vkCmdBindIndexBuffer(commandBuffer, renderInfo.meshBuffer, renderInfo.indexOffset, renderInfo.indexType);

    // Bind entity transform
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            context->loadedPipelines[context->currentBoundPipeline].layout,
                            LAYOUT_MODEL,
                            1,
                            &context->uniformDescriptorSets.model,
                            1,
                            &renderInfo.uniformBufferOffset);

    vkCmdDrawIndexed(commandBuffer, renderInfo.indexCount, 1, 0, 0, 0);
}

void
vulkan::record_line_draw_command(VulkanContext * context, Vector3 start, Vector3 end, float4 color)
{
    /*
    vulkan bufferless drawing
    https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
    */
    DEVELOPMENT_ASSERT(context->canDraw, "Invalid call to record_line_draw_command() when prepare_drawing() has not been called.")

    VkCommandBuffer commandBuffer = get_current_virtual_frame(context)->commandBuffer;//context->frameCommandBuffers[context->currentDrawFrameIndex];
 
    enum : uint32
    {
        LAYOUT_SCENE    = 0,
        LAYOUT_MATERIAL = 1,
        LAYOUT_MODEL    = 2,
    };

    Vector4 pushConstants [] = 
    {
        {start.x, start.y, start.z, 0},
        {end.x, end.y, end.z, 0},
        color,
    };

    vkCmdPushConstants(commandBuffer, context->lineDrawPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), pushConstants);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->lineDrawPipeline.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->lineDrawPipeline.layout,
                            LAYOUT_SCENE, 1, &context->uniformDescriptorSets.scene, 0, nullptr);
    vkCmdDraw(commandBuffer, 2, 1, 0, 0);
}