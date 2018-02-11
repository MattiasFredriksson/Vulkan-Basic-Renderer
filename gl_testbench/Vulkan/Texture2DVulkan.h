#pragma once

#include <GL/glew.h>

#include "../Texture2D.h"
#include "Sampler2DVulkan.h"
#include <vulkan/vulkan.h>

class VulkanRenderer;

class Texture2DVulkan :
	public Texture2D
{
private:
	void destroyImg();
public:
	Texture2DVulkan(VulkanRenderer *renderer);
	~Texture2DVulkan();

	int loadFromFile(std::string filename);
	void bind(unsigned int slot);

	VulkanRenderer *_renderHandle;
	VkImage _imageHandle;
	VkImageView _viewHandle;
};

