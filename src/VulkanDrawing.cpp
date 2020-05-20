/*=============================================================================
Leo Tamminen
shophorn @ internet

Vulkan drawing functions' definitions.
=============================================================================*/
namespace vulkan_drawing_internal_
{
    /* Note(Leo): This is EXPERIMENTAL. Idea is that we don't need to always
    create a new struct, but we can just reuse same. Vulkan functions always want
    a pointer to struct, so we cannot use a return value from a function, but this
    way we always have that struct here ready for us and we modify it accordingly
    with each call and return pointer to it. We can do this, since each thread 
    only has one usage of this at any time.

    Returned pointer is only valid until this is called again. Or rather, pointer
    will stay valid for the duration of thread, but values of struct will not. */

    thread_local VkCommandBufferInheritanceInfo global_inheritanceInfo_ = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
    VkCommandBufferInheritanceInfo const *

    get_vk_command_buffer_inheritance_info(VkRenderPass renderPass, VkFramebuffer framebuffer)
    {
        global_inheritanceInfo_.renderPass = renderPass;
        global_inheritanceInfo_.framebuffer = framebuffer;

        return &global_inheritanceInfo_;
    }
}

void 
vulkan::update_camera(VulkanContext * context, Camera const * camera)
{
    // Todo(Leo): alignment

    // Note (Leo): map vulkan memory directly to right type so we can easily avoid one (small) memcpy per frame
    CameraUniformBuffer * pUbo;
    vkMapMemory(context->device, context->sceneUniformBuffer.memory,
                context->cameraUniformOffset, sizeof(CameraUniformBuffer), 0, (void**)&pUbo);

    pUbo->view          = get_view_transform(camera);
    pUbo->projection   = get_projection_transform(camera);

    vkUnmapMemory(context->device, context->sceneUniformBuffer.memory);
}

void
vulkan::update_lighting(VulkanContext * context, Light const * light, Camera const * camera, v3 ambient)
{
    LightingUniformBuffer * lightPtr;
    vkMapMemory(context->device, context->sceneUniformBuffer.memory,
                context->lightingUniformOffset, sizeof(LightingUniformBuffer), 0, (void**)&lightPtr);

    lightPtr->direction    = v3_to_v4(light->direction, 0);
    lightPtr->color        = v3_to_v4(light->color, 0);
    lightPtr->ambient      = v3_to_v4(ambient, 0);

    vkUnmapMemory(context->device, context->sceneUniformBuffer.memory);

    // ------------------------------------------------------------------------------

    m44 lightViewProjection = get_light_view_projection (light, camera);

    CameraUniformBuffer * pUbo;
    vkMapMemory(context->device, context->sceneUniformBuffer.memory,
                context->cameraUniformOffset, sizeof(CameraUniformBuffer), 0, (void**)&pUbo);

    pUbo->lightViewProjection = lightViewProjection;

    vkUnmapMemory(context->device, context->sceneUniformBuffer.memory);
}

void
vulkan::prepare_drawing(VulkanContext * context)
{
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
                                                array_count(attachments),
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
    
    VkCommandBufferBeginInfo sceneCmdBeginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
        .pInheritanceInfo = vulkan_drawing_internal_::get_vk_command_buffer_inheritance_info(context->drawingResources.renderPass, frame->framebuffer),
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

    VkCommandBufferBeginInfo shadowCmdBeginInfo =
    {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,

        // Note(Leo): the pointer in this is actually same as above, but this function call changes values of struct at the other end of pointer.
        .pInheritanceInfo = vulkan_drawing_internal_::get_vk_command_buffer_inheritance_info(context->shadowPass.renderPass, context->shadowPass.framebuffer),
    };
    VULKAN_CHECK(vkBeginCommandBuffer(frame->commandBuffers.offscreen, &shadowCmdBeginInfo));

}

