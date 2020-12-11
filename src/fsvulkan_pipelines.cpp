/*
Leo Tamminen
shophorn @ internet

Vulkan layer pipeline creation things :)
*/

// *************************************************************************
/// QUESTIONABLE INITIALIZERS
/* Note(Leo): Not super sure about these, they kinda hide functionality we would rather expose. Works well for now though. */

internal VkDescriptorSetLayout fsvulkan_make_material_descriptor_set_layout(VkDevice device, u32 textureCount)
{
	VkDescriptorSetLayoutBinding binding = 
	{
		.binding             = 0,
		.descriptorType      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount     = textureCount,
		.stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT,
	};

	VkDescriptorSetLayoutCreateInfo layoutCreateInfo = 
	{
		.sType 			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount 	= 1,
		.pBindings 		= &binding
	};

	VkDescriptorSetLayout resultDescriptorSetLayout;
	VULKAN_CHECK(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout));
	
	return resultDescriptorSetLayout;
}



internal VkShaderModule fsvulkan_make_shader_module(VkDevice device, char const * sourceFilename)
{
	PlatformFileHandle file = platform_file_open(sourceFilename, FILE_MODE_READ);
	s64 fileSize 			= platform_file_get_size(file);

	u32 * memory = reinterpret_cast<u32*>(malloc(fileSize));
	platform_file_read(file, 0, fileSize, memory);

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = fileSize;
	createInfo.pCode = memory;


	VkShaderModule result;
	VULKAN_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &result));


	platform_file_close(file);
	free(memory);

	return result;
}

// *************************************************************************
/// GOOD INITIALIZERS
// Note(Leo): Only expose from these functions properties we are interested to change

constexpr VkVertexInputBindingDescription fsvulkan_defaultVertexBinding [] =
{ 
	{ 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX },
	{ 1, sizeof(VertexSkinData), VK_VERTEX_INPUT_RATE_VERTEX },
};

constexpr VkVertexInputAttributeDescription fsvulkan_defaultVertexAttributes [] =
{
	{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
	{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
	{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
	{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)},

	{4, 1, VK_FORMAT_R32G32B32A32_UINT, offsetof(VertexSkinData, boneIndices)},
	{5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexSkinData, boneWeights)},
};

constexpr s32 normalBindingCount = 1;
constexpr s32 animatedBindingCount = 2;

constexpr s32 normalAttributeCount = 4;
constexpr s32 animatedAttributeCount = 6;



// Note(Leo): We can specify zero viewport and scissors now since we use dynamic state on those,
// but eventually we will probably want to use actual sizes, since these are not going to change too much during the game
constexpr VkViewport fsvulkan_zero_viewport = {};
constexpr VkRect2D fsvulkan_zero_scissor = {};

constexpr VkDynamicState fsvulkan_default_dynamic_states [] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

constexpr VkPipelineColorBlendAttachmentState fsvulkan_default_pipeline_color_blend_attachment_state = 
{
	.blendEnable            = VK_FALSE,
	.srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,
	.dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO,
	.colorBlendOp           = VK_BLEND_OP_ADD,
	.srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,
	.dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
	.alphaBlendOp           = VK_BLEND_OP_ADD, 
	.colorWriteMask         = VK_COLOR_COMPONENT_R_BIT 
								| VK_COLOR_COMPONENT_G_BIT 
								| VK_COLOR_COMPONENT_B_BIT 
								| VK_COLOR_COMPONENT_A_BIT,
};

constexpr VkColorComponentFlags fsvulkan_all_colors =	VK_COLOR_COMPONENT_R_BIT 
														| VK_COLOR_COMPONENT_G_BIT 
														| VK_COLOR_COMPONENT_B_BIT 
														| VK_COLOR_COMPONENT_A_BIT;

internal VkPipelineShaderStageCreateInfo
fsvulkan_pipeline_shader_stage_create_info (VkShaderStageFlagBits 	stage,
											VkShaderModule 			module,
											char const * 			name)
{
	VkPipelineShaderStageCreateInfo createInfo = 
	{	
		.sType 					= VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext 					= nullptr,
		.flags 					= 0, // Study(Leo): These are probably important /useful
 		.stage 					= stage,
		.module 				= module,
		.pName 					= name,
		.pSpecializationInfo 	= nullptr
	};

	return createInfo;
};


internal VkPipelineViewportStateCreateInfo
fsvulkan_pipeline_viewport_state_create_info(u32 viewportCount, VkViewport const * pViewports, u32 scissorCount, VkRect2D const * pScissors)
{
	VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount  = viewportCount,
		.pViewports     = pViewports,
		.scissorCount   = scissorCount,
		.pScissors      = pScissors,
	};

	return viewportState;
}

