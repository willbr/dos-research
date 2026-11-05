#include "../shadows/casters.h"
#define SHADOW_DEPTH_BIAS 24
typedef struct { float x,z; } ShadowPoint;
typedef struct { float x,y,z; } ShadowView;
typedef struct { float y,x0,x1,z0,z1;int bias; } ShadowGround;
/* Split grass around the slightly raised path so each mark uses its real Z. */
static const ShadowGround shadow_ground[5]={
    {0,-5.3f,5.3f,-14,14,-2},
    {-.025f,-25,-1.35f,-62,-14.18f,-4},{-.025f,1.35f,25,-62,-14.18f,-4},
    {-.025f,-1.35f,1.35f,-62,-55,-4},{.005f,-1.35f,1.35f,-55,-14,-2}
};
static int shadow_polygons,shadow_triangles,shadow_active_casters,shadow_pier_hits,shadow_grave_hits;
static Vec shadow_forward,shadow_right,shadow_up;
static float shadow_cross(ShadowPoint a,ShadowPoint b,ShadowPoint c)
{ return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x); }
static int shadow_hull(ShadowPoint *p,int count,ShadowPoint *out)
{
    int i,j,n=0,lower;ShadowPoint item;
    for(i=1;i<count;++i){item=p[i];j=i-1;while(j>=0 && (p[j].x>item.x || (p[j].x==item.x && p[j].z>item.z))){p[j+1]=p[j];--j;}p[j+1]=item;}
    for(i=0;i<count;++i){while(n>=2 && shadow_cross(out[n-2],out[n-1],p[i])<=.000001f)--n;out[n++]=p[i];}
    lower=n+1;
    for(i=count-2;i>=0;--i){while(n>=lower && shadow_cross(out[n-2],out[n-1],p[i])<=.000001f)--n;out[n++]=p[i];}
    return n>1?n-1:0;
}
static void shadow_project_point(Vec p,float floor_y,ShadowPoint *out)
{
    float scale=(projector_eye.y-floor_y)/(projector_eye.y-p.y);
    out->x=projector_eye.x+(p.x-projector_eye.x)*scale;out->z=projector_eye.z+(p.z-projector_eye.z)*scale;
}
static int shadow_project_caster(const ShadowCaster *c,float floor_y,ShadowPoint *out)
{
    ShadowPoint points[32];int i,j,n=0;float levels[2];
    levels[0]=floor_y+.001f;levels[1]=projector_eye.y-.01f;
    if(levels[1]<=levels[0] || c->high.y<levels[0] || c->low.y>levels[1])return 0;
    /* Piers are vertical prisms: clip the two rings directly, avoiding an
       edge walk and repeated divisions at the light's horizon. */
    if(c->points==16 && !c->kind) {
        float bottom=c->low.y>levels[0]?c->low.y:levels[0];
        float top=c->high.y<levels[1]?c->high.y:levels[1];
        float scales[2];scales[0]=(projector_eye.y-floor_y)/(projector_eye.y-bottom);
        scales[1]=(projector_eye.y-floor_y)/(projector_eye.y-top);
        for(j=0;j<2;++j)for(i=0;i<8;++i) {
            points[n].x=projector_eye.x+(c->point[i].x-projector_eye.x)*scales[j];
            points[n++].z=projector_eye.z+(c->point[i].z-projector_eye.z)*scales[j];
        }
        return shadow_hull(points,n,out);
    }
    for(i=0;i<c->points;++i)if(c->point[i].y>=levels[0] && c->point[i].y<=levels[1])shadow_project_point(c->point[i],floor_y,&points[n++]);
    for(i=0;i<c->edges;++i)for(j=0;j<2;++j) {
        Vec a=c->point[c->edge[i][0]],b=c->point[c->edge[i][1]];float y=levels[j];
        if((a.y<y && b.y>y)||(a.y>y && b.y<y)) {
            float t=(y-a.y)/(b.y-a.y);Vec p;
            if(n==32){finite_error=1;return 0;}
            p.x=a.x+(b.x-a.x)*t;p.y=y;p.z=a.z+(b.z-a.z)*t;
            shadow_project_point(p,floor_y,&points[n++]);
        }
    }
    return n>=3?shadow_hull(points,n,out):0;
}
static float shadow_rect_distance(ShadowPoint *p,int plane,float x0,float x1,float z0,float z1)
{
    switch(plane){case 0:return p->x-x0;case 1:return x1-p->x;case 2:return p->z-z0;default:return z1-p->z;}
}
static float shadow_view_distance(ShadowView *p,int plane)
{
    switch(plane){case 0:return p->z-.45f;case 1:return 160-p->z;case 2:return p->x+p->z*frustum_x;
        case 3:return p->z*frustum_x-p->x;case 4:return p->y+p->z*(172/focal);default:return p->z*(172/focal)-p->y;}
}
static void shadow_stamp(const ShadowPoint *hull,int count,const ShadowGround *ground,float x0,float x1,float z0,float z1)
{
    ShadowPoint a[48],b[48];ShadowView v[64],w[64];GrVertex q[64];int i,plane,mask=0,common=15;
    for(i=0;i<count;++i) {
        int code=0;a[i]=hull[i];
        if(a[i].x<x0)code|=1;if(a[i].x>x1)code|=2;if(a[i].z<z0)code|=4;if(a[i].z>z1)code|=8;
        mask|=code;common&=code;
    }
    if(common)return;
    for(plane=0;plane<4;++plane) {
        int n=0;
        if(!(mask&(1<<plane)))continue;
        for(i=0;i<count;++i) {
            ShadowPoint *p=&a[i],*r=&a[(i+1)%count];float dp=shadow_rect_distance(p,plane,x0,x1,z0,z1),dr=shadow_rect_distance(r,plane,x0,x1,z0,z1);
            if(n>45){finite_error=1;return;}
            if(dp>=0)b[n++]=*p;
            if((dp>=0)!=(dr>=0)){float t=dp/(dp-dr);b[n].x=p->x+(r->x-p->x)*t;b[n].z=p->z+(r->z-p->z)*t;++n;}
        }
        if(n<3)return;count=n;for(i=0;i<count;++i)a[i]=b[i];
    }
    mask=0;common=63;
    for(i=0;i<count;++i) {
        int code=0;
        float x=a[i].x-current_camera.eye.x,y=ground->y-current_camera.eye.y,z=a[i].z-current_camera.eye.z;
        v[i].x=x*shadow_right.x+y*shadow_right.y+z*shadow_right.z;
        v[i].y=x*shadow_up.x+y*shadow_up.y+z*shadow_up.z;v[i].z=x*shadow_forward.x+y*shadow_forward.y+z*shadow_forward.z;
        for(plane=0;plane<6;++plane)if(shadow_view_distance(&v[i],plane)<0)code|=1<<plane;
        mask|=code;common&=code;
    }
    if(common)return;
    for(plane=0;plane<6;++plane) {
        int n=0;
        if(!(mask&(1<<plane)))continue;
        for(i=0;i<count;++i) {
            ShadowView *p=&v[i],*r=&v[(i+1)%count];float dp=shadow_view_distance(p,plane),dr=shadow_view_distance(r,plane);
            if(n>61){finite_error=1;return;}
            if(dp>=0)w[n++]=*p;
            if((dp>=0)!=(dr>=0)){float t=dp/(dp-dr);w[n].x=p->x+(r->x-p->x)*t;w[n].y=p->y+(r->y-p->y)*t;w[n].z=p->z+(r->z-p->z)*t;++n;}
        }
        if(n<3)return;count=n;for(i=0;i<count;++i)v[i]=w[i];
    }
    memset(q,0,count*sizeof(GrVertex));
    for(i=0;i<count;++i) {
        float inv=1/v[i].z;
        q[i].x=320+v[i].x*focal*inv;q[i].y=260-v[i].y*focal*inv;q[i].ooz=65535*.45f*inv;q[i].oow=inv;
        if(q[i].x!=q[i].x || q[i].y!=q[i].y || fabs(q[i].x)>641 || fabs(q[i].y)>481){finite_error=1;return;}
    }
    grDepthBiasLevel(ground->bias+SHADOW_DEPTH_BIAS);grDrawPolygonVertexList(count,q);
    ++shadow_polygons;shadow_triangles+=count-2;
}
static void shadow_mask_state(void)
{
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXTRUE);
    grFogMode(GR_FOG_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);grConstantColorValue(0xff000000UL);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE);
}
static int shadow_in_beam(const ShadowCaster *c)
{
    Vec d,extent,axis;float q,r;int side;
    d.x=(c->low.x+c->high.x)*.5f-projector_eye.x;
    d.y=(c->low.y+c->high.y)*.5f-projector_eye.y;
    d.z=(c->low.z+c->high.z)*.5f-projector_eye.z;
    extent.x=(c->high.x-c->low.x)*.5f;extent.y=(c->high.y-c->low.y)*.5f;extent.z=(c->high.z-c->low.z)*.5f;
    q=PROJECT_DOT(d,projector_forward);
    r=extent.x*fabs(projector_forward.x)+extent.y*fabs(projector_forward.y)+extent.z*fabs(projector_forward.z);
    if(q+r<0 || q-r>projector_range)return 0;
    for(side=0;side<4;++side) {
        Vec a=side<2?projector_right:projector_up;float sign=(side&1)?-1:1;
        axis.x=projector_forward.x*projector_tan+sign*a.x;
        axis.y=projector_forward.y*projector_tan+sign*a.y;
        axis.z=projector_forward.z*projector_tan+sign*a.z;
        if(PROJECT_DOT(d,axis)+extent.x*fabs(axis.x)+extent.y*fabs(axis.y)+extent.z*fabs(axis.z)<0)return 0;
    }
    return 1;
}
static void shadow_draw(void)
{
    int receiver,i,active[5];ShadowPoint hull[64];float bounds[5][4];
    shadow_polygons=shadow_triangles=shadow_active_casters=shadow_pier_hits=shadow_grave_hits=0;
    if(!shadows_enabled || !projector_enabled)return;
    shadow_forward=unit(sub(current_camera.look,current_camera.eye));shadow_right=unit(cross(shadow_forward,vec(0,1,0)));shadow_up=cross(shadow_right,shadow_forward);
    shadow_mask_state();
    for(receiver=0;receiver<5;++receiver) {
        const ShadowGround *g=&shadow_ground[receiver];float x0=g->x0,x1=g->x1,z0=g->z0,z1=g->z1;
        active[receiver]=0;
        if(g->y<projector_low.y || g->y>projector_high.y || projector_eye.y<=g->y+.02f)continue;
        if(x0<projector_low.x)x0=projector_low.x;if(x1>projector_high.x)x1=projector_high.x;
        if(z0<projector_low.z)z0=projector_low.z;if(z1>projector_high.z)z1=projector_high.z;if(x0>=x1 || z0>=z1)continue;
        active[receiver]=1;bounds[receiver][0]=x0;bounds[receiver][1]=x1;bounds[receiver][2]=z0;bounds[receiver][3]=z1;
    }
    for(i=0;i<shadow_caster_count;++i) {
        ShadowCaster *c=&shadow_casters[i];int count=0;float previous_y=1e20f;
        if(c->high.y<-.025f || c->low.y>projector_eye.y || c->high.x<projector_low.x-.2f || c->low.x>projector_high.x+.2f || c->high.z<projector_low.z-.2f || c->low.z>projector_high.z+.2f)continue;
        if(!shadow_in_beam(c))continue;
        for(receiver=0;receiver<5;++receiver) {
            const ShadowGround *g=&shadow_ground[receiver];int before;
            if(!active[receiver])continue;
            /* All three grass rectangles share a plane and silhouette. */
            if(previous_y!=g->y){count=shadow_project_caster(c,g->y,hull);previous_y=g->y;}
            if(count<3)continue;
            before=shadow_polygons;shadow_stamp(hull,count,g,bounds[receiver][0],bounds[receiver][1],bounds[receiver][2],bounds[receiver][3]);
            if(shadow_polygons>before){++shadow_active_casters;if(c->kind)++shadow_grave_hits;else ++shadow_pier_hits;}
        }
    }
    grDepthMask(FXFALSE);
}
