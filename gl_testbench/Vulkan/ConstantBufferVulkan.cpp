#include "ConstantBufferVulkan.h"

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
		VkBufferCreateInfo bufferCreateInfo = {};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.pNext = nullptr;
		bufferCreateInfo.flags = 0;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferCreateInfo.queueFamilyIndexCount = 0;
		bufferCreateInfo.pQueueFamilyIndices = nullptr;

		VkResult result = vkCreateBuffer(renderer->getDevice(), &bufferCreateInfo, nullptr, &buffer);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to create constant buffer.");
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