internal VkPipelineRasterizationStateCreateInfo
fsvulkan_pipeline_rasterization_state_create_info(VkCullModeFlags cullMode)
{
	VkPipelineRasterizationStateCreateInfo rasterizationState =
	{
		.sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable           = VK_FALSE,
		.rasterizerDiscardEnable    = VK_FALSE,
		.polygonMode                = VK_POLYGON_MODE_FILL,
		.cullMode                   = cullMode,
		.frontFace                  = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable            = VK_FALSE,
		.depthBiasConstantFactor    = 0.0f,
		.depthBiasClamp             = 0.0f,
		.depthBiasSlopeFactor       = 0.0f,
		.lineWidth                  = 1.0f,
	};

	return rasterizationState;
}

internal VkPipelineLayoutCreateInfo
fsvulkan_pipeline_layout_create_info(	u32 setLayoutCount, VkDescriptorSetLayout const * pSetLayouts,
										u32 pushConstantRangeCount, VkPushConstantRange const * pPushConstantRanges)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext					= nullptr,
		.flags 					= 0,
		.setLayoutCount         = setLayoutCount,
		.pSetLayouts            = pSetLayouts,
		.pushConstantRangeCount = pushConstantRangeCount,
		.pPushConstantRanges    = pPushConstantRanges,
	};

	return pipelineLayoutInfo;
}

internal VkPipelineVertexInputStateCreateInfo
fsvulkan_pipeline_vertex_input_state_create_info(	u32 bindingDescriptionCount, VkVertexInputBindingDescription const * bindingDescriptions,
													u32 attributeDescriptionCount, VkVertexInputAttributeDescription const * attributeDescriptions)
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo =
	{
		.sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.pNext 								= nullptr,
		.flags 								= 0, // Reserved for future
		.vertexBindingDescriptionCount      = bindingDescriptionCount,
		.pVertexBindingDescriptions         = bindingDescriptions,
		.vertexAttributeDescriptionCount    = attributeDescriptionCount,
		.pVertexAttributeDescriptions       = attributeDescriptions,
	};

	return vertexInputInfo;
}

internal VkPipelineInputAssemblyStateCreateInfo
fsvulkan_pipeline_input_assembly_create_info(VkPrimitiveTopology topology)
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = 
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext 					= nullptr,
		.flags 					= 0, // Reserved for future
		.topology               = topology,
		.primitiveRestartEnable = VK_FALSE,
	};
	
	return inputAssemblyState;
}


internal VkPipelineMultisampleStateCreateInfo
fsvulkan_pipeline_multisample_state_create_info(VkSampleCountFlagBits msaaSamples)
{
	VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples   = msaaSamples,
		.sampleShadingEnable    = VK_FALSE,
		.minSampleShading       = 1.0f,
		.pSampleMask            = nullptr,
		.alphaToCoverageEnable  = VK_FALSE,
		.alphaToOneEnable       = VK_FALSE,
	};

	return multisampling;
}

internal VkPipelineDepthStencilStateCreateInfo
fsvulkan_pipeline_depth_stencil_create_info (VkBool32 depthTestEnable, VkBool32 depthWriteEnable)
{
	VkPipelineDepthStencilStateCreateInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.depthTestEnable = depthTestEnable,
		.depthWriteEnable = depthWriteEnable,
		.depthCompareOp = VK_COMPARE_OP_LESS,

		// Note(Leo): These further limit depth range for this render pass
		.depthBoundsTestEnable = VK_FALSE,
		
		// Note(Leo): Configure stencil tests
		.stencilTestEnable = VK_FALSE,
		.front = {},
		.back = {},
		
		// TODO(Leo): Are these someting that should not have default value???
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	return info;
}

internal VkPipelineColorBlendStateCreateInfo
fsvulkan_pipeline_color_blend_state_create_info (u32 attachmentCount, VkPipelineColorBlendAttachmentState const * attachments)
{
	// Note Note(Leo): this comment is from before, it refers to what is now 'attachments' argument.
	// Note: This attachment is per framebuffer


	VkPipelineColorBlendStateCreateInfo info = 
	{
		.sType 				= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.pNext 				= nullptr,
		.flags 				= 0,
		.logicOpEnable		= VK_FALSE, // Note: enabling this disables per framebuffer/attachment operation,
		.logicOp 			= VK_LOGIC_OP_COPY,
		.attachmentCount 	= attachmentCount,
		.pAttachments 		= attachments,
		.blendConstants 	= {0,0,0,0},
	};

	return info;
}

