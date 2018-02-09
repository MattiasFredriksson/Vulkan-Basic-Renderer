#pragma once
#include <GL/glew.h>
#include "../ConstantBuffer.h"
#include "VulkanRenderer.h"

class ConstantBufferVulkan : public ConstantBuffer
{
public:
	ConstantBufferVulkan(std::string NAME, unsigned int location);
	~ConstantBufferVulkan();
	void setData(const void* data, size_t size, Material* m, unsigned int location);
	void bind(Material*);
	void init(VulkanRenderer* renderer);

private:
	VulkanRenderer* renderer;

	std::string name;
	uint32_t location;

	VkBuffer buffer;
};

