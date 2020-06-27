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


internal VkShaderModule fsvulkan_make_shader_module(VkDevice device, BinaryAsset code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType 					= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize 				= code.size();
	createInfo.pCode 					= reinterpret_cast<u32*>(code.data());

	VkShaderModule result;
	VULKAN_CHECK(vkCreateShaderModule (device, &createInfo, nullptr, &result));

	return result;
}


// *************************************************************************
/// GOOD INITIALIZERS
// Note(Leo): Only expose from these functions properties we are interested to change

constexpr VkVertexInputBindingDescription fsvulkan_defaultVertexBinding = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };

constexpr VkVertexInputAttributeDescription fsvulkan_defaultVertexAttributes [] =
{
	{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
	{1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
	{2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
	{3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)},
	{4, 0, VK_FORMAT_R32G32B32A32_UINT, offsetof(Vertex, boneIndices)},
	{5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, boneWeights)},
	{6, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)},
	{7, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, biTangent)},
};


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

internal FSVulkanPipeline fsvulkan_make_pipeline_WITH_EVEN_MORE_ARGUMENTS(
	VulkanContext * context,

	u32 											stageCount,
	VkPipelineShaderStageCreateInfo const * 		stages,

	VkPipelineVertexInputStateCreateInfo const * 	vertexInputState,
	VkPipelineInputAssemblyStateCreateInfo const * 	inputAssemblyState,
	VkPipelineViewportStateCreateInfo const *		viewportState,
	VkPipelineRasterizationStateCreateInfo const * 	rasterizationState,
	VkPipelineMultisampleStateCreateInfo const *	multisampleState,
	VkPipelineDepthStencilStateCreateInfo const * 	depthStencilState,
	VkPipelineColorBlendStateCreateInfo const * 	colorBlendState,
	VkPipelineDynamicStateCreateInfo const * 		dynamicState,

	VkPipelineLayout layout)
{		

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount             = stageCount,
		.pStages                = stages,

		// Note(Leo): Are these The Fixed-Function Stages?
		.pVertexInputState      = vertexInputState,
		.pInputAssemblyState    = inputAssemblyState,
		.pViewportState         = viewportState,
		.pRasterizationState    = rasterizationState,
		.pMultisampleState      = multisampleState,   
		.pDepthStencilState     = depthStencilState,
		.pColorBlendState       = colorBlendState,
		.pDynamicState          = dynamicState,

		.layout                 = layout,
		.renderPass             = context->drawingResources.renderPass,
		.subpass                = 0,

		// Note(Leo): These are for cheaper re-use of pipelines, not used right now.
		// Study(Leo): Like inheritance??
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = 0,
	};

	VkPipeline pipeline;
	VULKAN_CHECK(vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));


	FSVulkanPipeline result = 
	{
		.pipeline               = pipeline,
		.pipelineLayout         = layout,
		// .descriptorSetLayout    = materialLayout,
		// .textureCount           = options.textureCount
	};
	return result;
}

//////////////////////////////////////////////////////////////////////////////////////

