/* Native Glide filtering lab: identical samples, different sampler state. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
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
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
}
#include "../textures/overlay.h"
static int split=1,bilinear=1,swapped,oblique,moving=1;
static float phase,span=32,pan_u=41,pan_v=75;
static int software_check;
static int rtc_seconds(void)
{
    int mode,sec;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);
    return mode&4?sec:(sec>>4)*10+(sec&15);
}
static int upload(GrTexInfo *info,FxU32 *dest,FxU32 *next,void *pixels)
{
    FxU32 bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,info);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    *dest=*next;info->data=pixels;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,info);
    *next+=bytes;info->data=NULL;return 1;
}
static int load_textures(void)
{
    unsigned short *pixels=malloc(256UL*256*2),probe[4]={0xf800,0x07e0,0x001f,0xffff};
    unsigned char *rgb=malloc(256UL*256*3),*font=calloc(8192,1);
    FxU32 next=grTexMinAddress(GR_TMU0);int i,x,y,ok=0;FILE *f;
    if(!pixels || !rgb || !font)goto done;
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(glyphs[i].row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    tex[0].smallLod=tex[0].largeLod=GR_LOD_256;tex[0].aspectRatio=GR_ASPECT_1x1;tex[0].format=GR_TEXFMT_RGB_565;
    tex[1]=tex[0];
    for(y=0;y<256;++y)for(x=0;x<256;++x)pixels[y*256+x]=((x^y)&1)?0xffff:0;
    if(!upload(&tex[0],&address[0],&next,pixels))goto done;
    f=fopen("D:/STONE.RGB","rb");if(!f)goto done;
    i=fread(rgb,1,256UL*256*3,f)==256UL*256*3;fclose(f);if(!i)goto done;
    for(i=0;i<256UL*256;++i)pixels[i]=rgb565(rgb[i*3],rgb[i*3+1],rgb[i*3+2]);
    if(!upload(&tex[1],&address[1],&next,pixels))goto done;
    tex[3]=tex[0];tex[3].aspectRatio=GR_ASPECT_8x1;tex[3].format=GR_TEXFMT_ALPHA_8;
    if(!upload(&tex[3],&address[3],&next,font))goto done;
    probe_texture=tex[0];probe_texture.smallLod=probe_texture.largeLod=GR_LOD_2;
    if(!upload(&probe_texture,&probe_address,&next,probe))goto done;
    profiler_tex=tex[0];profiler_tex.aspectRatio=GR_ASPECT_1x2;
    memset(profiler_pixels,0,sizeof(profiler_pixels));
    for(i=0;i<2;++i)if(!upload(&profiler_tex,&profiler_address[i],&next,profiler_pixels))goto done;
    texture_bytes=next-grTexMinAddress(GR_TMU0);ok=1;
done:
    free(pixels);free(rgb);free(font);return ok;
}
static void sampler(int material,int min_filter,int mag_filter)
{
    grTexSource(GR_TMU0,address[material],GR_MIPMAPLEVELMASK_BOTH,&tex[material]);
    grTexFilterMode(GR_TMU0,min_filter?GR_TEXTUREFILTER_BILINEAR:GR_TEXTUREFILTER_POINT_SAMPLED,mag_filter?GR_TEXTUREFILTER_BILINEAR:GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
    grHints(GR_HINT_STWHINT,0);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grDepthMask(FXFALSE);
    grFogMode(GR_FOG_DISABLE);grCullMode(GR_CULL_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static void sample_quad(float x,float y,float width,float height,float extent,float u,float v,int angled)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i) {
        float sx=(i==1||i==2)?1:-1,sy=i<2?-1:1;
        float depth=3+(angled?sx*.8f:0),inv=1/depth;
        q[i].x=x+width*.5f+sx*width*.39f*3*inv;
        q[i].y=y+height*.5f+sy*height*.36f*3*inv;
        q[i].oow=q[i].tmuvtx[0].oow=inv;q[i].ooz=65535*.45f*inv;
        q[i].tmuvtx[0].sow=(u+(sx+1)*extent*.5f)*inv;
        q[i].tmuvtx[0].tow=(v+(sy+1)*extent*.5f)*inv;
        q[i].r=q[i].g=q[i].b=q[i].a=255;
    }
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static void draw_scene(int fps,int hud)
{
    int column,row,columns=split?2:1;char line[96];
    profile_begin();profile_stage(P_INPUT);grClipWindow(0,0,640,480);grDepthMask(FXTRUE);grBufferClear(0,0,0);
    background();profile_stage(P_BACKGROUND);
    for(column=0;column<columns;++column) {
        int filter=split?(column^swapped):bilinear;
        float left=split?column*320.0f:0,width=split?320.0f:640;
        grClipWindow((int)left+12,94,(int)(left+width)-12,421);
        for(row=0;row<2;++row) {
            sampler(row,filter,filter);
            sample_quad(left+12,114+row*152,width-24,144,span,pan_u+phase*.7f,pan_v+phase*.43f,oblique);
        }
    }
    profile_stage(P_OPAQUE);grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);local_color();
    if(hud) {
        text(20,18,"3DFX / FILTERING",1,176,157,122);
        text(20,39,"NEAREST AND BILINEAR",2,233,224,202);
        sprintf(line,"FPS %d",fps);text(535,20,line,1,135,209,202);
        sprintf(line,"%s  %.0f TEXELS  %s  %s",span<256?"MAGNIFY":"MINIFY",span,oblique?"OBLIQUE":"FRONT",moving?"MOVING":"PAUSED");text(20,66,line,1,135,209,202);
        for(column=0;column<columns;++column) {
            int filter=split?(column^swapped):bilinear;float left=split?column*320.0f:0;
            text(left+24,96,filter?"BILINEAR / SAME SAMPLES":"NEAREST / SAME SAMPLES",1,214,219,206);
            text(left+24,119,"CHECKERBOARD",1,135,209,202);text(left+24,271,"STONE",1,135,209,202);
        }
        text(20,447,"C SPLIT  B FILTER  S SWAP  V MAG/MIN  O OBLIQUE  SPACE MOTION",1,176,188,190);
        text(20,463,"+/- TEXELS  ARROWS PAN  R RESET  TAB PROFILER  ESC EXIT",1,116,140,148);
    }
    profile_stage(P_HUD);if(hud)profiler_panel();profile_stage(P_OVERLAY);
    while(grSstVRetraceOn()){}while(!grSstVRetraceOn()){}
    grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();profile_stage(P_PRESENT);profile_end();
}
static unsigned short *capture(const char *name)
{
    unsigned short *p=malloc(640UL*480*2);FILE *f;unsigned long i;
    if(!p || !grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,640,480,1280,p)){free(p);return NULL;}
    if(!name || software_check)return p;f=fopen(name,"wb");if(!f){free(p);return NULL;}
    fprintf(f,"P6\n640 480\n255\n");
    for(i=0;i<640UL*480;++i){unsigned char c[3];c[0]=((p[i]>>11)&31)*255/31;c[1]=((p[i]>>5)&63)*255/63;c[2]=(p[i]&31)*255/31;fwrite(c,1,3,f);}
    if(fclose(f)){free(p);return NULL;}return p;
}
#include "tests.h"
int main(int argc,char **argv)
{
    GrHwConfiguration hw;FILE *log=fopen("FILTER.LOG","w");int ok=1,key,frames=0,fps=0,samples=0,stamp,synced=0;
    int test=argc>1 && (!strcmp(argv[1],"/test") || !strcmp(argv[1],"/softwarecheck")),bench=argc>1 && !strcmp(argv[1],"/bench");
    software_check=argc>1 && !strcmp(argv[1],"/softwarecheck");
    grGlideInit();
    if(!grSstQueryHardware(&hw) || hw.num_sst<1){ok=0;goto done;}
    grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){ok=0;goto done;}
    if(!load_textures()){ok=0;goto done;}
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grDitherMode(GR_DITHER_4x4);profile_init();profiler.measure=1;
    if(log)fprintf(log,"Voodoo: framebuffer %d MB, %d TMUs, TMU0 %d MB, texture bytes %lu; timer %d\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)texture_bytes,profiler.available);
    if(test){ok=tests(log);goto done;}if(bench){ok=benchmark(log);goto done;}
    stamp=rtc_seconds();
    for(;;) {
        float dt=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
        while(kbhit()) {
            key=getch();if(key==27)goto done;
            if(key==0 || key==224){key=getch();if(key==75)pan_u-=1;if(key==77)pan_u+=1;if(key==72)pan_v-=1;if(key==80)pan_v+=1;continue;}
            if(key=='c'||key=='C')split=!split;if(key=='b'||key=='B'){if(split)split=0;bilinear=!bilinear;}
            if(key=='s'||key=='S')swapped=!swapped;if(key=='o'||key=='O')oblique=!oblique;
            if(key=='v'||key=='V')span=span<256?1024:32;if(key==' ')moving=!moving;
            if(key=='+'||key=='=')span=clamp(span*1.25f,8,2048);if(key=='-')span=clamp(span/1.25f,8,2048);
            if(key=='r'||key=='R'){span=32;phase=0;pan_u=41;pan_v=75;oblique=swapped=0;split=moving=1;}
            if(key==9){profile_toggle();profiler_dirty=1;}
        }
        if(moving)phase+=dt;draw_scene(fps,1);++frames;++samples;
        {int now=rtc_seconds(),elapsed=(now-stamp+60)%60;if(elapsed){if(synced)fps=samples/elapsed;synced=1;samples=0;stamp=now;}}
    }
done:
    if(log){fprintf(log,"%s: native filtering lab, %d interactive frames\n",ok?"PASS":"FAIL",frames);fclose(log);}
    grGlideShutdown();return ok?0:1;
}
