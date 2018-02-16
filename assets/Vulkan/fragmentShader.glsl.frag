#version 450
#define POSITION 0
#define NORMAL 1
#define TEXTCOORD 2
#define TRANSLATION 0
#define TRANSLATION_NAME TranslationBlock
#define DIFFUSE_TINT 1
#define DIFFUSE_TINT_NAME DiffuseColor
// inputs
#ifdef NORMAL
	layout( location = NORMAL ) in vec4 normal_in;
#endif

#ifdef TEXTCOORD
	layout (location = TEXTCOORD ) in vec2 uv_in;
#endif

layout(location = 0) out vec4 fragment_color;

layout(set=DIFFUSE_TINT,binding=0) uniform DIFFUSE_TINT_NAME
{
	vec4 diffuseTint;
};

// binding sets the TEXTURE_UNIT value!
#ifdef DIFFUSE_SLOT
layout(set=DIFFUSE_SLOT, binding=0) uniform sampler2D myTex;
#endif

void main () {
	#ifdef DIFFUSE_SLOT
    vec4 col = texture(myTex, uv_in);
	#else
	vec4 col = vec4(1.0,1.0,1.0, 1.0);
	#endif

	fragment_color = col * vec4(diffuseTint.rgb,1.0);

	return;
	
//	#ifdef NORMAL
//		fragment_color = vec4(1.0,1.0,normal_in.z, 1.0) * vec4(diffuseTint.rgb, 1.0);
//	#else
//		fragment_color = vec4(1.0,1.0,1.0,1.0) * vec4(diffuseTint.rgb, 1.0);
//	#endif
}
