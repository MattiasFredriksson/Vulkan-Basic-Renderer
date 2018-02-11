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

	VulkanRenderer* renderer;

	VkRenderPass renderPass;

	VkDescriptorSetLayout vertexDataSetLayout;

	VkPipeline pipeline;

	VkPipelineLayout pipelineLayout;
};

