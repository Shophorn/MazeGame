/*=============================================================================
Leo Tamminen
shophorn @ internet

Vulkan drawing functions' definitions.
=============================================================================*/
namespace fsvulkan_drawing_internal_
{
	/* Note(Leo): This is EXPERIMENTAL. Idea is that we don't need to always
	create a new struct, but we can just reuse same. Vulkan functions always want
	a pointer to struct, so we cannot use a return value from a function, but this
	way we always have that struct here ready for us and we modify it accordingly
	with each call and return pointer to it. We can do this, since each thread 
	only has one usage of this at any time.

	Returned pointer is only valid until this is called again. Or rather, pointer
	will stay valid for the duration of thread, but values of struct will not. */

	// TODO(Leo): probably remove....
	thread_local VkCommandBufferInheritanceInfo global_inheritanceInfo_ = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO };
	VkCommandBufferInheritanceInfo const *

	get_vk_command_buffer_inheritance_info(VkRenderPass renderPass, VkFramebuffer framebuffer)
	{
		global_inheritanceInfo_.renderPass = renderPass;
		global_inheritanceInfo_.framebuffer = framebuffer;

		return &global_inheritanceInfo_;
	}
}

internal VkDeviceSize
fsvulkan_get_aligned_uniform_buffer_size(VulkanContext * context, VkDeviceSize size)
{
	auto alignment = context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	return align_up(size, alignment);
} 


internal VkDeviceSize fsvulkan_get_uniform_memory(VulkanContext & context, VkDeviceSize size)
{
	// Note(Leo): offset data for different frames, so we do not overwrite previous frames' data before they are done.
	// Todo(Leo): make separate buffers for each frame
	VkDeviceSize frameSize = fsvulkan_get_aligned_uniform_buffer_size(&context, megabytes(150));
	VkDeviceSize frameOffset = frameSize * context.virtualFrameIndex;

	auto alignment 	= context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	size 			= align_up(size, alignment);

	VkDeviceSize currentOffset = context.modelUniformBuffer.used;

	Assert(currentOffset + size <= frameSize && "Trying to use too much memory.");

	context.modelUniformBuffer.used += size;

	return currentOffset + frameOffset;
};

internal void fsvulkan_reset_uniform_buffer(VulkanContext & context)
{
	/// TODO(Leo): This is used incorrectly. We should zero some other value, since this buffer is in 
	// practice split to three or two parts, but currently we use this same 'used' value to track each.
	// It luckily works because they are not accessed at same times.
	context.modelUniformBuffer.used = 0;
}

internal void fsvulkan_drawing_update_camera(VulkanContext * context, Camera const * camera)
{
	FSVulkanCameraUniformBuffer * pUbo = context->persistentMappedCameraUniformBufferMemory[context->virtualFrameIndex];

	pUbo->view 			= camera_view_matrix(camera->position, camera->direction);
	pUbo->projection 	= camera_projection_matrix(camera);
}

internal void fsvulkan_drawing_update_lighting(VulkanContext * context, Light const * light, Camera const * camera, v3 ambient)
{
	FSVulkanLightingUniformBuffer * lightPtr = context->persistentMappedLightingUniformBufferMemory[context->virtualFrameIndex];

	lightPtr->direction    		= v3_to_v4(light->direction, 0);
	lightPtr->color        		= v3_to_v4(light->color, 0);
	lightPtr->ambient      		= v3_to_v4(ambient, 0);
	lightPtr->cameraPosition 	= v3_to_v4(camera->position, 1);
	lightPtr->skyColor 			= light->skyColorSelection;

	// ------------------------------------------------------------------------------

	m44 lightViewProjection = get_light_view_projection (light, camera);

	FSVulkanCameraUniformBuffer * pUbo = context->persistentMappedCameraUniformBufferMemory[context->virtualFrameIndex];

	pUbo->lightViewProjection 		= lightViewProjection;
	pUbo->shadowDistance 			= light->shadowDistance;
	pUbo->shadowTransitionDistance 	= light->shadowTransitionDistance;
}

internal void fsvulkan_drawing_update_hdr_settings(VulkanContext * context, HdrSettings const * hdrSettings)
{
	context->hdrSettings.exposure = hdrSettings->exposure;
	context->hdrSettings.contrast = hdrSettings->contrast;
}

