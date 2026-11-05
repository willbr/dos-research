/* C / DOS / Glide 2.43: a textured diving helmet and silhouette-AA study. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <dos.h>
#include <glide.h>
#include "model.h"
#include "profiler.h"
#ifndef HELMET_MATERIAL_ROOT
#define HELMET_MATERIAL_ROOT "assets/"
#endif
#ifndef HELMET_LOG
#define HELMET_LOG "HELMET.LOG"
#endif

static GrVertex projected[MAX_VERTS];
static int visible_faces[MATERIALS][MAX_FACES],visible_per_mat[MATERIALS];
static int aa_faces[MATERIALS][MAX_FACES],aa_per_mat[MATERIALS];
static float shine[MAX_VERTS];
static unsigned char aa_edges[MAX_FACES];
static GrTexInfo tex[4];
static FxU32 address[4];
static int textured=1,antialias=1,silhouette_count,visible_count;
static float yaw=-.6f,pitch=-.12f,zoom=1;
static float target_yaw=-.6f,target_pitch=-.12f,target_zoom=1;
static float yaw_speed,pitch_speed,zoom_speed;
static float drag_yaw_speed,drag_pitch_speed,inertia_yaw,inertia_pitch;
static int mouse_present,wheel_present,previous_buttons,highlights=1,detailed;
static float material_average[3][3];
static FxU32 profiler_address[2];
static GrTexInfo profiler_tex;
static unsigned short profiler_pixels[128*256];
static unsigned char panel_glyph[96][7];
static int profiler_dirty=1;
static int profiler_front,profiler_upload,profiler_row,profiler_build;
static FrameProfiler panel_snapshot;
static float profiler_refresh;
static const float base[MATERIALS][3]={{151,99,60},{197,170,104},{153,161,158},{11,24,29},{22,19,16}};
static const float specular[MATERIALS]={.68f,.8f,.8f,.22f,.08f};
typedef struct { char c; unsigned char row[7]; } Glyph;
static const Glyph glyphs[]={
 {'A',{14,17,17,31,17,17,17}},{'B',{30,17,17,30,17,17,30}},
 {'C',{14,17,16,16,16,17,14}},{'D',{30,17,17,17,17,17,30}},
 {'E',{31,16,16,30,16,16,31}},{'F',{31,16,16,30,16,16,16}},
 {'G',{14,17,16,23,17,17,15}},{'H',{17,17,17,31,17,17,17}},
 {'I',{14,4,4,4,4,4,14}},{'J',{7,2,2,2,2,18,12}},
 {'K',{17,18,20,24,20,18,17}},{'L',{16,16,16,16,16,16,31}},
 {'M',{17,27,21,21,17,17,17}},{'N',{17,25,21,19,17,17,17}},
 {'O',{14,17,17,17,17,17,14}},{'P',{30,17,17,30,16,16,16}},
 {'Q',{14,17,17,17,21,18,13}},{'R',{30,17,17,30,20,18,17}},
 {'S',{15,16,16,14,1,1,30}},{'T',{31,4,4,4,4,4,4}},
 {'U',{17,17,17,17,17,17,14}},{'V',{17,17,17,17,17,10,4}},
 {'W',{17,17,17,21,21,21,10}},{'X',{17,17,10,4,10,17,17}},
 {'Y',{17,17,10,4,4,4,4}},{'Z',{31,1,2,4,8,16,31}},
 {'0',{14,17,19,21,25,17,14}},{'1',{4,12,4,4,4,4,14}},
 {'2',{14,17,1,2,4,8,31}},{'3',{30,1,1,14,1,1,30}},
 {'4',{2,6,10,18,31,2,2}},{'5',{31,16,16,30,1,1,30}},
 {'6',{14,16,16,30,17,17,14}},{'7',{31,1,2,4,8,8,8}},
 {'8',{14,17,17,14,17,17,14}},{'9',{14,17,17,15,1,1,14}},
 {'-',{0,0,0,31,0,0,0}},{'/',{1,1,2,4,8,16,16}},
 {'.',{0,0,0,0,0,12,12}},{':',{0,12,12,0,12,12,0}},
 {'+',{0,4,4,31,4,4,0}}
};
static float clamp(float x,float low,float high) { return x<low?low:x>high?high:x; }
/* Exact critically damped spring for a fixed target during this time step.
   Position and velocity are continuous when keyboard/mouse targets change. */
