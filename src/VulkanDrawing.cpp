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
    DEBUG_ASSERT((context->canDraw == false), "Invalid call to prepare_drawing() when finish_drawing() has not been called.")
    context->canDraw = true;

    context->currentUniformBufferOffset = 0;
    context->currentBoundPipeline = PipelineHandle::Null;

    VulkanVirtualFrame * frame = get_current_virtual_frame(context);

    // Note(Leo): We recreate these everytime we are here.
    // https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-4#inpage-nav-5
    vkDestroyFramebuffer(context->device, frame->framebuffer, nullptr);

    VkImageView attachments [] =
    {
        context->drawingResources.colorImageView,
        context->drawingResources.depthImageView,
        context->swapchainItems.imageViews[context->currentDrawFrameIndex],
    };

    frame->framebuffer = make_framebuffer(  context->device,
                                            context->renderPass,
                                            ARRAY_COUNT(attachments),
                                            attachments,
                                            context->swapchainItems.extent.width,
                                            context->swapchainItems.extent.height);    

    VkCommandBufferBeginInfo primaryCmdBeginInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo   = nullptr,
    };

    /* Note (Leo): beginning command buffer implicitly resets it, if we have specified
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT in command pool creation */
    DEBUG_ASSERT(
        vkBeginCommandBuffer(frame->commandBuffers.primary, &primaryCmdBeginInfo) == VK_SUCCESS,
        "Failed to begin primary command buffer");
    
    VkClearValue clearValues [] =
    {
        { .color = {0.0f, 0.0f, 0.0f, 1.0f} },
        { .depthStencil = {1.0f, 0} }
    };

    VkRenderPassBeginInfo renderPassInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass         = context->renderPass,
        .framebuffer        = frame->framebuffer,
        
        .renderArea.offset  = {0, 0},
        .renderArea.extent  = context->swapchainItems.extent,

        .clearValueCount    = 2,
        .pClearValues       = clearValues,
    };

    vkCmdBeginRenderPass(frame->commandBuffers.primary, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    VkCommandBufferInheritanceInfo inheritanceInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .renderPass = context->renderPass,

        // Todo(Leo): so far we only have 1 subpass, but whenever it changes, this probably must too
        .subpass = 0,
        .framebuffer = frame->framebuffer,
        .occlusionQueryEnable = VK_FALSE,
        .queryFlags = 0,
        .pipelineStatistics = 0,
    };

    VkCommandBufferBeginInfo sceneCmdBeginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritanceInfo,
    };

    DEBUG_ASSERT(
        vkBeginCommandBuffer(frame->commandBuffers.scene, &sceneCmdBeginInfo) == VK_SUCCESS,
        "Failed to begin secondary scene command buffer");
}

void
vulkan::finish_drawing(VulkanContext * context)
{
    DEBUG_ASSERT(context->canDraw, "Invalid call to finish_drawing() when prepare_drawing() has not been called.")
    context->canDraw = false;

    VulkanVirtualFrame * frame = get_current_virtual_frame(context);

    DEBUG_ASSERT(
        vkEndCommandBuffer(frame->commandBuffers.scene) == VK_SUCCESS,
        "Failed to end secondary scene command buffer");

    vkCmdExecuteCommands(frame->commandBuffers.primary, 1, &frame->commandBuffers.scene);
    vkCmdEndRenderPass(frame->commandBuffers.primary);
    
    DEBUG_ASSERT(
        vkEndCommandBuffer(frame->commandBuffers.primary) == VK_SUCCESS,
        "Failed to end primary command buffer");
}