internal void fsvulkan_drawing_prepare_frame(VulkanContext * context)
{
	AssertMsg((context->canDraw == false), "Invalid call to prepare_drawing() when finish_drawing() has not been called.")
	context->canDraw = true;

	// context->modelUniformBuffer.used = 0;
	fsvulkan_reset_uniform_buffer(*context);
	context->leafBufferUsed[context->virtualFrameIndex] = 0;

	auto * frame = fsvulkan_get_current_virtual_frame(context);

	// Note(Leo): We recreate these everytime we are here.
	// https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-4#inpage-nav-5
	vkDestroyFramebuffer(context->device, frame->framebuffer, nullptr);

	// Todo(Leo): We should not recreate this every frame. It was done to easily adjust to
	// screen size changes, but that is not something we expect player to do in the final product
	VkImageView attachments [] =
	{
		frame->colorImageView,
		frame->depthImageView,
		frame->resolveImageView,
	};

	// TODO HDR: use floating point frame buffer here
	frame->framebuffer = vulkan::make_vk_framebuffer(   context->device,
														context->renderPass,
														array_count(attachments),
														attachments,
														context->swapchainExtent.width,
														context->swapchainExtent.height);    

	VkCommandBufferBeginInfo sceneCmdBeginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,
		.pInheritanceInfo = fsvulkan_drawing_internal_::get_vk_command_buffer_inheritance_info(context->renderPass, frame->framebuffer),
	};
	VULKAN_CHECK (vkBeginCommandBuffer(frame->commandBuffers.scene, &sceneCmdBeginInfo));

	VkViewport viewport =
	{
		.x          = 0.0f,
		.y          = 0.0f,
		.width      = (float) context->swapchainExtent.width,
		.height     = (float) context->swapchainExtent.height,
		.minDepth   = 0.0f,
		.maxDepth   = 1.0f,
	};
	vkCmdSetViewport(frame->commandBuffers.scene, 0, 1, &viewport);

	VkRect2D scissor =
	{
		.offset = {0, 0},
		.extent = context->swapchainExtent,
	};
	vkCmdSetScissor(frame->commandBuffers.scene, 0, 1, &scissor);

	// -----------------------------------------------------

	VkCommandBufferBeginInfo shadowCmdBeginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT,

		// Note(Leo): the pointer in this is actually same as above, but this function call changes values of struct at the other end of pointer.
		// Todo(Leo): shadow framebuffer needs to be separate per virtual frame
		.pInheritanceInfo = fsvulkan_drawing_internal_::get_vk_command_buffer_inheritance_info(context->shadowRenderPass, context->shadowFramebuffer[context->virtualFrameIndex]),
	};
	VULKAN_CHECK(vkBeginCommandBuffer(frame->commandBuffers.offscreen, &shadowCmdBeginInfo));

	// Todo(Leo): this comment is not true anymore, but deductions from it have not been corrected
	// Note(Leo): We only ever use this one pipeline with shadows, so it is sufficient to only bind it once, here.
	vkCmdBindPipeline(frame->commandBuffers.offscreen, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowPipeline);
}

