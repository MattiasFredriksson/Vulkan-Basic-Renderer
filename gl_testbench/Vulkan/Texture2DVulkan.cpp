#include "Texture2DVulkan.h"
#include "stb_image.h"
#include "../VulkanConstruct.h"
#include "VulkanRenderer.h"

Texture2DVulkan::Texture2DVulkan(VulkanRenderer *renderer)
	: _renderHandle(renderer), _imageHandle(nullptr), imageInfo({NULL, NULL, VK_IMAGE_LAYOUT_UNDEFINED })
{
}

Texture2DVulkan::~Texture2DVulkan()
{
	destroyImg();
}

void Texture2DVulkan::destroyImg()
{
	if (_imageHandle)
	{
		vkDestroyImageView(_renderHandle->getDevice(), imageInfo.imageView, nullptr);
		vkDestroyImage(_renderHandle->getDevice(), _imageHandle, nullptr);
	}
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

	// Destroy image if it exists
	destroyImg();

	VkFormat format;
	uint32_t bytes; 
	if (bpp == 3)
	{
		//Warning! Might not be supported
		format = VK_FORMAT_R8G8B8_UNORM;
		bytes = 3;
	}
	else if (bpp == 4)
	{
		// Standard support format
		format = VK_FORMAT_R8G8B8A8_UNORM;
		bytes = 4;
	}
	else
		throw std::runtime_error("Image format not supported...");
	_imageHandle = createTexture2D(_renderHandle->getDevice(), w, h, format);
	_renderHandle->bindPhysicalMemory(_imageHandle, MemoryPool::IMAGE_RGBA8_BUFFER);
	// Create and transfer image
	_renderHandle->transitionImageFormat(_imageHandle, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	_renderHandle->transferImageData(_imageHandle, rgb, glm::uvec3(w, h, 1), bytes);
	stbi_image_free(rgb);
	_renderHandle->transitionImageFormat(_imageHandle, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// Create image view
	imageInfo.imageView = createImageView(_renderHandle->getDevice(), _imageHandle, format);
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	return 0;
}

void Texture2DVulkan::bind(unsigned int slot)
{
	Sampler2DVulkan *vksamp = dynamic_cast<Sampler2DVulkan*>(sampler);
	if (!vksamp)
		throw std::runtime_error("No suitable sampler, create a default sampler...");
	imageInfo.sampler = vksamp->_samplerHandle;
}