internal VkPipelineDynamicStateCreateInfo
fsvulkan_pipeline_dynamic_state_create_info(u32 dynamicStateCount, VkDynamicState const * dynamicStates)
{
	VkPipelineDynamicStateCreateInfo createInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount  = dynamicStateCount,
		.pDynamicStates     = dynamicStates,
	};

	return createInfo;
}

/*
Note(Leo): This is here for copying. I didint like a function for this, because there are so many parameters,
and then there would not be names so it is easier to mix those.

These are default values, so those that are not needed can just be left away (except sType, but that is needed anyway).

	VkGraphicsPipelineCreateInfo info =
	{
		.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.pNext 					= nullptr,
		.flags 					= 0,
		.stageCount 			= 0,
		.pStages 				= nullptr,
		.pVertexInputState 		= nullptr,
		.pInputAssemblyState 	= nullptr,
		.pTessellationState 	= nullptr,
		.pViewportState 		= nullptr,
		.pRasterizationState 	= nullptr,
		.pMultisampleState 		= nullptr,
		.pDepthStencilState 	= nullptr,
		.pColorBlendState 		= nullptr,
		.pDynamicState 			= nullptr,
		.layout 				= VK_NULL_HANDLE,
		.renderPass 			= VK_NULL_HANDLE,
		.subpass 				= 0,
		.basePipelineHandle 	= VK_NULL_HANDLE,
		.basePipelineIndex 		= 0,
	};
	
*/

//////////////////////////////////////////////////////////////////////////////////////

