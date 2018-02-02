#pragma once

#include "vulkan\vulkan.h"
#include <assert.h>

#define ALLOC_QUERY(fn, vec, ...) { unsigned int count=0; fn(__VA_ARGS__, &count, nullptr); vec.resize(count); fn(__VA_ARGS__, &count, vec.data()); }
#define ALLOC_QUERY_ASSERT(result, fn, vec, ...) { unsigned int count=0; fn(__VA_ARGS__, &count, nullptr); vec.resize(count); result = fn(__VA_ARGS__, &count, vec.data()); assert(result == VK_SUCCESS); }

#define VULKAN_DEVICE_IMPLEMENTATION
#ifdef VULKAN_DEVICE_IMPLEMENTATION

#include <vector>


/* Check if a mode is available in the list*/
template<class T>
bool hasMode(int mode, T *mode_list, size_t list_len)
{
	for (size_t i = 0; i < list_len; i++)
	{
		if (mode_list[i] == mode)
			return true;
	}
	return false;
}
/* Check if the flags are set in the property. */
template<class T>
bool hasFlag(T property, T flags)
{
	return (property & flags) == flags;
}

VkPresentModeKHR chooseSwapPresentMode(VkPhysicalDevice &device, VkSurfaceKHR &surface, VkPresentModeKHR *prefered_modes, size_t num_prefered) {

	// Find available present modes of the device
	std::vector<VkPresentModeKHR> presentModes;
	VkResult err;
	ALLOC_QUERY_ASSERT(err, vkGetPhysicalDeviceSurfacePresentModesKHR, presentModes, device, surface);
#ifdef DEBUG
	// Output present modes:
	std::cout << presentModes.size() << " present mode(s)\n";
	for (size_t i = 0; i < presentModes.size() - 1; i++)
		std::cout << presentModes[i] << ", ";
	std::cout << presentModes[presentModeCount - 1] << "\n";
#endif

	// Find an acceptable present mode
	VkPresentModeKHR presentMode;
	for (size_t i = 0; i < num_prefered; i++)
	{
		if (hasMode(prefered_modes[i], presentModes.data(), presentModes.size()))
			return prefered_modes[i];
	}
	// 'Guaranteed' to exist
	return VK_PRESENT_MODE_FIFO_KHR;
}

/* Choose a suitable queue family from by matching with the queue flag properties.
*/
int chooseQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag)
{
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;		// Holds queue properties of corresponding physical device in physicalDevices
	ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, queueFamilyProperties, device)

	//Find queue matching the queue flags:
	for (int f = 0; f < num_flag; f++)
	{
		VkQueueFlags flag = pref_queueFlag[f];
		for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
		{
			if (hasFlag(queueFamilyProperties[i].queueFlags, flag))
			{
				// Match with other properties ..?
				return i;
			}
		}
	}

	// No matching queue found
	return -1;
}

/* Find the first discrete GPU device (or the first integrated if there are no discrete devices).
*/
VkPhysicalDevice choosePhysicalDevice_First(VkInstance &instance)
{

	std::vector<VkPhysicalDevice> physicalDevices;									// Holds handles to the physical devices detected
	std::vector<VkPhysicalDeviceProperties> physicalDeviceProperties;				// Holds properties of corresponding physical device in physicalDevices
	//Params related (but currently not used in selection):
	std::vector<VkPhysicalDeviceFeatures> physicalDeviceFeatures;					// Holds features of corresponding physical device in physicalDevices
	std::vector<VkPhysicalDeviceMemoryProperties> physicalDeviceMemoryProperties;	// Holds memory properties of corresponding physical device in physicalDevices
	std::vector<std::vector<VkQueueFamilyProperties>> physicalDeviceQueueFamilyProperties;		// Holds queue properties of corresponding physical device in physicalDevices
	// Find a physical device suitable for rendering
	uint32_t numPhysicalDevices = 0;

	// Ask for number of devices by setting last parameter to nullptr
	vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, nullptr);
	// Resize the array
	physicalDevices.resize(numPhysicalDevices);
	// Fill the array with physical device handles
	vkEnumeratePhysicalDevices(instance, &numPhysicalDevices, &physicalDevices[0]);

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

		ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, physicalDeviceQueueFamilyProperties[i], physicalDevices[i])
	}

	int chosenPhysicalDevice = -1;

	// Check for a discrete GPU
	for (uint32_t i = 0; i < numPhysicalDevices; ++i)
		if (physicalDeviceProperties[i].deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			chosenPhysicalDevice = i;
			break;
		}

	// Then, if no discrete GPU was found, check for an integrated GPU
	if (chosenPhysicalDevice == -1)
	{
		for (uint32_t i = 0; i < numPhysicalDevices; ++i)
			if (physicalDeviceProperties[i].deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			{
				chosenPhysicalDevice = i;
				break;
			}
	}

	// If no GPU has been found, throw error
	if (chosenPhysicalDevice == -1)
		throw "No discrete or integrated GPU found";

	// Return the device choosen
	return physicalDevices[chosenPhysicalDevice];
}
#endif
