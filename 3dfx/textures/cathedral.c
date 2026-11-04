/* Native 32-bit DOS / Glide: Winchester Cathedral texture perspective lab. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <dos.h>
#include <glide.h>
#include "model.h"
#include "../helmet/profiler.h"
#ifndef MATERIAL_ROOT
#define MATERIAL_ROOT "assets/"
#endif
#ifndef DEMO_LOG
#define DEMO_LOG "CATHEDRL.LOG"
#endif
#ifndef DEMO_NAME
#define DEMO_NAME "Winchester Cathedral, textures"
#endif
#if defined(MIPMAP_DEMO) || defined(BLEND_DEMO)
#define TEXTURE_W(inv,mode) (inv)
#else
#define TEXTURE_W(inv,mode) ((mode)?1:(inv))
#endif
static float yaw=-.6f,pitch=-.12f,zoom=1,target_yaw=-.6f,target_pitch=-.12f,target_zoom=1;
static float yaw_speed,pitch_speed,zoom_speed,drag_yaw_speed,drag_pitch_speed,inertia_yaw,inertia_pitch;
static int previous_buttons,mouse_present,wheel_present;
static void fly_mode(int enabled);
#ifdef BLEND_DEMO
static void camera_home(void);
#endif
#include "controls.h"
#ifdef FOG_DEMO
#include "../fog/camera.h"
#elif defined(BLEND_DEMO)
#include "../blending/camera.h"
#else
#include "camera.h"
#endif
static GrTexInfo tex[8],profiler_tex;
static FxU32 address[8],profiler_address[2];
static unsigned short profiler_pixels[128*256];
static unsigned char panel_glyph[96][7];
static FrameProfiler panel_snapshot;
static int profiler_dirty=1,profiler_front,profiler_upload,profiler_row,profiler_build;
static float profiler_refresh;
static int affine,compare,checker,finite_error,drawn,clipped,test_frames;
static FxU32 texture_bytes;
static float focal=520,frustum_x=320.0f/520;
#ifdef LIGHTMAP_DEMO
static const char *stage_names[P_STAGES]={"Input","Backdrop","Transform/cull","Opaque","Blending","Lightmaps","HUD","Profiler","Present/wait"};
#elif defined(BLEND_DEMO)
static const char *stage_names[P_STAGES]={"Input","Backdrop","Transform/cull","Opaque","Blending","State","HUD","Profiler","Present/wait"};
#else
static const char *stage_names[P_STAGES]={"Input","Sky","Transform/cull","Scene","Comparison","Extra","HUD","Profiler","Present/wait"};
#endif
static void local_color(void)
{
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
}
#include "overlay.h"
#ifdef BLEND_DEMO
#include "../blending/appearance.h"
#endif
#ifdef FOG_DEMO
#include "../fog/fog.h"
#endif
#ifdef LIGHTMAP_DEMO
#include "../lightmap/lightmap.h"
#endif
#ifdef MIPMAP_DEMO
#include "../mipmaps/mipmap.h"
#endif
static int load_textures(void)
{
    static const char *paths[8]={MATERIAL_ROOT "STONE.RGB",MATERIAL_ROOT "ROOF.RGB",MATERIAL_ROOT "GRASS.RGB",NULL,MATERIAL_ROOT "PAVING.RGB",NULL,NULL,NULL};
    unsigned char *rgb=malloc(256UL*256*3),*font=calloc(8192,1);
    unsigned short *pixels=malloc(256UL*256*2);FxU32 next=grTexMinAddress(GR_TMU0);int t,i,x,y;
#ifdef FOG_DEMO
    paths[1]=MATERIAL_ROOT "GRASS.RGB";
#endif
    if(!rgb || !font || !pixels){free(rgb);free(font);free(pixels);return 0;}
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(glyphs[i].row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    for(t=0;t<8;++t) {
        FxU32 bytes;
        if(paths[t]) {
            FILE *f=fopen(paths[t],"rb");if(!f){free(rgb);free(font);free(pixels);return 0;}
            i=fread(rgb,1,256UL*256*3,f)==256UL*256*3;fclose(f);if(!i){free(rgb);free(font);free(pixels);return 0;}
        }
        if(t!=3)for(y=0;y<256;++y)for(x=0;x<256;++x) {
            int r,g,b;unsigned short alpha=0x8000;
            i=y*256+x;
#ifdef BLEND_DEMO
            if(t==2 || t==5 || t==6){pixels[i]=blend_texel(t,x,y);continue;}
            if(t==7){((unsigned char *)pixels)[i]=soft_alpha(x,y);continue;}
#endif
            if(paths[t]) {
                r=rgb[i*3];g=rgb[i*3+1];b=rgb[i*3+2];
                if(t==2){r=(r+3*89)/4;g=(g+3*110)/4;b=(b+3*70)/4;}
                if(t==1){r=(r+110)/2;g=(g+118)/2;b=(b+124)/2;}
            }
            else if(t==5 || t==6) {
                float u=x/255.0f,v=1-y/255.0f,d=(float)fabs(u-.5f)+.5f;
                float top=.54f+.46f*(float)sqrt(1-d*d)/.8660254f;
                int rim=u<.045f || u>.955f || v<.035f || v>top-.045f;
                int noise=(x*37+y*19+(x*y)%71)%13;
                if(v>top || u<.008f || u>.992f){alpha=0;r=g=b=0;}
                else if(rim){r=181+noise;g=177+noise;b=160+noise;}
                else if(t==6) {
                    int seam=(x%24<2) || abs(x-128)<2;
                    r=seam?26:65+noise;g=seam?25:49+noise;b=seam?23:34+noise;
                } else {
                    int bar=abs(x-128)<3 || (x%51<3) || abs(y-157)<3 || abs(y-99)<3;
                    if(v>.65f && x%51<7)bar=1;
                    if(bar){r=173+noise;g=172+noise;b=157+noise;}
                    else {r=26+noise;g=38+noise;b=42+noise;}
                }
            } else {int bright=((x/32)^(y/32))&1;r=bright?206:65;g=bright?197:82;b=bright?163:89;}
            pixels[i]=(t==5||t==6)?(unsigned short)(alpha|((r>>3)<<10)|((g>>3)<<5)|(b>>3)):
                (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
        }
        tex[t].smallLod=tex[t].largeLod=GR_LOD_256;tex[t].aspectRatio=t==3?GR_ASPECT_8x1:GR_ASPECT_1x1;
        tex[t].format=t==3?GR_TEXFMT_ALPHA_8:(t==5||t==6)?GR_TEXFMT_ARGB_1555:GR_TEXFMT_RGB_565;
#ifdef BLEND_DEMO
        if(t==2 || t==5 || t==6)tex[t].format=GR_TEXFMT_ARGB_4444;
        if(t==7)tex[t].format=GR_TEXFMT_ALPHA_8;
#endif
        tex[t].data=t==3?(void *)font:(void *)pixels;
#ifdef MIPMAP_DEMO
        if(t!=3 && t!=5 && t!=6) {
            if(!mip_download(t,pixels,&next)){free(rgb);free(font);free(pixels);return 0;}
            continue;
        }
#endif
        bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&tex[t]);
        if(next+bytes>grTexMaxAddress(GR_TMU0)+1){free(rgb);free(font);free(pixels);return 0;}
        address[t]=next;grTexDownloadMipMap(GR_TMU0,next,GR_MIPMAPLEVELMASK_BOTH,&tex[t]);next+=bytes;tex[t].data=NULL;
    }
#ifdef MIPMAP_DEMO
    if(!mip_debug_download(pixels,&next)){free(rgb);free(font);free(pixels);return 0;}
    grHints(GR_HINT_ALLOW_MIPMAP_DITHER,1);
#endif
#ifdef LIGHTMAP_DEMO
    if(!lightmap_load(&next,pixels)){free(rgb);free(font);free(pixels);return 0;}
#endif
    profiler_tex.smallLod=profiler_tex.largeLod=GR_LOD_256;profiler_tex.aspectRatio=GR_ASPECT_1x2;
    profiler_tex.format=GR_TEXFMT_RGB_565;profiler_tex.data=profiler_pixels;
    for(t=0;t<2;++t) {
        FxU32 bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&profiler_tex);
        if(next+bytes>grTexMaxAddress(GR_TMU0)+1){free(rgb);free(font);free(pixels);return 0;}
        profiler_address[t]=next;grTexDownloadMipMap(GR_TMU0,next,GR_MIPMAPLEVELMASK_BOTH,&profiler_tex);next+=bytes;
    }
    texture_bytes=next-grTexMinAddress(GR_TMU0);
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);
    /* SDK test29: keep texture reciprocal W separate from depth reciprocal W.
       TMU oow=1 with raw s,t gives affine interpolation; depth is unchanged. */