internal void
vulkan::draw_frame(VulkanContext * context, u32 imageIndex)
{
    /* Note(Leo): these have to do with sceneloading in game layer. We are then unloading
    all resources associated with command buffer, which makes it invalid to submit to queue */
    bool32 skipFrame            = context->sceneUnloaded;
    context->sceneUnloaded      = false;

    // u32 frameIndex = context->virtualFrameIndex;
    VulkanVirtualFrame * frame  = get_current_virtual_frame(context);


    /* Note(Leo): reset here, so that if we have recreated swapchain above,
    our fences will be left in signaled state */
    vkResetFences(context->device, 1, &frame->inFlightFence);

    // Note(Leo): We wait for these BEFORE drawing
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    
    VkCommandBuffer commandBuffers []
    {
        frame->commandBuffers.primary,
    };

    VkSubmitInfo submitInfo =
    {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,

        .waitSemaphoreCount     = 1,
        .pWaitSemaphores        = &frame->imageAvailableSemaphore,
        .pWaitDstStageMask      = &waitStage,

        .commandBufferCount     = (u32)(skipFrame ? 0 : (ARRAY_COUNT(commandBuffers))),
        .pCommandBuffers        = commandBuffers,

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
    u32 modelUniformBufferOffset = context->currentUniformBufferOffset;
    u32 modelMatrixSize          = get_aligned_uniform_buffer_size(context, sizeof(transform));

    context->currentUniformBufferOffset += modelMatrixSize;

    Matrix44 * pModelMatrix;
    vkMapMemory(context->device, context->modelUniformBuffer.memory, modelUniformBufferOffset, modelMatrixSize, 0, (void**)&pModelMatrix);
    *pModelMatrix = transform;
    vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

    // std::cout << "[vulkan::record_draw_command()]\n";
    DEBUG_ASSERT(context->canDraw, "Invalid call to record_draw_command() when prepare_drawing() has not been called.")

    VulkanVirtualFrame * frame      = get_current_virtual_frame(context);
    VkCommandBuffer commandBuffer   = frame->commandBuffers.scene;
 
    MeshHandle meshHandle           = context->loadedModels[model].mesh;
    MaterialHandle materialHandle   = context->loadedModels[model].material;

    // Todo(Leo): This was not needed elsewhere so I brought it here, it can be removed
    struct VulkanRenderInfo
    {
        VkBuffer meshBuffer;

        VkDeviceSize vertexOffset;
        VkDeviceSize indexOffset;
        
        u32 indexCount;
        VkIndexType indexType;
        
        u32 uniformBufferOffset;

        u32 materialIndex;
    };


    VulkanRenderInfo renderInfo =
    {
        .meshBuffer             = context->loadedMeshes[meshHandle].buffer, 
        .vertexOffset           = context->loadedMeshes[meshHandle].vertexOffset,
        .indexOffset            = context->loadedMeshes[meshHandle].indexOffset,
        .indexCount             = context->loadedMeshes[meshHandle].indexCount,
        .indexType              = context->loadedMeshes[meshHandle].indexType,

        .uniformBufferOffset    = modelUniformBufferOffset,//context->loadedModels[model].uniformBufferOffset,
        
        .materialIndex          = (u32)materialHandle,
    };

    // Todo(Leo): This is bad, these will be about to change
    enum : u32
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
vulkan::record_line_draw_command(VulkanContext * context, vector3 start, vector3 end, float4 color)
{
    /*
    vulkan bufferless drawing
    https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
    */
    DEBUG_ASSERT(context->canDraw, "Invalid call to record_line_draw_command() when prepare_drawing() has not been called.")

    VkCommandBuffer commandBuffer = get_current_virtual_frame(context)->commandBuffers.scene;
 
    enum : u32
    {
        LAYOUT_SCENE    = 0,
        LAYOUT_MATERIAL = 1,
        LAYOUT_MODEL    = 2,
    };

    // Todo(Leo): Define some struct etc. for this
    vector4 pushConstants [] = 
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

    // Note(Leo): This must be done so that next time we draw normally, we know to bind thos descriptors again
    context->currentBoundPipeline = PipelineHandle::Null;
}

struct VulkanGuiPushConstants
{
    vector2 bottomLeft;
    vector2 bottomRight;
    vector2 topLeft;
    vector2 topRight;
    float4 color;
};

void vulkan::record_gui_draw_command(VulkanContext * context, vector2 position, vector2 size, MaterialHandle materialHandle, float4 color)
{
    auto * frame = get_current_virtual_frame(context);
    enum : u32 { LAYOUT_SET_MATERIAL = 0 };

    auto transform_point = [context](vector2 position) -> vector2
    {
        // Note(Leo): This is temporarily here only, aka scale to fit width.
        const vector2 referenceResolution = {1920, 1080};
        float ratio = context->swapchainItems.extent.width / referenceResolution.x;

        float x = (position.x / context->swapchainItems.extent.width) * ratio * 2.0f - 1.0f;
        float y = (position.y / context->swapchainItems.extent.height) * ratio * 2.0f - 1.0f;

        return {x, y};
    };

    VulkanGuiPushConstants pushConstants =
    {
        transform_point(position),
        transform_point(position + vector2{size.x, 0}),
        transform_point(position + vector2{0, size.y}),
        transform_point(position + size),
        color,
    };

    vkCmdPushConstants( frame->commandBuffers.scene, context->guiDrawPipeline.layout,
                        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

    vkCmdBindPipeline(  frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        context->guiDrawPipeline.pipeline);


    // Todo(Leo): this is not good idea. Proper toggles etc and not null pointers for flow control, said wise in the internets
    if(is_valid_handle(materialHandle))
    {
        vkCmdBindDescriptorSets(    frame->commandBuffers.scene,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                                    context->guiDrawPipeline.layout,
                                    LAYOUT_SET_MATERIAL,
                                    1,
                                    &context->loadedGuiMaterials[materialHandle].descriptorSet,
                                    0,
                                    nullptr);
    }

    vkCmdDraw(frame->commandBuffers.scene, 4, 1, 0, 0);


    // Note(Leo): This must be done so that next time we draw normally, we know to bind thos descriptors again
    context->currentBoundPipeline = PipelineHandle::Null;
}
