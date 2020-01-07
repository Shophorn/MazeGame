/*=============================================================================
Leo Tamminen

Implementation of IGraphicsContext interface for VulkanContext
=============================================================================*/
MeshHandle
VulkanContext::PushMesh(MeshAsset * mesh)
{
    uint64 indexBufferSize = mesh->indices.count() * sizeof(mesh->indices[0]);
    uint64 vertexBufferSize = mesh->vertices.count() * sizeof(mesh->vertices[0]);
    uint64 totalBufferSize = indexBufferSize + vertexBufferSize;

    uint64 indexOffset = 0;
    uint64 vertexOffset = indexBufferSize;

    uint8 * data;
    vkMapMemory(this->device, this->stagingBufferPool.memory, 0, totalBufferSize, 0, (void**)&data);
    memcpy(data, &mesh->indices[0], indexBufferSize);
    data += indexBufferSize;
    memcpy(data, &mesh->vertices[0], vertexBufferSize);
    vkUnmapMemory(this->device, this->stagingBufferPool.memory);

    VkCommandBuffer commandBuffer = vulkan::BeginOneTimeCommandBuffer(this->device, this->commandPool);

    VkBufferCopy copyRegion = { 0, this->staticMeshPool.used, totalBufferSize };
    vkCmdCopyBuffer(commandBuffer, this->stagingBufferPool.buffer, this->staticMeshPool.buffer, 1, &copyRegion);

    vulkan::execute_command_buffer(this->device, this->commandPool, this->graphicsQueue, commandBuffer);

    VulkanModel model = {};

    model.buffer = this->staticMeshPool.buffer;
    model.memory = this->staticMeshPool.memory;

    model.vertexOffset = this->staticMeshPool.used + vertexOffset;
    model.indexOffset = this->staticMeshPool.used + indexOffset;
    
    this->staticMeshPool.used += totalBufferSize;

    model.indexCount = mesh->indices.count();
    model.indexType = vulkan::ConvertIndexType(mesh->indexType);

    uint32 modelIndex = this->loadedModels.size();

    this->loadedModels.push_back(model);

    MeshHandle resultHandle = { modelIndex };

    return resultHandle;
}

TextureHandle
VulkanContext::PushTexture (TextureAsset * texture)
{
    TextureHandle handle = { this->loadedTextures.size() };
    this->loadedTextures.push_back(vulkan::make_texture(texture, this));
    return handle;
}

TextureHandle
VulkanContext::push_cubemap(TextureAsset * assets)
{
    TextureHandle handle = { this->loadedTextures.size() };
    this->loadedTextures.push_back(vulkan::make_cubemap(this, assets));
    return handle;    
}


MaterialHandle
VulkanContext::PushMaterial (MaterialAsset * asset)
{
    MaterialHandle resultHandle = {this->loadedMaterials.size()};
    VulkanMaterial material = 
    {
        .pipeline = asset->pipeline,
        .descriptorSet = vulkan::CreateMaterialDescriptorSets(this, asset->albedo, asset->metallic, asset->testMask)
    };
    this->loadedMaterials.push_back(material);

    return resultHandle;
}

RenderedObjectHandle
VulkanContext::PushRenderedObject(MeshHandle mesh, MaterialHandle material)
{
    uint32 objectIndex = loadedRenderedObjects.size();

    VulkanRenderedObject object =
    {
        .mesh     = mesh,
        .material = material,
    };

    uint32 memorySizePerModelMatrix = align_up_to(
        this->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
        sizeof(Matrix44));
    object.uniformBufferOffset = objectIndex * memorySizePerModelMatrix;

    loadedRenderedObjects.push_back(object);

    RenderedObjectHandle resultHandle = { objectIndex };
    return resultHandle;    
}


// Note(Leo): Destroy vulkan objects only but keep load infos.
internal void
destroy_loaded_pipelines(VulkanContext * context)
{
    for (int i = 0; i < context->loadedPipelines.size(); ++i)
    {
        vkDestroyPipeline(context->device, context->loadedPipelines[i].pipeline, nullptr);
        vkDestroyPipelineLayout(context->device, context->loadedPipelines[i].layout, nullptr);

        context->loadedPipelines[i].layout      = VK_NULL_HANDLE;
        context->loadedPipelines[i].pipeline    = VK_NULL_HANDLE;
    }
}

void 
VulkanContext::UnloadAll()
{
    vkDeviceWaitIdle(device);

    // Meshes
    staticMeshPool.used = 0;
    loadedModels.resize (0);

    // Textures
    for (int32 imageIndex = 0; imageIndex < loadedTextures.size(); ++imageIndex)
    {
        vulkan::DestroyImageTexture(this, &loadedTextures[imageIndex]);
    }
    loadedTextures.resize(0);

    // Materials
    vkResetDescriptorPool(device, materialDescriptorPool, 0);   
    loadedMaterials.resize(0);

    // Rendered objects
    loadedRenderedObjects.resize(0);

    destroy_loaded_pipelines(this);
    loadedPipelines.resize(0);

    this->abortFrameDrawing = true;
}

