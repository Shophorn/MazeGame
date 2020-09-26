/*=============================================================================
Leo Tamminen

Implementation of Graphics interface for VulkanContext
=============================================================================*/

namespace fsvulkan_resources_internal_
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

internal VkDescriptorSet fsvulkan_make_texture_descriptor_set(  VulkanContext *         context,
																VkDescriptorSetLayout   descriptorSetLayout,
																VkDescriptorPool 		descriptorPool,
																s32                     textureCount,
																VkImageView *         	imageViews,
																VkSampler * 			samplers)
{
	constexpr u32 maxTextures = 10;
	AssertMsg(textureCount < maxTextures, "Too many textures on material");

	Assert(textureCount == 0 || imageViews != nullptr);

	// Todo(Leo): this function is worse than it should, since we implicitly expect a valid descriptorsetlayout
	Assert(descriptorSetLayout != VK_NULL_HANDLE);

	VkDescriptorSetAllocateInfo allocateInfo =
	{ 
		.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool     = descriptorPool,
		.descriptorSetCount = 1,
		.pSetLayouts        = &descriptorSetLayout
	};

	VkDescriptorSet resultSet;
	VULKAN_CHECK(vkAllocateDescriptorSets(context->device, &allocateInfo, &resultSet));

	if (textureCount > 0)
	{
		VkDescriptorImageInfo samplerInfos [maxTextures];
		for (s32 i = 0; i < textureCount; ++i)
		{
			samplerInfos[i] = 
			{
				.sampler        = samplers[i],
				.imageView      = imageViews[i],
				.imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};
		}

		VkWriteDescriptorSet writing =
		{  
			.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet             = resultSet,
			.dstBinding         = 0,
			.dstArrayElement    = 0,
			.descriptorCount    = (u32)textureCount,
			.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo         = samplerInfos,
		};

		// Note(Leo): Two first are write info, two latter are copy info
		vkUpdateDescriptorSets(context->device, 1, &writing, 0, nullptr);
	}

	return resultSet;
}

internal VulkanTexture BAD_VULKAN_make_texture(VulkanContext * context, TextureAssetData * asset)
{
	using namespace fsvulkan_resources_internal_;

	u32 width        = asset->width;
	u32 height       = asset->height;
	u32 mipLevels    = compute_mip_levels(width, height);

	VkDeviceSize textureMemorySize = get_texture_asset_memory_size(*asset);

	Assert(textureMemorySize <= context->stagingBufferCapacity);
	copy_memory(context->persistentMappedStagingBufferMemory, asset->pixelMemory, textureMemorySize);

	constexpr VkFormat formatTable [] =
	{
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_FORMAT_R32G32B32A32_SFLOAT,
	};

	VkFormat format = formatTable[asset->format];

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

	VkCommandBuffer cmd = vulkan::begin_command_buffer(context->device, context->commandPool);
	
	// Todo(Leo): begin and end command buffers once only and then just add commands from inside these
	vulkan::cmd_transition_image_layout(cmd, context->device, context->graphicsQueue, resultImage, format, mipLevels,
										VK_IMAGE_LAYOUT_UNDEFINED,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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
	vkCmdCopyBufferToImage (cmd, context->stagingBuffer, resultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	cmd_generate_mip_maps(cmd, context->physicalDevice, resultImage, format, asset->width, asset->height, mipLevels);

	vulkan::execute_command_buffer (cmd, context->device, context->commandPool, context->graphicsQueue);

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
		.sampler 	= asset->addressMode == TEXTURE_ADDRESS_MODE_REPEAT ? context->textureSampler : context->clampSampler,

		.format 	= format,

		.memory     = resultImageMemory,

		.width 		= width,
		.height 	= height,
		.mipLevels 	= mipLevels,
	};
	return resultTexture;
}

internal VkIndexType
BAD_VULKAN_convert_index_type(IndexType type)
{
	switch (type)
	{
		case IndexType::UInt16:
			return VK_INDEX_TYPE_UINT16;

		case IndexType::UInt32:
			return VK_INDEX_TYPE_UINT32;

		default:
			return VK_INDEX_TYPE_NONE_NV;
	};
}

internal TextureHandle graphics_memory_push_texture(VulkanContext * context, TextureAssetData * texture)
{
	TextureHandle handle = { (s64)context->loadedTextures.size() };
	context->loadedTextures.push_back(BAD_VULKAN_make_texture(context, texture));
	return handle;
}

internal GuiTextureHandle graphics_memory_push_gui_texture(VulkanContext * context, TextureAssetData * asset)
{
	VulkanTexture texture 			= BAD_VULKAN_make_texture(context, asset);
	VkDescriptorSet descriptorSet 	= fsvulkan_make_texture_descriptor_set(	context,	
																			context->pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].descriptorSetLayout,
																			context->materialDescriptorPool,
																			1, &texture.view, &context->clampSampler);
	s64 index = context->loadedGuiTextures.size();
	context->loadedGuiTextures.push_back({texture, descriptorSet});

	return {index};
}

