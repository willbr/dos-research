/* CPU fire, full-level versus inclusive changed-row texture uploads. */
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
#define FW 128
#define FH 128
#define ROW_BYTES (FW*2)
#define LEVEL_BYTES (FW*FH*2UL)
typedef struct {unsigned char heat[2][FW*FH];unsigned long random;int front,steps,first,top;} FireState;
static FireState states[4];
static unsigned short pixels[FW*FH],uploaded[FW*FH],heat_colors[256],combined_colors[7][256];
static unsigned char base_kind[FW*FH];
static float sine_table[256],accumulator,step_rate=30;
static int running=1,band_mode,band_index=1,split=1,single_path=1,swapped,bilinear=1,zoomed,manual_steps,sync_pending,save_captures=1;
static int dirty_first=-1,dirty_last=-1,changed_rows,uploads_first=-1,uploads_last=-1;
static unsigned long path_bytes[2],path_calls[2],frame_bytes[2],partial_row_calls,simulation_steps;
static const int band_sizes[3]={16,32,64};
static FireState *active_state(void){return &states[band_mode?band_index+1:0];}
static int rtc_seconds(void)
{int mode,sec;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);return mode&4?sec:(sec>>4)*10+(sec&15);}
static int upload(GrTexInfo *info,FxU32 *dest,FxU32 *next,void *data)
{
    FxU32 bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,info);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1 || ((*next&0x1fffffUL)+bytes)>0x200000UL)return 0;
    *dest=*next;info->data=data;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,info);*next+=bytes;info->data=NULL;return 1;
}
static unsigned long fire_random(FireState *s)
{s->random=s->random*1664525UL+1013904223UL;return s->random;}
static void simulate(FireState *s)
{
    unsigned char *src=s->heat[s->front],*dst=s->heat[1-s->front];int y,x,wind=(s->steps/35)%3-1,scale=FH/(FH-s->first),first=s->top-2,new_top=FH-1;
    if(first<s->first)first=s->first;
    /* Each cell reads at most two rows below it. Earlier rows cannot receive
       heat this step; clear the destination prefix to prevent stale cells. */
    memset(dst,0,first*FW);
    for(y=first;y<FH-1;++y) {
      int row_heat=0;
      for(x=0;x<FW;++x) {
        int sx=(x+wind+FW)&(FW-1),below=(y+1)*FW,below2=(y+2<FH?y+2:FH-1)*FW;
        int average=(src[below+sx]+src[below+((sx-1)&(FW-1))]+src[below+((sx+1)&(FW-1))]+src[below2+sx])/4;
        int cooling=2*scale+((fire_random(s)>>16)&3),value=average>cooling?average-cooling:0;dst[y*FW+x]=(unsigned char)value;row_heat|=value;
      }
      if(row_heat && new_top==FH-1)new_top=y;
    }
    for(x=0;x<FW;++x) {
        int fuel=(int)(155+65*sine_table[(x*5+s->steps*3)&255]+35*sine_table[(x*11-s->steps*5)&255])+((fire_random(s)>>16)&31)-15;
        if(fuel<0)fuel=0;if(fuel>255)fuel=255;dst[(FH-1)*FW+x]=(unsigned char)fuel;
    }
    s->front=1-s->front;s->top=new_top;++s->steps;
}
static void initialize_states(void)
{
    int state,i;for(state=0;state<4;++state) {
        FireState *s=&states[state];memset(s,0,sizeof(*s));s->random=0x4631UL+state*173;s->first=state?FH-band_sizes[state-1]:0;s->top=FH-1;
        for(i=0;i<96;++i)simulate(s);
    }
}
static void generate_tables(void)
{
    int h,x,y;
    for(h=0;h<256;++h) {
        int r,g,b;sine_table[h]=(float)sin(h*(6.28318530718/256));
        if(h<96){r=h*3;if(r>255)r=255;g=b=0;}
        else if(h<192){r=255;g=(h-96)*255/95;b=0;}
        else {r=g=255;b=(h-192)*255/63;}
        heat_colors[h]=rgb565(r,g,b);
    }
    for(y=0;y<FH;++y)for(x=0;x<FW;++x) {
        static const char sign[]="3DFX";int kind=(x*17+y*13)%6,gx=(x-28)/3,gy=(y-21)/3;
        if(x>=28 && x<100 && y>=21 && y<42 && gx%6<5 && (panel_glyph[sign[gx/6]-32][gy]&(1<<(4-gx%6))))kind=6;
        base_kind[y*FW+x]=(unsigned char)kind;
    }
    for(x=0;x<7;++x)for(h=0;h<256;++h) {
        unsigned short a=heat_colors[h],b=x==6?rgb565(38,49,59):rgb565(4+x,7+x,10+x);
        int r=(a>>11)&31,g=(a>>5)&63,blue=a&31;
        if(r<(int)((b>>11)&31))r=(b>>11)&31;if(g<(int)((b>>5)&63))g=(b>>5)&63;if(blue<(int)(b&31))blue=b&31;
        combined_colors[x][h]=(unsigned short)((r<<11)|(g<<5)|blue);
    }
}
static void map_rgb(void)
{
    FireState *s=active_state();int i;
    for(i=0;i<FW*FH;++i)pixels[i]=combined_colors[base_kind[i]][s->heat[s->front][i]];
}
static void find_dirty(void)
{
    int y;dirty_first=dirty_last=-1;changed_rows=0;
    for(y=0;y<FH;++y)if(memcmp(pixels+y*FW,uploaded+y*FW,ROW_BYTES)) {if(dirty_first<0)dirty_first=y;dirty_last=y;++changed_rows;}
}
static void whole_upload(int path)
{
    grTexDownloadMipMapLevel(GR_TMU0,address[path],GR_LOD_128,GR_LOD_128,GR_ASPECT_1x1,GR_TEXFMT_RGB_565,GR_MIPMAPLEVELMASK_BOTH,pixels);
    path_bytes[path]+=LEVEL_BYTES;frame_bytes[path]+=LEVEL_BYTES;++path_calls[path];
}
static void rows_upload(int first,int last)
{
    unsigned long bytes=(last-first+1)*ROW_BYTES;
    /* Destination stays at the mip level's base. Source starts at first row. */
    grTexDownloadMipMapLevelPartial(GR_TMU0,address[1],GR_LOD_128,GR_LOD_128,GR_ASPECT_1x1,GR_TEXFMT_RGB_565,GR_MIPMAPLEVELMASK_BOTH,pixels+first*FW,first,last);
    path_bytes[1]+=bytes;frame_bytes[1]+=bytes;++path_calls[1];++partial_row_calls;
}
static void publish_frame(void)
{
    map_rgb();find_dirty();whole_upload(0);
    if(dirty_first>=0)rows_upload(dirty_first,dirty_last);
    uploads_first=dirty_first;uploads_last=dirty_last;memcpy(uploaded,pixels,sizeof(pixels));
}
static void fire_tick(float dt)
{
    int steps=manual_steps,n=0;manual_steps=0;
    if(running)accumulator+=dt*step_rate;
    while(accumulator>=1 && n<8){++steps;++n;accumulator-=1;}
    for(n=0;n<steps;++n){simulate(active_state());++simulation_steps;}
    if(steps || sync_pending){publish_frame();sync_pending=0;}
}
static int load_textures(void)
{
    unsigned char *font=calloc(8192,1);FxU32 next=grTexMinAddress(GR_TMU0);int i,x,y,ok=0;
    if(!font)return 0;
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(glyphs[i].row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    generate_tables();initialize_states();map_rgb();memcpy(uploaded,pixels,sizeof(pixels));
    tex[0].smallLod=tex[0].largeLod=GR_LOD_128;tex[0].aspectRatio=GR_ASPECT_1x1;tex[0].format=GR_TEXFMT_RGB_565;tex[1]=tex[0];
    for(i=0;i<2;++i){if(!upload(&tex[i],&address[i],&next,pixels))goto done;path_bytes[i]=LEVEL_BYTES;path_calls[i]=1;}
    tex[3]=tex[0];tex[3].smallLod=tex[3].largeLod=GR_LOD_256;tex[3].aspectRatio=GR_ASPECT_8x1;tex[3].format=GR_TEXFMT_ALPHA_8;
    if(!upload(&tex[3],&address[3],&next,font))goto done;
    profiler_tex=tex[3];profiler_tex.aspectRatio=GR_ASPECT_1x2;profiler_tex.format=GR_TEXFMT_RGB_565;
    memset(profiler_pixels,0,sizeof(profiler_pixels));for(i=0;i<2;++i)if(!upload(&profiler_tex,&profiler_address[i],&next,profiler_pixels))goto done;
    texture_bytes=next-grTexMinAddress(GR_TMU0);ok=1;
done:free(font);return ok;
}
static void sample_state(int path)
{
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXFALSE);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);local_color();
    grTexSource(GR_TMU0,address[path],GR_MIPMAPLEVELMASK_BOTH,&tex[path]);grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);grTexFilterMode(GR_TMU0,bilinear?GR_TEXTUREFILTER_BILINEAR:GR_TEXTUREFILTER_POINT_SAMPLED,bilinear?GR_TEXTUREFILTER_BILINEAR:GR_TEXTUREFILTER_POINT_SAMPLED);
    grHints(GR_HINT_STWHINT,GR_STWHINT_W_DIFF_TMU0);grFogMode(GR_FOG_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);grCullMode(GR_CULL_DISABLE);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static void image_quad(float x,float y,float size)
{
    GrVertex q[4];int i;float u=zoomed?64:0,v=zoomed?128:0,span=zoomed?128:256;memset(q,0,sizeof(q));
    for(i=0;i<4;++i){q[i].x=x+((i==1||i==2)?size:0);q[i].y=y+(i<2?0:size);q[i].oow=q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=u+((i==1||i==2)?span:0);q[i].tmuvtx[0].tow=v+(i<2?0:span);q[i].r=q[i].g=q[i].b=q[i].a=255;}
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static void draw_scene(int fps,int hud,float dt)
{
    int pass,count=split?2:1;char line[112];
    if(!profiler.active){profile_begin();frame_bytes[0]=frame_bytes[1]=0;}fire_tick(dt);profile_stage(P_INPUT);
    grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0x0018140d,0,0);
    local_color();grDepthMask(FXFALSE);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    color_quad(20,77,600,1,51,56,56);color_quad(20,437,600,1,51,56,56);profile_stage(P_BACKGROUND);
    for(pass=0;pass<count;++pass){int path=split?(pass^swapped):single_path;sample_state(path);image_quad(split?(pass?352:32):192,132,256);}
    profile_stage(P_OPAQUE);local_color();grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    if(hud) {
        text(20,18,"3DFX / PARTIAL TEXTURE ROW UPLOADS",1,176,157,122);text(20,39,"CPU FIRE / ROW UPLOADS",2,233,224,202);
        sprintf(line,"FPS %d",fps);text(535,20,line,1,135,209,202);
        sprintf(line,"B %s  G BAND %d ROWS  %.0f STEPS/SEC  %s",band_mode?"CONTROLLED BAND":"FULL FIRE",band_sizes[band_index],step_rate,running?"RUNNING":"PAUSED");text(20,67,line,1,135,209,202);
        text(20,88,"SAME CPU IMAGE / TWO TEXTURE ADDRESSES / 0 PALETTE BYTES",1,176,188,190);
        for(pass=0;pass<count;++pass) {
            int path=split?(pass^swapped):single_path;float left=split?pass*320.0f:160;
            text(left+20,108,path?"CHANGED ROW SPAN":"FULL LEVEL / 32768 B PER UPDATE",1,233,224,202);
            if(path && uploads_first>=0) {
                float top=clamp((uploads_first-(zoomed?64:0))*(zoomed?4.0f:2),0,256),bottom=clamp((uploads_last+1-(zoomed?64:0))*(zoomed?4.0f:2),0,256);
                color_quad(left+292,132+top,3,bottom-top,135,209,202);
            }
        }
        sprintf(line,"LAST DIRTY %d-%d / %d ROWS CHANGED / %d ROWS IN SPAN",dirty_first,dirty_last,changed_rows,dirty_first<0?0:dirty_last-dirty_first+1);text(20,402,line,1,176,157,122);
        sprintf(line,"FRAME FULL %lu B / ROWS %lu B",frame_bytes[0],frame_bytes[1]);text(20,419,line,1,135,209,202);
        text(20,447,"B FULL/BAND G BAND SIZE C SPLIT U FULL/ROWS S SWAP N STEP",1,176,188,190);
        text(20,463,"+/- RATE SPACE PAUSE T FILTER Z CROP R RESET TAB PROF ESC",1,116,140,148);
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
    GrHwConfiguration hw;FILE *log=fopen("FIRE.LOG","w");int ok=1,key,frames=0,fps=0,samples_count=0,stamp,synced=0;
    int test=argc>1 && (!strcmp(argv[1],"/test") || !strcmp(argv[1],"/softwarecheck")),bench=argc>1 && !strcmp(argv[1],"/bench");
    if(argc>1 && !strcmp(argv[1],"/softwarecheck"))save_captures=0;
    grGlideInit();if(!grSstQueryHardware(&hw) || hw.num_sst<1){ok=0;goto done;}grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){ok=0;goto done;}
    if(!load_textures()){ok=0;goto done;}
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grDitherMode(GR_DITHER_4x4);profile_init();profiler.measure=1;
    if(log)fprintf(log,"Voodoo: framebuffer %d MB, %d TMUs, TMU0 %d MB, texture bytes %lu; timer %d; level bases %lu/%lu, bytes %lu\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)texture_bytes,profiler.available,(unsigned long)address[0],(unsigned long)address[1],LEVEL_BYTES);
    if(test){ok=tests(log);goto done;}if(bench){ok=benchmark(log);goto done;}stamp=rtc_seconds();
    for(;;) {
        float dt=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;profile_begin();frame_bytes[0]=frame_bytes[1]=0;
        while(kbhit()) {
            key=getch();if(key==27)goto done;if(key==0 || key==224){getch();continue;}
            if(key==' ')running=!running;if(key=='n'||key=='N')++manual_steps;
            if(key=='b'||key=='B'){band_mode=!band_mode;sync_pending=1;}if(key=='g'||key=='G'){band_index=(band_index+1)%3;if(band_mode)sync_pending=1;}
            if(key=='c'||key=='C')split=!split;if(key=='u'||key=='U'){single_path=!single_path;split=0;}if(key=='s'||key=='S')swapped=!swapped;
            if(key=='+'||key=='=')step_rate=clamp(step_rate+15,0,60);if(key=='-')step_rate=clamp(step_rate-15,0,60);
            if(key=='t'||key=='T')bilinear=!bilinear;if(key=='z'||key=='Z')zoomed=!zoomed;
            if(key=='r'||key=='R'){initialize_states();running=split=bilinear=single_path=1;band_mode=swapped=zoomed=manual_steps=0;band_index=1;accumulator=0;step_rate=30;sync_pending=1;}
            if(key==9){profile_toggle();profiler_dirty=1;}
        }
        draw_scene(fps,1,dt);++frames;++samples_count;
        {int now=rtc_seconds(),elapsed=(now-stamp+60)%60;if(elapsed){if(synced)fps=samples_count/elapsed;synced=1;samples_count=0;stamp=now;}}
    }
done:
    if(log){fprintf(log,"%s: CPU fire and texture-row uploads, %d interactive frames\n",ok?"PASS":"FAIL",frames);fclose(log);}grGlideShutdown();return ok?0:1;
}
