/*=============================================================================
Leo Tamminen
shophorn @ internet

Vulkan drawing functions' definitions.
=============================================================================*/
void 
vulkan::update_camera(VulkanContext * context, Camera const * camera)
{
    // Todo(Leo): alignment

    // Note (Leo): map vulkan memory directly to right type so we can easily avoid one (small) memcpy per frame
    VulkanCameraUniformBufferObject * pUbo;
    vkMapMemory(context->device, context->sceneUniformBuffer.memory,
                context->cameraUniformOffset, sizeof(VulkanCameraUniformBufferObject), 0, (void**)&pUbo);

    pUbo->view          = get_view_projection(camera);
    pUbo->perspective   = get_perspective_projection(camera);

    vkUnmapMemory(context->device, context->sceneUniformBuffer.memory);
}

void
vulkan::prepare_drawing(VulkanContext * context)
{
    LightingUniformBufferObject * light;
    vkMapMemory(context->device, context->sceneUniformBuffer.memory,
                context->lightingUniformOffset, sizeof(light), 0, (void**)&light);

    light->direction    = size_cast<vector4>(vector::normalize(vector3{2, 1, -1.5f}));
    light->color        = {0.5, 0.5, 1};
    light->ambient      = {0.1, 0.1, 0.2};

    vkUnmapMemory(context->device, context->sceneUniformBuffer.memory);

    // -----------------------------------------------------------
    
    DEBUG_ASSERT((context->canDraw == false), "Invalid call to prepare_drawing() when finish_drawing() has not been called.")
    context->canDraw                    = true;

    context->currentUniformBufferOffset = 0;
    context->currentBoundPipeline       = PipelineHandle::Null;

    auto * frame = get_current_virtual_frame(context);

    // Note(Leo): We recreate these everytime we are here.
    // https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-4#inpage-nav-5
    vkDestroyFramebuffer(context->device, frame->framebuffer, nullptr);

    VkImageView attachments [] =
    {
        context->drawingResources.colorImageView,
        context->drawingResources.depthImageView,

        // Todo(Leo): make nice accessor :D
        context->drawingResources.imageViews[context->currentDrawFrameIndex],
    };

    frame->framebuffer = make_vk_framebuffer(   context->device,
                                                context->drawingResources.renderPass,
                                                get_array_count(attachments),
                                                attachments,
                                                context->drawingResources.extent.width,
                                                context->drawingResources.extent.height);    

    VkCommandBufferBeginInfo primaryCmdBeginInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo   = nullptr,
    };

    /* Note (Leo): beginning command buffer implicitly resets it, if we have specified
    VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT in command pool creation */
    VULKAN_CHECK(vkBeginCommandBuffer(frame->commandBuffers.primary, &primaryCmdBeginInfo));
    
    VkCommandBufferInheritanceInfo inheritanceInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .renderPass             = context->drawingResources.renderPass,

        // Todo(Leo): so far we only have 1 subpass, but whenever it changes, this probably must too
        .subpass                = 0,
        .framebuffer            = frame->framebuffer,
        .occlusionQueryEnable   = VK_FALSE,
        .queryFlags             = 0,
        .pipelineStatistics     = 0,
    };

    VkCommandBufferBeginInfo sceneCmdBeginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &inheritanceInfo,
    };
    VULKAN_CHECK (vkBeginCommandBuffer(frame->commandBuffers.scene, &sceneCmdBeginInfo));

    VkViewport viewport =
    {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = (float) context->drawingResources.extent.width,
        .height     = (float) context->drawingResources.extent.height,
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };
    vkCmdSetViewport(frame->commandBuffers.scene, 0, 1, &viewport);

    VkRect2D scissor =
    {
        .offset = {0, 0},
        .extent = context->drawingResources.extent,
    };
    vkCmdSetScissor(frame->commandBuffers.scene, 0, 1, &scissor);

    // -----------------------------------------------------
    VkCommandBufferInheritanceInfo shadowInheritanceInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
        .renderPass             = context->shadowPass.renderPass,

        // Todo(Leo): so far we only have 1 subpass, but whenever it changes, this probably must too
        .subpass                = 0,
        .framebuffer            = context->shadowPass.framebuffer,
        .occlusionQueryEnable   = VK_FALSE,
        .queryFlags             = 0,
        .pipelineStatistics     = 0,
    };

    VkCommandBufferBeginInfo shadowCmdBeginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = &shadowInheritanceInfo,
    };
    VULKAN_CHECK(vkBeginCommandBuffer(frame->commandBuffers.offscreen, &shadowCmdBeginInfo));

}