internal void fsvulkan_initialize_normal_pipeline(VulkanContext & context)
{
	auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 3);

	VkDescriptorSetLayout descriptorSetLayouts[] =
	{
		context.descriptorSetLayouts.camera,
		materialLayout,
		context.descriptorSetLayouts.model,
		context.descriptorSetLayouts.lighting,
		context.descriptorSetLayouts.shadowMap,
	};
	// TODO(Leo): this was maybe stupid idea, this is material block, and we are going to use
	// textures for this probably
	VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(f32) };

	auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(	array_count(descriptorSetLayouts), descriptorSetLayouts,
																			1, &pushConstantRange);

	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_NORMAL].pipelineLayout));

	VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/vert.spv"));
	VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/frag.spv"));

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
	};

	auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(1, &fsvulkan_defaultVertexBinding,
																				array_count(fsvulkan_defaultVertexAttributes), fsvulkan_defaultVertexAttributes);
	auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &fsvulkan_zero_viewport, 1, &fsvulkan_zero_scissor);
	auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_BACK_BIT);
	auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
	auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
	auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
	auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);


	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
	{
		.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount 			= array_count(shaderStages),
		.pStages 				= shaderStages,

		.pVertexInputState 		= &vertexInputState,
		.pInputAssemblyState 	= &inputAssemblyState,
		.pViewportState 		= &viewportState,
		.pRasterizationState 	= &rasterizationState,
		.pMultisampleState 		= &multisampleState,
		.pDepthStencilState 	= &depthStencilState,
		.pColorBlendState 		= &colorBlendState,
		.pDynamicState 			= &dynamicState,
		
		.layout 				= context.pipelines[GRAPHICS_PIPELINE_NORMAL].pipelineLayout,
		.renderPass 			= context.drawingResources.renderPass,
	};

	vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_NORMAL].pipeline);

	context.pipelines[GRAPHICS_PIPELINE_NORMAL].descriptorSetLayout = materialLayout;
	context.pipelines[GRAPHICS_PIPELINE_NORMAL].textureCount 		= 3;

	vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
}

internal void fsvulkan_initialize_animated_pipeline(VulkanContext & context)
{
	auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 3);

	VkDescriptorSetLayout descriptorSetLayouts[] =
	{
		context.descriptorSetLayouts.camera,
		materialLayout,
		context.descriptorSetLayouts.model,
		context.descriptorSetLayouts.lighting,
		context.descriptorSetLayouts.shadowMap,
	};
	// TODO(Leo): this was maybe stupid idea, this is material block, and we are going to use
	// textures for this probably
	VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(f32) };

	auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(	array_count(descriptorSetLayouts), descriptorSetLayouts,
																			1, &pushConstantRange);

	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_ANIMATED].pipelineLayout));

	VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/animated_vert.spv"));
	VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/frag.spv"));

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
	};

	auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(1, &fsvulkan_defaultVertexBinding,
																				array_count(fsvulkan_defaultVertexAttributes), fsvulkan_defaultVertexAttributes);
	auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &fsvulkan_zero_viewport, 1, &fsvulkan_zero_scissor);
	auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_BACK_BIT);
	auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
	auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
	auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
	auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);


	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
	{
		.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount 			= array_count(shaderStages),
		.pStages 				= shaderStages,

		.pVertexInputState 		= &vertexInputState,
		.pInputAssemblyState 	= &inputAssemblyState,
		.pViewportState 		= &viewportState,
		.pRasterizationState 	= &rasterizationState,
		.pMultisampleState 		= &multisampleState,
		.pDepthStencilState 	= &depthStencilState,
		.pColorBlendState 		= &colorBlendState,
		.pDynamicState 			= &dynamicState,
		
		.layout 				= context.pipelines[GRAPHICS_PIPELINE_ANIMATED].pipelineLayout,
		.renderPass 			= context.drawingResources.renderPass,
	};

	vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_ANIMATED].pipeline);

	context.pipelines[GRAPHICS_PIPELINE_ANIMATED].descriptorSetLayout = materialLayout;
	context.pipelines[GRAPHICS_PIPELINE_ANIMATED].textureCount 		= 3;

	vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
}

