#pragma once

#include <GL/glew.h>

#include "../Texture2D.h"
#include "Sampler2DVulkan.h"


class Texture2DVulkan :
	public Texture2D
{
public:
	Texture2DVulkan();
	~Texture2DVulkan();

	int loadFromFile(std::string filename);
	void bind(unsigned int slot);
};