static void spring(float *value,float *speed,float target,float dt,float omega)
{
    float decay=(float)exp(-omega*dt),offset=*value-target;
    float impulse=(*speed+omega*offset)*dt;
    *value=target+(offset+impulse)*decay;
    *speed=(*speed-omega*impulse)*decay;
}
static void camera_step(float dt)
{
    if(!(previous_buttons&1)) {
        float decay=(float)exp(-4.5f*dt),travel=(1-decay)/4.5f;
        float next_pitch=target_pitch+inertia_pitch*travel;
        target_yaw+=inertia_yaw*travel;
        target_pitch=clamp(next_pitch,-.55f,.45f);
        if(target_pitch!=next_pitch) inertia_pitch=0;
        inertia_yaw*=decay;
        inertia_pitch*=decay;
        if(fabs(inertia_yaw)<.001f) inertia_yaw=0;
        if(fabs(inertia_pitch)<.001f) inertia_pitch=0;
    }
    spring(&yaw,&yaw_speed,target_yaw,dt,16);
    spring(&pitch,&pitch_speed,target_pitch,dt,16);
    spring(&zoom,&zoom_speed,target_zoom,dt,18);
    pitch=clamp(pitch,-.55f,.45f);zoom=clamp(zoom,.65f,1.3f);
}
static void reset_target(void)
{
    target_yaw=-.6f;target_pitch=-.12f;target_zoom=1;
    inertia_yaw=inertia_pitch=drag_yaw_speed=drag_pitch_speed=0;
}
static void mouse_apply(int dx,int dy,int buttons,int wheel,float dt,int *paused)
{
    if(buttons&1) {
        float blend=1-(float)exp(-25*dt);
        if(!(previous_buttons&1)) {
            target_yaw=yaw;target_pitch=pitch;
            inertia_yaw=inertia_pitch=drag_yaw_speed=drag_pitch_speed=0;
            yaw_speed=pitch_speed=0;
        }
        target_yaw+=dx*.007f;
        target_pitch=clamp(target_pitch+dy*.005f,-.55f,.45f);
        drag_yaw_speed+=(dx*.007f/dt-drag_yaw_speed)*blend;
        drag_pitch_speed+=(dy*.005f/dt-drag_pitch_speed)*blend;
        *paused=1;
    } else if(previous_buttons&1) {
        inertia_yaw=clamp(drag_yaw_speed*.8f,-5,5);
        inertia_pitch=clamp(drag_pitch_speed*.8f,-2,2);
    }
    if(buttons&2) {
        target_zoom=clamp(target_zoom-dy*.004f,.65f,1.3f);*paused=1;
        inertia_yaw=inertia_pitch=0;
    }
    if(wheel) target_zoom=clamp(target_zoom-wheel*.08f,.65f,1.3f);
    if((buttons&4) && !(previous_buttons&4)) { reset_target();*paused=1; }
    previous_buttons=buttons;
}
static void mouse_init(void)
{
    union REGS r;
    memset(&r,0,sizeof(r));int386(0x33,&r,&r);
    mouse_present=r.w.ax==0xffff;
    if(!mouse_present) return;
    memset(&r,0,sizeof(r));r.w.ax=0x11;int386(0x33,&r,&r);
    wheel_present=r.w.ax==0x574d && (r.w.cx&1);
    memset(&r,0,sizeof(r));r.w.ax=7;r.w.dx=639;int386(0x33,&r,&r);
    memset(&r,0,sizeof(r));r.w.ax=8;r.w.dx=479;int386(0x33,&r,&r);
    memset(&r,0,sizeof(r));r.w.ax=0x0b;int386(0x33,&r,&r); /* discard old motion */
}
static void mouse_poll(float dt,int *paused)
{
    union REGS r;
    int buttons,wheel,dx,dy;
    if(!mouse_present) return;
    memset(&r,0,sizeof(r));r.w.ax=3;int386(0x33,&r,&r);
    buttons=r.h.bl&7;wheel=wheel_present?(signed char)r.h.bh:0;
    memset(&r,0,sizeof(r));r.w.ax=0x0b;int386(0x33,&r,&r);
    dx=(short)r.w.cx;dy=(short)r.w.dx;
    mouse_apply(dx,dy,buttons,wheel,dt,paused);
}
static int camera_test(void)
{
    int p=0,i,ok=1;
    float first,second,v=0,s=0;
    yaw=target_yaw=-.6f;pitch=target_pitch=-.12f;zoom=target_zoom=1;
    yaw_speed=pitch_speed=zoom_speed=0;previous_buttons=0;
    mouse_apply(40,40,0,0,1/60.0f,&p);
    if(target_yaw!=-.6f || target_pitch!=-.12f || p) ok=0;
    mouse_apply(20,10,1,0,1/60.0f,&p);
    if(fabs(target_yaw-(-.46f))>.0001f || fabs(target_pitch-(-.07f))>.0001f || !p) ok=0;
    mouse_apply(0,0,0,-1,1/60.0f,&p);
    if(inertia_yaw<=0) ok=0;
    if(fabs(target_zoom-1.08f)>.0001f) ok=0;
    mouse_apply(0,20,2,0,1/60.0f,&p);
    if(fabs(target_zoom-1)>.0001f) ok=0;
    mouse_apply(0,20000,1,-100,1/60.0f,&p);
    if(target_pitch!=.45f || target_zoom!=1.3f) ok=0;
    mouse_apply(0,-20000,1,100,1/60.0f,&p);
    if(target_pitch!=-.55f || target_zoom!=.65f) ok=0;
    spring(&v,&s,1,1/60.0f,16);first=v;
    spring(&v,&s,1,1/60.0f,16);second=v-first;
    if(first<=0 || second<=first || v>=1) ok=0;
    for(i=0;i<120;++i) spring(&v,&s,1,1/60.0f,16);
    if(fabs(v-1)>.0001f || fabs(s)>.0001f) ok=0;
    /* A flick coasts, damping stops it, and grabbing cancels that coast. */
    reset_target();yaw=target_yaw;pitch=target_pitch;previous_buttons=0;
    mouse_apply(12,0,1,0,1/60.0f,&p);
    mouse_apply(0,0,0,0,1/60.0f,&p);
    first=target_yaw;camera_step(1/60.0f);
    if(target_yaw<=first || inertia_yaw<=0) ok=0;
    for(i=0;i<180;++i) camera_step(1/60.0f);
    if(inertia_yaw!=0) ok=0;
    inertia_yaw=2;mouse_apply(0,0,1,0,1/60.0f,&p);
    if(inertia_yaw!=0) ok=0;
    for(i=0;i<30;++i) mouse_apply(0,0,1,0,1/60.0f,&p);
    mouse_apply(0,0,0,0,1/60.0f,&p);
    if(inertia_yaw!=0) ok=0;
    reset_target();yaw=target_yaw;pitch=target_pitch;zoom=target_zoom;
    yaw_speed=pitch_speed=zoom_speed=0;previous_buttons=0;
    return ok;
}
static int rtc_seconds(void)
{
    int sec,mode;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);
    return mode&4?sec:(sec>>4)*10+(sec&15);
}
static void wait_retrace(void)
{ while(grSstVRetraceOn()){} while(!grSstVRetraceOn()){} }
static void local_color(void)
{
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
        GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
        GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
}
#ifdef ENVIRONMENT_DEMO
#include "../envmap/environment.h"
#endif
static int load_textures(void)
{
    unsigned char *rgb=malloc(256UL*256*3),*font=calloc(256UL*32,1);
    unsigned short *pixels=malloc(256UL*256*2);
    FILE *f=fopen(HELMET_MATERIAL_ROOT "COPPER.RGB","rb");
    FxU32 next=grTexMinAddress(GR_TMU0);
    int t,i,x,y;
    if(!rgb || !font || !pixels || !f) { if(f) fclose(f);free(rgb);free(font);free(pixels);return 0; }
    if(fread(rgb,1,256UL*256*3,f)!=256UL*256*3) { fclose(f);free(rgb);free(font);free(pixels);return 0; }
    fclose(f);
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;
        memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y) for(x=0;x<5;++x)
            if(glyphs[i].row[y]&(1<<(4-x))) font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    for(t=0;t<4;++t) {
        FxU32 bytes;
        unsigned long sum[3]={0,0,0};
        if(t<3) for(i=0;i<256*256;++i) {
            int r=rgb[i*3],g=rgb[i*3+1],b=rgb[i*3+2],v=(r+g+b)/3;
            if(t==1) { r=(int)(v*1.85);g=(int)(v*1.6);b=(int)(v*1.04); }
            if(t==2) { r=(int)(v*1.6);g=(int)(v*1.65);b=(int)(v*1.60); }
            if(r>255)r=255;if(g>255)g=255;if(b>255)b=255;
            sum[0]+=r;sum[1]+=g;sum[2]+=b;
            pixels[i]=(unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
        }
        tex[t].smallLod=tex[t].largeLod=GR_LOD_256;
        tex[t].aspectRatio=t==3?GR_ASPECT_8x1:GR_ASPECT_1x1;
        tex[t].format=t==3?GR_TEXFMT_ALPHA_8:GR_TEXFMT_RGB_565;
        tex[t].data=t==3?(void *)font:(void *)pixels;
        bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&tex[t]);
        if(next+bytes>grTexMaxAddress(GR_TMU0)+1) { free(rgb);free(font);free(pixels);return 0; }
        address[t]=next;grTexDownloadMipMap(GR_TMU0,next,GR_MIPMAPLEVELMASK_BOTH,&tex[t]);
        next+=bytes;
        tex[t].data=NULL;
        if(t<3) for(i=0;i<3;++i) material_average[t][i]=sum[i]/65536.0f;
    }
    profiler_address[0]=next;
    profiler_tex.smallLod=profiler_tex.largeLod=GR_LOD_256;
    profiler_tex.aspectRatio=GR_ASPECT_1x2;profiler_tex.format=GR_TEXFMT_RGB_565;
    profiler_tex.data=profiler_pixels;
    profiler_address[1]=next+grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&profiler_tex);
    if(profiler_address[1]+grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&profiler_tex)>grTexMaxAddress(GR_TMU0)+1) {
        free(rgb);free(font);free(pixels);return 0;
    }
    grTexDownloadMipMap(GR_TMU0,profiler_address[0],GR_MIPMAPLEVELMASK_BOTH,&profiler_tex);
    grTexDownloadMipMap(GR_TMU0,profiler_address[1],GR_MIPMAPLEVELMASK_BOTH,&profiler_tex);
