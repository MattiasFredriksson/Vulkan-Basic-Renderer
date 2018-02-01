#pragma once

#include "../Renderer.h"

#include <SDL.h>
#include <GL/glew.h>

#pragma comment(lib, "vulkan-1.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"SDL2.lib")
#pragma comment(lib,"SDL2main.lib")

#include "vulkan\vulkan.h"

class VulkanRenderer : public Renderer
{
public:
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

private:
	VkInstance vulkanInstance;
	std::vector<VkPhysicalDevice> physicalDevices;									// Holds handles to the physical devices detected
	std::vector<VkPhysicalDeviceProperties> physicalDeviceProperties;				// Holds properties of corresponding physical device in physicalDevices
	std::vector<VkPhysicalDeviceFeatures> physicalDeviceFeatures;					// Holds features of corresponding physical device in physicalDevices
	std::vector<VkPhysicalDeviceMemoryProperties> physicalDeviceMemoryProperties;	// Holds memory properties of corresponding physical device in physicalDevices
	std::vector<std::vector<VkQueueFamilyProperties>> physicalDeviceQueueFamilyProperties;		// Holds queue properties of corresponding physical device in physicalDevices

	int chosenPhysicalDevice;
};

