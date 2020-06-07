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

namespace vulkan_pipelines_internal_
{
	internal VkVertexInputBindingDescription get_vertex_binding_description ();;
	internal StaticArray<VkVertexInputAttributeDescription, 8> get_vertex_attribute_description();

	internal VkPipelineDynamicStateCreateInfo make_dynamic_state(   PipelineOptions * options,
																	VkDynamicState(&array)[10]);
}

// -------------------------------------------------------------------------
/// GOOD INITIALIZERS
// Note(Leo): Only expose from these functions properties we are interested to change

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
fsvulkan_pipeline_layout_create_info(	u32 setLayoutCount, VkDescriptorSetLayout * pSetLayouts,
										u32 pushConstantRangeCount, VkPushConstantRange * pPushConstantRanges)
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
fsvulkan_pipeline_vertex_input_state_create_info(	u32 bindingDescriptionCount, VkVertexInputBindingDescription * bindingDescriptions,
													u32 attributeDescriptionCount, VkVertexInputAttributeDescription * attributeDescriptions)
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
		auto bindingDescription     = vulkan_pipelines_internal_::get_vertex_binding_description();
		auto attributeDescriptions  = vulkan_pipelines_internal_::get_vertex_attribute_description();
		vertexInputInfo = fsvulkan_pipeline_vertex_input_state_create_info(	1, 
																			&bindingDescription,
																			attributeDescriptions.count(),
																			&attributeDescriptions[0]);
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

	auto viewportState = fsvulkan_pipeline_viewport_state_create_info(1, &viewport, 1, &scissor);

	auto rasterizer = fsvulkan_pipeline_rasterization_state_create_info(options.cullModeFlags);

	VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples   = context->msaaSamples,
		.sampleShadingEnable    = VK_FALSE,
		.minSampleShading       = 1.0f,
		.pSampleMask            = nullptr,
		.alphaToCoverageEnable  = VK_FALSE,
		.alphaToOneEnable       = VK_FALSE,
	};

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
	VkDynamicState dynamicStatesArray[10];
	auto dynamicState = vulkan_pipelines_internal_::make_dynamic_state(&options, dynamicStatesArray);

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

internal VkVertexInputBindingDescription
vulkan_pipelines_internal_::get_vertex_binding_description ()
{
	return {
		.binding    = 0,
		.stride     = sizeof(Vertex),
		.inputRate  = VK_VERTEX_INPUT_RATE_VERTEX
	};
}

internal StaticArray<VkVertexInputAttributeDescription, 8>
vulkan_pipelines_internal_::get_vertex_attribute_description()
{
	StaticArray<VkVertexInputAttributeDescription, 8> value = {};

	value[0].binding = 0;
	value[0].location = 0;
	value[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	value[0].offset = offsetof(Vertex, position);

	value[1].binding = 0;
	value[1].location = 1;
	value[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	value[1].offset = offsetof(Vertex, normal);

	value[2].binding = 0;
	value[2].location = 2;
	value[2].format = VK_FORMAT_R32G32B32_SFLOAT;
	value[2].offset = offsetof(Vertex, color);  

	value[3].binding = 0;
	value[3].location = 3;
	value[3].format = VK_FORMAT_R32G32_SFLOAT;
	value[3].offset = offsetof(Vertex, texCoord);

	value[4].binding = 0;
	value[4].location = 4;
	// Todo(Leo): This does not need to be 32 bit, but I do not yet want to mess around with it.
	value[4].format = VK_FORMAT_R32G32B32A32_UINT;
	value[4].offset = offsetof(Vertex, boneIndices);  

	value[5].binding = 0;
	value[5].location = 5;
	value[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	value[5].offset = offsetof(Vertex, boneWeights);    

	value[6] =
	{
		.location = 6,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, tangent),
	};

	value[7] =
	{
		.location = 7,
		.binding = 0,
		.format = VK_FORMAT_R32G32B32_SFLOAT,
		.offset = offsetof(Vertex, biTangent),
	};

	return value;
}   

internal VkPipelineDynamicStateCreateInfo
vulkan_pipelines_internal_::make_dynamic_state (PipelineOptions * options, VkDynamicState (&array)[10])
{
	u32 count = 0;
	array[count++] = VK_DYNAMIC_STATE_VIEWPORT;
	array[count++] = VK_DYNAMIC_STATE_SCISSOR;

	return {
		.sType              = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount  = count,
		.pDynamicStates     = array,
	};
}

// Todo(Leo): This is temporary helper here, goal is to remove options and make_pipeline things, 
// and make a bunch of useful functions that provide initializers and then in actual functions
// use those to make pipelines.
struct EvenMoreArguments
{
	u32 								vertexBindingDescriptionCount;
	VkVertexInputBindingDescription * 	vertexBindingDescriptions;

	u32 								vertexAttributeDescriptionCount;
	VkVertexInputAttributeDescription * vertexAttributeDescriptions;

	u32 					pushConstantRangeCount;
	VkPushConstantRange * 	pushConstantRanges;
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

	VkPipelineVertexInputStateCreateInfo vertexInputInfo =
	{
		.sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount      = arguments.vertexBindingDescriptionCount,
		.pVertexBindingDescriptions         = arguments.vertexBindingDescriptions,
		.vertexAttributeDescriptionCount    = arguments.vertexAttributeDescriptionCount,
		.pVertexAttributeDescriptions       = arguments.vertexAttributeDescriptions,
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology               = options.primitiveType,
		.primitiveRestartEnable = VK_FALSE,
	};

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

	VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType          = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount  = 1,
		.pViewports     = &viewport,
		.scissorCount   = 1,
		.pScissors      = &scissor,
	};

	VkPipelineRasterizationStateCreateInfo rasterizer =
	{
		.sType                      = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable           = VK_FALSE,
		.rasterizerDiscardEnable    = VK_FALSE,
		.polygonMode                = VK_POLYGON_MODE_FILL,
		.cullMode                   = options.cullModeFlags,
		.frontFace                  = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable            = VK_FALSE,
		.depthBiasConstantFactor    = 0.0f,
		.depthBiasClamp             = 0.0f,
		.depthBiasSlopeFactor       = 0.0f,
		.lineWidth                  = options.lineWidth,
	};

	VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples   = context->msaaSamples,
		.sampleShadingEnable    = VK_FALSE,
		.minSampleShading       = 1.0f,
		.pSampleMask            = nullptr,
		.alphaToCoverageEnable  = VK_FALSE,
		.alphaToOneEnable       = VK_FALSE,
	};

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
	VkDynamicState dynamicStatesArray[10];
	auto dynamicState = vulkan_pipelines_internal_::make_dynamic_state(&options, dynamicStatesArray);

	//  LAYOUT -------------------------------------

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

	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount         = layoutSetCount,
		.pSetLayouts            = layoutSets,
		.pushConstantRangeCount = arguments.pushConstantRangeCount,
		.pPushConstantRanges    = arguments.pushConstantRanges,
	};

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
		.descriptorSetLayout    = materialLayout,
		.textureCount           = options.textureCount
	};
	return result;
}

  //////////////////////////////////////////////////////////////////////////////////////
 /// Actual Pipeline initializers