#ifdef ENVIRONMENT_DEMO
    next=profiler_address[1]+grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&profiler_tex);
    if(!environment_load(&next,pixels)){free(rgb);free(font);free(pixels);return 0;}
#endif
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
        GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
    free(rgb);free(font);free(pixels);return 1;
}
static void material(int mat)
{
    if(textured && mat<GLASS) {
        grTexSource(GR_TMU0,address[mat],GR_MIPMAPLEVELMASK_BOTH,&tex[mat]);
        grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,
            GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    } else local_color();
}
static int experiment_flags=64; /* Production shared-normal C path. */
static int automatic_lod=1,mesh_error;
static void prepare_experimental(void);
static void finish_prepare(float camx,float camy,float camz)
{
    int i,j,mat;
    memset(visible_per_mat,0,sizeof(visible_per_mat));memset(aa_per_mat,0,sizeof(aa_per_mat));
    visible_count=silhouette_count=0;
    for(i=0;i<face_count;++i) {
        Face *f=&faces[i];
        f->visible=f->normal.x*camx+f->normal.y*camy+f->normal.z*camz>f->plane;
        if(f->visible) { visible_faces[f->mat][visible_per_mat[f->mat]++]=i;++visible_count; }
    }
    for(mat=0;mat<MATERIALS;++mat) for(j=0;j<visible_per_mat[mat];++j) {
        int k;Face *f;i=visible_faces[mat][j];f=&faces[i];aa_edges[i]=0;
        for(k=0;k<3;++k) {
            Edge *e=&edges[f->e[k]];int other=e->face[0]==i?e->face[1]:e->face[0];
            if(other<0 || !faces[other].visible) { aa_edges[i]|=1<<k;++silhouette_count; }
        }
        if(aa_edges[i]) aa_faces[mat][aa_per_mat[mat]++]=i;
    }
}
static void prepare_reference(void)
{
    float sy=(float)sin(yaw),cy=(float)cos(yaw),sp=(float)sin(pitch),cp=(float)cos(pitch);
    float r10=sp*sy,r12=-sp*cy,r20=-cp*sy,r22=cp*cy;
    Vec light=unit(vec(-.6f,.75f,1)),half=unit(add(light,vec(0,0,1)));
    float lx=cy*light.x+r10*light.y+r20*light.z,ly=cp*light.y+sp*light.z,lz=sy*light.x+r12*light.y+r22*light.z;
    float hx=cy*half.x+r10*half.y+r20*half.z,hy=cp*half.y+sp*half.z,hz=sy*half.x+r12*half.y+r22*half.z;
    float camx=r20*6.6f,camy=sp*6.6f+.58f,camz=r22*6.6f,scale=660*zoom;
    int i;
    for(i=0;i<vertex_count;++i) {
        Vertex *s=&vertices[i];GrVertex *v=&projected[i];
        float x=s->p.x,y=s->p.y-.58f,z=s->p.z;
        float inverse=1/(6.6f-(r20*x+sp*y+r22*z));
        float diff=s->n.x*lx+s->n.y*ly+s->n.z*lz;
        float h=s->n.x*hx+s->n.y*hy+s->n.z*hz,h2,h4,h8,h16;
        if(diff<0)diff=0;diff=.34f+.66f*diff;if(h<0)h=0;
        h2=h*h;h4=h2*h2;h8=h4*h4;h16=h8*h8;
        shine[i]=(s->mat==GLASS?h16:h16*h16)*specular[s->mat]*255;
        v->x=320+(cy*x+sy*z)*scale*inverse;v->y=244-(r10*x+cp*y+r12*z)*scale*inverse;
        v->oow=inverse;v->ooz=65535*inverse;v->a=255;
        v->tmuvtx[0].sow=s->u*inverse;v->tmuvtx[0].tow=s->v*inverse;v->tmuvtx[0].oow=inverse;
        if(textured && s->mat<GLASS) {
            if(detailed) v->r=v->g=v->b=diff*255;
            else { float light=diff*255+(highlights?shine[i]*1.4f:0);if(light>255)light=255;v->r=v->g=v->b=light; }
        } else {
            float spec=(!detailed && highlights)?shine[i]:0;
            v->r=clamp(diff*base[s->mat][0]+spec,0,255);
            v->g=clamp(diff*base[s->mat][1]+spec*.94f,0,255);
            v->b=clamp(diff*base[s->mat][2]+spec*.8f,0,255);
        }
    }
    finish_prepare(camx,camy,camz);
}
#include "lighting.h"

