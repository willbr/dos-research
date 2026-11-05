/* Native Z/W buffer controls and precision fixtures. C89 / Glide 2.43. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <glide.h>
#include "../helmet/profiler.h"
static GrTexInfo tex[8],profiler_tex;
static FxU32 address[8],profiler_address[2],texture_bytes;
static unsigned short profiler_pixels[128*256];
static unsigned char panel_glyph[96][7];
static FrameProfiler panel_snapshot;
static int profiler_dirty=1,profiler_front,profiler_upload,profiler_row,profiler_build;
static float profiler_refresh;
static float clamp(float v,float lo,float hi){return v<lo?lo:v>hi?hi:v;}
static void local_color(void)
{
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
}
#include "../textures/overlay.h"
static int split=1,w_mode,depth_enabled=1,depth_writes=1,reverse_order,compare_mode,precision_view,tight_range,moving=1,save_captures=1;
static float phase=.37f,precision_distance=4096;
static int finite_error;
static const int comparisons[9]={-1,GR_CMP_NEVER,GR_CMP_LESS,GR_CMP_EQUAL,GR_CMP_LEQUAL,GR_CMP_GREATER,GR_CMP_NOTEQUAL,GR_CMP_GEQUAL,GR_CMP_ALWAYS};
static const char *compare_names[9]={"AUTO","NEVER","LESS","EQUAL","LEQUAL","GREATER","NOTEQUAL","GEQUAL","ALWAYS"};
static const float gaps[4]={.0001f,.0005f,.001f,.01f};
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
    unsigned char *font=calloc(8192,1);FxU32 next=grTexMinAddress(GR_TMU0);int i,x,y,ok=0;
    if(!font)return 0;
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(glyphs[i].row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    tex[3].smallLod=tex[3].largeLod=GR_LOD_256;tex[3].aspectRatio=GR_ASPECT_8x1;tex[3].format=GR_TEXFMT_ALPHA_8;
    if(!upload(&tex[3],&address[3],&next,font))goto done;
    profiler_tex=tex[3];profiler_tex.aspectRatio=GR_ASPECT_1x2;profiler_tex.format=GR_TEXFMT_RGB_565;
    memset(profiler_pixels,0,sizeof(profiler_pixels));
    for(i=0;i<2;++i)if(!upload(&profiler_tex,&profiler_address[i],&next,profiler_pixels))goto done;
    texture_bytes=next-grTexMinAddress(GR_TMU0);ok=1;
done:free(font);return ok;
}
static int active_compare(int w)
{return compare_mode?comparisons[compare_mode]:(w?GR_CMP_LESS:GR_CMP_GREATER);}
static float z_value(float distance)
{
    float near_z=1,far_z=65535;
    if(tight_range){near_z=precision_view?precision_distance*.95f:3;far_z=precision_view?precision_distance*1.05f:10;}
    return clamp(65535*(1/distance-1/far_z)/(1/near_z-1/far_z),0,65535);
}
static void set_depth(GrVertex *v,float distance)
{
    v->ooz=z_value(distance);v->oow=1/distance;v->a=255;
    if(distance<1 || distance>65535 || v->ooz!=v->ooz)finite_error=1;
}
static void drawing_state(int w,int enabled,int writes,int comparison)
{
    local_color();grFogMode(GR_FOG_DISABLE);grCullMode(GR_CULL_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);grDepthBiasLevel(0);grColorMask(FXTRUE,FXFALSE);
    grDepthBufferMode(w?GR_DEPTHBUFFER_WBUFFER:GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(comparison);grDepthMask(writes?FXTRUE:FXFALSE);
    if(!enabled)grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
}
static void clear_view(int w,int comparison)
{
    int clear_depth;
    if(comparison==GR_CMP_LESS || comparison==GR_CMP_LEQUAL)clear_depth=65535;
    else if(comparison==GR_CMP_GREATER || comparison==GR_CMP_GEQUAL)clear_depth=0;
    else clear_depth=w?GR_WDEPTHVALUE_FARTHEST:GR_ZDEPTHVALUE_FARTHEST;
    /* Clearing must work even when the previous frame disabled depth writes. */
    grDepthBufferMode(w?GR_DEPTHBUFFER_WBUFFER:GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grColorMask(FXTRUE,FXFALSE);
    grBufferClear(0x001b120c,0,(FxU16)clear_depth);
}
static void flat_rect(float x,float y,float width,float height,float distance,int color)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i) {
        q[i].x=x+((i==1||i==2)?width:0);q[i].y=y+(i<2?0:height);set_depth(&q[i],distance);
        q[i].r=color==0?240:color==1?45:50;q[i].g=color==0?125:color==1?205:70;q[i].b=color==0?60:color==1?230:230;
    }
    grDrawPolygonVertexList(4,q);
}
static void intersection(float center,int order)
{
    int n,i;float focal=split?290:350;
    for(n=0;n<2;++n) {
        int color=order?1-n:n;float yaw=(color?-.8f:.8f)+.14f*(float)sin(phase),roll=(color?-.24f:.24f)+.08f*(float)cos(phase*.7f);
        float cy=(float)cos(yaw),sy=(float)sin(yaw),cr=(float)cos(roll),sr=(float)sin(roll);GrVertex q[4];
        memset(q,0,sizeof(q));
        for(i=0;i<4;++i) {
            float x=(i==1||i==2)?2.6f:-2.6f,y=i<2?-1.35f:1.35f;
            float z=6+x*sy,xx=x*cy,rx=xx*cr-y*sr,ry=xx*sr+y*cr,shade=i<2?.75f:1;
            q[i].x=center+(float)floor(rx*focal/z*16+.5f)/16;q[i].y=260-ry*focal/z;set_depth(&q[i],z);
            q[i].r=(color?45:240)*shade;q[i].g=(color?205:125)*shade;q[i].b=(color?230:60)*shade;
        }
        grDrawPolygonVertexList(4,q);
    }
}
static void precision(float left,float width)
{
    int row,column,n;float chip=(width-64)/2;
    for(row=0;row<4;++row)for(column=0;column<2;++column)for(n=0;n<2;++n) {
        int is_far=(column^reverse_order)?!n:n;
        float x=left+24+column*(chip+16),y=153+row*62,dist=precision_distance*(is_far?1+gaps[row]:1);
        if(is_far)flat_rect(x,y,chip,39,dist,1);
        else flat_rect(x+5,y+5,chip-10,29,dist,0);
    }
}
static void draw_scene(int fps,int hud)
{
    int pass,passes=split?2:1;char line[100];
    profile_begin();profile_stage(P_INPUT);grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0,0,0);
    background();profile_stage(P_BACKGROUND);
    for(pass=0;pass<passes;++pass) {
        int w=split?pass:w_mode,cmp=active_compare(w);float left=split?pass*320.0f:0,width=split?320:640;
        grClipWindow((int)left,88,(int)(left+width),432);clear_view(w,cmp);drawing_state(w,depth_enabled,depth_writes,cmp);
        if(precision_view)precision(left,width);else intersection(left+width*.5f,reverse_order);
    }
    profile_stage(P_OPAQUE);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grClipWindow(0,0,640,480);local_color();
    if(hud) {
        text(20,18,"3DFX / DEPTH BUFFER LAB",1,176,157,122);text(20,39,precision_view?"DEPTH / PRECISION":"DEPTH / INTERSECTIONS",2,233,224,202);
        sprintf(line,"FPS %d",fps);text(535,20,line,1,135,209,202);
        sprintf(line,"H TEST %s  W WRITE %s  G CMP %s  D ORDER %s",depth_enabled?"ON":"OFF",depth_writes?"ON":"OFF",compare_names[compare_mode],reverse_order?"REVERSED":"NORMAL");text(20,67,line,1,135,209,202);
        for(pass=0;pass<passes;++pass) {
            int w=split?pass:w_mode;float left=split?pass*320.0f:0,width=split?320:640;
            sprintf(line,"%s BUFFER / %s",w?"W FLOAT":"Z FIXED",compare_mode?compare_names[compare_mode]:(w?"LESS":"GREATER"));text(left+18,96,line,1,214,219,206);
            if(precision_view) {
                int row;float chip=(width-64)/2;
                text(left+24,121,reverse_order?"FAR FIRST":"NEAR FIRST",1,135,209,202);text(left+40+chip,121,reverse_order?"NEAR FIRST":"FAR FIRST",1,135,209,202);
                for(row=0;row<4;++row){sprintf(line,"GAP %.3f",precision_distance*gaps[row]);text(left+24,140+row*62,line,1,176,188,190);}
            }
        }
        if(precision_view)sprintf(line,"DIST %.0f  N Z RANGE %s  ORANGE NEAR / CYAN FAR",precision_distance,tight_range?"FITTED":"1-65535");
        else sprintf(line,"ORANGE AND CYAN INTERSECT  N Z RANGE %s  %s",tight_range?"3-10":"1-65535",moving?"MOVING":"PAUSED");
        text(20,420,line,1,176,157,122);
        text(20,447,"C SPLIT  B Z/W  H TEST  W WRITES  G COMPARE  D ORDER  P PRECISION",1,176,188,190);
        text(20,463,"N Z RANGE  +/- DISTANCE  SPACE MOTION  R RESET  TAB PROFILER  ESC",1,116,140,148);
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
    GrHwConfiguration hw;FILE *log=fopen("DEPTH.LOG","w");int ok=1,key,frames=0,fps=0,samples=0,stamp,synced=0;
    int test=argc>1 && (!strcmp(argv[1],"/test") || !strcmp(argv[1],"/softwarecheck")),bench=argc>1 && !strcmp(argv[1],"/bench");
    if(argc>1 && !strcmp(argv[1],"/softwarecheck"))save_captures=0;
    grGlideInit();if(!grSstQueryHardware(&hw) || hw.num_sst<1){ok=0;goto done;}grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){ok=0;goto done;}
    if(!load_textures()){ok=0;goto done;}
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grDitherMode(GR_DITHER_4x4);profile_init();profiler.measure=1;
    if(log)fprintf(log,"Voodoo: framebuffer %d MB, %d TMUs, TMU0 %d MB, texture bytes %lu; timer %d\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)texture_bytes,profiler.available);
    if(test){ok=tests(log);goto done;}if(bench){ok=benchmark(log);goto done;}stamp=rtc_seconds();
    for(;;) {
        float dt=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
        while(kbhit()) {
            key=getch();if(key==27)goto done;if(key==0 || key==224){getch();continue;}
            if(key=='c'||key=='C')split=!split;if(key=='b'||key=='B'){split=0;w_mode=!w_mode;}
            if(key=='h'||key=='H')depth_enabled=!depth_enabled;if(key=='w'||key=='W')depth_writes=!depth_writes;
            if(key=='g'||key=='G')compare_mode=(compare_mode+1)%9;if(key=='d'||key=='D')reverse_order=!reverse_order;
            if(key=='p'||key=='P')precision_view=!precision_view;if(key=='n'||key=='N')tight_range=!tight_range;
            if(key=='+'||key=='=')precision_distance=clamp(precision_distance*2,16,32768);if(key=='-')precision_distance=clamp(precision_distance/2,16,32768);
            if(key==' ')moving=!moving;
            if(key=='r'||key=='R'){split=depth_enabled=depth_writes=moving=1;w_mode=reverse_order=compare_mode=precision_view=tight_range=0;phase=.37f;precision_distance=4096;}
            if(key==9){profile_toggle();profiler_dirty=1;}
        }
        if(moving)phase+=dt;draw_scene(fps,1);++frames;++samples;
        {int now=rtc_seconds(),elapsed=(now-stamp+60)%60;if(elapsed){if(synced)fps=samples/elapsed;synced=1;samples=0;stamp=now;}}
    }
done:
    if(log){fprintf(log,"%s: Z/W depth lab, %d interactive frames\n",ok?"PASS":"FAIL",frames);fclose(log);}grGlideShutdown();return ok?0:1;
}
