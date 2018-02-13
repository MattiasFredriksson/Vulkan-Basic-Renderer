#pragma once
#pragma once
#include "Technique.h"
#include "vulkan\vulkan.h"
#include "Vulkan\VulkanRenderer.h"
#include <map>

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
	void updateDescriptors(VkDescriptorBufferInfo* translationBufferDescriptorInfo, VkDescriptorImageInfo* textureDescriptorInfo);	// Updates the description set with the specified translation buffer and texture
private:
	void createRenderPass();
	void createDescriptorSetLayout();
	void createPipeline();
	void createShaders();
	void createDescriptorParams();

	// Assigns values to members of the VkWriteDescriptorSet struct. For a buffer descriptor, imageInfo must be nullptr, and vice versa
	void setupWriteDescriptorSet(VkWriteDescriptorSet* descriptorSet, unsigned bindingSlot, VkDescriptorType type, VkDescriptorImageInfo* imageInfo, VkDescriptorBufferInfo* bufferInfo);	
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

	VkDescriptorPool descriptorPool;
	uint32_t uniformBufferCount = 0;
	uint32_t combinedImageSamplerCount = 0;
	uint32_t requiredDescriptorTypes = 0;

	VkDescriptorSet descriptorSet;
};

