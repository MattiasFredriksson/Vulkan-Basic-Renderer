#include "TechniqueVulkan.h"
#include "Vulkan\RenderStateVulkan.h"
#include "IA.h"
#include "Vulkan\MaterialVulkan.h"
#include "VulkanConstruct.h"

TechniqueVulkan::TechniqueVulkan(Material* m, RenderState* r, VulkanRenderer* renderer) : Technique(m, r)
{
	setRenderer(renderer);

	createRenderPass();
	createDescriptorSet();
	createPipeline();
}

TechniqueVulkan::~TechniqueVulkan()
{
}

void TechniqueVulkan::enable(Renderer* renderer)
{
}

void TechniqueVulkan::setRenderer(VulkanRenderer* renderer)
{
	this->renderer = renderer;
}

void TechniqueVulkan::createRenderPass()
{
	VkAttachmentDescription attatchment = {};
	attatchment.flags = 0;
	attatchment.format = renderer->getSwapchainFormat().format;
	attatchment.samples = VK_SAMPLE_COUNT_1_BIT;
	attatchment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attatchment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attatchment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attatchment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attatchment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attatchment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attachmentReference = {};
	attachmentReference.attachment = 0;
	attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.flags = 0;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.inputAttachmentCount = 0;
	subpass.pInputAttachments = nullptr;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;
	subpass.pResolveAttachments = nullptr;
	subpass.pDepthStencilAttachment = nullptr;
	subpass.preserveAttachmentCount = 0;
	subpass.pPreserveAttachments = nullptr;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.pNext = nullptr;
	renderPassCreateInfo.flags = 0;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attatchment;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &dependency;

	VkResult result = vkCreateRenderPass(renderer->getDevice(), &renderPassCreateInfo, nullptr, &renderPass);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create renderpass.");
}

void TechniqueVulkan::createDescriptorSet()
{
	// This is all hardcoded, not very good
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

	// Vertex shader slots
	if (((MaterialVulkan*)material)->hasDefine(Material::ShaderType::VS, "#define POSITION "))
	{
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), POSITION, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	}

	if (((MaterialVulkan*)material)->hasDefine(Material::ShaderType::VS, "#define NORMAL "))
	{
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), NORMAL, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	}

	if (((MaterialVulkan*)material)->hasDefine(Material::ShaderType::VS, "#define TEXTCOORD "))
	{
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), TEXTCOORD, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	}
	
	if (((MaterialVulkan*)material)->hasDefine(Material::ShaderType::VS, "#define TRANSLATION "))
	{
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), TRANSLATION, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	}

	VkShaderStageFlags stage = ((MaterialVulkan*)material)->hasDefine(Material::ShaderType::VS, "#define DIFFUSE_TINT ") ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	stage |= ((MaterialVulkan*)material)->hasDefine(Material::ShaderType::PS, "#define DIFFUSE_TINT ") ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	if (stage)
	{
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), DIFFUSE_TINT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage);
	}

	if (((MaterialVulkan*)material)->hasDefine(Material::ShaderType::PS, "#define DIFFUSE_SLOT "))
	{
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), DIFFUSE_SLOT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	// Create layout
	vertexDataSetLayout = createDescriptorLayout(renderer->getDevice(), layoutBindings.data(), layoutBindings.size());
}

void TechniqueVulkan::createPipeline()
{
	// todo: create shader modules
	VkShaderModule vertexShaderModule;
	VkShaderModule fragmentShaderModule;

	VkPipelineShaderStageCreateInfo vertexShaderStageInfo = {};
	vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageInfo.module = vertexShaderModule;
	vertexShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageInfo = {};
	fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageInfo.module = fragmentShaderModule;
	fragmentShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo stages[2];
	stages[0] = vertexShaderStageInfo;
	stages[1] = fragmentShaderStageInfo;

	// todo
	VkVertexInputBindingDescription vertexBindingDescription = {};
	VkVertexInputAttributeDescription vertexAttributeDescription = {};

	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.pNext = nullptr;
	pipelineVertexInputStateCreateInfo.flags = 0;
	pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;	// todo
	pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = &vertexAttributeDescription;

	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo = {};
	pipelineInputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	pipelineInputAssemblyStateCreateInfo.pNext = nullptr;
	pipelineInputAssemblyStateCreateInfo.flags = 0;
	pipelineInputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineInputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = defineViewport((float)renderer->getWidth(), (float)renderer->getHeight());
	VkRect2D scissor = defineScissorRect(viewport);

	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo = {};
	pipelineViewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	pipelineViewportStateCreateInfo.pNext = nullptr;
	pipelineViewportStateCreateInfo.flags = 0;
	pipelineViewportStateCreateInfo.viewportCount = 1;
	pipelineViewportStateCreateInfo.pViewports = &viewport;
	pipelineViewportStateCreateInfo.scissorCount = 1;
	pipelineViewportStateCreateInfo.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo = {};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.pNext = nullptr;
	pipelineRasterizationStateCreateInfo.flags = 0;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.polygonMode = ((RenderStateVulkan*)renderState)->getWireframe() ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
	pipelineRasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	pipelineRasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	pipelineRasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	pipelineRasterizationStateCreateInfo.depthBiasClamp = 1.0f;
	pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 1.0f;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo = {};
	pipelineMultisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	pipelineMultisampleStateCreateInfo.pNext = nullptr;
	pipelineMultisampleStateCreateInfo.flags = 0;
	pipelineMultisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	pipelineMultisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
	pipelineMultisampleStateCreateInfo.minSampleShading = 0.0f;
	pipelineMultisampleStateCreateInfo.pSampleMask = nullptr;
	pipelineMultisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
	pipelineMultisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
	pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo = {};
	pipelineColorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	pipelineColorBlendStateCreateInfo.pNext = nullptr;
	pipelineColorBlendStateCreateInfo.flags = 0;
	pipelineColorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
	pipelineColorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
	pipelineColorBlendStateCreateInfo.attachmentCount = 1;
	pipelineColorBlendStateCreateInfo.pAttachments = &pipelineColorBlendAttachmentState;
	pipelineColorBlendStateCreateInfo.blendConstants[0] =
	pipelineColorBlendStateCreateInfo.blendConstants[1] =
	pipelineColorBlendStateCreateInfo.blendConstants[2] =
	pipelineColorBlendStateCreateInfo.blendConstants[3] = 0.0f;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = nullptr;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &vertexDataSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

	VkResult result = vkCreatePipelineLayout(renderer->getDevice(), &pipelineLayoutCreateInfo, nullptr, &pipelineLayout);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline layout.");


	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = nullptr;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = stages;
	pipelineCreateInfo.pVertexInputState = &pipelineVertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &pipelineInputAssemblyStateCreateInfo;
	pipelineCreateInfo.pTessellationState = nullptr;
	pipelineCreateInfo.pViewportState = &pipelineViewportStateCreateInfo;
	pipelineCreateInfo.pRasterizationState = &pipelineRasterizationStateCreateInfo;
	pipelineCreateInfo.pMultisampleState = &pipelineMultisampleStateCreateInfo;
	pipelineCreateInfo.pDepthStencilState = nullptr;
	pipelineCreateInfo.pColorBlendState = &pipelineColorBlendStateCreateInfo;
	pipelineCreateInfo.pDynamicState = nullptr;
	pipelineCreateInfo.layout = pipelineLayout;
	pipelineCreateInfo.renderPass = renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineCreateInfo.basePipelineIndex = 0;

	result = vkCreateGraphicsPipelines(renderer->getDevice(), VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &pipeline);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create pipeline.");
}