internal void fsvulkan_drawing_finish_frame(VulkanContext * context)
{
	Assert(context->canDraw);
	context->canDraw = false;

	VulkanVirtualFrame * frame = fsvulkan_get_current_virtual_frame(context);


	VkCommandBufferBeginInfo masterCmdBeginInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.flags              = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo   = nullptr,
	};

	/* Note (Leo): beginning command buffer implicitly resets it, if we have specified
	VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT in command pool creation */
	VULKAN_CHECK(vkBeginCommandBuffer(frame->commandBuffers.master, &masterCmdBeginInfo));
	

	// SHADOWS RENDER PASS ----------------------------------------------------
	VULKAN_CHECK(vkEndCommandBuffer(frame->commandBuffers.offscreen));

	VkClearValue shadowClearValue = { .depthStencil = {1.0f, 0} };
	VkRenderPassBeginInfo shadowRenderPassInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass         = context->shadowRenderPass,
		.framebuffer        = context->shadowFramebuffer[context->virtualFrameIndex],
		
		.renderArea = {
			.offset = {0,0},
			.extent  = {context->shadowTextureWidth, context->shadowTextureHeight},
		},

		.clearValueCount    = 1,
		.pClearValues       = &shadowClearValue,
	};
	vkCmdBeginRenderPass(frame->commandBuffers.master, &shadowRenderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	vkCmdExecuteCommands(frame->commandBuffers.master, 1, &frame->commandBuffers.offscreen);
	vkCmdEndRenderPass(frame->commandBuffers.master);

	// MAIN SCENE RENDER PASS -------------------------------------------------
	// Todo(Leo): add separate pass for gui and debug stuffs
	VULKAN_CHECK(vkEndCommandBuffer(frame->commandBuffers.scene));

	VkClearValue clearValues [] =
	{
		{ .color = {0.35f, 0.0f, 0.35f, 1.0f} },
		{ .depthStencil = {1.0f, 0} }
	};

	VkRenderPassBeginInfo renderPassInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass         = context->renderPass,
		.framebuffer        = frame->framebuffer,
		
		.renderArea = {
			.offset  = {0, 0},
			.extent  = context->swapchainExtent,
		},
		
		.clearValueCount    = 2,
		.pClearValues       = clearValues,
	};
	vkCmdBeginRenderPass(frame->commandBuffers.master, &renderPassInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
	vkCmdExecuteCommands(frame->commandBuffers.master, 1, &frame->commandBuffers.scene);
	vkCmdEndRenderPass(frame->commandBuffers.master);

	/// HDR RENDER PASS --------------------------------------------------------------------
	/*	
		Do all these in master command buffer
		1. set viewport and scissor sizes
		2. create extra frame buffer for this pass only
		3. start render pass
		4. add commands for pass through shader
			-> bind passthroug pipeline
			-> bind resolveimage as texture
			-> draw 3 vertices, no buffers
		5. end renderpass
	*/

	VkViewport viewport =
	{
		.x          = 0.0f,
		.y          = 0.0f,
		.width      = (float) context->swapchainExtent.width,
		.height     = (float) context->swapchainExtent.height,
		.minDepth   = 0.0f,
		.maxDepth   = 1.0f,
	};
	vkCmdSetViewport(frame->commandBuffers.master, 0, 1, &viewport);

	VkRect2D scissor =
	{
		.offset = {0, 0},
		.extent = context->swapchainExtent,
	};
	vkCmdSetScissor(frame->commandBuffers.master, 0, 1, &scissor);

	local_persist s32 debugged = 0;
	if (context->virtualFrameIndex == 0 && debugged < 2)
	{
		debugged += 1;
		logDebug(0) << "hdr framebuffer: " << frame->passThroughFramebuffer << ", " << (frame->passThroughFramebuffer == VK_NULL_HANDLE);
	}

	vkDestroyFramebuffer(context->device, frame->passThroughFramebuffer, nullptr);
	frame->passThroughFramebuffer = vulkan::make_vk_framebuffer(context->device,
																context->passThroughRenderPass,
																1,
																&context->swapchainImageViews[context->currentDrawFrameIndex],
																context->swapchainExtent.width,
																context->swapchainExtent.height);	

	VkRenderPassBeginInfo passthroughPassBeginInfo 	= {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	passthroughPassBeginInfo.renderPass 			= context->passThroughRenderPass;
	passthroughPassBeginInfo.framebuffer 			= frame->passThroughFramebuffer;
	passthroughPassBeginInfo.renderArea 			= {{0,0}, context->swapchainExtent};
	// Note(Leo): no need to clear this, we fill every pixel anyway


	vkCmdBeginRenderPass(frame->commandBuffers.master, &passthroughPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(frame->commandBuffers.master, VK_PIPELINE_BIND_POINT_GRAPHICS, context->passThroughPipeline);

	VkDescriptorSet testDescriptor = frame->resolveImageDescriptor;
	vkCmdBindDescriptorSets(frame->commandBuffers.master, VK_PIPELINE_BIND_POINT_GRAPHICS, 
							context->passThroughPipelineLayout,
							0, 1, &testDescriptor, 0, nullptr);

	vkCmdPushConstants(frame->commandBuffers.master, context->passThroughPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FSVulkanHdrSettings), &context->hdrSettings);

	vkCmdDraw(frame->commandBuffers.master, 3, 1, 0, 0);

	vkCmdEndRenderPass(frame->commandBuffers.master);

	// PRIMARY COMMAND BUFFER -------------------------------------------------

	VULKAN_CHECK(vkEndCommandBuffer(frame->commandBuffers.master));

	/* Note(Leo): these have to do with sceneloading in game layer. We are then unloading
	all resources associated with command buffer, which makes it invalid to submit to queue */
	// Todo(Leo): this should be removed, just unload stuff after the frame is done
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
		.pCommandBuffers        = &frame->commandBuffers.master,
		.signalSemaphoreCount   = 1,
		.pSignalSemaphores      = &frame->renderFinishedSemaphore,
	};
	vkResetFences(context->device, 1, &frame->queueSubmitFence);
	VULKAN_CHECK(vkQueueSubmit(context->graphicsQueue, 1, &submitInfo, frame->queueSubmitFence));

	VkPresentInfoKHR presentInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores    = &frame->renderFinishedSemaphore,
		.swapchainCount     = 1,
		.pSwapchains        = &context->swapchain,
		.pImageIndices      = &context->currentDrawFrameIndex,
		.pResults           = nullptr,
	};

	// Todo(Leo): Should we do something with this? We are handling VK_ERROR_OUT_OF_DATE_KHR elsewhere anyway.
	VkResult result = vkQueuePresentKHR(context->presentQueue, &presentInfo);
	if (result != VK_SUCCESS)
	{
		logVulkan() << FILE_ADDRESS << "Present result = " << vulkan::to_str(result);
	}

	/// ADVANCE VIRTUAL FRAME INDEX
	{
		context->virtualFrameIndex += 1;
		context->virtualFrameIndex %= VIRTUAL_FRAME_COUNT;
	}

	/* Todo(Leo): there are limited number of these they should be listed explicitly,
	and especially not called from here, but in this functions call site */
	if (context->onPostRender != nullptr)
	{
		vkDeviceWaitIdle(context->device);
		context->onPostRender(context);
		context->onPostRender = nullptr;
	}
}

