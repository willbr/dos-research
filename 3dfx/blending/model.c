/* Winchester-inspired nave built for the blending experiment.
   Approximate scene units are metres; this is not a measured reconstruction. */
#include <math.h>
#include "../textures/model.h"
#include "scene.h"
GlassWindow glass_windows[24];int glass_count;
const Vec candle_positions[12]={
    {-2.15f,1.05f,-9},{2.15f,1.05f,-9},{-2.15f,1.05f,-5},{2.15f,1.05f,-5},
    {-2.15f,1.05f,-1},{2.15f,1.05f,-1},{-2.15f,1.05f,3},{2.15f,1.05f,3},
    {-2.15f,1.05f,7},{2.15f,1.05f,7},{-1.5f,1.05f,11},{1.5f,1.05f,11}
};
Vertex vertices[MAX_VERTS];Face faces[MAX_FACES];
int vertex_count,face_count;
static int failed;
Vec vec(float x,float y,float z) { Vec a;a.x=x;a.y=y;a.z=z;return a; }
Vec add(Vec a,Vec b) { return vec(a.x+b.x,a.y+b.y,a.z+b.z); }
Vec sub(Vec a,Vec b) { return vec(a.x-b.x,a.y-b.y,a.z-b.z); }
Vec mul(Vec a,float s) { return vec(a.x*s,a.y*s,a.z*s); }
float dot(Vec a,Vec b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec cross(Vec a,Vec b) { return vec(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x); }
Vec unit(Vec a) { float l=(float)sqrt(dot(a,a));return l>.00001f?mul(a,1/l):vec(0,1,0); }
static int vertex(Vec p,float u,float v)
{
    int i=vertex_count;if(i==MAX_VERTS){failed=1;return 0;}
    vertices[i].p=p;vertices[i].u=u;vertices[i].v=v;++vertex_count;return i;
}
static void triangle(int a,int b,int c,int mat,Vec normal)
{
    Face *f;int t;float light;
    Vec n=cross(sub(vertices[b].p,vertices[a].p),sub(vertices[c].p,vertices[a].p));
    if(dot(n,n)<.0000001f)return;
    if(dot(n,normal)<0){t=b;b=c;c=t;}
    if(face_count==MAX_FACES){failed=1;return;}
    f=&faces[face_count++];f->v[0]=a;f->v[1]=b;f->v[2]=c;f->mat=mat;f->normal=unit(normal);
    f->plane=dot(f->normal,vertices[a].p);
    light=.16f+.10f*dot(f->normal,unit(vec(-.5f,.9f,-.4f)));if(light<.09f)light=.09f;
    if(mat==WINDOW || mat==DOOR)light=.92f;
    if(mat==SHADOW)light=.43f;
    if(mat==TRIM)light*=1.12f;
    if(light>1)light=1;
    f->r=255*light;f->g=218*light;f->b=175*light;
    vertices[a].r=vertices[b].r=vertices[c].r=f->r;
    vertices[a].g=vertices[b].g=vertices[c].g=f->g;
    vertices[a].b=vertices[b].b=vertices[c].b=f->b;
}
static void quad_uv(Vec a,Vec b,Vec c,Vec d,int mat,Vec n,float u0,float v0,float u1,float v1)
{
    int aa=vertex(a,u0,v1),bb=vertex(b,u1,v1),cc=vertex(c,u1,v0),dd=vertex(d,u0,v0);
    triangle(aa,bb,cc,mat,n);triangle(aa,cc,dd,mat,n);
}
static void quad(Vec a,Vec b,Vec c,Vec d,int mat,Vec n)
{
    Vec p[4];int ids[4],i;p[0]=a;p[1]=b;p[2]=c;p[3]=d;
    for(i=0;i<4;++i) {
        float u,v;
        if(mat==ROOF) { u=(fabs(n.x)>fabs(n.z)?p[i].z:p[i].x)*100;v=(fabs(n.x)>fabs(n.z)?p[i].x:p[i].z)*160; }
        else if(fabs(n.y)>.7f){u=p[i].x*128;v=p[i].z*128;}
        else {u=(fabs(n.x)>.5f?p[i].z:p[i].x)*128;v=-p[i].y*128;}
        ids[i]=vertex(p[i],u,v);
    }
    triangle(ids[0],ids[1],ids[2],mat,n);triangle(ids[0],ids[2],ids[3],mat,n);
}
static void box(float x0,float y0,float z0,float x1,float y1,float z1,int mat)
{
    quad(vec(x0,y0,z0),vec(x1,y0,z0),vec(x1,y1,z0),vec(x0,y1,z0),mat,vec(0,0,-1));
    quad(vec(x0,y0,z1),vec(x1,y0,z1),vec(x1,y1,z1),vec(x0,y1,z1),mat,vec(0,0,1));
    quad(vec(x0,y0,z0),vec(x0,y0,z1),vec(x0,y1,z1),vec(x0,y1,z0),mat,vec(-1,0,0));
    quad(vec(x1,y0,z0),vec(x1,y0,z1),vec(x1,y1,z1),vec(x1,y1,z0),mat,vec(1,0,0));
    quad(vec(x0,y1,z0),vec(x1,y1,z0),vec(x1,y1,z1),vec(x0,y1,z1),mat,vec(0,1,0));
}
static void beam(Vec a,Vec b,float radius)
{
    Vec side=vec(0,0,radius),up=mul(unit(vec(-(b.y-a.y),b.x-a.x,0)),radius);
    Vec p[4],q[4];int i;
    p[0]=add(add(a,side),up);p[1]=add(sub(a,side),up);p[2]=sub(sub(a,side),up);p[3]=sub(add(a,side),up);
    for(i=0;i<4;++i)q[i]=add(p[i],sub(b,a));
    for(i=0;i<4;++i) {int j=(i+1)%4;Vec n=unit(sub(mul(add(p[i],p[j]),.5f),a));quad(p[i],p[j],q[j],q[i],TRIM,n);}
}

static void cylinder(float x,float z,float y0,float y1,float radius,int mat)
{
    int i;
    for(i=0;i<8;++i) {
        float a=i*(6.28318530718f/8),b=(i+1)*(6.28318530718f/8);
        Vec p=vec(x+radius*(float)cos(a),y0,z+radius*(float)sin(a));
        Vec q=vec(x+radius*(float)cos(b),y0,z+radius*(float)sin(b));
        quad(p,q,vec(q.x,y1,q.z),vec(p.x,y1,p.z),mat,vec((p.x+q.x)*.5f-x,0,(p.z+q.z)*.5f-z));
    }
}
static void glass_window(float x,float y,float z,float width,float height,int side)
{
    int segment;GlassWindow *g;Vec c=vec(x,y,z),u=side<2?vec(width*.5f,0,0):vec(0,0,width*.5f);
    if(glass_count==24){failed=1;return;}g=&glass_windows[glass_count++];
    g->p[0]=sub(c,u);g->p[1]=add(c,u);g->p[2]=add(g->p[1],vec(0,height,0));g->p[3]=add(g->p[0],vec(0,height,0));
    g->center=add(c,vec(0,height*.5f,0));
    g->normal=side==0?vec(0,0,-1):side==1?vec(0,0,1):side==2?vec(1,0,0):vec(-1,0,0);
    for(segment=0;segment<12;++segment) {
        float u0=segment/12.0f,u1=(segment+1)/12.0f;
        float d0=(float)fabs(u0-.5f)+.5f,d1=(float)fabs(u1-.5f)+.5f;
        float h0=height*(.54f+.46f*(float)sqrt(1-d0*d0)/.8660254f);
        float h1=height*(.54f+.46f*(float)sqrt(1-d1*d1)/.8660254f);
        Vec a=add(sub(c,u),mul(u,2*u0)),b=add(sub(c,u),mul(u,2*u1));
        quad(add(a,vec(0,h0,0)),add(b,vec(0,h1,0)),add(b,vec(0,height,0)),add(a,vec(0,height,0)),STONE,g->normal);
    }
}
static float vault_height(float x)
{ return 4.7f+3.1f*(float)pow(1-fabs(x)/3.1f,.64f); }
#ifdef SHADOW_DEMO
#include "../shadows/castmesh.h"
#endif
#ifdef FOG_WORLD
#include "../fog/outside.h"
#endif
int build_cathedral(void)
{
    int i,j,side;vertex_count=face_count=failed=glass_count=0;
#ifdef SHADOW_DEMO
    shadow_caster_count=0;
#endif
    quad(vec(-5.3f,0,-14),vec(5.3f,0,-14),vec(5.3f,0,14),vec(-5.3f,0,14),PAVING,vec(0,1,0));
    /* Paired stone piers, capitals and transverse pointed vault ribs. */
    for(j=0;j<7;++j) {
        float z=-12+j*4.0f;
        for(side=-1;side<=1;side+=2) {
            float x=side*3.1f;
#ifdef SHADOW_DEMO
            shadow_pier(x,z);
#endif
            box(x-.38f,0,z-.38f,x+.38f,.22f,z+.38f,STONE);
            cylinder(x,z,.22f,4.7f,.25f,STONE);
            box(x-.35f,4.5f,z-.35f,x+.35f,4.8f,z+.35f,TRIM);
        }
        for(i=0;i<12;++i) {
            float x0=-3.1f+i*(6.2f/12),x1=-3.1f+(i+1)*(6.2f/12);
            beam(vec(x0,vault_height(x0)-.05f,z),vec(x1,vault_height(x1)-.05f,z),.09f);
        }
    }
    /* Barrel-like pointed ceiling, longitudinal ribs and lower aisle roofs. */
    for(i=0;i<12;++i) {
        float x0=-3.1f+i*(6.2f/12),x1=-3.1f+(i+1)*(6.2f/12),y0=vault_height(x0),y1=vault_height(x1);
        quad(vec(x0,y0,-14),vec(x1,y1,-14),vec(x1,y1,14),vec(x0,y0,14),STONE,vec(0,-1,0));
    }
    for(side=-1;side<=1;side+=2) {
        float x=side*5.1f;Vec normal=vec((float)-side,0,0);
        quad(vec(side*3.1f,4.65f,-14),vec(x,4.3f,-14),vec(x,4.3f,14),vec(side*3.1f,4.65f,14),STONE,vec(0,-1,0));
        for(j=0;j<6;++j) {
            float z=-11.5f+j*4.5f,w=1.7f;
            /* Rectangular holes are completed to pointed arches by stone
               spandrel triangles surrounding the pane's arch curve. */
            quad(vec(x,0,z-1.1f),vec(x,0,z+1.1f),vec(x,1.2f,z+1.1f),vec(x,1.2f,z-1.1f),STONE,normal);
            quad(vec(x,4.05f,z-1.1f),vec(x,4.05f,z+1.1f),vec(x,4.55f,z+1.1f),vec(x,4.55f,z-1.1f),STONE,normal);
            quad(vec(x,0,z-2.25f),vec(x,0,z-w*.5f),vec(x,4.55f,z-w*.5f),vec(x,4.55f,z-2.25f),STONE,normal);
            quad(vec(x,0,z+w*.5f),vec(x,0,z+2.25f),vec(x,4.55f,z+2.25f),vec(x,4.55f,z+w*.5f),STONE,normal);
            glass_window(x-side*.015f,1.2f,z,w,2.85f,side<0?2:3);
        }
        quad(vec(x,0,13.25f),vec(x,0,14),vec(x,4.55f,14),vec(x,4.55f,13.25f),STONE,normal);
        quad(vec(x,0,-14),vec(x,0,-13.75f),vec(x,4.55f,-13.75f),vec(x,4.55f,-14),STONE,normal);
        /* Pointed longitudinal arcades above the open side aisles. */
        for(j=0;j<6;++j)for(i=0;i<8;++i) {
            float z0=-12+j*4+i*.5f,z1=z0+.5f;
            float u0=i/8.0f,u1=(i+1)/8.0f;
            float y0=2.8f+1.9f*(float)pow(1-fabs(2*u0-1),.64f);
            float y1=2.8f+1.9f*(float)pow(1-fabs(2*u1-1),.64f);
            quad(vec(side*3.1f,y0,z0),vec(side*3.1f,y1,z1),vec(side*3.1f,5.1f,z1),vec(side*3.1f,5.1f,z0),STONE,normal);
        }
        /* Thin upper wall connects the nave piers, preserving aisle views. */
        quad(vec(side*3.1f,4.60f,-14),vec(side*3.1f,4.60f,14),vec(side*3.1f,5.1f,14),vec(side*3.1f,5.1f,-14),STONE,normal);
    }
    /* Tall east window in a framed opening, and a dark west doorway. */
    quad(vec(-5.1f,0,14),vec(5.1f,0,14),vec(5.1f,1.9f,14),vec(-5.1f,1.9f,14),STONE,vec(0,0,-1));
    quad(vec(-5.1f,1.9f,14),vec(-2.0f,1.9f,14),vec(-2,7.8f,14),vec(-5.1f,7.8f,14),STONE,vec(0,0,-1));
    quad(vec(2,1.9f,14),vec(5.1f,1.9f,14),vec(5.1f,7.8f,14),vec(2,7.8f,14),STONE,vec(0,0,-1));
    quad(vec(-2,7.3f,14),vec(2,7.3f,14),vec(2,7.8f,14),vec(-2,7.8f,14),STONE,vec(0,0,-1));
    glass_window(0,1.9f,13.98f,4.0f,5.4f,0);
#ifdef FOG_WORLD
    west_portal();
#else
    quad(vec(-5.1f,0,-14),vec(5.1f,0,-14),vec(5.1f,7.8f,-14),vec(-5.1f,7.8f,-14),STONE,vec(0,0,1));
#endif
    /* Opaque wax and brass stands remain when the flame/glow switch is off. */
    for(i=0;i<12;++i) {
        Vec p=candle_positions[i];
        cylinder(p.x,p.z,0,.65f,.055f,TRIM);
        cylinder(p.x,p.z,.64f,.72f,.16f,TRIM);
        cylinder(p.x,p.z,.72f,p.y,.065f,TRIM);
    }
#ifdef FOG_WORLD
    build_graveyard();
#endif
    return !failed && face_count>0;
}
