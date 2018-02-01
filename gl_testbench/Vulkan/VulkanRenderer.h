#pragma once

#include "../Renderer.h"

#include <SDL.h>
#include <GL/glew.h>

#pragma comment(lib, "opengl32.lib")
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

	void setClearColor(float, float, float, float);
	void clearBuffer(unsigned int);
	void setRenderState(RenderState* ps);
	void submit(Mesh* mesh);
	void frame();

private:
};

