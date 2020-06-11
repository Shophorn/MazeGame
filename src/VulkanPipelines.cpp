/*=============================================================================
Leo Tamminen
shophorn @ internet

Vulkan pipeline things separated to a different place, nothing too sensible yet.
Todo(Leo): These totally should be combined more/somehow
=============================================================================*/

struct PipelineOptions
{
	s32 textureCount 		= 0;
	u32	pushConstantSize 	= 0;
	f32 lineWidth 			= 1.0f;

	VkPrimitiveTopology primitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	VkCullModeFlags cullModeFlags = VK_CULL_MODE_BACK_BIT;

	bool8 enableDepth 			= true;
	bool8 clampDepth 			= false;
	bool8 useVertexInput		= true;
	bool8 useSceneLayoutSet 	= true;
	bool8 useMaterialLayoutSet  = true;
	bool8 useModelLayoutSet 	= true;
	bool8 useLighting			= true;
	bool8 enableTransparency 	= false;
};


// -------------------------------------------------------------------------
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

internal VkPipelineViewportStateCreateInfo
fsvulkan_pipeline_viewport_state_create_info(u32 viewportCount, VkViewport * pViewports, u32 scissorCount, VkRect2D * pScissors)
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
	VkPipelineRasterizationStateCreateInfo rasterizer =
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

	return rasterizer;
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
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.pNext 					= nullptr,
		.flags 					= 0, // Reserved for future
		.topology               = topology,
		.primitiveRestartEnable = VK_FALSE,
	};
	
	return inputAssembly;
}

