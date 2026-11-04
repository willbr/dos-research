#if !defined(PROJECTOR_DEMO) || !defined(SHADOW_DEMO)
#error Use build.ps1 for consistent renderer and profiler configuration
#endif
#define LIGHTMAP_DEMO
#define FOG_DEMO
#define BLEND_DEMO
#define MATERIAL_ROOT "../textures/assets/"
#define DEMO_LOG "SHADOWS.LOG"
#define DEMO_NAME "Winchester lantern ground shadows"
#include "../textures/cathedral.c"
