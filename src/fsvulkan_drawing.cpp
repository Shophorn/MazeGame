/*=============================================================================
Leo Tamminen
shophorn @ internet

Vulkan drawing functions' definitions.
=============================================================================*/

internal VkDeviceSize
fsvulkan_get_aligned_uniform_buffer_size(VulkanContext * context, VkDeviceSize size)
{
	auto alignment = context->physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	return memory_align_up(size, alignment);
} 

// Todo(Leo): these two call for a struct and functions
internal VkDeviceSize fsvulkan_get_uniform_memory(VulkanContext & context, VkDeviceSize size)
{
	VkDeviceSize start 	= context.modelUniformBufferFrameStart[context.virtualFrameIndex];
	VkDeviceSize & used = context.modelUniformBufferFrameUsed[context.virtualFrameIndex];

	VkDeviceSize result = start + used;

	VkDeviceSize alignment 	= context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	size 					= memory_align_up(size, alignment);
	used 					+= size;

	Assert(used <= context.modelUniformBufferFrameCapacity);

	return result;
}

internal VkDeviceSize fsvulkan_get_dynamic_mesh_memory(VulkanContext & context, VkDeviceSize size)
{
	VkDeviceSize start 	= context.dynamicMeshFrameStart[context.virtualFrameIndex];
	VkDeviceSize & used = context.dynamicMeshFrameUsed[context.virtualFrameIndex];

	VkDeviceSize result = start + used;

	VkDeviceSize alignment 	= context.physicalDeviceProperties.limits.minUniformBufferOffsetAlignment;
	size 					= memory_align_up(size, alignment);
	used 					+= size;

	Assert(used <= context.dynamicMeshFrameCapacity);

	return result;
}


internal void graphics_drawing_update_camera(VulkanContext * context, Camera const * camera)
{
	FSVulkanCameraUniformBuffer * pUbo = context->persistentMappedCameraUniformBufferMemory[context->virtualFrameIndex];

	pUbo->view 			= camera_view_matrix(camera->position, camera->direction);
	pUbo->projection 	= camera_projection_matrix(camera);
}

internal void graphics_drawing_update_lighting(VulkanContext * context, Light const * light, Camera const * camera, v3 ambient)
{
	FSVulkanLightingUniformBuffer * lightPtr = context->persistentMappedLightingUniformBufferMemory[context->virtualFrameIndex];

	lightPtr->direction    		= v3_to_v4(light->direction, 0);
	lightPtr->color        		= v3_to_v4(light->color, 0);
	lightPtr->ambient      		= v3_to_v4(ambient, 0);
	lightPtr->cameraPosition 	= v3_to_v4(camera->position, 1);
	lightPtr->skyColor 			= light->skyColorSelection;

	lightPtr->skyBottomColor 	= light->skyBottomColor;
	lightPtr->skyTopColor 		= light->skyTopColor;
	lightPtr->skyGroundColor 	= light->skyGroundColor;

	lightPtr->horizonHaloColorAndFalloff 	= light->horizonHaloColorAndFalloff;
	lightPtr->sunHaloColorAndFalloff 		= light->sunHaloColorAndFalloff;

	lightPtr->sunDiscColor 			= light->sunDiscColor;
	lightPtr->sunDiscSizeAndFalloff = light->sunDiscSizeAndFalloff;

	// ------------------------------------------------------------------------------

	m44 lightViewProjection = get_light_view_projection (light, camera);

	FSVulkanCameraUniformBuffer * pUbo = context->persistentMappedCameraUniformBufferMemory[context->virtualFrameIndex];

	pUbo->lightViewProjection 		= lightViewProjection;
	pUbo->shadowDistance 			= light->shadowDistance;
	pUbo->shadowTransitionDistance 	= light->shadowTransitionDistance;
}

internal void graphics_drawing_update_hdr_settings(VulkanContext * context, HdrSettings const * hdrSettings)
{
	context->hdrSettings.exposure = hdrSettings->exposure;
	context->hdrSettings.contrast = hdrSettings->contrast;
}