static void prepare(void)
{ if(experiment_flags==64)prepare_lit();else if(experiment_flags)prepare_experimental();else prepare_reference(); }
static void model(void)
{
    int mat,i;
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXTRUE);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    local_color();
    for(mat=0;mat<MATERIALS;++mat) {
        material(mat);
        for(i=0;i<visible_per_mat[mat];++i) {
            int id=visible_faces[mat][i];Face *f=&faces[id];
            if(!detailed && antialias && aa_edges[id]) continue;
            grDrawTriangle(&projected[f->v[0]],&projected[f->v[1]],&projected[f->v[2]]);
        }
    }
    profile_stage(P_OPAQUE);
    if(antialias) {
        grDepthMask(FXFALSE);grDepthBufferFunction(GR_CMP_GEQUAL);
        grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
        if(!detailed) { grDepthMask(FXTRUE);grDepthBufferFunction(GR_CMP_GREATER); }
        for(mat=0;mat<MATERIALS;++mat) {
            material(mat);
            for(i=0;i<aa_per_mat[mat];++i) {
                int id=aa_faces[mat][i];Face *f=&faces[id];int flags=aa_edges[id];
                grAADrawTriangle(&projected[f->v[0]],&projected[f->v[1]],&projected[f->v[2]],
                    (flags&1)!=0,(flags&2)!=0,(flags&4)!=0);
            }
        }
    }
    profile_stage(P_AA);
    /* CPU-computed Gouraud highlights, added in a separate fixed-function pass. */
    if(!highlights || !detailed) { profile_stage(P_SPECULAR);return; }
    local_color();grDepthMask(FXFALSE);grDepthBufferFunction(GR_CMP_GEQUAL);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
    for(mat=0;mat<MATERIALS;++mat) for(i=0;i<visible_per_mat[mat];++i) {
        Face *f=&faces[visible_faces[mat][i]];GrVertex q[3];int j;
        if(shine[f->v[0]]<1 && shine[f->v[1]]<1 && shine[f->v[2]]<1) continue;
        for(j=0;j<3;++j) {
            float s=shine[f->v[j]];q[j]=projected[f->v[j]];
            q[j].r=s;q[j].g=s*.94f;q[j].b=s*.8f;
        }
        grDrawTriangle(&q[0],&q[1],&q[2]);
    }
    profile_stage(P_SPECULAR);
}
static void color_quad(float x,float y,float w,float h,float r,float g,float b)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i) { q[i].r=r;q[i].g=g;q[i].b=b;q[i].a=255; }
    q[0].x=x;q[0].y=y;q[1].x=x+w;q[1].y=y;q[2].x=x+w;q[2].y=y+h;q[3].x=x;q[3].y=y+h;
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static void background(void)
{
    grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grDepthMask(FXFALSE);local_color();
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    color_quad(0,0,640,480,13,20,24);
    color_quad(20,77,600,1,51,56,56);
    color_quad(20,437,600,1,51,56,56);
}
static void text(float x,float y,const char *s,float size,float r,float g,float b)
{
    grTexSource(GR_TMU0,address[3],GR_MIPMAPLEVELMASK_BOTH,&tex[3]);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
    local_color();
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,
        GR_COMBINE_LOCAL_NONE,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
    for(;*s;++s,x+=6*size) {
        int index=*s-32,i;float u=(float)(index%32*8),v=(float)(index/32*8);
        GrVertex q[4];
        if(index<0 || index>95 || *s==' ') continue;
        memset(q,0,sizeof(q));
        for(i=0;i<4;++i) { q[i].r=r;q[i].g=g;q[i].b=b;q[i].a=255;q[i].oow=1;q[i].tmuvtx[0].oow=1; }
        q[0].x=x;q[0].y=y;q[1].x=x+5*size;q[1].y=y;
        q[2].x=x+5*size;q[2].y=y+7*size;q[3].x=x;q[3].y=y+7*size;
        q[0].tmuvtx[0].sow=q[3].tmuvtx[0].sow=u;
        q[1].tmuvtx[0].sow=q[2].tmuvtx[0].sow=u+5;
        q[0].tmuvtx[0].tow=q[1].tmuvtx[0].tow=v;
        q[2].tmuvtx[0].tow=q[3].tmuvtx[0].tow=v+7;
        grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
    }
}
static unsigned short rgb565(int r,int g,int b)
{ return (unsigned short)(((r>>3)<<11)|((g>>2)<<5)|(b>>3)); }
static void panel_pixel(int x,int y,unsigned short color)
{ if(x>=0 && x<128 && y>=0 && y<256) profiler_pixels[y*128+x]=color; }
static void panel_text(int x,int y,const char *s,unsigned short color)
{
    for(;*s;++s,x+=6) {
        int r,index=*s-32;unsigned short *dest;
        if(index<0 || index>=96 || x<0 || x>123 || y<0 || y>249) continue;
        dest=&profiler_pixels[y*128+x];
        for(r=0;r<7;++r,dest+=128) {
            unsigned char bits=panel_glyph[index][r];
            if(bits&16)dest[0]=color;if(bits&8)dest[1]=color;if(bits&4)dest[2]=color;
            if(bits&2)dest[3]=color;if(bits&1)dest[4]=color;
        }
    }
}
static void panel_line(int x0,int y0,int x1,int y1,unsigned short color)
{
    int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
    for(;;) {
        int e2;panel_pixel(x0,y0,color);if(x0==x1 && y0==y1)break;
        e2=2*err;if(e2>=dy){err+=dy;x0+=sx;}if(e2<=dx){err+=dx;y0+=sy;}
    }
}
static void panel_graph(int y,int fps_graph)
{
    int i,n=panel_snapshot.count,first=(panel_snapshot.next-n+PROFILE_HISTORY)%PROFILE_HISTORY,px=4,py=y+29;
    float range=fps_graph?90.0f:50.0f,target=fps_graph?60.0f:1000/60.0f;
    if(!fps_graph) for(i=0;i<n;++i) {
        float ms=panel_snapshot.frame[(first+i)%PROFILE_HISTORY];if(ms>range)range=(float)ceil(ms/25)*25;
    }
    if(!fps_graph) { char label[32];sprintf(label,"MS 0-%d / 16.7",(int)range);panel_text(4,y-12,label,rgb565(131,164,169)); }
    for(i=4;i<124;i+=3) panel_pixel(i,y+29-(int)(target/range*29),rgb565(168,147,87));
    panel_line(4,y+29,123,y+29,rgb565(48,66,70));
    for(i=0;i<n;++i) {
        float ms=panel_snapshot.frame[(first+i)%PROFILE_HISTORY],value=fps_graph?(ms>0?1000/ms:0):ms;
        int x=4+(n>1?i*119/(n-1):0),yy=y+29-(int)(clamp(value/range,0,1)*29);
        if(i) panel_line(px,py,x,yy,ms>18?rgb565(235,137,95):rgb565(117,215,202));
        px=x;py=yy;
    }
}
static void profiler_panel(void)
{
#ifdef ENVIRONMENT_DEMO
    static const char *labels[P_STAGES]={"INPUT","CLEAR","XFORM","OPAQUE","AA","SPEC","HUD","PROF","WAIT","ENVMAP"};
#else
    static const char *labels[P_STAGES]={"INPUT","CLEAR","XFORM","OPAQUE","AA","SPEC","HUD","PROF","WAIT"};
#endif
    unsigned short ink=rgb565(214,219,206),muted=rgb565(131,164,169),gold=rgb565(200,167,104);
    GrVertex q[4];int i;
    if(!profiler.show) return;
    profiler_refresh+=profiler.average>0?profiler.average:1000/60.0f;
    if(profiler_dirty) { profiler_upload=0;profiler_build=0; }
    if(!profiler_upload && !profiler_build && (profiler_dirty || profiler_refresh>=250)) {
        panel_snapshot=profiler;profiler_refresh=0;profiler_dirty=0;profiler_build=1;
    }
    /* Seven small raster jobs, then an incremental upload. Updating the whole
       dashboard in one frame can itself cause the frame-time spike. */
    if(profiler_build) {
        char line[48];int j,phase=profiler_build;
#ifdef ENVIRONMENT_DEMO
        if(phase<=4) {
            unsigned short clear_color=rgb565(8,16,22);int start=(phase-1)*8192;
            for(i=start;i<start+8192;i+=8) {
                profiler_pixels[i]=clear_color;profiler_pixels[i+1]=clear_color;profiler_pixels[i+2]=clear_color;profiler_pixels[i+3]=clear_color;
                profiler_pixels[i+4]=clear_color;profiler_pixels[i+5]=clear_color;profiler_pixels[i+6]=clear_color;profiler_pixels[i+7]=clear_color;
            }
            ++profiler_build;goto environment_profiler_publish;
        }
        phase-=4;
#endif
        if(phase==1) {
            unsigned short background_color=rgb565(8,16,22);
#ifndef ENVIRONMENT_DEMO
            for(i=0;i<128*256;++i) profiler_pixels[i]=background_color;
#endif
            panel_line(0,0,127,0,muted);panel_line(0,255,127,255,muted);
            panel_line(0,0,0,255,muted);panel_line(127,0,127,255,muted);
            panel_text(4,5,"FRAME PROFILER",gold);panel_text(4,16,"EMU MS / TAB CLOSE",muted);
            if(!panel_snapshot.available) {
                panel_text(4,40,"TIMER UNAVAILABLE",ink);panel_text(4,52,"USE SUPPLIED CONF",muted);
            } else if(panel_snapshot.average>0) {
                int fps10=(int)(10000/panel_snapshot.average+.5f),ms10=(int)(panel_snapshot.average*10+.5f);
                sprintf(line,"FPS %d.%d  %d.%dMS",fps10/10,fps10%10,ms10/10,ms10%10);
                panel_text(4,29,line,ink);
            } else panel_text(4,29,"COLLECTING FRAMES",ink);
        } else if(phase>=2 && phase<=4 && panel_snapshot.available) {
            int first=(phase-2)*3;
            int end=first+3,step=10;
#ifdef ENVIRONMENT_DEMO
            if(phase==4)end=P_STAGES;step=9;
#endif
            for(i=first;i<end && i<P_STAGES;++i) {
                int y=42+i*step,width=panel_snapshot.average>0?(int)(panel_snapshot.mean[i]/panel_snapshot.average*116):0;
                int value=(int)(panel_snapshot.mean[i]*100+.5f);
                panel_text(4,y,labels[i],muted);sprintf(line,"%2d.%02d",value/100,value%100);panel_text(80,y,line,ink);
                for(j=0;j<width && j<116;++j) panel_pixel(4+j,y+8,i==P_PRESENT?muted:gold);
            }
            if(phase==4) {
                int p=(int)(panel_snapshot.p95*10+.5f),m=(int)(panel_snapshot.max*10+.5f);
                sprintf(line,"P95 %d.%d MAX %d.%d",p/10,p%10,m/10,m%10);panel_text(4,135,line,ink);
            }
        } else if(phase==5 && panel_snapshot.available) {
            panel_text(4,148,"FPS 0-90 / 60",muted);panel_graph(159,1);
        } else if(phase==6 && panel_snapshot.available) {
            panel_graph(207,0);panel_text(4,245,"16.7 MS = 60 FPS",gold);
        }
        ++profiler_build;
        if(profiler_build==
#ifdef ENVIRONMENT_DEMO
            12
#else
            8
#endif
        ) { profiler_build=0;profiler_row=0;profiler_upload=1; }
    }
#ifdef ENVIRONMENT_DEMO
environment_profiler_publish:
#endif
    if(profiler_upload) {
        /* Publish a complete new dashboard only after its incremental upload.
           4 KiB per frame avoids a 64 KiB burst that could miss retrace. */
        grTexDownloadMipMapLevelPartial(GR_TMU0,profiler_address[1-profiler_front],GR_LOD_256,GR_LOD_256,
            GR_ASPECT_1x2,GR_TEXFMT_RGB_565,GR_MIPMAPLEVELMASK_BOTH,
            profiler_pixels+profiler_row*128,profiler_row,profiler_row+15);
        profiler_row+=16;
        if(profiler_row==256) { profiler_front=1-profiler_front;profiler_upload=0; }
    }
    grTexSource(GR_TMU0,profiler_address[profiler_front],GR_MIPMAPLEVELMASK_BOTH,&profiler_tex);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
    local_color();
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,
        GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    memset(q,0,sizeof(q));
    for(i=0;i<4;++i) { q[i].r=q[i].g=q[i].b=q[i].a=255;q[i].oow=q[i].tmuvtx[0].oow=1; }
    q[0].x=q[3].x=14;q[1].x=q[2].x=174;
    q[0].y=q[1].y=89;q[2].y=q[3].y=409;
    q[1].tmuvtx[0].sow=q[2].tmuvtx[0].sow=128;
    q[2].tmuvtx[0].tow=q[3].tmuvtx[0].tow=256;
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static int select_view_mesh(void)
{
    int distant=model_distant;
    if(detailed)return 1;
    if(zoom<.84f)distant=1;else if(zoom>.92f)distant=0;
    if(distant!=model_distant) { model_distance(distant);return build_helmet(); }
    return 1;
}
static void render(int fps,int paused)
{
    char line[80];
    if(!profiler.active) { profile_begin();profile_stage(P_INPUT); }
    grDepthMask(FXTRUE);grBufferClear(0x0018140d,0,0);
    background();profile_stage(P_BACKGROUND);
    if(automatic_lod && !select_view_mesh()) { mesh_error=1;return; }
    prepare();profile_stage(P_TRANSFORM);model();
#ifdef ENVIRONMENT_DEMO
    environment_draw();profile_stage(P_ENVIRONMENT);
#endif
    grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grDepthMask(FXFALSE);
#ifdef ENVIRONMENT_DEMO
    environment_hud(fps);
#else
    text(22,19,"WINCHESTER / HELMET STUDY",1,176,157,122);
    text(21,39,"DIVING HELMET",3,233,224,202);
    text(510,22,antialias?"AA ON":"AA OFF",2,135,209,202);
    if(fps>0) sprintf(line,"FPS %d",fps);else strcpy(line,"FPS --");
    text(510,48,line,1,176,188,190);
    text(510,63,detailed?"Q DETAIL":model_distant?"Q FAST FAR":"Q FAST",1,176,157,122);
    text(22,452,"LMB DRAG  TURN    WHEEL/RMB DRAG  ZOOM    A  EDGE AA",1,176,188,190);
    text(22,466,"SPACE PAUSE   ARROWS TURN   T TEXTURE   TAB PROFILER   ESC EXIT",1,116,140,148);
#endif
    if(paused) text(274,417,"PAUSED",1,194,164,104);
    profile_stage(P_HUD);
    profiler_panel();
    profile_stage(P_OVERLAY);
    wait_retrace();grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();
    profile_stage(P_PRESENT);profile_end();
}
static unsigned short *capture(const char *path)
{
    unsigned short *p=malloc(640UL*480*2);unsigned long i;FILE *f;
    if(!p || !grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,640,480,1280,p)) { free(p);return NULL; }
    f=fopen(path,"wb");if(!f) { free(p);return NULL; }
    fprintf(f,"P6\n640 480\n255\n");
    for(i=0;i<640UL*480;++i) {
        unsigned char c[3];c[0]=((p[i]>>11)&31)*255/31;c[1]=((p[i]>>5)&63)*255/63;c[2]=(p[i]&31)*255/31;
        if(fwrite(c,1,3,f)!=3) { fclose(f);free(p);return NULL; }
    }
    if(fclose(f)) { free(p);return NULL; }return p;
}
#include "experiments.h"
#include "lod-test.h"
#ifdef ENVIRONMENT_DEMO
#include "../envmap/tests.h"
#endif

