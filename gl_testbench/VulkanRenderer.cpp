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

	// create logical device

	// create window

	// create swap chain

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