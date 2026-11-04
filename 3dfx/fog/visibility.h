#ifndef FOG_SCENE_H
#define FOG_SCENE_H
#define FOG_MAX_GROUPS 1536
typedef struct { int first,last,lod;Vec center,lod_center;float radius,minz,maxz,lod_distance; } FogGroup;
extern FogGroup fog_groups[FOG_MAX_GROUPS];
extern int fog_group_count,fog_nave_faces,fog_facade_first,fog_facade_last;
#endif