internal void fsvulkan_drawing_draw_model(VulkanContext * context, ModelHandle model, m44 transform, bool32 castShadow, m44 const * bones, u32 bonesCount)
{
	Assert(context->canDraw);

	/* Todo(Leo): Get rid of these, we can just as well get them directly from user.
	That is more flexible and then we don't need to save that data in multiple places. */
	MeshHandle meshHandle           = context->loadedModels[model].mesh;
	MaterialHandle materialHandle   = context->loadedModels[model].material;

	u32 uniformBufferSize   = sizeof(FSVulkanModelUniformBuffer);

	VkDeviceSize uniformBufferOffset = fsvulkan_get_uniform_memory(*context, sizeof(FSVulkanModelUniformBuffer));
	FSVulkanModelUniformBuffer * pBuffer;

	// Optimize(Leo): Just map this once when rendering starts
	vkMapMemory(context->device, context->modelUniformBuffer.memory, uniformBufferOffset, uniformBufferSize, 0, (void**)&pBuffer);
	
	pBuffer->localToWorld = transform;
	pBuffer->isAnimated = bonesCount;

	Assert(bonesCount <= array_count(pBuffer->bonesToLocal));    
	copy_memory(pBuffer->bonesToLocal, bones, sizeof(m44) * bonesCount);

	vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

	// ---------------------------------------------------------------

	VulkanVirtualFrame * frame = fsvulkan_get_current_virtual_frame(context);
 
	auto * mesh     = fsvulkan_get_loaded_mesh(context, meshHandle);
	auto * material = fsvulkan_get_loaded_material(context, materialHandle);

	Assert(material->pipeline < GRAPHICS_PIPELINE_SCREEN_GUI && "This pipeline does not support mesh rendering");

	VkPipeline pipeline = context->pipelines[material->pipeline].pipeline;
	VkPipelineLayout pipelineLayout = context->pipelines[material->pipeline].pipelineLayout;

	// Material type
	vkCmdBindPipeline(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	
	VkDescriptorSet sets [] =
	{   
		context->cameraDescriptorSet[context->virtualFrameIndex],
		material->descriptorSet,
		context->modelDescriptorSet,
		context->lightingDescriptorSet[context->virtualFrameIndex],
		context->shadowMapTextureDescriptorSet[context->virtualFrameIndex],
		context->skyGradientDescriptorSet,
	};

	// Note(Leo): this works, because models are only dynamic set
	Assert(uniformBufferOffset <= max_value_u32);
	u32 dynamicOffsets [] = {(u32)uniformBufferOffset};

	vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, array_count(sets), sets,
							array_count(dynamicOffsets), dynamicOffsets);


	vkCmdBindVertexBuffers(frame->commandBuffers.scene, 0, 1, &mesh->bufferReference, &mesh->vertexOffset);
	vkCmdBindIndexBuffer(frame->commandBuffers.scene, mesh->bufferReference, mesh->indexOffset, mesh->indexType);

	Assert(mesh->indexCount > 0);

	vkCmdDrawIndexed(frame->commandBuffers.scene, mesh->indexCount, 1, 0, 0, 0);

	// ------------------------------------------------
	///////////////////////////
	///   SHADOW PASS       ///
	///////////////////////////
	if (castShadow)
	{
		vkCmdBindVertexBuffers(frame->commandBuffers.offscreen, 0, 1, &mesh->bufferReference, &mesh->vertexOffset);
		vkCmdBindIndexBuffer(frame->commandBuffers.offscreen, mesh->bufferReference, mesh->indexOffset, mesh->indexType);

		VkDescriptorSet shadowSets [] =
		{
			context->cameraDescriptorSet[context->virtualFrameIndex],
			context->modelDescriptorSet,
		};

		vkCmdBindDescriptorSets(frame->commandBuffers.offscreen,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								context->shadowPipelineLayout,
								0,
								2,
								shadowSets,
								1,
								&dynamicOffsets[0]);

		vkCmdDrawIndexed(frame->commandBuffers.offscreen, mesh->indexCount, 1, 0, 0, 0);
	}
}

