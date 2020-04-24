 /*=============================================================================
Leo Tamminen

Implementation of Graphics interface for VulkanContext
=============================================================================*/
namespace vulkan_scene_internal_
{
    internal u32 compute_mip_levels (u32 texWidth, u32 texHeight);
    internal void cmd_generate_mip_maps(    VkCommandBuffer commandBuffer,
                                            VkPhysicalDevice physicalDevice,
                                            VkImage image,
                                            VkFormat imageFormat,
                                            u32 texWidth,
                                            u32 texHeight,
                                            u32 mipLevels,
                                            u32 layerCount = 1);
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
        .descriptorSet = make_material_vk_descriptor_set(context, asset->pipeline, asset->textures)
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
        .descriptorSet = make_material_vk_descriptor_set(   context,
                                                            &context->guiDrawPipeline,
                                                            get_loaded_texture(context, texture),
                                                            context->descriptorPools.material),
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

    VkCommandBuffer commandBuffer = begin_command_buffer(context->device, context->commandPool);

    VkBufferCopy copyRegion = { 0, context->staticMeshPool.used, totalBufferSize };
    vkCmdCopyBuffer(commandBuffer, context->stagingBufferPool.buffer, context->staticMeshPool.buffer, 1, &copyRegion);

    execute_command_buffer(commandBuffer,context->device, context->commandPool, context->queues.graphics);

    VulkanMesh model = {};

    model.bufferReference = context->staticMeshPool.buffer;
    // model.memoryReference = context->staticMeshPool.memory;

    model.vertexOffset = context->staticMeshPool.used + vertexOffset;
    model.indexOffset = context->staticMeshPool.used + indexOffset;
    
    context->staticMeshPool.used += totalBufferSize;

    model.indexCount = mesh->indices.count();
    model.indexType = convert_index_type(mesh->indexType);

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
    vkResetDescriptorPool(context->device, context->descriptorPools.material, 0);   
    context->loadedMaterials.resize(0);

    // Rendered objects
    context->loadedModels.resize(0);

    for (auto & pipeline : context->loadedPipelines)
    {
        destroy_loaded_pipeline(context, &pipeline);
    }
    context->loadedPipelines.resize(0);

    context->sceneUnloaded = true;
}

internal u32
vulkan_scene_internal_::compute_mip_levels(u32 texWidth, u32 texHeight)
{
   u32 result = std::floor(std::log2(math::max(texWidth, texHeight))) + 1;
   return result;
}