internal void vulkan_prepare_frame(VulkanContext * context)
{
	auto * frame 				= fsvulkan_get_current_virtual_frame(context);
	auto & sceneRenderTarget 	= context->sceneRenderTargets[context->virtualFrameIndex];

	/// ACQUIRE SWAPCHAIN IMAGE
	{
		// Todo(Leo): frameInUseFence has a weird name, is this really the right place for it?
	    VULKAN_CHECK(vkWaitForFences(context->device, 1, &frame->frameInUseFence, VK_TRUE, VULKAN_NO_TIME_OUT));

	    VkResult result = vkAcquireNextImageKHR(context->device,
	                                            context->swapchain,
	                                            VULKAN_NO_TIME_OUT,
	                                            frame->imageAvailableSemaphore,
	                                            VK_NULL_HANDLE,
	                                            &context->currentDrawFrameIndex);

	    Assert(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR);

	    if (result == VK_SUBOPTIMAL_KHR)
	    {
	    	log_graphics(1, FILE_ADDRESS, "Swapchain is currently suboptimal");
	    }
	}

	// Todo(Leo): For the most part at least currently, we do not need these to be super dynamic, so maybe
	// make some kind of semi-permanent allocations, and don't waste time copying these every frame.
	// FLUSH DYNAMIC BUFFERS
	{
		context->modelUniformBufferFrameUsed[context->virtualFrameIndex] 	= 0;
		context->dynamicMeshFrameUsed[context->virtualFrameIndex] 			= 0;
		context->leafBufferUsed[context->virtualFrameIndex] 				= 0;
	}


	/// RECREATE PRESENT FRAMEBUFFER
	{
		/* Note(Leo): We MUST recreate present framebuffer each frame, since swapchain images do not necessarily
		equal number of virtual frames and therefore we do not know which image we are going to draw.

		Study:https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-4#inpage-nav-5
		
		Historical Note(Leo): We did this for scene framebuffers too until I realized, we actually know their images
		aka attachments all the time.
		*/
		vkDestroyFramebuffer(context->device, context->presentFramebuffers[context->virtualFrameIndex], nullptr);

		auto presentFramebufferCreateInfo 				= vk_framebuffer_create_info();
		presentFramebufferCreateInfo.renderPass 		= context->screenSpaceRenderPass;
		presentFramebufferCreateInfo.attachmentCount 	= 1;
		presentFramebufferCreateInfo.pAttachments 		= &context->swapchainImageViews[context->currentDrawFrameIndex];
		presentFramebufferCreateInfo.width 				= context->swapchainExtent.width;
		presentFramebufferCreateInfo.height 			= context->swapchainExtent.height;
		presentFramebufferCreateInfo.layers 			= 1;

		VULKAN_CHECK(vkCreateFramebuffer(context->device, &presentFramebufferCreateInfo, nullptr, &context->presentFramebuffers[context->virtualFrameIndex]));
	}

	// -----------------------------------------------------

	auto shadowCmdInheritance 			= vk_command_buffer_inheritance_info();
	shadowCmdInheritance.renderPass 	= context->shadowRenderPass;
	shadowCmdInheritance.framebuffer 	= context->shadowFramebuffer[context->virtualFrameIndex];

	auto shadowCmdBegin					= vk_command_buffer_begin_info();
	shadowCmdBegin.flags 				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
										| VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	shadowCmdBegin.pInheritanceInfo 	= &shadowCmdInheritance;

	VULKAN_CHECK(vkBeginCommandBuffer(frame->shadowCommandBuffer, &shadowCmdBegin));

	// -----------------------------------------------------

	auto sceneCmdInheritanceInfo 		= vk_command_buffer_inheritance_info();
	sceneCmdInheritanceInfo.renderPass 	= context->sceneRenderPass;
	sceneCmdInheritanceInfo.framebuffer = sceneRenderTarget.framebuffer;

	auto sceneCmdBeginInfo 				= vk_command_buffer_begin_info();
	sceneCmdBeginInfo.flags 			= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
										| VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	sceneCmdBeginInfo.pInheritanceInfo 	= &sceneCmdInheritanceInfo;

	VULKAN_CHECK (vkBeginCommandBuffer(frame->sceneCommandBuffer, &sceneCmdBeginInfo));
	VULKAN_CHECK (vkBeginCommandBuffer(frame->debugCommandBuffer, &sceneCmdBeginInfo));

	// -----------------------------------------------------

	auto screenSpaceCmdInheritance 			= vk_command_buffer_inheritance_info();
	screenSpaceCmdInheritance.renderPass 	= context->screenSpaceRenderPass;
	screenSpaceCmdInheritance.framebuffer 	= context->presentFramebuffers[context->virtualFrameIndex];

	auto screenSpaceCmdBegin 				= vk_command_buffer_begin_info();
	screenSpaceCmdBegin.flags 				= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
											| VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	screenSpaceCmdBegin.pInheritanceInfo 	= &screenSpaceCmdInheritance;

	VULKAN_CHECK(vkBeginCommandBuffer(frame->postProcessCommandBuffer, &screenSpaceCmdBegin));
	VULKAN_CHECK (vkBeginCommandBuffer(frame->guiCommandBuffer, &screenSpaceCmdBegin));
	

	/// RECORD POST-PROCESS / HDR COMMAND BUFFER
	{
		// Note(Leo): Currently there is nothing else, so we record also here.
		// Todo(Leo): maybe combine this with gui command buffer, and just record these at the beningining

		vkCmdBindPipeline(	frame->postProcessCommandBuffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							context->screenSpacePipeline);

		vkCmdBindDescriptorSets(frame->postProcessCommandBuffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								context->screenSpacePipelineLayout,
								0, 1, &context->sceneRenderTargetSamplerDescriptors[context->virtualFrameIndex],
								0, nullptr);

		vkCmdPushConstants(	frame->postProcessCommandBuffer,
							context->screenSpacePipelineLayout,
							VK_SHADER_STAGE_FRAGMENT_BIT,
							0, sizeof(FSVulkanHdrSettings),
							&context->hdrSettings);

		vkCmdDraw(frame->postProcessCommandBuffer, 3, 1, 0, 0);
	}
}