// internal void * get_leaf_buffer_memory()
// {
// 	return context->persistentMappedLeafBufferMemory;
// }

// internal void fsvulkan_drawing_draw_leaves(VulkanContext * context, s64 count, s64 bufferOffset)
// {
// 	// bind camera
// 	// bind material
// 	// bind shadows
// 	// bind lights
// 	// bind etc.

// 	vkCmdBindVertexBuffers(commandBuffer, 0, 1, context->leafBuffer, bufferOffset);
// 	vkCmdDraw(commandBuffer, 4, count, 0, 0);
// }

internal void fsvulkan_drawing_draw_leaves(	VulkanContext * context,
											s32 instanceCount,
											m44 const * instanceTransforms,
											s32 colourIndex,
											MaterialHandle materialHandle)
{
	Assert(instanceCount > 0 && "Vulkan cannot map memory of size 0, and this function should no be called for 0 meshes");
	Assert(context->canDraw);
	// Note(Leo): lets keep this sensible
	Assert(instanceCount <= 20000);

	VulkanVirtualFrame * frame 	= fsvulkan_get_current_virtual_frame(context);
	s64 frameOffset 			= context->leafBufferCapacity * context->virtualFrameIndex;
	u64 instanceBufferOffset 	= frameOffset + context->leafBufferUsed[context->virtualFrameIndex];

	u64 instanceBufferSize 		= instanceCount * sizeof(m44);
	context->leafBufferUsed[context->virtualFrameIndex] += instanceBufferSize;

	Assert(context->leafBufferUsed[context->virtualFrameIndex] <= context->leafBufferCapacity);

	void * bufferPointer = (u8*)context->persistentMappedLeafBufferMemory + instanceBufferOffset;
	copy_memory(bufferPointer, instanceTransforms, instanceCount * sizeof(m44));

	VkPipeline pipeline 			= context->pipelines[GRAPHICS_PIPELINE_LEAVES].pipeline;
	VkPipelineLayout pipelineLayout = context->pipelines[GRAPHICS_PIPELINE_LEAVES].pipelineLayout;
	
	vkCmdBindPipeline(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// Note(Leo): These todos are not so relevant anymore, I decided to make a single collective rendering function, that
	// is not so much separated from graphics api layer, so while points remain valid, the situation has changed.
	// Todo(Leo): These can be bound once per frame, they do not change
	// Todo(Leo): Except that they need to be bound per pipeline, maybe unless selected binding etc. are identical
	vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, 1, &context->cameraDescriptorSet[context->virtualFrameIndex],
							0, nullptr);

	vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							1, 1, &context->lightingDescriptorSet[context->virtualFrameIndex],
							0, nullptr);

	vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							2, 1, &context->shadowMapTextureDescriptorSet[context->virtualFrameIndex],
							0, nullptr);

	VkDescriptorSet materialDescriptorSet = fsvulkan_get_loaded_material(context, materialHandle)->descriptorSet;
	vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1, &materialDescriptorSet, 0, nullptr);

	vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							4, 1, &context->skyGradientDescriptorSet,
							0, nullptr);

	vkCmdPushConstants(frame->commandBuffers.scene, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s32), &colourIndex);

	vkCmdBindVertexBuffers(frame->commandBuffers.scene, 0, 1, &context->leafBuffer, &instanceBufferOffset);

	// Note(Leo): Fukin cool, instantiation at last :DDDD 4.6.
	// Note(Leo): This is hardcoded in shaders, we generate vertex data on demand
	constexpr s32 leafVertexCount = 4;
	vkCmdDraw(frame->commandBuffers.scene, leafVertexCount, instanceCount, 0, 0);

	// ************************************************
	// SHADOWS

	VkPipeline shadowPipeline 				= context->leavesShadowPipeline;
	VkPipelineLayout shadowPipelineLayout 	= context->leavesShadowPipelineLayout;

	vkCmdBindPipeline(frame->commandBuffers.offscreen, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

	vkCmdBindDescriptorSets(frame->commandBuffers.offscreen, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout,
							0, 1, &context->cameraDescriptorSet[context->virtualFrameIndex], 0, nullptr);

	vkCmdBindDescriptorSets(frame->commandBuffers.offscreen, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout,
							1, 1, &materialDescriptorSet, 0, nullptr);

	vkCmdBindVertexBuffers(frame->commandBuffers.offscreen, 0, 1, &context->leafBuffer, &instanceBufferOffset);

	vkCmdDraw(frame->commandBuffers.offscreen, leafVertexCount, instanceCount, 0, 0);
}