internal void
vulkan_scene_internal_::cmd_generate_mip_maps(  VkCommandBuffer commandBuffer,
                                                VkPhysicalDevice physicalDevice,
                                                VkImage image,
                                                VkFormat imageFormat,
                                                u32 texWidth,
                                                u32 texHeight,
                                                u32 mipLevels,
                                                u32 layerCount)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    // Note(Leo): Texture image format does not support blitting
    Assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);

    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;
    barrier.subresourceRange.levelCount = 1;

    int mipWidth = texWidth;
    int mipHeight = texHeight;

    // Todo(Leo): stupid loop with some stuff outside... fix pls
    for (int i = 1; i <mipLevels; ++i)
    {
        int newMipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
        int newMipHeight = mipHeight > 1 ? mipHeight / 2 : 1;

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit = {};
        
        blit.srcOffsets [0] = {0, 0, 0};
        blit.srcOffsets [1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layerCount; 

        blit.dstOffsets [0] = {0, 0, 0};
        blit.dstOffsets [1] = {newMipWidth, newMipHeight, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = layerCount;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit, VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier (commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
            1, &barrier);

        mipWidth = newMipWidth;
        mipHeight = newMipHeight;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
        1, &barrier);
}

internal VulkanTexture
vulkan::make_texture(VulkanContext * context, TextureAsset * asset)
{
    using namespace vulkan_scene_internal_;

    u32 width        = asset->width;
    u32 height       = asset->height;
    u32 mipLevels    = compute_mip_levels(width, height);

    VkDeviceSize imageSize = width * height * asset->channels;

    void * data;
    vkMapMemory(context->device, context->stagingBufferPool.memory, 0, imageSize, 0, &data);
    memcpy (data, (void*)asset->pixels.begin(), imageSize);
    vkUnmapMemory(context->device, context->stagingBufferPool.memory);

    VkImageCreateInfo imageInfo =
    {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags          = 0,
        .imageType      = VK_IMAGE_TYPE_2D,
        .format         = VK_FORMAT_R8G8B8A8_UNORM,
        .extent         = { width, height, 1 },
        .mipLevels      = mipLevels,
        .arrayLayers    = 1,

        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .tiling         = VK_IMAGE_TILING_OPTIMAL,
        .usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode    = VK_SHARING_MODE_EXCLUSIVE, // Note(Leo): this concerns queue families,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage resultImage = {};
    VkDeviceMemory resultImageMemory = {};

    VULKAN_CHECK(vkCreateImage(context->device, &imageInfo, nullptr, &resultImage));

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements (context->device, resultImage, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize     = memoryRequirements.size,
        .memoryTypeIndex    = vulkan::find_memory_type( context->physicalDevice,
                                                        memoryRequirements.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VULKAN_CHECK(vkAllocateMemory(context->device, &allocateInfo, nullptr, &resultImageMemory));
   

    vkBindImageMemory(context->device, resultImage, resultImageMemory, 0);   

    /* Note(Leo):
        1. change layout to copy optimal
        2. copy contents
        3. change layout to read optimal

        This last is good to use after generated textures, finally some 
        benefits from this verbose nonsense :D
    */

    VkCommandBuffer cmd = begin_command_buffer(context->device, context->commandPool);
    
    // Todo(Leo): begin and end command buffers once only and then just add commands from inside these
    cmd_transition_image_layout(cmd, context->device, context->queues.graphics,
                            resultImage, VK_FORMAT_R8G8B8A8_UNORM, mipLevels,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region =
    {
        .bufferOffset       = 0,
        .bufferRowLength    = 0,
        .bufferImageHeight  = 0,

        .imageSubresource = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel       = 0,
            .baseArrayLayer = 0,
            .layerCount     = 1,
        },

        // .imageSubresource.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
        // .imageSubresource.mipLevel          = 0,
        // .imageSubresource.baseArrayLayer    = 0,
        // .imageSubresource.layerCount        = 1,

        .imageOffset = { 0, 0, 0 },
        .imageExtent = { width, height, 1 },
    };
    vkCmdCopyBufferToImage (cmd, context->stagingBufferPool.buffer, resultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    cmd_generate_mip_maps(  cmd, context->physicalDevice,
                        resultImage, VK_FORMAT_R8G8B8A8_UNORM,
                        asset->width, asset->height, mipLevels);

    execute_command_buffer (cmd, context->device, context->commandPool, context->queues.graphics);

    VkImageViewCreateInfo imageViewInfo =
    { 
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = resultImage,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
        .format     = VK_FORMAT_R8G8B8A8_UNORM,

        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },

        .subresourceRange = {
            .aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel      = 0,
            .levelCount        = mipLevels,
            .baseArrayLayer    = 0,
            .layerCount        = 1,
        },

    };

    VkImageView resultView = {};
    VULKAN_CHECK(vkCreateImageView(context->device, &imageViewInfo, nullptr, &resultView));

    VulkanTexture resultTexture =
    {
        .image      = resultImage,
        .view       = resultView,
        .memory     = resultImageMemory,
    };
    return resultTexture;
}

internal VulkanTexture
vulkan::make_texture(VulkanContext * context, u32 width, u32 height, float4 color, VkFormat format)
{
    using namespace vulkan_scene_internal_;

    u32 mipLevels    = compute_mip_levels(width, height);

    u32 channels = 4;
    VkDeviceSize imageSize = width * height * channels;

    u32 pixelCount = width * height;
    u32 pixelValue = make_pixel(color);

    u32 * data;
    vkMapMemory(context->device, context->stagingBufferPool.memory, 0, imageSize, 0, (void**)&data);

    // Todo(Leo): use make_material_vk_descriptor_set
    for (u32 i = 0; i < pixelCount; ++i)
    {
        data[i] = pixelValue;
    }

    vkUnmapMemory(context->device, context->stagingBufferPool.memory);

    VkImageCreateInfo imageInfo =
    {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags          = 0,
        .imageType      = VK_IMAGE_TYPE_2D,
        .format         = format,
        .extent         = { width, height, 1 },
        .mipLevels      = mipLevels,
        .arrayLayers    = 1,

        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .tiling         = VK_IMAGE_TILING_OPTIMAL,
        .usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode    = VK_SHARING_MODE_EXCLUSIVE, // Note(Leo): this concerns queue families,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage resultImage = {};
    VkDeviceMemory resultImageMemory = {};

    VULKAN_CHECK(vkCreateImage(context->device, &imageInfo, nullptr, &resultImage));

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements (context->device, resultImage, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize     = memoryRequirements.size,
        .memoryTypeIndex    = vulkan::find_memory_type( context->physicalDevice,
                                                        memoryRequirements.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    VULKAN_CHECK(vkAllocateMemory(context->device, &allocateInfo, nullptr, &resultImageMemory));
   

    vkBindImageMemory(context->device, resultImage, resultImageMemory, 0);   

    /* Note(Leo):
        1. change layout to copy optimal
        2. copy contents
        3. change layout to read optimal

        This last is good to use after generated textures, finally some 
        benefits from this verbose nonsense :D
    */

    VkCommandBuffer cmd = begin_command_buffer(context->device, context->commandPool);
    
    // Todo(Leo): begin and end command buffers once only and then just add commands from inside these
    cmd_transition_image_layout(cmd, context->device, context->queues.graphics,
                            resultImage, format, mipLevels,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region =
    {
        .bufferOffset       = 0,
        .bufferRowLength    = 0,
        .bufferImageHeight  = 0,

        .imageSubresource = {
            .aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel          = 0,
            .baseArrayLayer    = 0,
            .layerCount        = 1,
        },

        .imageOffset = { 0, 0, 0 },
        .imageExtent = { width, height, 1 },
    };
    vkCmdCopyBufferToImage (cmd, context->stagingBufferPool.buffer, resultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


    /// CREATE MIP MAPS
    cmd_generate_mip_maps(  cmd, context->physicalDevice,
                            resultImage, format,
                            width, height, mipLevels);

    execute_command_buffer (cmd, context->device, context->commandPool, context->queues.graphics);

    /// CREATE IMAGE VIEW
    VkImageViewCreateInfo imageViewInfo =
    { 
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = resultImage,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
        .format     = format,

        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },

        .subresourceRange = {
            .aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel      = 0,
            .levelCount        = mipLevels,
            .baseArrayLayer    = 0,
            .layerCount        = 1,
        },
    };

    VkImageView resultView = {};
    VULKAN_CHECK(vkCreateImageView(context->device, &imageViewInfo, nullptr, &resultView));

    VulkanTexture resultTexture =
    {
        .image      = resultImage,
        .view       = resultView,
        .memory     = resultImageMemory,
    };
    return resultTexture;
}

internal VulkanTexture
make_shadow_texture(VulkanContext * context, u32 width, u32 height, VkFormat format)
{
    using namespace vulkan;
    using namespace vulkan_scene_internal_;

    u32 mipLevels   = 1;

    VkImageCreateInfo imageInfo =
    {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags          = 0,
        .imageType      = VK_IMAGE_TYPE_2D,
        .format         = format,
        .extent         = { width, height, 1 },
        .mipLevels      = mipLevels,
        .arrayLayers    = 1,

        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .tiling         = VK_IMAGE_TILING_OPTIMAL,
        .usage          = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode    = VK_SHARING_MODE_EXCLUSIVE, // Note(Leo): this concerns queue families,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    VkImage resultImage = {};
    VULKAN_CHECK(vkCreateImage(context->device, &imageInfo, nullptr, &resultImage));

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements (context->device, resultImage, &memoryRequirements);
    VkMemoryAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize     = memoryRequirements.size,
        .memoryTypeIndex    = vulkan::find_memory_type( context->physicalDevice,
                                                        memoryRequirements.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    VkDeviceMemory resultImageMemory = {};
    VULKAN_CHECK(vkAllocateMemory(context->device, &allocateInfo, nullptr, &resultImageMemory));

    vkBindImageMemory(context->device, resultImage, resultImageMemory, 0);   

    VkImageMemoryBarrier barrier =
    { 
        .sType                  = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

        .srcAccessMask          = 0,
        .dstAccessMask          = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,

        .oldLayout              = VK_IMAGE_LAYOUT_UNDEFINED, // Note(Leo): Can be VK_IMAGE_LAYOUT_UNDEFINED if we dont care??
        .newLayout              = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,

        .srcQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex    = VK_QUEUE_FAMILY_IGNORED,

        .image                  = resultImage,

        .subresourceRange = {
            .aspectMask        = VK_IMAGE_ASPECT_DEPTH_BIT, // ||  VK_IMAGE_ASPECT_STENCIL_BIT,
            .baseMipLevel      = 0,
            .levelCount        = mipLevels,
            .baseArrayLayer    = 0,
            .layerCount        = 1,
        }
    };

    VkCommandBuffer cmd = begin_command_buffer(context->device, context->commandPool);
    vkCmdPipelineBarrier(cmd,   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                0, 0, nullptr, 0, nullptr, 1, &barrier);
    execute_command_buffer (cmd, context->device, context->commandPool, context->queues.graphics);


    /// CREATE IMAGE VIEW
    VkImageViewCreateInfo imageViewInfo =
    { 
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = resultImage,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
        .format     = format,

        .subresourceRange = {
            .aspectMask        = VK_IMAGE_ASPECT_DEPTH_BIT,
            .baseMipLevel      = 0,
            .levelCount        = mipLevels,
            .baseArrayLayer    = 0,
            .layerCount        = 1,
        },

        // .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        // .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        // .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        // .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    VkImageView resultView = {};
    VULKAN_CHECK(vkCreateImageView(context->device, &imageViewInfo, nullptr, &resultView));

    VulkanTexture resultTexture =
    {
        .image      = resultImage,
        .view       = resultView,
        .memory     = resultImageMemory,
    };
    return resultTexture;
}

internal VulkanTexture
vulkan::make_cubemap(VulkanContext * context, StaticArray<TextureAsset, 6> * assets)
{
    using namespace vulkan_scene_internal_;

    u32 width        = (*assets)[0].width;
    u32 height       = (*assets)[0].height;
    u32 channels     = (*assets)[0].channels;
    u32 mipLevels    = compute_mip_levels(width, height);

    constexpr u32 CUBEMAP_LAYERS = 6;

    VkDeviceSize layerSize = width * height * channels;
    VkDeviceSize imageSize = layerSize * CUBEMAP_LAYERS;

    VkExtent3D extent = { width, height, 1};

    byte * data;
    vkMapMemory(context->device, context->stagingBufferPool.memory, 0, imageSize, 0, (void**)&data);
    for (int i = 0; i < 6; ++i)
    {
        u32 * start          = (*assets)[i].pixels.begin();
        u32 * end            = (*assets)[i].pixels.end();
        u32 * destination    = reinterpret_cast<u32*>(data + (i * layerSize));

        std::copy(start, end, destination);
    }
    vkUnmapMemory(context->device, context->stagingBufferPool.memory);

    VkImageCreateInfo imageInfo =
    {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags          = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        .imageType      = VK_IMAGE_TYPE_2D,
        .format         = VK_FORMAT_R8G8B8A8_UNORM,
        .extent         = extent,
        .mipLevels      = mipLevels,
        .arrayLayers    = CUBEMAP_LAYERS,

        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .tiling         = VK_IMAGE_TILING_OPTIMAL,
        .usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode    = VK_SHARING_MODE_EXCLUSIVE, // Note(Leo): this concerns queue families,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    VkImage resultImage = {};
    VkDeviceMemory resultImageMemory = {};

    // Note(Leo): some image formats may not be supported
    if (vkCreateImage(context->device, &imageInfo, nullptr, &resultImage) != VK_SUCCESS)
    {
        throw std::runtime_error ("Failed to create image");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements (context->device, resultImage, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize     = memoryRequirements.size,
        .memoryTypeIndex    = vulkan::find_memory_type( context->physicalDevice,
                                                        memoryRequirements.memoryTypeBits,
                                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };

    if (vkAllocateMemory(context->device, &allocateInfo, nullptr, &resultImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory");
    }

    vkBindImageMemory(context->device, resultImage, resultImageMemory, 0);   

    /* Note(Leo):
        1. change layout to copy optimal
        2. copy contents
        3. change layout to read optimal

        This last is good to use after generated textures, finally some 
        benefits from this verbose nonsense :D
    */

    VkCommandBuffer cmd = begin_command_buffer(context->device, context->commandPool);
    
    // Todo(Leo): begin and end command buffers once only and then just add commands from inside these
    cmd_transition_image_layout(cmd, context->device, context->queues.graphics,
                                resultImage, VK_FORMAT_R8G8B8A8_UNORM, mipLevels,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, CUBEMAP_LAYERS);

    VkBufferImageCopy region =
    {
        .bufferOffset       = 0,
        .bufferRowLength    = 0,
        .bufferImageHeight  = 0,

        .imageSubresource = {
            .aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel          = 0,
            .baseArrayLayer    = 0,
            .layerCount        = CUBEMAP_LAYERS,
        },

        .imageOffset = { 0, 0, 0 },
        .imageExtent = extent,
    };
    vkCmdCopyBufferToImage (cmd, context->stagingBufferPool.buffer, resultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


    /// CREATE MIP MAPS
    cmd_generate_mip_maps(  cmd, context->physicalDevice,
                            resultImage, VK_FORMAT_R8G8B8A8_UNORM,
                            width, height, mipLevels, CUBEMAP_LAYERS);

    execute_command_buffer(cmd, context->device, context->commandPool, context->queues.graphics);

    /// CREATE IMAGE VIEW
    VkImageViewCreateInfo imageViewInfo =
    { 
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = resultImage,
        .viewType   = VK_IMAGE_VIEW_TYPE_CUBE,
        .format     = VK_FORMAT_R8G8B8A8_UNORM,

        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY,
        },

        .subresourceRange = {
            .aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel      = 0,
            .levelCount        = mipLevels,
            .baseArrayLayer    = 0,
            .layerCount        = CUBEMAP_LAYERS,
        },

    };

    VkImageView resultView = {};
    if (vkCreateImageView(context->device, &imageViewInfo, nullptr, &resultView) != VK_SUCCESS)
    {
        throw std::runtime_error ("Failed to create image view");
    }

    VulkanTexture resultTexture =
    {
        .image      = resultImage,
        .view       = resultView,
        .memory     = resultImageMemory,
    };
    return resultTexture;
}