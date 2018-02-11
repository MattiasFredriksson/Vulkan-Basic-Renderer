#include <vector>

#include "Vulkan\VulkanRenderer.h"
#include "Vulkan\MaterialVulkan.h"
#include "Vulkan\MeshVulkan.h"
#include "Vulkan\VertexBufferVulkan.h"
#include "Vulkan\Texture2DVulkan.h"
#include "Vulkan\Sampler2DVulkan.h"
#include "Vulkan\RenderStateVulkan.h"
#include "Vulkan\ConstantBufferVulkan.h"
#include "Technique.h"
#include "TechniqueVulkan.h"
#include <SDL_syswm.h>
#include <assert.h>
#include <iostream>

#define VULKAN_DEVICE_IMPLEMENTATION
#include "VulkanConstruct.h"



VulkanRenderer::VulkanRenderer()
 : memPool((int)MemoryPool::INDEX_BUFFER + 1) 
{
}
VulkanRenderer::~VulkanRenderer() { }

Material* VulkanRenderer::makeMaterial(const std::string& name)
{
	MaterialVulkan* m = new MaterialVulkan(name);
	m->setRenderer(this);
	return (Material*)m;
}
Mesh* VulkanRenderer::makeMesh()
{
	return (Mesh*) new MeshVulkan();
}
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage)
{
	return (VertexBuffer*) new VertexBufferVulkan(this, size, usage);
}
Texture2D* VulkanRenderer::makeTexture2D()
{
	return (Texture2D*) new Texture2DVulkan();
}
Sampler2D* VulkanRenderer::makeSampler2D()
{
	return (Sampler2D*) new Sampler2DVulkan();
}
RenderState* VulkanRenderer::makeRenderState()
{
	RenderStateVulkan* newRS = new RenderStateVulkan();
	newRS->setGlobalWireFrame(&this->globalWireframeMode);
	newRS->setWireFrame(false);
	return (RenderState*)newRS;
}
std::string VulkanRenderer::getShaderPath()
{
	return "..\\assets\\GL45\\";
}
std::string VulkanRenderer::getShaderExtension()
{
	return ".glsl";
}
ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string NAME, unsigned int location)
{
	ConstantBufferVulkan* cb = new ConstantBufferVulkan(NAME, location);
	cb->init(this);
	return (ConstantBuffer*) cb;
}
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r)
{
	return (Technique*) new TechniqueVulkan(m, r, this);
}