internal void fsvulkan_initialize_skybox_pipeline(VulkanContext & context)
{
	auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 0);

	VkDescriptorSetLayout descriptorSetLayouts[] =
	{
		context.descriptorSetLayouts.camera,
		materialLayout,
		context.descriptorSetLayouts.model,
		context.descriptorSetLayouts.lighting,
		context.descriptorSetLayouts.shadowMap,
	};

	auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(array_count(descriptorSetLayouts), descriptorSetLayouts, 0, nullptr);
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_SKYBOX].pipelineLayout));

	// --------------------------------------------------------------------------------------------

	VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/vert_sky.spv"));
	VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/frag_sky.spv"));

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
	};

	auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(1, &fsvulkan_defaultVertexBinding,
																				array_count(fsvulkan_defaultVertexAttributes), fsvulkan_defaultVertexAttributes);
	auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &fsvulkan_zero_viewport, 1, &fsvulkan_zero_scissor);
	auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
	auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
	auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_FALSE, VK_FALSE);
	auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
	auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
	{
		.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount 			= array_count(shaderStages),
		.pStages 				= shaderStages,
		.pVertexInputState 		= &vertexInputState,
		.pInputAssemblyState 	= &inputAssemblyState,
		.pViewportState 		= &viewportState,
		.pRasterizationState 	= &rasterizationState,
		.pMultisampleState 		= &multisampleState,
		.pDepthStencilState 	= &depthStencilState,
		.pColorBlendState 		= &colorBlendState,
		.pDynamicState 			= &dynamicState,
		.layout 				= context.pipelines[GRAPHICS_PIPELINE_SKYBOX].pipelineLayout,
		.renderPass 			= context.drawingResources.renderPass,
	};

	vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_SKYBOX].pipeline);

	context.pipelines[GRAPHICS_PIPELINE_SKYBOX].descriptorSetLayout = materialLayout;
	context.pipelines[GRAPHICS_PIPELINE_SKYBOX].textureCount 		= 0;

	vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
}

internal void fsvulkan_initialize_screen_gui_pipeline(VulkanContext & context)
{
	auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 1);

	// Note(Leo): 4 v2s for coordinates and uv, v4 for color
	u32 pushConstantSize 					= sizeof(v2) * 4 + sizeof(v4);
	VkPushConstantRange pushConstantRange 	= { VK_SHADER_STAGE_VERTEX_BIT, 0, pushConstantSize };

	auto pipelineLayoutCreateInfo 			= fsvulkan_pipeline_layout_create_info(1, &materialLayout, 1, &pushConstantRange);
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].pipelineLayout));

	// ---------------------------------------------------------------------------------------------------------------------------

	VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/gui_vert3.spv"));
	VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/gui_frag2.spv"));

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
	};

	auto vertexInputState 		= fsvulkan_pipeline_vertex_input_state_create_info	(	0, nullptr, 0, nullptr);
	auto inputAssemblyState 	= fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
	auto viewportState 			= fsvulkan_pipeline_viewport_state_create_info		(1, &fsvulkan_zero_viewport, 1, &fsvulkan_zero_scissor);
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
		.alphaBlendOp           = VK_BLEND_OP_SUBTRACT, 

		.colorWriteMask         = VK_COLOR_COMPONENT_R_BIT 
									| VK_COLOR_COMPONENT_G_BIT 
									| VK_COLOR_COMPONENT_B_BIT 
									| VK_COLOR_COMPONENT_A_BIT,
	};
	auto colorBlendState 		= fsvulkan_pipeline_color_blend_state_create_info(1, &colorBlendAttachment);

	auto dynamicState 			= fsvulkan_pipeline_dynamic_state_create_info(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
	{
		.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount 			= array_count(shaderStages),
		.pStages 				= shaderStages,
		.pVertexInputState 		= &vertexInputState,
		.pInputAssemblyState 	= &inputAssemblyState,
		.pViewportState 		= &viewportState,
		.pRasterizationState 	= &rasterizationState,
		.pMultisampleState 		= &multisampleState,
		.pDepthStencilState 	= &depthStencilState,
		.pColorBlendState 		= &colorBlendState,
		.pDynamicState 			= &dynamicState,
		.layout 				= context.pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].pipelineLayout,
		.renderPass 			= context.drawingResources.renderPass,
	};

	vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].pipeline);

	context.pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].descriptorSetLayout = materialLayout;
	context.pipelines[GRAPHICS_PIPELINE_SCREEN_GUI].textureCount 		= 1;

	vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
}