internal void fsvulkan_drawing_draw_meshes(VulkanContext * context, s32 count, m44 const * transforms, MeshHandle meshHandle, MaterialHandle materialHandle)
{
	Assert(count > 0 && "Vulkan cannot map memory of size 0, and this function should no be called for 0 meshes");
	Assert(context->canDraw);

	VulkanVirtualFrame * frame = fsvulkan_get_current_virtual_frame(context);

	// Todo(Leo): We should instead allocate these from transient memory, but we currently can't access to it from platform layer.
	Assert(count <= 1000);
	u32 uniformBufferOffsets [1000];
	
	/* Note(Leo): This function does not consider animated skeletons, so we skip those.
	We assume that shaders use first entry in uniform buffer as transfrom matrix, so it
	is okay to ignore rest that are unnecessary and just set uniformbuffer offsets properly. */
	// Todo(Leo): use instantiation, so we do no need to align these twice
	VkDeviceSize uniformBufferSizePerItem 	= fsvulkan_get_aligned_uniform_buffer_size(context, sizeof(m44));
	VkDeviceSize totalUniformBufferSize 	= count * uniformBufferSizePerItem;

	VkDeviceSize uniformBufferOffset = fsvulkan_get_uniform_memory(*context, totalUniformBufferSize);

	u8 * bufferPointer;
	vkMapMemory(context->device, context->modelUniformBuffer.memory, uniformBufferOffset, totalUniformBufferSize, 0, (void**)&bufferPointer);
	for (s32 i = 0; i < count; ++i)
	{
		*((m44*)bufferPointer) = transforms[i];
		bufferPointer += uniformBufferSizePerItem;
	}
	vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

	for(s32 i = 0; i < count; ++i)
	{
		uniformBufferOffsets[i] = uniformBufferOffset + i * uniformBufferSizePerItem;
	}

	VulkanMesh * mesh 			= fsvulkan_get_loaded_mesh(context, meshHandle);
	VulkanMaterial * material 	= fsvulkan_get_loaded_material(context, materialHandle);

	Assert(material->pipeline < GRAPHICS_PIPELINE_SCREEN_GUI && "This pipeline does not support mesh rendering");

	VkPipeline pipeline = context->pipelines[material->pipeline].pipeline;
	VkPipelineLayout pipelineLayout = context->pipelines[material->pipeline].pipelineLayout;

	// Bind mesh to normal draw buffer
	vkCmdBindVertexBuffers(frame->commandBuffers.scene, 0, 1, &mesh->bufferReference, &mesh->vertexOffset);
	vkCmdBindIndexBuffer(frame->commandBuffers.scene, mesh->bufferReference, mesh->indexOffset, mesh->indexType);
	
	// Material type
	vkCmdBindPipeline(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkDescriptorSet sets [] =
	{   
		context->cameraDescriptorSet[context->virtualFrameIndex],
		material->descriptorSet,
		context->modelDescriptorSet,
		context->lightingDescriptorSet[context->virtualFrameIndex],
		context->shadowMapTextureDescriptorSet[context->virtualFrameIndex],
		context->skyGradientDescriptorSet,
	};

	Assert(mesh->indexCount > 0);

	f32 smoothness 			= 0.5;
	f32 specularStrength 	= 0.5;
	f32 materialBlock [] = {smoothness, specularStrength};

	vkCmdPushConstants(frame->commandBuffers.scene, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(materialBlock), materialBlock);

	for (s32 i = 0; i < count; ++i)
	{
		vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
								0, array_count(sets), sets,
								1, &uniformBufferOffsets[i]);


		vkCmdDrawIndexed(frame->commandBuffers.scene, mesh->indexCount, 1, 0, 0, 0);

	}

	// ------------------------------------------------
	///////////////////////////
	///   SHADOW PASS       ///
	///////////////////////////
	bool castShadow = true;
	if (castShadow)
	{
		vkCmdBindVertexBuffers(frame->commandBuffers.offscreen, 0, 1, &mesh->bufferReference, &mesh->vertexOffset);
		vkCmdBindIndexBuffer(frame->commandBuffers.offscreen, mesh->bufferReference, mesh->indexOffset, mesh->indexType);

		VkDescriptorSet shadowSets [] =
		{
			context->cameraDescriptorSet[context->virtualFrameIndex],
			context->modelDescriptorSet,
		};

		for (s32 i = 0; i < count; ++i)
		{
			vkCmdBindDescriptorSets(frame->commandBuffers.offscreen,
									VK_PIPELINE_BIND_POINT_GRAPHICS,
									context->shadowPipelineLayout,
									0,
									2,
									shadowSets,
									1,
									&uniformBufferOffsets[i]);

			vkCmdDrawIndexed(frame->commandBuffers.offscreen, mesh->indexCount, 1, 0, 0, 0);
		}
	} 
}

