#pragma once

#include "vulkan\vulkan.h"
#include <assert.h>
#include <algorithm>
#include <vector>

#pragma region Inline and type defs

#define ALLOC_QUERY_NOPARAM(fn, vec) { unsigned int count=0; fn(&count, nullptr); vec.resize(count); fn(&count, vec.data()); }
#define ALLOC_QUERY(fn, vec, ...) { unsigned int count=0; fn(__VA_ARGS__, &count, nullptr); vec.resize(count); fn(__VA_ARGS__, &count, vec.data()); }
#define ALLOC_QUERY_ASSERT(result, fn, vec, ...) { unsigned int count=0; fn(__VA_ARGS__, &count, nullptr); vec.resize(count); result = fn(__VA_ARGS__, &count, vec.data()); assert(result == VK_SUCCESS); }


/* Check if a mode is available in the list*/
template<class T>
inline bool hasMode(int mode, T *mode_list, size_t list_len)
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
inline bool hasFlag(T property, T flags)
{
	return (property & flags) == flags;
}
/* Find if the flags are equal. */
template<class T>
inline bool matchFlag(T property, T flags)
{
	return property == flags;
}
/* Unset bit in the flag. */
template<class T>
inline T rmvFlag(T property, T rmv)
{
	return property & ~rmv;
}

#pragma endregion

/* Function declarations
*/

VkPresentModeKHR chooseSwapPresentMode(VkPhysicalDevice &device, VkSurfaceKHR &surface, VkPresentModeKHR *prefered_modes, size_t num_prefered);

/*	Queue selection */
int anyQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag);
int matchQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag);
int pickQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag);

/* Device selection */
namespace vk
{
	/*	Determines if a physical device is suitable for the system. Return rank of the device, ranks greater then 0 will be available for selection. */
	typedef int(*isDeviceSuitable)(VkPhysicalDevice &device, VkPhysicalDeviceProperties &prop, VkPhysicalDeviceFeatures &feat, VkQueueFamilyProperties *queue_family_prop, size_t len_family_prop);

	/* Function selecting any dedicated device making a preference for discrete over integrated devices.
	*/
	int specifyAnyDedicatedDevice(VkPhysicalDevice &device, VkPhysicalDeviceProperties &prop, VkPhysicalDeviceFeatures &feat, VkQueueFamilyProperties *queue_family_prop, size_t len_family_prop);
}
int choosePhysicalDevice(VkInstance &instance, VkSurfaceKHR &surface, vk::isDeviceSuitable deviceSpec, VkQueueFlags queueSupportReq, VkPhysicalDevice &result);

std::vector<char*> checkValidationLayerSupport(char** validationLayers, size_t num_layer);


/* Memory */

VkBuffer createBuffer(VkDevice device, size_t byte_size, VkBufferUsageFlags usage, uint32_t queueCount = 0, uint32_t *queueFamilyIndices = nullptr);
VkImage createTexture2D(VkDevice device, uint32_t width, uint32_t height, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM, VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL);
VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);

VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer buffer, VkMemoryPropertyFlags properties, bool bindToBuffer = false);
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkImage image, VkMemoryPropertyFlags properties, bool bindToImage = false);
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryRequirements requirements, VkMemoryPropertyFlags properties);

/* Commands */

VkCommandBuffer beginSingleCommand(VkDevice device, VkCommandPool commandPool);
void endSingleCommand_Wait(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuf);


#ifdef VULKAN_DEVICE_IMPLEMENTATION


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

/* Find any queue family supporting the specific queue preferences.
*/
int anyQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag)
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
/* Find a queue family that exactly matches one of the preferences.
*/
int matchQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag)
{
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;		// Holds queue properties of corresponding physical device in physicalDevices
	ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, queueFamilyProperties, device);

	//Find queue matching the queue flags:
	for (int f = 0; f < num_flag; f++)
	{
		VkQueueFlags flag = pref_queueFlag[f];
		for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
		{
			if (matchFlag(queueFamilyProperties[i].queueFlags, flag))
			{
				// Match with other properties ..?
				return i;
			}
		}
	}
	// No matching queue found
	return -1;
}

