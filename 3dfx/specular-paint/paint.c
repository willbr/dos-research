/* Native texture-alpha selection of additive, CPU-computed specular RGB. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <dos.h>
#include <glide.h>
#include "../helmet/profiler.h"
static GrTexInfo tex[8],profiler_tex,probe_texture;
static FxU32 address[8],profiler_address[2],probe_address,texture_bytes;
static unsigned short profiler_pixels[128*256];
static unsigned char panel_glyph[96][7];
static FrameProfiler panel_snapshot;
static int profiler_dirty=1,profiler_front,profiler_upload,profiler_row,profiler_build;
static float profiler_refresh;
static float clamp(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
static void local_color(void)
{
    grAlphaControlsITRGBLighting(FXFALSE);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
}
#include "../textures/overlay.h"
typedef struct {float x,y,z;} Vec;
typedef struct {Vec p,n;} MeshVertex;
typedef struct {Vec p;float r,g,b,ndoth,ndotl;} LitVertex;
#define NX 20
#define NY 12
#define NV ((NX+1)*(NY+1))
static MeshVertex mesh[NV];
static LitVertex lit[NV];
static int split=1,mask_enabled=1,specular_enabled=1,mask_view,swapped,moving=1,light_moving=1,orbiting,mouse_present,save_captures=1,point_sampling;
static int gloss_index=2,draw_calls,finite_error;
static const int gloss_values[4]={8,16,32,64};
static float phase,light_time,strength=1,yaw,pitch,distance=5,pose_yaw,pose_pitch;
static Vec lamp;
static Vec vec(float x,float y,float z){Vec a;a.x=x;a.y=y;a.z=z;return a;}
static Vec sub(Vec a,Vec b){return vec(a.x-b.x,a.y-b.y,a.z-b.z);}
static float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec unit(Vec a){float d=(float)sqrt(dot(a,a));return d>.00001f?vec(a.x/d,a.y/d,a.z/d):vec(0,0,0);}
static int rtc_seconds(void)
{int mode,sec;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);return mode&4?sec:(sec>>4)*10+(sec&15);}
static int upload(GrTexInfo *info,FxU32 *dest,FxU32 *next,void *pixels)
{
    FxU32 bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,info);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    *dest=*next;info->data=pixels;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,info);*next+=bytes;info->data=NULL;return 1;
}
static int load_textures(void)
{
    unsigned short *pixels=malloc(128UL*128*2),probe[4]={0x4210,0xc210,0x4210,0xc210};
    unsigned char *font=calloc(8192,1);FxU32 next=grTexMinAddress(GR_TMU0);int i,x,y,ok=0;
    if(!pixels || !font)goto done;
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(glyphs[i].row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    for(y=0;y<128;++y)for(x=0;x<128;++x) {
        int noise=(x*13+y*31+x*y%17)%9,paint=x>=46 && x<=81 && y>4 && y<123;
        int chip=((x*71+y*47)%431<5 && (x<50 || x>77)) || ((y*13+x*3)%139==0);
        int r=80+noise,g=86+noise,b=99+noise;
        if(chip)paint=0;
        if(paint){r=24+noise;g=55+noise;b=114+noise;}
        if(y>=53 && y<74 && x>=17 && x<113) {
            static const char sign[]="3DFX";int sx=(x-17)/4,sy=(y-53)/3;
            if(sx%6<5 && sy<7 && (panel_glyph[sign[sx/6]-32][sy]&(1<<(4-sx%6)))){paint=1;r=165;g=152;b=118;}
        }
        if((x<5 || x>122) && (y<5 || y>122)){paint=0;r=52;g=57;b=65;}
        pixels[y*128+x]=(unsigned short)((paint?0x8000:0)|((r>>3)<<10)|((g>>3)<<5)|(b>>3));
    }
    tex[0].smallLod=tex[0].largeLod=GR_LOD_128;tex[0].aspectRatio=GR_ASPECT_1x1;tex[0].format=GR_TEXFMT_ARGB_1555;
    if(!upload(&tex[0],&address[0],&next,pixels))goto done;
    tex[3]=tex[0];tex[3].smallLod=tex[3].largeLod=GR_LOD_256;tex[3].aspectRatio=GR_ASPECT_8x1;tex[3].format=GR_TEXFMT_ALPHA_8;
    if(!upload(&tex[3],&address[3],&next,font))goto done;
    probe_texture=tex[0];probe_texture.smallLod=probe_texture.largeLod=GR_LOD_2;
    if(!upload(&probe_texture,&probe_address,&next,probe))goto done;
    profiler_tex=tex[3];profiler_tex.aspectRatio=GR_ASPECT_1x2;profiler_tex.format=GR_TEXFMT_RGB_565;
    memset(profiler_pixels,0,sizeof(profiler_pixels));
    for(i=0;i<2;++i)if(!upload(&profiler_tex,&profiler_address[i],&next,profiler_pixels))goto done;
    texture_bytes=next-grTexMinAddress(GR_TMU0);ok=1;
done:free(pixels);free(font);return ok;
}
static void model_init(void)
{
    int row,col;for(row=0;row<=NY;++row)for(col=0;col<=NX;++col) {
        MeshVertex *v=&mesh[row*(NX+1)+col];float x=-2.2f+col*(4.4f/NX),y=1.25f-row*(2.5f/NY);
        v->p=vec(x,y,.12f*x*x+.14f*y*y);v->n=unit(vec(.24f*x,.28f*y,-1));
    }
}
static Vec rotate(Vec p,float cy,float sy,float cx,float sx)
{float x=p.x*cy+p.z*sy,z=-p.x*sy+p.z*cy;return vec(x,p.y*cx-z*sx,p.y*sx+z*cx);}
static float specular_power(float x)
{int n=gloss_values[gloss_index];while(n>1){x*=x;n/=2;}return x;}
static void prepare_panel(void)
{
    int i;float cy,sy,cx,sx;
    pose_yaw=yaw+(orbiting?.23f*(float)sin(phase*.3f):0);pose_pitch=pitch+(orbiting?.15f*(float)sin(phase*.23f):0);
    cy=(float)cos(pose_yaw);sy=(float)sin(pose_yaw);cx=(float)cos(pose_pitch);sx=(float)sin(pose_pitch);
    lamp=vec(4.2f*(float)sin(light_time*.85f+.9f),1.8f*(float)cos(light_time*.6f+.3f),-2);
    for(i=0;i<NV;++i) {
        LitVertex *v=&lit[i];Vec n=rotate(mesh[i].n,cy,sy,cx,sx),l,view,h;float spec;
        v->p=rotate(mesh[i].p,cy,sy,cx,sx);v->p.z+=distance;
        l=unit(sub(lamp,v->p));view=unit(vec(-v->p.x,-v->p.y,-v->p.z));h=unit(vec(l.x+view.x,l.y+view.y,l.z+view.z));
        v->ndotl=dot(n,l);v->ndoth=clamp(dot(n,h),0,1);
        spec=specular_enabled && v->ndotl>0?clamp(strength*specular_power(v->ndoth),0,1):0;
        v->r=245*spec;v->g=225*spec;v->b=205*spec;
    }
}
static void paint_state(int enabled,int preview)
{
    grTexSource(GR_TMU0,address[0],GR_MIPMAPLEVELMASK_BOTH,&tex[0]);
    grTexFilterMode(GR_TMU0,point_sampling || preview?GR_TEXTUREFILTER_POINT_SAMPLED:GR_TEXTUREFILTER_BILINEAR,point_sampling || preview?GR_TEXTUREFILTER_POINT_SAMPLED:GR_TEXTUREFILTER_BILINEAR);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);grHints(GR_HINT_STWHINT,0);
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXTRUE);
    grFogMode(GR_FOG_DISABLE);grCullMode(GR_CULL_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);grDepthBiasLevel(0);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    grConstantColorValue(preview?0xffffffffUL:0xff000000UL);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
    grColorCombine(preview?GR_COMBINE_FUNCTION_SCALE_OTHER:GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL,
        preview?GR_COMBINE_FACTOR_TEXTURE_ALPHA:GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_ITERATED,preview?GR_COMBINE_OTHER_CONSTANT:GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaControlsITRGBLighting(enabled && !preview?FXTRUE:FXFALSE);
}
static void draw_panel(float center)
{
    int row,col;float focal=split?330:550;
    for(row=0;row<NY;++row)for(col=0;col<NX;++col) {
        int a=row*(NX+1)+col,ids[4],i;GrVertex q[4];ids[0]=a;ids[1]=a+1;ids[2]=a+NX+2;ids[3]=a+NX+1;memset(q,0,sizeof(q));
        for(i=0;i<4;++i) {
            int id=ids[i];LitVertex *v=&lit[id];float inv=1/v->p.z,u=(id%(NX+1))*(256.0f/NX),t=(id/(NX+1))*(256.0f/NY);
            q[i].x=center+(float)floor(v->p.x*focal*inv*16+.5f)/16;q[i].y=260-v->p.y*focal*inv;
            q[i].oow=q[i].tmuvtx[0].oow=inv;q[i].ooz=65535*inv;q[i].tmuvtx[0].sow=u*inv;q[i].tmuvtx[0].tow=t*inv;
            q[i].r=v->r;q[i].g=v->g;q[i].b=v->b;q[i].a=255;
            if(q[i].x!=q[i].x || q[i].y!=q[i].y || v->p.z<1)finite_error=1;
        }
        grDrawPolygonVertexList(4,q);++draw_calls;
    }
}
static void draw_scene(int fps,int hud)
{
    int pass,passes=split?2:1;char line[100];
    profile_begin();profile_stage(P_INPUT);grAlphaControlsITRGBLighting(FXFALSE);grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0,0,0);
    background();profile_stage(P_BACKGROUND);prepare_panel();profile_stage(P_TRANSFORM);draw_calls=0;
    for(pass=0;pass<passes;++pass) {
        int feature=split?(pass^swapped):1;float center=split?(pass?480:160):320;
        grClipWindow(split?pass*320:0,88,split?(pass+1)*320:640,432);paint_state(feature && mask_enabled,feature && mask_view);draw_panel(center);
    }
    profile_stage(P_OPAQUE);local_color();grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grClipWindow(0,0,640,480);
    if(hud) {
        text(20,18,"3DFX / ALPHA-MASKED SPECULAR",1,176,157,122);text(20,39,"PAINTED METAL",2.5f,233,224,202);
        sprintf(line,"FPS %d",fps);text(535,20,line,1,135,209,202);
        sprintf(line,"E MASK %s  G SPEC %s  N GLOSS %d  POWER %.0f  L LIGHT %s",mask_enabled?"ON":"OFF",specular_enabled?"ON":"OFF",gloss_values[gloss_index],strength*100,light_moving?"MOVING":"HELD");text(20,67,line,1,135,209,202);
        for(pass=0;pass<passes;++pass) {
            int feature=split?(pass^swapped):1;float x=split?pass*320.0f:0;
            text(x+18,96,feature?(mask_view?"PAINT MASK WHITE / METAL BLACK":mask_enabled?"PAINT MATTE / METAL SHINES":"MASK OFF / WHOLE PANEL SHINES"):"UNMASKED ADDITIVE HIGHLIGHT",1,214,219,206);
        }
        text(20,402,"CPU BLINN HIGHLIGHT / NATIVE SINGLE-PASS PAINT MASK",1,176,157,122);
        text(20,447,"E MASK G SPEC C SPLIT S SWAP M MASK N GLOSS +/- STRENGTH R RESET",1,176,188,190);
        text(20,463,"L LIGHT F ORBIT LMB LOOK RMB ZOOM SPACE PAUSE TAB PROF ESC",1,116,140,148);
    }
    profile_stage(P_HUD);if(hud)profiler_panel();profile_stage(P_OVERLAY);
    while(grSstVRetraceOn()){}while(!grSstVRetraceOn()){}grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();profile_stage(P_PRESENT);profile_end();
}
static unsigned short *capture(const char *name)
{
    unsigned short *p=malloc(640UL*480*2);FILE *f;unsigned long i;
    if(!p || !grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,640,480,1280,p)){free(p);return NULL;}
    if(!name || !save_captures)return p;f=fopen(name,"wb");if(!f){free(p);return NULL;}fprintf(f,"P6\n640 480\n255\n");
    for(i=0;i<640UL*480;++i){unsigned char c[3];c[0]=((p[i]>>11)&31)*255/31;c[1]=((p[i]>>5)&63)*255/63;c[2]=(p[i]&31)*255/31;fwrite(c,1,3,f);}
    if(fclose(f)){free(p);return NULL;}return p;
}
#include "tests.h"
int main(int argc,char **argv)
{
    GrHwConfiguration hw;union REGS regs;FILE *log=fopen("PAINT.LOG","w");int ok=1,key,frames=0,fps=0,samples=0,stamp,synced=0;
    int test=argc>1 && (!strcmp(argv[1],"/test") || !strcmp(argv[1],"/softwarecheck")),bench=argc>1 && !strcmp(argv[1],"/bench");
    if(argc>1 && !strcmp(argv[1],"/softwarecheck"))save_captures=0;
    grGlideInit();if(!grSstQueryHardware(&hw) || hw.num_sst<1){ok=0;goto done;}grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){ok=0;goto done;}
    if(!load_textures()){ok=0;goto done;}
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grDitherMode(GR_DITHER_4x4);profile_init();profiler.measure=1;model_init();
    if(log)fprintf(log,"Voodoo: framebuffer %d MB, %d TMUs, TMU0 %d MB, texture bytes %lu; timer %d; %d panel cells\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)texture_bytes,profiler.available,NX*NY);
    if(test){ok=tests(log);goto done;}if(bench){ok=benchmark(log);goto done;}
    memset(&regs,0,sizeof(regs));int386(0x33,&regs,&regs);mouse_present=regs.w.ax==0xffff;stamp=rtc_seconds();
    for(;;) {
        float dt=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
        while(kbhit()) {
            key=getch();if(key==27)goto done;if(key==0 || key==224){getch();continue;}
            if(key=='e'||key=='E')mask_enabled=!mask_enabled;if(key=='g'||key=='G')specular_enabled=!specular_enabled;
            if(key=='c'||key=='C')split=!split;if(key=='s'||key=='S')swapped=!swapped;if(key=='m'||key=='M')mask_view=!mask_view;
            if(key=='n'||key=='N')gloss_index=(gloss_index+1)%4;if(key=='l'||key=='L')light_moving=!light_moving;if(key=='f'||key=='F')orbiting=!orbiting;
            if(key=='+'||key=='=')strength=clamp(strength+.1f,0,1.5f);if(key=='-')strength=clamp(strength-.1f,0,1.5f);if(key==' ')moving=!moving;
            if(key=='r'||key=='R'){split=mask_enabled=specular_enabled=moving=light_moving=1;mask_view=swapped=orbiting=0;phase=light_time=yaw=pitch=0;distance=5;strength=1;gloss_index=2;}
            if(key==9){profile_toggle();profiler_dirty=1;}
        }
        if(mouse_present) {
            int buttons,dx,dy;regs.w.ax=3;int386(0x33,&regs,&regs);buttons=regs.w.bx&3;regs.w.ax=11;int386(0x33,&regs,&regs);dx=(short)regs.w.cx;dy=(short)regs.w.dx;
            if(buttons){if(orbiting){yaw=pose_yaw;pitch=pose_pitch;orbiting=0;}if(buttons&1){yaw=clamp(yaw-dx*.004f,-.45f,.45f);pitch=clamp(pitch+dy*.004f,-.3f,.3f);}if(buttons&2)distance=clamp(distance+dy*.025f,4.5f,8);}
        }
        if(moving){phase+=dt;if(light_moving)light_time+=dt;}draw_scene(fps,1);++frames;++samples;
        {int now=rtc_seconds(),elapsed=(now-stamp+60)%60;if(elapsed){if(synced)fps=samples/elapsed;synced=1;samples=0;stamp=now;}}
    }
done:
    if(log){fprintf(log,"%s: alpha-masked specular paint, %d interactive frames\n",ok?"PASS":"FAIL",frames);fclose(log);}grGlideShutdown();return ok?0:1;
}