internal MaterialHandle graphics_memory_push_material (	VulkanContext *     context,
															GraphicsPipeline    pipeline,
															s32                 textureCount,
															TextureHandle *     textures)
{

	Assert(textureCount == context->pipelines[pipeline].textureCount);
	Assert(textureCount <= 10);

	VkImageView textureImageViews [10];
	VkSampler 	samplers [10];
	for (s32 i = 0; i < textureCount; ++i)
	{
		textureImageViews[i] = fsvulkan_get_loaded_texture(context, textures[i])->view;
		samplers[i] = fsvulkan_get_loaded_texture(context, textures[i])->sampler;
	}

	VkDescriptorSet descriptorSet = fsvulkan_make_texture_descriptor_set(   context,
																			context->pipelines[pipeline].descriptorSetLayout,
																			context->materialDescriptorPool,
																			textureCount,
																			textureImageViews,
																			samplers);

	s64 index = (s64)context->loadedMaterials.size();
	context->loadedMaterials.push_back({pipeline, descriptorSet});


	if (pipeline == GRAPHICS_PIPELINE_SKYBOX)
	{
		context->skyGradientDescriptorSet = descriptorSet;
	}

	return {index};
}

internal MeshHandle graphics_memory_push_mesh(VulkanContext * context, MeshAssetData * mesh)
{
	// Todo(Leo): this is messy af
	// at least map staging buffer persitently


	u64 indexBufferSize  = mesh->indexCount * sizeof(mesh->indices[0]);
	u64 vertexBufferSize = mesh->vertexCount * sizeof(mesh->vertices[0]);
	// u64 vertexBufferSize = mesh->vertices.count() * sizeof(mesh->vertices[0]);

	// Todo(Leo): Currently we align on 4 on both indices and vertices, indices maybe wouldn't need to,
	// and vertices maybe have something else sometimes.
	u64 alignedIndexBufferSize = align_up(indexBufferSize, 4);
	u64 alignedVertexBufferSize = align_up(vertexBufferSize, 4);

	u64 totalBufferSize = alignedIndexBufferSize + alignedVertexBufferSize;

	u64 indexOffset = 0;
	u64 vertexOffset = alignedIndexBufferSize;

	Assert(totalBufferSize <= context->stagingBufferCapacity);
	copy_memory(context->persistentMappedStagingBufferMemory, mesh->indices, indexBufferSize);
	copy_memory(context->persistentMappedStagingBufferMemory + vertexOffset, mesh->vertices, vertexBufferSize);

	VkCommandBuffer commandBuffer = vulkan::begin_command_buffer(context->device, context->commandPool);

	VkBufferCopy copyRegion = { 0, context->staticMeshPool.used, totalBufferSize };
	vkCmdCopyBuffer(commandBuffer, context->stagingBuffer, context->staticMeshPool.buffer, 1, &copyRegion);

	vulkan::execute_command_buffer(commandBuffer,context->device, context->commandPool, context->graphicsQueue);

	VulkanMesh model = {};

	model.bufferReference = context->staticMeshPool.buffer;
	// model.memoryReference = context->staticMeshPool.memory;

	model.vertexOffset = context->staticMeshPool.used + vertexOffset;
	model.indexOffset = context->staticMeshPool.used + indexOffset;
	
	context->staticMeshPool.used += totalBufferSize;

	model.indexCount = mesh->indexCount;
	model.indexType = BAD_VULKAN_convert_index_type(mesh->indexType);

	u32 modelIndex = context->loadedMeshes.size();

	context->loadedMeshes.push_back(model);

	MeshHandle resultHandle = { modelIndex };

	return resultHandle;
}

