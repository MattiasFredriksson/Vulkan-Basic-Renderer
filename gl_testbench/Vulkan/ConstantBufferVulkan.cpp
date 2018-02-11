#include "ConstantBufferVulkan.h"
#include "../VulkanConstruct.h"
ConstantBufferVulkan::ConstantBufferVulkan(std::string NAME, unsigned int location)
	: buffer(nullptr)
{
	name = NAME;
	this->location = location;
}

ConstantBufferVulkan::~ConstantBufferVulkan()
{
	vkDestroyBuffer(renderer->getDevice(), buffer, nullptr);
}

void ConstantBufferVulkan::setData(const void * data, size_t size, Material * m, unsigned int location)
{
	if (!buffer)
	{
		buffer = createBuffer(renderer->getDevice(), size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
		memSize = size;
		poolOffset = renderer->bindPhysicalMemory(buffer, memSize, MemoryPool::UNIFORM_BUFFER);
	}
	else if(memSize < size)
		throw std::runtime_error("Constant buffer cannot fit the data.");
	renderer->transferBufferData(buffer, data, size, 0);
}

void ConstantBufferVulkan::bind(Material *)
{
}

void ConstantBufferVulkan::init(VulkanRenderer* renderer)
{
	this->renderer = renderer;
}