/* Find a queue family that exactly matches one of the preferences, if nothing found the any family that supports the preference is selected.
*/
int pickQueueFamily(VkPhysicalDevice &device, VkQueueFlags* pref_queueFlag, int num_flag)
{
	int family = matchQueueFamily(device, pref_queueFlag, num_flag);
	if (family > -1)
		return family;
	return anyQueueFamily(device, pref_queueFlag, num_flag);
}

/* Check if there are a combination of queue families that supports all requested features. 
 * (does not guarantee support combinations). Returns 0 on success.
*/
int noQueueFamilySupport(VkQueueFlags feature, VkQueueFamilyProperties *family_list, size_t list_len)
{
	for (uint32_t i = 0; i < list_len; ++i)
		feature = rmvFlag(feature, family_list[i].queueFlags);
	return feature;
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
instance		<<	Vulkan instance
deviceSpec		<<	Ranking function of available physical devices used to determine a suitable device.
queueSupportReq	<<	Flag verifying that (to be) ranked devices supports a set of queue families.  
result			>>	Selected device
return			>>	Positive: index of the related device, Negative: Error code
*/
int choosePhysicalDevice(VkInstance &instance, VkSurfaceKHR &surface, vk::isDeviceSuitable deviceSpec, VkQueueFlags queueSupportReq, VkPhysicalDevice &result)
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

		ALLOC_QUERY(vkGetPhysicalDeviceQueueFamilyProperties, familyProperty, physicalDevices[i]);		// Holds queue properties of corresponding physical device in physicalDevices

		// Ensure the device can support specific queue families (and thus related operations)
		if (noQueueFamilySupport(queueSupportReq, familyProperty.data(), familyProperty.size()))
			continue;

		// Ensure the device can present on the specific surface (it is physically connected to the screen?).
		VkBool32 presentSupport = false;
		VkResult err = vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], i, surface, &presentSupport);
		assert(err == VK_SUCCESS);
		if(!presentSupport)
			continue;

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


/* Find validation layers that are supported.
validationLayers	<<	Set of validation layers requested.
num_layer			<<	Number of layers in the set.
*/
std::vector<char*> checkValidationLayerSupport(char** validationLayers, size_t num_layer) {
	std::vector<VkLayerProperties> availableLayers;
	ALLOC_QUERY_NOPARAM(vkEnumerateInstanceLayerProperties, availableLayers);

	std::vector<char*> available;
	available.reserve(num_layer);
	for (size_t i = 0; i < num_layer; i++) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(validationLayers[i], layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (layerFound)
			available.push_back(validationLayers[i]);
	}

	return available;
}

#pragma region Memory

/* Create a vulkan buffer of specific byte size and type.
byte_size		<<	Byte size of the buffer.
return			>>	The vertex buffer handle.
*/
VkBuffer createBuffer(VkDevice device, size_t byte_size, VkBufferUsageFlags usage, uint32_t queueCount, uint32_t *queueFamilyIndices)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = byte_size;
	bufferInfo.flags = 0;
	bufferInfo.usage = usage;
	if (queueCount <= 1)
		// Indicates it'll only be accessed by a single queue at a time (presumably only use this option if it will only be used by one queue)
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	else
		// Indicate multiple queues will concurrently use the buffer.
		bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
	// Since exclusive the queue params are redundant:
	bufferInfo.queueFamilyIndexCount = queueCount;
	bufferInfo.pQueueFamilyIndices = queueFamilyIndices;

	VkBuffer buffer;
	if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}
	return buffer;
}

/* Create a 2D texture image of specific size and format.
*/
VkImage createTexture2D(VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.format = format;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;

	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	// This image is used for sampling!...
	imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0; // Optional
	imageInfo.queueFamilyIndexCount = 0;
	imageInfo.pQueueFamilyIndices = nullptr;

	VkImage texture;
	VkResult result = vkCreateImage(device, &imageInfo, nullptr, &texture);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}
	return texture;
}

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageViewType viewType) {
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = viewType;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &imageView);
	if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void checkValidImageFormats(VkPhysicalDevice device)
{
	const int NUM_FORMAT = 2;
	VkFormat FORMATS[NUM_FORMAT] =
	{
		VK_FORMAT_R8G8B8_UNORM,
		VK_FORMAT_R8G8B8_UINT
	};
	char* NAMES[NUM_FORMAT] =
	{
		"VK_FORMAT_R8G8B8_UNORM",
		"VK_FORMAT_R8G8B8_UINT"
	};
	VkFormatProperties prop;
	for (int i = 0; i < NUM_FORMAT; i++)
	{
		vkGetPhysicalDeviceFormatProperties(device, FORMATS[i], &prop);
		if (prop.optimalTilingFeatures == 0)
			std::cout << NAMES[i] << " not supported\n";
		else
			std::cout << NAMES[i] << " supported\n";
	}
}

