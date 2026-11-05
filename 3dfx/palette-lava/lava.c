/* Fixed P8 texels, cycling palette ranges, one shared TMU palette. */
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
    grAlphaControlsITRGBLighting(FXFALSE);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
}
#include "../textures/overlay.h"
static unsigned char lava_pixels[256*256],logo_pixels[256*128],strip_pixels[256*32];
static GuTexPalette base_palette,palette,full_bank[2];
static float sines[256],palette_phase,speed=24;
static int cycle_enabled=1,partial_updates=1,narrow_band,direction=1,bilinear=1,zoomed,force_update,last_step,save_captures=1;
static unsigned long palette_bytes,partial_calls,full_calls,frame_palette_bytes,p8_uploads,p8_bytes;
static int first_entry(void){return narrow_band?96:64;}
static int last_entry(void){return narrow_band?159:191;}
static FxU32 packed_rgb(int r,int g,int b){return ((FxU32)r<<16)|((FxU32)g<<8)|(FxU32)b;}
static int rtc_seconds(void)
{int mode,sec;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);return mode&4?sec:(sec>>4)*10+(sec&15);}
static int upload(GrTexInfo *info,FxU32 *dest,FxU32 *next,void *pixels)
{
    FxU32 bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,info);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    *dest=*next;info->data=pixels;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,info);*next+=bytes;info->data=NULL;return 1;
}
static void palette_full(void)
{
    int bank=full_calls&1;memcpy(&full_bank[bank],&palette,sizeof(palette));
    grTexDownloadTable(GR_TMU0,GR_TEXTABLE_PALETTE,&full_bank[bank]);++full_calls;palette_bytes+=1024;frame_palette_bytes+=1024;
}
static void palette_partial(int first,int last)
{
    /* The API takes the complete palette pointer; start/end are inclusive. */
    grTexDownloadTablePartial(GR_TMU0,GR_TEXTABLE_PALETTE,&palette,first,last);
    ++partial_calls;palette_bytes+=(last-first+1)*4;frame_palette_bytes+=(last-first+1)*4;
}
static void palette_restore(void)
{
    palette=base_palette;palette_phase=0;last_step=0;force_update=0;palette_full();
}
static void cycle_tick(float dt)
{
    int step,i;
    if(cycle_enabled)palette_phase+=dt*speed*direction;
    while(palette_phase>=128)palette_phase-=128;while(palette_phase<0)palette_phase+=128;
    step=(int)palette_phase;
    if(step==last_step && !force_update)return;
    for(i=first_entry();i<=last_entry();++i)palette.data[i]=base_palette.data[64+((i-64+step)&127)];
    if(partial_updates)palette_partial(first_entry(),last_entry());else palette_full();
    last_step=step;force_update=0;
}
static void generate_palette(void)
{
    int i;for(i=0;i<256;++i)sines[i]=(float)sin(i*(6.28318530718/256));
    for(i=0;i<64;++i)base_palette.data[i]=packed_rgb(8+(int)(i*.48f),5+(int)(i*.36f),4+(int)(i*.27f));
    for(i=0;i<128;++i) {
        float heat=.5f+.5f*sines[(i*2+64)&255],h2=heat*heat,h5=h2*h2*heat;
        base_palette.data[64+i]=packed_rgb((int)(60+195*sqrt(heat)),(int)(8+232*h2),(int)(1+116*h5));
    }
    for(i=0;i<32;++i){base_palette.data[192+i]=packed_rgb(12+i*2,35+i*4,52+i*5);base_palette.data[224+i]=packed_rgb(100+i*4,80+i*3,20+i*2);}
}
static void generate_indices(void)
{
    int x,y,i;
    for(y=0;y<256;++y)for(x=0;x<256;++x) {
        float wx=x+10*sines[(y*2)&255]+5*sines[(x+y)&255],wy=y+9*sines[x]+4*sines[(x-y+64)&255];
        float nearest=1e9f,second=1e9f,gap,dist;int site=0,value;
        for(i=0;i<16;++i) {
            float dx=wx-((i%4)*64+24+(i*37)%25),dy=wy-((i/4)*64+20+(i*19)%29),d;
            if(dx>128)dx-=256;if(dx< -128)dx+=256;if(dy>128)dy-=256;if(dy< -128)dy+=256;
            d=dx*dx+dy*dy;if(d<nearest){second=nearest;nearest=d;site=i;}else if(d<second)second=d;
        }
        dist=(float)sqrt(nearest);gap=(float)sqrt(second)-dist;
        if(gap<5.4f+1.4f*sines[(x-y)&255])value=64+(((int)(x*.27f+y*.19f+dist*3+site*11+6*sines[(x*2)&255]))&127);
        else value=8+(x*37+y*19+(x*y)%73)%48;
        lava_pixels[y*256+x]=(unsigned char)value;
    }
    for(y=0;y<128;++y)for(x=0;x<256;++x) {
        static const char sign[]="3DFX";int gx=(x-44)/7,gy=(y-39)/7,value=14+(x*13+y*7)%20;
        if(x<3 || x>252 || y<3 || y>124)value=224+(x+y)%32;
        if(x>=44 && x<212 && y>=39 && y<88 && gx%6<5 && (panel_glyph[sign[gx/6]-32][gy]&(1<<(4-gx%6))))value=64+((x/2+y*2)&127);
        logo_pixels[y*256+x]=(unsigned char)value;
    }
    for(y=0;y<32;++y)for(x=0;x<256;++x)strip_pixels[y*256+x]=(unsigned char)x;
}
static int load_textures(void)
{
    unsigned char *font=calloc(8192,1);unsigned short *reference=malloc(256UL*256*2);
    FxU32 next=grTexMinAddress(GR_TMU0);int i,x,y,ok=0;
    if(!font || !reference)goto done;
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(glyphs[i].row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    generate_palette();generate_indices();
    tex[0].smallLod=tex[0].largeLod=GR_LOD_256;tex[0].aspectRatio=GR_ASPECT_1x1;tex[0].format=GR_TEXFMT_P_8;
    tex[1]=tex[2]=tex[0];tex[1].aspectRatio=GR_ASPECT_2x1;tex[2].aspectRatio=GR_ASPECT_8x1;
    if(!upload(&tex[0],&address[0],&next,lava_pixels) || !upload(&tex[1],&address[1],&next,logo_pixels) || !upload(&tex[2],&address[2],&next,strip_pixels))goto done;
    p8_uploads=3;p8_bytes=65536+32768+8192;
    tex[4]=tex[0];tex[4].format=GR_TEXFMT_RGB_565;
    for(i=0;i<256UL*256;++i){FxU32 c=base_palette.data[lava_pixels[i]];reference[i]=rgb565((c>>16)&255,(c>>8)&255,c&255);}
    if(!upload(&tex[4],&address[4],&next,reference))goto done;
    tex[3]=tex[0];tex[3].aspectRatio=GR_ASPECT_8x1;tex[3].format=GR_TEXFMT_ALPHA_8;
    if(!upload(&tex[3],&address[3],&next,font))goto done;
    profiler_tex=tex[3];profiler_tex.aspectRatio=GR_ASPECT_1x2;profiler_tex.format=GR_TEXFMT_RGB_565;
    memset(profiler_pixels,0,sizeof(profiler_pixels));for(i=0;i<2;++i)if(!upload(&profiler_tex,&profiler_address[i],&next,profiler_pixels))goto done;
    palette_restore();texture_bytes=next-grTexMinAddress(GR_TMU0);ok=1;
done:free(font);free(reference);return ok;
}
static void sample_state(int texture)
{
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXFALSE);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);local_color();
    grTexSource(GR_TMU0,address[texture],GR_MIPMAPLEVELMASK_BOTH,&tex[texture]);
    grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grTexFilterMode(GR_TMU0,bilinear?GR_TEXTUREFILTER_BILINEAR:GR_TEXTUREFILTER_POINT_SAMPLED,bilinear?GR_TEXTUREFILTER_BILINEAR:GR_TEXTUREFILTER_POINT_SAMPLED);
    grHints(GR_HINT_STWHINT,GR_STWHINT_W_DIFF_TMU0);grFogMode(GR_FOG_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);grCullMode(GR_CULL_DISABLE);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static void image_quad(float x,float y,float w,float h,float u,float v,float us,float vs)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i){q[i].x=x+((i==1||i==2)?w:0);q[i].y=y+(i<2?0:h);q[i].oow=q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=u+((i==1||i==2)?us:0);q[i].tmuvtx[0].tow=v+(i<2?0:vs);q[i].r=q[i].g=q[i].b=q[i].a=255;}
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static void draw_scene(int fps,int hud,float dt)
{
    char line[112];float uv=zoomed?64:0,extent=zoomed?128:256;
    if(!profiler.active){profile_begin();frame_palette_bytes=0;}cycle_tick(dt);profile_stage(P_INPUT);
    grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0x0018140d,0,0);
    local_color();grDepthMask(FXFALSE);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    color_quad(20,77,600,1,51,56,56);color_quad(20,437,600,1,51,56,56);profile_stage(P_BACKGROUND);
    sample_state(0);image_quad(32,124,256,256,uv,uv,extent,extent);
    sample_state(1);image_quad(352,114,256,128,0,0,256,128);
    sample_state(4);image_quad(416,262,128,128,uv,uv,extent,extent);
    sample_state(2);image_quad(20,410,600,14,0,0,256,32);profile_stage(P_OPAQUE);
    local_color();grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    if(hud) {
        text(20,18,"3DFX / PARTIAL PALETTE UPDATES",1,176,157,122);text(20,39,"LAVA / PALETTE CYCLING",2,233,224,202);
        sprintf(line,"FPS %d",fps);text(535,20,line,1,135,209,202);
        sprintf(line,"%s  %.0f STEPS/SEC  %s  %s UPLOAD",cycle_enabled?"CYCLING":"PAUSED",speed,direction>0?"FORWARD":"REVERSE",partial_updates?"PARTIAL":"FULL");text(20,66,line,1,135,209,202);
        sprintf(line,"0 TEXEL B/FRAME  LAST %lu PALETTE B  TOTAL %lu B",frame_palette_bytes,palette_bytes);text(20,88,line,1,176,188,190);
        text(20,108,"FIXED P8 DATA / LIVE PALETTE",1,233,224,202);text(352,99,"SECOND P8 / SAME PALETTE",1,233,224,202);
        text(352,249,"RGB565 / FROZEN STARTUP COLOURS",1,176,188,190);
        sprintf(line,"PALETTE 0-255 / ACTIVE %d-%d / %d B PER PARTIAL STEP",first_entry(),last_entry(),(last_entry()-first_entry()+1)*4);text(20,396,line,1,176,157,122);
        color_quad(20+first_entry()*(600.0f/256),426,(last_entry()-first_entry()+1)*(600.0f/256),2,135,209,202);
        text(20,447,"SPACE PAUSE  N STEP  +/- SPEED  D REVERSE  C FULL/PARTIAL  G BAND",1,176,188,190);
        text(20,463,"B FILTER  Z CROP  R RESET  TAB PROFILER  ESC EXIT",1,116,140,148);
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
    GrHwConfiguration hw;FILE *log=fopen("LAVA.LOG","w");int ok=1,key,frames=0,fps=0,samples_count=0,stamp,synced=0;
    int test=argc>1 && (!strcmp(argv[1],"/test") || !strcmp(argv[1],"/softwarecheck")),bench=argc>1 && !strcmp(argv[1],"/bench");
    if(argc>1 && !strcmp(argv[1],"/softwarecheck"))save_captures=0;
    grGlideInit();if(!grSstQueryHardware(&hw) || hw.num_sst<1){ok=0;goto done;}grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){ok=0;goto done;}
    if(!load_textures()){ok=0;goto done;}
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grDitherMode(GR_DITHER_4x4);profile_init();profiler.measure=1;
    if(log)fprintf(log,"Voodoo: framebuffer %d MB, %d TMUs, TMU0 %d MB, texture bytes %lu; timer %d; startup P8 uploads %lu (%lu bytes)\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)texture_bytes,profiler.available,p8_uploads,p8_bytes);
    if(test){ok=tests(log);goto done;}if(bench){ok=benchmark(log);goto done;}stamp=rtc_seconds();
    for(;;) {
        float dt=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;profile_begin();frame_palette_bytes=0;
        while(kbhit()) {
            key=getch();if(key==27)goto done;if(key==0 || key==224){getch();continue;}
            if(key==' ')cycle_enabled=!cycle_enabled;if(key=='n'||key=='N')palette_phase+=direction;
            if(key=='+'||key=='=')speed=clamp(speed+4,0,64);if(key=='-')speed=clamp(speed-4,0,64);
            if(key=='d'||key=='D')direction=-direction;if(key=='c'||key=='C'){partial_updates=!partial_updates;force_update=1;}
            if(key=='g'||key=='G'){narrow_band=!narrow_band;force_update=1;}
            if(key=='b'||key=='B')bilinear=!bilinear;if(key=='z'||key=='Z')zoomed=!zoomed;
            if(key=='r'||key=='R'){cycle_enabled=partial_updates=bilinear=direction=1;narrow_band=zoomed=0;speed=24;palette_restore();}
            if(key==9){profile_toggle();profiler_dirty=1;}
        }
        draw_scene(fps,1,dt);++frames;++samples_count;
        {int now=rtc_seconds(),elapsed=(now-stamp+60)%60;if(elapsed){if(synced)fps=samples_count/elapsed;synced=1;samples_count=0;stamp=now;}}
    }
done:
    if(log){fprintf(log,"%s: palette-cycled lava, %d interactive frames\n",ok?"PASS":"FAIL",frames);fclose(log);}grGlideShutdown();return ok?0:1;
}
