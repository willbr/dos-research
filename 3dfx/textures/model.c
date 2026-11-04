/* Interpreted exterior from the supplied photographs, aerial views and plan.
   Approximate units: four metres. No reference images are embedded. */
#include <math.h>
#include "model.h"
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
    light=.55f+.45f*dot(f->normal,unit(vec(-.5f,.9f,-.4f)));if(light<.42f)light=.42f;
    if(mat==WINDOW || mat==DOOR)light=.92f;
    if(mat==SHADOW)light=.43f;
    if(mat==TRIM)light*=1.12f;
    if(light>1)light=1;
    f->r=255*light;f->g=252*light;f->b=243*light;
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
static void gable(float x0,float z0,float x1,float z1,float eave,float ridge,int along_z)
{
    Vec a,b,c,d,n;float mid=(along_z?x0+x1:z0+z1)*.5f;int i,j,k;
    if(along_z) {
        a=vec(x0,eave,z0);b=vec(mid,ridge,z0);c=vec(mid,ridge,z1);d=vec(x0,eave,z1);
        n=cross(sub(b,a),sub(d,a));if(n.y<0)n=mul(n,-1);quad(a,b,c,d,ROOF,n);
        a.x=d.x=x1;n.x=-n.x;quad(a,b,c,d,ROOF,n);
        for(k=0;k<2;++k) {
            float z=k?z1:z0;Vec normal=vec(0,0,k?1.0f:-1.0f);
            i=vertex(vec(x0,eave,z),x0*128,-eave*128);j=vertex(vec(x1,eave,z),x1*128,-eave*128);
            {int tip=vertex(vec(mid,ridge,z),mid*128,-ridge*128);triangle(i,j,tip,STONE,normal);}
        }
    } else {
        a=vec(x0,eave,z0);b=vec(x0,ridge,mid);c=vec(x1,ridge,mid);d=vec(x1,eave,z0);
        n=cross(sub(b,a),sub(d,a));if(n.y<0)n=mul(n,-1);quad(a,b,c,d,ROOF,n);
        a.z=d.z=z1;n.z=-n.z;quad(a,b,c,d,ROOF,n);
        for(k=0;k<2;++k) {
            float x=k?x1:x0;Vec normal=vec(k?1.0f:-1.0f,0,0);
            i=vertex(vec(x,eave,z0),z0*128,-eave*128);j=vertex(vec(x,eave,z1),z1*128,-eave*128);
            {int tip=vertex(vec(x,ridge,mid),mid*128,-ridge*128);triangle(i,j,tip,STONE,normal);}
        }
    }
}
static void window(float x,float y,float z,float w,float h,int side,int mat)
{
    Vec center=vec(x,y,z),u=side<2?vec(w*.5f,0,0):vec(0,0,w*.5f);
    Vec n=side==0?vec(0,0,-1):side==1?vec(0,0,1):side==2?vec(-1,0,0):vec(1,0,0);
    quad_uv(sub(center,u),add(center,u),add(add(center,u),vec(0,h,0)),add(sub(center,u),vec(0,h,0)),mat,n,0,0,255,255);
}
static void pinnacle(float x,float y,float z,float size,float height)
{
    int i;Vec top=vec(x,y+height,z);
    box(x-size,y-.25f,z-size,x+size,y+.20f,z+size,TRIM);
    for(i=0;i<4;++i) {
        Vec a=vec(x+(i==0||i==3?-size:size),y+.2f,z+(i<2?-size:size));
        Vec b=vec(x+((i+1)%4==0||(i+1)%4==3?-size:size),y+.2f,z+((i+1)%4<2?-size:size));
        Vec n=cross(sub(b,a),sub(top,a));
        int aa=vertex(a,0,255),bb=vertex(b,255,255),cc=vertex(top,128,0);triangle(aa,bb,cc,STONE,n);
    }
}
static void beam(Vec a,Vec b,float radius)
{
    Vec side=vec(0,0,radius),up=mul(unit(vec(-(b.y-a.y),b.x-a.x,0)),radius);
    Vec p[4],q[4];int i;
    p[0]=add(add(a,side),up);p[1]=add(sub(a,side),up);p[2]=sub(sub(a,side),up);p[3]=sub(add(a,side),up);
    for(i=0;i<4;++i)q[i]=add(p[i],sub(b,a));
    for(i=0;i<4;++i) {int j=(i+1)%4;Vec n=unit(sub(mul(add(p[i],p[j]),.5f),a));quad(p[i],p[j],q[j],q[i],TRIM,n);}
}
int build_cathedral(void)
{
    int side,i;vertex_count=face_count=failed=0;
    /* Cathedral close, a west forecourt and a long oblique texture control. */
    quad(vec(-160,0,-160),vec(160,0,-160),vec(160,0,160),vec(-160,0,160),GRASS,vec(0,1,0));
    quad(vec(-12,.025f,-32),vec(12,.025f,-32),vec(12,.025f,-22),vec(-12,.025f,-22),PAVING,vec(0,1,0));
    quad(vec(-5,.025f,-22),vec(-3.55f,.025f,-22),vec(-3.55f,.025f,22),vec(-5,.025f,22),PAVING,vec(0,1,0));
    quad(vec(3.55f,.025f,-22),vec(5,.025f,-22),vec(5,.025f,22),vec(3.55f,.025f,22),PAVING,vec(0,1,0));
    /* Nave and choir: long, tall central vessel above low lean-to aisles. */
    box(-1.72f,0,-22,1.72f,6.15f,10,STONE);gable(-1.85f,-22,1.85f,10,6.15f,8.25f,1);
    for(side=-1;side<=1;side+=2) {
        float x=side*3.52f;
        box(side<0?x:1.72f,0,-22,side<0?-1.72f:x,3.1f,10,STONE);
        quad(vec(side*1.72f,4.25f,-22),vec(x,3.15f,-22),vec(x,3.15f,10),vec(side*1.72f,4.25f,10),ROOF,vec(side*.61f,1,0));
        box(side<0?x-.06f:x-.08f,3.06f,-22,side<0?x+.08f:x+.06f,3.20f,10,TRIM);
        for(i=0;i<14;++i) {
            float z=-20.9f+i*2.2f;if(z>-.6f && z<4.7f)continue;
            window(side*1.735f,4.55f,z,.96f,1.32f,side<0?2:3,WINDOW);
            window(x+side*.018f,.65f,z,1.15f,2.17f,side<0?2:3,WINDOW);
            box(side<0?x-.55f:x-.05f,0,z-1.03f,side<0?x+.05f:x+.55f,2.95f,z-.76f,STONE);
            box(side<0?x-.46f:x-.03f,2.95f,z-1.0f,side<0?x+.03f:x+.46f,3.65f,z-.79f,TRIM);
            pinnacle(x+side*.21f,3.65f,z-.89f,.10f,.80f);
            beam(vec(side*1.78f,5.15f,z-.89f),vec(x+side*.15f,3.5f,z-.89f),.095f);
            box(side*1.72f-.065f,4.15f,z-1.04f,side*1.72f+.065f,6.38f,z-.75f,TRIM);
        }
    }
    /* Transepts through the crossing, with lower eastern chapels. */
    box(-7.45f,0,-.25f,7.45f,5.95f,4.25f,STONE);gable(-7.65f,-.38f,7.65f,4.38f,5.95f,8.15f,0);
    for(side=-1;side<=1;side+=2) {
        float x=side*7.47f;
        for(i=0;i<3;++i)window(x,1.15f,.45f+i*1.25f,.73f,3.75f,side<0?2:3,WINDOW);
        for(i=0;i<3;++i) {
            float xx=side*(2.65f+i*1.65f);
            window(xx,3.6f,-.27f,.78f,1.88f,0,WINDOW);
            window(xx,.7f,-.28f,1.12f,2.24f,0,WINDOW);
        }
        box(side<0?-7.48f:2.4f,0,4.2f,side<0?-2.4f:7.48f,2.7f,6.1f,STONE);
        gable(side<0?-7.5f:2.4f,4.15f,side<0?-2.4f:7.5f,6.2f,2.7f,3.7f,0);
        for(i=0;i<3;++i)window(side*(3.2f+i*1.5f),.65f,6.12f,.85f,1.8f,1,WINDOW);
        for(i=0;i<2;++i)pinnacle(x,5.95f,i?4.2f:-.2f,.12f,.9f);
    }
    /* Square Norman crossing tower, shallow roof and corner parapets. */
    box(-1.97f,6.0f,-.02f,1.97f,10.85f,4.02f,STONE);
    box(-2.08f,10.75f,-.13f,2.08f,10.97f,4.13f,TRIM);
    box(-1.82f,10.96f,.12f,1.82f,11.02f,3.88f,ROOF);
    for(side=-1;side<=1;side+=2)for(i=0;i<3;++i) {
        window(side*1.985f,8.35f,.65f+i*1.32f,.77f,2.05f,side<0?2:3,WINDOW);
        window(-1.28f+i*1.28f,8.35f,side<0?-.035f:4.035f,.77f,2.05f,side<0?0:1,WINDOW);
    }
    for(side=-1;side<=1;side+=2)for(i=0;i<2;++i)box(side*1.93f-.13f,10.85f,i?3.75f:-.05f,side*1.93f+.13f,11.3f,i?4.05f:.25f,TRIM);
    /* Lower, wider retrochoir and stepped Lady Chapel at the east end. */
    box(-3.28f,0,10,3.28f,3.55f,18.6f,STONE);gable(-3.4f,9.9f,3.4f,18.7f,3.55f,5.8f,1);
    box(-1.48f,0,18.6f,1.48f,3.25f,22.5f,STONE);gable(-1.58f,18.5f,1.58f,22.6f,3.25f,4.8f,1);
    for(side=-1;side<=1;side+=2)for(i=0;i<4;++i) {
        float z=11.15f+i*2.0f;
        window(side*3.30f,.58f,z,1.03f,2.68f,side<0?2:3,WINDOW);
        box(side<0?-3.66f:3.22f,0,z+.72f,side<0?-3.22f:3.66f,3.55f,z+.96f,STONE);
        pinnacle(side*3.42f,3.55f,z+.84f,.09f,.65f);
    }
    window(0,.65f,22.52f,2.3f,2.55f,1,WINDOW);
    /* Perpendicular west front: large traceried window, three portals,
       high gable and two slender flanking pinnacles, not twin bell towers. */
    box(-1.95f,0,-22.16f,1.95f,6.25f,-21.82f,STONE);
    gable(-1.95f,-22.18f,1.95f,-21.82f,6.25f,8.65f,1);
    window(0,2.35f,-22.185f,3.03f,4.06f,0,WINDOW);
    for(i=-1;i<=1;++i)window(i*2.35f,.05f,-22.185f,i?1.05f:1.50f,2.20f,0,DOOR);
    for(side=-1;side<=1;side+=2) {
        box(side*1.88f-.16f,0,-22.36f,side*1.88f+.16f,8.18f,-21.92f,TRIM);
        pinnacle(side*1.88f,8.18f,-22.12f,.21f,2.03f);
        box(side*3.45f-.16f,0,-22.32f,side*3.45f+.16f,4.5f,-21.88f,TRIM);
        pinnacle(side*3.45f,4.5f,-22.1f,.14f,1.00f);
        window(side*2.6f,2.48f,-22.026f,.75f,.55f,0,WINDOW);
    }
    pinnacle(0,8.65f,-22.08f,.12f,.72f);
    box(-1.89f,2.22f,-22.23f,1.89f,2.36f,-22.08f,TRIM);
    box(-1.89f,6.23f,-22.23f,1.89f,6.39f,-22.08f,TRIM);
    /* Low forecourt walls and paving establish scale without a city model. */
    box(-12,0,-22.4f,-5.7f,.34f,-22.0f,STONE);box(5.7f,0,-22.4f,12,.34f,-22.0f,STONE);
    return !failed && face_count>0;
}