void
vulkan::finish_drawing(VulkanContext * context)
{
    assert(context->canDraw);
    context->canDraw = false;

    VulkanVirtualFrame * frame = get_current_virtual_frame(context);

    // SHADOWS RENDER PASS ----------------------------------------------------
    VULKAN_CHECK(vkEndCommandBuffer(frame->commandBuffers.offscreen));

    VkClearValue shadowClearValue = { .depthStencil = {1.0f, 0} };
    VkRenderPassBeginInfo shadowRenderPassInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass         = context->shadowPass.renderPass,
        .framebuffer        = context->shadowPass.framebuffer,
        
        .renderArea.offset  = {0, 0},
        .renderArea.extent  = {context->shadowPass.width, context->shadowPass.height},

        .clearValueCount    = 1,
        .pClearValues       = &shadowClearValue,
    };
    vkCmdBeginRenderPass(frame->commandBuffers.primary, &shadowRenderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    vkCmdExecuteCommands(frame->commandBuffers.primary, 1, &frame->commandBuffers.offscreen);
    vkCmdEndRenderPass(frame->commandBuffers.primary);

    // Note(Leo): Debug quad, MaterialHandle::Null directs to debug material where shadow image is bound
    record_gui_draw_command(context, {1500, 20}, {400, 400}, MaterialHandle::Null, {0,1,1,1});

    // MAIN SCENE RENDER PASS -------------------------------------------------
    VULKAN_CHECK(vkEndCommandBuffer(frame->commandBuffers.scene));

    VkClearValue clearValues [] =
    {
        { .color = {0.35f, 0.0f, 0.35f, 1.0f} },
        { .depthStencil = {1.0f, 0} }
    };
    VkRenderPassBeginInfo renderPassInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass         = context->drawingResources.renderPass,
        .framebuffer        = frame->framebuffer,
        
        .renderArea.offset  = {0, 0},
        .renderArea.extent  = context->drawingResources.extent,

        .clearValueCount    = 2,
        .pClearValues       = clearValues,
    };
    vkCmdBeginRenderPass(frame->commandBuffers.primary, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    vkCmdExecuteCommands(frame->commandBuffers.primary, 1, &frame->commandBuffers.scene);
    vkCmdEndRenderPass(frame->commandBuffers.primary);
    

    // PRIMARY COMMAND BUFFER -------------------------------------------------
    VULKAN_CHECK(vkEndCommandBuffer(frame->commandBuffers.primary));

    /* Note(Leo): these have to do with sceneloading in game layer. We are then unloading
    all resources associated with command buffer, which makes it invalid to submit to queue */
    bool32 skipFrame            = context->sceneUnloaded;
    context->sceneUnloaded      = false;

    VkPipelineStageFlags waitStages [] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT  };
    VkSubmitInfo submitInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount     = get_array_count(waitStages),
        .pWaitSemaphores        = &frame->imageAvailableSemaphore,
        .pWaitDstStageMask      = waitStages,
        .commandBufferCount     = (u32)(skipFrame ? 0 : 1),
        .pCommandBuffers        = &frame->commandBuffers.primary,
        .signalSemaphoreCount   = 1,
        .pSignalSemaphores      = &frame->renderFinishedSemaphore,
    };
    vkResetFences(context->device, 1, &frame->inFlightFence);
    VULKAN_CHECK(vkQueueSubmit(context->queues.graphics, 1, &submitInfo, frame->inFlightFence));

    VkPresentInfoKHR presentInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &frame->renderFinishedSemaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &context->drawingResources.swapchain,
        .pImageIndices      = &context->currentDrawFrameIndex,
        .pResults           = nullptr,
    };

    // Todo(Leo): Should we do something with this? We are handling VK_ERROR_OUT_OF_DATE_KHR elsewhere anyway.
    VkResult result = vkQueuePresentKHR(context->queues.present, &presentInfo);
    if (result != VK_SUCCESS)
    {
        std::cout << "[finish_drawing()]: present result = " << result << "\n";
    }

    advance_virtual_frame(context);
}