internal void vulkan_render_frame(VulkanContext * context)
{
	VulkanVirtualFrame * frame = fsvulkan_get_current_virtual_frame(context);


	/* Note (Leo): beginning command buffer implicitly resets it, if we have specified
	VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT in command pool creation.
	This is done implicitly with other command buffers as well, and I'm bit concerned that I will forget those :S */

	/// BEGIN MAIN CMD
	auto mainCmdBegin 	= vk_command_buffer_begin_info();
	mainCmdBegin.flags 	= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VULKAN_CHECK(vkBeginCommandBuffer(frame->mainCommandBuffer, &mainCmdBegin));

	// Todo(Leo): Some of these may actually be non changing stuff, so we could just do them once before. Maybe.
	// SHADOW RENDER PASS
	{
		VULKAN_CHECK(vkEndCommandBuffer(frame->shadowCommandBuffer));
	
		VkClearValue shadowClearValue = { .depthStencil = {1.0f, 0} };

		auto shadowRenderPassBegin 				= vk_render_pass_begin_info();
		shadowRenderPassBegin.renderPass 		= context->shadowRenderPass;
		shadowRenderPassBegin.framebuffer 		= context->shadowFramebuffer[context->virtualFrameIndex];
		shadowRenderPassBegin.renderArea 		= {{0,0}, {context->shadowTextureWidth, context->shadowTextureHeight}};
		shadowRenderPassBegin.clearValueCount 	= 1;
		shadowRenderPassBegin.pClearValues 		= &shadowClearValue;

		vkCmdBeginRenderPass(frame->mainCommandBuffer, &shadowRenderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		
		vkCmdExecuteCommands(frame->mainCommandBuffer, 1, &frame->shadowCommandBuffer);
		
		vkCmdEndRenderPass(frame->mainCommandBuffer);
	}

	/// SCENE RENDER PASS
	{
		// Todo(Low): Do debug lines in separate render pass after post processing, but that requires separate render pass

		VULKAN_CHECK(vkEndCommandBuffer(frame->sceneCommandBuffer));
		VULKAN_CHECK(vkEndCommandBuffer(frame->debugCommandBuffer));

		VkClearValue clearValues [2] 	= {};
		clearValues[0].color 			= {0.35f, 0.0f, 0.35f, 1.0f};
		clearValues[1].depthStencil 	= {1.0f, 0};

		auto sceneRenderPassBegin 				= vk_render_pass_begin_info();
		sceneRenderPassBegin.renderPass 		= context->sceneRenderPass;
		sceneRenderPassBegin.framebuffer 		= context->sceneRenderTargets[context->virtualFrameIndex].framebuffer;
		sceneRenderPassBegin.renderArea 		= {{0,0}, {context->sceneRenderTargetWidth, context->sceneRenderTargetHeight}};
		sceneRenderPassBegin.clearValueCount 	= array_count(clearValues);
		sceneRenderPassBegin.pClearValues 		= clearValues;

		vkCmdBeginRenderPass(frame->mainCommandBuffer, &sceneRenderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
		
		vkCmdExecuteCommands(frame->mainCommandBuffer, 1, &frame->sceneCommandBuffer);
		vkCmdExecuteCommands(frame->mainCommandBuffer, 1, &frame->debugCommandBuffer);
		
		vkCmdEndRenderPass(frame->mainCommandBuffer);
	}

	/// SCREEN SPACE RENDER PASS
	{
		VULKAN_CHECK(vkEndCommandBuffer(frame->postProcessCommandBuffer));
		VULKAN_CHECK(vkEndCommandBuffer(frame->guiCommandBuffer));

		// Note(Leo): no need to clear this, we fill every pixel anyway
		auto screenSpaceRenderPassBegin 		= vk_render_pass_begin_info();
		screenSpaceRenderPassBegin.renderPass 	= context->screenSpaceRenderPass;
		screenSpaceRenderPassBegin.framebuffer 	= context->presentFramebuffers[context->virtualFrameIndex];
		screenSpaceRenderPassBegin.renderArea 	= {{0,0}, context->swapchainExtent};

		vkCmdBeginRenderPass(frame->mainCommandBuffer, &screenSpaceRenderPassBegin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		vkCmdExecuteCommands(frame->mainCommandBuffer, 1, &frame->postProcessCommandBuffer);
		vkCmdExecuteCommands(frame->mainCommandBuffer, 1, &frame->guiCommandBuffer);

		vkCmdEndRenderPass(frame->mainCommandBuffer);
	}

	/// SUBMIT MAIN COMMAND BUFFER TO GRAPHICS QUEUE
	{
		VULKAN_CHECK(vkEndCommandBuffer(frame->mainCommandBuffer));

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		
		auto renderQueueSubmit 					= vk_submit_info();
		renderQueueSubmit.waitSemaphoreCount 	= 1;
		renderQueueSubmit.pWaitSemaphores 		= &frame->imageAvailableSemaphore;
		renderQueueSubmit.pWaitDstStageMask 	= &waitStage;
		renderQueueSubmit.commandBufferCount 	= 1;
		renderQueueSubmit.pCommandBuffers 		= &frame->mainCommandBuffer;
		renderQueueSubmit.signalSemaphoreCount 	= 1;
		renderQueueSubmit.pSignalSemaphores 	= &frame->renderFinishedSemaphore;

		// Todo(Leo): Should fence be signaled with presenting?
		vkResetFences(context->device, 1, &frame->frameInUseFence);
		VULKAN_CHECK(vkQueueSubmit(context->graphicsQueue, 1, &renderQueueSubmit, frame->frameInUseFence));
	}

	/// PRESENT IMAGE
	{
		auto present 				= vk_present_info_KHR();
		present.waitSemaphoreCount 	= 1;
		present.pWaitSemaphores	 	= &frame->renderFinishedSemaphore;
		present.swapchainCount 		= 1;
		present.pSwapchains 		= &context->swapchain;

		// Todo(Leo): this is maybe fishy, probably should be more specific about how we manage this index.
		present.pImageIndices 		= &context->currentDrawFrameIndex;

		VkResult result = vkQueuePresentKHR(context->presentQueue, &present);

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			log_graphics(1, FILE_ADDRESS, "Present result = ", fsvulkan_result_string(result));
			Assert(false);
		}
	}

	/// ----------- POST RENDER EVENTS ---------------

	// Todo(Leo): maybe make proper separate functions for these

	if(context->postRenderEvents.reloadShaders)
	{
		vkDeviceWaitIdle(context->device);

		vkResetDescriptorPool(context->device, context->drawingResourceDescriptorPool, 0);

		fsvulkan_cleanup_pipelines(context);
		fsvulkan_initialize_pipelines(*context, context->sceneRenderTargetWidth, context->sceneRenderTargetHeight);


		for (s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
		{
			// Todo(Leo): not cool man!
			context->shadowMapTextureDescriptorSet[i] = make_material_vk_descriptor_set_2( context,
																					context->shadowMapTextureDescriptorSetLayout,
																					context->shadowAttachment[i].view,
																					context->drawingResourceDescriptorPool,
																					context->shadowTextureSampler,
																					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

		log_graphics(0, "Reloaded shaders");
	}

	if (context->postRenderEvents.unloadAssets)
	{
		vkDeviceWaitIdle(context->device);
		BAD_BUT_ACTUAL_graphics_memory_unload(context);
	}

	/// RESET EVENTS
	context->postRenderEvents = {};

	// ---------------------------------------------------------------------

	context->virtualFrameIndex += 1;
	context->virtualFrameIndex %= VIRTUAL_FRAME_COUNT;
}

internal void graphics_draw_model(VulkanContext * context, ModelHandle model, m44 transform, bool32 castShadow, m44 const * bones, u32 bonesCount)
{
	/* Todo(Leo): Get rid of these, we can just as well get them directly from user.
	That is more flexible and then we don't need to save that data in multiple places. */
	MeshHandle meshHandle           = context->loadedModels[model].mesh;
	MaterialHandle materialHandle   = context->loadedModels[model].material;

	u32 uniformBufferSize   = sizeof(VulkanModelUniformBuffer);

	VkDeviceSize uniformBufferOffset 	= fsvulkan_get_uniform_memory(*context, sizeof(VulkanModelUniformBuffer));
	VulkanModelUniformBuffer * pBuffer 	= reinterpret_cast<VulkanModelUniformBuffer*>(context->persistentMappedModelUniformBufferMemory + uniformBufferOffset);

	// Todo(Leo): Just map this once when rendering starts
	// // Todo(Leo): or even better, get this pointer to game code and write directly to buffer, no copy needed
	
	pBuffer->localToWorld 	= transform;
	pBuffer->isAnimated 	= bonesCount;

	Assert(bonesCount <= array_count(pBuffer->bonesToLocal));    
	memory_copy(pBuffer->bonesToLocal, bones, sizeof(m44) * bonesCount);

	// ---------------------------------------------------------------

	VulkanVirtualFrame * frame = fsvulkan_get_current_virtual_frame(context);
 
	auto * mesh     = fsvulkan_get_loaded_mesh(context, meshHandle);
	auto * material = fsvulkan_get_loaded_material(context, materialHandle);

	Assert(material->pipeline < GraphicsPipeline_screen_gui && "This pipeline does not support mesh rendering");

	VkPipeline pipeline = context->pipelines[material->pipeline].pipeline;
	VkPipelineLayout pipelineLayout = context->pipelines[material->pipeline].pipelineLayout;

	// Material type
	vkCmdBindPipeline(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	
	VkDescriptorSet sets [] =
	{   
		context->cameraDescriptorSet[context->virtualFrameIndex],
		material->descriptorSet,
		context->modelDescriptorSet[context->virtualFrameIndex],
		context->lightingDescriptorSet[context->virtualFrameIndex],
		context->shadowMapTextureDescriptorSet[context->virtualFrameIndex],
		context->skyGradientDescriptorSet,
	};

	// Note(Leo): this works, because models are only dynamic set
	Assert(uniformBufferOffset <= max_value_u32);
	u32 dynamicOffsets [] = {(u32)uniformBufferOffset};

	vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, array_count(sets), sets,
							array_count(dynamicOffsets), dynamicOffsets);


	VkBuffer vertexBuffers [] 		= {mesh->bufferReference, mesh->bufferReference};
	VkDeviceSize vertexOffsets [] 	= {mesh->vertexOffset, mesh->skinningOffset};
	s32 vertexBindingCount 			= mesh->hasSkinning ? 2 : 1;
	vkCmdBindVertexBuffers(frame->sceneCommandBuffer, 0, vertexBindingCount, vertexBuffers, vertexOffsets);

	vkCmdBindIndexBuffer(frame->sceneCommandBuffer, mesh->bufferReference, mesh->indexOffset, mesh->indexType);

	Assert(mesh->indexCount > 0);

	vkCmdDrawIndexed(frame->sceneCommandBuffer, mesh->indexCount, 1, 0, 0, 0);

	// ------------------------------------------------
	///////////////////////////
	///   SHADOW PASS       ///
	///////////////////////////
	if (castShadow)
	{
		vkCmdBindPipeline(frame->shadowCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowPipeline);

		vkCmdBindVertexBuffers(frame->shadowCommandBuffer, 0, vertexBindingCount, vertexBuffers, vertexOffsets);
		vkCmdBindIndexBuffer(frame->shadowCommandBuffer, mesh->bufferReference, mesh->indexOffset, mesh->indexType);

		VkDescriptorSet shadowSets [] =
		{
			context->cameraDescriptorSet[context->virtualFrameIndex],
			context->modelDescriptorSet[context->virtualFrameIndex],
		};

		vkCmdBindDescriptorSets(frame->shadowCommandBuffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								context->shadowPipelineLayout,
								0,
								2,
								shadowSets,
								1,
								&dynamicOffsets[0]);

		vkCmdDrawIndexed(frame->shadowCommandBuffer, mesh->indexCount, 1, 0, 0, 0);
	}
}

// Note(Leo): these are notes :)
// internal void * get_leaf_buffer_memory()
// {
// 	return context->persistentMappedLeafBufferMemory;
// }

// internal void graphics_draw_leaves(VulkanContext * context, s64 count, s64 bufferOffset)
// {
// 	// bind camera
// 	// bind material
// 	// bind shadows
// 	// bind lights
// 	// bind etc.

// 	vkCmdBindVertexBuffers(commandBuffer, 0, 1, context->leafBuffer, bufferOffset);
// 	vkCmdDraw(commandBuffer, 4, count, 0, 0);
// }

internal void graphics_draw_leaves(	VulkanContext * context,
											s32 instanceCount,
											m44 const * instanceTransforms,
											s32 colourIndex,
											v3 colour,
											MaterialHandle materialHandle)
{
	// Assert(instanceCount > 0 && "Vulkan cannot map memory of size 0, and this function should no be called for 0 meshes");
	// Note(Leo): lets keep this sensible
	// Assert(instanceCount <= 20000);
	if (instanceCount == 0)
	{
		log_graphics(FILE_ADDRESS, "Drawing 0 meshes!");
		return;
	}

	constexpr s64 maxCount = 20000;
	if (instanceCount > maxCount)
	{
		log_graphics(FILE_ADDRESS, "Drawing too many (over 20000) meshes, skipping the rest");
		instanceCount = maxCount;
	}


	VulkanVirtualFrame * frame 	= fsvulkan_get_current_virtual_frame(context);
	s64 frameOffset 			= context->leafBufferCapacity * context->virtualFrameIndex;
	u64 instanceBufferOffset 	= frameOffset + context->leafBufferUsed[context->virtualFrameIndex];

	u64 instanceBufferSize 		= instanceCount * sizeof(m44);
	context->leafBufferUsed[context->virtualFrameIndex] += instanceBufferSize;

	Assert(context->leafBufferUsed[context->virtualFrameIndex] <= context->leafBufferCapacity);

	void * bufferPointer = (u8*)context->persistentMappedLeafBufferMemory + instanceBufferOffset;
	memory_copy(bufferPointer, instanceTransforms, instanceCount * sizeof(m44));

	VkPipeline pipeline 			= context->pipelines[GraphicsPipeline_leaves].pipeline;
	VkPipelineLayout pipelineLayout = context->pipelines[GraphicsPipeline_leaves].pipelineLayout;
	
	vkCmdBindPipeline(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	// Note(Leo): These todos are not so relevant anymore, I decided to make a single collective rendering function, that
	// is not so much separated from graphics api layer, so while points remain valid, the situation has changed.
	// Todo(Leo): These can be bound once per frame, they do not change
	// Todo(Leo): Except that they need to be bound per pipeline, maybe unless selected binding etc. are identical
	vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, 1, &context->cameraDescriptorSet[context->virtualFrameIndex],
							0, nullptr);

	vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							1, 1, &context->lightingDescriptorSet[context->virtualFrameIndex],
							0, nullptr);

	vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							2, 1, &context->shadowMapTextureDescriptorSet[context->virtualFrameIndex],
							0, nullptr);

	VkDescriptorSet materialDescriptorSet = fsvulkan_get_loaded_material(context, materialHandle)->descriptorSet;
	vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 3, 1, &materialDescriptorSet, 0, nullptr);

	vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							4, 1, &context->skyGradientDescriptorSet,
							0, nullptr);

	struct PushConstants
	{
		s32 ___;
		v3 colour;
	};

	PushConstants pushConstants = {colourIndex, colour};

	vkCmdPushConstants(frame->sceneCommandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(v3), &colour);
	// vkCmdPushConstants(frame->sceneCommandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(s32), &colourIndex);

	vkCmdBindVertexBuffers(frame->sceneCommandBuffer, 0, 1, &context->leafBuffer, &instanceBufferOffset);

	// Note(Leo): Fukin cool, instantiation at last :DDDD 4.6.
	// Note(Leo): This is hardcoded in shaders, we generate vertex data on demand
	constexpr s32 leafVertexCount = 4;
	vkCmdDraw(frame->sceneCommandBuffer, leafVertexCount, instanceCount, 0, 0);

	// ************************************************
	// SHADOWS

	VkPipeline shadowPipeline 				= context->leavesShadowPipeline;
	VkPipelineLayout shadowPipelineLayout 	= context->leavesShadowPipelineLayout;

	vkCmdBindPipeline(frame->shadowCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipeline);

	vkCmdBindDescriptorSets(frame->shadowCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout,
							0, 1, &context->cameraDescriptorSet[context->virtualFrameIndex], 0, nullptr);

	vkCmdBindDescriptorSets(frame->shadowCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, shadowPipelineLayout,
							1, 1, &materialDescriptorSet, 0, nullptr);

	vkCmdBindVertexBuffers(frame->shadowCommandBuffer, 0, 1, &context->leafBuffer, &instanceBufferOffset);

	vkCmdDraw(frame->shadowCommandBuffer, leafVertexCount, instanceCount, 0, 0);
}

internal void graphics_draw_meshes(VulkanContext * context, s32 count, m44 const * transforms, MeshHandle meshHandle, MaterialHandle materialHandle)
{
	if (count == 0)
	{
		log_graphics(1, FILE_ADDRESS, "Drawing 0 meshes!");
		return;
	}

	// Todo(Leo): this was speedy addition, look below, we kinda handled this already, do same with leaves also
	constexpr s64 maxCount = 20000;
	if (count > maxCount)
	{
		log_graphics(FILE_ADDRESS, "Drawing too many (over 20000) meshes, skipping the rest");
		count = maxCount;
	}


	VulkanVirtualFrame * frame = fsvulkan_get_current_virtual_frame(context);

	// Todo(Leo): We should instead allocate these from transient memory, but we currently can't access to it from platform layer.
	// Assert(count <= 2000);
	
	if (count > 1000)
	{
		while(count > 1000)
		{
			graphics_draw_meshes(context, 1000, transforms, meshHandle, materialHandle);

			count 		-= 1000;
			transforms 	+= 1000;
		}
	}
	u32 uniformBufferOffsets [1000];


	/* Note(Leo): This function does not consider animated skeletons, so we skip those.
	We assume that shaders use first entry in uniform buffer as transfrom matrix, so it
	is okay to ignore rest that are unnecessary and just set uniformbuffer offsets properly. */
	// Todo(Leo): use instantiation, so we do no need to align these twice
	VkDeviceSize uniformBufferSizePerItem 	= fsvulkan_get_aligned_uniform_buffer_size(context, sizeof(m44));
	VkDeviceSize totalUniformBufferSize 	= count * uniformBufferSizePerItem;

	VkDeviceSize uniformBufferOffset 	= fsvulkan_get_uniform_memory(*context, totalUniformBufferSize);
	u8 * bufferPointer 					= context->persistentMappedModelUniformBufferMemory + uniformBufferOffset;
	
	for (s32 i = 0; i < count; ++i)
	{
		*reinterpret_cast<m44*>(bufferPointer) 	= transforms[i];
		bufferPointer 							+= uniformBufferSizePerItem;
		uniformBufferOffsets[i] 				= uniformBufferOffset + i * uniformBufferSizePerItem;
	}

	VulkanMesh * mesh 			= fsvulkan_get_loaded_mesh(context, meshHandle);
	VulkanMaterial * material 	= fsvulkan_get_loaded_material(context, materialHandle);

	Assert(material->pipeline < GraphicsPipeline_screen_gui && "This pipeline does not support mesh rendering");

	VkPipeline pipeline = context->pipelines[material->pipeline].pipeline;
	VkPipelineLayout pipelineLayout = context->pipelines[material->pipeline].pipelineLayout;

	// Bind mesh to normal draw buffer
	vkCmdBindVertexBuffers(frame->sceneCommandBuffer, 0, 1, &mesh->bufferReference, &mesh->vertexOffset);
	vkCmdBindIndexBuffer(frame->sceneCommandBuffer, mesh->bufferReference, mesh->indexOffset, mesh->indexType);
	
	// Material type
	vkCmdBindPipeline(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	VkDescriptorSet sets [] =
	{   
		context->cameraDescriptorSet[context->virtualFrameIndex],
		material->descriptorSet,
		context->modelDescriptorSet[context->virtualFrameIndex],
		context->lightingDescriptorSet[context->virtualFrameIndex],
		context->shadowMapTextureDescriptorSet[context->virtualFrameIndex],
		context->skyGradientDescriptorSet,
	};

	Assert(mesh->indexCount > 0);

	f32 smoothness 			= 0.5;
	f32 specularStrength 	= 0.5;
	f32 materialBlock [] = {smoothness, specularStrength};

	vkCmdPushConstants(frame->sceneCommandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(materialBlock), materialBlock);

	for (s32 i = 0; i < count; ++i)
	{
		vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
								0, array_count(sets), sets,
								1, &uniformBufferOffsets[i]);


		vkCmdDrawIndexed(frame->sceneCommandBuffer, mesh->indexCount, 1, 0, 0, 0);

	}

	// ------------------------------------------------
	///////////////////////////
	///   SHADOW PASS       ///
	///////////////////////////
	bool castShadow = true;
	if (castShadow)
	{
		VkDeviceSize vertexOffsets [] 	= {mesh->vertexOffset, mesh->skinningOffset};
		VkBuffer vertexBuffers [] 		= {mesh->bufferReference, mesh->bufferReference};

		vkCmdBindPipeline(frame->shadowCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowPipeline);

		vkCmdBindVertexBuffers(frame->shadowCommandBuffer, 0, 2, vertexBuffers, vertexOffsets);
		vkCmdBindIndexBuffer(frame->shadowCommandBuffer, mesh->bufferReference, mesh->indexOffset, mesh->indexType);

		VkDescriptorSet shadowSets [] =
		{
			context->cameraDescriptorSet[context->virtualFrameIndex],
			context->modelDescriptorSet[context->virtualFrameIndex],
		};

		for (s32 i = 0; i < count; ++i)
		{
			vkCmdBindDescriptorSets(frame->shadowCommandBuffer,
									VK_PIPELINE_BIND_POINT_GRAPHICS,
									context->shadowPipelineLayout,
									0,
									2,
									shadowSets,
									1,
									&uniformBufferOffsets[i]);

			vkCmdDrawIndexed(frame->shadowCommandBuffer, mesh->indexCount, 1, 0, 0, 0);
		}
	} 
}

internal void graphics_draw_procedural_mesh(VulkanContext * context,
											s32 vertexCount, Vertex const * vertices,
											s32 indexCount, u16 const * indices,
											m44 transform, MaterialHandle materialHandle)
{
	Assert(vertexCount > 0);
	Assert(indexCount > 0);

	auto * frame = fsvulkan_get_current_virtual_frame(context);

	VkCommandBuffer commandBuffer 	= fsvulkan_get_current_virtual_frame(context)->sceneCommandBuffer;

	VulkanMaterial * material 		= fsvulkan_get_loaded_material(context, materialHandle);
	VkPipeline pipeline 			= context->pipelines[material->pipeline].pipeline;
	VkPipelineLayout pipelineLayout = context->pipelines[material->pipeline].pipelineLayout;

	VkDeviceSize uniformBufferOffset 	= fsvulkan_get_uniform_memory(*context, sizeof(VulkanModelUniformBuffer));
	VulkanModelUniformBuffer * pBuffer 	= reinterpret_cast<VulkanModelUniformBuffer*>(context->persistentMappedModelUniformBufferMemory + uniformBufferOffset);

	pBuffer->localToWorld = transform;
	pBuffer->isAnimated = 0;
	// Note(Leo): skip animation info

	VkDescriptorSet sets [] =
	{   
		context->cameraDescriptorSet[context->virtualFrameIndex],
		material->descriptorSet,
		context->modelDescriptorSet[context->virtualFrameIndex],
		context->lightingDescriptorSet[context->virtualFrameIndex],
		context->shadowMapTextureDescriptorSet[context->virtualFrameIndex],
		context->skyGradientDescriptorSet
	};

	u32 uniformBufferOffsets [] = {(u32)uniformBufferOffset};

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
						0, array_count(sets), sets,
						1, &uniformBufferOffsets[0]);

	VkDeviceSize vertexBufferSize 	= vertexCount * sizeof(Vertex);
	VkDeviceSize vertexBufferOffset = fsvulkan_get_dynamic_mesh_memory(*context, vertexBufferSize);
	Vertex * vertexData 			= reinterpret_cast<Vertex*>(context->persistentMappedDynamicMeshMemory + vertexBufferOffset);
	memory_copy(vertexData, vertices, sizeof(Vertex) * vertexCount);

	VkDeviceSize indexBufferSize 	= indexCount * sizeof(u16);
	VkDeviceSize indexBufferOffset 	= fsvulkan_get_dynamic_mesh_memory(*context, indexBufferSize);
	u16 * indexData					= reinterpret_cast<u16*>(context->persistentMappedDynamicMeshMemory + indexBufferOffset);
	memory_copy(indexData, indices, sizeof(u16) *indexCount);





	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &context->dynamicMeshBuffer, &vertexBufferOffset);
	vkCmdBindIndexBuffer(commandBuffer, context->dynamicMeshBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT16);

	vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);

	// ****************************************************************
	/// SHADOWS

	VkDeviceSize vertexOffsets [] 	= {vertexBufferOffset, 0};
	VkBuffer vertexBuffers [] 		= {context->dynamicMeshBuffer, context->dynamicMeshBuffer};

	vkCmdBindPipeline(frame->shadowCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->shadowPipeline);

	vkCmdBindVertexBuffers(frame->shadowCommandBuffer, 0, 2, vertexBuffers, vertexOffsets);
	vkCmdBindIndexBuffer(frame->shadowCommandBuffer, context->dynamicMeshBuffer, indexBufferOffset, VK_INDEX_TYPE_UINT16);

	VkDescriptorSet shadowSets [] =
	{
		context->cameraDescriptorSet[context->virtualFrameIndex],
		context->modelDescriptorSet[context->virtualFrameIndex],
	};

	vkCmdBindDescriptorSets(frame->shadowCommandBuffer,
							VK_PIPELINE_BIND_POINT_GRAPHICS,
							context->shadowPipelineLayout,
							0,
							2,
							shadowSets,
							1,
							uniformBufferOffsets);

	vkCmdDrawIndexed(frame->shadowCommandBuffer, indexCount, 1, 0, 0, 0);
}