void
vulkan::finish_drawing(VulkanContext * context)
{
    Assert(context->canDraw);
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
        
        .renderArea = {
            .offset = {0,0},
            .extent  = {context->shadowPass.width, context->shadowPass.height},
        },

        .clearValueCount    = 1,
        .pClearValues       = &shadowClearValue,
    };
    vkCmdBeginRenderPass(frame->commandBuffers.primary, &shadowRenderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    vkCmdExecuteCommands(frame->commandBuffers.primary, 1, &frame->commandBuffers.offscreen);
    vkCmdEndRenderPass(frame->commandBuffers.primary);

    // Note(Leo): Debug quad, MaterialHandle::Null directs to debug material where shadow image is bound
    // record_gui_draw_command(context, {1500, 20}, {400, 400}, MaterialHandle::Null, {0,1,1,1});

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
        
        .renderArea = {
            .offset  = {0, 0},
            .extent  = context->drawingResources.extent,
        },
        
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
        .waitSemaphoreCount     = array_count(waitStages),
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
        logVulkan() << FILE_ADDRESS << "Present result = " << to_str(result);
    }

    advance_virtual_frame(context);
}


void
vulkan::record_draw_command(VulkanContext * context, ModelHandle model, m44 transform, bool32 castShadow, m44 const * bones, u32 bonesCount)
{
    Assert(context->canDraw);

    /* Todo(Leo): Get rid of these, we can just as well get them directly from user.
    That is more flexible and then we don't need to save that data in multiple places. */
    MeshHandle meshHandle           = context->loadedModels[model].mesh;
    MaterialHandle materialHandle   = context->loadedModels[model].material;

    u32 uniformBufferOffset = context->currentUniformBufferOffset;
    u32 uniformBufferSize   = get_aligned_uniform_buffer_size(context, sizeof(ModelUniformBuffer));

    context->currentUniformBufferOffset += uniformBufferSize;

    Assert(context->currentUniformBufferOffset <= context->modelUniformBuffer.size);

    ModelUniformBuffer * pBuffer;

    // Optimize(Leo): Just map this once when rendering starts
    vkMapMemory(context->device, context->modelUniformBuffer.memory, uniformBufferOffset, uniformBufferSize, 0, (void**)&pBuffer);
    
    pBuffer->localToWorld = transform;
    pBuffer->isAnimated = bonesCount;

    Assert(bonesCount <= array_count(pBuffer->bonesToLocal));    
    copy_memory(pBuffer->bonesToLocal, bones, sizeof(m44) * bonesCount);

    vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

    // ---------------------------------------------------------------

    VulkanVirtualFrame * frame = get_current_virtual_frame(context);
 
    auto * mesh     = get_loaded_mesh(context, meshHandle);
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
        context->defaultGuiMaterial.descriptorSet,
        // context->shadowMapDescriptorSet
    };

    // Note(Leo): this works, because models are only dynamic set
    u32 dynamicOffsets [] = {uniformBufferOffset};

    vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                            0, array_count(sets), sets,
                            array_count(dynamicOffsets), dynamicOffsets);


    vkCmdBindVertexBuffers(frame->commandBuffers.scene, 0, 1, &mesh->bufferReference, &mesh->vertexOffset);
    vkCmdBindIndexBuffer(frame->commandBuffers.scene, mesh->bufferReference, mesh->indexOffset, mesh->indexType);

    vkCmdDrawIndexed(frame->commandBuffers.scene, mesh->indexCount, 1, 0, 0, 0);

    // ------------------------------------------------
    ///////////////////////////
    ///   SHADOW PASS       ///
    ///////////////////////////
    if (castShadow)
    {
        vkCmdBindVertexBuffers(frame->commandBuffers.offscreen, 0, 1, &mesh->bufferReference, &mesh->vertexOffset);
        vkCmdBindIndexBuffer(frame->commandBuffers.offscreen, mesh->bufferReference, mesh->indexOffset, mesh->indexType);

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
                                &uniformBufferOffset);

        vkCmdDrawIndexed(frame->commandBuffers.offscreen, mesh->indexCount, 1, 0, 0, 0);
    }
}

void
vulkan::record_line_draw_command(VulkanContext * context, v3 start, v3 end, float width, v4 color)
{
    /*
    vulkan bufferless drawing
    https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
    */
    DEBUG_ASSERT(context->canDraw, "Invalid call to record_line_draw_command() when prepare_drawing() has not been called.")

    VkCommandBuffer commandBuffer = get_current_virtual_frame(context)->commandBuffers.scene;

    // Todo(Leo): Define some struct etc. for this
    v4 pushConstants [] = 
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
    v2 bottomLeft;
    v2 bottomRight;
    v2 topLeft;
    v2 topRight;
    v4 color;
};