internal VkPipelineDynamicStateCreateInfo
fsvulkan_pipeline_dynamic_state_create_info(u32 dynamicStateCount, VkDynamicState * dynamicStates)
{
	VkPipelineDynamicStateCreateInfo createInfo =
	{
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount  = dynamicStateCount,
		.pDynamicStates     = dynamicStates,
	};

	return createInfo;
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


// -------------------------------------------------------------------------


internal FSVulkanPipeline fsvulkan_make_pipeline(
	VulkanContext * context,
	char const * 	vertexShaderPath,
	char const * 	fragmentShaderPath,
	PipelineOptions options)
{
	BinaryAsset vertexShaderCode    = read_binary_file(vertexShaderPath);
	BinaryAsset fragmentShaderCode  = read_binary_file(fragmentShaderPath);

	VkShaderModule vertexShaderModule = vulkan::make_vk_shader_module(vertexShaderCode, context->device);
	VkShaderModule fragmentShaderModule = vulkan::make_vk_shader_module(fragmentShaderCode, context->device);

	enum : u32
	{
		VERTEX_STAGE_ID     = 0,
		FRAGMENT_STAGE_ID   = 1,
		SHADER_STAGE_COUNT  = 2
	};

	VkPipelineShaderStageCreateInfo shaderStages [SHADER_STAGE_COUNT] =
	{
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule,
			.pName = "main",
		}
	};

	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	if (options.useVertexInput)
	{
		VkVertexInputBindingDescription bindingDescription = {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
		vertexInputInfo = fsvulkan_pipeline_vertex_input_state_create_info(	1,
																			&bindingDescription,
																			array_count(fsvulkan_defaultVertexAttributes),
																			fsvulkan_defaultVertexAttributes);
	}
	else
	{
		vertexInputInfo = fsvulkan_pipeline_vertex_input_state_create_info(0, nullptr, 0, nullptr);
	}

	auto inputAssembly = fsvulkan_pipeline_input_assembly_create_info(options.primitiveType);

	VkViewport viewport =
	{
		.x          = 0.0f,
		.y          = 0.0f,
		.width      = (f32) context->drawingResources.extent.width,
		.height     = (f32) context->drawingResources.extent.height,
		.minDepth   = 0.0f,
		.maxDepth   = 1.0f,
	};

	VkRect2D scissor =
	{
		.offset = {0, 0},
		.extent = context->drawingResources.extent,
	};

	auto viewportState 	= fsvulkan_pipeline_viewport_state_create_info(1, &viewport, 1, &scissor);
	auto rasterizer 	= fsvulkan_pipeline_rasterization_state_create_info(options.cullModeFlags);
	auto multisampling 	= fsvulkan_pipeline_multisample_state_create_info(context->msaaSamples);

	// Note: This attachment is per framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment =
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

	if (options.enableTransparency)
	{
		colorBlendAttachment.blendEnable = VK_TRUE;

		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
	}

	// Note: this is global
	VkPipelineColorBlendStateCreateInfo colorBlending =
	{
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable      = VK_FALSE, // Note: enabling this disables per framebuffer method above,
		.logicOp            = VK_LOGIC_OP_COPY,
		.attachmentCount    = 1,
		.pAttachments       = &colorBlendAttachment,
		.blendConstants     = {0, 0, 0, 0}
	};

	VkPipelineDepthStencilStateCreateInfo depthStencil =
	{ 
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

		.depthTestEnable    = (VkBool32)options.enableDepth,
		.depthWriteEnable   = (VkBool32)options.enableDepth,
		.depthCompareOp     = VK_COMPARE_OP_LESS,

		// Note(Leo): These further limit depth range for this render pass
		.depthBoundsTestEnable  = VK_FALSE,

		// Note(Leo): Configure stencil tests
		.stencilTestEnable  = VK_FALSE,
		.front              = {},
		.back               = {},
	   
		.minDepthBounds         = 0.0f,
		.maxDepthBounds         = 1.0f,
	};


	//// DYNAMIC STATE
	VkDynamicState dynamicStatesArray [] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	auto dynamicState = fsvulkan_pipeline_dynamic_state_create_info(array_count(dynamicStatesArray), dynamicStatesArray);

	//  LAYOUT -------------------------------------
	/// PUSH CONSTANTS
	u32 pushConstantRangeCount = 0;
	VkPushConstantRange pushConstantRange;

	if (options.pushConstantSize > 0)
	{
		pushConstantRange =
		{
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			.offset     = 0,
			.size       = options.pushConstantSize,
		};
		pushConstantRangeCount = 1;
	}


	u32 layoutSetCount = 0;
	VkDescriptorSetLayout layoutSets [5];

	auto materialLayout = vulkan::make_material_vk_descriptor_set_layout(context->device, options.textureCount);

	u32 sceneSetIndex = -1;
	u32 modelSetIndex = -1;
	u32 materialSetIndex = -1;
	u32 lightingSetIndex = -1;

	if (options.useSceneLayoutSet)
	{ 
		layoutSets[layoutSetCount] = context->descriptorSetLayouts.camera;
		sceneSetIndex = layoutSetCount;
		++layoutSetCount;
	}

	if (options.useMaterialLayoutSet)
	{ 
		layoutSets[layoutSetCount] = materialLayout;
		materialSetIndex = layoutSetCount;
		++layoutSetCount;
	}
	if (options.useModelLayoutSet)
	{ 
		layoutSets[layoutSetCount] = context->descriptorSetLayouts.model;
		modelSetIndex = layoutSetCount;
		++layoutSetCount;
	}

	if (options.useLighting)
	{
		layoutSets[layoutSetCount] = context->descriptorSetLayouts.lighting;
		lightingSetIndex = layoutSetCount;
		++layoutSetCount;
	
		layoutSets[layoutSetCount] = context->descriptorSetLayouts.shadowMap;
		++layoutSetCount;
	}

	auto pipelineLayoutInfo = fsvulkan_pipeline_layout_create_info(layoutSetCount, layoutSets, pushConstantRangeCount, &pushConstantRange);

	VkPipelineLayout layout;
	VULKAN_CHECK(vkCreatePipelineLayout (context->device, &pipelineLayoutInfo, nullptr, &layout));

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount             = SHADER_STAGE_COUNT,
		.pStages                = shaderStages,

		// Note(Leo): Are these The Fixed-Function Stages?
		.pVertexInputState      = &vertexInputInfo,
		.pInputAssemblyState    = &inputAssembly,
		.pViewportState         = &viewportState,
		.pRasterizationState    = &rasterizer,
		.pMultisampleState      = &multisampling,   
		.pDepthStencilState     = &depthStencil,
		.pColorBlendState       = &colorBlending,
		.pDynamicState          = &dynamicState,

		.layout                 = layout,
		.renderPass             = context->drawingResources.renderPass,
		.subpass                = 0,

		// Note(Leo): These are for cheaper re-use of pipelines, not used right now.
		// Study(Leo): Like inheritance??
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	VkPipeline pipeline;
	VULKAN_CHECK(vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	// Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
	vkDestroyShaderModule(context->device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context->device, vertexShaderModule, nullptr);

	FSVulkanPipeline result = 
	{
		.pipeline       		= pipeline,
		.pipelineLayout     	= layout,
		.descriptorSetLayout 	= materialLayout,
		.textureCount 			= options.textureCount
	};
	return result;
}

// Todo(Leo): This is temporary helper here, goal is to remove options and make_pipeline things, 
// and make a bunch of useful functions that provide initializers and then in actual functions
// use those to make pipelines.
struct EvenMoreArguments
{
	u32 									vertexBindingDescriptionCount;
	VkVertexInputBindingDescription const * vertexBindingDescriptions;

	u32 										vertexAttributeDescriptionCount;
	VkVertexInputAttributeDescription const * 	vertexAttributeDescriptions;

	u32 						pushConstantRangeCount;
	VkPushConstantRange const * pushConstantRanges;

	u32 							descriptorSetLayoutCount;
	VkDescriptorSetLayout const * 	descriptorSetLayouts;
};

internal FSVulkanPipeline fsvulkan_make_pipeline_WITH_EVEN_MORE_ARGUMENTS(
	VulkanContext * context,
	char const *    vertexShaderPath,
	char const *    fragmentShaderPath,

	EvenMoreArguments const & arguments,

	PipelineOptions options)
{		
	// Note(Leo): Deprecated options
	Assert(options.pushConstantSize == 0 && "Use push constant ranges in 'arguments' instead");

	// ------------------------------------------------------------

	BinaryAsset vertexShaderCode    = read_binary_file(vertexShaderPath);
	BinaryAsset fragmentShaderCode  = read_binary_file(fragmentShaderPath);

	VkShaderModule vertexShaderModule = vulkan::make_vk_shader_module(vertexShaderCode, context->device);
	VkShaderModule fragmentShaderModule = vulkan::make_vk_shader_module(fragmentShaderCode, context->device);

	enum : u32
	{
		VERTEX_STAGE_ID     = 0,
		FRAGMENT_STAGE_ID   = 1,
		SHADER_STAGE_COUNT  = 2
	};

	VkPipelineShaderStageCreateInfo shaderStages [SHADER_STAGE_COUNT] =
	{
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName = "main",
		},
		{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = fragmentShaderModule,
			.pName = "main",
		}
	};

	auto vertexInputInfo = fsvulkan_pipeline_vertex_input_state_create_info(arguments.vertexBindingDescriptionCount,
																			arguments.vertexBindingDescriptions,
																			arguments.vertexAttributeDescriptionCount,
																			arguments.vertexAttributeDescriptions);

	auto inputAssembly = fsvulkan_pipeline_input_assembly_create_info(options.primitiveType);

	// TODO(Leo): These are using dynamic state, remove that, since now that pipelines
	// are created entirely on platform side, reloaaing them is super easy and in final game
	// we do not expect lot of screen size changing
	VkViewport viewport =
	{
		.x          = 0.0f,
		.y          = 0.0f,
		.width      = (f32) context->drawingResources.extent.width,
		.height     = (f32) context->drawingResources.extent.height,
		.minDepth   = 0.0f,
		.maxDepth   = 1.0f,
	};

	VkRect2D scissor =
	{
		.offset = {0, 0},
		.extent = context->drawingResources.extent,
	};

	auto viewportState 	= fsvulkan_pipeline_viewport_state_create_info(1, &viewport, 1, &scissor);
	auto rasterizer 	= fsvulkan_pipeline_rasterization_state_create_info(options.cullModeFlags);
	auto multisampling 	= fsvulkan_pipeline_multisample_state_create_info(context->msaaSamples);

	// Note: This attachment is per framebuffer
	VkPipelineColorBlendAttachmentState colorBlendAttachment =
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

	if (options.enableTransparency)
	{
		colorBlendAttachment.blendEnable = VK_TRUE;

		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_SUBTRACT;
	}

	// Note: this is global
	VkPipelineColorBlendStateCreateInfo colorBlending =
	{
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable      = VK_FALSE, // Note: enabling this disables per framebuffer method above,
		.logicOp            = VK_LOGIC_OP_COPY,
		.attachmentCount    = 1,
		.pAttachments       = &colorBlendAttachment,
		.blendConstants     = {0, 0, 0, 0}
	};

	VkPipelineDepthStencilStateCreateInfo depthStencil =
	{ 
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

		.depthTestEnable    = (VkBool32)options.enableDepth,
		.depthWriteEnable   = (VkBool32)options.enableDepth,
		.depthCompareOp     = VK_COMPARE_OP_LESS,

		// Note(Leo): These further limit depth range for this render pass
		.depthBoundsTestEnable  = VK_FALSE,

		// Note(Leo): Configure stencil tests
		.stencilTestEnable  = VK_FALSE,
		.front              = {},
		.back               = {},
	   
		.minDepthBounds         = 0.0f,
		.maxDepthBounds         = 1.0f,
	};


	//// DYNAMIC STATE
	VkDynamicState dynamicStatesArray [] =
	{
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
	};
	auto dynamicState = fsvulkan_pipeline_dynamic_state_create_info(array_count(dynamicStatesArray), dynamicStatesArray);

	auto pipelineLayoutInfo = fsvulkan_pipeline_layout_create_info(	arguments.descriptorSetLayoutCount,
																	arguments.descriptorSetLayouts,
																	arguments.pushConstantRangeCount,
																	arguments.pushConstantRanges);

	VkPipelineLayout layout;
	VULKAN_CHECK(vkCreatePipelineLayout (context->device, &pipelineLayoutInfo, nullptr, &layout));

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount             = SHADER_STAGE_COUNT,
		.pStages                = shaderStages,

		// Note(Leo): Are these The Fixed-Function Stages?
		.pVertexInputState      = &vertexInputInfo,
		.pInputAssemblyState    = &inputAssembly,
		.pViewportState         = &viewportState,
		.pRasterizationState    = &rasterizer,
		.pMultisampleState      = &multisampling,   
		.pDepthStencilState     = &depthStencil,
		.pColorBlendState       = &colorBlending,
		.pDynamicState          = &dynamicState,

		.layout                 = layout,
		.renderPass             = context->drawingResources.renderPass,
		.subpass                = 0,

		// Note(Leo): These are for cheaper re-use of pipelines, not used right now.
		// Study(Leo): Like inheritance??
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};

	VkPipeline pipeline;
	VULKAN_CHECK(vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	// Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
	vkDestroyShaderModule(context->device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context->device, vertexShaderModule, nullptr);

	FSVulkanPipeline result = 
	{
		.pipeline               = pipeline,
		.pipelineLayout         = layout,
		// .descriptorSetLayout    = materialLayout,
		.textureCount           = options.textureCount
	};
	return result;
}

  //////////////////////////////////////////////////////////////////////////////////////
 /// Actual Pipeline initializers
//////////////////////////////////////////////////////////////////////////////////////


internal void fsvulkan_initialize_normal_pipeline(VulkanContext & context)
{
	auto materialLayout = vulkan::make_material_vk_descriptor_set_layout(context.device, 3);

	VkDescriptorSetLayout descriptorSetLayouts[] =
	{
		context.descriptorSetLayouts.camera,
		materialLayout,
		context.descriptorSetLayouts.model,
		context.descriptorSetLayouts.lighting,
		context.descriptorSetLayouts.shadowMap,
	};

	VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 2 * sizeof(f32) };

	EvenMoreArguments arguments = 
	{
		.vertexBindingDescriptionCount 		= 1,
		.vertexBindingDescriptions 			= &fsvulkan_defaultVertexBinding,

		.vertexAttributeDescriptionCount 	= array_count(fsvulkan_defaultVertexAttributes),
		.vertexAttributeDescriptions 		= fsvulkan_defaultVertexAttributes,

		.pushConstantRangeCount 	= 1,
		.pushConstantRanges 		= &pushConstantRange,

		.descriptorSetLayoutCount 	= array_count(descriptorSetLayouts),
		.descriptorSetLayouts 		= descriptorSetLayouts,
	};

	// Todo(Leo): do not use make_pipeline(), goal is to get rid of that
	// context.pipelines[GRAPHICS_PIPELINE_NORMAL] = fsvulkan_make_pipeline(
	context.pipelines[GRAPHICS_PIPELINE_NORMAL] = fsvulkan_make_pipeline_WITH_EVEN_MORE_ARGUMENTS(
		&context,
		"assets/shaders/vert.spv",
		"assets/shaders/frag.spv",

		arguments,

		{	
			.textureCount = 3,
		}
	);

	context.pipelines[GRAPHICS_PIPELINE_NORMAL].descriptorSetLayout = materialLayout;
}

