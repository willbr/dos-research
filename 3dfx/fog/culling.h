#include "../fog/visibility.h"
#define FOG_DOT(a,b) ((a).x*(b).x+(a).y*(b).y+(a).z*(b).z)
typedef struct { Vec normal[4];float radius[4]; } FogPortal;
static FogPortal fog_portals[14];
static int fog_portal_count,fog_inside,fog_culling_enabled=1;
static int fog_sphere_visible(Vec d,float r,Vec forward,Vec right,Vec up,float bx,float by,float rx,float ry)
{
    float z=FOG_DOT(d,forward);
    return z+r>=.45f && z-r<=160 && fabs(FOG_DOT(d,right))<=z*bx+r*rx && fabs(FOG_DOT(d,up))<=z*by+r*ry;
}
static void fog_portal_add(Vec *p,Vec forward,Vec right,Vec up,float bx,float by,float rx,float ry)
{
    FogPortal *portal;Vec center=mul(add(p[0],p[2]),.5f),d=sub(center,current_camera.eye),extent=sub(p[2],p[0]);
    float radius=.5f*(float)sqrt(dot(extent,extent));int i;
    if(!fog_sphere_visible(d,radius,forward,right,up,bx,by,rx,ry))return;
    portal=&fog_portals[fog_portal_count++];
    for(i=0;i<4;++i) {
        Vec n=cross(sub(p[i],current_camera.eye),sub(p[(i+1)%4],current_camera.eye));
        if(dot(n,d)<0)n=mul(n,-1);
        portal->normal[i]=n;portal->radius[i]=(float)sqrt(dot(n,n));
    }
}
static void fog_portal_prepare(Vec forward,Vec right,Vec up,float bx,float by,float rx,float ry)
{
    Vec p[4];int i;
    fog_inside=current_camera.eye.z> -13.6f && current_camera.eye.z<14 && fabs(current_camera.eye.x)<5.1f;
    fog_portal_count=0;
    p[0]=vec(-1.45f,0,-14);p[1]=vec(1.45f,0,-14);p[2]=vec(1.45f,3.9f,-14);p[3]=vec(-1.45f,3.9f,-14);
    fog_portal_add(p,forward,right,up,bx,by,rx,ry);
    if(fog_inside)for(i=0;i<glass_count;++i)fog_portal_add(glass_windows[i].p,forward,right,up,bx,by,rx,ry);
}
static int fog_through_portal(Vec d,float r)
{
    int i,j;
    for(i=0;i<fog_portal_count;++i) {
        FogPortal *p=&fog_portals[i];
        for(j=0;j<4;++j)if(FOG_DOT(p->normal[j],d)< -r*p->radius[j])break;
        if(j==4)return 1;
    }
    return 0;
}
static int fog_group_visible(int index,Vec forward,Vec right,Vec up,float bx,float by,float rx,float ry)
{
    FogGroup *g=&fog_groups[index];Vec d;float r=g->radius;
    d.x=g->center.x-current_camera.eye.x;d.y=g->center.y-current_camera.eye.y;d.z=g->center.z-current_camera.eye.z;
    if(g->lod) {
        float x=g->lod_center.x-current_camera.eye.x,y=g->lod_center.y-current_camera.eye.y,z=g->lod_center.z-current_camera.eye.z;
        int coarse=x*x+y*y+z*z>g->lod_distance;
        if((g->lod==1 && coarse)||(g->lod==2 && !coarse))return 0;
    }
    if(!fog_culling_enabled)return 1;
    if(!fog_sphere_visible(d,r,forward,right,up,bx,by,rx,ry))return 0;
    if(fog_inside && g->first>=fog_nave_faces && g->maxz< -14)return fog_through_portal(d,r);
    if(g->last<=fog_nave_faces && current_camera.eye.z< -14.5f && fabs(current_camera.eye.x)<5.1f &&
       (g->last<=fog_facade_first || g->first>=fog_facade_last))return fog_through_portal(d,r);
    return 1;
}
