#include "TechniqueVulkan.h"
#include "Vulkan\RenderStateVulkan.h"
#include "IA.h"
#include "Vulkan\MaterialVulkan.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <Windows.h>
#include <locale>
#include <codecvt>
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
	createShaders();

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
	stages[0] = defineShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule);
	stages[1] = defineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule);

	// Vertex buffer bindings (static description...)
	const uint32_t NUM_BUFFER = 1;
	const uint32_t NUM_ATTRI = 3;
	VkVertexInputBindingDescription vertexBufferBindings[NUM_BUFFER] = 
	{
		defineVertexBinding(0, 10 * 4)
	};
	VkVertexInputAttributeDescription vertexAttributes[NUM_ATTRI] =
	{
		defineVertexAttribute(0, POSITION, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 0),
		defineVertexAttribute(0, NORMAL, VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT, 16),
		defineVertexAttribute(0, TEXTCOORD, VkFormat::VK_FORMAT_R32G32_SFLOAT, 32)
	};
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = 
		defineVertexBufferBindings(vertexBufferBindings, NUM_BUFFER, vertexAttributes, NUM_ATTRI);

	//
	VkPipelineInputAssemblyStateCreateInfo pipelineInputAssemblyStateCreateInfo =
		defineInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// Viewport
	VkViewport viewport = defineViewport((float)renderer->getWidth(), (float)renderer->getHeight());
	VkRect2D scissor = defineScissorRect(viewport);
	VkPipelineViewportStateCreateInfo pipelineViewportStateCreateInfo =
		defineViewportState(&viewport, &scissor);

	// Rasterization state
	int rasterFlag = 0;
	if (((RenderStateVulkan*)renderState)->getWireframe())
		rasterFlag |= WIREFRAME_BIT;
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo =
		defineRasterizationState(rasterFlag, VK_CULL_MODE_BACK_BIT);

	// Multisampling
	VkPipelineMultisampleStateCreateInfo pipelineMultisampleStateCreateInfo =
		defineMultiSampling_OFF();

	// Blend states
	VkPipelineColorBlendAttachmentState pipelineColorBlendAttachmentState = {};
	pipelineColorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	pipelineColorBlendAttachmentState.blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo pipelineColorBlendStateCreateInfo =
		defineBlendState(&pipelineColorBlendAttachmentState, 1);

	// Uniform layout description
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
		defineUniformLayout(&vertexDataSetLayout, 1);

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

void TechniqueVulkan::createShaders()
{
	std::string vs = assembleShader(Material::ShaderType::VS);
	std::string fs = assembleShader(Material::ShaderType::PS);

	std::string vsOut = runCompiler(Material::ShaderType::VS, vs);
	std::string fsOut = runCompiler(Material::ShaderType::PS, fs);

	std::vector<char> vsData = loadSPIR_V(vsOut);
	std::vector<char> fsData = loadSPIR_V(fsOut);

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = nullptr;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = vsData.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(vsData.data());

	VkResult result = vkCreateShaderModule(renderer->getDevice(), &shaderModuleCreateInfo, nullptr, &vertexShaderModule);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create vertex shader module.");

	shaderModuleCreateInfo.codeSize = fsData.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(fsData.data());

	result = vkCreateShaderModule(renderer->getDevice(), &shaderModuleCreateInfo, nullptr, &fragmentShaderModule);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create fragment shader module.");
}

const char *path = "..\\assets\\Vulkan\\";
// Returns relative file path of created file
std::string TechniqueVulkan::assembleShader(Material::ShaderType type)
{
	std::string fileName;

	if (type == Material::ShaderType::VS)
		fileName = "vertexShader.glsl.vert";
	else if (type == Material::ShaderType::PS)
		fileName = "fragmentShader.glsl.frag";
	else
		throw std::runtime_error("Unsupported shader type!");

	// Read shader into string
	std::ifstream file(material->shaderFileNames[type]);
	std::stringstream fileContents;
	if (file.is_open())
	{
		fileContents << file.rdbuf();
		file.close();
	}
	else
		throw std::runtime_error("Could not open shader file.");

	// Write complete shader into file
	std::ofstream completeShader(path + fileName);
	if (completeShader.is_open())
	{
		completeShader << "#version 450\n";

		completeShader << assembleDefines(type);

		completeShader << fileContents.str();

		completeShader.close();
	}
	else
		throw std::runtime_error("Could not create shader file.");

	return fileName;
}
// Returns relative file path of created file
std::string TechniqueVulkan::assembleDefines(Material::ShaderType type)
{

	std::string args;
	for (std::string def : material->shaderDefines[type])
		args.append(def);
	return args;
}