internal void fsvulkan_initialize_pipelines(VulkanContext & context, u32 targetTextureWidth, u32 targetTextureHeight)
{
	// Todo(Leo): Target texture sizes are not taken into consideration anywhere yet, they probably fall back
	// to swapchain sizes, but it is not this functions problem.

	context.skyGradientDescriptorSetLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 2);

	// Note(Leo): These are common to every pipeline, right??
	VkViewport viewport = {0, 0, (f32)targetTextureWidth, (f32)targetTextureHeight, 0, 1};
	VkRect2D scissor 	= {{0,0}, {targetTextureWidth, targetTextureHeight}};

	auto common_viewportState = vk_pipeline_viewport_state_create_info();
	{
		common_viewportState.viewportCount 	= 1;
		common_viewportState.pViewports 	= &viewport;
		common_viewportState.scissorCount 	= 1;
		common_viewportState.pScissors 		= &scissor;
	}


	/// GraphicsPipeline_normal
	log_graphics(1, "Initializing Normal Pipeline");
	{
		auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 3);

		VkDescriptorSetLayout descriptorSetLayouts[] =
		{
			context.cameraDescriptorSetLayout,
			materialLayout,
			context.modelDescriptorSetLayout,
			context.lightingDescriptorSetLayout,
			context.shadowMapTextureDescriptorSetLayout,
			context.skyGradientDescriptorSetLayout
		};
		// TODO(Leo): this was maybe stupid idea, this is material block, and we are going to use
		// textures for this probably
		VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(f32) };

		auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(	array_count(descriptorSetLayouts), descriptorSetLayouts,
																				1, &pushConstantRange);

		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_normal].pipelineLayout));

		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/vert.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/frag.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(normalBindingCount, fsvulkan_defaultVertexBinding,
																					normalAttributeCount, fsvulkan_defaultVertexAttributes);
		auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		// auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);

		// Todo(Leo): Cull mode is disabled while we develop marching cubes thing
		// auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
		auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_BACK_BIT);
		auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
		auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
		auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
		// auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);


		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,

			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,
			.pViewportState 		= &common_viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			
			.layout 				= context.pipelines[GraphicsPipeline_normal].pipelineLayout,
			.renderPass 			= context.sceneRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_normal].pipeline);

		context.pipelines[GraphicsPipeline_normal].descriptorSetLayout = materialLayout;
		context.pipelines[GraphicsPipeline_normal].textureCount 		= 3;

		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
	}

	/// GraphicsPipeline_triplanar
	log_graphics(1, "Initializing Triplanar Pipeline");
	{
		s32 textureCount = 1;
		auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, textureCount);

		VkDescriptorSetLayout descriptorSetLayouts[] =
		{
			context.cameraDescriptorSetLayout,
			materialLayout,
			context.modelDescriptorSetLayout,
			context.lightingDescriptorSetLayout,
			context.shadowMapTextureDescriptorSetLayout,
			context.skyGradientDescriptorSetLayout,
		};
		// TODO(Leo): this was maybe stupid idea, this is material block, and we are going to use
		// textures for this probably

		auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(array_count(descriptorSetLayouts), descriptorSetLayouts, 0, nullptr);

		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_triplanar].pipelineLayout));

		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/vert.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/triplanar_frag.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(normalBindingCount, fsvulkan_defaultVertexBinding,
																					normalAttributeCount, fsvulkan_defaultVertexAttributes);
		auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		// auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);

		// Todo(Leo): Cull mode is disabled while we develop marching cubes thing
		// auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);	
		auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_BACK_BIT);
		auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
		auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
		auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
		// auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);


		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,

			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,
			.pViewportState 		= &common_viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			
			.layout 				= context.pipelines[GraphicsPipeline_triplanar].pipelineLayout,
			.renderPass 			= context.sceneRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_triplanar].pipeline);

		context.pipelines[GraphicsPipeline_triplanar].descriptorSetLayout = materialLayout;
		context.pipelines[GraphicsPipeline_triplanar].textureCount 		= textureCount;

		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
	}

	/// GraphicsPipeline_animated
	log_graphics(1, "Initializing Animated Pipeline");
	{
		auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 3);

		VkDescriptorSetLayout descriptorSetLayouts[] =
		{
			context.cameraDescriptorSetLayout,
			materialLayout,
			context.modelDescriptorSetLayout,
			context.lightingDescriptorSetLayout,
			context.shadowMapTextureDescriptorSetLayout,
			context.skyGradientDescriptorSetLayout
		};
		// TODO(Leo): this was maybe stupid idea, this is material block, and we are going to use
		// textures for this probably
		VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(f32) };

		auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(	array_count(descriptorSetLayouts), descriptorSetLayouts,
																				1, &pushConstantRange);

		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_animated].pipelineLayout));

		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/animated_vert.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/frag.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(animatedBindingCount, fsvulkan_defaultVertexBinding,
																					animatedAttributeCount, fsvulkan_defaultVertexAttributes);
		auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		// auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);
		auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_BACK_BIT);
		auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
		auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
		auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
		// auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);


		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,

			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,
			.pViewportState 		= &common_viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			
			.layout 				= context.pipelines[GraphicsPipeline_animated].pipelineLayout,
			.renderPass 			= context.sceneRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_animated].pipeline);

		context.pipelines[GraphicsPipeline_animated].descriptorSetLayout = materialLayout;
		context.pipelines[GraphicsPipeline_animated].textureCount 		= 3;

		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
	}

	/// GraphicsPipeline_skybox
	log_graphics(1, "Initializing Skybox Pipeline");
	{
		constexpr s32 gradientTextureCount = 2;
		auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, gradientTextureCount);

		VkDescriptorSetLayout descriptorSetLayouts[] =
		{
			context.cameraDescriptorSetLayout,
			materialLayout,
			context.modelDescriptorSetLayout,
			context.lightingDescriptorSetLayout,
			context.shadowMapTextureDescriptorSetLayout,
			context.skyGradientDescriptorSetLayout
		};

		auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(array_count(descriptorSetLayouts), descriptorSetLayouts, 0, nullptr);
		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_skybox].pipelineLayout));

		// --------------------------------------------------------------------------------------------

		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/vert_sky.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/frag_sky.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		VkVertexInputBindingDescription vertexInputBinding 		= {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
		VkVertexInputAttributeDescription attributeDescription 	= {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0};

		auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(1, &vertexInputBinding,
																					1, &attributeDescription);
		auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		// auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);
		auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
		auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
		auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_FALSE, VK_FALSE);
		auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
		// auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,
			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,
			.pViewportState 		= &common_viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			.layout 				= context.pipelines[GraphicsPipeline_skybox].pipelineLayout,
			.renderPass 			= context.sceneRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_skybox].pipeline);

		context.pipelines[GraphicsPipeline_skybox].descriptorSetLayout = materialLayout;
		context.pipelines[GraphicsPipeline_skybox].textureCount 		= gradientTextureCount;

		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
	}

	/// GraphicsPipeline_screen_gui
	log_graphics(1, "Initializing Screen Gui Pipeline");
	{
		auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 1);

		// Note(Leo): 4 v2s for coordinates and uv, v4 for color
		VkPushConstantRange pushConstantRanges [] = 
		{ 
			{VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ScreenRect)},
			{VK_SHADER_STAGE_FRAGMENT_BIT, 64, sizeof(v4)},
		};

		auto pipelineLayoutCreateInfo 			= fsvulkan_pipeline_layout_create_info(1, &materialLayout, array_count(pushConstantRanges), pushConstantRanges);
		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_screen_gui].pipelineLayout));

		// ---------------------------------------------------------------------------------------------------------------------------

		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/gui_vert.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/gui_frag.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		auto vertexInputState 		= fsvulkan_pipeline_vertex_input_state_create_info	(	0, nullptr, 0, nullptr);
		auto inputAssemblyState 	= fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		// auto viewportState 			= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);
		auto rasterizationState 	= fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
		auto multisampleState 		= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
		auto depthStencilState 		= fsvulkan_pipeline_depth_stencil_create_info		(VK_FALSE, VK_FALSE);

		VkPipelineColorBlendAttachmentState colorBlendAttachment = 
		{
			.blendEnable            = VK_TRUE,
			.srcColorBlendFactor    = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp           = VK_BLEND_OP_ADD,

			.srcAlphaBlendFactor    = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstAlphaBlendFactor    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,

			// Note(Leo): For some reason this was subtract, I can't remember why, but it seemed to have worked like add.
			// .alphaBlendOp           = VK_BLEND_OP_SUBTRACT, 
			.alphaBlendOp           = VK_BLEND_OP_ADD, 

			.colorWriteMask         = fsvulkan_all_colors,
		};
		auto colorBlendState 		= fsvulkan_pipeline_color_blend_state_create_info(1, &colorBlendAttachment);

		// auto dynamicState 			= fsvulkan_pipeline_dynamic_state_create_info(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,
			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,
			.pViewportState 		= &common_viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			.layout 				= context.pipelines[GraphicsPipeline_screen_gui].pipelineLayout,
			.renderPass 			= context.sceneRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_screen_gui].pipeline);

		context.pipelines[GraphicsPipeline_screen_gui].descriptorSetLayout = materialLayout;
		context.pipelines[GraphicsPipeline_screen_gui].textureCount 		= 1;

		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
	}

	/// GraphicsPipeline_leaves
	log_graphics(1, "Initializing Leaves Pipeline");
	{
		auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 1);

		VkDescriptorSetLayout descriptorSetLayouts[] =
		{
			context.cameraDescriptorSetLayout,
			context.lightingDescriptorSetLayout,
			context.shadowMapTextureDescriptorSetLayout,
			materialLayout,	
			context.skyGradientDescriptorSetLayout,
		};

		VkPushConstantRange pushConstantRange = {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(v3)};

		auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(array_count(descriptorSetLayouts), descriptorSetLayouts, 1, &pushConstantRange);
		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_leaves].pipelineLayout));

		VkVertexInputBindingDescription vertexBindingDescription = { 0, sizeof(m44), VK_VERTEX_INPUT_RATE_INSTANCE };

		// Note(Leo): Input is 4x4 matrix, it is described as 4 4d vectors
		VkVertexInputAttributeDescription vertexAttributeDescriptions [4] =
		{
			{ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 * sizeof(v4)},
			{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 1 * sizeof(v4)},
			{ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sizeof(v4)},
			{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 3 * sizeof(v4)},
		};
		
		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/leaves_vert.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/leaves_frag.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(1, & vertexBindingDescription,
																					array_count(vertexAttributeDescriptions), vertexAttributeDescriptions);
		auto inputAssemblyState	= fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
		// auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);
		auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
		auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
		auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
		auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
		// auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,
			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,
			.pViewportState 		= &common_viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			.layout 				= context.pipelines[GraphicsPipeline_leaves].pipelineLayout,
			.renderPass 			= context.sceneRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_leaves].pipeline);

		context.pipelines[GraphicsPipeline_leaves].descriptorSetLayout = materialLayout;
		context.pipelines[GraphicsPipeline_leaves].textureCount 		= 1;

		// Note(Leo): Always remember to destroy these :)
		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);	
	}

	/// GraphicsPipeline_water
	log_graphics(1, "Initializing Water Pipeline");
	{
		auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 3);

		VkDescriptorSetLayout descriptorSetLayouts[] =
		{
			context.cameraDescriptorSetLayout,
			materialLayout,
			context.modelDescriptorSetLayout,
			context.lightingDescriptorSetLayout,
			context.shadowMapTextureDescriptorSetLayout,
			context.skyGradientDescriptorSetLayout,
		};
		// TODO(Leo): this was maybe stupid idea, this is material block, and we are going to use
		// textures for this probably
		VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(f32) };

		auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(	array_count(descriptorSetLayouts), descriptorSetLayouts,
																				1, &pushConstantRange);

		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_water].pipelineLayout));

		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/vert.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/frag.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(normalBindingCount, fsvulkan_defaultVertexBinding,
																					normalAttributeCount, fsvulkan_defaultVertexAttributes);
		auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		// auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);
		auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_BACK_BIT);
		auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
		auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_FALSE);

		VkPipelineColorBlendAttachmentState colorBlendAttachment = 
		{
			.blendEnable            = VK_TRUE,
			.srcColorBlendFactor    = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor    = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp           = VK_BLEND_OP_ADD,

			.srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE, //VK_BLEND_FACTOR_SRC_ALPHA,
			.dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO, //VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.alphaBlendOp           = VK_BLEND_OP_ADD, 

			.colorWriteMask         = fsvulkan_all_colors
		};

		auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &colorBlendAttachment);
		// auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);


		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,

			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,
			.pViewportState 		= &common_viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			
			.layout 				= context.pipelines[GraphicsPipeline_water].pipelineLayout,
			.renderPass 			= context.sceneRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GraphicsPipeline_water].pipeline);

		context.pipelines[GraphicsPipeline_water].descriptorSetLayout = materialLayout;
		context.pipelines[GraphicsPipeline_water].textureCount 		= 3;

		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
	}

	/// INTERNAL LINE PIPELINE
	log_graphics(1, "Initializing Line Pipeline");
	{
		/*
		Document(Leo): Lines are drawn by building line lists on demand. List
		are build on uniform/vertex buffer dynamically.
		*/

		VkVertexInputBindingDescription vertexBindingDescription = 
		{
			.binding 	= 0,
			.stride 	= 2 * sizeof(v3),
			.inputRate 	= VK_VERTEX_INPUT_RATE_VERTEX,
		};

		VkVertexInputAttributeDescription vertexAttributeDescriptions [] =
		{
			{0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
			{1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(v3)},
		};

		// Note(Leo): Always remember to destroy these :)
		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/line_vert.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/line_frag.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		auto vertexInputState = fsvulkan_pipeline_vertex_input_state_create_info	(1, &vertexBindingDescription,
																					array_count(vertexAttributeDescriptions), vertexAttributeDescriptions);
		auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
		// auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);
		auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
		auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
		auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
		auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
		// auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);

		auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(1, &context.cameraDescriptorSetLayout, 0, nullptr);

		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.linePipelineLayout));

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,
			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,
			.pViewportState 		= &common_viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			.layout 				= context.linePipelineLayout,
			.renderPass 			= context.sceneRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.linePipeline);

		// Note(Leo): Always remember to destroy these :)
		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);	
	}

	/// INTERNAL HDR TONEMAP PIPELINE
	log_graphics(1, "Initializing Postprocess Pipeline");
	{
		auto materialLayout 			= fsvulkan_make_material_descriptor_set_layout(context.device, 1);

		VkDescriptorSetLayout layouts [] = 
		{
			materialLayout,
		};

		VkPushConstantRange pushConstantRange = {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(FSVulkanHdrSettings)};

		auto pipelineLayoutCreateInfo 	= fsvulkan_pipeline_layout_create_info(array_count(layouts), layouts, 1, &pushConstantRange);
		VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.screenSpacePipelineLayout));

		// ---------------------------------------------------------------------------------------------------------------------------

		VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, "shaders/hdr_vert.spv");
		VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/hdr_frag.spv");

		VkPipelineShaderStageCreateInfo shaderStages [] =
		{
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
			fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
		};

		auto vertexInputState 		= fsvulkan_pipeline_vertex_input_state_create_info	(0, nullptr, 0, nullptr);
		auto inputAssemblyState 	= fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
		// auto viewportState 			= fsvulkan_pipeline_viewport_state_create_info		(1, &common_viewport, 1, &common_scissor);

		VkViewport viewport = {0, 0, (f32)context.swapchainExtent.width, (f32)context.swapchainExtent.height, 0, 1};
		VkRect2D scissor 	= {{0,0}, {context.swapchainExtent.width, context.swapchainExtent.height}};

		auto viewportState = vk_pipeline_viewport_state_create_info();
		{
			viewportState.viewportCount = 1;
			viewportState.pViewports 	= &viewport;
			viewportState.scissorCount 	= 1;
			viewportState.pScissors 	= &scissor;
		}


		auto rasterizationState 	= fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
		auto multisampleState 		= fsvulkan_pipeline_multisample_state_create_info	(VK_SAMPLE_COUNT_1_BIT);
		auto depthStencilState 		= fsvulkan_pipeline_depth_stencil_create_info		(VK_FALSE, VK_FALSE);
		auto colorBlendState 		= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
		// auto dynamicState 			= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);

		VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
		{
			.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount 			= array_count(shaderStages),
			.pStages 				= shaderStages,
			.pVertexInputState 		= &vertexInputState,
			.pInputAssemblyState 	= &inputAssemblyState,

			// Todo(Leo): This pipeline may not actually use common viewport, look into that
			.pViewportState 		= &viewportState,
			.pRasterizationState 	= &rasterizationState,
			.pMultisampleState 		= &multisampleState,
			.pDepthStencilState 	= &depthStencilState,
			.pColorBlendState 		= &colorBlendState,
			// .pDynamicState 			= &dynamicState,
			.layout 				= context.screenSpacePipelineLayout,
			.renderPass 			= context.screenSpaceRenderPass,
		};

		vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.screenSpacePipeline);

		context.screenSpaceDescriptorSetLayout = materialLayout;

		vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);


		/// Make resolve image sampleable in pass through pipeline
		// Todo(Leo): These depend on pass through layouts etc, but otherwise should not be here...
		{
			for(s32 i = 0; i < VIRTUAL_FRAME_COUNT; ++i)
			{	
				auto allocateInfo = fsvulkan_descriptor_set_allocate_info(	context.drawingResourceDescriptorPool,
																			1,
																			&context.screenSpaceDescriptorSetLayout);
				VULKAN_CHECK(vkAllocateDescriptorSets(	context.device,
														&allocateInfo,
														&context.sceneRenderTargetSamplerDescriptors[i]));


				// Todo(Leo): specify fields maybe grrr
				VkDescriptorImageInfo info = 
				{
					context.linearRepeatSampler,
					// context.nearestRepeatSampler,
					context.sceneRenderTargets[i].resolveAttachment,
					VK_IMAGE_LAYOUT_GENERAL,
					// VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					// VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
				};

				VkWriteDescriptorSet write = 
				{
					.sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet             = context.sceneRenderTargetSamplerDescriptors[i],
					.dstBinding         = 0,
					.dstArrayElement    = 0,
					.descriptorCount    = 1,
					.descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo         = &info,
				};
				vkUpdateDescriptorSets(context.device, 1, &write, 0, nullptr);
			}
		}
	}
}

