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
const uint32_t STORAGE_SIZE[] = { 256, 1024*1024, 1024 * 1024, 1024 * 1024 };

struct DevMemoryAllocation
{
	VkDeviceMemory handle;	// The device memory handle
	uint32_t freeOffset;	// Offset to the next free area in the storage memory

	DevMemoryAllocation() : handle(NULL), freeOffset(0) {}
};
enum MemoryPool
{
	STAGING_BUFFER = 0,	// GPU memory allocation accessible to CPU. Used to move data from CPU to GPU
	UNIFORM_BUFFER = 1,	// GPU memory allocation used to store uniform data.
	VERTEX_BUFFER = 2,
	INDEX_BUFFER = 3
};

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
	VkPhysicalDevice getPhysical();

	/* Bind a physical memory partition on the device to the buffer from the specific memory pool. */
	uint32_t bindPhysicalMemory(VkBuffer buffer, uint32_t size, MemoryPool memPool);
	void transferBufferData(VkBuffer buffer, const void* data, size_t size, size_t offset);

	VkSurfaceFormatKHR getSwapchainFormat();

	unsigned int getWidth();
	unsigned int getHeight();

private:
	std::vector<MemoryTypeInfo> memoryTypes;

	void createStagingBuffer();
	void updateStagingBuffer(const void* data, uint32_t size);	// Writes memory from data into the staging buffer
	void allocateStorageMemory(MemoryPool type, uint32_t size, VkFlags usage);	// Allocates memory pool data

	VkInstance instance;

	VkDevice device;
	int chosenPhysicalDevice;
	VkPhysicalDevice physicalDevice;

	bool globalWireframeMode = false;

	SDL_Window* window;

	VkSurfaceKHR windowSurface;
	VkSwapchainKHR swapchain;

	std::vector<VkImage> swapchainImages;			// Array of images in the swapchain, use vkAquireNextImageKHR(...) to aquire image for drawing to
	std::vector<VkImageView> swapchainImageViews;	// Image views for the swap chain images

	glm::vec4 clearColor;

	/* Memory pool of device memory
	*/
	std::vector<DevMemoryAllocation> memPool;
	

	VkBuffer stagingBuffer;			// Buffer to temporarily hold data being transferred to GPU

	int chosenQueueFamily;		// The queue family to be used

	VkCommandPool stagingCommandPool;	// Allocates commands used when moving data to the GPU
	VkCommandPool drawingCommandPool;	// Allocates commands used for drawing

	VkQueue queue;		// Handle to the queue used

	VkSurfaceFormatKHR swapchainFormat;

	unsigned int width;
	unsigned int height;
};