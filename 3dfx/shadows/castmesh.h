#include "../shadows/casters.h"
ShadowCaster shadow_casters[MAX_SHADOW_CASTERS];int shadow_caster_count;
static void shadow_prism(Vec *ring,int count,Vec extrusion,int kind)
{
    ShadowCaster *c;int i,j;
    if(shadow_caster_count==MAX_SHADOW_CASTERS){failed=1;return;}
    c=&shadow_casters[shadow_caster_count++];c->points=count*2;c->edges=count*3;c->kind=kind;
    c->low=vec(1e20f,1e20f,1e20f);c->high=vec(-1e20f,-1e20f,-1e20f);
    for(i=0;i<count;++i) {
        c->point[i]=ring[i];c->point[i+count]=add(ring[i],extrusion);
        c->edge[i][0]=(unsigned char)i;c->edge[i][1]=(unsigned char)((i+1)%count);
        c->edge[i+count][0]=(unsigned char)(i+count);c->edge[i+count][1]=(unsigned char)((i+1)%count+count);
        c->edge[i+2*count][0]=(unsigned char)i;c->edge[i+2*count][1]=(unsigned char)(i+count);
    }
    for(j=0;j<c->points;++j) {
        Vec p=c->point[j];
        if(p.x<c->low.x)c->low.x=p.x;if(p.y<c->low.y)c->low.y=p.y;if(p.z<c->low.z)c->low.z=p.z;
        if(p.x>c->high.x)c->high.x=p.x;if(p.y>c->high.y)c->high.y=p.y;if(p.z>c->high.z)c->high.z=p.z;
    }
}
static void shadow_box(float x0,float y0,float z0,float x1,float y1,float z1,int kind)
{
    Vec p[4];p[0]=vec(x0,y0,z0);p[1]=vec(x1,y0,z0);p[2]=vec(x1,y1,z0);p[3]=vec(x0,y1,z0);
    shadow_prism(p,4,vec(0,0,z1-z0),kind);
}
static void shadow_pier(float x,float z)
{
    Vec p[8];int i;
    shadow_box(x-.38f,0,z-.38f,x+.38f,.22f,z+.38f,0);
    for(i=0;i<8;++i){float a=i*(6.28318530718f/8);p[i]=vec(x+.25f*(float)cos(a),.22f,z+.25f*(float)sin(a));}
    shadow_prism(p,8,vec(0,4.48f,0),0);
    shadow_box(x-.35f,4.5f,z-.35f,x+.35f,4.8f,z+.35f,0);
}
static void shadow_headstone(float x,float z,int style)
{
    float h=.9f+(style%3)*.14f;Vec p[7];int i;
    shadow_box(x-.52f,0,z-.32f,x+.52f,.12f,z+.32f,1);
    if(style%4==0) {
        shadow_box(x-.12f,.12f,z-.12f,x+.12f,h+.6f,z+.12f,1);
        shadow_box(x-.43f,h,z-.12f,x+.43f,h+.22f,z+.12f,1);
    } else {
        p[0]=vec(x-.38f,.12f,z-.12f);p[1]=vec(x+.38f,.12f,z-.12f);
        for(i=0;i<5;++i){float a=i*3.141593f/4;p[i+2]=vec(x+.38f*(float)cos(a),h+.38f*(float)sin(a),z-.12f);}
        shadow_prism(p,7,vec(0,0,.24f),1);
    }
}
