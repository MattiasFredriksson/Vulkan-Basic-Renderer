#include "ConstantBufferVulkan.h"
#include "../VulkanConstruct.h"
#include "MaterialVulkan.h"
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
		poolOffset = renderer->bindPhysicalMemory(buffer, MemoryPool::UNIFORM_BUFFER);

		// Set the descriptor info
		descriptorBufferInfo.buffer = buffer;
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = VK_WHOLE_SIZE;
	}
	else if(memSize < size)
		throw std::runtime_error("Constant buffer cannot fit the data.");
	renderer->transferBufferData(buffer, data, size, 0);
}

void ConstantBufferVulkan::bind(Material *m)
{
	vkCmdBindDescriptorSets(renderer->getFrameCmdBuf(), VK_PIPELINE_BIND_POINT_GRAPHICS, ((MaterialVulkan*)m)->pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
}

void ConstantBufferVulkan::init(VulkanRenderer* renderer)
{
	this->renderer = renderer;
}

VkDescriptorBufferInfo* ConstantBufferVulkan::getDescriptorBufferInfo()
{
	return &descriptorBufferInfo;
}