int main(int argc,char **argv)
{
    GrHwConfiguration hw;
    int key,paused=0,frames=0,ok=1,fps=0,second,frame_sample=0,synced=0;
    int profile_test=argc>1 && !strcmp(argv[1],"/profiletest");
    int detail_test=argc>1 && !strcmp(argv[1],"/detailtest");
    int test=detail_test || profile_test || (argc>1 && !strcmp(argv[1],"/test"));
    int bench=argc>1 && !strcmp(argv[1],"/bench");
    int soak=argc>1 && !strcmp(argv[1],"/soak");
    int experiments=argc>1 && !strcmp(argv[1],"/optbench");
    int shared_test=argc>1 && !strcmp(argv[1],"/sharedtest");
    int lod_test=argc>1 && !strcmp(argv[1],"/lodtest");
    FILE *log=fopen(HELMET_LOG,"w");
#ifdef ENVIRONMENT_DEMO
    int environment_control=argc>1 && !strcmp(argv[1],"/envcontrol");
    if(argc>1 && !strcmp(argv[1],"/envstress"))soak=2;
#endif
    if(detail_test) { detailed=1;model_detail(1); }
    if(!build_helmet()) { puts("Mesh capacity or topology error");if(log)fclose(log);return 1; }
    if(!detailed) {
        int ready;model_distance(1);ready=build_helmet();model_distance(0);ready=build_helmet() && ready;
        if(!ready) { puts("Cannot cache Fast meshes");if(log)fclose(log);return 1; }
    }
    if(log) { fprintf(log,"Mesh: %d vertices, %d faces, %d edges\n",vertex_count,face_count,edge_count);fflush(log); }
    grGlideInit();
    if(!grSstQueryHardware(&hw) || hw.num_sst<1) { grGlideShutdown();if(log)fclose(log);return 1; }
    grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)) {
        grGlideShutdown();if(log)fclose(log);return 1;
    }
    grCullMode(GR_CULL_DISABLE);grDitherMode(GR_DITHER_4x4);
    if(!load_textures()) { grGlideShutdown();puts("Cannot load assets/COPPER.RGB");if(log)fclose(log);return 1; }
    profile_init();
    mouse_init();
    if(log) { fprintf(log,"DOS mouse: %s; WheelAPI: %s\n",mouse_present?"present":"absent",wheel_present?"present":"absent");fflush(log); }
