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
	VkInstance instance;

	VkDevice device;

	bool globalWireframeMode = false;

	SDL_Window* window;

	VkSurfaceKHR windowSurface;
	VkSwapchainKHR swapchain;

	std::vector<VkImage> swapchainImages;			// Array of images in the swapchain, use vkAquireNextImageKHR(...) to aquire image for drawing to
	std::vector<VkImageView> swapchainImageViews;	// Image views for the swap chain images

	glm::vec4 clearColor;
};