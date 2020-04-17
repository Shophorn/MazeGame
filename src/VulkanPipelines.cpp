/*=============================================================================
Leo Tamminen
shophorn @ internet

Vulkan pipeline things separated to a different place, nothing too sensible yet.
Todo(Leo): These totally should be combined more/somehow
=============================================================================*/

namespace vulkan_pipelines_internal_
{
    internal VkVertexInputBindingDescription get_vertex_binding_description ();;
    internal StaticArray<VkVertexInputAttributeDescription, 6> get_vertex_attribute_description();

    internal VkPipelineDynamicStateCreateInfo make_dynamic_state(   platform::RenderingOptions * options,
                                                                    VkDynamicState(&array)[10]);
}

internal void
vulkan::destroy_loaded_pipeline(VulkanContext * context, VulkanLoadedPipeline * pipeline)
{
    vkDestroyPipeline(context->device, pipeline->pipeline, nullptr);
    vkDestroyPipelineLayout(context->device, pipeline->layout, nullptr);
    vkDestroyDescriptorSetLayout(context->device, pipeline->materialLayout, nullptr);
 
    pipeline->pipeline = VK_NULL_HANDLE;
    pipeline->layout = VK_NULL_HANDLE;
    pipeline->materialLayout = VK_NULL_HANDLE;
}

internal void
vulkan::recreate_loaded_pipeline(VulkanContext * context, VulkanLoadedPipeline * pipeline)
{
    destroy_loaded_pipeline(context, pipeline);
    *pipeline = make_pipeline(context, pipeline->info);
}

internal VulkanLoadedPipeline
vulkan::make_pipeline(
    VulkanContext * context,
    VulkanPipelineLoadInfo info)
{
    BinaryAsset vertexShaderCode = read_binary_file(info.vertexShaderPath.c_str());
    BinaryAsset fragmentShaderCode = read_binary_file(info.fragmentShaderPath.c_str());

    VkShaderModule vertexShaderModule = make_vk_shader_module(vertexShaderCode, context->device);
    VkShaderModule fragmentShaderModule = make_vk_shader_module(fragmentShaderCode, context->device);

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

    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    if (options.useVertexInput)
    {
        auto bindingDescription     = vulkan_pipelines_internal_::get_vertex_binding_description();
        auto attributeDescriptions  = vulkan_pipelines_internal_::get_vertex_attribute_description();

        vertexInputInfo = {
            .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount      = 1,
            .pVertexBindingDescriptions         = &bindingDescription,
            .vertexAttributeDescriptionCount    = attributeDescriptions.count(),
            .pVertexAttributeDescriptions       = attributeDescriptions.begin(),
        };
    }
    else
    {
        vertexInputInfo =  {
            .sType                              = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount      = 0,
            .pVertexBindingDescriptions         = nullptr,
            .vertexAttributeDescriptionCount    = 0,
            .pVertexAttributeDescriptions       = nullptr,
        };
    }


    VkPrimitiveTopology topology;
    switch(options.primitiveType)
    {
        case platform::RenderingOptions::PRIMITIVE_LINE:            topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
        case platform::RenderingOptions::PRIMITIVE_TRIANGLE:        topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
        case platform::RenderingOptions::PRIMITIVE_TRIANGLE_STRIP:  topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
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
        .width      = (float) context->drawingResources.extent.width,
        .height     = (float) context->drawingResources.extent.height,
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

    VkCullModeFlags cullMode;
    switch(options.cullMode)
    {
        case platform::RenderingOptions::CULL_NONE: cullMode = VK_CULL_MODE_NONE; break;
        case platform::RenderingOptions::CULL_BACK: cullMode = VK_CULL_MODE_BACK_BIT; break;
        case platform::RenderingOptions::CULL_FRONT: cullMode = VK_CULL_MODE_FRONT_BIT; break;
    }    

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

    auto materialLayout = make_material_vk_descriptor_set_layout(context->device, options.textureCount);

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
        .pushConstantRangeCount = pushConstantRangeCount,
        .pPushConstantRanges    = &pushConstantRange,
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

    return {
        .info           = info,
        .pipeline       = pipeline,
        .layout         = layout,
        .materialLayout = materialLayout
    };
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

internal StaticArray<VkVertexInputAttributeDescription, 6>
vulkan_pipelines_internal_::get_vertex_attribute_description()
{
    StaticArray<VkVertexInputAttributeDescription, 6> value = {};

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

    return value;
}   

internal VkPipelineDynamicStateCreateInfo
vulkan_pipelines_internal_::make_dynamic_state (platform::RenderingOptions * options, VkDynamicState (&array)[10])
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