int VulkanRenderer::initialize(unsigned int width, unsigned int height)
{
	this->width = width;
	this->height = height;

	/* Create Vulkan instance
	*/

	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = nullptr;
	applicationInfo.pApplicationName = "Basic Vulkan Renderer";
	applicationInfo.applicationVersion = 1;
	applicationInfo.pEngineName = "";
	applicationInfo.apiVersion = 0;		// setting to 1 causes error when creating instance

	char* enabledLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	char* enabledExtensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_report" };
	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nullptr;
	instanceCreateInfo.flags = 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = 1;
	instanceCreateInfo.ppEnabledLayerNames = enabledLayers;
	instanceCreateInfo.enabledExtensionCount = 3;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

	// Create instance
	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create Vulkan instance");

	/* Create window
	*/

	// Initiate SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}

	// Create window
	window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);

	/* Create window surface
	Note* Surface 'must' be created before physical device creation as it influences the selection.
	*/

	// Get the window version
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	assert(SDL_GetWindowWMInfo(window, &info) == SDL_TRUE);

	// Utilize version to create the window surface
	VkWin32SurfaceCreateInfoKHR w32sci = {};
	w32sci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	w32sci.pNext = NULL;
	w32sci.hinstance = GetModuleHandle(NULL);
	w32sci.hwnd = info.info.win.window;
	assert(
		vkCreateWin32SurfaceKHR(
			instance,
			&w32sci,
			nullptr,
			&windowSurface)
		== VK_SUCCESS);


	/* Create physical device
	*/

	VkQueueFlags queueSupport = (VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT);
	chosenPhysicalDevice = choosePhysicalDevice(instance, windowSurface, vk::specifyAnyDedicatedDevice, queueSupport, physicalDevice);
	if (chosenPhysicalDevice < 0)
		throw std::runtime_error("No available physical device matched specification.");
	
	/* Create logical device
	*/

	// Find a suitable queue family
	VkQueueFlags prefQueueFlag[] =
	{
		(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT)
	};
	chosenQueueFamily = pickQueueFamily(physicalDevice, prefQueueFlag, sizeof(prefQueueFlag) / sizeof(VkQueueFlags));

	// Info on queues
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.pNext = nullptr;
	queueCreateInfo.flags = 0;
	queueCreateInfo.queueFamilyIndex = chosenQueueFamily;
	queueCreateInfo.queueCount = 1;
	float prios[] = { 1.0f };
	queueCreateInfo.pQueuePriorities = prios;

	// Info on device
	char* deviceLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
	char* deviceExtensions[] = { "VK_KHR_swapchain" };

	VkDeviceCreateInfo deviceCreateInfo = {};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = nullptr;
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

	deviceCreateInfo.enabledLayerCount = 1;
	deviceCreateInfo.ppEnabledLayerNames = deviceLayers;

	deviceCreateInfo.enabledExtensionCount = 1;
	deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

	VkPhysicalDeviceFeatures deviceFeatures = {}; // empty
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

	// Create (vulkan) device
	vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);

	vkGetDeviceQueue(device, chosenQueueFamily, 0, &queue);

	// Allocate storage buffers
	createStagingBuffer();
	allocateStorageMemory(MemoryPool::UNIFORM_BUFFER, STORAGE_SIZE[MemoryPool::UNIFORM_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	allocateStorageMemory(MemoryPool::VERTEX_BUFFER, STORAGE_SIZE[MemoryPool::VERTEX_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	allocateStorageMemory(MemoryPool::INDEX_BUFFER, STORAGE_SIZE[MemoryPool::INDEX_BUFFER], VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

	/* Create swap chain
	*/

	// Check that queue supports presenting
	VkBool32 presentingSupported;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, chosenQueueFamily, windowSurface, &presentingSupported);

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities);

	if (presentingSupported == VK_FALSE)
		throw std::runtime_error("The selected queue does not support presenting. Do more programming >:|");

	// Get supported formats
	std::vector<VkSurfaceFormatKHR> formats;
	ALLOC_QUERY_ASSERT(result, vkGetPhysicalDeviceSurfaceFormatsKHR, formats, physicalDevice, windowSurface);
	swapchainFormat = formats[0];

	// Choose the mode for the swap chain that determines how the frame buffers are swapped.
	VkPresentModeKHR presentModePref[] =
	{
#ifdef NO_VSYNC
		VK_PRESENT_MODE_IMMEDIATE_KHR,// Immediately present images to screen
#endif
		VK_PRESENT_MODE_MAILBOX_KHR // Oldest finished frame are replaced if 'framebuffer' queue is filled.
	};
	VkPresentModeKHR presentMode = chooseSwapPresentMode(physicalDevice, windowSurface, presentModePref, sizeof(presentModePref) / sizeof(VkPresentModeKHR));

	// Create swap chain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = windowSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = formats[0].format;	// Just select the first available format
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	VkExtent2D swapchainExtent;
	swapchainExtent.height = height;
	swapchainExtent.width = width;
	swapchainCreateInfo.imageExtent = swapchainExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_FALSE;
	swapchainCreateInfo.oldSwapchain = NULL;

	result = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create swapchain");

	// Aquire the swapchain images
	ALLOC_QUERY_ASSERT(result, vkGetSwapchainImagesKHR, swapchainImages, device, swapchain);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to get swapchain images");

	// Create image views for the swapchain images
	swapchainImageViews.resize(swapchainImages.size());
	for (int i = 0; i < swapchainImages.size(); ++i)
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.pNext = nullptr;
		imageViewCreateInfo.flags = 0;
		imageViewCreateInfo.image = swapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = swapchainCreateInfo.imageFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		result = vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapchainImageViews[i]);
		if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to create image view");
	}

	// Create staging command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = nullptr;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	commandPoolCreateInfo.queueFamilyIndex = chosenQueueFamily;
	result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &stagingCommandPool);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create staging command pool.");

	// Create drawing command pool
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &drawingCommandPool);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create staging command pool.");


	return 0;
}
void VulkanRenderer::setWinTitle(const char* title)
{
	SDL_SetWindowTitle(window, title);
}
void VulkanRenderer::present()
{

}
int VulkanRenderer::shutdown()
{
	// Clean up Vulkan
	vkDestroyCommandPool(device, drawingCommandPool, nullptr);
	vkDestroyCommandPool(device, stagingCommandPool, nullptr);
	vkDestroyBuffer(device, stagingBuffer, nullptr);


	for (int i = 0; i < swapchainImageViews.size(); ++i)
		vkDestroyImageView(device, swapchainImageViews[i], nullptr);
	for (int i = 0; i < swapchainImages.size(); ++i)
		vkDestroyImage(device, swapchainImages[i], nullptr);
	for(uint32_t i = 0; i < memPool.size(); i++)
		vkFreeMemory(device, memPool[i].handle, nullptr);

	vkDestroySwapchainKHR(device, swapchain, nullptr);
	vkDestroySurfaceKHR(instance, windowSurface, nullptr);
	vkDestroyDevice(device, nullptr);
	vkDestroyInstance(instance, nullptr);

	//Clean up SDL
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;	// temp
}

