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
#include <SDL_syswm.h>
#include <assert.h>
#include <iostream>

#include "VulkanConstruct.h"



VulkanRenderer::VulkanRenderer() { }
VulkanRenderer::~VulkanRenderer() { }

Material* VulkanRenderer::makeMaterial(const std::string& name)
{
	return (Material*) new MaterialVulkan(name);
}
Mesh* VulkanRenderer::makeMesh()
{
	return (Mesh*) new MeshVulkan();
}
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage)
{
	return (VertexBuffer*) new VertexBufferVulkan(1, VertexBuffer::DATA_USAGE::DONTCARE);
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
	return "temp";
}
std::string VulkanRenderer::getShaderExtension()
{
	return "temp";
}
ConstantBuffer* VulkanRenderer::makeConstantBuffer(std::string NAME, unsigned int location)
{
	return (ConstantBuffer*) new ConstantBufferVulkan(NAME, location);
}
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r)
{
	return (Technique*) new Technique(m, r);
}

int VulkanRenderer::initialize(unsigned int width, unsigned int height)
{
	// Create Vulkan instance
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

	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

	if (result != VK_SUCCESS)
		throw "Failed to create Vulkan instance";

	VkPhysicalDevice physicalDevice = choosePhysicalDevice(instance);

	// Create logical device

	// Find a suitable queue family
	VkQueueFlags prefQueueFlag[] =
	{
		(VK_QUEUE_TRANSFER_BIT | VK_QUEUE_GRAPHICS_BIT)
	};
	int chosenQueueFamily = chooseQueueFamily(physicalDevice, prefQueueFlag, sizeof(prefQueueFlag) / sizeof(VkQueueFlags));

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

	// Initiate SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0)
	{
		fprintf(stderr, "%s", SDL_GetError());
		exit(-1);
	}
	
	// Create window
	window = SDL_CreateWindow("Vulkan", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_SHOWN);

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

	// Check that queue supports presenting
	VkBool32 presentingSupported;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, chosenQueueFamily, windowSurface, &presentingSupported);

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, windowSurface, &surfaceCapabilities);

	if (presentingSupported == VK_FALSE)
		throw "The selected queue does not support presenting. Do more programming >:|";

	// Get supported formats
	uint32_t numFormats;
	std::vector<VkSurfaceFormatKHR> formats;
	// Get number of formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &numFormats, nullptr);
	// Resize array
	formats.resize(numFormats);
	// Finally, get the supported formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, windowSurface, &numFormats, &formats[0]);

	// Create swap chain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = nullptr;
	swapchainCreateInfo.flags = 0;
	swapchainCreateInfo.surface = windowSurface;
	swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount;
	swapchainCreateInfo.imageFormat = formats[0].format;	// Just select the first available format
	swapchainCreateInfo.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

	VkPresentModeKHR presentModePref[] =
	{
#ifdef NO_VSYNC
		VK_PRESENT_MODE_IMMEDIATE_KHR,// Immediately presents images to screen
#endif
		VK_PRESENT_MODE_MAILBOX_KHR
	};
	VkPresentModeKHR presentMode = chooseSwapPresentMode(physicalDevice, windowSurface, presentModePref, sizeof(presentModePref)/sizeof(VkPresentModeKHR));
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
		throw "Failed to create swapchain";

	// Aquire the swapchain images
	uint32_t numSwapchainImages;
	// First ask for number of images
	vkGetSwapchainImagesKHR(device, swapchain, &numSwapchainImages, nullptr);
	// Resize array
	swapchainImages.resize(numSwapchainImages);
	// Get the images
	result = vkGetSwapchainImagesKHR(device, swapchain, &numSwapchainImages, &swapchainImages[0]);
	if (result != VK_SUCCESS)
		throw "Failed to get swapchain images";

	// assign queues/command buffers??
	return 0;
}
void VulkanRenderer::setWinTitle(const char* title)
{

}
void VulkanRenderer::present()
{

}
int VulkanRenderer::shutdown()
{
	vkDestroyInstance(instance, nullptr);
	SDL_DestroyWindow(window);
	SDL_Quit();
	return 0;	// temp
}

void VulkanRenderer::setClearColor(float r, float g, float b, float a)
{

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