internal void fsvulkan_initialize_animated_pipeline(VulkanContext & context)
{
	context.pipelines[GRAPHICS_PIPELINE_ANIMATED] = fsvulkan_make_pipeline(&context,
		"assets/shaders/animated_vert.spv",
		"assets/shaders/frag.spv",
		{
			.textureCount = 3
		}
	);
}

internal void fsvulkan_initialize_skybox_pipeline(VulkanContext & context)
{
	context.pipelines[GRAPHICS_PIPELINE_SKYBOX] = fsvulkan_make_pipeline(&context,
		"assets/shaders/vert_sky.spv",
		"assets/shaders/frag_sky.spv",
		{
			.textureCount = 1,
			.enableDepth = false
		}
	);
}

internal void fsvulkan_initialize_screen_gui_pipeline(VulkanContext & context)
{
	context.pipelines[GRAPHICS_PIPELINE_SCREEN_GUI] = fsvulkan_make_pipeline(&context,
		"assets/shaders/gui_vert3.spv",
		"assets/shaders/gui_frag2.spv",
		{
			.textureCount           = 1,
			.pushConstantSize       = sizeof(v2) * 4 + sizeof(v4),

			.primitiveType          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
			.cullModeFlags          = VK_CULL_MODE_NONE,

			.enableDepth 			= false,
			.useVertexInput         = false,
			.useSceneLayoutSet      = false,
			.useMaterialLayoutSet   = true,
			.useModelLayoutSet      = false,
			.enableTransparency     = true
		}
	);
}