void VulkanRenderer::setClearColor(float r, float g, float b, float a)
{
	clearColor = glm::vec4{ r, g, b, a };
}
void VulkanRenderer::clearBuffer(unsigned int flag)
{

}
void VulkanRenderer::setRenderState(RenderState* ps)
{

}
void VulkanRenderer::submit(Mesh* mesh)
{

}
void VulkanRenderer::frame()
{

}

VkDevice VulkanRenderer::getDevice()
{
	return device;
}

VkPhysicalDevice VulkanRenderer::getPhysical()
{
	return physicalDevice;
}

size_t VulkanRenderer::bindPhysicalMemory(VkBuffer buffer, size_t size, MemoryPool pool)
{
	// Adjust the memory offset to achieve proper alignment
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);

	size_t freeOffset = memPool[pool].freeOffset;
	if (freeOffset % memoryRequirements.alignment != 0)
		freeOffset += memoryRequirements.alignment - (freeOffset % memoryRequirements.alignment);

	VkResult result = vkBindBufferMemory(device, buffer, memPool[pool].handle, freeOffset);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to bind buffer to memory.");

	// Update offset
	memPool[pool].freeOffset = freeOffset + size;
	return freeOffset;
}

void VulkanRenderer::transferBufferData(VkBuffer buffer, const void* data, size_t size, size_t offset)
{
	updateStagingBuffer(data, size);

	// Create command buffer
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.commandPool = stagingCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer stagingCommandBuffer;
	VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &stagingCommandBuffer);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create command buffer for staging.");

	// Begin recording into command buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	result = vkBeginCommandBuffer(stagingCommandBuffer, &commandBufferBeginInfo);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to begin command buffer.");

	// Record the copying command
	VkBufferCopy bufferCopyRegion = {};
	bufferCopyRegion.srcOffset = 0;
	bufferCopyRegion.dstOffset = offset;
	bufferCopyRegion.size = size;

	vkCmdCopyBuffer(stagingCommandBuffer, stagingBuffer, buffer, 1, &bufferCopyRegion);
	
	// End command recording
	vkEndCommandBuffer(stagingCommandBuffer);

	// Submit command buffer to queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &stagingCommandBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);		// Wait until the copy is complete

	vkFreeCommandBuffers(device, stagingCommandPool, 1, &stagingCommandBuffer);
}

VkSurfaceFormatKHR VulkanRenderer::getSwapchainFormat()
{
	return swapchainFormat;
}

unsigned int VulkanRenderer::getWidth()
{
	return width;
}

unsigned int VulkanRenderer::getHeight()
{
	return height;
}

void VulkanRenderer::updateStagingBuffer(const void* data, size_t size)
{
	if (size > STORAGE_SIZE[MemoryPool::STAGING_BUFFER])
		throw std::runtime_error("The data requested does not fit in the staging buffer.");

	void* bufferContents = nullptr;
	VkResult result = vkMapMemory(device, memPool[MemoryPool::STAGING_BUFFER].handle, 0, VK_WHOLE_SIZE, 0, &bufferContents);

	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to map staging buffer to memory.");

	memcpy(bufferContents, data, size);

	vkUnmapMemory(device, memPool[MemoryPool::STAGING_BUFFER].handle);
}

void VulkanRenderer::createStagingBuffer()
{
	stagingBuffer = createBuffer(device, STORAGE_SIZE[MemoryPool::STAGING_BUFFER], VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
	memPool[MemoryPool::STAGING_BUFFER].handle = allocPhysicalMemory(device, physicalDevice, stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
}

void VulkanRenderer::allocateStorageMemory(MemoryPool type, size_t size, VkFlags usage)
{
	VkBuffer dummy = createBuffer(device, size, usage);
	memPool[type].handle = allocPhysicalMemory(device, physicalDevice, dummy, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkDestroyBuffer(device, dummy, nullptr);
}
