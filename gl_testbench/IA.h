
#define VULKANENGINE
#if defined(VULKANENGINE)

#define POSITION 0
#define NORMAL 1
#define TEXTCOORD 2
#define INDEXBUFF 4

#define TRANSLATION 0
#define TRANSLATION_NAME "TranslationBlock"

#define DIFFUSE_TINT 1
#define DIFFUSE_TINT_NAME "DiffuseColor"

#define DIFFUSE_SLOT 2

#elif defined(GLENGINE)

#define POSITION 0
#define NORMAL 1
#define TEXTCOORD 2
#define INDEXBUFF 4

#define TRANSLATION 5
#define TRANSLATION_NAME "TranslationBlock"

#define DIFFUSE_TINT 6
#define DIFFUSE_TINT_NAME "DiffuseColor"

#define DIFFUSE_SLOT 0


#endif
