#pragma once
#pragma once
#include "Technique.h"
#include "vulkan\vulkan.h"
#include "Vulkan\VulkanRenderer.h"

class Renderer;

class TechniqueVulkan : public Technique
{
public:
	TechniqueVulkan(Material* m, RenderState* r, VulkanRenderer* renderer);
	virtual ~TechniqueVulkan();
	Material* getMaterial() { return material; };
	RenderState* getRenderState() { return renderState; };
	virtual void enable(Renderer* renderer);

	void setRenderer(VulkanRenderer* renderer);
private:
	void createRenderPass();
	void createDescriptorSet();
	void createPipeline();
	void createShaders();
	std::string assembleShader(Material::ShaderType type);
	std::string assembleDefines(Material::ShaderType type);
	std::string runCompiler(Material::ShaderType type, std::string inputFileName);
	std::vector<char> loadSPIR_V(std::string fileName);

	VulkanRenderer* renderer;

	VkRenderPass renderPass;

	VkDescriptorSetLayout vertexDataSetLayout;

	VkPipeline pipeline;

	VkPipelineLayout pipelineLayout;

	VkShaderModule vertexShaderModule;
	VkShaderModule fragmentShaderModule;
};