internal void fsvulkan_initialize_leaves_pipeline(VulkanContext & context)
{
	VkVertexInputBindingDescription vertexBindingDescription = { 0, sizeof(m44), VK_VERTEX_INPUT_RATE_INSTANCE };

	// Note(Leo): Input is 4x4 matrix, it is described as 4 4d vectors
	VkVertexInputAttributeDescription vertexAttributeDescriptions [4] =
	{
		{ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 * sizeof(v4)},
		{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 1 * sizeof(v4)},
		{ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sizeof(v4)},
		{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 3 * sizeof(v4)},
	};
	
	auto materialLayout = fsvulkan_make_material_descriptor_set_layout(context.device, 0);

	VkDescriptorSetLayout descriptorSetLayouts[] =
	{
		context.descriptorSetLayouts.camera,
		materialLayout,
		context.descriptorSetLayouts.model,
		context.descriptorSetLayouts.lighting,
		context.descriptorSetLayouts.shadowMap,
	};

	auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(array_count(descriptorSetLayouts), descriptorSetLayouts, 0, nullptr);
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_LEAVES].pipelineLayout));

	VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/leaves_vert.spv"));
	VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/leaves_frag.spv"));

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
	};

	auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info	(1, & vertexBindingDescription,
																				array_count(vertexAttributeDescriptions), vertexAttributeDescriptions);
	auto inputAssemblyState	= fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);
	auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &fsvulkan_zero_viewport, 1, &fsvulkan_zero_scissor);
	auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
	auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
	auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
	auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
	auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);


	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
	{
		.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount 			= array_count(shaderStages),
		.pStages 				= shaderStages,
		.pVertexInputState 		= &vertexInputState,
		.pInputAssemblyState 	= &inputAssemblyState,
		.pViewportState 		= &viewportState,
		.pRasterizationState 	= &rasterizationState,
		.pMultisampleState 		= &multisampleState,
		.pDepthStencilState 	= &depthStencilState,
		.pColorBlendState 		= &colorBlendState,
		.pDynamicState 			= &dynamicState,
		.layout 				= context.pipelines[GRAPHICS_PIPELINE_LEAVES].pipelineLayout,
		.renderPass 			= context.drawingResources.renderPass,
	};

	vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.pipelines[GRAPHICS_PIPELINE_LEAVES].pipeline);

	context.pipelines[GRAPHICS_PIPELINE_LEAVES].descriptorSetLayout = materialLayout;
	context.pipelines[GRAPHICS_PIPELINE_LEAVES].textureCount 		= 1;

	// Note(Leo): Always remember to destroy these :)
	vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);	
}

internal void fsvulkan_initialize_line_pipeline(VulkanContext & context)
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
	VkShaderModule vertexShaderModule 	= fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/line_vert.spv"));
	VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/line_frag.spv"));

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main"),
		fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule, "main"),
	};

	auto vertexInputState = fsvulkan_pipeline_vertex_input_state_create_info	(1, &vertexBindingDescription,
																				array_count(vertexAttributeDescriptions), vertexAttributeDescriptions);
	auto inputAssemblyState = fsvulkan_pipeline_input_assembly_create_info		(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
	auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info		(1, &fsvulkan_zero_viewport, 1, &fsvulkan_zero_scissor);
	auto rasterizationState = fsvulkan_pipeline_rasterization_state_create_info	(VK_CULL_MODE_NONE);
	auto multisampleState 	= fsvulkan_pipeline_multisample_state_create_info	(context.msaaSamples);
	auto depthStencilState 	= fsvulkan_pipeline_depth_stencil_create_info		(VK_TRUE, VK_TRUE);
	auto colorBlendState 	= fsvulkan_pipeline_color_blend_state_create_info	(1, &fsvulkan_default_pipeline_color_blend_attachment_state);
	auto dynamicState 		= fsvulkan_pipeline_dynamic_state_create_info		(array_count(fsvulkan_default_dynamic_states), fsvulkan_default_dynamic_states);

	auto pipelineLayoutCreateInfo = fsvulkan_pipeline_layout_create_info(1, &context.descriptorSetLayouts.camera, 0, nullptr);

	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutCreateInfo, nullptr, &context.linePipelineLayout));

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo =
	{
		.sType 					= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount 			= array_count(shaderStages),
		.pStages 				= shaderStages,
		.pVertexInputState 		= &vertexInputState,
		.pInputAssemblyState 	= &inputAssemblyState,
		.pViewportState 		= &viewportState,
		.pRasterizationState 	= &rasterizationState,
		.pMultisampleState 		= &multisampleState,
		.pDepthStencilState 	= &depthStencilState,
		.pColorBlendState 		= &colorBlendState,
		.pDynamicState 			= &dynamicState,
		.layout 				= context.linePipelineLayout,
		.renderPass 			= context.drawingResources.renderPass,
	};

	vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &graphicsPipelineCreateInfo, nullptr, &context.linePipeline);

	// Note(Leo): Always remember to destroy these :)
	vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);	
}