/* Find a memory type on the device mathcing the specification
*/
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {

	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	// Iterate the different types matching the filter mask and find one that matches the properties:
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && matchFlag(memProperties.memoryTypes[i].propertyFlags, properties)) {
			return i;
		}
	}
	throw std::runtime_error("failed to find suitable memory type!");
}
/* Allocate physical memory on the device of specific parameters.
device			<< Handle to the device.
physicalDevice	<< Handle to the physical device.
buffer			<< Related buffer.
requirements	<< The memory requirements.
properties		<< The properties the memory should have (dependent on buffer and how it's used).
*/
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkMemoryRequirements requirements, VkMemoryPropertyFlags properties)
{
	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = requirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDevice, requirements.memoryTypeBits, properties);

	//Allocate:
	// *Note that number allocations is limited, hence a custom allocator is required to allocate chunks for multiple buffers. 
	// *One possible solution is to use VulkanMemoryAllocator library.
	VkDeviceMemory mem;
	if (vkAllocateMemory(device, &allocInfo, nullptr, &mem) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate vertex buffer memory!");
	}
	return mem;
}
/* Allocate physical memory on the device for a buffer object.
device			<< Handle to the device.
physicalDevice	<< Handle to the physical device.
buffer			<< Buffer specifying the memory req.
properties		<< The properties the memory should have (dependent on buffer and how it's used).
bindToBuffer	<< If the memory should be bound to the buffer
*/
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkBuffer buffer, VkMemoryPropertyFlags properties, bool bindToBuffer)
{
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	//Allocate:
	// *Note that the number of allocations are limited!
	VkDeviceMemory mem = allocPhysicalMemory(device, physicalDevice, memRequirements, properties);
	// Bind to buffer.
	if(bindToBuffer)
		vkBindBufferMemory(device, buffer, mem, 0);
	return mem;
}
/* Allocate physical memory on the device for a buffer object.
device			<< Handle to the device.
physicalDevice	<< Handle to the physical device.
image			<< Image specifying the memory req.
properties		<< The properties the memory should have (dependent on buffer and how it's used).
bindToImage		<< If the memory should be bound to the image parameter.
*/
VkDeviceMemory allocPhysicalMemory(VkDevice device, VkPhysicalDevice physicalDevice, VkImage image, VkMemoryPropertyFlags properties, bool bindToImage)
{
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	//Allocate:
	// *Note that the number of allocations are limited!
	VkDeviceMemory mem = allocPhysicalMemory(device, physicalDevice, memRequirements, properties);
	// Bind to buffer.
	if (bindToImage)
		vkBindImageMemory(device, image, mem, 0);
	return mem;
}


#pragma endregion



#pragma region Command
/* Create a command of single time use taking one command.
commandPool << Pool to allocate command buffer from.
*/
VkCommandBuffer beginSingleCommand(VkDevice device, VkCommandPool commandPool)
{
	// Create command buffer
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = nullptr;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	VkResult result = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to create command buffer for staging.");

	// Begin recording into command buffer
	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.pNext = nullptr;
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	commandBufferBeginInfo.pInheritanceInfo = nullptr;

	result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
	if (result != VK_SUCCESS)
		throw std::runtime_error("Failed to begin command buffer.");
	return commandBuffer;
}

/* Submit and clean-up a single time use command buffer.
*/
void endSingleCommand_Wait(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkCommandBuffer commandBuf)
{
	// End command recording
	vkEndCommandBuffer(commandBuf);

	// Submit command buffer to queue
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = nullptr;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = nullptr;
	submitInfo.pWaitDstStageMask = nullptr;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuf;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = nullptr;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);		// Wait until the copy is complete

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuf);
}


#pragma endregion

#endif