internal void fsvulkan_initialize_shadow_pipeline(VulkanContext & context)
{
	VkShaderModule vertexShaderModule = fsvulkan_make_shader_module(context.device, "shaders/shadow_vert.spv");

	auto vertexShaderStage 	= fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main");


	constexpr VkVertexInputAttributeDescription attributeDescriptions [] =
	{
		{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},

		{5, 1, VK_FORMAT_R32G32B32A32_UINT, offsetof(VertexSkinData, boneIndices)},
		{6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexSkinData, boneWeights)},
	};

	auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info(animatedBindingCount, fsvulkan_defaultVertexBinding,
																				array_count(attributeDescriptions), attributeDescriptions);
	auto inputAssemblyState	= fsvulkan_pipeline_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	VkViewport viewport 	= {0, 0, (f32)context.shadowTextureWidth, (f32)context.shadowTextureHeight, 0, 1};
	VkRect2D scissor 		= {{0,0}, {context.shadowTextureWidth, context.shadowTextureHeight}};
	auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info(1, &viewport, 1, &scissor);

	auto rasterizationState	= fsvulkan_pipeline_rasterization_state_create_info(VK_CULL_MODE_NONE);
	auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info(VK_SAMPLE_COUNT_1_BIT);
	auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info(0, nullptr);

	auto depthStencilState 				= fsvulkan_pipeline_depth_stencil_create_info(VK_TRUE, VK_TRUE);
	depthStencilState.depthCompareOp 	= VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.minDepthBounds	= 0.0f;
	depthStencilState.maxDepthBounds	= 1.0f;

	VkDescriptorSetLayout setLayouts [] =
	{
		context.cameraDescriptorSetLayout,
		context.modelDescriptorSetLayout,
	};

	auto pipelineLayoutInfo = fsvulkan_pipeline_layout_create_info(array_count(setLayouts), setLayouts, 0, nullptr);
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutInfo, nullptr, &context.shadowPipelineLayout));

	VkGraphicsPipelineCreateInfo pipelineCreateInfo =
	{
		.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount 			= 1,
		.pStages 				= &vertexShaderStage,
		.pVertexInputState 		= &vertexInputState,
		.pInputAssemblyState 	= &inputAssemblyState,
		.pViewportState 		= &viewportState,
		.pRasterizationState 	= &rasterizationState,
		.pMultisampleState 		= &multisampleState,
		.pDepthStencilState 	= &depthStencilState,
		.pColorBlendState 		= &colorBlendState,
		.layout 				= context.shadowPipelineLayout,
		.renderPass 			= context.shadowRenderPass,
	};

	VULKAN_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &context.shadowPipeline));

	// Note(Leo): Always remember to destroy these :)
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
}

