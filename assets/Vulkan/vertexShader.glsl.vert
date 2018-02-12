#version 450
#define POSITION 0
#define NORMAL 1
#define TEXTCOORD 2
#define TRANSLATION 5
#define TRANSLATION_NAME TranslationBlock
#define DIFFUSE_TINT 6
#define DIFFUSE_TINT_NAME DiffuseColor
// buffer inputs
#ifdef NORMAL
	layout(binding=NORMAL) uniform Normal 
	{
		vec4 normal_in[];
	} nor;
	layout(location=NORMAL) out vec4 normal_out;
#endif

#ifdef TEXTCOORD
	layout(binding=TEXTCOORD) uniform TextureCoordinate
	{
		vec2 uv_in[];
	} text;
	layout(location=TEXTCOORD) out vec2 uv_out;
#endif
layout(binding=POSITION) uniform Position
{
	vec4 position_in[];
} pos;

// uniform block
// layout(std140, binding = 20) uniform TransformBlock
// {
//  	vec4 tx;
// } transform;

layout(binding=TRANSLATION) uniform TRANSLATION_NAME
{
	vec4 translate;
};

layout(binding=DIFFUSE_TINT) uniform DIFFUSE_TINT_NAME
{
	vec4 diffuseTint;
};

void main() {

	#ifdef NORMAL
		normal_out = normal_in[gl_VertexIndex];
	#endif

	#ifdef TEXTCOORD
		uv_out = uv_in[gl_VertexIndex];
	#endif

	gl_Position = position_in[gl_VertexIndex] + translate;
}