void vulkan::record_gui_draw_command(VulkanContext * context, v2 position, v2 size, MaterialHandle materialHandle, v4 color)
{
    auto * frame = get_current_virtual_frame(context);

    auto transform_point = [context](v2 position) -> v2
    {
        // Note(Leo): This is temporarily here only, aka scale to fit width.
        const v2 referenceResolution = {1920, 1080};
        float ratio = context->drawingResources.extent.width / referenceResolution.x;

        float x = (position.x / context->drawingResources.extent.width) * ratio * 2.0f - 1.0f;
        float y = (position.y / context->drawingResources.extent.height) * ratio * 2.0f - 1.0f;

        return {x, y};
    };

    VulkanGuiPushConstants pushConstants =
    {
        transform_point(position),
        transform_point(position + v2{size.x, 0}),
        transform_point(position + v2{0, size.y}),
        transform_point(position + size),
        color,
    };

    // // Todo(Leo): this is not good idea. Proper toggles etc and not null pointers for flow control, said wise in the internets
    auto * pipeline = is_valid_handle(materialHandle) ? &context->guiDrawPipeline : &context->shadowPass.debugView;
    auto * material = is_valid_handle(materialHandle) ? get_loaded_gui_material(context, materialHandle) :
                                                        &context->defaultGuiMaterial;

    vkCmdPushConstants( frame->commandBuffers.scene, pipeline->layout,
                        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

    vkCmdBindPipeline(  frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline->pipeline);

    vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                            0, 1, &material->descriptorSet,
                            0, nullptr);

    vkCmdDraw(frame->commandBuffers.scene, 4, 1, 0, 0);

    // Note(Leo): This must be done so that next time we draw normally, we know to bind those descriptors again
    context->currentBoundPipeline = PipelineHandle::Null;
}

internal void FSVulkanImpl_draw_screen_rect(VulkanContext * context, v2 position, v2 size, v2 uvPosition, v2 uvSize, MaterialHandle materialHandle, v4 color)
{
    auto * frame = vulkan::get_current_virtual_frame(context);

    auto transform_point = [context](v2 position) -> v2
    {
        // Note(Leo): This is temporarily here only, aka scale to fit width.
        const v2 referenceResolution = {1920, 1080};
        float ratio = context->drawingResources.extent.width / referenceResolution.x;

        float x = (position.x / context->drawingResources.extent.width) * ratio * 2.0f - 1.0f;
        float y = (position.y / context->drawingResources.extent.height) * ratio * 2.0f - 1.0f;

        return {x, y};
    };

    v2 transformedStart = transform_point(position);
    v2 transformedEnd = transform_point(position + size);
    v2 transformedDelta = transformedEnd - transformedStart;


    VulkanGuiPushConstants pushConstants =
    {
        // transform_point(position),
        // transform_point(size),
        transformedStart,
        transformedDelta,

        uvPosition, 
        uvSize,

        color,
    };



    // auto * pipeline = is_valid_handle(materialHandle) ? &context->guiDrawPipeline : &context->shadowPass.debugView;
    auto * material = is_valid_handle(materialHandle) ? vulkan::get_loaded_material(context, materialHandle) :
                                                        &context->defaultGuiMaterial;
    auto * pipeline = vulkan::get_loaded_pipeline(context, material->pipeline);

    vkCmdPushConstants( frame->commandBuffers.scene, pipeline->layout,
                        VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);

    vkCmdBindPipeline(  frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS,
                        pipeline->pipeline);

    vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->layout,
                            0, 1, &material->descriptorSet,
                            0, nullptr);

    vkCmdDraw(frame->commandBuffers.scene, 4, 1, 0, 0);

    // Note(Leo): This must be done so that next time we draw normally, we know to bind those descriptors again
    context->currentBoundPipeline = PipelineHandle::Null;
}

// Note(Leo): Quad with two triangles, and offsettable uv.s
// struct SimpleQuad
// {
//     v2 position;
//     v2 size;
//     v2 uvPosition;
//     v2 uvSize;
// };

// // void vulkan_draw_simple_quad(VulkanContext * context, SimpleQuad * quad, MaterialHandle material, v4 color)
// // {
// //     auto frame = get_current_virtual_frame(context);
    
// // }

// // void vulkan_draw_nine_sliced_quad();