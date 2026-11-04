#ifndef SHADOW_SCENE_H
#define SHADOW_SCENE_H
#define MAX_SHADOW_CASTERS 160
typedef struct {
    Vec point[16],low,high;
    unsigned char edge[24][2];
    int points,edges,kind;
} ShadowCaster;
extern ShadowCaster shadow_casters[MAX_SHADOW_CASTERS];
extern int shadow_caster_count;
#endif
