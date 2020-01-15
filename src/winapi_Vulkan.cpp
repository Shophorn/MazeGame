/*=============================================================================
Leo Tamminen

Implementations of vulkan related functions
=============================================================================*/
#include "winapi_Vulkan.hpp"
#include "VulkanScene.cpp"
#include "VulkanDrawing.cpp"

internal uint32
vulkan::find_memory_type (VkPhysicalDevice physicalDevice, uint32 typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties (physicalDevice, &memoryProperties);

    for (int i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        bool32 filterMatch = (typeFilter & (1 << i)) != 0;
        bool32 memoryTypeOk = (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties;
        if (filterMatch && memoryTypeOk)
        {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type.");
}   

// Todo(Leo): see if we can change this to proper functional function
internal void
vulkan::CreateBuffer(
    VkDevice                logicalDevice,
    VkPhysicalDevice        physicalDevice,
    VkDeviceSize            bufferSize,
    VkBufferUsageFlags      usage,
    VkMemoryPropertyFlags   memoryProperties,
    VkBuffer *              resultBuffer,
    VkDeviceMemory *        resultBufferMemory
){
    VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
    bufferInfo.size = bufferSize;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.flags = 0;

    if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, resultBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create buffer");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(logicalDevice, *resultBuffer, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = vulkan::find_memory_type(
                                        physicalDevice,
                                        memoryRequirements.memoryTypeBits,
                                        memoryProperties);

    std::cout << "memoryTypeIndex : " << allocateInfo.memoryTypeIndex << "\n";

    /* Todo(Leo): do not actually always use new allocate, but instead allocate
    once and create allocator to handle multiple items */
    if (vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, resultBufferMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate buffer memory");
    }

    vkBindBufferMemory(logicalDevice, *resultBuffer, *resultBufferMemory, 0); 
}

internal VulkanBufferResource
vulkan::make_buffer_resource(
    VulkanContext *         context,
    VkDeviceSize            size,
    VkBufferUsageFlags      usage,
    VkMemoryPropertyFlags   memoryProperties)
{
    VulkanBufferResource result = {};

    // TODO(Leo): This is reminder to remove this system. It used to be so that result was passed with an out pointer
    if (result.created == true)
    {
        // return;
    }
    
    // TODO(Leo): inline this
    vulkan::CreateBuffer(   context->device, context->physicalDevice, size,
                            usage, memoryProperties, &result.buffer, &result.memory);

    result.size = size;
    result.created = true;
    return result;
}

internal void
vulkan::destroy_buffer_resource(VkDevice logicalDevice, VulkanBufferResource * resource)
{
    vkDestroyBuffer(logicalDevice, resource->buffer, nullptr);
    vkFreeMemory(logicalDevice, resource->memory, nullptr);

    resource->created = false;
}

internal VkVertexInputBindingDescription
vulkan::get_vertex_binding_description ()
{
    return {
        .binding    = 0,
        .stride     = sizeof(Vertex),
        .inputRate  = VK_VERTEX_INPUT_RATE_VERTEX
    };
}

internal Array<VkVertexInputAttributeDescription, 4>
vulkan::get_vertex_attribute_description()
{
	Array<VkVertexInputAttributeDescription, 4> value = {};

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

	return value;
}	

internal VkIndexType
vulkan::convert_index_type(IndexType type)
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

internal VulkanSwapchainSupportDetails
vulkan::QuerySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VulkanSwapchainSupportDetails result = {};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &result.capabilities);

    uint32 formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (formatCount > 0)
    {
        result.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, result.formats.data());
    }

    // std::cout << "physicalDevice surface formats count: " << formatCount << "\n";

    uint32 presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount > 0)
    {
        result.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, result.presentModes.data());
    }

    return result;
}

internal VkSurfaceFormatKHR
vulkan::ChooseSwapSurfaceFormat(std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    constexpr VkFormat preferredFormat = VK_FORMAT_R8G8B8A8_UNORM;
    constexpr VkColorSpaceKHR preferredColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    VkSurfaceFormatKHR result = availableFormats [0];
    for (int i = 0; i < availableFormats.size(); ++i)
    {
        if (availableFormats[i].format == preferredFormat && availableFormats[i].colorSpace == preferredColorSpace)
        {
            result = availableFormats [i];
        }   
    }
    return result;
}

internal VkPresentModeKHR
vulkan::ChooseSurfacePresentMode(std::vector<VkPresentModeKHR> & availablePresentModes)
{
    // Todo(Leo): Is it really preferred???
    constexpr VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    VkPresentModeKHR result = VK_PRESENT_MODE_FIFO_KHR;
    for (int i = 0; i < availablePresentModes.size(); ++i)
    {
        if (availablePresentModes[i] == preferredPresentMode)
        {
            result = availablePresentModes[i];
        }
    }
    return result;
}


VulkanQueueFamilyIndices
vulkan::FindQueueFamilies (VkPhysicalDevice device, VkSurfaceKHR surface)
{
    // Note: A card must also have a graphics queue family available; We do want to draw after all
    VkQueueFamilyProperties queueFamilyProps [50];
    uint32 queueFamilyCount = ARRAY_COUNT(queueFamilyProps);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProps);

    // std::cout << "queueFamilyCount: " << queueFamilyCount << "\n";

    bool32 properQueueFamilyFound = false;
    VulkanQueueFamilyIndices result = {};
    for (int i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilyProps[i].queueCount > 0 && (queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            result.graphics = i;
            result.hasGraphics = true;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if (queueFamilyProps[i].queueCount > 0 && presentSupport)
        {
            result.present = i;
            result.hasPresent = true;
        }

        if (result.hasAll())
        {
            break;
        }
    }
    return result;
}


VkShaderModule
vulkan::CreateShaderModule(BinaryAsset code, VkDevice logicalDevice)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<uint32 *>(code.data());

    VkShaderModule result;
    if (vkCreateShaderModule (logicalDevice, &createInfo, nullptr, &result) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create shader module");
    }

    return result;
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

    enum : uint32
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
        case platform::PipelineOptions::PRIMITIVE_LINE:     topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
        case platform::PipelineOptions::PRIMITIVE_TRIANGLE: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
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

    enum : uint32
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
        case platform::PipelineOptions::PRIMITIVE_LINE:     topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
        case platform::PipelineOptions::PRIMITIVE_TRIANGLE: topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; break;
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

internal void
vulkan::destroy_pipeline(VulkanContext * context, VulkanLoadedPipeline * pipeline)
{
    vkDestroyPipeline(context->device, pipeline->pipeline, nullptr);
    pipeline->layout = VK_NULL_HANDLE;
 
    vkDestroyPipelineLayout(context->device, pipeline->layout, nullptr);
    pipeline->pipeline = VK_NULL_HANDLE;

    vkDestroyDescriptorSetLayout(context->device, pipeline->materialLayout, nullptr);
    pipeline->materialLayout = VK_NULL_HANDLE;
}

/******************************************************************************

Todo[Vulkan](Leo):
AFTER THIS THESE FUNCTIONS ARE JUST YEETED HERE, SET THEM PROPERLY IN 'vulkan'
NAMESPACE AND SPLIT TO MULTIPLE FILES IF APPROPRIATE


*****************************************************************************/

