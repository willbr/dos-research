/* RGB565, indexed and YIQ/NCC textures produced by the vendored TexUS. */
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
typedef struct {
    GrTexInfo info,error_info;GuTexTable table;FxU32 address,error_address,bytes;
    unsigned char *raw,*rgb;float mean_error;int max_error;
} Sample;
static Sample samples[2][3];
static GrTexInfo code_info[2];static FxU32 code_address[2];
static int palette_loaded=-1,palette_uploads,ncc_uploads,codec_uploads,ncc_selections;
static int gallery=1,format_selected,image_selected,table_view,error_view,zoomed,bilinear,wrong_ncc,wrong_palette,moving,save_captures=1;
static int opengl_diagnostic,diagnostic_limits;
static float phase,pan_u=96,pan_v=80;
static const char *format_names[3]={"RGB565","P8 / 256 COLOURS","YIQ / NCC"};
static const char *file_names[2][3]={{"assets/A565.3DF","assets/AP8.3DF","assets/AYIQ.3DF"},{"assets/B565.3DF","assets/BP8.3DF","assets/BYIQ.3DF"}};
static int rtc_seconds(void)
{int mode,sec;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);return mode&4?sec:(sec>>4)*10+(sec&15);}
static int upload(GrTexInfo *info,FxU32 *dest,FxU32 *next,void *pixels)
{
    FxU32 bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,info);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    *dest=*next;info->data=pixels;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,info);*next+=bytes;info->data=NULL;return 1;
}
static void table_rgb(const Sample *s,int code,int format,int *rgb)
{
    if(format==1) {
        FxU32 c=s->table.palette.data[code];rgb[0]=(c>>16)&255;rgb[1]=(c>>8)&255;rgb[2]=c&255;
    } else {
        int channel;const GuNccTable *n=&s->table.nccTable;
        for(channel=0;channel<3;++channel) {
            /* SDK tables may retain the hardware's signed 9-bit encoding. */
            int a=n->iRGB[(code>>2)&3][channel]&511,b=n->qRGB[code&3][channel]&511;
            if(a&256)a-=512;if(b&256)b-=512;
            rgb[channel]=(int)clamp(n->yRGB[code>>4]+a+b,0,255);
        }
    }
}
static int load_sample(int image,int format,FxU32 *next)
{
    Gu3dfInfo file;Sample *s=&samples[image][format];unsigned long i;int rgb[3];
    memset(&file,0,sizeof(file));if(!gu3dfGetInfo(file_names[image][format],&file))return 0;
    file.data=malloc(file.mem_required);if(!file.data)return 0;s->raw=file.data;
    if(!gu3dfLoad(file_names[image][format],&file))return 0;
    s->info.smallLod=file.header.small_lod;s->info.largeLod=file.header.large_lod;s->info.aspectRatio=file.header.aspect_ratio;s->info.format=file.header.format;
    if(s->info.smallLod!=GR_LOD_256 || s->info.largeLod!=GR_LOD_256 || file.header.width!=256 || file.header.height!=256)return 0;
    if(s->info.format!=(format==0?GR_TEXFMT_RGB_565:format==1?GR_TEXFMT_P_8:GR_TEXFMT_YIQ_422))return 0;
    s->table=file.table;s->bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&s->info);
    if(!upload(&s->info,&s->address,next,s->raw))return 0;++codec_uploads;
    s->rgb=malloc(256UL*256*3);if(!s->rgb)return 0;
    for(i=0;i<256UL*256;++i) {
        if(!format){unsigned short p=((unsigned short *)s->raw)[i];rgb[0]=((p>>11)&31)*255/31;rgb[1]=((p>>5)&63)*255/63;rgb[2]=(p&31)*255/31;}
        else table_rgb(s,s->raw[i],format,rgb);
        s->rgb[i*3]=(unsigned char)rgb[0];s->rgb[i*3+1]=(unsigned char)rgb[1];s->rgb[i*3+2]=(unsigned char)rgb[2];
    }
    return 1;
}
static int load_textures(void)
{
    unsigned char *font=calloc(8192,1),codes[256];unsigned short *errors=malloc(256UL*256*2);
    FxU32 next=grTexMinAddress(GR_TMU0);int i,x,y,image,format,ok=0;
    if(!font || !errors)goto done;
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(glyphs[i].row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    for(image=0;image<2;++image)for(format=0;format<3;++format)if(!load_sample(image,format,&next))goto done;
    for(image=0;image<2;++image)for(format=1;format<3;++format) {
        Sample *s=&samples[image][format],*ref=&samples[image][0];unsigned long pixel,total=0;
        for(pixel=0;pixel<256UL*256;++pixel) {
            int channel,d[3];for(channel=0;channel<3;++channel){d[channel]=abs(s->rgb[pixel*3+channel]-ref->rgb[pixel*3+channel]);total+=d[channel];if(d[channel]>s->max_error)s->max_error=d[channel];}
            errors[pixel]=rgb565((int)clamp(d[0]*4,0,255),(int)clamp(d[1]*4,0,255),(int)clamp(d[2]*4,0,255));
        }
        s->mean_error=total/(256.0f*256*3);s->error_info=s->info;s->error_info.format=GR_TEXFMT_RGB_565;
        if(!upload(&s->error_info,&s->error_address,&next,errors))goto done;
    }
    tex[3].smallLod=tex[3].largeLod=GR_LOD_256;tex[3].aspectRatio=GR_ASPECT_8x1;tex[3].format=GR_TEXFMT_ALPHA_8;
    if(!upload(&tex[3],&address[3],&next,font))goto done;
    code_info[0]=samples[0][1].info;code_info[0].smallLod=code_info[0].largeLod=GR_LOD_16;
    for(i=0;i<256;++i)codes[i]=(unsigned char)i;
    if(!upload(&code_info[0],&code_address[0],&next,codes))goto done;
    code_info[1]=code_info[0];code_info[1].format=GR_TEXFMT_YIQ_422;
    if(!upload(&code_info[1],&code_address[1],&next,codes))goto done;
    profiler_tex=tex[3];profiler_tex.aspectRatio=GR_ASPECT_1x2;profiler_tex.format=GR_TEXFMT_RGB_565;
    memset(profiler_pixels,0,sizeof(profiler_pixels));
    for(i=0;i<2;++i)if(!upload(&profiler_tex,&profiler_address[i],&next,profiler_pixels))goto done;
    for(image=0;image<2;++image){grTexDownloadTable(GR_TMU0,image?GR_TEXTABLE_NCC1:GR_TEXTABLE_NCC0,&samples[image][2].table.nccTable);++ncc_uploads;}
    texture_bytes=next-grTexMinAddress(GR_TMU0);ok=1;
done:free(font);free(errors);return ok;
}
static void palette_select(int image)
{
    if(palette_loaded!=image){grTexDownloadTable(GR_TMU0,GR_TEXTABLE_PALETTE,&samples[image][1].table.palette);palette_loaded=image;++palette_uploads;}
}
static void sample_state(int image,int format,int errors)
{
    Sample *s=&samples[image][format];
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXFALSE);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    local_color();grFogMode(GR_FOG_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);grCullMode(GR_CULL_DISABLE);
    grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grTexFilterMode(GR_TMU0,bilinear?GR_TEXTUREFILTER_BILINEAR:GR_TEXTUREFILTER_POINT_SAMPLED,bilinear?GR_TEXTUREFILTER_BILINEAR:GR_TEXTUREFILTER_POINT_SAMPLED);
    grHints(GR_HINT_STWHINT,GR_STWHINT_W_DIFF_TMU0);
    if(errors && format)grTexSource(GR_TMU0,s->error_address,GR_MIPMAPLEVELMASK_BOTH,&s->error_info);
    else {
        if(format==2){grTexNCCTable(GR_TMU0,(wrong_ncc?1-image:image)?GR_NCCTABLE_NCC1:GR_NCCTABLE_NCC0);++ncc_selections;}
        grTexSource(GR_TMU0,s->address,GR_MIPMAPLEVELMASK_BOTH,&s->info);
        if(format==1)palette_select(wrong_palette?1-image:image);
    }
    grColorCombine(errors && !format?GR_COMBINE_FUNCTION_ZERO:GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static void image_quad(float x,float y,float size,float u,float v,float extent)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i){q[i].x=x+((i==1||i==2)?size:0);q[i].y=y+(i<2?0:size);q[i].oow=q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=u+((i==1||i==2)?extent:0);q[i].tmuvtx[0].tow=v+(i<2?0:extent);q[i].r=q[i].g=q[i].b=q[i].a=255;}
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static void draw_scene(int fps,int hud)
{
    int pass,count=table_view?2:gallery?3:1;char line[112];float extent=zoomed?64:256;
    float u=zoomed?clamp(pan_u+(moving?24*(float)sin(phase*.5f):0),0,192):0,v=zoomed?clamp(pan_v+(moving?18*(float)cos(phase*.4f):0),0,192):0;
    profile_begin();profile_stage(P_INPUT);grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0x0018140d,0,0);
    /* The clear already fills the background. Avoid another full-screen
       shaded quad in the software rasterizer; only the dividers need draws. */
    local_color();grDepthMask(FXFALSE);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    color_quad(20,77,600,1,51,56,56);color_quad(20,437,600,1,51,56,56);profile_stage(P_BACKGROUND);
    for(pass=0;pass<count;++pass) {
        int image=table_view?pass:image_selected,format=table_view?2:gallery?pass:format_selected;
        float size=table_view?208:gallery?176:256,left=table_view?pass*320.0f:gallery?pass*(640.0f/3):0;
        float x=table_view?left+56:gallery?left+18:192,y=table_view?154:gallery?154:130;
        sample_state(image,format,!table_view && error_view);image_quad(x,y,size,u,v,extent);
    }
    profile_stage(P_OPAQUE);local_color();grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    if(hud) {
        text(20,18,"3DFX / TEXTURE FORMATS AND TABLES",1,176,157,122);text(20,39,"TEXTURE COMPRESSION",2,233,224,202);
        sprintf(line,"FPS %d",fps);text(535,20,line,1,135,209,202);
        sprintf(line,"T IMAGE %c  B %s  Z %s  E %s",'A'+image_selected,bilinear?"BILINEAR":"NEAREST",zoomed?"64 TEXELS":"FULL IMAGE",error_view && !table_view?"CPU ERROR X4":"TEXTURE");text(20,66,line,1,135,209,202);
        sprintf(line,"N NCC %s  P PALETTE %s  %d NCC UPLOADS  %d PALETTE UPLOADS",wrong_ncc?"WRONG":"CORRECT",wrong_palette?"WRONG":"CORRECT",ncc_uploads,palette_uploads);text(20,88,line,1,176,188,190);
        for(pass=0;pass<count;++pass) {
            int image=table_view?pass:image_selected,format=table_view?2:gallery?pass:format_selected;Sample *s=&samples[image][format];
            float left=table_view?pass*320.0f:gallery?pass*(640.0f/3):0,x=left+12;
            if(table_view)sprintf(line,"YIQ IMAGE %c / NCC SLOT %d",'A'+image,wrong_ncc?1-image:image);else strcpy(line,format_names[format]);text(x,108,line,1,233,224,202);
            sprintf(line,"%lu KIB TEXELS / %d B TABLE",(unsigned long)s->bytes/1024,format==0?0:format==1?1024:48);text(x,123,line,1,176,188,190);
            if(!table_view){sprintf(line,"MAE %.2f MAX %d",s->mean_error,s->max_error);text(x,360,line,1,176,188,190);}
        }
        text(20,400,table_view?"BOTH NCC TABLES RESIDENT / SLOT SWITCHES UPLOAD NO TEXELS":"ERRORS COMPARE CPU-DECODED RGB TO RGB565 / SAME SOURCE IMAGE",1,176,157,122);
        text(20,447,"1-3 FORMAT C GALLERY T IMAGE E ERROR Z ZOOM ARROWS PAN V NCC SLOTS",1,176,188,190);
        text(20,463,"N WRONG NCC P WRONG PALETTE B FILTER SPACE PAN R RESET TAB PROF ESC",1,116,140,148);
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
    GrHwConfiguration hw;FILE *log=fopen("COMPRESS.LOG","w");int ok=1,key,frames=0,fps=0,samples_count=0,stamp,synced=0,image,format;
    int test=argc>1 && (!strcmp(argv[1],"/test") || !strcmp(argv[1],"/softwarecheck") || !strcmp(argv[1],"/openglcheck")),bench=argc>1 && !strcmp(argv[1],"/bench");
    opengl_diagnostic=argc>1 && !strcmp(argv[1],"/openglcheck");if(opengl_diagnostic)save_captures=0;
    if(argc>1 && !strcmp(argv[1],"/softwarecheck"))save_captures=0;
    grGlideInit();if(!grSstQueryHardware(&hw) || hw.num_sst<1){ok=0;goto done;}grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){ok=0;goto done;}
    if(!load_textures()){ok=0;goto done;}
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grDitherMode(GR_DITHER_4x4);profile_init();profiler.measure=1;
    if(log) {
        fprintf(log,"Voodoo: framebuffer %d MB, %d TMUs, TMU0 %d MB, texture bytes %lu; timer %d\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)texture_bytes,profiler.available);
        for(image=0;image<2;++image)for(format=0;format<3;++format)fprintf(log,"Image %c %s: %lu texel bytes, decoded MAE %.3f max %d vs RGB565\n",'A'+image,format_names[format],(unsigned long)samples[image][format].bytes,samples[image][format].mean_error,samples[image][format].max_error);
    }
    if(test){ok=tests(log);goto done;}if(bench){ok=benchmark(log);goto done;}stamp=rtc_seconds();
    for(;;) {
        float dt=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
        while(kbhit()) {
            key=getch();if(key==27)goto done;
            if(key==0 || key==224){key=getch();if(key==75)pan_u=clamp(pan_u-4,0,192);if(key==77)pan_u=clamp(pan_u+4,0,192);if(key==72)pan_v=clamp(pan_v-4,0,192);if(key==80)pan_v=clamp(pan_v+4,0,192);continue;}
            if(key>='1' && key<='3'){format_selected=key-'1';gallery=table_view=0;}if(key=='c'||key=='C'){gallery=!gallery;table_view=0;}
            if(key=='t'||key=='T')image_selected=!image_selected;if(key=='e'||key=='E')error_view=!error_view;if(key=='z'||key=='Z')zoomed=!zoomed;
            if(key=='v'||key=='V')table_view=!table_view;if(key=='n'||key=='N')wrong_ncc=!wrong_ncc;if(key=='p'||key=='P')wrong_palette=!wrong_palette;
            if(key=='b'||key=='B')bilinear=!bilinear;if(key==' ')moving=!moving;
            if(key=='r'||key=='R'){gallery=1;format_selected=image_selected=table_view=error_view=zoomed=bilinear=wrong_ncc=wrong_palette=moving=0;phase=0;pan_u=96;pan_v=80;}
            if(key==9){profile_toggle();profiler_dirty=1;}
        }
        if(moving)phase+=dt;draw_scene(fps,1);++frames;++samples_count;
        {int now=rtc_seconds(),elapsed=(now-stamp+60)%60;if(elapsed){if(synced)fps=samples_count/elapsed;synced=1;samples_count=0;stamp=now;}}
    }
done:
    if(log){if(opengl_diagnostic)fprintf(log,"OpenGL diagnostic: %d recognized NCC cache limits; unexpected errors still fail.\n",diagnostic_limits);fprintf(log,"%s: %s, %d interactive frames\n",ok?"PASS":"FAIL",opengl_diagnostic?"OpenGL diagnostic; see LIMIT entries":"native texture compression gallery",frames);fclose(log);}grGlideShutdown();
    for(image=0;image<2;++image)for(format=0;format<3;++format){free(samples[image][format].raw);free(samples[image][format].rgb);}return ok?0:1;
}
