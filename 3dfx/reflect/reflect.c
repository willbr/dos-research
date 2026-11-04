#if !defined(PROJECTOR_DEMO) || !defined(SHADOW_DEMO) || !defined(REFLECTION_DEMO)
#error Use build.ps1 for consistent renderer and profiler configuration
#endif
#define LIGHTMAP_DEMO
#define FOG_DEMO
#define BLEND_DEMO
#define MATERIAL_ROOT "../textures/assets/"
#define DEMO_LOG "REFLECT.LOG"
#define DEMO_NAME "Winchester graveyard reflection"
#include "../textures/cathedral.c"
