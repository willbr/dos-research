#if !defined(PROJECTOR_DEMO) || !defined(SHADOW_DEMO) || !defined(DETAIL_DEMO) || !defined(FOLIAGE_DEMO) || !defined(REFLECTION_DEMO) || !defined(WATER_DEMO)
#error Use build.ps1 for consistent renderer and profiler configuration
#endif
#define LIGHTMAP_DEMO
#define FOG_DEMO
#define BLEND_DEMO
#define MATERIAL_ROOT "../textures/assets/"
#define DEMO_LOG "RAIN.LOG"
#define DEMO_NAME "Winchester wind and rain"
#include "../textures/cathedral.c"
