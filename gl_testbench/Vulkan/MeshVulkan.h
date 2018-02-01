#pragma once
#include <unordered_map>
#include <gl\glew.h>
#include "../Mesh.h"

class MeshVulkan :
	public Mesh
{
public:
	MeshVulkan();
	~MeshVulkan();
};