internal void fsvulkan_drawing_draw_procedural_mesh(VulkanContext * context,
													s32 vertexCount, Vertex const * vertices,
													s32 indexCount, u16 const * indices,
													m44 transform, MaterialHandle materialHandle)
{
	// Note(Leo): call prepare_drawing() first
	Assert(context->canDraw);
	Assert(vertexCount > 0);
	Assert(indexCount > 0);

	auto * frame = fsvulkan_get_current_virtual_frame(context);

	VkCommandBuffer commandBuffer 	= fsvulkan_get_current_virtual_frame(context)->commandBuffers.scene;

	VulkanMaterial * material 		= fsvulkan_get_loaded_material(context, materialHandle);
	VkPipeline pipeline 			= context->pipelines[material->pipeline].pipeline;
	VkPipelineLayout pipelineLayout = context->pipelines[material->pipeline].pipelineLayout;

	VkDeviceSize uniformBufferOffset = fsvulkan_get_uniform_memory(*context, sizeof(FSVulkanModelUniformBuffer));
	FSVulkanModelUniformBuffer * pBuffer;

	// Optimize(Leo): Just map this once when rendering starts
	vkMapMemory(context->device, context->modelUniformBuffer.memory, uniformBufferOffset, sizeof(FSVulkanModelUniformBuffer), 0, (void**)&pBuffer);
	
	pBuffer->localToWorld = transform;
	pBuffer->isAnimated = 0;

	vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

	VkDescriptorSet sets [] =
	{   
		context->cameraDescriptorSet[context->virtualFrameIndex],
		material->descriptorSet,
		context->modelDescriptorSet,
		context->lightingDescriptorSet[context->virtualFrameIndex],
		context->shadowMapTextureDescriptorSet[context->virtualFrameIndex],
		context->skyGradientDescriptorSet
	};

	u32 uniformBufferOffsets [] = {(u32)uniformBufferOffset};

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
						0, array_count(sets), sets,
						1, &uniformBufferOffsets[0]);


	Vertex * vertexData;
	VkDeviceSize vertexBufferSize = vertexCount * sizeof(Vertex);
	VkDeviceSize vertexBufferOffset = fsvulkan_get_uniform_memory(*context, vertexBufferSize);

	// Todo(Leo): check map results
	VkResult result = vkMapMemory(context->device, context->modelUniformBuffer.memory, vertexBufferOffset, vertexBufferSize, 0, (void**)&vertexData);

	copy_memory(vertexData, vertices, sizeof(Vertex) * vertexCount);

	vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

	u16 * indexData;
	VkDeviceSize indexBufferSize = indexCount * sizeof(u16);
	VkDeviceSize indexBufferOffset = fsvulkan_get_uniform_memory(*context, indexBufferSize);

	VULKAN_CHECK(vkMapMemory(context->device, context->modelUniformBuffer.memory, indexBufferOffset, indexBufferSize, 0, (void**)&indexData));

	copy_memory(indexData, indices, sizeof(u16) *indexCount);

	vkUnmapMemory(context->device, context->modelUniformBuffer.memory);


	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &context->modelUniformBuffer.buffer, &vertexBufferOffset);
	vkCmdBindIndexBuffer(commandBuffer, context->modelUniformBuffer.buffer, indexBufferOffset, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

	// ****************************************************************
	/// SHADOWS

	vkCmdBindVertexBuffers(frame->commandBuffers.offscreen, 0, 1, &context->modelUniformBuffer.buffer, &vertexBufferOffset);
	vkCmdBindIndexBuffer(frame->commandBuffers.offscreen, context->modelUniformBuffer.buffer, indexBufferOffset, VK_INDEX_TYPE_UINT16);

	VkDescriptorSet shadowSets [] =
	{
		context->cameraDescriptorSet[context->virtualFrameIndex],
		context->modelDescriptorSet,
	};

	vkCmdBindDescriptorSets(frame->commandBuffers.offscreen,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							context->shadowPipelineLayout,
							0,
							2,
							shadowSets,
							1,
							uniformBufferOffsets);

	vkCmdDrawIndexed(frame->commandBuffers.offscreen, indexCount, 1, 0, 0, 0);
}

