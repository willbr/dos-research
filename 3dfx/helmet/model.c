/* Procedural study of the user's diving-helmet photograph. */
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "model.h"
#define PI 3.14159265358979323846
#define MAX_SEG 32
static int segment_count=16,full_detail,want_distant;
#define SEG segment_count
void model_detail(int detailed) { full_detail=detailed;segment_count=detailed?32:16; }
void model_distance(int distant) { want_distant=distant; }
#define HASH_SIZE 65536
static Vertex build_vertices[MAX_VERTS];
static Face build_faces[MAX_FACES];
static Edge build_edges[MAX_EDGES];
static LightingGroup build_groups[MAX_VERTS];
static int build_group_index[MAX_VERTS];
Vertex *vertices=build_vertices;
Face *faces=build_faces;
Edge *edges=build_edges;
LightingGroup *lighting_groups=build_groups;
int *vertex_lighting_group=build_group_index;
int lighting_group_count,model_distant;
typedef struct {
    Vertex *v;Face *f;Edge *e;LightingGroup *groups;int *index;
    int nv,nf,ne,ng;
} CachedMesh;
static CachedMesh cached[3]; /* Fast near, Fast distant, Detail. */
static void activate(CachedMesh *m,int distant)
{
    vertices=m->v;faces=m->f;edges=m->e;lighting_groups=m->groups;vertex_lighting_group=m->index;
    vertex_count=m->nv;face_count=m->nf;edge_count=m->ne;lighting_group_count=m->ng;model_distant=distant;
}
int vertex_count,face_count,edge_count;
static int weld_hash[HASH_SIZE],edge_hash[HASH_SIZE];
static int wx[MAX_VERTS],wy[MAX_VERTS],wz[MAX_VERTS],weld_count;
static int failed;
Vec vec(float x,float y,float z) { Vec p; p.x=x;p.y=y;p.z=z;return p; }
Vec add(Vec a,Vec b) { return vec(a.x+b.x,a.y+b.y,a.z+b.z); }
Vec mul(Vec a,float s) { return vec(a.x*s,a.y*s,a.z*s); }
Vec cross(Vec a,Vec b) { return vec(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x); }
float dot(Vec a,Vec b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec unit(Vec a) { float l=(float)sqrt(dot(a,a)); return l>0.000001f?mul(a,1/l):vec(0,1,0); }
static unsigned hash3(int x,int y,int z)
{ return ((unsigned)x*73856093u^(unsigned)y*19349663u^(unsigned)z*83492791u)&(HASH_SIZE-1); }
static int weld(Vec p)
{
    int x=(int)floor(p.x*10000+.5),y=(int)floor(p.y*10000+.5),z=(int)floor(p.z*10000+.5),id;
    unsigned h=hash3(x,y,z);
    while(weld_hash[h]) {
        id=weld_hash[h]-1;
        if(wx[id]==x && wy[id]==y && wz[id]==z) return id;
        h=(h+1)&(HASH_SIZE-1);
    }
    id=weld_count++; wx[id]=x;wy[id]=y;wz[id]=z;weld_hash[h]=id+1;return id;
}
static int vertex(Vec p,Vec n,float u,float v,int mat)
{
    Vertex *a;
    if(vertex_count==MAX_VERTS) { failed=1;return 0; }
    a=&vertices[vertex_count];a->p=p;a->n=unit(n);a->u=u;a->v=v;a->mat=mat;a->weld=weld(p);
    return vertex_count++;
}
static int edge(int a,int b,int face)
{
    int id,t;
    unsigned h;
    if(a>b) { t=a;a=b;b=t; }
    h=hash3(a,b,0);
    while(edge_hash[h]) {
        id=edge_hash[h]-1;
        if(edges[id].a==a && edges[id].b==b) {
            if(edges[id].face[1]!=-1) failed=1; /* nonmanifold topology */
            else edges[id].face[1]=face;
            return id;
        }
        h=(h+1)&(HASH_SIZE-1);
    }
    if(edge_count==MAX_EDGES) { failed=1;return 0; }
    id=edge_count++;edges[id].a=a;edges[id].b=b;edges[id].face[0]=face;edges[id].face[1]=-1;
    edge_hash[h]=id+1;return id;
}
static void triangle(int a,int b,int c,int mat)
{
    Face *f;
    Vec n=cross(add(vertices[b].p,mul(vertices[a].p,-1)),add(vertices[c].p,mul(vertices[a].p,-1)));
    int tmp,i;
    if(dot(n,n)<0.00000001f) return;
    if(dot(n,add(add(vertices[a].n,vertices[b].n),vertices[c].n))<0) { tmp=b;b=c;c=tmp;n=mul(n,-1); }
    if(face_count==MAX_FACES) { failed=1;return; }
    f=&faces[face_count];f->v[0]=a;f->v[1]=b;f->v[2]=c;f->mat=mat;f->normal=unit(n);
    f->plane=dot(f->normal,vertices[a].p);
    for(i=0;i<3;++i) f->e[i]=edge(vertices[f->v[i]].weld,vertices[f->v[(i+1)%3]].weld,face_count);
    ++face_count;
}
static void quad(int a,int b,int c,int d,int mat)
{ triangle(a,b,c,mat);triangle(a,c,d,mat); }
static void basis(Vec axis,Vec *u,Vec *v)
{
    *u=unit(cross(fabs(axis.y)<.9?vec(0,1,0):vec(0,0,1),axis));
    *v=unit(cross(axis,*u));
}
static Vec radial(Vec u,Vec v,double angle)
{ return add(mul(u,(float)cos(angle)),mul(v,(float)sin(angle))); }

/* Elliptical torus oriented along axis, for window rims and collar grooves. */
static void torus(Vec center,Vec axis,float ru,float rv,float tube,int mat,int segments,int sides)
{
    int grid[65][9],i,j;
    Vec u,v;
    if(!full_detail) { if(segments>16)segments=16;if(sides>4)sides=4; }
    if(!full_detail && want_distant && ru<.25f && segments>8)segments=8;
    axis=unit(axis);basis(axis,&u,&v);
    for(i=0;i<=segments;++i) for(j=0;j<=sides;++j) {
        double a=2*PI*i/segments,b=2*PI*j/sides;
        Vec outward=radial(u,v,a),n=add(mul(outward,(float)cos(b)),mul(axis,(float)sin(b)));
        Vec p=add(center,add(mul(u,ru*(float)cos(a)),mul(v,rv*(float)sin(a))));
        grid[i][j]=vertex(add(p,mul(n,tube)),n,(float)i/segments*255,(float)j/sides*255,mat);
    }
    for(i=0;i<segments;++i) for(j=0;j<sides;++j)
        quad(grid[i][j],grid[i+1][j],grid[i+1][j+1],grid[i][j+1],mat);
}
static void cylinder(Vec a,Vec b,float radius,int mat,int segments)
{
    int side[2][33],cap[2][33],centers[2],i,j;
    Vec axis=unit(add(b,mul(a,-1))),u,v;
    if(!full_detail && segments>6) segments=6;
    if(!full_detail && want_distant && radius<=.085f && segments>4)segments=4;
    basis(axis,&u,&v);
    for(j=0;j<2;++j) {
        Vec center=j?b:a,n=mul(axis,j?1.0f:-1.0f);
        centers[j]=vertex(center,n,128,128,mat);
        for(i=0;i<=segments;++i) {
            Vec r=radial(u,v,2*PI*i/segments),p=add(center,mul(r,radius));
            side[j][i]=vertex(p,r,(float)i/segments*255,(float)j*255,mat);
            cap[j][i]=vertex(p,n,128+r.x*100,128+r.y*100,mat);
        }
    }
    for(i=0;i<segments;++i) {
        quad(side[0][i],side[1][i],side[1][i+1],side[0][i+1],mat);
        if(full_detail) triangle(centers[0],cap[0][i],cap[0][i+1],mat);
        triangle(centers[1],cap[1][i],cap[1][i+1],mat);
    }
}
static void glass(Vec center,Vec axis,float ru,float rv)
{
    int ring[MAX_SEG+1],c,i;
    Vec u,v;
    axis=unit(axis);basis(axis,&u,&v);
    c=vertex(add(center,mul(axis,.025f)),axis,128,128,GLASS);
    for(i=0;i<=SEG;++i) {
        double a=2*PI*i/SEG;
        Vec r=radial(u,v,a),p=add(center,add(mul(u,ru*(float)cos(a)),mul(v,rv*(float)sin(a))));
        ring[i]=vertex(p,add(axis,mul(r,.3f)),0,0,GLASS);
    }
    for(i=0;i<SEG;++i) triangle(c,ring[i],ring[i+1],GLASS);
}
static void shoulder(void)
{
    int grid[4][MAX_SEG+1],rim[MAX_SEG+1][7],i,j,sides=full_detail?6:4;
    for(j=0;j<4;++j) for(i=0;i<=SEG;++i) {
        float t=j/3.0f;
        double a=2*PI*i/SEG;
        Vec p=vec((.8f+.82f*t)*(float)sin(a),-.30f-.55f*t-.3f*t*(float)cos(a)+.08f*t*(float)cos(2*a),(.7f+.56f*t)*(float)cos(a));
        grid[j][i]=vertex(p,vec(.8f*(float)sin(a),1,1.1f*(float)cos(a)),(float)i/SEG*255,t*255,COPPER);
    }
    for(j=0;j<3;++j) for(i=0;i<SEG;++i)
        quad(grid[j][i],grid[j+1][i],grid[j+1][i+1],grid[j][i+1],COPPER);
    for(i=0;i<=SEG;++i) for(j=0;j<=sides;++j) {
        double a=2*PI*i/SEG,b=2*PI*j/sides;
        Vec n=vec((float)(sin(a)*cos(b)),(float)sin(b),(float)(cos(a)*cos(b)));
        Vec p=vertices[grid[3][i]].p;
        rim[i][j]=vertex(add(p,mul(n,.055f)),n,(float)i/SEG*255,(float)j/sides*255,BRASS);
    }
    for(i=0;i<SEG;++i) for(j=0;j<sides;++j)
        quad(rim[i][j],rim[i+1][j],rim[i+1][j+1],rim[i][j+1],BRASS);
}
int build_helmet(void)
{
    static const float y[]={-.12f,.08f,.4f,.8f,1.2f,1.53f,1.82f,2.03f,2.13f,2.16f};
    static const float r[]={.96f,1.02f,1.10f,1.12f,1.06f,.92f,.70f,.42f,.16f,0};
    int shell[10][MAX_SEG+1],i,j,k;
    CachedMesh *m=&cached[full_detail?2:want_distant?1:0];
    if(m->v) { activate(m,!full_detail && want_distant);return 1; }
    vertices=build_vertices;faces=build_faces;edges=build_edges;
    lighting_groups=build_groups;vertex_lighting_group=build_group_index;
    vertex_count=face_count=edge_count=weld_count=failed=0;
    memset(weld_hash,0,sizeof(weld_hash));memset(edge_hash,0,sizeof(edge_hash));
    for(j=0;j<10;++j) for(i=0;i<=SEG;++i) {
        double a=2*PI*i/SEG;
        int lo=j?j-1:j,hi=j<9?j+1:j;
        float slope=-(r[hi]-r[lo])/(y[hi]-y[lo]);
        Vec p=vec(r[j]*(float)sin(a),y[j],r[j]*.92f*(float)cos(a));
        shell[j][i]=vertex(p,vec((float)sin(a),slope,(float)cos(a)/.92f),(float)i/SEG*255,(float)j/9*255,COPPER);
    }
    for(j=0;j<9;++j) for(i=0;i<SEG;++i)
        quad(shell[j][i],shell[j][i+1],shell[j+1][i+1],shell[j+1][i],COPPER);
    shoulder();
    /* The neck skirt intersects the shoulder surface so it is not a floating dome. */
    cylinder(vec(0,-.60f,0),vec(0,-.02f,0),.975f,COPPER,SEG);
    for(i=0;i<3;++i)
        torus(vec(0,-.27f+i*.105f,0),vec(0,1,0),1.015f,.96f,.055f,i==1?STEEL:BRASS,SEG,6);
    /* Long oval front window, raised brass flange, dark glass, protective bars. */
    torus(vec(0,.62f,1.015f),vec(0,0,1),.535f,.755f,.09f,STEEL,SEG,6);
    torus(vec(0,.62f,1.102f),vec(0,0,1),.49f,.70f,.065f,BRASS,SEG,6);
    glass(vec(0,.62f,1.115f),vec(0,0,1),.45f,.65f);
    cylinder(vec(0,-.025f,1.17f),vec(0,1.265f,1.17f),.025f,BRASS,8);
    for(i=-1;i<=1;i+=2) {
        float yy=.62f+i*.225f,half=.45f*(float)sqrt(1-(.225f/.65f)*(.225f/.65f));
        cylinder(vec(-half,yy,1.175f),vec(half,yy,1.175f),.025f,BRASS,8);
    }
    /* Side lights, thick rims, hinge pins and locking knobs. */
    for(k=-1;k<=1;k+=2) {
        Vec axis=vec((float)k,0,0),center=vec(k*1.015f,.65f,.03f);
        torus(center,axis,.63f,.63f,.09f,STEEL,SEG,6);
        torus(add(center,mul(axis,.10f)),axis,.57f,.57f,.085f,BRASS,SEG,6);
        glass(add(center,mul(axis,.15f)),axis,.50f,.50f);
        for(i=-1;i<=1;i+=2) {
            Vec a=vec(k*1.15f,.65f+i*.48f,.37f);
            cylinder(a,add(a,mul(axis,.30f)),.045f,BRASS,10);
            cylinder(add(a,mul(axis,.28f)),add(a,mul(axis,.33f)),.065f,COPPER,10);
        }
    }
    /* Hexagonal hose fitting and smaller upper air fitting. */
    cylinder(vec(-.75f,.25f,.8f),vec(-.99f,.25f,1.05f),.15f,BRASS,6);
    cylinder(vec(-.99f,.25f,1.05f),vec(-1.06f,.25f,1.13f),.11f,BLACK,12);
    cylinder(vec(-.72f,1.19f,.65f),vec(-.88f,1.23f,.82f),.12f,BRASS,12);
    /* Shoulder-plate studs and a small front maker's plate, without invented lettering. */
    for(k=-1;k<=1;k+=2) for(i=0;i<2;++i) {
        Vec p=vec(k*(i?1.13f:.62f),i?-.72f:-.68f,i?.74f:1.07f);
        Vec axis=unit(vec(k*.35f,.7f,.8f));
        cylinder(p,add(p,mul(axis,.15f)),.07f,BRASS,8);
        cylinder(add(p,mul(axis,.12f)),add(p,mul(axis,.18f)),.085f,COPPER,8);
    }
    torus(vec(.35f,-.68f,1.045f),unit(vec(0,.55f,1)),.22f,.14f,.018f,BRASS,24,4);
    /* Builders retain separate cap/UV-seam vertices. Drop any that no face
       references (notably buried start caps in FAST) before per-frame work. */
    {
        static unsigned char referenced[MAX_VERTS];
        static int remap[MAX_VERTS];
        int next=0;
        memset(referenced,0,sizeof(referenced));
        for(i=0;i<face_count;++i) for(j=0;j<3;++j) referenced[faces[i].v[j]]=1;
        for(i=0;i<vertex_count;++i) if(referenced[i]) { remap[i]=next;vertices[next++]=vertices[i]; }
        for(i=0;i<face_count;++i) for(j=0;j<3;++j) faces[i].v[j]=remap[faces[i].v[j]];
        vertex_count=next;
    }
    if(failed || vertex_count<=0 || face_count<=0)return 0;
    lighting_group_count=0;
    for(i=0;i<vertex_count;++i) {
        Vertex *v=&vertices[i];
        for(j=0;j<lighting_group_count;++j) {
            LightingGroup *g=&lighting_groups[j];
            if(g->mat==v->mat && g->normal.x==v->n.x && g->normal.y==v->n.y && g->normal.z==v->n.z)break;
        }
        if(j==lighting_group_count) { lighting_groups[j].normal=v->n;lighting_groups[j].mat=v->mat;++lighting_group_count; }
        vertex_lighting_group[i]=j;
    }
    m->v=malloc(vertex_count*sizeof(Vertex));m->f=malloc(face_count*sizeof(Face));m->e=malloc(edge_count*sizeof(Edge));
    m->groups=malloc(lighting_group_count*sizeof(LightingGroup));m->index=malloc(vertex_count*sizeof(int));
    if(!m->v || !m->f || !m->e || !m->groups || !m->index) {
        free(m->v);free(m->f);free(m->e);free(m->groups);free(m->index);memset(m,0,sizeof(*m));return 0;
    }
    m->nv=vertex_count;m->nf=face_count;m->ne=edge_count;m->ng=lighting_group_count;
    memcpy(m->v,vertices,m->nv*sizeof(Vertex));memcpy(m->f,faces,m->nf*sizeof(Face));memcpy(m->e,edges,m->ne*sizeof(Edge));
    memcpy(m->groups,lighting_groups,m->ng*sizeof(LightingGroup));memcpy(m->index,vertex_lighting_group,m->nv*sizeof(int));
    activate(m,!full_detail && want_distant);return 1;
}