#ifdef FOG_DEMO
    /* Fog and TMU0 use identical reciprocal depth in this perspective scene. */
    grHints(GR_HINT_STWHINT,0);
#else
    grHints(GR_HINT_STWHINT,GR_STWHINT_W_DIFF_TMU0);
#endif
    free(rgb);free(font);free(pixels);return 1;
}
typedef struct { float x,y,z,u,v,r,g,b;unsigned char code; } ViewVertex;
static ViewVertex view[MAX_VERTS];
static GrVertex projected[2][MAX_VERTS];
static int visible[MATERIALS][MAX_FACES],visible_count[MATERIALS];
static unsigned char used[MAX_VERTS];
#ifdef FOG_DEMO
static int active_vertices[MAX_VERTS],active_count;
#define MARK_ACTIVE(v) do {int index=(v);if(!used[index]){used[index]=1;active_vertices[active_count++]=index;}} while(0)
#include "../fog/culling.h"
#include "../fog/flat.h"
#endif
static float plane_distance(ViewVertex *v,int plane)
{
    switch(plane) {
    case 0:return v->z-.45f;case 1:return 160-v->z;
    case 2:return v->x+v->z*frustum_x;case 3:return v->z*frustum_x-v->x;
    case 4:return v->y+v->z*(172/focal);default:return v->z*(172/focal)-v->y;
    }
}
static void project(ViewVertex *s,GrVertex *v,int affine_mode,float center)
{
    float inv=1/s->z,w=TEXTURE_W(inv,affine_mode);
    v->x=center+s->x*focal*inv;v->y=260-s->y*focal*inv;
    v->oow=inv;v->ooz=65535*.45f*inv;v->a=255;
    v->r=s->r;v->g=s->g;v->b=s->b;
    v->tmuvtx[0].sow=s->u*w;v->tmuvtx[0].tow=s->v*w;v->tmuvtx[0].oow=w;
    if(v->x!=v->x || v->y!=v->y || fabs(v->x)>32760 || fabs(v->y)>32760)finite_error=1;
}
#ifdef FOG_DEMO
#include "../fog/transform.h"
#endif
static void prepare(void)
{
    Vec forward=unit(sub(current_camera.look,current_camera.eye)),right=unit(cross(forward,vec(0,1,0))),up=cross(right,forward);
    float ex=current_camera.eye.x,ey=current_camera.eye.y,ez=current_camera.eye.z;
    float bx,by;int i;
#ifdef FOG_DEMO
    float rx,ry;int group=0;
#endif
    focal=compare?400:520;bx=(compare?160.0f:320.0f)/focal;frustum_x=bx;by=172/focal;
#ifdef FOG_DEMO
    rx=(float)sqrt(1+bx*bx);ry=(float)sqrt(1+by*by);
    fog_portal_prepare(forward,right,up,bx,by,rx,ry);
#endif
    memset(visible_count,0,sizeof(visible_count));memset(used,0,vertex_count);
#ifdef FOG_DEMO
    active_count=0;
    for(i=0;i<flat_count;++i)flat_head[i]=-1;
#endif
    for(i=0;i<face_count;++i) {
        Face *f=&faces[i];
#ifdef FOG_DEMO
        if(i==fog_groups[group].first) {
            int end=fog_groups[group].last,visible_group=fog_group_visible(group,forward,right,up,bx,by,rx,ry);++group;
            if(!visible_group){i=end-1;continue;}
        }
#endif
        if(f->normal.x*ex+f->normal.y*ey+f->normal.z*ez>f->plane) {
#ifdef FOG_DEMO
            flat_next[i]=flat_head[flat_id[i]];flat_head[flat_id[i]]=i;
#else
            visible[f->mat][visible_count[f->mat]++]=i;
#endif
#ifdef FOG_DEMO
            MARK_ACTIVE(f->v[0]);MARK_ACTIVE(f->v[1]);MARK_ACTIVE(f->v[2]);
#else
            used[f->v[0]]=used[f->v[1]]=used[f->v[2]]=1;
#endif
        }
    }
#ifdef FOG_DEMO
    for(i=0;i<flat_count;++i) {
        int face=flat_head[i],mat=flat_material[i];
        while(face>=0){visible[mat][visible_count[mat]++]=face;face=flat_next[face];}
    }
#endif
#ifdef FOG_DEMO
    position_transform(forward,right,up,bx,by);
#else
    for(i=0;i<vertex_count;++i) {
        Vertex *s=&vertices[i];ViewVertex *v=&view[i];
        float x,y,z,xx,yy,zz,xb,yb;
        unsigned char code=0;
        if(!used[i])continue;
        x=s->p.x-ex;y=s->p.y-ey;z=s->p.z-ez;
        xx=x*right.x+y*right.y+z*right.z;
        yy=x*up.x+y*up.y+z*up.z;
        zz=x*forward.x+y*forward.y+z*forward.z;
        v->x=xx;v->y=yy;v->z=zz;xb=zz*bx;yb=zz*by;
        if(zz<.45f)code|=1;if(zz>160)code|=2;
        if(xx < -xb)code|=4;if(xx>xb)code|=8;if(yy < -yb)code|=16;if(yy>yb)code|=32;v->code=code;
        if(!code) {
            GrVertex *p=&projected[0][i];float inv=1/zz,w=TEXTURE_W(inv,!compare && affine);
            p->x=(compare?160:320)+xx*focal*inv;p->y=260-yy*focal*inv;
            p->oow=inv;p->ooz=65535*.45f*inv;
            p->tmuvtx[0].sow=s->u*w;p->tmuvtx[0].tow=s->v*w;p->tmuvtx[0].oow=w;
            if(compare) {
                GrVertex *q=&projected[1][i];q->x=p->x+320;q->y=p->y;q->oow=inv;q->ooz=p->ooz;
#if defined(MIPMAP_DEMO) || defined(BLEND_DEMO)
                q->tmuvtx[0].sow=p->tmuvtx[0].sow;q->tmuvtx[0].tow=p->tmuvtx[0].tow;q->tmuvtx[0].oow=inv;
#else
                q->tmuvtx[0].sow=s->u;q->tmuvtx[0].tow=s->v;q->tmuvtx[0].oow=1;
#endif
            }
        }
    }
#endif
}
static void prepare_constants(void)
{
    int i,j;
    for(i=0;i<vertex_count;++i) {
        Vertex *s=&vertices[i];ViewVertex *v=&view[i];
        v->u=s->u;v->v=s->v;v->r=s->r;v->g=s->g;v->b=s->b;
        for(j=0;j<2;++j) {GrVertex *p=&projected[j][i];p->a=255;p->r=s->r;p->g=s->g;p->b=s->b;}
    }
}