internal void graphics_draw_lines(VulkanContext * context, s32 pointCount, v3 const * points, v4 color)
{
	VkCommandBuffer commandBuffer = fsvulkan_get_current_virtual_frame(context)->debugCommandBuffer;

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->linePipeline);
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, context->linePipelineLayout,
							0, 1, &context->cameraDescriptorSet[context->virtualFrameIndex], 0, nullptr);

	VkDeviceSize vertexBufferSize 	= pointCount * sizeof(v3) * 2;
	VkDeviceSize vertexBufferOffset = fsvulkan_get_dynamic_mesh_memory(*context, vertexBufferSize);
	v3 * vertexData 				= reinterpret_cast<v3*>(context->persistentMappedDynamicMeshMemory + vertexBufferOffset);

	for (s32 i = 0; i < pointCount; ++i)
	{
		vertexData[i * 2] = points[i];
		vertexData[i * 2 + 1] = color.rgb;
	}

	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &context->dynamicMeshBuffer, &vertexBufferOffset);

	Assert(pointCount > 0);

	vkCmdDraw(commandBuffer, pointCount, 1, 0, 0);
}

internal void graphics_draw_screen_rects(	VulkanContext * 	context,
											s32 				count,
											ScreenRect const * 	rects,
											MaterialHandle 		materialHandle,
											// GuiTextureHandle 	textureHandle,
											v4 					color)
{
	// /*
	// vulkan bufferless drawing
	// https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
	// */

	auto * frame = fsvulkan_get_current_virtual_frame(context);

	VkPipeline 			pipeline;
	VkPipelineLayout 	pipelineLayout;
	VkDescriptorSet 	descriptorSet;

	if (materialHandle == GRAPHICS_RESOURCE_SHADOWMAP_GUI_MATERIAL)
	{
		descriptorSet = context->shadowMapTextureDescriptorSet[context->virtualFrameIndex];
	}
	else
	{
		VulkanMaterial * material = fsvulkan_get_loaded_material(context, materialHandle);
		Assert(material->pipeline == GraphicsPipeline_screen_gui);

		descriptorSet = material->descriptorSet;
	}

	// if(textureHandle == GRAPHICS_RESOURCE_SHADOWMAP_GUI_TEXTURE)
	// {
	// 	descriptorSet = context->shadowMapTextureDescriptorSet[context->virtualFrameIndex];
	// }
	// else
	// {
	// 	descriptorSet = fsvulkan_get_loaded_gui_texture(*context, textureHandle).descriptorSet;
	// }

	pipeline 		= context->pipelines[GraphicsPipeline_screen_gui].pipeline;
	pipelineLayout 	= context->pipelines[GraphicsPipeline_screen_gui].pipelineLayout;

	vkCmdBindPipeline(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	
	vkCmdBindDescriptorSets(frame->sceneCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
							0, 1, &descriptorSet,
							0, nullptr);

	// Note(Leo): Color does not change, so we update it only once, this will break if we change shader :(
	vkCmdPushConstants(frame->sceneCommandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(v4), &color);

	// TODO(Leo): Instantiatee!
	for(s32 i = 0; i < count; ++i)
	{      
		vkCmdPushConstants( frame->sceneCommandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ScreenRect), &rects[i]);
		vkCmdDraw(frame->sceneCommandBuffer, 4, 1, 0, 0);
	}
}