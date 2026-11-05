/* Fixed-function RGB and alpha equations on one shared textured object. */
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
    grAlphaControlsITRGBLighting(FXFALSE);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
}
#include "../textures/overlay.h"
#define COLS 13
#define ROWS 9
#define VERTS (COLS*ROWS)
typedef struct {float x,y,z,u,v,r,g,b;} OrbVertex;
static OrbVertex source[VERTS],transformed[VERTS];
static int visible_cells[96],visible_count,finite_error,scene_draws;
static int gallery=1,color_mode,alpha_mode,blend_enabled,moving=1,tint,material,save_captures=1;
static float phase,light_gain=.65f,vertex_alpha=.7f;
static int constant_alpha=128;
static const int constant_colors[3][3]={{32,80,192},{200,85,35},{35,170,90}};
static const char *tint_names[3]={"BLUE","COPPER","GREEN"};
static const char *color_labels[4]={"1 REPLACE  T","2 MODULATE  T*L","3 ADD  CLAMP(T+L)","4 INTERP  CA*T+(1-CA)*C"};
static const char *alpha_labels[5]={"OPAQUE VA=1","REPLACE TA","MODULATE TA*CA","ADD CLAMP(TA+CA)","INTERP TA*VA+(1-TA)*CA"};
static int rtc_seconds(void)
{int mode,sec;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);return mode&4?sec:(sec>>4)*10+(sec&15);}
static int upload(GrTexInfo *info,FxU32 *dest,FxU32 *next,void *pixels)
{
    FxU32 bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,info);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    *dest=*next;info->data=pixels;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,info);*next+=bytes;info->data=NULL;return 1;
}
static void font_glyph(unsigned char *font,const Glyph *g)
{
    int slot=g->c-32,x,y;memcpy(panel_glyph[slot],g->row,7);
    for(y=0;y<7;++y)for(x=0;x<5;++x)if(g->row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
}
static int load_textures(void)
{
    static const Glyph math_glyphs[]={{'*',{0,4,21,14,21,4,0}},{'=',{0,0,31,0,31,0,0}},{'(',{2,4,8,8,8,4,2}},{')',{8,4,2,2,2,4,8}},{',',{0,0,0,0,12,12,8}},{'%',{25,26,2,4,8,11,19}}};
    unsigned short *pixels=malloc(128UL*128*2),probe[4]={0x084c,0x584c,0xa84c,0xf84c};
    unsigned char *font=calloc(8192,1);FxU32 next=grTexMinAddress(GR_TMU0);int t,x,y,i,ok=0;
    if(!pixels || !font)goto done;
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i)font_glyph(font,&glyphs[i]);
    for(i=0;i<(int)(sizeof(math_glyphs)/sizeof(math_glyphs[0]));++i)font_glyph(font,&math_glyphs[i]);
    tex[0].smallLod=tex[0].largeLod=GR_LOD_128;tex[0].aspectRatio=GR_ASPECT_1x1;tex[0].format=GR_TEXFMT_ARGB_4444;
    tex[1]=tex[0];
    for(t=0;t<2;++t) {
        for(y=0;y<128;++y)for(x=0;x<128;++x) {
            int r,g,b,a,grid=x%32<2 || y%32<2;float u=x*(6.2831853f/128),v=y*(6.2831853f/128);
            if(!t) {
                r=(int)(105+65*sin(u)*cos(v));g=(int)(100+62*sin(v+.9f));b=(int)(110+70*cos(u-v));
                a=(int)(120+115*sin(u+v));
                if(grid){r=200;g=175;b=125;}
            } else {
                r=x<64?185:60;g=y<64?150:55;b=(x<64)==(y<64)?65:190;a=x*255/127;
                if(grid){r=g=b=210;}
            }
            pixels[y*128+x]=(unsigned short)(((a>>4)<<12)|((r>>4)<<8)|((g>>4)<<4)|(b>>4));
        }
        if(!upload(&tex[t],&address[t],&next,pixels))goto done;
    }
    tex[2]=tex[0];tex[2].smallLod=tex[2].largeLod=GR_LOD_32;tex[2].format=GR_TEXFMT_RGB_565;
    for(y=0;y<32;++y)for(x=0;x<32;++x){int c=((x/8)^(y/8))&1;pixels[y*32+x]=c?rgb565(38,50,59):rgb565(20,28,35);}
    if(!upload(&tex[2],&address[2],&next,pixels))goto done;
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
    int row,col;for(row=0;row<ROWS;++row)for(col=0;col<COLS;++col) {
        float lat=row*(3.14159265f/8),lon=col*(6.2831853f/12);OrbVertex *v=&source[row*COLS+col];
        v->x=(float)(sin(lat)*cos(lon));v->y=(float)cos(lat);v->z=(float)(sin(lat)*sin(lon));v->u=col*(256.0f/12);v->v=row*32.0f;
    }
}
static void prepare_object(void)
{
    int i,row,col;float cy=(float)cos(phase*.42f),sy=(float)sin(phase*.42f),cx=(float)cos(.3f+phase*.19f),sx=(float)sin(.3f+phase*.19f);
    float lx=(float)sin(phase*.6f)*.7f,ly=.6f,lz=-(float)cos(phase*.6f)*.7f,length=(float)sqrt(lx*lx+ly*ly+lz*lz);
    visible_count=0;
    for(i=0;i<VERTS;++i) {
        OrbVertex *v=&transformed[i],*s=&source[i];float xx=s->x*cy+s->z*sy,zz=-s->x*sy+s->z*cy;
        float nx=xx,ny=s->y*cx-zz*sx,nz=s->y*sx+zz*cx,shade=light_gain*(.12f+.88f*clamp((nx*lx+ny*ly+nz*lz)/length,0,1));
        v->x=nx;v->y=ny;v->z=3.5f+nz;v->u=s->u;v->v=s->v;v->r=210*shade;v->g=160*shade;v->b=96*shade;
    }
    for(row=0;row<8;++row)for(col=0;col<12;++col) {
        int a=row*COLS+col,b=a+1,c=a+COLS+1,d=a+COLS;
        float x=(transformed[a].x+transformed[b].x+transformed[c].x+transformed[d].x)*.25f;
        float y=(transformed[a].y+transformed[b].y+transformed[c].y+transformed[d].y)*.25f;
        float z=(transformed[a].z+transformed[b].z+transformed[c].z+transformed[d].z)*.25f;
        if(x*x+y*y+(z-3.5f)*z<0)visible_cells[visible_count++]=row*12+col;
    }
}
static void combine_state(int mode)
{
    int function=GR_COMBINE_FUNCTION_SCALE_OTHER,factor=GR_COMBINE_FACTOR_ONE,local=GR_COMBINE_LOCAL_ITERATED;
    int af=GR_COMBINE_FUNCTION_SCALE_OTHER,ak=GR_COMBINE_FACTOR_ONE,other=GR_COMBINE_OTHER_TEXTURE;
    const int *c=constant_colors[tint];
    grAlphaControlsITRGBLighting(FXFALSE);grTexSource(GR_TMU0,address[material],GR_MIPMAPLEVELMASK_BOTH,&tex[material]);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_CLAMP);grHints(GR_HINT_STWHINT,0);
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXTRUE);
    grFogMode(GR_FOG_DISABLE);grCullMode(GR_CULL_DISABLE);grDepthBiasLevel(0);grAlphaTestFunction(GR_CMP_ALWAYS);
    grConstantColorValue(((FxU32)constant_alpha<<24)|(FxU32)c[0]|((FxU32)c[1]<<8)|((FxU32)c[2]<<16));
    if(!alpha_mode)other=GR_COMBINE_OTHER_ITERATED;
    else if(alpha_mode==2)ak=GR_COMBINE_FACTOR_LOCAL;
    else if(alpha_mode==3)af=GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
    else if(alpha_mode==4){af=GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL;ak=GR_COMBINE_FACTOR_TEXTURE_ALPHA;other=GR_COMBINE_OTHER_ITERATED;}
    /* Keep Alocal=constant alpha in every mode: RGB interpolation reads it. */
    grAlphaCombine(af,ak,GR_COMBINE_LOCAL_CONSTANT,other,FXFALSE);
    if(mode==1)factor=GR_COMBINE_FACTOR_LOCAL;
    else if(mode==2)function=GR_COMBINE_FUNCTION_SCALE_OTHER_ADD_LOCAL;
    else if(mode==3){function=GR_COMBINE_FUNCTION_SCALE_OTHER_MINUS_LOCAL_ADD_LOCAL;factor=GR_COMBINE_FACTOR_LOCAL_ALPHA;local=GR_COMBINE_LOCAL_CONSTANT;}
    grColorCombine(function,factor,local,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(blend_enabled?GR_BLEND_SRC_ALPHA:GR_BLEND_ONE,blend_enabled?GR_BLEND_ONE_MINUS_SRC_ALPHA:GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static void checker_panel(float x,float y,float width,float height)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));local_color();
    /* Disable writes before disabling the test: backdrop draws must never
       carry a previous object's depth gradients into another panel. */
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXFALSE);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    grHints(GR_HINT_STWHINT,GR_STWHINT_W_DIFF_TMU0);
    grTexSource(GR_TMU0,address[2],GR_MIPMAPLEVELMASK_BOTH,&tex[2]);grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    for(i=0;i<4;++i){q[i].x=x+((i==1||i==2)?width:0);q[i].y=y+(i<2?0:height);q[i].oow=q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=(i==1||i==2)?512:0;q[i].tmuvtx[0].tow=i<2?0:256;q[i].r=q[i].g=q[i].b=q[i].a=255;}
    grDrawPolygonVertexList(4,q);
}
static void draw_object(float center_x,float center_y,float focal)
{
    int f;
    for(f=0;f<visible_count;++f) {
        int cell=visible_cells[f],row=cell/12,col=cell%12,a=row*COLS+col;
        int ids[4],count=4,i;GrVertex q[4];ids[0]=a;ids[1]=a+1;ids[2]=a+COLS+1;ids[3]=a+COLS;
        if(row==0){ids[1]=a+COLS+1;ids[2]=a+COLS;count=3;}
        if(row==7)count=3;
        memset(q,0,sizeof(q));
        for(i=0;i<count;++i) {
            OrbVertex *v=&transformed[ids[i]];float inv=1/v->z;
            q[i].x=center_x+(float)floor(v->x*focal*inv*16+.5f)/16;q[i].y=center_y-v->y*focal*inv;
            q[i].ooz=65535*inv;q[i].oow=q[i].tmuvtx[0].oow=inv;
            q[i].tmuvtx[0].sow=v->u*inv;q[i].tmuvtx[0].tow=v->v*inv;
            q[i].r=v->r;q[i].g=v->g;q[i].b=v->b;q[i].a=alpha_mode?vertex_alpha*255:255;
            if(q[i].x!=q[i].x || q[i].y!=q[i].y || v->z<1)finite_error=1;
        }
        grDrawPolygonVertexList(count,q);++scene_draws;
    }
}
static void draw_scene(int fps,int hud)
{
    int pass,count=gallery?4:1;char line[112];
    profile_begin();profile_stage(P_INPUT);grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0,0,0);
    background();profile_stage(P_BACKGROUND);prepare_object();profile_stage(P_TRANSFORM);scene_draws=0;
    for(pass=0;pass<count;++pass) {
        int mode=gallery?pass:color_mode;float x=gallery?(pass%2?480:160):320,y=gallery?(pass<2?202:366):273;
        float width=gallery?256:560,height=gallery?114:282;
        checker_panel(x-width*.5f,y-height*.5f,width,height);combine_state(mode);draw_object(x,y,gallery?180:410);
    }
    profile_stage(P_OPAQUE);local_color();grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grClipWindow(0,0,640,480);
    if(hud) {
        text(20,18,"3DFX / FIXED-FUNCTION COMBINERS",1,176,157,122);text(20,39,"COLOR AND ALPHA LAB",2,233,224,202);
        sprintf(line,"FPS %d",fps);text(535,20,line,1,135,209,202);
        sprintf(line,"CA %d%%  VA %.0f%%  L %.0f%%  N %s  B BLEND %s",constant_alpha*100/255,vertex_alpha*100,light_gain*100,tint_names[tint],blend_enabled?"ON":"OFF");text(20,66,line,1,135,209,202);
        sprintf(line,"A ALPHA: %s  T TEXTURE %d  %s",alpha_labels[alpha_mode],material+1,moving?"MOVING":"PAUSED");text(20,88,line,1,176,188,190);
        for(pass=0;pass<count;++pass){int mode=gallery?pass:color_mode;float x=gallery?(pass%2?340:20):20,y=gallery?(pass<2?113:281):113;text(x,y,color_labels[mode],1,233,224,202);}
        text(20,447,"1-4 MODE  C GALLERY  A ALPHA  B BLEND  +/- CA  J/K LIGHT  U/I VA",1,176,188,190);
        text(20,463,"N TINT  T TEXTURE  SPACE MOTION  R RESET  TAB PROFILER  ESC EXIT",1,116,140,148);
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
    GrHwConfiguration hw;FILE *log=fopen("COMBINE.LOG","w");int ok=1,key,frames=0,fps=0,samples=0,stamp,synced=0;
    int test=argc>1 && (!strcmp(argv[1],"/test") || !strcmp(argv[1],"/softwarecheck")),bench=argc>1 && !strcmp(argv[1],"/bench");
    if(argc>1 && !strcmp(argv[1],"/softwarecheck"))save_captures=0;
    grGlideInit();if(!grSstQueryHardware(&hw) || hw.num_sst<1){ok=0;goto done;}grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){ok=0;goto done;}
    if(!load_textures()){ok=0;goto done;}
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grDitherMode(GR_DITHER_4x4);profile_init();profiler.measure=1;model_init();
    if(log)fprintf(log,"Voodoo: framebuffer %d MB, %d TMUs, TMU0 %d MB, texture bytes %lu; timer %d\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)texture_bytes,profiler.available);
    if(test){ok=tests(log);goto done;}if(bench){ok=benchmark(log);goto done;}stamp=rtc_seconds();
    for(;;) {
        float dt=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
        while(kbhit()) {
            key=getch();if(key==27)goto done;if(key==0 || key==224){getch();continue;}
            if(key>='1' && key<='4'){color_mode=key-'1';gallery=0;}if(key=='c'||key=='C')gallery=!gallery;
            if(key=='a'||key=='A')alpha_mode=(alpha_mode+1)%5;if(key=='b'||key=='B')blend_enabled=!blend_enabled;
            if(key=='+'||key=='=')constant_alpha=(int)clamp(constant_alpha+17,0,255);if(key=='-')constant_alpha=(int)clamp(constant_alpha-17,0,255);
            if(key=='j'||key=='J')light_gain=clamp(light_gain-.1f,0,1);if(key=='k'||key=='K')light_gain=clamp(light_gain+.1f,0,1);
            if(key=='u'||key=='U')vertex_alpha=clamp(vertex_alpha-.1f,0,1);if(key=='i'||key=='I')vertex_alpha=clamp(vertex_alpha+.1f,0,1);
            if(key=='n'||key=='N')tint=(tint+1)%3;if(key=='t'||key=='T')material=!material;if(key==' ')moving=!moving;
            if(key=='r'||key=='R'){gallery=moving=1;color_mode=alpha_mode=blend_enabled=tint=material=0;constant_alpha=128;light_gain=.65f;vertex_alpha=.7f;phase=0;}
            if(key==9){profile_toggle();profiler_dirty=1;}
        }
        if(moving)phase+=dt;draw_scene(fps,1);++frames;++samples;
        {int now=rtc_seconds(),elapsed=(now-stamp+60)%60;if(elapsed){if(synced)fps=samples/elapsed;synced=1;samples=0;stamp=now;}}
    }
done:
    if(log){fprintf(log,"%s: color/alpha combiners, %d interactive frames\n",ok?"PASS":"FAIL",frames);fclose(log);}grGlideShutdown();return ok?0:1;
}