internal void fsvulkan_initialize_leaves_shadow_pipeline(VulkanContext & context)
{
	VkDescriptorSetLayout textureMaskLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 1);
	context.leavesShadowMaskDescriptorSetLayout = textureMaskLayout;

	VkDescriptorSetLayout descriptorSetLayouts [] =
	{
		context.cameraDescriptorSetLayout,
		textureMaskLayout,
	};

	auto pipelineLayoutInfo = fsvulkan_pipeline_layout_create_info(array_count(descriptorSetLayouts), descriptorSetLayouts, 0, nullptr);
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutInfo, nullptr, &context.leavesShadowPipelineLayout));

	/// ----------------------------------------------------------------------------------------------

	VkShaderModule vertexShaderModule	= fsvulkan_make_shader_module(context.device, "shaders/leaves_shadow_vert.spv");
	VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, "shaders/leaves_shadow_frag.spv");

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
	};

	// Note(Leo): Input is 4x4 matrix, it is described as 4 4d vectors
	VkVertexInputBindingDescription vertexBindingDescription = { 0, sizeof(m44), VK_VERTEX_INPUT_RATE_INSTANCE };
	VkVertexInputAttributeDescription vertexAttributeDescriptions [4] =
	{
		{ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 * sizeof(v4)},
		{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 1 * sizeof(v4)},
		{ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sizeof(v4)},
		{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 3 * sizeof(v4)},
	};
	auto vertexInputInfo 	= fsvulkan_pipeline_vertex_input_state_create_info								(1, &vertexBindingDescription,	
																											array_count(vertexAttributeDescriptions),
																											vertexAttributeDescriptions);
	auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info									(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

	VkViewport viewport 	= {0, 0, (f32)context.shadowTextureWidth, (f32)context.shadowTextureHeight, 0, 1};
	VkRect2D scissor 		= {{0,0}, {context.shadowTextureWidth, context.shadowTextureHeight}};
	auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info									(1, &viewport, 1, &scissor);

	auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info								(VK_CULL_MODE_NONE);
	auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info								(VK_SAMPLE_COUNT_1_BIT);
	auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info									(VK_TRUE, VK_TRUE);
	auto colorBlendingState	= fsvulkan_pipeline_color_blend_state_create_info								(0, nullptr);


	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount             = array_count(shaderStages),
		.pStages                = shaderStages,

		.pVertexInputState      = &vertexInputInfo,
		.pInputAssemblyState    = &inputAssemblyState,
		.pViewportState         = &viewportState,
		.pRasterizationState    = &rasterizationState,
		.pMultisampleState      = &multisampleState,
		.pDepthStencilState     = &depthStencilState,
		.pColorBlendState       = &colorBlendingState,

		.layout                 = context.leavesShadowPipelineLayout,
		.renderPass             = context.shadowRenderPass,
	};

	VULKAN_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &context.leavesShadowPipeline));

	// Note(Leo): Always remember to destroy these :)
	vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
}

internal void fsvulkan_cleanup_pipelines(VulkanContext * context)
{
	VkDevice device = context->device;

	for (s32 i = 0; i < GraphicsPipelineCount; ++i)
	{
		vkDestroyDescriptorSetLayout(device, context->pipelines[i].descriptorSetLayout, nullptr);
		vkDestroyPipelineLayout(device, context->pipelines[i].pipelineLayout, nullptr);
		vkDestroyPipeline(device, context->pipelines[i].pipeline, nullptr);				
	}

	vkDestroyPipelineLayout(device, context->linePipelineLayout, nullptr);
	vkDestroyPipeline(device, context->linePipeline, nullptr);

	vkDestroyDescriptorSetLayout(device, context->screenSpaceDescriptorSetLayout, nullptr);
	vkDestroyPipelineLayout(device, context->screenSpacePipelineLayout, nullptr);
	vkDestroyPipeline(device, context->screenSpacePipeline, nullptr);

	vkDestroyDescriptorSetLayout(device, context->skyGradientDescriptorSetLayout, nullptr);
}