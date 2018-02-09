#include "ConstantBufferVulkan.h"

ConstantBufferVulkan::ConstantBufferVulkan(std::string NAME, unsigned int location)
{
	name = NAME;
	this->location = location;
}

ConstantBufferVulkan::~ConstantBufferVulkan()
{
}

void ConstantBufferVulkan::setData(const void * data, size_t size, Material * m, unsigned int location)
{
}

void ConstantBufferVulkan::bind(Material *)
{
}

void ConstantBufferVulkan::init(VulkanRenderer* renderer)
{
	this->renderer = renderer;
}
