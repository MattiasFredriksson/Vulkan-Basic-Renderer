#include "Texture2DVulkan.h"
#include "stb_image.h"
#include "../VulkanConstruct.h"
#include "VulkanRenderer.h"

Texture2DVulkan::Texture2DVulkan(VulkanRenderer *renderer)
	: _renderHandle(renderer), _imageHandle(nullptr)
{
}

Texture2DVulkan::~Texture2DVulkan()
{
}

int Texture2DVulkan::loadFromFile(std::string filename)
{
	int w, h, bpp;
	unsigned char* rgb = stbi_load(filename.c_str(), &w, &h, &bpp, STBI_rgb_alpha);
	if (rgb == nullptr)
	{
		fprintf(stderr, "Error loading texture file: %s\n", filename.c_str());
		return -1;
	}

	

	// not 0
	if (_imageHandle)
	{
		vkDestroyImage(_renderHandle->getDevice(), _imageHandle, nullptr);
	}

	VkFormat format;
	size_t bytes = w*h; 
	if (bpp == 3)
	{
		format = VK_FORMAT_R8G8B8_UNORM;
		bytes *= 3;
	}
	else if (bpp == 4)
	{
		format = VK_FORMAT_R8G8B8A8_UNORM;
		bytes *= 4;
	}
	else
		throw std::runtime_error("Image format not supported...");
	_imageHandle = createTexture2D(_renderHandle->getDevice(), w, h, format);


	_renderHandle->transitionImageFormat(_imageHandle, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	_renderHandle->transferImageData(_imageHandle, rgb, bytes, 0);
	stbi_image_free(rgb);
	_renderHandle->transitionImageFormat(_imageHandle, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	return 0;
}

void Texture2DVulkan::bind(unsigned int slot)
{
}