void
vulkan::record_draw_command(VulkanContext * context, ModelHandle model, Matrix44 transform)
{
    /* Todo(Leo): Get rid of these, we can just as well get them directly from user.
    That is more flexible and then we don't need to save that data in multiple places. */
    MeshHandle meshHandle           = context->loadedModels[model].mesh;
    MaterialHandle materialHandle   = context->loadedModels[model].material;

    u32 modelUniformBufferOffset = context->currentUniformBufferOffset;
    u32 modelMatrixSize          = get_aligned_uniform_buffer_size(context, sizeof(transform));

    context->currentUniformBufferOffset += modelMatrixSize;

    // Todo(Leo): Just map this once when rendering starts
    Matrix44 * pModelMatrix;
    vkMapMemory(context->device, context->modelUniformBuffer.memory, modelUniformBufferOffset, modelMatrixSize, 0, (void**)&pModelMatrix);
    *pModelMatrix = transform;
    vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

    DEBUG_ASSERT(context->canDraw, "Invalid call to record_draw_command() when prepare_drawing() has not been called.")

    VulkanVirtualFrame * frame      = get_current_virtual_frame(context);
 
    auto * mesh = get_loaded_mesh(context, meshHandle);
    auto * material = get_loaded_material(context, materialHandle);
    auto * pipeline = get_loaded_pipeline(context, material->pipeline);

    // Material type
    vkCmdBindPipeline(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->pipeline);
    
    VkDescriptorSet sets [] =
    {   
        context->uniformDescriptorSets.camera,
        material->descriptorSet,
        context->uniformDescriptorSets.model,
        context->uniformDescriptorSets.lighting,
    };

    // Note(Leo): this works, because models are only dynamic set
    u32 dynamicOffsets [] = {modelUniformBufferOffset};

    vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                            0, get_array_count(sets), sets,
                            get_array_count(dynamicOffsets), dynamicOffsets);


    vkCmdBindVertexBuffers(frame->commandBuffers.scene, 0, 1, &mesh->buffer, &mesh->vertexOffset);
    vkCmdBindIndexBuffer(frame->commandBuffers.scene, mesh->buffer, mesh->indexOffset, mesh->indexType);

    vkCmdDrawIndexed(frame->commandBuffers.scene, mesh->indexCount, 1, 0, 0, 0);

    // ------------------------------------------------

    // Bind model info
    vkCmdBindVertexBuffers(frame->commandBuffers.offscreen, 0, 1, &mesh->buffer, &mesh->vertexOffset);
    vkCmdBindIndexBuffer(frame->commandBuffers.offscreen, mesh->buffer, mesh->indexOffset, mesh->indexType);


    u32 shadowPassSceneSet = 0;
    u32 shadowPassModelSet = 1;

    VkDescriptorSet shadowSets [] =
    {
        context->uniformDescriptorSets.camera,
        context->uniformDescriptorSets.model,
    };

    vkCmdBindPipeline(frame->commandBuffers.offscreen, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowPass.pipeline);
    vkCmdBindDescriptorSets(frame->commandBuffers.offscreen,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            context->shadowPass.layout,
                            0,
                            2,
                            shadowSets,
                            1,
                            &modelUniformBufferOffset);

    vkCmdDrawIndexed(frame->commandBuffers.offscreen, mesh->indexCount, 1, 0, 0, 0);
}

void
vulkan::record_line_draw_command(VulkanContext * context, vector3 start, vector3 end, float width, float4 color)
{
    /*
    vulkan bufferless drawing
    https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
    */
    DEBUG_ASSERT(context->canDraw, "Invalid call to record_line_draw_command() when prepare_drawing() has not been called.")

    VkCommandBuffer commandBuffer = get_current_virtual_frame(context)->commandBuffers.scene;

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
                            0, 1, &context->uniformDescriptorSets.camera, 0, nullptr);
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

    auto transform_point = [context](vector2 position) -> vector2
    {
        // Note(Leo): This is temporarily here only, aka scale to fit width.
        const vector2 referenceResolution = {1920, 1080};
        float ratio = context->drawingResources.extent.width / referenceResolution.x;

        float x = (position.x / context->drawingResources.extent.width) * ratio * 2.0f - 1.0f;
        float y = (position.y / context->drawingResources.extent.height) * ratio * 2.0f - 1.0f;

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

    auto * pipeline = is_valid_handle(materialHandle) ? &context->guiDrawPipeline : &context->shadowPass.debugView;
    auto * material = is_valid_handle(materialHandle) ? get_loaded_gui_material(context, materialHandle) :
                                                        &context->defaultGuiMaterial;

    vkCmdPushConstants( frame->commandBuffers.scene, pipeline->layout,
                        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

    vkCmdBindPipeline(  frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline->pipeline);

    // // Todo(Leo): this is not good idea. Proper toggles etc and not null pointers for flow control, said wise in the internets
    vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                            0, 1, &material->descriptorSet,
                            0, nullptr);

    vkCmdDraw(frame->commandBuffers.scene, 4, 1, 0, 0);

    // Note(Leo): This must be done so that next time we draw normally, we know to bind thos descriptors again
    context->currentBoundPipeline = PipelineHandle::Null;
}
