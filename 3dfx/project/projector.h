/* Independent projector homogeneous coordinates, with camera depth unchanged. */
#define PROJECT_DOT(a,b) ((a).x*(b).x+(a).y*(b).y+(a).z*(b).z)
static int projector_enabled=1,projector_locked,projector_grid;
static int projector_count,projector_triangles,projector_intersections;
static float projector_time,projector_range=11,projector_tan=.36f,projector_min_camera_w;
static int projector_in_quad;
static Vec projector_eye,projector_forward,projector_right,projector_up,projector_low,projector_high;
static float projector_s[4],projector_t[4],projector_q[4];
static GrTexInfo projector_texture;
static FxU32 projector_address[2];
typedef struct { Vec center,low,high;float radius;int owner,count,ids[6]; } ProjectorBounds;
static ProjectorBounds projector_bounds[MAX_FACES];
static unsigned long projector_stamp[MAX_FACES],projector_frame;
static unsigned char projector_active[MAX_FACES];
typedef struct { float x,y,w,z; } ProjectorInput;
typedef struct { ProjectorInput p[12];int bias,count; } ProjectorSurface;
static ProjectorSurface projector_cache[1024];
static int projector_load(FxU32 *next,unsigned short *pixels)
{
    int kind,x,y;FxU32 bytes;
    projector_texture.smallLod=projector_texture.largeLod=GR_LOD_128;
    projector_texture.aspectRatio=GR_ASPECT_1x1;projector_texture.format=GR_TEXFMT_RGB_565;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&projector_texture);
    for(kind=0;kind<2;++kind) {
        for(y=0;y<128;++y)for(x=0;x<128;++x) {
            float u=(x-63.5f)/61,v=(y-63.5f)/61,d=u*u+v*v,fall=clamp((1-d)*2.5f,0,1),r,g,b;
            fall*=fall;
            if(!kind) {
                float ring=1-.07f*(float)cos(d*55);
                r=235*fall*ring;g=170*fall*ring;b=88*fall*ring;
            } else {
                int grid=x%16<2 || y%16<2;
                r=fall*(grid?240:35);g=fall*(grid?70:125);b=fall*(grid?30:205);
            }
            pixels[y*128+x]=rgb565((int)(r*.38f),(int)(g*.38f),(int)(b*.38f));
        }
        if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
        projector_address[kind]=*next;projector_texture.data=pixels;
        grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,&projector_texture);*next+=bytes;
    }
    projector_texture.data=NULL;return 1;
}
static void projector_mesh_init(void)
{
    int i,j,k;
    for(i=0;i<face_count;++i) {
        Vec lo=vec(1e20f,1e20f,1e20f),hi=vec(-1e20f,-1e20f,-1e20f),d;
        int first=i,last=i;ProjectorBounds *bound=&projector_bounds[i];bound->count=0;
        if(i && faces[i-1].v[0]==faces[i].v[0])first=i-1;
        if(i+1<face_count && faces[i+1].v[0]==faces[i].v[0])last=i+1;
        bound->owner=first;
        for(j=first;j<=last;++j)for(k=0;k<3;++k) {
            int n,id=faces[j].v[k];Vec p=vertices[id].p;
            for(n=0;n<bound->count;++n)if(bound->ids[n]==id)break;
            if(n==bound->count && bound->count<6)bound->ids[bound->count++]=id;
            if(p.x<lo.x)lo.x=p.x;if(p.y<lo.y)lo.y=p.y;if(p.z<lo.z)lo.z=p.z;
            if(p.x>hi.x)hi.x=p.x;if(p.y>hi.y)hi.y=p.y;if(p.z>hi.z)hi.z=p.z;
        }
        bound->low=lo;bound->high=hi;
        projector_bounds[i].center=mul(add(lo,hi),.5f);d=sub(hi,lo);
        projector_bounds[i].radius=.5f*(float)sqrt(dot(d,d))+.001f;
    }
}
static void projector_coefficients(Vec axis,Vec right,Vec up,Vec forward,Vec offset,float *c)
{
    float x=dot(axis,right)/focal,y=-dot(axis,up)/focal;
    c[0]=x;c[1]=y;c[2]=dot(axis,offset);c[3]=dot(axis,forward)-320*x-260*y;
}
static void projector_axis_bounds(float position,float forward,float right,float up,float *lo,float *hi)
{
    float extent=projector_tan*((float)fabs(right)+(float)fabs(up)),low=forward-extent,high=forward+extent;
    *lo=position+low*(low<0?projector_range:.12f)-.005f;
    *hi=position+high*(high>0?projector_range:.12f)+.005f;
}
static void projector_prepare(int paused)
{
    Vec forward=unit(sub(current_camera.look,current_camera.eye)),right=unit(cross(forward,vec(0,1,0))),up=cross(right,forward),offset;
    float u[4],v[4],q[4];int i;
    if(!paused)projector_time+=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
    if(!projector_locked) {
        float pan=.55f*(float)sin(projector_time*.73f),dip=-.42f+.20f*(float)sin(projector_time*.49f);
        projector_eye=add(current_camera.eye,add(mul(right,.45f),mul(up,.16f)));
        projector_forward=unit(add(forward,add(mul(right,pan),mul(up,dip))));
        projector_right=unit(cross(projector_forward,vec(0,1,0)));projector_up=cross(projector_right,projector_forward);
    }
    projector_axis_bounds(projector_eye.x,projector_forward.x,projector_right.x,projector_up.x,&projector_low.x,&projector_high.x);
    projector_axis_bounds(projector_eye.y,projector_forward.y,projector_right.y,projector_up.y,&projector_low.y,&projector_high.y);
    projector_axis_bounds(projector_eye.z,projector_forward.z,projector_right.z,projector_up.z,&projector_low.z,&projector_high.z);
    offset=sub(current_camera.eye,projector_eye);
    projector_min_camera_w=1/(projector_range*1.123f+(float)sqrt(dot(offset,offset))+.02f);
    projector_coefficients(projector_right,right,up,forward,offset,u);
    projector_coefficients(projector_up,right,up,forward,offset,v);
    projector_coefficients(projector_forward,right,up,forward,offset,q);
    for(i=0;i<4;++i){projector_s[i]=127.5f*(q[i]+u[i]/projector_tan);projector_t[i]=127.5f*(q[i]-v[i]/projector_tan);projector_q[i]=q[i];}
    projector_count=projector_triangles=projector_intersections=0;
    if(++projector_frame==0){memset(projector_stamp,0,sizeof(projector_stamp));projector_frame=1;}
}
static int projector_classify(int face)
{
    Face *f=&faces[face];ProjectorBounds *b=&projector_bounds[face];Vec d;float z,x,y,r=b->radius,side=1.063f;int active=0;
    if(!projector_enabled)return 0;
    if(projector_stamp[face]==projector_frame)return projector_active[face];
    projector_stamp[face]=projector_frame;
    if(f->normal.x*projector_eye.x+f->normal.y*projector_eye.y+f->normal.z*projector_eye.z>f->plane+.001f) {
        d.x=b->center.x-projector_eye.x;d.y=b->center.y-projector_eye.y;d.z=b->center.z-projector_eye.z;
        z=PROJECT_DOT(d,projector_forward);
        if(z+r>=.12f && z-r<=projector_range) {
            x=PROJECT_DOT(d,projector_right);y=PROJECT_DOT(d,projector_up);
            active=fabs(x)<=z*projector_tan+r*side && fabs(y)<=z*projector_tan+r*side;
        }
    }
    if(active) {
        unsigned char common=63;int i;
        for(i=0;i<b->count;++i) {
            Vec p=vertices[b->ids[i]].p;Vec delta;float u,v,q,side;unsigned char code=0;
            delta.x=p.x-projector_eye.x;delta.y=p.y-projector_eye.y;delta.z=p.z-projector_eye.z;
            u=PROJECT_DOT(delta,projector_right);v=PROJECT_DOT(delta,projector_up);q=PROJECT_DOT(delta,projector_forward);side=q*projector_tan;
            if(q<.12f)code|=1;if(q>projector_range)code|=2;
            if(u< -side)code|=4;if(u>side)code|=8;if(v< -side)code|=16;if(v>side)code|=32;
            common&=code;
        }
        if(common)active=0;
    }
    projector_active[face]=(unsigned char)active;return active;
}
#define projector_candidate(id) (!projector_enabled?0:(projector_stamp[projector_bounds[id].owner]==projector_frame?projector_active[projector_bounds[id].owner]:projector_classify(projector_bounds[id].owner)))
#define PROJECT_BOX_OUTSIDE(b) ((b)->low.y>projector_high.y || (b)->high.y<projector_low.y || (b)->low.x>projector_high.x || (b)->high.x<projector_low.x || (b)->low.z>projector_high.z || (b)->high.z<projector_low.z)
static void projector_record_polygon(Face *f,const GrVertex *list,const int *ids,int count)
{
    ProjectorSurface *t;ProjectorBounds *bound=&projector_bounds[f-faces];int i,nearby=0;
    if(!projector_enabled || PROJECT_BOX_OUTSIDE(bound))return;
    for(i=0;i<count;++i)if(list[ids?ids[i]:i].oow>=projector_min_camera_w){nearby=1;break;}
    if(!nearby || !projector_candidate(f-faces))return;
    if(projector_count==1024 || count>12){finite_error=1;return;}
    t=&projector_cache[projector_count++];t->count=count;
    for(i=0;i<count;++i){const GrVertex *p=&list[ids?ids[i]:i];t->p[i].x=p->x;t->p[i].y=p->y;t->p[i].w=p->oow;t->p[i].z=p->ooz;}
    t->bias=f->mat==GRASS?-4:f->mat==PAVING?-2:0;
}
static void projector_record(Face *f,const GrVertex *a,const GrVertex *b,const GrVertex *c)
{
    ProjectorSurface *t;ProjectorBounds *bound=&projector_bounds[f-faces];
    if(projector_in_quad || !projector_enabled || PROJECT_BOX_OUTSIDE(bound))return;
    if(a->oow<projector_min_camera_w && b->oow<projector_min_camera_w && c->oow<projector_min_camera_w)return;
    if(!projector_candidate(f-faces))return;
    if(projector_count==1024){finite_error=1;return;}
    t=&projector_cache[projector_count++];t->count=3;
    t->p[0].x=a->x;t->p[0].y=a->y;t->p[0].w=a->oow;t->p[0].z=a->ooz;
    t->p[1].x=b->x;t->p[1].y=b->y;t->p[1].w=b->oow;t->p[1].z=b->ooz;
    t->p[2].x=c->x;t->p[2].y=c->y;t->p[2].w=c->oow;t->p[2].z=c->ooz;
    t->bias=f->mat==GRASS?-4:f->mat==PAVING?-2:0;
}
typedef struct { float x,y,w,z,s,t,q; } ProjectorVertex;
static float projector_plane(ProjectorVertex *v,int plane)
{
    switch(plane){case 0:return v->q-.12f*v->w;case 1:return projector_range*v->w-v->q;
        case 2:return v->s;case 3:return 255*v->q-v->s;case 4:return v->t;default:return 255*v->q-v->t;}
}
static void projector_interpolate(ProjectorVertex *a,ProjectorVertex *b,float t,ProjectorVertex *out)
{
    out->x=a->x+(b->x-a->x)*t;out->y=a->y+(b->y-a->y)*t;out->w=a->w+(b->w-a->w)*t;out->z=a->z+(b->z-a->z)*t;
    out->s=a->s+(b->s-a->s)*t;out->t=a->t+(b->t-a->t)*t;out->q=a->q+(b->q-a->q)*t;
}
static void projector_polygon(ProjectorSurface *source)
{
    ProjectorVertex a[24],b[24];GrVertex p[24];int i,plane,count=source->count;unsigned char mask=0,common=63;
    for(i=0;i<count;++i) {
        ProjectorInput *s=&source->p[i];ProjectorVertex *v=&a[i];unsigned char code=0;
        v->x=s->x;v->y=s->y;v->w=s->w;v->z=s->z;
        v->s=projector_s[0]*v->x+projector_s[1]*v->y+projector_s[2]*v->w+projector_s[3];
        v->t=projector_t[0]*v->x+projector_t[1]*v->y+projector_t[2]*v->w+projector_t[3];
        v->q=projector_q[0]*v->x+projector_q[1]*v->y+projector_q[2]*v->w+projector_q[3];
        if(v->q<.12f*v->w)code|=1;if(v->q>projector_range*v->w)code|=2;
        if(v->s<0)code|=4;if(v->s>255*v->q)code|=8;if(v->t<0)code|=16;if(v->t>255*v->q)code|=32;
        mask|=code;common&=code;
    }
    if(common)return;++projector_intersections;
    for(plane=0;plane<6;++plane)if(mask&(1<<plane)) {
        int n=0;
        for(i=0;i<count;++i) {
            ProjectorVertex *u=&a[i],*v=&a[(i+1)%count];float du=projector_plane(u,plane),dv=projector_plane(v,plane);
            if(n>21){finite_error=1;return;}
            if(du>=0)b[n++]=*u;
            if((du>=0)!=(dv>=0))projector_interpolate(u,v,du/(du-dv),&b[n++]);
        }
        if(n<3)return;count=n;for(i=0;i<count;++i)a[i]=b[i];
    }
    memset(p,0,count*sizeof(GrVertex));
    for(i=0;i<count;++i) {
        ProjectorVertex *v=&a[i];
        if(v->w<=0 || v->q<=0 || v->q!=v->q){finite_error=1;return;}
        p[i].x=v->x;p[i].y=v->y;p[i].oow=v->w;p[i].ooz=v->z;
        p[i].a=projector_grid?255:255*clamp((projector_range-v->q/v->w)/(projector_range*.45f),0,1);
        p[i].tmuvtx[0].sow=v->s;p[i].tmuvtx[0].tow=v->t;p[i].tmuvtx[0].oow=v->q;
    }
    grDrawPolygonVertexList(count,p);projector_triangles+=count-2;
}
static void projector_draw(void)
{
    int i,last_bias=999;
    if(!projector_enabled)return;
    grDepthMask(FXFALSE);grDepthBufferFunction(GR_CMP_GEQUAL);grAlphaTestFunction(GR_CMP_ALWAYS);grFogMode(fog_enabled?GR_FOG_WITH_TABLE|GR_FOG_ADD2:GR_FOG_DISABLE);
    grTexSource(GR_TMU0,projector_address[projector_grid],GR_MIPMAPLEVELMASK_BOTH,&projector_texture);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grHints(GR_HINT_STWHINT,GR_STWHINT_W_DIFF_TMU0);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL_ALPHA,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
#ifdef SHADOW_DEMO
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
#endif
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
    for(i=0;i<projector_count;++i) {
        ProjectorSurface *t=&projector_cache[i];
        if(t->bias+1!=last_bias){grDepthBiasLevel(t->bias+1);last_bias=t->bias+1;}
        projector_polygon(t);
    }
    grHints(GR_HINT_STWHINT,0);

}
#include "../project/hud.h"
