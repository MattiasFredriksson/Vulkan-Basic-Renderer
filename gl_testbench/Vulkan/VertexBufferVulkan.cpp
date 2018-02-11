#include "VertexBufferVulkan.h"
#include <vulkan/vulkan.h>
#include <stdexcept>
#include "../VulkanConstruct.h"

/* Find a memory type on the device mathcing the specification
*/
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	// Iterate the different types matching the filter mask and find one that matches the properties:
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) &&  matchFlag(memProperties.memoryTypes[i].propertyFlags, properties)) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}

/* Create a vertex buffer of specific byte size exclusive to a single queue.
byte_size		<<	Byte size of the buffer.
return			>>	The vertex buffer handle.
*/
VkBuffer createVertexBuffer(VkDevice device, size_t byte_size)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size =  byte_size;
	bufferInfo.flags = 0;
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	// Indicates it'll only be accessed by a single queue at a time (presumably only use this option if it will only be used by one queue)
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// Since exclusive the queue params are redundant:
	bufferInfo.queueFamilyIndexCount = 0;
	bufferInfo.pQueueFamilyIndices = nullptr;

	VkBuffer buffer;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create vertex buffer!");
	}
	return buffer;
}
/* Allocate physical memory on the device for a buffer object.
device			<< Handle to the device.
physicalDevice	<< Handle to the physical device.
buffer			<< Related buffer.
properties		<< The properties the memory should have (dependent on buffer and how it's used).
*/
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer buffer, VkMemoryPropertyFlags properties)
{
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);

	//Allocate:
	// *Note that number allocations is limited, hence a custom allocator is required to allocate chunks for multiple buffers. 
	// *One possible solution is to use VulkanMemoryAllocator library.
	VkDeviceMemory mem;
	if (vkAllocateMemory(device, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}
	// Bind to buffer.
	vkBindBufferMemory(device, buffer, mem, 0);
	return mem;
}

VertexBufferVulkan::VertexBufferVulkan(VulkanRenderer *renderer, size_t size, VertexBuffer::DATA_USAGE usage)
	: _renderHandle(renderer), _bufferHandle(NULL), totalSize(size)
{
	// Create buffer and allocater physical memory for it
	_bufferHandle = createVertexBuffer(_renderHandle->getDevice(), size);
	_memHandle = allocPhysicalMemory(_renderHandle->getDevice(), _renderHandle->getPhysical(),
		_bufferHandle, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
}

VertexBufferVulkan::~VertexBufferVulkan()
{
	//Clean-up
	vkDestroyBuffer(_renderHandle->getDevice(), _bufferHandle, nullptr);
	vkFreeMemory(_renderHandle->getDevice(), _memHandle, nullptr);
}

void VertexBufferVulkan::setData(const void * data, size_t size, size_t offset)
{
}

void VertexBufferVulkan::bind(size_t offset, size_t size, unsigned int location)
{
}

void VertexBufferVulkan::unbind()
{
}

size_t VertexBufferVulkan::getSize()
{
	return totalSize;
}
