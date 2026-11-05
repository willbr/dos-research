#if !defined(PROJECTOR_DEMO) || !defined(SHADOW_DEMO) || !defined(DETAIL_DEMO) || !defined(FOLIAGE_DEMO)
#error Use build.ps1 for consistent renderer and profiler configuration
#endif
#define LIGHTMAP_DEMO
#define FOG_DEMO
#define BLEND_DEMO
#define MATERIAL_ROOT "../textures/assets/"
#define DEMO_LOG "FOLIAGE.LOG"
#define DEMO_NAME "Winchester wind and alpha-tested foliage"
#include "../textures/cathedral.c"
