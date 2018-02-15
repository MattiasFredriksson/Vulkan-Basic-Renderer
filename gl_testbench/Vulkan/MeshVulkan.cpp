#include "MeshVulkan.h"
#include "../TechniqueVulkan.h"
MeshVulkan::MeshVulkan()
{
}

MeshVulkan::~MeshVulkan()
{
}
void MeshVulkan::updateDescriptors()
{
	VkWriteDescriptorSet writes[MAX_DESCRIPTOR_TYPES];
	int num_updated = 0;

	if (textureDescriptorInfo)
	{
		writeDescriptorStruct_IMG_COMBINED(writes[num_updated], dstSet, DIFFUSE_SLOT, 0, 1, textureDescriptorInfo);
		++num_updated;
	}

	if (translationBufferDescriptorInfo)
	{
		writeDescriptorStruct_UNI_BUFFER(writes[num_updated], dstSet, DIFFUSE_SLOT, 0, 1, translationBufferDescriptorInfo);
		++num_updated;
	}


	for (auto cb : constantBuffers)
	{
		ConstantBufferVulkan *cbv = (ConstantBufferVulkan*)cb.second;
		writeDescriptorStruct_UNI_BUFFER(writes[num_updated], dstSet, cb.first, 0, 1, cbv->getDescriptorBufferInfo());
		++num_updated;
	}

	// Write the info into the descriptor set
	vkUpdateDescriptorSets(_renderHandle->getDevice(), num_updated, writes, 0, nullptr);
}