internal void fsvulkan_drawing_draw_lines(VulkanContext * context, s32 pointCount, v3 const * points, v4 color)
{

	// Note(Leo): call prepare_drawing() first
	Assert(context->canDraw);

	VkCommandBuffer commandBuffer = fsvulkan_get_current_virtual_frame(context)->commandBuffers.scene;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->linePipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->linePipelineLayout,
							0, 1, &context->cameraDescriptorSet[context->virtualFrameIndex], 0, nullptr);

	VkDeviceSize vertexBufferSize 	= pointCount * sizeof(v3) * 2;
	VkDeviceSize vertexBufferOffset = fsvulkan_get_uniform_memory(*context, vertexBufferSize);


	v3 * vertexData;
	// Todo(Leo): check map results
	VkResult result = vkMapMemory(context->device, context->modelUniformBuffer.memory, vertexBufferOffset, vertexBufferSize, 0, (void**)&vertexData);

	for (s32 i = 0; i < pointCount; ++i)
	{
		vertexData[i * 2] = points[i];
		vertexData[i * 2 + 1] = color.rgb;
	}

	vkUnmapMemory(context->device, context->modelUniformBuffer.memory);

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &context->modelUniformBuffer.buffer, &vertexBufferOffset);

	Assert(pointCount > 0);

	vkCmdDraw(commandBuffer, pointCount, 1, 0, 0);
}

internal void fsvulkan_drawing_draw_screen_rects(VulkanContext * context, s32 count, ScreenRect const * rects, GuiTextureHandle textureHandle, v4 color)
{
	// /*
	// vulkan bufferless drawing
	// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
	// */

	auto * frame = fsvulkan_get_current_virtual_frame(context);

	VkPipeline 			pipeline;
	VkPipelineLayout 	pipelineLayout;
	VkDescriptorSet 	descriptorSet;

	if(textureHandle == GRAPHICS_RESOURCE_SHADOWMAP_GUI_TEXTURE)
	{
		descriptorSet = context->shadowMapTextureDescriptorSet[context->virtualFrameIndex];
	}
	else
	{
		descriptorSet = fsvulkan_get_loaded_gui_texture(*context, textureHandle).descriptorSet;
	}

	pipeline 		= context->pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].pipeline;
	pipelineLayout 	= context->pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].pipelineLayout;

	vkCmdBindPipeline(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	
	vkCmdBindDescriptorSets(frame->commandBuffers.scene, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, 1, &descriptorSet,
							0, nullptr);

	// Note(Leo): Color does not change, so we update it only once, this will break if we change shader :(
	vkCmdPushConstants(frame->commandBuffers.scene, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(v4), &color);

	// TODO(Leo): Instantiatee!
	for(s32 i = 0; i < count; ++i)
	{      
		vkCmdPushConstants( frame->commandBuffers.scene, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ScreenRect), &rects[i]);
		vkCmdDraw(frame->commandBuffers.scene, 4, 1, 0, 0);
	}
}