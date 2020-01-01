/*=============================================================================
Leo Tamminen

Implementation of IGraphicsContext interface for VulkanContext
=============================================================================*/
void
VulkanContext::Apply()
{
    vulkan::RefreshCommandBuffers (this);
}

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

    vulkan::EndOneTimeCommandBuffer(this->device, this->commandPool, this->graphicsQueue, commandBuffer);

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
    this->loadedTextures.push_back(vulkan::CreateImageTexture(texture, this));
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

GuiHandle
VulkanContext::PushGui(MeshHandle mesh, MaterialHandle material)
{
    uint32 guiIndex = loadedGuiObjects.size();

    VulkanRenderedObject object =
    {
        .mesh     = mesh,
        .material = material,
    };

    uint32 memorySizePerModelMatrix = align_up_to(
        this->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment,
        sizeof(Matrix44));
    object.uniformBufferOffset = guiIndex * memorySizePerModelMatrix;

    loadedGuiObjects.push_back(object);

    GuiHandle resultHandle = { guiIndex };
    return resultHandle;    
}

// Note(Leo): Destroy vulkan objects only but keep load infos.
internal void
destroy_loaded_pipelines(VulkanContext * context)
{
    for (int i = 0; i < context->loadedPipelines.size(); ++i)
    {
        vkDestroyPipeline(context->device, context->loadedPipelines[i].payload.pipeline, nullptr);
        vkDestroyPipelineLayout(context->device, context->loadedPipelines[i].payload.layout, nullptr);

        context->loadedPipelines[i].payload.layout = VK_NULL_HANDLE;
        context->loadedPipelines[i].payload.pipeline = VK_NULL_HANDLE;
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

    // Gui objects
    loadedGuiObjects.resize(0);

    destroy_loaded_pipelines(this);
    loadedPipelines.resize(0);
}

PipelineHandle
VulkanContext::push_pipeline(const char * vertexShaderPath, const char * fragmentShaderPath, platform::PipelineOptions options)
{
    std::cout << "[push_shader()]: TODO these must be recreated with swapchain.\n";

    auto pipeline = vulkan::make_pipeline(this, 3, vertexShaderPath, fragmentShaderPath, options);
    uint64 index = loadedPipelines.size();
    loadedPipelines.push_back({{vertexShaderPath, fragmentShaderPath, options}, pipeline});

    return {index};
}

internal void
recreate_loaded_pipelines(VulkanContext * context)
{
    destroy_loaded_pipelines(context);

    for (int i = 0; i < context->loadedPipelines.size(); ++i)
    {
        auto pipeline = vulkan::make_pipeline(
                            context,
                            3, 
                            context->loadedPipelines[i].info.vertexShaderPath.c_str(),
                            context->loadedPipelines[i].info.fragmentShaderPath.c_str(),
                            context->loadedPipelines[i].info.options);
 
        context->loadedPipelines[i].payload = pipeline;
    }
}