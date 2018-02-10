#pragma once
#include <vector>
#include "../RenderState.h"

class RenderStateVulkan : public RenderState
{
public:
	RenderStateVulkan();
	~RenderStateVulkan();
	void setWireFrame(bool);
	void set();

	void setGlobalWireFrame(bool* global);

	bool getWireframe();
private:
	bool wireframe;
	bool* globalWireframe;
};