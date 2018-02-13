#include "MaterialVulkan.h"

MaterialVulkan::MaterialVulkan(const std::string & name)
{

}

MaterialVulkan::~MaterialVulkan()
{
	// Free all constant buffers
	for (auto cb : constantBuffers)
	{
		delete cb.second;
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
	return 0;
}

void MaterialVulkan::addConstantBuffer(std::string name, unsigned int location)
{
	constantBuffers[location] = renderer->makeConstantBuffer(name, location);
}

void MaterialVulkan::updateConstantBuffer(const void* data, size_t size, unsigned int location)
{
	constantBuffers[location]->setData(data, size, this, location);
}

int MaterialVulkan::enable()
{
	return 0;
}

void MaterialVulkan::disable()
{
}

void MaterialVulkan::setRenderer(VulkanRenderer* renderer)
{
	this->renderer = renderer;
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