internal ModelHandle graphics_memory_push_model (VulkanContext * context, MeshHandle mesh, MaterialHandle material)
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


internal void graphics_memory_unload(VulkanContext * context)
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

	for(VulkanGuiTexture & gui : context->loadedGuiTextures)
	{
		vulkan::destroy_texture(context, &gui.texture);
	}
	context->loadedGuiTextures.resize(0);

	// Materials
	vkResetDescriptorPool(context->device, context->materialDescriptorPool, 0);   
	context->loadedMaterials.resize(0);

	// Rendered objects
	context->loadedModels.resize(0);




	context->sceneUnloaded = true;
}

internal u32
fsvulkan_resources_internal_::compute_mip_levels(u32 texWidth, u32 texHeight)
{
   u32 result = floor_f32(std::log2(max_f32(texWidth, texHeight))) + 1;
   return result;
}

internal void
fsvulkan_resources_internal_::cmd_generate_mip_maps(  VkCommandBuffer commandBuffer,
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
make_shadow_texture(VulkanContext * context, u32 width, u32 height, VkFormat format)
{
	using namespace vulkan;
	using namespace fsvulkan_resources_internal_;

	u32 mipLevels   = 1;

	VkImageCreateInfo imageInfo =
	{
		.sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags          = 0,
		.imageType      = VK_IMAGE_TYPE_2D,
		.format         = format,
		.extent         = { width, height, 1 },
		.mipLevels      = 1,
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
	execute_command_buffer (cmd, context->device, context->commandPool, context->graphicsQueue);


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

internal void graphics_development_update_texture(VulkanContext * context, TextureHandle textureHandle, TextureAssetData * asset)
{
	using namespace fsvulkan_resources_internal_;


	VulkanTexture & texture = context->loadedTextures[textureHandle];

	Assert(asset->width == texture.width);
	Assert(asset->height == texture.height);
	// Todo(Leo): Also assert format and miplevels

	VkDeviceSize textureMemorySize = get_texture_asset_memory_size(*asset);

	Assert(textureMemorySize <= context->stagingBufferCapacity);
	copy_memory(context->persistentMappedStagingBufferMemory, asset->pixelMemory, textureMemorySize);

	VkCommandBuffer cmd 	= vulkan::begin_command_buffer(context->device, context->commandPool);
	
	// Todo(Leo): graphics queue may be in use, so this may fail...???
	vulkan::cmd_transition_image_layout(cmd, context->device,
										context->graphicsQueue, texture.image, texture.format, texture.mipLevels,
										VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

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
		.imageExtent = { texture.width, texture.height, 1 },
	};
	vkCmdCopyBufferToImage (cmd, context->stagingBuffer, texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	// Todo(Leo): This probably implicitly changes layout to shader read optimal or something, change
	cmd_generate_mip_maps(cmd, context->physicalDevice, texture.image, texture.format, texture.width, texture.height, texture.mipLevels);

	vulkan::execute_command_buffer (cmd, context->device, context->commandPool, context->graphicsQueue);
}