#ifdef ENVIRONMENT_DEMO
    if(log && hw.SSTs[0].type==GR_SSTTYPE_VOODOO) {
        fprintf(log,"Voodoo framebuffer %d MB, %d TMU(s), TMU0 %d MB; environment %lu bytes, total textures %lu bytes\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)environment_bytes,(unsigned long)(environment_address+environment_bytes+32768));fflush(log);
    }
    if(environment_control){ok=environment_controls(log);goto done;}
#endif
    if(experiments || shared_test) { automatic_lod=0;ok=run_experiments(log,shared_test);goto done; }
    if(lod_test) { ok=run_lod_test(log);goto done; }
    if(test) {
        unsigned short *a,*b;unsigned long i,changes=0,lit=0;
        int controls=camera_test();if(!controls)ok=0;
        if(log) fprintf(log,"Mouse targets, clamps, smooth acceleration, flick inertia and settling: %s\n",controls?"PASS":"FAIL");
        antialias=0;render(0,1);a=capture("AAOFF.PPM");
        antialias=1;render(0,1);b=capture("AAON.PPM");
        if(!a || !b) ok=0;
        else for(i=90UL*640;i<410UL*640;++i) {
            int x=(int)(i%640);
            if(x<80 || x>560) continue;
            if(a[i]!=b[i]) ++changes;
            if(((b[i]>>11)&31)>8 || ((b[i]>>5)&63)>16) ++lit;
        }
        free(a);free(b);
        if(changes<10 || lit<15000 || silhouette_count<50) ok=0;
        if(log) fprintf(log,"Same-pose AA: %lu changed pixels, %lu lit pixels, %d silhouette edges, %d visible faces\n",
            changes,lit,silhouette_count,visible_count);
        yaw=1.1f;render(0,1);b=capture("SIDE.PPM");if(!b)ok=0;free(b);
#ifdef ENVIRONMENT_DEMO
        if(!environment_checks(log))ok=0;
#endif
        if(profile_test) {
            int n;profile_toggle();yaw=-.6f;
            for(n=0;n<120;++n) { render(60,1);yaw+=.001f; }
            b=capture("PROFILER.PPM");if(!b || !profiler.available || profiler.count<100 || profiler.average<=0)ok=0;free(b);
            profile_log(log);
        }
        goto done;
    }
    if(soak) {
        int n;float min_ms=10000,max_ms=0,sum=0;int misses=0;
#ifdef ENVIRONMENT_DEMO
        int count=soak==2?240:1440;
#else
        int count=1440;
#endif
        FILE *mark=fopen("SOAKSTAR.MRK","w");if(mark)fclose(mark);else ok=0;
#ifdef ENVIRONMENT_DEMO
        render(60,0);render(60,0);profile_reset();
#endif
        profiler.measure=1;
        for(n=0;n<count;++n) {
            float ms;
#ifdef ENVIRONMENT_DEMO
            float sample=n*(1440.0f/count);
            yaw=-.6f+sample*(6.2831853f/720);
            if(n==count/2){profile_toggle();profiler_dirty=1;}
            if(n>=count/2){pitch=-.05f+.45f*(float)sin(sample*.017f);zoom=.97f+.32f*(float)sin(sample*.013f);}
#else
            yaw=-.6f+n*(6.2831853f/720);
            if(n==720) { profile_toggle();profiler_dirty=1; }
            if(n>=720) { pitch=-.05f+.45f*(float)sin(n*.017f);zoom=.97f+.32f*(float)sin(n*.013f); }
#endif
            render(60,0);++frames;ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];
            if(ms<min_ms)min_ms=ms;if(ms>max_ms)max_ms=ms;sum+=ms;
            if(ms>18) {
                int stage,slot=(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY;++misses;
                if(log
#ifdef ENVIRONMENT_DEMO
                    && misses<=12
#endif
                ) {
                    fprintf(log,"Slow frame %d: %.3f ms, zoom %.4f, distant %d; stages",n,ms,zoom,model_distant);
                    for(stage=0;stage<P_STAGES;++stage)fprintf(log," %.3f",profiler.stage[slot][stage]);
                    fprintf(log,"\n");
                }
            }
        }
        mark=fopen("SOAKEND.MRK","w");if(mark)fclose(mark);else ok=0;
        if(log) { fprintf(log,"AA full-orbit soak: %d frames, %.2f FPS, min %.3f / max %.3f ms; %d frames above 18ms\n",frames,frames*1000.0f/sum,min_ms,max_ms,misses);profile_log(log); }
        if(misses>0)ok=0;
        goto done;
    }
    if(bench) {
        int phase,i;
        for(phase=0;phase<3;++phase) {
            char marker[20];FILE *mark;
            antialias=phase==1;highlights=phase!=2;yaw=-.6f;
            profiler.measure=1;profile_reset();
            for(i=0;i<2;++i) { profile_begin();profile_stage(P_INPUT);render(0,0); }
            profile_reset();
            sprintf(marker,"P%dSTART.MRK",phase);mark=fopen(marker,"w");if(mark)fclose(mark);else ok=0;
            for(i=0;i<60;++i) { profile_begin();profile_stage(P_INPUT);render(0,0);yaw+=.003f;++frames; }
            sprintf(marker,"P%dEND.MRK",phase);mark=fopen(marker,"w");if(mark)fclose(mark);else ok=0;
            if(log) { fprintf(log,"Phase %d:\n",phase);profile_log(log);fflush(log); }
        }
        goto done;
    }
    second=rtc_seconds();
    for(;;) {
        profile_begin();
        while(kbhit()) {
            key=getch();
            if(key==27) goto done;
            if(key==9) { profile_toggle();profiler_dirty=1; }
            if(key=='a' || key=='A') antialias=!antialias;
            if(key=='t' || key=='T') textured=!textured;
#ifdef ENVIRONMENT_DEMO
            if(key=='e' || key=='E')environment_enabled=!environment_enabled;
            if(key=='+' || key=='=')environment_adjust(1);
            if(key=='-')environment_adjust(-1);
#else
            if(key=='+' || key=='=') target_zoom=clamp(target_zoom+.05f,.65f,1.3f);
            if(key=='-') target_zoom=clamp(target_zoom-.05f,.65f,1.3f);
#endif
            if(key=='q' || key=='Q') { detailed=!detailed;model_detail(detailed);if(!build_helmet()){ok=0;goto done;} }
            if(key==' ') { paused=!paused;inertia_yaw=inertia_pitch=0; }
            if(key=='r' || key=='R') reset_target();
            if(key==0 || key==224) {
                inertia_yaw=inertia_pitch=0;
                key=getch();if(key==75)target_yaw-=.08f;if(key==77)target_yaw+=.08f;
                if(key==72)target_pitch=clamp(target_pitch-.05f,-.55f,.45f);
                if(key==80)target_pitch=clamp(target_pitch+.05f,-.55f,.45f);
            }
        }
        mouse_poll(fps>0?clamp(1.0f/fps,1/120.0f,.1f):1/30.0f,&paused);
        if(!paused) target_yaw+=.003f;
        camera_step(fps>0?clamp(1.0f/fps,1/120.0f,.1f):1/30.0f);
        profile_stage(P_INPUT);
        render(fps,paused);if(mesh_error){ok=0;goto done;}++frames;++frame_sample;
        {
            int now=rtc_seconds(),elapsed=(now-second+60)%60;
            if(elapsed) { if(synced)fps=frame_sample/elapsed;synced=1;frame_sample=0;second=now; }
        }
    }
done:
    if(mesh_error)ok=0;
    grGlideShutdown();
    if(log) { fprintf(log,"%s: textured C diving helmet, AA toggle, 640x480, %d frames\n",ok?"PASS":"FAIL",frames);fclose(log); }
    return ok?0:1;
}
