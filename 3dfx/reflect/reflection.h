/* Planar reflection, painter ordered within a convex puddle portal.
   Raster Z belongs to the water plane; texture/fog W belongs to the mirror. */
#define REFLECT_MAX 768
#include "../reflect/footprint.h"
#ifdef WATER_DEMO
#define REFLECT_ORDER_MAX (REFLECT_MAX+FOLIAGE_CARDS)
static int reflect_vegetation_enabled=1,reflect_plant_culling=1,reflect_grass_polygons,reflect_plant_polygons;
#else
#define REFLECT_ORDER_MAX REFLECT_MAX
#endif
static int reflect_low_enabled=1;
typedef struct { float x,y,z,u,v; } ReflectVertex;
typedef struct { Vec n;float d; } ReflectPlane;
typedef struct { int face,count,ids[4],two_sided;Vec center;float radius; } ReflectSurface;
static ReflectSurface reflect_mesh[REFLECT_MAX];
static int reflect_count,reflect_order[REFLECT_ORDER_MAX],reflect_visible,reflect_polygons,reflect_triangles;
static float reflect_sort[REFLECT_ORDER_MAX];
static ReflectPlane reflect_planes[15];
static ReflectVertex reflect_vertices[MAX_VERTS];
static unsigned long reflect_stamps[MAX_VERTS],reflect_frame;
static Vec reflect_forward,reflect_right,reflect_up;
static GrVertex water_polygon[24];static int water_count;
static int reflection_quick;
static float reflect_distance(const ReflectVertex *v,int plane)
{
    ReflectPlane *p=&reflect_planes[plane];return v->x*p->n.x+v->y*p->n.y+v->z*p->n.z+p->d;
}
static int reflect_clip(ReflectVertex *a,int count,int planes,unsigned int mask)
{
    ReflectVertex b[24];int plane,i;
    for(plane=0;plane<planes;++plane)if(mask&(1U<<plane)) {
        int n=0;
        for(i=0;i<count;++i) {
            ReflectVertex *p=&a[i],*r=&a[(i+1)%count];float dp=reflect_distance(p,plane),dr=reflect_distance(r,plane);
            if(n>21){finite_error=1;return 0;}
            if(dp>=0)b[n++]=*p;
            if((dp>=0)!=(dr>=0)) {
                float t=dp/(dp-dr);
                b[n].x=p->x+(r->x-p->x)*t;b[n].y=p->y+(r->y-p->y)*t;b[n].z=p->z+(r->z-p->z)*t;
                b[n].u=p->u+(r->u-p->u)*t;b[n].v=p->v+(r->v-p->v)*t;++n;
            }
        }
        if(n<3)return 0;count=n;for(i=0;i<count;++i)a[i]=b[i];
    }
    return count;
}
static ReflectVertex reflect_view(Vec p,float u,float v)
{
    ReflectVertex out;Vec d;d.x=p.x-current_camera.eye.x;d.y=p.y-current_camera.eye.y;d.z=p.z-current_camera.eye.z;
    out.x=PROJECT_DOT(d,reflect_right);out.y=PROJECT_DOT(d,reflect_up);out.z=PROJECT_DOT(d,reflect_forward);out.u=u;out.v=v;return out;
}
#ifdef WATER_DEMO
static int reflection_plant_visible(Vec center,float radius)
{
    ReflectVertex v;int plane;if(!reflect_plant_culling)return 1;
    center.y=2*WATER_Y-center.y;
    if(fog_inside && !fog_through_portal(sub(center,current_camera.eye),radius))return 0;
    v=reflect_view(center,0,0);
    for(plane=0;plane<15;++plane)if(reflect_distance(&v,plane)< -radius)return 0;
    return 1;
}
#endif
static void reflect_project(const ReflectVertex *v,GrVertex *p)
{
    float inv=1/v->z,ray_y;
    memset(p,0,sizeof(*p));p->x=320+v->x*focal*inv;p->y=260-v->y*focal*inv;
    ray_y=reflect_forward.y+reflect_right.y*(p->x-320)/focal+reflect_up.y*(260-p->y)/focal;
    p->ooz=-65535*.45f*ray_y/(current_camera.eye.y-WATER_Y);
    p->oow=p->tmuvtx[0].oow=inv;p->tmuvtx[0].sow=v->u*inv;p->tmuvtx[0].tow=v->v*inv;p->a=255;
    if(p->x!=p->x || p->y!=p->y || p->x<-.1f || p->x>640.1f || p->y<87.9f || p->y>432.1f || p->ooz<0)finite_error=1;
}
static void reflection_init(void)
{
    int group,i,j;reflect_count=0;
    for(group=0;group<fog_group_count;++group) {
        FogGroup *g=&fog_groups[group];
        if(g->lod==1)continue;
        for(i=g->first;i<g->last;++i) {
            ReflectSurface s;Vec lo=vec(1e20f,1e20f,1e20f),hi=vec(-1e20f,-1e20f,-1e20f),d;
            if(i<fog_nave_faces && (i<fog_facade_first || i>=fog_facade_last))continue;
            if(quad_kind[i]==2)continue;
            s.face=i;s.count=quad_kind[i]==1?4:3;
            for(j=0;j<s.count;++j) {
                Vec p;int id=s.count==4?quad_vertices[i][j]:faces[i].v[j];s.ids[j]=id;p=vertices[id].p;
                if(p.x<lo.x)lo.x=p.x;if(p.x>hi.x)hi.x=p.x;if(p.y<lo.y)lo.y=p.y;if(p.y>hi.y)hi.y=p.y;if(p.z<lo.z)lo.z=p.z;if(p.z>hi.z)hi.z=p.z;
            }
            if(hi.y<=WATER_Y+.010f || hi.z< -48 || lo.z> -13.9f || hi.x< -23 || lo.x>13)continue;
            /* Exclude the long side walls/roof and ground; the west facade
               and nearby scenery are the bounded reflection scene. */
            if(hi.z-lo.z>20 || hi.x-lo.x>25)continue;
            /* Thin grave slabs have no modelled underside: reflect both sides. */
            s.two_sided=hi.y<.08f && hi.y-lo.y<.001f && faces[i].mat==STONE;
            s.center=mul(add(lo,hi),.5f);d=sub(hi,lo);s.radius=.5f*(float)sqrt(dot(d,d))+.002f;
            if(reflect_count==REFLECT_MAX){finite_error=1;return;}reflect_mesh[reflect_count++]=s;
        }
    }
}
static int reflection_prepare(void)
{
    ReflectVertex water[24];Vec center=vec(-5.8f,WATER_Y,-28);int i,j;unsigned int mask=0,common=63;
    reflect_visible=reflect_polygons=reflect_triangles=water_count=0;
    if(current_camera.eye.y<=WATER_Y+.1f)return 0;
    reflect_forward=unit(sub(current_camera.look,current_camera.eye));
    {
        Vec delta=sub(center,current_camera.eye);float depth=PROJECT_DOT(delta,reflect_forward);
        if(depth+4.1f<.45f || depth-4.1f>80)return 0;
    }
    if(fog_culling_enabled && fog_inside) {
        int portal,visible=0;
        /* Reject the actual flat footprint against room apertures, rather
           than using its much looser bounding sphere through solid walls. */
        for(portal=0;portal<fog_portal_count && !visible;++portal) {
            for(j=0;j<4;++j) {
                Vec n=fog_portals[portal].normal[j];
                for(i=0;i<8;++i) {
                    Vec d=vec(water_outline[i][0]-current_camera.eye.x,WATER_Y-current_camera.eye.y,water_outline[i][1]-current_camera.eye.z);
                    if(PROJECT_DOT(n,d)>=0)break;
                }
                if(i==8)break;
            }
            if(j==4)visible=1;
        }
        if(!visible)return 0;
    }
    reflect_right=unit(cross(reflect_forward,vec(0,1,0)));reflect_up=cross(reflect_right,reflect_forward);
    reflect_planes[0].n=vec(0,0,1);reflect_planes[0].d=-.45f;
    reflect_planes[1].n=vec(0,0,-1);reflect_planes[1].d=80;
    reflect_planes[2].n=unit(vec(1,0,frustum_x));reflect_planes[3].n=unit(vec(-1,0,frustum_x));
    reflect_planes[4].n=unit(vec(0,1,172/focal));reflect_planes[5].n=unit(vec(0,-1,172/focal));
    for(i=2;i<6;++i)reflect_planes[i].d=0;
    for(i=0;i<8;++i) {
        Vec a=sub(vec(water_outline[i][0],WATER_Y,water_outline[i][1]),current_camera.eye);
        Vec b=sub(vec(water_outline[(i+1)%8][0],WATER_Y,water_outline[(i+1)%8][1]),current_camera.eye);
        Vec n=unit(cross(a,b));unsigned int code=0;
        if(dot(n,sub(center,current_camera.eye))<0)n=mul(n,-1);
        reflect_planes[i+6].n=vec(dot(n,reflect_right),dot(n,reflect_up),dot(n,reflect_forward));reflect_planes[i+6].d=0;
        water[i]=reflect_view(vec(water_outline[i][0],WATER_Y,water_outline[i][1]),0,0);
        for(j=0;j<6;++j)if(reflect_distance(&water[i],j)<0)code|=1U<<j;
        mask|=code;common&=code;
    }
    if(common)return 0;
    water_count=reflect_clip(water,8,6,mask);if(water_count<3)return 0;
    for(i=0;i<water_count;++i)reflect_project(&water[i],&water_polygon[i]);
    reflect_planes[14].n=vec(-reflect_right.y,-reflect_up.y,-reflect_forward.y);reflect_planes[14].d=WATER_Y-.01f-current_camera.eye.y;
    return 1;
}
static void reflect_surface(int index)
{
    ReflectSurface *s=&reflect_mesh[index];ReflectVertex a[24];GrVertex p[24];int i,j,count=s->count;unsigned int mask=0,common=32767;
    for(i=0;i<count;++i) {
        int id=s->ids[i];unsigned int code=0;
        if(reflect_stamps[id]!=reflect_frame) {
            Vertex *v=&vertices[id];Vec pos=v->p;pos.y=2*WATER_Y-pos.y;
            reflect_vertices[id]=reflect_view(pos,v->u,v->v);reflect_stamps[id]=reflect_frame;
        }
        a[i]=reflect_vertices[id];
        for(j=0;j<15;++j)if(reflect_distance(&a[i],j)<0)code|=1U<<j;
        mask|=code;common&=code;
    }
    if(common)return;
    count=reflect_clip(a,count,15,mask);if(count<3)return;
    for(i=0;i<count;++i)reflect_project(&a[i],&p[i]);
    grDrawPolygonVertexList(count,p);++reflect_polygons;reflect_triangles+=count-2;
}
#ifdef WATER_DEMO
static void reflect_plant(int index)
{
    EffectQuad *q=&foliage_cards[index];ReflectVertex a[24];GrVertex p[24];
    int i,j,count=4;unsigned int mask=0,common=32767;
    for(i=0;i<4;++i) {
        Vec pos=q->p[i];unsigned int code=0;pos.y=2*WATER_Y-pos.y;
        a[i]=reflect_view(pos,(i==1||i==2)?255:0,i<2?255:0);
        for(j=0;j<15;++j)if(reflect_distance(&a[i],j)<0)code|=1U<<j;
        mask|=code;common&=code;
    }
    if(common)return;count=reflect_clip(a,count,15,mask);if(count<3)return;
    for(i=0;i<count;++i){reflect_project(&a[i],&p[i]);p[i].a=q->alpha;}
    grDrawPolygonVertexList(count,p);++reflect_plant_polygons;
    if(q->texture==1)++reflect_grass_polygons;
    ++reflect_polygons;reflect_triangles+=count-2;
}
#endif
static void reflection_state(void)
{
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXFALSE);grDepthBiasLevel(1);
    grHints(GR_HINT_STWHINT,0);grFogMode(GR_FOG_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);local_color();
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static void reflection_draw(void)
{
    static const int source[MATERIALS]={0,1,1,4,5,6,0,1,7};int i,j,last=-1;Vec virtual_eye;
#ifdef WATER_DEMO
    reflect_grass_polygons=reflect_plant_polygons=0;
#endif
    if(!reflection_prepare())return;
    reflection_state();
    for(i=0;i<water_count;++i){water_polygon[i].r=38;water_polygon[i].g=53;water_polygon[i].b=65;}
    grDrawPolygonVertexList(water_count,water_polygon);
    if(reflections_enabled) {
        if(++reflect_frame==0){memset(reflect_stamps,0,sizeof(reflect_stamps));reflect_frame=1;}
        virtual_eye=current_camera.eye;virtual_eye.y=2*WATER_Y-virtual_eye.y;
        for(i=0;i<reflect_count;++i) {
            ReflectSurface *s=&reflect_mesh[i];Face *f=&faces[s->face];Vec center=s->center;ReflectVertex v;int outside=0;
            if(s->two_sided && !reflect_low_enabled)continue;
            if(!s->two_sided && PROJECT_DOT(f->normal,virtual_eye)<=f->plane)continue;
            center.y=2*WATER_Y-center.y;v=reflect_view(center,0,0);
            for(j=0;j<15;++j)if(reflect_distance(&v,j)<-s->radius){outside=1;break;}
            if(outside)continue;
            j=reflect_visible++;while(j>0 && reflect_sort[j-1]<v.z){reflect_sort[j]=reflect_sort[j-1];reflect_order[j]=reflect_order[j-1];--j;}
            reflect_sort[j]=v.z;reflect_order[j]=i;
        }
#ifdef WATER_DEMO
        if(reflect_vegetation_enabled) {
            foliage_mirrored=1;foliage_collect();foliage_mirrored=0;
            for(i=0;i<foliage_card_count;++i) {
                j=reflect_visible++;
                while(j>0 && reflect_sort[j-1]<foliage_cards[i].depth){reflect_sort[j]=reflect_sort[j-1];reflect_order[j]=reflect_order[j-1];--j;}
                reflect_sort[j]=foliage_cards[i].depth;reflect_order[j]=-1-i;
            }
        }
#endif
        grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
        grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
        grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_TEXTURE,FXFALSE);fog_apply(0);
        grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
        for(i=0;i<reflect_visible;++i) {
            Face *f;Vertex *v;int texture;
#ifdef WATER_DEMO
            if(reflect_order[i]<0) {
                EffectQuad *q=&foliage_cards[-1-reflect_order[i]];int kind=q->texture;
                if(last<8) {
                    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
                    grAlphaTestReferenceValue(127);grAlphaTestFunction(foliage_mode==0?GR_CMP_GREATER:GR_CMP_ALWAYS);
                    grAlphaBlendFunction(foliage_mode==1?GR_BLEND_SRC_ALPHA:GR_BLEND_ONE,foliage_mode==1?GR_BLEND_ONE_MINUS_SRC_ALPHA:GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
                }
                if(last!=8+kind){grTexSource(GR_TMU0,foliage_address[kind],GR_MIPMAPLEVELMASK_BOTH,&foliage_texture[kind]);last=8+kind;}
                grConstantColorValue(((FxU32)q->alpha<<24)|(FxU32)q->r|((FxU32)q->g<<8)|((FxU32)q->b<<16));
                reflect_plant(-1-reflect_order[i]);continue;
            }
            if(last>=8 || i==0){grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);grAlphaTestReferenceValue(0);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);}
#endif
            f=&faces[reflect_mesh[reflect_order[i]].face];v=&vertices[f->v[0]];texture=source[f->mat];
            if(texture!=last){grTexSource(GR_TMU0,address[texture],GR_MIPMAPLEVELMASK_BOTH,&tex[texture]);last=texture;}
            grConstantColorValue(0xff000000UL|(FxU32)clamp(v->r,0,255)|((FxU32)clamp(v->g,0,255)<<8)|((FxU32)clamp(v->b,0,255)<<16));
            grAlphaTestFunction(f->mat==WINDOW || f->mat==DOOR?GR_CMP_GREATER:GR_CMP_ALWAYS);reflect_surface(reflect_order[i]);
        }
    }
    grFogMode(GR_FOG_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);local_color();
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
    for(i=0;i<water_count;++i) {
        float tint=85;
#ifdef WATER_DEMO
        float nx=(water_polygon[i].x-320)/focal,ny=(260-water_polygon[i].y)/focal;
        float cosine=clamp(-(reflect_forward.y+reflect_right.y*nx+reflect_up.y*ny)/(float)sqrt(1+nx*nx+ny*ny),0,1);
        float edge=1-cosine,fresnel=.10f+.90f*edge*edge*edge*edge*edge;
        tint=160*(1-fresnel);
#endif
        water_polygon[i].r=23;water_polygon[i].g=39;water_polygon[i].b=47;water_polygon[i].a=tint;
    }
    grDrawPolygonVertexList(water_count,water_polygon);grDepthBiasLevel(0);
}