PipelineHandle
VulkanContext::push_pipeline(const char * vertexShaderPath, const char * fragmentShaderPath, platform::PipelineOptions options)
{
    VulkanPipelineLoadInfo info = 
    {
        .vertexShaderPath   = vertexShaderPath,
        .fragmentShaderPath = fragmentShaderPath,
        .options            = options
    };
    auto pipeline = vulkan::make_pipeline(this, info);
    uint64 index = loadedPipelines.size();
    loadedPipelines.push_back(pipeline);

    return {index};
}

internal void
recreate_loaded_pipelines(VulkanContext * context)
{
    destroy_loaded_pipelines(context);

    for (int i = 0; i < context->loadedPipelines.size(); ++i)
    {
        auto pipeline = vulkan::make_pipeline(context, context->loadedPipelines[i].info);
        context->loadedPipelines[i] = pipeline;
    }
}

void
vulkan::start_drawing(VulkanContext * context, uint32 frameIndex)
{
    // std::cout << "[vulkan::start_drawing()]\n";
    
    DEVELOPMENT_ASSERT((context->canDraw == false), "Invalid call to start_drawing() when finish_drawing() has not been called.")
    

    context->currentDrawFrameIndex = frameIndex;
    context->canDraw = true;

    VkCommandBuffer commandBuffer = context->frameCommandBuffers[context->currentDrawFrameIndex];

    // CLEAR COMMAND BUFFER
    vkResetCommandBuffer(commandBuffer, 0);

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
        .framebuffer        = context->frameBuffers[context->currentDrawFrameIndex],
        
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
    // std::cout << "[vulkan::finish_drawing()]\n";


    DEVELOPMENT_ASSERT(context->canDraw, "Invalid call to finish_drawing() when start_drawing() has not been called.")
    context->canDraw = false;
    context->currentBoundPipeline = PipelineHandle::Null;


    VkCommandBuffer commandBuffer = context->frameCommandBuffers[context->currentDrawFrameIndex];
    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to record command buffer");
    }

}

void
vulkan::record_draw_command(VulkanContext * context, RenderedObjectHandle objectHandle, Matrix44 transform)
{
    // std::cout << "[vulkan::record_draw_command()]\n";
    DEVELOPMENT_ASSERT(context->canDraw, "Invalid call to record_draw_command() when start_drawing() has not been called.")

    VkCommandBuffer commandBuffer = context->frameCommandBuffers[context->currentDrawFrameIndex];
 
    MeshHandle meshHandle           = context->loadedRenderedObjects[objectHandle].mesh;
    MaterialHandle materialHandle   = context->loadedRenderedObjects[objectHandle].material;

    VulkanRenderInfo renderInfo =
    {
        .meshBuffer             = context->loadedModels[meshHandle].buffer, 
        .vertexOffset           = context->loadedModels[meshHandle].vertexOffset,
        .indexOffset            = context->loadedModels[meshHandle].indexOffset,
        .indexCount             = context->loadedModels[meshHandle].indexCount,
        .indexType              = context->loadedModels[meshHandle].indexType,

        .uniformBufferOffset    = context->loadedRenderedObjects[objectHandle].uniformBufferOffset,
        
        .materialIndex          = (uint32)materialHandle,
    };

    // Todo(Leo): This is bad, these will be about to change
    enum : uint32
    {
        LAYOUT_SCENE    = 0,
        LAYOUT_MATERIAL = 1,
        LAYOUT_MODEL    = 2,
    };


    auto materialIndex = renderInfo.materialIndex;
    PipelineHandle newPipeline = context->loadedMaterials[materialIndex].pipeline;
    if (newPipeline != context->currentBoundPipeline)
    {
        context->currentBoundPipeline   = newPipeline;
    
        vkCmdBindPipeline(      commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                context->loadedPipelines[context->currentBoundPipeline].pipeline);

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                context->loadedPipelines[context->currentBoundPipeline].layout,
                                LAYOUT_SCENE,
                                1, 
                                &context->sceneDescriptorSets[context->currentDrawFrameIndex],
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
                            &context->descriptorSets[context->currentDrawFrameIndex],
                            1,
                            &renderInfo.uniformBufferOffset);

    vkCmdDrawIndexed(commandBuffer, renderInfo.indexCount, 1, 0, 0, 0);
}

void
vulkan::record_line_draw_command(VulkanContext * context, Vector3 start, Vector3 end, Vector3 color)
{
    /*
    vulkan bufferless drawing
    https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
    */
    DEVELOPMENT_ASSERT(context->canDraw, "Invalid call to record_line_draw_command() when start_drawing() has not been called.")

    VkCommandBuffer commandBuffer = context->frameCommandBuffers[context->currentDrawFrameIndex];
 
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
        {color.x, color.y, color.z, 0},
    };
    vkCmdPushConstants(commandBuffer, context->lineDrawPipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), pushConstants);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->lineDrawPipeline.pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->lineDrawPipeline.layout,
                            LAYOUT_SCENE, 1, &context->sceneDescriptorSets[context->currentDrawFrameIndex], 0, nullptr);
    vkCmdDraw(commandBuffer, 2, 1, 0, 0);
}