internal void fsvulkan_initialize_shadow_pipeline(VulkanContext & context)
{
	VkShaderModule vertexShaderModule = fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/shadow_vert.spv"));

	auto vertexShaderStage 	= fsvulkan_pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule, "main");

	auto vertexInputState 	= fsvulkan_pipeline_vertex_input_state_create_info(1, &fsvulkan_defaultVertexBinding,
																			array_count(fsvulkan_defaultVertexAttributes), fsvulkan_defaultVertexAttributes);
	auto inputAssemblyState	= fsvulkan_pipeline_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	VkViewport viewport 	= {0, 0, (f32)context.shadowPass.width, (f32)context.shadowPass.height, 0, 1};
	VkRect2D scissor 		= {{0,0}, {context.shadowPass.width, context.shadowPass.height}};
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
		context.descriptorSetLayouts.camera,
		context.descriptorSetLayouts.model,
	};

	auto pipelineLayoutInfo = fsvulkan_pipeline_layout_create_info(array_count(setLayouts), setLayouts, 0, nullptr);
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutInfo, nullptr, &context.shadowPass.layout));

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
		.layout 				= context.shadowPass.layout,
		.renderPass 			= context.shadowPass.renderPass,
	};

	VULKAN_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &context.shadowPass.pipeline));

	// Note(Leo): Always remember to destroy these :)
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
}

internal void fsvulkan_initialize_leaves_shadow_pipeline(VulkanContext & context)
{
	auto pipelineLayoutInfo = fsvulkan_pipeline_layout_create_info(1, &context.descriptorSetLayouts.camera, 0, nullptr);
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutInfo, nullptr, &context.leavesShadowPipelineLayout));

	/// ----------------------------------------------------------------------------------------------

	VkShaderModule vertexShaderModule	= fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/leaves_shadow_vert.spv"));
	VkShaderModule fragmentShaderModule = fsvulkan_make_shader_module(context.device, BAD_read_binary_file("assets/shaders/leaves_shadow_frag.spv"));

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

	VkViewport viewport 	= {0, 0, (f32)context.shadowPass.width, (f32)context.shadowPass.height, 0, 1};
	VkRect2D scissor 		= {{0,0}, {context.shadowPass.width, context.shadowPass.height}};
	auto viewportState 		= fsvulkan_pipeline_viewport_state_create_info									(1, &viewport, 1, &scissor);

	auto rasterizationState 		= fsvulkan_pipeline_rasterization_state_create_info								(VK_CULL_MODE_NONE);
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
		.renderPass             = context.shadowPass.renderPass,
	};

	VULKAN_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &context.leavesShadowPipeline));

	// Note(Leo): Always remember to destroy these :)
	vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);
}

