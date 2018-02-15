#include "MaterialVulkan.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <Windows.h>
#include <locale>
#include <codecvt>
#include "../VulkanConstruct.h"
#include "../IA.h"


MaterialVulkan::MaterialVulkan(const std::string & name, VulkanRenderer *renderHandle)
	: name(name), _renderHandle(renderHandle)
{
	spawned = false;
}

MaterialVulkan::~MaterialVulkan()
{
	// Free all constant buffers
	for (auto cb : constantBuffers)
	{
		delete cb.second;
	}
}

void MaterialVulkan::destroyShaderObjects()
{
	if (spawned)
	{
		vkDestroyShaderModule(_renderHandle->getDevice(), vertexShader, nullptr);
		vkDestroyShaderModule(_renderHandle->getDevice(), fragmentShader, nullptr);
		vkDestroyPipelineLayout(_renderHandle->getDevice(), pipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(_renderHandle->getDevice(), matDescriptorLayout, nullptr);
		vkDestroyDescriptorPool(_renderHandle->getDevice(), matPool, nullptr);
		spawned = false;
	}
}

void MaterialVulkan::setShader(const std::string & shaderFileName, ShaderType type)
{
	shaderFileNames[type] = shaderFileName;
}

void MaterialVulkan::removeShader(ShaderType type)
{
	shaderFileNames.erase(type);
}

void MaterialVulkan::setDiffuse(Color c)
{
	color = c;
}

int MaterialVulkan::compileMaterial(std::string & errString)
{
	//Clear first
	destroyShaderObjects();
	int success = createShaders();
	createDescriptorParams_Material();
	generatePipelineLayout();
	spawned = true;
	return success;
}

void MaterialVulkan::addConstantBuffer(std::string name, unsigned int location)
{
	constantBuffers[location] = _renderHandle->makeConstantBuffer(name, location);
}

void MaterialVulkan::updateConstantBuffer(const void* data, size_t size, unsigned int location)
{
	constantBuffers[location]->setData(data, size, this, location);
}

int MaterialVulkan::enable()
{
	if (program == 0 || isValid == false)
		return -1;
	glUseProgram(program);

	for (auto cb : constantBuffers)
	{
		cb.second->bind(this);
	}
	return 0;
}

void MaterialVulkan::disable()
{
}

bool MaterialVulkan::hasDefine(Material::ShaderType shaderType, std::string searchString)
{
	size_t size = shaderDefines.size();

	std::set<std::string>::iterator it;
	for (std::string str : shaderDefines[shaderType])
	{
		if (str.find(searchString) != std::string::npos)
			return true;
	}
	return false;
}

std::vector<std::pair<unsigned, VkDescriptorBufferInfo*>> MaterialVulkan::getBufferInfos()
{
	std::vector<std::pair<unsigned, VkDescriptorBufferInfo*>> bufferInfos;

	for (auto cb : constantBuffers)
	{
		bufferInfos.push_back(std::make_pair(
			cb.first, ((ConstantBufferVulkan*)cb.second)->getDescriptorBufferInfo()
		));
	}

	return bufferInfos;
}

void MaterialVulkan::generatePipelineLayout()
{
	// Uniform layout description
	pipelineLayout = createPipelineLayout(_renderHandle->getDevice(), &matDescriptorLayout, 1);
}

#pragma region Descriptor set & layout


VkDescriptorSet MaterialVulkan::allocMeshDescriptor()
{
	return createDescriptorSet(_renderHandle->getDevice(), meshPool, &meshDescriptorLayout, 1);
}

VkDescriptorPool createDescriptorPool(VkDevice device, uint32_t uniBuffers, uint32_t imageBuffers, uint32_t poolSize)
{
	// Describes how many of every descriptor type can be created in the pool
	VkDescriptorPoolSize descriptorSizes[MAX_DESCRIPTOR_TYPES];
	uint32_t i = 0;
	if (uniBuffers > 0)
	{
		descriptorSizes[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorSizes[i].descriptorCount = uniBuffers;
		i++;
	}
	if (imageBuffers > 0)
	{
		descriptorSizes[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorSizes[i].descriptorCount = imageBuffers;
		i++;
	}
	// Create pool
	return createDescriptorPool(device, descriptorSizes, i, poolSize);
}

void MaterialVulkan::createDescriptorParams_Mesh()
{
	// This is all hardcoded, not very good
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	uint32_t uniBuf = 0;

	if (hasDefine(Material::ShaderType::VS, "#define TRANSLATION "))
	{
		uniBuf++;
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), TRANSLATION, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	}

	// Create mesh descriptor layout
	meshDescriptorLayout = createDescriptorLayout(_renderHandle->getDevice(), layoutBindings.data(), layoutBindings.size());
	meshPool = createDescriptorPool(_renderHandle->getDevice(), uniBuf, 0, 40);
}

void MaterialVulkan::createDescriptorParams_Material()
{
	// This is all hardcoded, not very good
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;

	uint32_t uniBuf = 0, imgSamp = 0;

	VkShaderStageFlags stage = hasDefine(Material::ShaderType::VS, "#define DIFFUSE_TINT ") ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	stage |= hasDefine(Material::ShaderType::PS, "#define DIFFUSE_TINT ") ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	if (stage)
	{
		uniBuf++;
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), DIFFUSE_TINT, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stage);
	}

	if (hasDefine(Material::ShaderType::PS, "#define DIFFUSE_SLOT "))
	{
		imgSamp++;
		layoutBindings.push_back(VkDescriptorSetLayoutBinding{});
		writeLayoutBinding(layoutBindings.back(), DIFFUSE_SLOT, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	// Create material descriptor layout
	matDescriptorLayout = createDescriptorLayout(_renderHandle->getDevice(), layoutBindings.data(), layoutBindings.size());

	// Create set
	matPool = createDescriptorPool(_renderHandle->getDevice(), uniBuf, imgSamp, 1);
	matDescriptor = createDescriptorSet(_renderHandle->getDevice(), matPool, &matDescriptorLayout, 1);
}

#pragma endregion

#pragma region Shader creation



int MaterialVulkan::createShaders()
{
	std::string vs = assembleShader(Material::ShaderType::VS);
	std::string fs = assembleShader(Material::ShaderType::PS);

	std::string vsOut = runCompiler(Material::ShaderType::VS, vs);
	std::string fsOut = runCompiler(Material::ShaderType::PS, fs);

	//TODO: Implement error codes
	std::vector<char> vsData = loadSPIR_V(vsOut);
	std::vector<char> fsData = loadSPIR_V(fsOut);

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = nullptr;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = vsData.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(vsData.data());

	VkResult result = vkCreateShaderModule(_renderHandle->getDevice(), &shaderModuleCreateInfo, nullptr, &vertexShader);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create vertex shader module.\n";
		return -1;
	}
	shaderModuleCreateInfo.codeSize = fsData.size();
	shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(fsData.data());

	result = vkCreateShaderModule(_renderHandle->getDevice(), &shaderModuleCreateInfo, nullptr, &fragmentShader);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create fragment shader module.\n";
		return -2;
	}
}

const char *path = "..\\assets\\Vulkan\\";
// Returns relative file path of created file
std::string MaterialVulkan::assembleShader(Material::ShaderType type)
{
	std::string fileName;

	if (type == Material::ShaderType::VS)
		fileName = "vertexShader.glsl.vert";
	else if (type == Material::ShaderType::PS)
		fileName = "fragmentShader.glsl.frag";
	else
		throw std::runtime_error("Unsupported shader type!");

	// Read shader into string
	std::ifstream file(shaderFileNames[type]);
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
std::string MaterialVulkan::assembleDefines(Material::ShaderType type)
{

	std::string args;
	for (std::string def : shaderDefines[type])
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
std::string MaterialVulkan::runCompiler(Material::ShaderType type, std::string inputFileName)
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

std::vector<char> MaterialVulkan::loadSPIR_V(std::string fileName)
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

#pragma endregion