void printThreadError(const char *msg)
{
	DWORD err = GetLastError();
	if (err != 0)
	{
		LPSTR messageBuffer = nullptr;
		size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);

		//Free the buffer.
		LocalFree(messageBuffer);
		std::cout << msg << message << "\n";
	}
}
// Returns output file name
std::string TechniqueVulkan::runCompiler(Material::ShaderType type, std::string inputFileName)
{
	// pass defines
	std::string commandLineStr;
	if (type == Material::ShaderType::VS)
		commandLineStr.append("-v -V -o vertexShader.spv -e main ");
	else if (type == Material::ShaderType::PS)
		commandLineStr.append("-v -V -o fragmentShader.spv -e main ");

	commandLineStr += "\"" + inputFileName + "\"";

	LPSTR commandLine = const_cast<char *>(commandLineStr.c_str());
	
	STARTUPINFOA startupInfo = {};
	startupInfo.cb = sizeof(STARTUPINFOA);
	startupInfo.lpReserved = NULL;
	startupInfo.lpDesktop = "desktop";
	startupInfo.lpTitle = NULL;
	startupInfo.dwX = 0;
	startupInfo.dwY = 0;
	startupInfo.dwXSize = 0;
	startupInfo.dwYSize = 0;
	startupInfo.dwXCountChars = 0;
	startupInfo.dwYCountChars = 0;
	startupInfo.dwFillAttribute = 0;
	startupInfo.dwFlags = 0;
	startupInfo.wShowWindow = 0;
	startupInfo.cbReserved2 = 0;
	startupInfo.lpReserved2 = NULL;
	startupInfo.hStdInput = 0;
	startupInfo.hStdError = 0;
	startupInfo.hStdOutput = 0;

	LPSTARTUPINFOA startupInfoPointer = &startupInfo;

	PROCESS_INFORMATION processInfo = {};
	if (!CreateProcessA("..\\assets\\Vulkan\\glslangValidator.exe", commandLine, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, path, startupInfoPointer, &processInfo))
	{
		//HRESULT res = HRESULT_FROM_WIN32(GetLastError());
		throw std::runtime_error("Failed to start shader compilation process.");
	}

	WaitForSingleObject(processInfo.hProcess, INFINITE);
	
	
	DWORD exitCode;
	bool acquired = GetExitCodeProcess(processInfo.hProcess, &exitCode);

	if (!acquired)
	{
		printThreadError("Error: Fetching process error failed with msg: ");
		throw std::runtime_error("Could not get exit code from process.");
	}
	else
		std::cout << "Process exited with msg: " << exitCode << "\n";

	CloseHandle(processInfo.hProcess);
	CloseHandle(processInfo.hThread);


	return (type == Material::ShaderType::VS) ? "..\\assets\\Vulkan\\vertexShader.spv" : "..\\assets\\Vulkan\\fragmentShader.spv";
}

std::vector<char> TechniqueVulkan::loadSPIR_V(std::string fileName)
{
	// Open file and seek to end
	std::ifstream shaderFile(fileName, std::ios::ate | std::ios::binary);

	if (!shaderFile.is_open())
		throw std::runtime_error("Could not open SPIR-V file.");

	// Get file size
	size_t fileSize = static_cast<size_t>(shaderFile.tellg());

	// Create and resize vector to fit the file
	std::vector<char> data;
	data.resize(fileSize);

	// Reset to beginning
	shaderFile.seekg(0);

	// Read data into vector
	shaderFile.read(data.data(), fileSize);
	shaderFile.close();

	return data;
}
