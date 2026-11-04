#ifndef HELMET_MODEL_H
#define HELMET_MODEL_H
#define MAX_VERTS 16000
#define MAX_FACES 16000
#define MAX_EDGES 32000
enum { COPPER, BRASS, STEEL, GLASS, BLACK, MATERIALS };
typedef struct { float x,y,z; } Vec;
typedef struct { Vec p,n; float u,v; int weld,mat; } Vertex;
typedef struct { int v[3],e[3],mat,visible; Vec normal; float plane; } Face;
typedef struct { int a,b,face[2]; } Edge;
typedef struct { Vec normal; int mat; } LightingGroup;
extern Vertex *vertices;
extern Face *faces;
extern Edge *edges;
extern LightingGroup *lighting_groups;
extern int *vertex_lighting_group;
extern int lighting_group_count,model_distant;
extern int vertex_count,face_count,edge_count;
Vec vec(float x,float y,float z);
Vec add(Vec a,Vec b);
Vec mul(Vec a,float s);
Vec cross(Vec a,Vec b);
float dot(Vec a,Vec b);
Vec unit(Vec a);
int build_helmet(void);
void model_detail(int detailed);
void model_distance(int distant);
#endif