internal void fsvulkan_initialize_leaves_pipeline(VulkanContext & context)
{
	VkVertexInputBindingDescription vertexBindingDescription =
	{
		.binding 	= 0,
		.stride 	= sizeof(m44),
		.inputRate 	= VK_VERTEX_INPUT_RATE_INSTANCE
	};

	// Note(Leo): We input matrix, it is described as 4 4d vectors
	VkVertexInputAttributeDescription vertexAttributeDescriptions [4] =
	{
		{ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 * sizeof(v4)},
		{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 1 * sizeof(v4)},
		{ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sizeof(v4)},
		{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 3 * sizeof(v4)},
	};
	
	auto materialLayout = vulkan::make_material_vk_descriptor_set_layout(context.device, 0);

	VkDescriptorSetLayout descriptorSetLayouts[] =
	{
		context.descriptorSetLayouts.camera,
		materialLayout,
		context.descriptorSetLayouts.model,
		context.descriptorSetLayouts.lighting,
		context.descriptorSetLayouts.shadowMap,
	};

	EvenMoreArguments arguments = 
	{
		.vertexBindingDescriptionCount 		= 1,
		.vertexBindingDescriptions 			= &vertexBindingDescription,
		.vertexAttributeDescriptionCount 	= 4,
		.vertexAttributeDescriptions 		= vertexAttributeDescriptions,

		.descriptorSetLayoutCount  	= array_count(descriptorSetLayouts),
		.descriptorSetLayouts 		= descriptorSetLayouts
	};

	context.pipelines[GRAPHICS_PIPELINE_LEAVES] = fsvulkan_make_pipeline_WITH_EVEN_MORE_ARGUMENTS(&context,
		"assets/shaders/leaves_vert.spv",
		"assets/shaders/leaves_frag.spv",

		arguments,

		{
			.primitiveType          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
			.cullModeFlags          = VK_CULL_MODE_NONE,
		}
	);

	context.pipelines[GRAPHICS_PIPELINE_LEAVES].descriptorSetLayout = materialLayout;
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

	EvenMoreArguments arguments = 
	{
		.vertexBindingDescriptionCount 		= 1,
		.vertexBindingDescriptions 			= &vertexBindingDescription,
		.vertexAttributeDescriptionCount 	= array_count(vertexAttributeDescriptions),
		.vertexAttributeDescriptions 		= vertexAttributeDescriptions,

		.descriptorSetLayoutCount 	= 1,
		.descriptorSetLayouts 		= &context.descriptorSetLayouts.camera,
	};

	FSVulkanPipeline TEMP_SOLUTION = fsvulkan_make_pipeline_WITH_EVEN_MORE_ARGUMENTS(&context,
		"assets/shaders/line_vert.spv",
		"assets/shaders/line_frag.spv",

		arguments,

		{
			.primitiveType          = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,

			.useVertexInput         = false,
			.useSceneLayoutSet      = true,
			.useMaterialLayoutSet   = false,
			.useModelLayoutSet      = false,
		}
	);

	context.linePipeline 					= TEMP_SOLUTION.pipeline;
	context.linePipelineLayout 				= TEMP_SOLUTION.pipelineLayout;
	context.linePipelineDescriptorSetLayout = TEMP_SOLUTION.descriptorSetLayout;
}

