 /*=============================================================================
Leo Tamminen

Implementation of Graphics interface for VulkanContext
=============================================================================*/
// Note(Leo): Destroy vulkan objects only but keep load infos.
internal void
destroy_loaded_pipelines(VulkanContext * context)
{
    for (int i = 0; i < context->loadedPipelines.size(); ++i)
    {
        vulkan::destroy_loaded_pipeline(context, &context->loadedPipelines[i]);
    }
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



TextureHandle
vulkan::push_texture (VulkanContext * context, TextureAsset * texture)
{
    TextureHandle handle = { context->loadedTextures.size() };
    context->loadedTextures.push_back(vulkan::make_texture(context, texture));
    return handle;
}

MaterialHandle
vulkan::push_material (VulkanContext * context, MaterialAsset * asset)
{
    MaterialHandle resultHandle = {context->loadedMaterials.size()};
    VulkanMaterial material = 
    {
        .pipeline = asset->pipeline,
        .descriptorSet = vulkan::make_material_descriptor_set(context, asset->pipeline, asset->textures)
    };
    context->loadedMaterials.push_back(material);

    return resultHandle;
}

MaterialHandle
vulkan::push_gui_material (VulkanContext * context, TextureHandle texture)
{
    MaterialHandle resultHandle = {context->loadedGuiMaterials.size()};
    VulkanMaterial material = 
    {
        .pipeline = PipelineHandle::Null,
        .descriptorSet = vulkan::make_material_descriptor_set(context, &context->guiDrawPipeline, texture)
    };
    context->loadedGuiMaterials.push_back(material);

    return resultHandle;
}

MeshHandle
vulkan::push_mesh(VulkanContext * context, MeshAsset * mesh)
{
    u64 indexBufferSize  = mesh->indices.count() * sizeof(mesh->indices[0]);
    u64 vertexBufferSize = mesh->vertices.count() * sizeof(mesh->vertices[0]);
    u64 totalBufferSize  = indexBufferSize + vertexBufferSize;

    u64 indexOffset = 0;
    u64 vertexOffset = indexBufferSize;

    u8 * data;
    vkMapMemory(context->device, context->stagingBufferPool.memory, 0, totalBufferSize, 0, (void**)&data);
    memcpy(data, &mesh->indices[0], indexBufferSize);
    data += indexBufferSize;
    memcpy(data, &mesh->vertices[0], vertexBufferSize);
    vkUnmapMemory(context->device, context->stagingBufferPool.memory);

    VkCommandBuffer commandBuffer = vulkan::begin_command_buffer(context->device, context->commandPool);

    VkBufferCopy copyRegion = { 0, context->staticMeshPool.used, totalBufferSize };
    vkCmdCopyBuffer(commandBuffer, context->stagingBufferPool.buffer, context->staticMeshPool.buffer, 1, &copyRegion);

    vulkan::execute_command_buffer(context->device, context->commandPool, context->graphicsQueue, commandBuffer);

    VulkanMesh model = {};

    model.buffer = context->staticMeshPool.buffer;
    model.memory = context->staticMeshPool.memory;

    model.vertexOffset = context->staticMeshPool.used + vertexOffset;
    model.indexOffset = context->staticMeshPool.used + indexOffset;
    
    context->staticMeshPool.used += totalBufferSize;

    model.indexCount = mesh->indices.count();
    model.indexType = vulkan::convert_index_type(mesh->indexType);

    u32 modelIndex = context->loadedMeshes.size();

    context->loadedMeshes.push_back(model);

    MeshHandle resultHandle = { modelIndex };

    return resultHandle;
}

ModelHandle
vulkan::push_model (VulkanContext * context, MeshHandle mesh, MaterialHandle material)
{
    u32 objectIndex = context->loadedModels.size();

    VulkanModel object =
    {
        .mesh     = mesh,
        .material = material,
    };

    context->loadedModels.push_back(object);

    ModelHandle resultHandle = { objectIndex };
    return resultHandle;    
}

TextureHandle
vulkan::push_cubemap(VulkanContext * context, StaticArray<TextureAsset, 6> * assets)
{
    TextureHandle handle = { context->loadedTextures.size() };
    context->loadedTextures.push_back(vulkan::make_cubemap(context, assets));
    return handle;    
}


PipelineHandle
vulkan::push_pipeline(VulkanContext * context, const char * vertexShaderPath, const char * fragmentShaderPath, platform::RenderingOptions options)
{
    VulkanPipelineLoadInfo info = 
    {
        .vertexShaderPath   = vertexShaderPath,
        .fragmentShaderPath = fragmentShaderPath,
        .options            = options
    };
    auto pipeline = vulkan::make_pipeline(context, info);
    u64 index = context->loadedPipelines.size();
    context->loadedPipelines.push_back(pipeline);

    return {index};
}

void
vulkan::unload_scene(VulkanContext * context)
{
    vkDeviceWaitIdle(context->device);

    // Meshes
    context->staticMeshPool.used = 0;
    context->loadedMeshes.resize (0);

    // Textures
    for (s32 imageIndex = 0; imageIndex < context->loadedTextures.size(); ++imageIndex)
    {
        vulkan::destroy_texture(context, &context->loadedTextures[imageIndex]);
    }
    context->loadedTextures.resize(0);

    // Materials
    vkResetDescriptorPool(context->device, context->materialDescriptorPool, 0);   
    context->loadedMaterials.resize(0);

    // Rendered objects
    context->loadedModels.resize(0);

    destroy_loaded_pipelines(context);
    context->loadedPipelines.resize(0);

    context->sceneUnloaded = true;
}