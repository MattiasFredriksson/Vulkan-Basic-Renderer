#pragma once
#include "../Material.h"
#include <GL/glew.h>
#include <vector>
#include "ConstantBufferVulkan.h"

class VulkanRenderer;

#define DBOUTW( s )\
{\
std::wostringstream os_;\
os_ << s;\
OutputDebugStringW( os_.str().c_str() );\
}

#define DBOUT( s )\
{\
std::ostringstream os_;\
os_ << s;\
OutputDebugString( os_.str().c_str() );\
}

// use X = {Program or Shader}
#define INFO_OUT(S,X) { \
char buff[1024];\
memset(buff, 0, 1024);\
glGet##X##InfoLog(S, 1024, nullptr, buff);\
DBOUTW(buff);\
}

// use X = {Program or Shader}
#define COMPILE_LOG(S,X,OUT) { \
char buff[1024];\
memset(buff, 0, 1024);\
glGet##X##InfoLog(S, 1024, nullptr, buff);\
OUT=std::string(buff);\
}


class MaterialVulkan :
	public Material
{
	friend VulkanRenderer;

public:
	MaterialVulkan(const std::string& name);
	~MaterialVulkan();
	void setShader(const std::string& shaderFileName, ShaderType type);
	void removeShader(ShaderType type);

	void setDiffuse(Color c);

	int compileMaterial(std::string& errString);
	void addConstantBuffer(std::string name, unsigned int location);

	void updateConstantBuffer(const void* data, size_t size, unsigned int location);

	int enable();

	void disable();

private:

};

#pragma once