internal VkSampleCountFlagBits 
GetMaxUsableMsaaSampleCount (VkPhysicalDevice physicalDevice)
{
    // Todo(Leo): to be easier on machine when developing for 2 players at same time
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = std::min(
            physicalDeviceProperties.limits.framebufferColorSampleCounts,
            physicalDeviceProperties.limits.framebufferDepthSampleCounts
        );

    VkSampleCountFlagBits result;

    if (counts & VK_SAMPLE_COUNT_64_BIT) { result = VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { result = VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { result = VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { result = VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { result = VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { result = VK_SAMPLE_COUNT_2_BIT; }

    if (result > VULKAN_MAX_MSAA_SAMPLE_COUNT)
    {
        result = VULKAN_MAX_MSAA_SAMPLE_COUNT;
    }

    return result;
}

// Todo(Leo): put into namespace, I already forgot where this was :)
internal VkInstance
CreateInstance()
{
    auto CheckValidationLayerSupport = [] () -> bool32
    {
        VkLayerProperties availableLayers [50];
        uint32 availableLayersCount = ARRAY_COUNT(availableLayers);

        bool32 result = true;
        if (vkEnumerateInstanceLayerProperties (&availableLayersCount, availableLayers) == VK_SUCCESS)
        {

            for (
                int validationLayerIndex = 0;
                validationLayerIndex < vulkan::VALIDATION_LAYERS_COUNT;
                ++validationLayerIndex)
            {
                bool32 layerFound = false;
                for(
                    int availableLayerIndex = 0; 
                    availableLayerIndex < availableLayersCount; 
                    ++availableLayerIndex)
                {
                    if (strcmp (vulkan::validationLayers[validationLayerIndex], availableLayers[availableLayerIndex].layerName) == 0)
                    {
                        layerFound = true;
                        break;
                    }
                }

                if (layerFound == false)
                {
                    result = false;
                    break;
                }
            }
        }

        return result;
    };

    if (vulkan::enableValidationLayers && CheckValidationLayerSupport() == false)
    {
        throw std::runtime_error("Validation layers required but not present");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan practice";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    instanceInfo.enabledExtensionCount = 2;
    const char * extensions [] = {"VK_KHR_surface", "VK_KHR_win32_surface"}; 
    instanceInfo.ppEnabledExtensionNames = extensions;

    if constexpr (vulkan::enableValidationLayers)
    {
        instanceInfo.enabledLayerCount = vulkan::VALIDATION_LAYERS_COUNT;
        instanceInfo.ppEnabledLayerNames = vulkan::validationLayers;
    }
    else
    {
        instanceInfo.enabledLayerCount = 0;
    }

    VkInstance instance;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
    {
        // Todo: Bad stuff happened, abort
    }

    return instance;
}

internal VkPhysicalDevice
PickPhysicalDevice(VkInstance vulkanInstance, VkSurfaceKHR surface)
{
    auto CheckDeviceExtensionSupport = [] (VkPhysicalDevice testDevice) -> bool32
    {
        VkExtensionProperties availableExtensions [100];
        uint32 availableExtensionsCount = ARRAY_COUNT(availableExtensions);
        vkEnumerateDeviceExtensionProperties (testDevice, nullptr, &availableExtensionsCount, availableExtensions);

        bool32 result = true;
        for (int requiredIndex = 0;
            requiredIndex < vulkan::DEVICE_EXTENSION_COUNT;
            ++requiredIndex)
        {

            bool32 requiredExtensionFound = false;
            for (int availableIndex = 0;
                availableIndex < availableExtensionsCount;
                ++availableIndex)
            {
                if (strcmp(vulkan::deviceExtensions[requiredIndex], availableExtensions[availableIndex].extensionName) == 0)
                {
                    requiredExtensionFound = true;
                    break;
                }
            }

            result = requiredExtensionFound;
            if (result == false)
            {
                break;
            }
        }

        return result;
    };

    auto IsPhysicalDeviceSuitable = [CheckDeviceExtensionSupport] (VkPhysicalDevice testDevice, VkSurfaceKHR surface) -> bool32
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(testDevice, &props);
        bool32 isDedicatedGPU = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures(testDevice, &features);

        bool32 extensionsAreSupported = CheckDeviceExtensionSupport(testDevice);

        bool32 swapchainIsOk = false;
        if (extensionsAreSupported)
        {
            VulkanSwapchainSupportDetails swapchainSupport = vulkan::QuerySwapChainSupport(testDevice, surface);
            swapchainIsOk = !swapchainSupport.formats.empty() && !swapchainSupport.presentModes.empty();
        }

        VulkanQueueFamilyIndices indices = vulkan::FindQueueFamilies(testDevice, surface);
        return  isDedicatedGPU 
                && indices.hasAll()
                && extensionsAreSupported
                && swapchainIsOk
                && features.samplerAnisotropy;
    };

    VkPhysicalDevice resultPhysicalDevice;


    VkPhysicalDevice devices [10];
    uint32 deviceCount = ARRAY_COUNT(devices);
    vkEnumeratePhysicalDevices(vulkanInstance, &deviceCount, devices);

    if (deviceCount == 0)
    {
        throw std::runtime_error("No GPU devices found");
    }

    for (int i = 0; i < deviceCount; i++)
    {
        if (IsPhysicalDeviceSuitable(devices[i], surface))
        {
            resultPhysicalDevice = devices[i];
            break;
        }
    }

    if (resultPhysicalDevice == VK_NULL_HANDLE)
    {
        throw std::runtime_error("No suitable GPU device found");
    }

    return resultPhysicalDevice;
}

internal VkDevice
CreateLogicalDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VulkanQueueFamilyIndices queueIndices = vulkan::FindQueueFamilies(physicalDevice, surface);
    
    /* Note: We need both graphics and present queue, but they might be on
    separate devices (correct???), so we may need to end up with multiple queues */
    int uniqueQueueFamilyCount = queueIndices.graphics == queueIndices.present ? 1 : 2;
    VkDeviceQueueCreateInfo queueCreateInfos [2] = {};
    float queuePriorities[/*queueCount*/] = { 1.0f };
    for (int i = 0; i <uniqueQueueFamilyCount; ++i)
    {
        // VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfos[i].queueFamilyIndex = queueIndices.getAt(i);
        queueCreateInfos[i].queueCount = 1;

        queueCreateInfos[i].pQueuePriorities = queuePriorities;
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures = {};
    physicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
    deviceCreateInfo.enabledExtensionCount = vulkan::DEVICE_EXTENSION_COUNT;
    deviceCreateInfo.ppEnabledExtensionNames = vulkan::deviceExtensions;

    if constexpr (vulkan::enableValidationLayers)
    {
        deviceCreateInfo.enabledLayerCount = vulkan::VALIDATION_LAYERS_COUNT;
        deviceCreateInfo.ppEnabledLayerNames = vulkan::validationLayers;
    }
    else
    {
        deviceCreateInfo.enabledLayerCount = 0;
    }

    VkDevice resultLogicalDevice;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo,nullptr, &resultLogicalDevice) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create vulkan logical device");
    }

    return resultLogicalDevice;
}

internal VkImage
CreateImage(
    VkDevice logicalDevice, 
    VkPhysicalDevice physicalDevice,
    uint32 texWidth,
    uint32 texHeight,
    uint32 mipLevels,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkSampleCountFlagBits msaaSamples
){
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };

    imageInfo.imageType     = VK_IMAGE_TYPE_2D;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE; // note(leo): concerning queue families
    imageInfo.arrayLayers   = 1;

    imageInfo.extent        = { texWidth, texHeight, 1 };
    imageInfo.mipLevels     = mipLevels;
    imageInfo.format        = format;
    imageInfo.tiling        = tiling;
    imageInfo.usage         = usage;
    imageInfo.samples       = msaaSamples;

    imageInfo.flags         = 0;

    VkImage resultImage;
    if (vkCreateImage(logicalDevice, &imageInfo, nullptr, &resultImage) != VK_SUCCESS)
    {
        throw std::runtime_error ("Failed to create image");
    }
    return resultImage;
}

internal void
create_image_and_memory(
    VkDevice logicalDevice, 
    VkPhysicalDevice physicalDevice,
    uint32 texWidth,
    uint32 texHeight,
    uint32 mipLevels,
    VkFormat format,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags memoryFlags,
    VkSampleCountFlagBits msaaSamples,
    VkImage * resultImage,
    VkDeviceMemory * resultImageMemory)
{
    VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = { texWidth, texHeight, 1 };
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;

    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // note(leo): concerning queue families
    imageInfo.samples = msaaSamples;
    imageInfo.flags = 0;

    // Note(Leo): some image formats may not be supported
    if (vkCreateImage(logicalDevice, &imageInfo, nullptr, resultImage) != VK_SUCCESS)
    {
        throw std::runtime_error ("Failed to create image");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements (logicalDevice, *resultImage, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = vulkan::find_memory_type (
                                    physicalDevice,
                                    memoryRequirements.memoryTypeBits,
                                    memoryFlags);

    if (vkAllocateMemory(logicalDevice, &allocateInfo, nullptr, resultImageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate image memory");
    }

    vkBindImageMemory(logicalDevice, *resultImage, *resultImageMemory, 0);   
}


internal VkImageView
create_image_view(
    VkDevice logicalDevice,
    VkImage image,
    uint32 mipLevels,
    VkFormat format,
    VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo imageViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    imageViewInfo.image = image;
    imageViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewInfo.format = format;

    imageViewInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewInfo.subresourceRange.baseMipLevel = 0;
    imageViewInfo.subresourceRange.levelCount = mipLevels;
    imageViewInfo.subresourceRange.baseArrayLayer = 0;
    imageViewInfo.subresourceRange.layerCount = 1;

    imageViewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    VkImageView result;
    if (vkCreateImageView(logicalDevice, &imageViewInfo, nullptr, &result) != VK_SUCCESS)
    {
        throw std::runtime_error ("Failed to create image view");
    }

    return result;
}


internal VulkanSwapchainItems
CreateSwapchainAndImages(VulkanContext * context, VkExtent2D frameBufferSize)
{
    VulkanSwapchainItems resultSwapchain = {};

    VulkanSwapchainSupportDetails swapchainSupport = vulkan::QuerySwapChainSupport(context->physicalDevice, context->surface);

    VkSurfaceFormatKHR surfaceFormat = vulkan::ChooseSwapSurfaceFormat(swapchainSupport.formats);
    VkPresentModeKHR presentMode = vulkan::ChooseSurfacePresentMode(swapchainSupport.presentModes);

    // Find extent ie. size of drawing window
    /* Note(Leo): max value is special value denoting that all are okay.
    Or something else, so we need to ask platform */
    if (swapchainSupport.capabilities.currentExtent.width != MaxValue<uint32>)
    {
        resultSwapchain.extent = swapchainSupport.capabilities.currentExtent;
    }
    else
    {   
        VkExtent2D min = swapchainSupport.capabilities.minImageExtent;
        VkExtent2D max = swapchainSupport.capabilities.maxImageExtent;

        resultSwapchain.extent.width = clamp(static_cast<uint32>(frameBufferSize.width), min.width, max.width);
        resultSwapchain.extent.height = clamp(static_cast<uint32>(frameBufferSize.height), min.height, max.height);
    }

    std::cout << "Creating swapchain actually, width: " << resultSwapchain.extent.width << ", height: " << resultSwapchain.extent.height << "\n";

    uint32 imageCount = swapchainSupport.capabilities.minImageCount + 1;
    uint32 maxImageCount = swapchainSupport.capabilities.maxImageCount;
    if (maxImageCount > 0 && imageCount > maxImageCount)
    {
        imageCount = maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = context->surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = resultSwapchain.extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VulkanQueueFamilyIndices queueIndices = vulkan::FindQueueFamilies(context->physicalDevice, context->surface);
    uint32 queueIndicesArray [2] = {queueIndices.graphics, queueIndices.present};

    if (queueIndices.graphics == queueIndices.present)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueIndicesArray;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(context->device, &createInfo, nullptr, &resultSwapchain.swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create swap chain");
    }

    resultSwapchain.imageFormat = surfaceFormat.format;


    // Bug(Leo): following imageCount variable is reusing one from previous definition, maybe bug
    // Note(Leo): Swapchain images are not created, they are gotten from api
    vkGetSwapchainImagesKHR(context->device, resultSwapchain.swapchain, &imageCount, nullptr);
    resultSwapchain.images.resize (imageCount);
    vkGetSwapchainImagesKHR(context->device, resultSwapchain.swapchain, &imageCount, &resultSwapchain.images[0]);

    std::cout << "Created swapchain and images\n";
    
    resultSwapchain.imageViews.resize(imageCount);

    for (int i = 0; i < imageCount; ++i)
    {
        resultSwapchain.imageViews[i] = create_image_view(
            context->device, resultSwapchain.images[i], 1, 
            resultSwapchain.imageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    std::cout << "Created image views\n";

    return resultSwapchain;
}

internal VkFormat
FindSupportedFormat(
    VkPhysicalDevice physicalDevice,
    int32 candidateCount,
    VkFormat * pCandidates,
    VkImageTiling requestedTiling,
    VkFormatFeatureFlags requestedFeatures
){
    bool32 requestOptimalTiling = requestedTiling == VK_IMAGE_TILING_OPTIMAL;

    for (VkFormat * pFormat = pCandidates; pFormat != pCandidates + candidateCount; ++pFormat)
    {
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, *pFormat, &properties);

        VkFormatFeatureFlags features = requestOptimalTiling ? 
            properties.optimalTilingFeatures : properties.linearTilingFeatures;    

        if ((features & requestedFeatures) == requestedFeatures)
        {
            return *pFormat;
        }
    }

    throw std::runtime_error("Failed to find supported format");
}

internal VkFormat
FindSupportedDepthFormat(VkPhysicalDevice physicalDevice)   
{
    VkFormat formats [] = { VK_FORMAT_D32_SFLOAT,
                            VK_FORMAT_D32_SFLOAT_S8_UINT,
                            VK_FORMAT_D24_UNORM_S8_UINT };
    int32 formatCount = 3;


    VkFormat result = FindSupportedFormat(
                        physicalDevice, formatCount, formats, VK_IMAGE_TILING_OPTIMAL, 
                        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    return result;
}

internal VkRenderPass
CreateRenderPass(VulkanContext * context, VulkanSwapchainItems * swapchainItems, VkSampleCountFlagBits msaaSamples)
{
    constexpr int 
        COLOR_ATTACHMENT_ID     = 0,
        DEPTH_ATTACHMENT_ID     = 1,
        RESOLVE_ATTACHMENT_ID   = 2,
        ATTACHMENT_COUNT        = 3;

    VkAttachmentDescription attachments[ATTACHMENT_COUNT] = {};

    /*
    Note(Leo): We render internally to color attachment and depth attachment
    using multisampling. After that final image is compiled to 'resolve'
    attachment that is image from swapchain and present that
    */
    attachments[COLOR_ATTACHMENT_ID].format         = swapchainItems->imageFormat;
    attachments[COLOR_ATTACHMENT_ID].samples        = msaaSamples;
    attachments[COLOR_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[COLOR_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[COLOR_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[COLOR_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[COLOR_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[COLOR_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[DEPTH_ATTACHMENT_ID].format         = FindSupportedDepthFormat(context->physicalDevice);
    attachments[DEPTH_ATTACHMENT_ID].samples        = msaaSamples;
    attachments[DEPTH_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[DEPTH_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[DEPTH_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[DEPTH_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[DEPTH_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[DEPTH_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachments[RESOLVE_ATTACHMENT_ID].format         = swapchainItems->imageFormat;
    attachments[RESOLVE_ATTACHMENT_ID].samples        = VK_SAMPLE_COUNT_1_BIT;
    attachments[RESOLVE_ATTACHMENT_ID].loadOp         = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[RESOLVE_ATTACHMENT_ID].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[RESOLVE_ATTACHMENT_ID].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[RESOLVE_ATTACHMENT_ID].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[RESOLVE_ATTACHMENT_ID].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[RESOLVE_ATTACHMENT_ID].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    constexpr int32 COLOR_ATTACHMENT_COUNT = 1;        
    VkAttachmentReference colorAttachmentRefs[COLOR_ATTACHMENT_COUNT] = {};
    colorAttachmentRefs[0].attachment = COLOR_ATTACHMENT_ID;
    colorAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Note(Leo): there can be only one depth attachment
    VkAttachmentReference depthStencilAttachmentRef = {};
    depthStencilAttachmentRef.attachment = DEPTH_ATTACHMENT_ID;
    depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveAttachmentRefs [COLOR_ATTACHMENT_COUNT] = {};
    resolveAttachmentRefs[0].attachment = RESOLVE_ATTACHMENT_ID;
    resolveAttachmentRefs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpasses[1] = {};
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = COLOR_ATTACHMENT_COUNT;
    subpasses[0].pColorAttachments = &colorAttachmentRefs[0];
    subpasses[0].pResolveAttachments = &resolveAttachmentRefs[0];
    subpasses[0].pDepthStencilAttachment = &depthStencilAttachmentRef;

    // Note(Leo): subpass dependencies
    VkSubpassDependency dependencies[1] = {};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = 0;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].dstAccessMask = 0;//VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
    renderPassInfo.attachmentCount = ATTACHMENT_COUNT;
    renderPassInfo.pAttachments = &attachments[0];
    renderPassInfo.subpassCount = ARRAY_COUNT(subpasses);
    renderPassInfo.pSubpasses = &subpasses[0];
    renderPassInfo.dependencyCount = ARRAY_COUNT(dependencies);
    renderPassInfo.pDependencies = &dependencies[0];

    VkRenderPass resultRenderPass;
    if (vkCreateRenderPass(context->device, &renderPassInfo, nullptr, &resultRenderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass");
    }

    return resultRenderPass;
}

/* Note(leo): these are fixed per available renderpipeline thing. They describe
'kinds' of resources or something, so their amount does not change

IMPORTANT(Leo): These must be same in shaders
*/
enum : uint32
{
    DESCRIPTOR_LAYOUT_SCENE_BINDING     = 0,
    DESCRIPTOR_LAYOUT_MODEL_BINDING     = 0,
    DESCRIPTOR_LAYOUT_SAMPLER_BINDING   = 0,
};

internal VkDescriptorSetLayout
CreateModelDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding binding =
    {
        .binding            = DESCRIPTOR_LAYOUT_MODEL_BINDING,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo =
    { 
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount   = 1,
        .pBindings      = &binding,
    };

    VkDescriptorSetLayout resultDescriptorSetLayout;

    if(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create the descriptor set layout");
    }

    return resultDescriptorSetLayout;
}

internal VkDescriptorSetLayout
vulkan::create_material_descriptor_set_layout(VkDevice device, uint32 textureCount)
{
    VkDescriptorSetLayoutBinding binding = 
    {
        .binding             = DESCRIPTOR_LAYOUT_SAMPLER_BINDING,
        .descriptorCount     = textureCount,
        .descriptorType      = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags          = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers  = nullptr,
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo =
    { 
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount   = 1,
        .pBindings      = &binding,
    };

    VkDescriptorSetLayout resultDescriptorSetLayout;
    if(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create the descriptor set layout");
    }
    return resultDescriptorSetLayout;
}

internal VkDescriptorSetLayout
CreateSceneDescriptorSetLayout(VkDevice device)
{
    VkDescriptorSetLayoutBinding binding =
    {
        .binding            = DESCRIPTOR_LAYOUT_SCENE_BINDING,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr, // Note(Leo): relevant for sampler stuff, like textures,
    };

    VkDescriptorSetLayoutCreateInfo layoutCreateInfo =
    { 
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount   = 1,
        .pBindings      = &binding,
    };

    VkDescriptorSetLayout resultDescriptorSetLayout;

    if(vkCreateDescriptorSetLayout(device, &layoutCreateInfo, nullptr, &resultDescriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create the descriptor set layout");
    }

    return resultDescriptorSetLayout;
}



// Change image layout from stored pixel array layout to device optimal layout
internal void
cmd_transition_image_layout(
    VkCommandBuffer commandBuffer,
    VkDevice        device,
    VkQueue         graphicsQueue,
    VkImage         image,
    VkFormat        format,
    uint32          mipLevels,
    VkImageLayout   oldLayout,
    VkImageLayout   newLayout,
    uint32          layerCount = 1)
{
    VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
    barrier.oldLayout = oldLayout; // Note(Leo): Can be VK_IMAGE_LAYOUT_UNDEFINED if we dont care??
    barrier.newLayout = newLayout;

    /* Note(Leo): these are used if we are to change queuefamily ownership.
    Otherwise must be set to 'VK_QUEUE_FAMILY_IGNORED' */
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = image;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (vulkan::FormatHasStencilComponent(format))
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    /* Todo(Leo): This is ultimate stupid, we rely on knowing all this based
    on two variables, instead rather pass more arguments, or struct, or a 
    index to lookup table or a struct from that lookup table.

    This function should at most handle the command buffer part instead of
    quessing omitted values.
    */

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED 
        && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL 
            && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    // DEPTH IMAGE
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
            && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =  VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT 
                                | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    // COLOR IMAGE
    else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED
            && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                                | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }


    else
    {
        throw std::runtime_error("This layout transition is not supported!");
    }

    VkDependencyFlags dependencyFlags = 0;
    vkCmdPipelineBarrier(   commandBuffer,
                            sourceStage, destinationStage,
                            dependencyFlags,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
}

// Todo(Leo): Remove this
internal void
TransitionImageLayout(
    VkDevice        device,
    VkCommandPool   commandPool,
    VkQueue         graphicsQueue,
    VkImage         image,
    VkFormat        format,
    uint32          mipLevels,
    VkImageLayout   oldLayout,
    VkImageLayout   newLayout)
{
    VkCommandBuffer commandBuffer = vulkan::begin_command_buffer(device, commandPool);

    cmd_transition_image_layout(commandBuffer, device, graphicsQueue, image, format, mipLevels, oldLayout, newLayout);

    vulkan::execute_command_buffer (device, commandPool, graphicsQueue, commandBuffer);
}


internal void
vulkan::create_drawing_resources(VulkanContext * context)
{
    VulkanDrawingResources resultResources = {};

    VkFormat colorFormat = context->swapchainItems.imageFormat;
    resultResources.colorImage = CreateImage(  context->device, context->physicalDevice,
                                                context->swapchainItems.extent.width, context->swapchainItems.extent.height,
                                                1, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                                                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                context->msaaSamples);

    VkFormat depthFormat = FindSupportedDepthFormat(context->physicalDevice);
    resultResources.depthImage = CreateImage(  context->device, context->physicalDevice,
                                                context->swapchainItems.extent.width, context->swapchainItems.extent.height,
                                                1, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                                context->msaaSamples);

    VkMemoryRequirements colorMemoryRequirements, depthMemoryRequirements;
    vkGetImageMemoryRequirements(context->device, resultResources.colorImage, &colorMemoryRequirements);
    vkGetImageMemoryRequirements(context->device, resultResources.depthImage, &depthMemoryRequirements);


    // Todo(Leo): Do these need to be aligned like below????
    uint64 colorMemorySize = colorMemoryRequirements.size;
    uint64 depthMemorySize = depthMemoryRequirements.size;
    // uint64 colorMemorySize = align_up_to(colorMemoryRequirements.alignment, colorMemoryRequirements.size);
    // uint64 depthMemorySize = align_up_to(depthMemoryRequirements.alignment, depthMemoryRequirements.size);

    uint32 memoryTypeIndex = vulkan::find_memory_type(
                                context->physicalDevice,
                                colorMemoryRequirements.memoryTypeBits | depthMemoryRequirements.memoryTypeBits,
                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkMemoryAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = (colorMemorySize + depthMemorySize);
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    if (vkAllocateMemory(context->device, &allocateInfo, nullptr, &resultResources.memory))
    {
        throw std::runtime_error("Failed to allocate drawing resource memory");
    }            

    // Todo(Leo): Result check binding memories
    vkBindImageMemory(context->device, resultResources.colorImage, resultResources.memory, 0);
    vkBindImageMemory(context->device, resultResources.depthImage, resultResources.memory, colorMemorySize);

    resultResources.colorImageView = create_image_view(  context->device, resultResources.colorImage,
                                                        1, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);

    resultResources.depthImageView = create_image_view(  context->device, resultResources.depthImage,
                                                        1, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

    TransitionImageLayout(  context->device, context->commandPool, context->graphicsQueue, resultResources.colorImage, colorFormat, 1,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    TransitionImageLayout(  context->device, context->commandPool, context->graphicsQueue, resultResources.depthImage, depthFormat, 1,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    context->drawingResources = resultResources;
}

internal void
vulkan::create_uniform_descriptor_pool(VulkanContext * context)
{
    /*
    Note(Leo): 
    There needs to only one per type, not one per user

    We create a single big buffer and then use offsets to divide it to smaller chunks

    'count' is one for actor (characters, animated scenery) uniform buffers which are dynamic and 
    one for static scenery.
    */
    constexpr int32 count = 2;
    VkDescriptorPoolSize poolSizes [count] = {};

    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[0].descriptorCount = 1; // Note(Leo): 2 = 1 for 3d models and 1 for gui 

    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
    poolInfo.poolSizeCount = count;
    poolInfo.pPoolSizes = &poolSizes[0];
    poolInfo.maxSets = 20;

    DEVELOPMENT_ASSERT(
        vkCreateDescriptorPool(context->device, &poolInfo, nullptr, &context->uniformDescriptorPool) == VK_SUCCESS,
        "Failed to create descriptor pool");
}

internal void
vulkan::create_material_descriptor_pool(VulkanContext * context)
{
    VkDescriptorPoolSize poolSize =
    {
        .type               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = vulkan::MAX_LOADED_TEXTURES,
    };

    VkDescriptorPoolCreateInfo poolCreateInfo = 
    {
        .sType          = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount  = 1,
        .pPoolSizes     = &poolSize,
        .maxSets        = vulkan::MAX_LOADED_TEXTURES,
    };

    DEVELOPMENT_ASSERT(
        vkCreateDescriptorPool(context->device, &poolCreateInfo, nullptr, &context->materialDescriptorPool) == VK_SUCCESS,
        "Failed to create descriptor pool for materials!");
}

internal void
vulkan::create_model_descriptor_sets(VulkanContext * context)
{
    VkDescriptorSetAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = context->uniformDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &context->descriptorSetLayouts.model,
    };

    VkDescriptorSet resultDescriptorSet;
    if (vkAllocateDescriptorSets(context->device, &allocateInfo, &resultDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
    }

    VkDescriptorBufferInfo modelBufferInfo =
    {
        .buffer = context->modelUniformBuffer.buffer,
        .offset = 0,

        // Todo(Leo): Align Align, this works now because matrix44 happens to fit 64 bytes.
        .range = sizeof(Matrix44),
    };

    VkWriteDescriptorSet descriptorWrite =
    {
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultDescriptorSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_MODEL_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount    = 1,
        .pBufferInfo        = &modelBufferInfo,
    };

    vkUpdateDescriptorSets(context->device, 1, &descriptorWrite, 0, nullptr);
    context->uniformDescriptorSets.model = resultDescriptorSet;
}

internal void
vulkan::create_scene_descriptor_sets(VulkanContext * context)
{
    VkDescriptorSetAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = context->uniformDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &context->descriptorSetLayouts.scene,
    };

    VkDescriptorSet resultDescriptorSet;
    if (vkAllocateDescriptorSets(context->device, &allocateInfo, &resultDescriptorSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
    }

    VkDescriptorBufferInfo sceneBufferInfo =
    {
        .buffer = context->sceneUniformBuffer.buffer,
        .offset = 0,
        .range = sizeof(VulkanCameraUniformBufferObject),
    };

    VkWriteDescriptorSet descriptorWrite =
    {
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultDescriptorSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_SCENE_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .pBufferInfo        = &sceneBufferInfo,
    };

    vkUpdateDescriptorSets(context->device, 1, &descriptorWrite, 0, nullptr);
    context->uniformDescriptorSets.scene = resultDescriptorSet;
}

internal VkDescriptorSet
vulkan::make_material_descriptor_set(
    VulkanContext * context,
    PipelineHandle pipelineHandle,
    ArenaArray<TextureHandle> textures)
{
    uint32 textureCount = context->loadedPipelines[pipelineHandle].info.options.textureCount;
    constexpr uint32 maxTextures = 10;
    
    DEVELOPMENT_ASSERT(textureCount < maxTextures, "Too many textures on material");
    DEVELOPMENT_ASSERT(textureCount == textures.count(), "Wrong number of textures in ArenaArray 'textures'");

    VkDescriptorSetAllocateInfo allocateInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = context->materialDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts        = &context->loadedPipelines[pipelineHandle].materialLayout,
    };

    VkDescriptorSet resultSet;
    if (vkAllocateDescriptorSets(context->device, &allocateInfo, &resultSet) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate DESCRIPTOR SETS");
    }

    VkDescriptorImageInfo samplerInfos [maxTextures];
    for (int i = 0; i < textureCount; ++i)
    {
        samplerInfos[i] = 
        {
            .sampler        = context->textureSampler,
            .imageView      = context->loadedTextures[textures[i]].view,
            .imageLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
    }

    VkWriteDescriptorSet writing =
    {  
        .sType              = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet             = resultSet,
        .dstBinding         = DESCRIPTOR_LAYOUT_SAMPLER_BINDING,
        .dstArrayElement    = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount    = textureCount,
        .pImageInfo         = samplerInfos,
    };

    // Note(Leo): Two first are write info, two latter are copy info
    vkUpdateDescriptorSets(context->device, 1, &writing, 0, nullptr);

    return resultSet;
}

// internal void
// create_frame_buffers(VulkanContext * context)
// { 
//     /* Note(Leo): This is basially allocating right, there seems to be no
//     need for VkDeviceMemory for swapchainimages??? */
    
//     int imageCount = context->swapchainItems.imageViews.size();
//     context->frameBuffers.resize(imageCount);

//     for (int i = 0; i < imageCount; ++i)
//     {
//         constexpr int ATTACHMENT_COUNT = 3;
//         VkImageView attachments[ATTACHMENT_COUNT] = {
//             context->drawingResources.colorImageView,
//             context->drawingResources.depthImageView,
//             context->swapchainItems.imageViews[i]
//         };

//         VkFramebufferCreateInfo framebufferInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
//         framebufferInfo.renderPass      = context->renderPass;
//         framebufferInfo.attachmentCount = ATTACHMENT_COUNT;
//         framebufferInfo.pAttachments    = &attachments[0];
//         framebufferInfo.width           = context->swapchainItems.extent.width;
//         framebufferInfo.height          = context->swapchainItems.extent.height;
//         framebufferInfo.layers          = 1;

//         if (vkCreateFramebuffer(context->device, &framebufferInfo, nullptr, &context->frameBuffers[i]) != VK_SUCCESS)
//         {
//             throw std::runtime_error("failed to create framebuffer");
//         }
//     }
// }

internal VkFramebuffer
vulkan::make_framebuffer(   VulkanContext * context,
                            VkRenderPass    renderPass,
                            uint32          attachmentCount,
                            VkImageView *   attachments,
                            uint32          width,
                            uint32          height)
{
    VkFramebufferCreateInfo createInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass         = renderPass,
        .attachmentCount    = attachmentCount,
        .pAttachments       = attachments,
        .width              = width,
        .height             = height,
        .layers             = 1,
    };

    VkFramebuffer result;
    DEVELOPMENT_ASSERT(
        vkCreateFramebuffer(context->device, &createInfo, nullptr, &result) == VK_SUCCESS,
        "Failed to create framebuffer");

    return result;
}


internal uint32
compute_mip_levels(uint32 texWidth, uint32 texHeight)
{
   uint32 result = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;
   return result;
}

internal void
cmd_generate_mip_maps(  VkCommandBuffer commandBuffer,
                    VkPhysicalDevice physicalDevice,
                    VkImage image,
                    VkFormat imageFormat,
                    uint32 texWidth,
                    uint32 texHeight,
                    uint32 mipLevels,
                    uint32 layerCount = 1)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    if ((formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == false)
    {
        throw std::runtime_error("Texture image format does not support blitting!");
    }

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
vulkan::make_texture(TextureAsset * asset, VulkanContext * context)
{
    uint32 width        = asset->width;
    uint32 height       = asset->height;
    uint32 mipLevels    = compute_mip_levels(width, height);


    VkDeviceSize imageSize = width * height * asset->channels;

    void * data;
    vkMapMemory(context->device, context->stagingBufferPool.memory, 0, imageSize, 0, &data);
    memcpy (data, (void*)asset->pixels.begin(), imageSize);
    vkUnmapMemory(context->device, context->stagingBufferPool.memory);

    VkImageCreateInfo imageInfo =
    {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType      = VK_IMAGE_TYPE_2D,
        .extent         = { width, height, 1 },
        .mipLevels      = mipLevels,
        .arrayLayers    = 1,
        .format         = VK_FORMAT_R8G8B8A8_UNORM,

        .tiling         = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode    = VK_SHARING_MODE_EXCLUSIVE, // Note(Leo): this concerns queue families,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .flags          = 0,
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

    VkCommandBuffer commandBuffer = vulkan::begin_command_buffer(context->device, context->commandPool);
    
    // Todo(Leo): begin and end command buffers once only and then just add commands from inside these
    cmd_transition_image_layout(commandBuffer, context->device, context->graphicsQueue,
                            resultImage, VK_FORMAT_R8G8B8A8_UNORM, mipLevels,
                            VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkBufferImageCopy region =
    {
        .bufferOffset       = 0,
        .bufferRowLength    = 0,
        .bufferImageHeight  = 0,

        .imageSubresource.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel          = 0,
        .imageSubresource.baseArrayLayer    = 0,
        .imageSubresource.layerCount        = 1,

        .imageOffset = { 0, 0, 0 },
        .imageExtent = { width, height, 1 },
    };
    vkCmdCopyBufferToImage (commandBuffer, context->stagingBufferPool.buffer, resultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


    /// CREATE MIP MAPS
    cmd_generate_mip_maps(  commandBuffer, context->physicalDevice,
                        resultImage, VK_FORMAT_R8G8B8A8_UNORM,
                        asset->width, asset->height, mipLevels);

    vulkan::execute_command_buffer (context->device, context->commandPool, context->graphicsQueue, commandBuffer);

    /// CREATE IMAGE VIEW
    VkImageViewCreateInfo imageViewInfo =
    { 
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = resultImage,
        .viewType   = VK_IMAGE_VIEW_TYPE_2D,
        .format     = VK_FORMAT_R8G8B8A8_UNORM,

        .subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel      = 0,
        .subresourceRange.levelCount        = mipLevels,
        .subresourceRange.baseArrayLayer    = 0,
        .subresourceRange.layerCount        = 1,

        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    VkImageView resultView = {};
    if (vkCreateImageView(context->device, &imageViewInfo, nullptr, &resultView) != VK_SUCCESS)
    {
        throw std::runtime_error ("Failed to create image view");
    }

    VulkanTexture resultTexture =
    {
        .image      = resultImage,
        .memory     = resultImageMemory,
        .view       = resultView,
    };
    return resultTexture;
}

internal VulkanTexture
vulkan::make_cubemap(VulkanContext * context, TextureAsset * assets)
{
    uint32 width        = assets[0].width;
    uint32 height       = assets[0].height;
    uint32 channels     = assets[0].channels;
    uint32 mipLevels    = compute_mip_levels(width, height);

    constexpr uint32 CUBEMAP_LAYERS = 6;

    VkDeviceSize layerSize = width * height * channels;
    VkDeviceSize imageSize = layerSize * CUBEMAP_LAYERS;

    VkExtent3D extent = { width, height, 1};

    byte * data;
    vkMapMemory(context->device, context->stagingBufferPool.memory, 0, imageSize, 0, (void**)&data);
    for (int i = 0; i < 6; ++i)
    {
        uint32 * start          = assets[i].pixels.begin();
        uint32 * end            = assets[i].pixels.end();
        uint32 * destination    = reinterpret_cast<uint32*>(data + (i * layerSize));

        std::copy(start, end, destination);
    }
    vkUnmapMemory(context->device, context->stagingBufferPool.memory);

    VkImageCreateInfo imageInfo =
    {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType      = VK_IMAGE_TYPE_2D,
        .extent         = extent,
        .mipLevels      = mipLevels,
        .arrayLayers    = CUBEMAP_LAYERS,
        .format         = VK_FORMAT_R8G8B8A8_UNORM,

        .tiling         = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage          = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode    = VK_SHARING_MODE_EXCLUSIVE, // Note(Leo): this concerns queue families,
        .samples        = VK_SAMPLE_COUNT_1_BIT,
        .flags          = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
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

    VkCommandBuffer commandBuffer = vulkan::begin_command_buffer(context->device, context->commandPool);
    
    // Todo(Leo): begin and end command buffers once only and then just add commands from inside these
    cmd_transition_image_layout(commandBuffer, context->device, context->graphicsQueue,
                                resultImage, VK_FORMAT_R8G8B8A8_UNORM, mipLevels,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, CUBEMAP_LAYERS);

    VkBufferImageCopy region =
    {
        .bufferOffset       = 0,
        .bufferRowLength    = 0,
        .bufferImageHeight  = 0,

        .imageSubresource.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel          = 0,
        .imageSubresource.baseArrayLayer    = 0,
        .imageSubresource.layerCount        = CUBEMAP_LAYERS,

        .imageOffset = { 0, 0, 0 },
        .imageExtent = extent,
    };
    vkCmdCopyBufferToImage (commandBuffer, context->stagingBufferPool.buffer, resultImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);


    /// CREATE MIP MAPS
    cmd_generate_mip_maps(  commandBuffer, context->physicalDevice,
                        resultImage, VK_FORMAT_R8G8B8A8_UNORM,
                        width, height, mipLevels, CUBEMAP_LAYERS);

    vulkan::execute_command_buffer (context->device, context->commandPool, context->graphicsQueue, commandBuffer);

    /// CREATE IMAGE VIEW
    VkImageViewCreateInfo imageViewInfo =
    { 
        .sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image      = resultImage,
        .viewType   = VK_IMAGE_VIEW_TYPE_CUBE,
        .format     = VK_FORMAT_R8G8B8A8_UNORM,

        .subresourceRange.aspectMask        = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel      = 0,
        .subresourceRange.levelCount        = mipLevels,
        .subresourceRange.baseArrayLayer    = 0,
        .subresourceRange.layerCount        = CUBEMAP_LAYERS,

        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
    };

    VkImageView resultView = {};
    if (vkCreateImageView(context->device, &imageViewInfo, nullptr, &resultView) != VK_SUCCESS)
    {
        throw std::runtime_error ("Failed to create image view");
    }

    VulkanTexture resultTexture =
    {
        .image      = resultImage,
        .memory     = resultImageMemory,
        .view       = resultView,
    };
    return resultTexture;
}


internal void
vulkan::destroy_texture(VulkanContext * context, VulkanTexture * texture)
{
    vkDestroyImage(context->device, texture->image, nullptr);
    vkFreeMemory(context->device, texture->memory, nullptr);
    vkDestroyImageView(context->device, texture->view, nullptr);
}

internal void
vulkan::create_texture_sampler(VulkanContext * context)
{
    VkSamplerCreateInfo samplerInfo =
    { 
        .sType              = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter          = VK_FILTER_LINEAR,
        .minFilter          = VK_FILTER_LINEAR,

        .addressModeU       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV       = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW       = VK_SAMPLER_ADDRESS_MODE_REPEAT,

        .anisotropyEnable   = VK_TRUE,
        .maxAnisotropy      = 16,

        .borderColor                 = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates     = VK_FALSE,
        .compareEnable               = VK_FALSE,
        .compareOp                   = VK_COMPARE_OP_ALWAYS,

        .mipmapMode         = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .minLod             = 0.0f,
        .maxLod             = VULKAN_MAX_LOD_FLOAT,
        .mipLodBias         = 0.0f,
    };

    if (vkCreateSampler(context->device, &samplerInfo, nullptr, &context->textureSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create texture sampler!");
    }
}

internal void
CleanupSwapchain(VulkanContext * context)
{
    vkDestroyImage      (context->device, context->drawingResources.colorImage, nullptr);
    vkDestroyImageView  (context->device, context->drawingResources.colorImageView, nullptr);
    vkDestroyImage      (context->device, context->drawingResources.depthImage, nullptr);
    vkDestroyImageView  (context->device, context->drawingResources.depthImageView, nullptr);
    vkFreeMemory        (context->device, context->drawingResources.memory, nullptr);

    // for (auto framebuffer : context->frameBuffers)
    // {
    //     vkDestroyFramebuffer(context->device, framebuffer, nullptr);
    // }

    vkDestroyRenderPass(context->device, context->renderPass, nullptr);

    for (auto imageView : context->swapchainItems.imageViews)
    {
        vkDestroyImageView(context->device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(context->device, context->swapchainItems.swapchain, nullptr);

    vkDestroyPipeline(context->device, context->lineDrawPipeline.pipeline, nullptr);
    vkDestroyPipelineLayout(context->device, context->lineDrawPipeline.layout, nullptr);

    context->lineDrawPipeline.pipeline    = VK_NULL_HANDLE;
    context->lineDrawPipeline.layout      = VK_NULL_HANDLE;
}

internal void
Cleanup(VulkanContext * context)
{
    // Note(Leo): these are GraphicsContext things
    vulkan::unload_scene(context);

    CleanupSwapchain(context);

    vkDestroyDescriptorPool(context->device, context->uniformDescriptorPool, nullptr);
    vkDestroyDescriptorPool(context->device, context->materialDescriptorPool, nullptr);

    vkDestroySampler(context->device, context->textureSampler, nullptr);

    // Todo(Leo): When we have scene, or different models, these too move away from here
    vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayouts.model, nullptr);  
    vkDestroyDescriptorSetLayout(context->device, context->descriptorSetLayouts.scene, nullptr);  

    vulkan::destroy_buffer_resource(context->device, &context->staticMeshPool);
    vulkan::destroy_buffer_resource(context->device, &context->stagingBufferPool);
    vulkan::destroy_buffer_resource(context->device, &context->modelUniformBuffer);
    vulkan::destroy_buffer_resource(context->device, &context->sceneUniformBuffer);
    vulkan::destroy_buffer_resource(context->device, &context->guiUniformBuffer);

    for (auto & frame : context->virtualFrames)
    {
        /* Note(Leo): command buffers are destroyed with command pool, but we need to destroy
        framebuffers here, since they are always recreated immediately right after destroying
        them in drawing procedure */
        vkDestroyFramebuffer(context->device, frame.framebuffer, nullptr);
        
        vkDestroySemaphore(context->device, frame.renderFinishedSemaphore, nullptr);
        vkDestroySemaphore(context->device, frame.imageAvailableSemaphore, nullptr);
        vkDestroyFence(context->device, frame.inFlightFence, nullptr);
    }

    vkDestroyCommandPool(context->device, context->commandPool, nullptr);

    vkDestroyDevice(context->device, nullptr);
    vkDestroySurfaceKHR(context->instance, context->surface, nullptr);
    vkDestroyInstance(context->instance, nullptr);
}

internal void
create_virtual_frames(VulkanContext * context)
{
    VkCommandBufferAllocateInfo primaryCmdAllocateInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = context->commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBufferAllocateInfo secondaryCmdAllocateInfo =
    {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = context->commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY,
        .commandBufferCount = 1,
    };

    VkEventCreateInfo eventCreateInfo =
    {
        .sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
    };

    VkSemaphoreCreateInfo semaphoreInfo = 
    {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    };

    VkFenceCreateInfo fenceInfo =
    {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };

    for (auto & frame : context->virtualFrames)
    {
        // Command buffers
        bool32 success = vkAllocateCommandBuffers(context->device, &primaryCmdAllocateInfo, &frame.commandBuffers.primary) == VK_SUCCESS;
        success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.commandBuffers.scene) == VK_SUCCESS;
        success = success && vkAllocateCommandBuffers(context->device, &secondaryCmdAllocateInfo, &frame.commandBuffers.gui) == VK_SUCCESS;

        // Synchronization stuff
        success = success && vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) == VK_SUCCESS;
        success = success && vkCreateSemaphore(context->device, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) == VK_SUCCESS;
        success = success && vkCreateFence(context->device, &fenceInfo, nullptr, &frame.inFlightFence) == VK_SUCCESS;

        DEVELOPMENT_ASSERT(success, "Failed to create VulkanVirtualFrame");
    }
}


internal void
vulkan::recreate_swapchain(VulkanContext * context, uint32 width, uint32 height)
{
    vkDeviceWaitIdle(context->device);

    CleanupSwapchain(context);

    std::cout << "Recreating swapchain, width: " << width << ", height: " << height <<"\n";

    context->swapchainItems         = CreateSwapchainAndImages(context, {width, height} );
    context->renderPass             = CreateRenderPass(context, &context->swapchainItems, context->msaaSamples);
    
    recreate_loaded_pipelines(context);
    context->lineDrawPipeline       = vulkan::make_line_pipeline(context, context->lineDrawPipeline.info);
    
    create_drawing_resources(context);
}

/* Todo(Leo): this belongs to winapi namespace because it is definetly windows specific.
Make better separation between windows part of this and vulkan part of this. */
namespace winapi
{
    VkExtent2D get_hwnd_size(HWND winWindow)
    {
        RECT rect;
        GetWindowRect(winWindow, &rect);

        return {
            .width = (uint32)(rect.right - rect.left),
            .height = (uint32)(rect.bottom - rect.top),
        };
    }

    internal VulkanContext
    make_vulkan_context(HINSTANCE winInstance, HWND winWindow)
    {
        VulkanContext resultContext = {};

        resultContext.instance = CreateInstance();
        // TODO(Leo): (if necessary, but at this point) Setup debug callbacks, look vulkan-tutorial.com
        
        VkWin32SurfaceCreateInfoKHR surfaceCreateInfo =
        {
            .sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .hinstance  = winInstance,
            .hwnd       = winWindow 
        };
        DEVELOPMENT_ASSERT(vkCreateWin32SurfaceKHR(resultContext.instance, &surfaceCreateInfo, nullptr, &resultContext.surface) == VK_SUCCESS,
                        "Failed to create win32 surface.");

        resultContext.physicalDevice = PickPhysicalDevice(resultContext.instance, resultContext.surface);
        vkGetPhysicalDeviceProperties(resultContext.physicalDevice, &resultContext.physicalDeviceProperties);
        resultContext.msaaSamples = GetMaxUsableMsaaSampleCount(resultContext.physicalDevice);
        std::cout << "Sample count: " << vulkan::to_str(resultContext.msaaSamples) << "\n";

        resultContext.physicalDevice = resultContext.physicalDevice;
        resultContext.physicalDeviceProperties = resultContext.physicalDeviceProperties;

        resultContext.device = CreateLogicalDevice(resultContext.physicalDevice, resultContext.surface);

        VulkanQueueFamilyIndices queueFamilyIndices = vulkan::FindQueueFamilies(resultContext.physicalDevice, resultContext.surface);
        vkGetDeviceQueue(resultContext.device, queueFamilyIndices.graphics, 0, &resultContext.graphicsQueue);
        vkGetDeviceQueue(resultContext.device, queueFamilyIndices.present, 0, &resultContext.presentQueue);

        /// ---- Create Command Pool ----
        VkCommandPoolCreateInfo poolInfo =
        {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex   = queueFamilyIndices.graphics,
            .flags              = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        };

        if (vkCreateCommandPool(resultContext.device, &poolInfo, nullptr, &resultContext.commandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool");
        }

        /* 
        Above is device
        -----------------------------------------------------------------------
        Below is content
        */ 

        /* Todo(Leo): now that everything is in VulkanContext, it does not make so much sense
        to explicitly specify each component as parameter */
        resultContext.swapchainItems      = CreateSwapchainAndImages(&resultContext, get_hwnd_size(winWindow));
        resultContext.renderPass          = CreateRenderPass(&resultContext, &resultContext.swapchainItems, resultContext.msaaSamples);
        
        resultContext.descriptorSetLayouts.scene    = CreateSceneDescriptorSetLayout(resultContext.device);
        resultContext.descriptorSetLayouts.model    = CreateModelDescriptorSetLayout(resultContext.device);

        vulkan::create_drawing_resources(&resultContext); 
        create_virtual_frames(&resultContext);

        {
            VulkanPipelineLoadInfo info = 
            {
                .vertexShaderPath       = "shaders/line_vert.spv",
                .fragmentShaderPath     = "shaders/line_frag.spv",
                .options.primitiveType  = platform::PipelineOptions::PRIMITIVE_LINE
            };
            resultContext.lineDrawPipeline = vulkan::make_line_pipeline(&resultContext, info);
        }

        std::cout << "\nVulkan Initialized succesfully\n\n";

        return resultContext;
    }
}