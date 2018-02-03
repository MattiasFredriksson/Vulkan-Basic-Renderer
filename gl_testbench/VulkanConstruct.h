#pragma once

#include "vulkan\vulkan.h"
#include <assert.h>
#include <algorithm>

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
#ifdef _DEBUG
	// Output present modes:
	std::cout << presentModes.size() << " present mode(s)\n";
	for (size_t i = 0; i < presentModes.size() - 1; i++)
		std::cout << presentModes[i] << ", ";
	std::cout << presentModes[presentModes.size() - 1] << "\n";
#endif

	// Find an acceptable present mode
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
	ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, queueFamilyProperties, device);

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

namespace vk
{
	/*	Determines if a physical device is suitable for the system. Return rank of the device, ranks greater then 0 will be available for selection. */
	typedef int(*isDeviceSuitable)(VkPhysicalDevice &device, VkPhysicalDeviceProperties &prop, VkPhysicalDeviceFeatures &feat, VkQueueFamilyProperties *queue_family_prop, size_t len_family_prop);

	/* Function selecting any dedicated device making a preference for discrete over integrated devices.
	*/
	int specifyAnyDedicatedDevice(VkPhysicalDevice &device, VkPhysicalDeviceProperties &prop, VkPhysicalDeviceFeatures &feat, VkQueueFamilyProperties *queue_family_prop, size_t len_family_prop)
	{
		if (prop.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			return 2;
		else if (prop.deviceType == VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
			return 1;
		return 0;
	}
}

/* Abstract device selection function using a ranking function to select device. 
instance	<<	Vulkan instance
deviceSpec	<<	Ranking function of available physical devices used to determine a suitable device.
result		>>	Selected device
return		>>	Positive: index of the related device, Negative: Error code
*/
int choosePhysicalDevice(VkInstance &instance, vk::isDeviceSuitable deviceSpec, VkPhysicalDevice &result)
{
	// Handles to the physical devices detected
	std::vector<VkPhysicalDevice> physicalDevices;
						
	// Query for suitable devices
	VkResult err;
	ALLOC_QUERY_ASSERT(err, vkEnumeratePhysicalDevices, physicalDevices, instance);
		if (physicalDevices.size() == 0)
			throw std::runtime_error("Failed to find GPUs with Vulkan support!");

	VkPhysicalDeviceProperties property;
	VkPhysicalDeviceFeatures feature;
	VkPhysicalDeviceMemoryProperties memProperty;
	std::vector<VkQueueFamilyProperties> familyProperty;

	struct DevPair
	{
		int index, rank;
	};
	std::vector<DevPair> list;
	list.resize(physicalDevices.size());
	// Check for a discrete GPU
	for (uint32_t i = 0; i < physicalDevices.size(); ++i)
	{
		vkGetPhysicalDeviceProperties(physicalDevices[i], &property);									// Holds properties of corresponding physical device in physicalDevices
		vkGetPhysicalDeviceFeatures(physicalDevices[i], &feature);										// Holds features of corresponding physical device in physicalDevices
		vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &memProperty);							// Holds memory properties of corresponding physical device in physicalDevices
		ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, familyProperty, physicalDevices[i]);		// Holds queue properties of corresponding physical device in physicalDevices
		// Find suitable device
		int rank = deviceSpec(physicalDevices[i], property, feature, familyProperty.data(), familyProperty.size());
		if (rank > 0)
			list.push_back({ (int)i, rank });
	}
	if (list.size() == 0)
	{
		//throw std::runtime_error("Failed to find physical device matching specification!");
		return -1;
	}
	//Select suitable device
	DevPair dev = list[0];
	for (size_t i = 1; i < list.size(); i++)
	{
		if (list[i].rank > dev.rank)
			dev = list[i];
	}
	//Return selected device
	result = physicalDevices[dev.index];
	return dev.index;
}

#endif
