// buffer inputs
#ifdef NORMAL
	layout(location=NORMAL) in vec4 normal_in;
	layout(location=NORMAL) out vec4 normal_out;
#endif

#ifdef TEXTCOORD
	layout(location=TEXTCOORD) in vec2 uv_in;
	layout(location=TEXTCOORD) out vec2 uv_out;
#endif
layout(location=POSITION) in vec4 position_in;

// uniform block
// layout(std140, binding = 20) uniform TransformBlock
// {
//  	vec4 tx;
// } transform;

layout(set=TRANSLATION, binding=0) uniform TRANSLATION_NAME
{
	vec4 translate;
};

layout(set=DIFFUSE_TINT, binding=0) uniform DIFFUSE_TINT_NAME
{
	vec4 diffuseTint;
};

void main() {

	#ifdef NORMAL
		normal_out = normal_in;
	#endif

	#ifdef TEXTCOORD
		uv_out = uv_in;
	#endif
	gl_Position = position_in + translate;
}
