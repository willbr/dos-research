#ifndef CATHEDRAL_MODEL_H
#define CATHEDRAL_MODEL_H
#define MAX_VERTS 18000
#define MAX_FACES 10000
enum { STONE,ROOF,GRASS,PAVING,WINDOW,DOOR,TRIM,SHADOW,CHECKER,MATERIALS };
typedef struct { float x,y,z; } Vec;
typedef struct { Vec p;float u,v,r,g,b; } Vertex;
typedef struct { int v[3],mat;Vec normal;float plane,r,g,b; } Face;
extern Vertex vertices[MAX_VERTS];
extern Face faces[MAX_FACES];
extern int vertex_count,face_count;
Vec vec(float x,float y,float z);
Vec add(Vec a,Vec b);
Vec sub(Vec a,Vec b);
Vec mul(Vec a,float s);
float dot(Vec a,Vec b);
Vec cross(Vec a,Vec b);
Vec unit(Vec a);
int build_cathedral(void);
#endif