static void interpolate(const ViewVertex *a,const ViewVertex *b,float t,ViewVertex *r)
{
    r->x=a->x+(b->x-a->x)*t;r->y=a->y+(b->y-a->y)*t;r->z=a->z+(b->z-a->z)*t;
    r->u=a->u+(b->u-a->u)*t;r->v=a->v+(b->v-a->v)*t;
    r->r=a->r+(b->r-a->r)*t;r->g=a->g+(b->g-a->g)*t;r->b=a->b+(b->b-a->b)*t;r->code=0;
}
#ifdef FOG_DEMO
#include "../fog/quad.h"
#endif
static void clipped_triangle(Face *f,int pass)
{
    ViewVertex a[12],b[12];GrVertex q[12];int count=3,plane,i,j;unsigned char mask=0;
    for(i=0;i<3;++i){a[i]=view[f->v[i]];mask|=a[i].code;}
    for(plane=0;plane<6;++plane) {
        int n=0;
        /* A convex interpolation cannot leave a halfspace containing all
           three source vertices. Skip those planes entirely. */
        if(!(mask&(1<<plane)))continue;
        for(i=0;i<count;++i) {
            ViewVertex *p=&a[i],*r=&a[(i+1)%count];float dp=plane_distance(p,plane),dr=plane_distance(r,plane);
            if(dp>=0)b[n++]=*p;
            if((dp>=0)!=(dr>=0))interpolate(p,r,dp/(dp-dr),&b[n++]);
        }
        if(n<3)return;count=n;for(i=0;i<count;++i)a[i]=b[i];
    }
    for(i=0;i<count;++i)project(&a[i],&q[i],compare?pass:affine,compare?(pass?480:160):320);
    for(j=1;j<count-1;++j){
        grDrawTriangle(&q[0],&q[j],&q[j+1]);++drawn;
#ifdef LIGHTMAP_DEMO
        lightmap_record(f,&q[0],&q[j],&q[j+1]);
#endif
    }
    ++clipped;
}
static void scene(int pass)
{
#ifdef FOG_DEMO
    static const int source[MATERIALS]={0,1,1,4,5,6,0,1,7};int mat,i;
#else
    static const int source[MATERIALS]={0,1,2,4,5,6,0,2,7};int mat,i;
#endif
#ifdef FOG_DEMO
    FxU32 last_color=0;
#endif
    grClipWindow(compare?(pass?320:0):0,88,compare?(pass?640:320):640,432);
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXTRUE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
#ifdef FOG_DEMO
    if(flat_enabled)grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
#endif
    for(mat=0;mat<MATERIALS;++mat) {
        int cutout=mat==WINDOW || mat==DOOR;
#ifdef LIGHTMAP_DEMO
        int last_receiver=-1;
#endif
        int texture=checker && (mat==STONE || mat==ROOF || mat==PAVING || mat==TRIM)?7:source[mat];
#ifdef MIPMAP_DEMO
        mip_source(texture,pass);
#else
        grTexSource(GR_TMU0,address[texture],GR_MIPMAPLEVELMASK_BOTH,&tex[texture]);
#endif
        /* Keep ground/paving and surface-detail cards stable in 16-bit Z. */
        grDepthBiasLevel(mat==GRASS?-4:mat==PAVING?-2:cutout?2:0);
        grTexClampMode(GR_TMU0,cutout?GR_TEXTURECLAMP_CLAMP:GR_TEXTURECLAMP_WRAP,cutout?GR_TEXTURECLAMP_CLAMP:GR_TEXTURECLAMP_WRAP);
        grAlphaTestFunction(cutout?GR_CMP_GREATER:GR_CMP_ALWAYS);grAlphaTestReferenceValue(127);
        grAlphaCombine(cutout?GR_COMBINE_FUNCTION_SCALE_OTHER:GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_ONE,
            cutout?GR_COMBINE_LOCAL_NONE:GR_COMBINE_LOCAL_ITERATED,cutout?GR_COMBINE_OTHER_TEXTURE:GR_COMBINE_OTHER_NONE,FXFALSE);
        for(i=0;i<visible_count[mat];++i) {
            Face *f=&faces[visible[mat][i]];unsigned char a=view[f->v[0]].code,b=view[f->v[1]].code,c=view[f->v[2]].code;
#ifdef FOG_DEMO
            if(quad_enabled && quad_kind[f-faces]==2)continue;
#endif
#ifdef FOG_DEMO
            if((!quad_enabled || quad_kind[f-faces]!=1) && (a&b&c))continue;
#else
            if(a&b&c)continue;
#endif
#ifdef LIGHTMAP_DEMO
            {
                int receiver=lightmaps_enabled && lightmap_face[f-faces];
                if(receiver!=last_receiver){fog_apply(receiver);last_receiver=receiver;}
            }
#endif
#ifdef FOG_DEMO
            if(flat_enabled && flat_color[f-faces]!=last_color){last_color=flat_color[f-faces];grConstantColorValue(last_color);}
#endif
#ifdef FOG_DEMO
            if(quad_enabled && quad_kind[f-faces]==1){quad_draw(f,quad_vertices[f-faces]);continue;}
#endif
            if(a|b|c)clipped_triangle(f,pass);
            else {
                grDrawTriangle(&projected[pass][f->v[0]],&projected[pass][f->v[1]],&projected[pass][f->v[2]]);++drawn;
#ifdef LIGHTMAP_DEMO
                lightmap_record(f,&projected[pass][f->v[0]],&projected[pass][f->v[1]],&projected[pass][f->v[2]]);
#endif
            }
        }
    }
}
#ifdef BLEND_DEMO
#include "../blending/effects.h"
#endif
static void render(int fps,int paused)
{
    char line[80];GrVertex sky[4];int i;
    if(!profiler.active){profile_begin();profile_stage(P_INPUT);}
#ifdef FOG_DEMO
    grFogMode(GR_FOG_DISABLE);
#endif
    grClipWindow(0,0,640,480);grDepthMask(FXTRUE);grBufferClear(0x0018140d,0,0);
    background();
    memset(sky,0,sizeof(sky));
    for(i=0;i<4;++i){sky[i].x=(i==1||i==2)?640:0;sky[i].y=i<2?88:432;sky[i].a=255;sky[i].r=i<2?78:184;sky[i].g=i<2?111:197;sky[i].b=i<2?139:200;}
#ifdef BLEND_DEMO
    for(i=0;i<4;++i){sky[i].r=i<2?10:30;sky[i].g=i<2?16:39;sky[i].b=i<2?27:58;}
#endif
#ifdef FOG_DEMO
    fog_sky(sky);
#endif
    grDrawTriangle(&sky[0],&sky[1],&sky[2]);grDrawTriangle(&sky[0],&sky[2],&sky[3]);
    profile_stage(P_BACKGROUND);
    drawn=clipped=0;prepare();profile_stage(P_TRANSFORM);
#ifdef FOG_DEMO
    fog_apply(0);
#endif
#ifdef LIGHTMAP_DEMO
    lightmap_count=0;
#endif
    scene(0);profile_stage(P_OPAQUE);
#ifdef LIGHTMAP_DEMO
    lightmap_draw();profile_stage(P_SPECULAR);
#endif
#ifdef BLEND_DEMO
    blending_effects(paused);
#else
    if(compare)scene(1);
#endif
    profile_stage(P_AA);
#ifndef BLEND_DEMO
    profile_stage(P_SPECULAR);
#endif
#ifdef FOG_DEMO
    grFogMode(GR_FOG_DISABLE);
#endif
    grClipWindow(0,0,640,480);grDepthBiasLevel(0);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grDepthMask(FXFALSE);grAlphaTestFunction(GR_CMP_ALWAYS);
#ifdef BLEND_DEMO
#ifndef LIGHTMAP_DEMO
    profile_stage(P_SPECULAR);
#endif
#ifdef FOG_DEMO
    fog_hud(fps);
#else
    blending_hud(fps);
#endif
#elif defined(MIPMAP_DEMO)
    mip_hud(fps);
#else
    text(21,19,"3DFX / TEXTURE PERSPECTIVE STUDY",1,176,157,122);
    text(21,39,"WINCHESTER CATHEDRAL",2.6f,233,224,202);
    text(480,20,compare?"SIDE BY SIDE":affine?"AFFINE":"PERSPECTIVE",1,135,209,202);
    text(480,36,flying?"F DRONE FLIGHT":"F ORBIT CAMERA",1,176,188,190);
    if(fps>0)sprintf(line,"FPS %d",fps);else strcpy(line,"FPS --");text(480,53,line,1,176,157,122);
    if(compare){color_quad(319,88,2,344,30,40,40);text(18,96,"PERSPECTIVE",1,245,231,184);text(338,96,"AFFINE",1,245,231,184);}
#endif
    if(paused)text(290,416,"PAUSED",1,240,215,154);
#ifdef LIGHTMAP_DEMO
    text(21,452,"B LIGHTMAPS  H FOG  F JOURNEY  SPACE PAUSE  LMB LOOK  WHEEL DOLLY",1,176,188,190);
    text(21,466,"1/2/3 FOG CURVE  [ ] DENSITY  Q COLOR  TAB PROFILER  ESC EXIT",1,116,140,148);
#elif defined(FOG_DEMO)
    text(21,452,"F JOURNEY  SPACE PAUSE  LMB LOOK  WHEEL/RMB DOLLY  TAB PROFILER",1,176,188,190);
    text(21,466,"H FOG  1 LINEAR  2 EXP  3 EXP2  [ ] DENSITY  Q COLOR  ESC EXIT",1,116,140,148);
#elif defined(BLEND_DEMO)
    text(21,452,"LMB LOOK  WHEEL/RMB DOLLY  ARROWS TURN  F NAVE TOUR  SPACE PAUSE",1,176,188,190);
    text(21,466,"G GLASS  C CANDLES  M MIST  L LIGHT PATCHES  S SORTING  TAB PROFILER  ESC EXIT",1,116,140,148);
#else
    text(21,452,"LMB ORBIT   WHEEL/RMB ZOOM   F DRONE FLIGHT   SPACE PAUSE",1,176,188,190);
#ifdef MIPMAP_DEMO
    text(21,466,"M MIPS C COMPARE G LEVELS [ ] BIAS N DITHER K CHECKER TAB PROFILER ESC EXIT",1,116,140,148);
#else
    text(21,466,"C COMPARE   P PERSPECTIVE/AFFINE   K CHECKER   TAB PROFILER   ESC EXIT",1,116,140,148);
#endif
#endif
    profile_stage(P_HUD);profiler_panel();profile_stage(P_OVERLAY);
    while(grSstVRetraceOn()){}while(!grSstVRetraceOn()){}
    grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();profile_stage(P_PRESENT);profile_end();
}
static unsigned short *capture(const char *path)
{
    unsigned short *p=malloc(640UL*480*2);FILE *f;unsigned long i;
    if(!p || !grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,640,480,1280,p)){free(p);return NULL;}
    f=fopen(path,"wb");if(!f){free(p);return NULL;}fprintf(f,"P6\n640 480\n255\n");
    for(i=0;i<640UL*480;++i){unsigned char c[3];c[0]=((p[i]>>11)&31)*255/31;c[1]=((p[i]>>5)&63)*255/63;c[2]=(p[i]&31)*255/31;fwrite(c,1,3,f);}
    if(fclose(f)){free(p);return NULL;}return p;
}
static void log_profile(FILE *log)
{
    int i;if(!log)return;
    fprintf(log,"Frame %.3f ms, p95 %.3f, max %.3f; guest CPU/API timer\n",profiler.average,profiler.p95,profiler.max);
    for(i=0;i<P_STAGES;++i)fprintf(log,"  %s %.3f ms\n",stage_names[i],profiler.mean[i]);
}
static int rtc_seconds(void)
{ int sec,mode;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);return mode&4?sec:(sec>>4)*10+(sec&15); }
#ifdef FOG_DEMO
#include "../fog/tests.h"
#elif defined(BLEND_DEMO)
#include "../blending/tests.h"
#elif defined(MIPMAP_DEMO)
#include "../mipmaps/tests.h"
#else
#include "tests.h"
#endif
int main(int argc,char **argv)
{
    GrHwConfiguration hw;int key,paused=0,fps=0,frames=0,sample=0,second,synced=0,ok=1;
    int test=argc>1 && !strcmp(argv[1],"/test"),soak=argc>1 && !strcmp(argv[1],"/soak");
    FILE *log=fopen(DEMO_LOG,"w");
    if(argc>1 && !strcmp(argv[1],"/tour"))soak=2;
#ifdef MIPMAP_DEMO
    mip_quick=argc>1 && !strcmp(argv[1],"/check");if(mip_quick)test=1;
    mip_bench=argc>1 && !strcmp(argv[1],"/bench");if(mip_bench)test=1;
#endif
#ifdef BLEND_DEMO
    blend_control=argc>1 && !strcmp(argv[1],"/control");if(blend_control)test=1;
#endif
    if(!build_cathedral()){puts("Mesh capacity error");if(log)fclose(log);return 1;}
#ifdef LIGHTMAP_DEMO
    lightmap_mesh_init();
#endif
#ifdef FOG_DEMO
    position_init();flat_init();
    quad_init();
#endif
    prepare_constants();path_init();current_camera=orbit_pose();transition_from=current_camera;
#ifdef BLEND_DEMO
    flying=1;transition=1;current_camera=drone_pose(0);transition_from=current_camera;
#endif
    if(log){fprintf(log,"Mesh: %d vertices, %d faces\n",vertex_count,face_count);fflush(log);}
    grGlideInit();
    if(!grSstQueryHardware(&hw) || hw.num_sst<1){grGlideShutdown();if(log)fclose(log);return 1;}
    if(log && hw.SSTs[0].type==GR_SSTTYPE_VOODOO)fprintf(log,"Voodoo Graphics: framebuffer %d MB, %d TMU(s), TMU0 %d MB\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam);
    grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){grGlideShutdown();if(log)fclose(log);return 1;}
    grCullMode(GR_CULL_DISABLE);grDitherMode(GR_DITHER_4x4);
    if(!load_textures()){grGlideShutdown();puts("Cannot load material textures / texture memory exhausted");if(log)fclose(log);return 1;}
    profile_init();mouse_init();
#ifdef FOG_DEMO
    mist_enabled=0;fog_update();
#ifdef LIGHTMAP_DEMO
    light_enabled=0;
#endif
#endif
    if(log){fprintf(log,"TMU0 start-address bounds %lu..%lu; textures used %lu bytes; mouse %d wheel %d timer %d\n",(unsigned long)grTexMinAddress(GR_TMU0),(unsigned long)grTexMaxAddress(GR_TMU0),(unsigned long)texture_bytes,mouse_present,wheel_present,profiler.available);fflush(log);}
    if(test || soak){
#ifdef MIPMAP_DEMO
        if(mip_bench)ok=run_mip_benchmark(log);else
#endif
        ok=run_tests(log,soak);frames=test_frames;goto done;
    }
    second=rtc_seconds();
    for(;;) {
        float dt=profiler.available && profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
        profile_begin();
        while(kbhit()) {
            key=getch();if(key==27)goto done;
#ifdef FOG_DEMO
            fog_key(key);
#ifdef LIGHTMAP_DEMO
            if(key=='b'||key=='B')lightmaps_enabled=!lightmaps_enabled;
#endif
#endif
            if(key==9){profile_toggle();profiler_dirty=1;}
            if(key=='f'||key=='F'){fly_mode(!flying);paused=0;}
#ifdef BLEND_DEMO
            if(key=='g'||key=='G')glass_enabled=!glass_enabled;
            if(key=='c'||key=='C')candles_enabled=!candles_enabled;
            if(key=='m'||key=='M')mist_enabled=!mist_enabled;
            if(key=='l'||key=='L')light_enabled=!light_enabled;
            if(key=='s'||key=='S')sorted_blending=!sorted_blending;
#else
            if(key=='c'||key=='C')compare=!compare;
#ifndef MIPMAP_DEMO
            if(key=='p'||key=='P')affine=!affine;
#else
            if(key=='m'||key=='M')mip_enabled=!mip_enabled;
            if(key=='g'||key=='G')mip_colored=!mip_colored;
            if(key=='n'||key=='N')mip_dither=!mip_dither;
            if(key=='[')lod_bias=clamp(lod_bias-.25f,-4,4);
            if(key==']')lod_bias=clamp(lod_bias+.25f,-4,4);
            if(key=='0')lod_bias=.5f;
#endif
            if(key=='k'||key=='K')checker=!checker;
#endif
            if(key==' ')paused=!paused;
            if(key=='r'||key=='R'){fly_mode(0);reset_target();}
            if(key=='+'||key=='='){fly_mode(0);target_zoom=clamp(target_zoom+.05f,.65f,1.3f);}
            if(key=='-'){fly_mode(0);target_zoom=clamp(target_zoom-.05f,.65f,1.3f);}
            if(key==0||key==224) {
                fly_mode(0);inertia_yaw=inertia_pitch=0;key=getch();
                if(key==75)target_yaw-=.08f;if(key==77)target_yaw+=.08f;
                if(key==72)target_pitch=clamp(target_pitch-.05f,-.55f,.45f);if(key==80)target_pitch=clamp(target_pitch+.05f,-.55f,.45f);
            }
        }
        mouse_poll(dt,&paused);
#ifndef BLEND_DEMO
        if(!flying && !paused)target_yaw+=.12f*dt;
#endif
        camera_step(dt);update_camera(dt,paused);profile_stage(P_INPUT);render(fps,paused);++frames;++sample;
        if(finite_error){ok=0;goto done;}
        {int now=rtc_seconds(),elapsed=(now-second+60)%60;if(elapsed){if(synced)fps=sample/elapsed;synced=1;sample=0;second=now;}}
    }
done:
    grGlideShutdown();if(log){fprintf(log,"%s: %s, orbit + drone camera, %d frames\n",ok&&!finite_error?"PASS":"FAIL",DEMO_NAME,frames);fclose(log);}return ok&&!finite_error?0:1;
}
