#pragma once

#include "../Renderer.h"

#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan\vulkan.h"

#include <SDL.h>
#include <GL/glew.h>
#include "../glm/glm.hpp"

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")

// Size in bytes of the memory types used
#define STAGING_MEMORY_SIZE 256
#define STORAGE_MEMORY_SIZE (1024 * 1024)

class VulkanRenderer : public Renderer
{
public:
	struct MemoryTypeInfo
	{
		bool deviceLocal;
		bool hostVisible;
		bool hostCoherent;
	};

	VulkanRenderer();
	~VulkanRenderer();

	Material* makeMaterial(const std::string& name);
	Mesh* makeMesh();
	VertexBuffer* makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage);
	Texture2D* makeTexture2D();
	Sampler2D* makeSampler2D();
	RenderState* makeRenderState();
	std::string getShaderPath();
	std::string getShaderExtension();
	ConstantBuffer* makeConstantBuffer(std::string NAME, unsigned int location);
	Technique* makeTechnique(Material* m, RenderState* r);


	int initialize(unsigned int width = 640, unsigned int height = 480);
	void setWinTitle(const char* title);
	void present();
	int shutdown();

	void setClearColor(float r, float g, float b, float a);
	void clearBuffer(unsigned int flag);
	void setRenderState(RenderState* ps);
	void submit(Mesh* mesh);
	void frame();

	VkDevice getDevice();

	void setConstantBufferData(VkBuffer buffer, const void* data, size_t size, Material * m, unsigned int location);

private:
	std::vector<MemoryTypeInfo> memoryTypes;

	void createStagingBuffer();
	void updateStagingBuffer(const void* data, uint32_t size);	// Writes memory from data into the staging buffer
	void allocateStorageMemory();	// Used during initialization to find suitable memory type for storage and allocate device memory from it

	VkInstance instance;

	VkDevice device;

	bool globalWireframeMode = false;

	SDL_Window* window;

	VkSurfaceKHR windowSurface;
	VkSwapchainKHR swapchain;

	std::vector<VkImage> swapchainImages;			// Array of images in the swapchain, use vkAquireNextImageKHR(...) to aquire image for drawing to
	std::vector<VkImageView> swapchainImageViews;	// Image views for the swap chain images

	glm::vec4 clearColor;

	VkDeviceMemory stagingMemory;	// GPU memory allocation accessible to CPU. Used to move data from CPU to GPU
	VkDeviceMemory storageMemory;	// GPU memory allocation used to store data.
	uint32_t storageMemoryNextFreeOffset{ 0 };	// Offset to the next free area in the storage memory

	VkBuffer stagingBuffer;			// Buffer to temporarily hold data being transferred to GPU
	VkMemoryRequirements stagingBufferMemoryRequirement;

	int chosenQueueFamily;		// The queue family to be used

	VkCommandPool stagingCommandPool;	// Allocates commands used when moving data to the GPU
	VkCommandPool drawingCommandPool;	// Allocates commands used for drawing

	VkQueue queue;		// Handle to the queue used
};