internal void fsvulkan_initialize_shadow_pipeline(VulkanContext & context)
{
	VkShaderModule vertexShaderModule = vulkan::make_vk_shader_module(read_binary_file("assets/shaders/shadow_vert.spv"), context.device);

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		{
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName  = "main",
		},
	};

	/// VERTEX INPUT STATE
	VkVertexInputBindingDescription bindingDescription = { 0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX };
	auto vertexInputInfo = fsvulkan_pipeline_vertex_input_state_create_info(1,
																			&bindingDescription,
																			array_count(fsvulkan_defaultVertexAttributes),
																			fsvulkan_defaultVertexAttributes);

	/// INPUT ASSEMBLY
	auto inputAssembly = fsvulkan_pipeline_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	/// VIEWPORT STATE
	VkViewport viewport = {0, 0, (f32)context.shadowPass.width, (f32)context.shadowPass.height, 0, 1};
	VkRect2D scissor 	= {{0,0}, {context.shadowPass.width, context.shadowPass.height}};
	auto viewportState 	= fsvulkan_pipeline_viewport_state_create_info(1, &viewport, 1, &scissor);

	/// RASTERIZATION
	auto rasterizer = fsvulkan_pipeline_rasterization_state_create_info(VK_CULL_MODE_NONE);

	VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable    = VK_FALSE,
		.minSampleShading       = 1.0f,
		.pSampleMask            = nullptr,
		.alphaToCoverageEnable  = VK_FALSE,
		.alphaToOneEnable       = VK_FALSE,
	};

	// Note(Leo): this defaults nicely to zero, but I'm not sure its a good idea to rely on that
	VkPipelineColorBlendStateCreateInfo colorBlending =	{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

	VkPipelineDepthStencilStateCreateInfo depthStencil =
	{ 
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

		.depthTestEnable    = VK_TRUE,
		.depthWriteEnable   = VK_TRUE,
		.depthCompareOp     = VK_COMPARE_OP_LESS_OR_EQUAL,

		.depthBoundsTestEnable  = VK_FALSE,

		.stencilTestEnable  = VK_FALSE,
		.front              = {},
		.back               = {},
	  
		.minDepthBounds     = 0.0f,
		.maxDepthBounds     = 1.0f,
	};

	VkDescriptorSetLayout setLayouts [] =
	{
		context.descriptorSetLayouts.camera,
		context.descriptorSetLayouts.model,
	};

	auto pipelineLayoutInfo = fsvulkan_pipeline_layout_create_info(array_count(setLayouts), setLayouts, 0, nullptr);

	VkPipelineLayout layout;
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutInfo, nullptr, &layout));

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount             = 1,
		.pStages                = shaderStages,

		.pVertexInputState      = &vertexInputInfo,
		.pInputAssemblyState    = &inputAssembly,
		.pViewportState         = &viewportState,
		.pRasterizationState    = &rasterizer,
		.pMultisampleState      = &multisampling,
		.pDepthStencilState     = &depthStencil,
		.pColorBlendState       = &colorBlending,
		.pDynamicState          = nullptr,

		.layout                 = layout,
		.renderPass             = context.shadowPass.renderPass,
		.subpass                = 0,

		// Note(Leo): These are for cheaper re-use of pipelines, not used right now.
		// Study(Leo): Like inheritance??
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};


	VkPipeline pipeline;
	VULKAN_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	// Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
	// vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);

	context.shadowPass.pipeline = pipeline;
	context.shadowPass.layout = layout;
}