//////////////////////////////////////////////////////////////////////////////////////


internal void fsvulkan_initialize_normal_pipeline(VulkanContext & context)
{
	// Todo(Leo): do not use make_pipeline(), goal is to get rid of that
	context.pipelines[GRAPHICS_PIPELINE_NORMAL] = fsvulkan_make_pipeline(
		&context,
		"assets/shaders/vert.spv",
		"assets/shaders/frag.spv",
		{	
			.textureCount = 3,
		}
	);
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

	EvenMoreArguments arguments = 
	{
		.vertexBindingDescriptionCount 		= 1,
		.vertexBindingDescriptions 			= &vertexBindingDescription,
		.vertexAttributeDescriptionCount 	= 4,
		.vertexAttributeDescriptions 		= vertexAttributeDescriptions,
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
		.vertexAttributeDescriptions 		= vertexAttributeDescriptions
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

internal void fsvulkan_initialize_sky_pipeline(VulkanContext & context)
{
	logSystem(0) << FILE_ADDRESS << "Unused function";
	// VkPushConstantRange pushConstantRange = { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(m44) };

	// EvenMoreArguments arguments = 
	// {
	// 	.pushConstantRangeCount = 1,
	// 	.pushConstantRanges 	= &pushConstantRange
	// };

	// FSVulkanPipeline TEMP_SOLUTION = fsvulkan_make_pipeline_WITH_EVEN_MORE_ARGUMENTS(&context,
	// 	"assets/shaders/sky_quad_vert.spv",
	// 	"assets/shaders/sky_quad_frag.spv",

	// 	arguments,

	// 	{
	// 		.primitiveType          = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
	// 		.cullModeFlags 			= VK_CULL_MODE_NONE,

	// 		.enableDepth 			= false,
	// 		.useVertexInput         = false,
	// 		.useSceneLayoutSet      = true,
	// 		.useMaterialLayoutSet   = false,
	// 		.useModelLayoutSet      = false,
	// 		.useLighting			= false,
	// 	}
	// );

	// vkDestroyDescriptorSetLayout (context.device, TEMP_SOLUTION.descriptorSetLayout, nullptr);

	// context.skyPipeline 					= TEMP_SOLUTION.pipeline;
	// context.skyPipelineLayout 				= TEMP_SOLUTION.pipelineLayout;
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
	auto bindingDescription     = vulkan_pipelines_internal_::get_vertex_binding_description();
	auto attributeDescriptions  = vulkan_pipelines_internal_::get_vertex_attribute_description();
	auto vertexInputInfo 		= fsvulkan_pipeline_vertex_input_state_create_info(	1,
																					&bindingDescription,
																					attributeDescriptions.count(),
																					&attributeDescriptions[0]);

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

