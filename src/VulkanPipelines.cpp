/*=============================================================================
Leo Tamminen
shophorn @ internet

Vulkan pipeline things separated to a different place, nothing too sensible yet.
Todo(Leo): These totally should be combined more/somehow
=============================================================================*/
internal void
vulkan::destroy_loaded_pipeline(VulkanContext * context, VulkanLoadedPipeline * pipeline)
{
    vkDestroyPipeline(context->device, pipeline->pipeline, nullptr);
    pipeline->layout = VK_NULL_HANDLE;
 
    vkDestroyPipelineLayout(context->device, pipeline->layout, nullptr);
    pipeline->pipeline = VK_NULL_HANDLE;

    vkDestroyDescriptorSetLayout(context->device, pipeline->materialLayout, nullptr);
    pipeline->materialLayout = VK_NULL_HANDLE;
}

internal VulkanLoadedPipeline
vulkan::make_pipeline(
    VulkanContext * context,
    VulkanPipelineLoadInfo info)
{
    // Todo(Leo): unhardcode these
    BinaryAsset vertexShaderCode = ReadBinaryFile(info.vertexShaderPath.c_str());
    BinaryAsset fragmentShaderCode = ReadBinaryFile(info.fragmentShaderPath.c_str());

    VkShaderModule vertexShaderModule = vulkan::CreateShaderModule(vertexShaderCode, context->device);
    VkShaderModule fragmentShaderModule = vulkan::CreateShaderModule(fragmentShaderCode, context->device);

    auto & options = info.options;

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

    auto bindingDescription     = vulkan::get_vertex_binding_description();
    auto attributeDescriptions  = vulkan::get_vertex_attribute_description();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo =
    {
        .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount      = 1,
        .pVertexBindingDescriptions         = &bindingDescription,
        .vertexAttributeDescriptionCount    = attributeDescriptions.count(),
        .pVertexAttributeDescriptions       = &attributeDescriptions[0],
    };

    VkPrimitiveTopology topology;
    switch(options.primitiveType)
    {
        case platform::RenderingOptions::PRIMITIVE_LINE:     topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
        case platform::RenderingOptions::PRIMITIVE_TRIANGLE: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport =
    {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = (float) context->swapchainItems.extent.width,
        .height     = (float) context->swapchainItems.extent.height,
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    VkRect2D scissor =
    {
        .offset = {0, 0},
        .extent = context->swapchainItems.extent,
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
        .lineWidth                  = 1.0f,
        .cullMode                   = VK_CULL_MODE_BACK_BIT,
        .frontFace                  = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable            = VK_FALSE,
        .depthBiasConstantFactor    = 0.0f,
        .depthBiasClamp             = 0.0f,
        .depthBiasSlopeFactor       = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable    = VK_FALSE,
        .rasterizationSamples   = context->msaaSamples,
        .minSampleShading       = 1.0f,
        .pSampleMask            = nullptr,
        .alphaToCoverageEnable  = VK_FALSE,
        .alphaToOneEnable       = VK_FALSE,
    };

    // Note: This attachment is per framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment =
    {
        .colorWriteMask         =   VK_COLOR_COMPONENT_R_BIT 
                                    | VK_COLOR_COMPONENT_G_BIT 
                                    | VK_COLOR_COMPONENT_B_BIT 
                                    | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable            = VK_FALSE,
        .srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp           = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp           = VK_BLEND_OP_ADD, 
    };

    // Note: this is global
    VkPipelineColorBlendStateCreateInfo colorBlending =
    {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable      = VK_FALSE, // Note: enabling this disables per framebuffer method above,
        .logicOp            = VK_LOGIC_OP_COPY,
        .attachmentCount    = 1,
        .pAttachments       = &colorBlendAttachment,
        .blendConstants[0]  = 0.0f,
        .blendConstants[1]  = 0.0f,
        .blendConstants[2]  = 0.0f,
        .blendConstants[3]  = 0.0f,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil =
    { 
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

        .depthTestEnable    = (VkBool32)options.enableDepth,
        .depthWriteEnable   = (VkBool32)options.enableDepth,
        .depthCompareOp     = VK_COMPARE_OP_LESS,

        // Note(Leo): These further limit depth range for this render pass
        .depthBoundsTestEnable  = VK_FALSE,
        .minDepthBounds         = 0.0f,
        .maxDepthBounds         = 1.0f,

        // Note(Leo): Configure stencil tests
        .stencilTestEnable  = VK_FALSE,
        .front              = {},
        .back               = {},
    };

    auto materialLayout = vulkan::create_material_descriptor_set_layout(context->device, options.textureCount);
    VkDescriptorSetLayout descriptorSetLayouts [3]
    {
        context->descriptorSetLayouts.scene,
        materialLayout,
        context->descriptorSetLayouts.model,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 3,
        .pSetLayouts            = descriptorSetLayouts,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    };

    VkPipelineLayout layout;
    if (vkCreatePipelineLayout (context->device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

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
        .pDynamicState          = nullptr,
        .layout                 = layout,
        .renderPass             = context->renderPass,
        .subpass                = 0,

        // Note(Leo): These are for cheaper re-use of pipelines, not used right now.
        // Study(Leo): Like inheritance??
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };


    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    // Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
    vkDestroyShaderModule(context->device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(context->device, vertexShaderModule, nullptr);

    return {
        .info           = info,
        .pipeline       = pipeline,
        .layout         = layout,
        .materialLayout = materialLayout
    };
}

internal VulkanLoadedPipeline
vulkan::make_line_pipeline(
    VulkanContext * context,
    VulkanPipelineLoadInfo info)
{

    // Todo(Leo): unhardcode these
    BinaryAsset vertexShaderCode = ReadBinaryFile(info.vertexShaderPath.c_str());
    BinaryAsset fragmentShaderCode = ReadBinaryFile(info.fragmentShaderPath.c_str());

    VkShaderModule vertexShaderModule = vulkan::CreateShaderModule(vertexShaderCode, context->device);
    VkShaderModule fragmentShaderModule = vulkan::CreateShaderModule(fragmentShaderCode, context->device);

    auto & options = info.options;

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

    auto bindingDescription     = vulkan::get_vertex_binding_description();
    auto attributeDescriptions  = vulkan::get_vertex_attribute_description();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo =
    {
        .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount      = 0,
        .pVertexBindingDescriptions         = nullptr,
        .vertexAttributeDescriptionCount    = 0,
        .pVertexAttributeDescriptions       = nullptr,
    };

    VkPrimitiveTopology topology;
    switch(options.primitiveType)
    {
        case platform::RenderingOptions::PRIMITIVE_LINE:     topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
        case platform::RenderingOptions::PRIMITIVE_TRIANGLE: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
    }

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport =
    {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = (float) context->swapchainItems.extent.width,
        .height     = (float) context->swapchainItems.extent.height,
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    VkRect2D scissor =
    {
        .offset = {0, 0},
        .extent = context->swapchainItems.extent,
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
        .lineWidth                  = 2.0f,
        .cullMode                   = VK_CULL_MODE_BACK_BIT,
        .frontFace                  = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable            = VK_FALSE,
        .depthBiasConstantFactor    = 0.0f,
        .depthBiasClamp             = 0.0f,
        .depthBiasSlopeFactor       = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable    = VK_FALSE,
        .rasterizationSamples   = context->msaaSamples,
        .minSampleShading       = 1.0f,
        .pSampleMask            = nullptr,
        .alphaToCoverageEnable  = VK_FALSE,
        .alphaToOneEnable       = VK_FALSE,
    };

    // Note: This attachment is per framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment =
    {
        .colorWriteMask         =   VK_COLOR_COMPONENT_R_BIT 
                                    | VK_COLOR_COMPONENT_G_BIT 
                                    | VK_COLOR_COMPONENT_B_BIT 
                                    | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable            = VK_FALSE,
        .srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp           = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp           = VK_BLEND_OP_ADD, 
    };

    // Note: this is global
    VkPipelineColorBlendStateCreateInfo colorBlending =
    {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable      = VK_FALSE, // Note: enabling this disables per framebuffer method above,
        .logicOp            = VK_LOGIC_OP_COPY,
        .attachmentCount    = 1,
        .pAttachments       = &colorBlendAttachment,
        .blendConstants[0]  = 0.0f,
        .blendConstants[1]  = 0.0f,
        .blendConstants[2]  = 0.0f,
        .blendConstants[3]  = 0.0f,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil =
    { 
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

        .depthTestEnable    = VK_TRUE,
        .depthWriteEnable   = (VkBool32)options.enableDepth,
        .depthCompareOp     = VK_COMPARE_OP_LESS,

        // Note(Leo): These further limit depth range for this render pass
        .depthBoundsTestEnable  = VK_FALSE,
        .minDepthBounds         = 0.0f,
        .maxDepthBounds         = 1.0f,

        // Note(Leo): Configure stencil tests
        .stencilTestEnable  = VK_FALSE,
        .front              = {},
        .back               = {},
    };

    VkPushConstantRange pushConstantRange =
    {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset     = 0, 
        .size       = sizeof(Vector4[3])
    };

    auto materialLayout = vulkan::create_material_descriptor_set_layout(context->device, 0);
    VkDescriptorSetLayout descriptorSetLayouts [3]
    {
        context->descriptorSetLayouts.scene,
        materialLayout,
        context->descriptorSetLayouts.model,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 3,
        .pSetLayouts            = descriptorSetLayouts,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushConstantRange,
    };

    VkPipelineLayout layout;
    if (vkCreatePipelineLayout (context->device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

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
        .pDynamicState          = nullptr,
        .layout                 = layout,
        .renderPass             = context->renderPass,
        .subpass                = 0,

        // Note(Leo): These are for cheaper re-use of pipelines, not used right now.
        // Study(Leo): Like inheritance??
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };


    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    // Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
    vkDestroyShaderModule(context->device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(context->device, vertexShaderModule, nullptr);

    return {
        .info           = info,
        .pipeline       = pipeline,
        .layout         = layout,
        .materialLayout = materialLayout,
    };
}

internal VulkanLoadedPipeline
vulkan::make_gui_pipeline(
    VulkanContext * context,
    VulkanPipelineLoadInfo info)
{

    // Todo(Leo): unhardcode these
    BinaryAsset vertexShaderCode = ReadBinaryFile(info.vertexShaderPath.c_str());
    BinaryAsset fragmentShaderCode = ReadBinaryFile(info.fragmentShaderPath.c_str());

    VkShaderModule vertexShaderModule = vulkan::CreateShaderModule(vertexShaderCode, context->device);
    VkShaderModule fragmentShaderModule = vulkan::CreateShaderModule(fragmentShaderCode, context->device);

    auto & options = info.options;

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
        .vertexBindingDescriptionCount      = 0,
        .pVertexBindingDescriptions         = nullptr,
        .vertexAttributeDescriptionCount    = 0,
        .pVertexAttributeDescriptions       = nullptr,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = 
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewport =
    {
        .x          = 0.0f,
        .y          = 0.0f,
        .width      = (float) context->swapchainItems.extent.width,
        .height     = (float) context->swapchainItems.extent.height,
        .minDepth   = 0.0f,
        .maxDepth   = 1.0f,
    };

    VkRect2D scissor =
    {
        .offset = {0, 0},
        .extent = context->swapchainItems.extent,
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
        .lineWidth                  = 1.0f,
        .cullMode                   = VK_CULL_MODE_NONE,
        .frontFace                  = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable            = VK_FALSE,
        .depthBiasConstantFactor    = 0.0f,
        .depthBiasClamp             = 0.0f,
        .depthBiasSlopeFactor       = 0.0f,
    };

    VkPipelineMultisampleStateCreateInfo multisampling =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable    = VK_FALSE,
        .rasterizationSamples   = context->msaaSamples,
        .minSampleShading       = 1.0f,
        .pSampleMask            = nullptr,
        .alphaToCoverageEnable  = VK_FALSE,
        .alphaToOneEnable       = VK_FALSE,
    };

    // Note: This attachment is per framebuffer
    VkPipelineColorBlendAttachmentState colorBlendAttachment =
    {
        .colorWriteMask         =   VK_COLOR_COMPONENT_R_BIT 
                                    | VK_COLOR_COMPONENT_G_BIT 
                                    | VK_COLOR_COMPONENT_B_BIT 
                                    | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable            = VK_FALSE,
        .srcColorBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp           = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor    = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor    = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp           = VK_BLEND_OP_ADD, 
    };

    // Note: this is global
    VkPipelineColorBlendStateCreateInfo colorBlending =
    {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable      = VK_FALSE, // Note: enabling this disables per framebuffer method above,
        .logicOp            = VK_LOGIC_OP_COPY,
        .attachmentCount    = 1,
        .pAttachments       = &colorBlendAttachment,
        .blendConstants[0]  = 0.0f,
        .blendConstants[1]  = 0.0f,
        .blendConstants[2]  = 0.0f,
        .blendConstants[3]  = 0.0f,
    };

    VkPipelineDepthStencilStateCreateInfo depthStencil =
    { 
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,

        .depthTestEnable    = VK_TRUE,
        .depthWriteEnable   = VK_TRUE,//(VkBool32)options.enableDepth,
        .depthCompareOp     = VK_COMPARE_OP_LESS,

        // Note(Leo): These further limit depth range for this render pass
        .depthBoundsTestEnable  = VK_FALSE,
        .minDepthBounds         = 0.0f,
        .maxDepthBounds         = 1.0f,

        // Note(Leo): Configure stencil tests
        .stencilTestEnable  = VK_FALSE,
        .front              = {},
        .back               = {},
    };

    VkPushConstantRange pushConstantRange =
    {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .offset     = 0, 
        .size       = sizeof(Vector4[3])
    };

    auto materialLayout = vulkan::create_material_descriptor_set_layout(context->device, 1);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo =
    {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1,
        .pSetLayouts            = &materialLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges    = &pushConstantRange,
    };

    VkPipelineLayout layout;
    if (vkCreatePipelineLayout (context->device, &pipelineLayoutInfo, nullptr, &layout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout");
    }

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
        .pDynamicState          = nullptr,
        .layout                 = layout,
        .renderPass             = context->renderPass,
        .subpass                = 0,

        // Note(Leo): These are for cheaper re-use of pipelines, not used right now.
        // Study(Leo): Like inheritance??
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };


    VkPipeline pipeline;
    if (vkCreateGraphicsPipelines(context->device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline");
    }

    // Note: These can only be destroyed AFTER vkCreateGraphicsPipelines
    vkDestroyShaderModule(context->device, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(context->device, vertexShaderModule, nullptr);

    return {
        .info           = info,
        .pipeline       = pipeline,
        .layout         = layout,
        .materialLayout = materialLayout,
    };
}