internal void fsvulkan_initialize_leaves_shadow_pipeline(VulkanContext & context)
{
	VkShaderModule vertexShaderModule = vulkan::make_vk_shader_module(read_binary_file("assets/shaders/leaves_shadow_vert.spv"), context.device);

	VkPipelineShaderStageCreateInfo shaderStages [] =
	{
		{
			.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage  = VK_SHADER_STAGE_VERTEX_BIT,
			.module = vertexShaderModule,
			.pName  = "main",
		},
	};

	/// VERTEX INPUT STATE
	VkVertexInputBindingDescription vertexBindingDescription =
	{
		.binding 	= 0,
		.stride 	= sizeof(m44),
		.inputRate 	= VK_VERTEX_INPUT_RATE_INSTANCE
	};

	// Note(Leo): We input matrix, it is described as 4 4d vectors
	VkVertexInputAttributeDescription vertexAttributeDescriptions [4] =
	{
		{ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 * sizeof(v4)},
		{ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 1 * sizeof(v4)},
		{ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 2 * sizeof(v4)},
		{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 3 * sizeof(v4)},
	};
	auto vertexInputInfo = fsvulkan_pipeline_vertex_input_state_create_info(1,
																			&vertexBindingDescription,	
																			array_count(vertexAttributeDescriptions),
																			vertexAttributeDescriptions);

	/// INPUT ASSEMBLY
	auto inputAssembly = fsvulkan_pipeline_input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP);

	/// VIEWPORT STATE
	VkViewport viewport = {0, 0, (f32)context.shadowPass.width, (f32)context.shadowPass.height, 0, 1};
	VkRect2D scissor 	= {{0,0}, {context.shadowPass.width, context.shadowPass.height}};
	auto viewportState 	= fsvulkan_pipeline_viewport_state_create_info(1, &viewport, 1, &scissor);

	/// RASTERIZATION
	auto rasterizer = fsvulkan_pipeline_rasterization_state_create_info(VK_CULL_MODE_NONE);

	VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples   = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable    = VK_FALSE,
		.minSampleShading       = 1.0f,
		.pSampleMask            = nullptr,
		.alphaToCoverageEnable  = VK_FALSE,
		.alphaToOneEnable       = VK_FALSE,
	};

	// Note(Leo): this defaults nicely to zero, but I'm not sure its a good idea to rely on that
	VkPipelineColorBlendStateCreateInfo colorBlending =	{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };

	VkPipelineDepthStencilStateCreateInfo depthStencil =
	{ 
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

		.depthTestEnable    = VK_TRUE,
		.depthWriteEnable   = VK_TRUE,
		.depthCompareOp     = VK_COMPARE_OP_LESS_OR_EQUAL,

		.depthBoundsTestEnable  = VK_FALSE,

		.stencilTestEnable  = VK_FALSE,
		.front              = {},
		.back               = {},
	  
		.minDepthBounds     = 0.0f,
		.maxDepthBounds     = 1.0f,
	};


	auto pipelineLayoutInfo = fsvulkan_pipeline_layout_create_info(1, &context.descriptorSetLayouts.camera, 0, nullptr);

	VkPipelineLayout layout;
	VULKAN_CHECK(vkCreatePipelineLayout (context.device, &pipelineLayoutInfo, nullptr, &layout));

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount             = 1,
		.pStages                = shaderStages,

		.pVertexInputState      = &vertexInputInfo,
		.pInputAssemblyState    = &inputAssembly,
		.pViewportState         = &viewportState,
		.pRasterizationState    = &rasterizer,
		.pMultisampleState      = &multisampling,
		.pDepthStencilState     = &depthStencil,
		.pColorBlendState       = &colorBlending,
		.pDynamicState          = nullptr,

		.layout                 = layout,
		.renderPass             = context.shadowPass.renderPass,
		.subpass                = 0,

		// Note(Leo): These are for cheaper re-use of pipelines, not used right now.
		// Study(Leo): Like inheritance??
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1,
	};


	VkPipeline pipeline;
	VULKAN_CHECK(vkCreateGraphicsPipelines(context.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline));

	// Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
	// vkDestroyShaderModule(context.device, fragmentShaderModule, nullptr);
	vkDestroyShaderModule(context.device, vertexShaderModule, nullptr);

	context.leavesShadowPipeline 		= pipeline;
	context.leavesShadowPipelineLayout 	= layout;
}

