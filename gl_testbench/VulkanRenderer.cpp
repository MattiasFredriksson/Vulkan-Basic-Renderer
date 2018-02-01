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

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

VulkanRenderer::VulkanRenderer() { }
VulkanRenderer::~VulkanRenderer() { }

Material* VulkanRenderer::makeMaterial(const std::string& name)
{
	MaterialVulkan m("m");
	return &m;	// bad
}
Mesh* VulkanRenderer::makeMesh()
{
	MeshVulkan m = MeshVulkan();
	return &m;	// bad
}
VertexBuffer* VulkanRenderer::makeVertexBuffer(size_t size, VertexBuffer::DATA_USAGE usage)
{
	VertexBufferVulkan v(1, VertexBuffer::DATA_USAGE::DONTCARE);
	return &v;	// bad
}
Texture2D* VulkanRenderer::makeTexture2D()
{
	Texture2DVulkan t;
	return &t;	// bad
}
Sampler2D* VulkanRenderer::makeSampler2D()
{
	Sampler2DVulkan s;
	return &s;	// bad
}
RenderState* VulkanRenderer::makeRenderState()
{
	RenderStateVulkan r;
	return &r;	// bad
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
	ConstantBufferVulkan c("temp", 0);
	return &c;	// bad
}
Technique* VulkanRenderer::makeTechnique(Material* m, RenderState* r)
{
	Technique t(m, r);
	return &t;	// bad
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

	VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &vulkanInstance);

	if (result != VK_SUCCESS)
		throw "Failed to create Vulkan instance";

	// Find a physical device suitable for rendering
	uint32_t numPhysicalDevices = 0;

	// Ask for number of devices by setting last parameter to nullptr
	vkEnumeratePhysicalDevices(vulkanInstance, &numPhysicalDevices, nullptr);
	// Resize the array
	physicalDevices.resize(numPhysicalDevices);
	// Fill the array with physical device handles
	vkEnumeratePhysicalDevices(vulkanInstance, &numPhysicalDevices, &physicalDevices[0]);

	// Resize arrays
	physicalDeviceProperties.resize(numPhysicalDevices);
	physicalDeviceFeatures.resize(numPhysicalDevices);
	physicalDeviceMemoryProperties.resize(numPhysicalDevices);
	physicalDeviceQueueFamilyProperties.resize(numPhysicalDevices);
	// Fill the array
	for (uint32_t i = 0; i < numPhysicalDevices; ++i)
	{
		vkGetPhysicalDeviceProperties(physicalDevices[i], &physicalDeviceProperties[i]);
		vkGetPhysicalDeviceFeatures(physicalDevices[i], &physicalDeviceFeatures[i]);
		vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &physicalDeviceMemoryProperties[i]);

		// Ask for number of queues on the device and resize the array
		uint32_t numQueues = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &numQueues, nullptr);
		physicalDeviceQueueFamilyProperties[i].resize(numQueues);

		// Get queue info
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &numQueues, &physicalDeviceQueueFamilyProperties[i][0]);
	}

	chosenPhysicalDevice = -1;

	// Check for a discrete GPU
	for (uint32_t i = 0; i < numPhysicalDevices; ++i)
		if (physicalDeviceProperties[i].deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			chosenPhysicalDevice = i;
			break;
		}

	// Then, if no discrete GPU was found, check for an integrated GPU
	if (chosenPhysicalDevice == -1)
		for (uint32_t i = 0; i < numPhysicalDevices; ++i)
			if (physicalDeviceProperties[i].deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			{
				chosenPhysicalDevice = i;
				break;
			}
	
	// If no GPU has been found, throw error
	if (chosenPhysicalDevice == -1)
		throw "No discrete or integrated GPU found";

	// Create logical device

	// First attempt to find a queue family supporting both transfer and graphics commands
	int chosenQueueFamily = -1;
	for (uint32_t i = 0; i < physicalDeviceQueueFamilyProperties[chosenPhysicalDevice].size(); ++i)
	{
		if ((physicalDeviceQueueFamilyProperties[chosenPhysicalDevice][i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
			(physicalDeviceQueueFamilyProperties[chosenPhysicalDevice][i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			chosenQueueFamily = i;
			break;
		}
	}

	if (chosenQueueFamily == -1)
		throw "No simple queue solution found. Do more programming >:|";

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

	// Create device
	vkCreateDevice(physicalDevices[chosenPhysicalDevice], &deviceCreateInfo, nullptr, &device);

	// create window
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(window, this);
	glfwCreateWindowSurface(vulkanInstance, window, nullptr, &windowSurface);

	// Check that queue supports presenting
	VkBool32 presentingSupported;
	vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[chosenPhysicalDevice], chosenQueueFamily, windowSurface, &presentingSupported);
	
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[chosenPhysicalDevice], windowSurface, &surfaceCapabilities);

	if (presentingSupported == VK_FALSE)
		throw "The selected queue does not support presenting. Do more programming >:|";

	// Get supported formats
	uint32_t numFormats;
	std::vector<VkSurfaceFormatKHR> formats;
	// Get number of formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[chosenPhysicalDevice], windowSurface, &numFormats, nullptr);
	// Resize array
	formats.resize(numFormats);
	// Finally, get the supported formats
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[chosenPhysicalDevice], windowSurface, &numFormats, &formats[0]);

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
	swapchainCreateInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // vkGetPhysicalDeviceSurfacePresentModesKHR(...) should be called to check valid present modes
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
}
void VulkanRenderer::setWinTitle(const char* title)
{

}
void VulkanRenderer::present()
{

}
int VulkanRenderer::shutdown()
{
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