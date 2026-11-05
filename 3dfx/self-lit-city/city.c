/* Self-lit texels through Glide's native alpha-controlled local RGB selector. */
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
typedef struct {float x,y,z,u,v,r,g,b;} CityVertex;
typedef struct {Vec p[4],normal;float u,v;int material;} CityFace;
#define CITY_FACES 160
static CityFace faces[CITY_FACES];
static CityVertex prepared[CITY_FACES][4];
static int face_count,draw_calls,frame_calls,finite_error;
static int split=1,mask_enabled=1,mask_view,swapped,moving=1,touring=1,mouse_present;
static int save_captures=1;
static float scene_time,camera_yaw=.65f,camera_radius=29,light_strength=.7f;
static Vec eye,forward,right,up,lamp;
static float focal;
static Vec vec(float x,float y,float z){Vec a;a.x=x;a.y=y;a.z=z;return a;}
static Vec sub(Vec a,Vec b){return vec(a.x-b.x,a.y-b.y,a.z-b.z);}
static float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static Vec cross(Vec a,Vec b){return vec(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);}
static Vec unit(Vec a){float d=(float)sqrt(dot(a,a));return d>.00001f?vec(a.x/d,a.y/d,a.z/d):vec(0,0,0);}
static int rtc_seconds(void)
{int mode,sec;outp(0x70,11);mode=inp(0x71);outp(0x70,0);sec=inp(0x71);return mode&4?sec:(sec>>4)*10+(sec&15);}
static int upload(GrTexInfo *info,FxU32 *dest,FxU32 *next,void *pixels)
{
    FxU32 bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,info);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    *dest=*next;info->data=pixels;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,info);
    *next+=bytes;info->data=NULL;return 1;
}
static unsigned short facade_pixel(int kind,int x,int y)
{
    int cx=x%28,cy=y%24,lit=0,r,g,b,noise=(x*17+y*31+x*y%23)%12;
    r=75+noise;g=84+noise;b=107+noise;
    if(y<100 && x>5 && x<120 && cx>=8 && cx<=21 && cy>=6 && cy<=19) {
        int room=(x/28+3*(y/24)+kind)%5;
        if(room==0){r=13;g=22;b=38;}
        else {lit=1;r=kind?80+noise:220+noise;g=kind?188+noise:158+noise;b=kind?224+noise:64+noise;}
        if(cx==14 || cy==12){lit=0;r=42;g=48;b=61;}
    }
    if(y%24<2){r=40;g=47;b=66;}
    if(y>=103 && y<=123 && x>=8 && x<=119) {
        static const char sign[]="3DFX";int gx=(x-39)/2,gy=(y-106)/2,index;
        r=9;g=15;b=27;lit=0;
        if(y==103 || y==123 || x==8 || x==119){r=kind?245:50;g=kind?76:210;b=kind?182:245;lit=1;}
        if(x>=39 && y>=106 && gx>=0 && gx<24 && gy>=0 && gy<7 && gx%6<5) {
            index=sign[gx/6]-32;
            if(panel_glyph[index][gy]&(1<<(4-gx%6))){r=kind?245:50;g=kind?76:210;b=kind?182:245;lit=1;}
        }
    }
    return (unsigned short)((lit?0x8000:0)|((r>>3)<<10)|((g>>3)<<5)|(b>>3));
}
static int load_textures(void)
{
    unsigned short *pixels=malloc(128UL*128*2),probe[4]={0x4210,0xc210,0x4210,0xc210};
    unsigned char *font=calloc(8192,1);FxU32 next=grTexMinAddress(GR_TMU0);int i,x,y,t,ok=0;
    if(!pixels || !font)goto done;
    for(i=0;i<(int)(sizeof(glyphs)/sizeof(glyphs[0]));++i) {
        int slot=glyphs[i].c-32;memcpy(panel_glyph[slot],glyphs[i].row,7);
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(glyphs[i].row[y]&(1<<(4-x)))font[((slot/32)*8+y)*256+(slot%32)*8+x]=255;
    }
    tex[0].smallLod=tex[0].largeLod=GR_LOD_128;tex[0].aspectRatio=GR_ASPECT_1x1;tex[0].format=GR_TEXFMT_ARGB_1555;
    tex[1]=tex[2]=tex[0];
    for(t=0;t<3;++t) {
        for(y=0;y<128;++y)for(x=0;x<128;++x) {
            if(t<2)pixels[y*128+x]=facade_pixel(t,x,y);
            else {int n=48+(x*13+y*29)%12;pixels[y*128+x]=(unsigned short)(((n>>3)<<10)|((n>>3)<<5)|((n+12)>>3));}
        }
        if(!upload(&tex[t],&address[t],&next,pixels))goto done;
    }
    tex[3]=tex[0];tex[3].smallLod=tex[3].largeLod=GR_LOD_256;tex[3].aspectRatio=GR_ASPECT_8x1;tex[3].format=GR_TEXFMT_ALPHA_8;
    if(!upload(&tex[3],&address[3],&next,font))goto done;
    probe_texture=tex[0];probe_texture.smallLod=probe_texture.largeLod=GR_LOD_2;
    if(!upload(&probe_texture,&probe_address,&next,probe))goto done;
    profiler_tex=tex[3];profiler_tex.aspectRatio=GR_ASPECT_1x2;profiler_tex.format=GR_TEXFMT_RGB_565;
    memset(profiler_pixels,0,sizeof(profiler_pixels));
    for(i=0;i<2;++i)if(!upload(&profiler_tex,&profiler_address[i],&next,profiler_pixels))goto done;
    texture_bytes=next-grTexMinAddress(GR_TMU0);ok=1;
done:
    free(pixels);free(font);return ok;
}
static void emit(Vec a,Vec b,Vec c,Vec d,Vec normal,int material,float u,float v)
{
    CityFace *f;if(face_count>=CITY_FACES){finite_error=1;return;}f=&faces[face_count++];
    f->p[0]=a;f->p[1]=b;f->p[2]=c;f->p[3]=d;f->normal=normal;f->material=material;f->u=u;f->v=v;
}
static void building(float x,float z,float w,float h,float d,int material)
{
    float a=x-w,b=x+w,c=z-d,e=z+d;
    emit(vec(a,0,e),vec(b,0,e),vec(b,h,e),vec(a,h,e),vec(0,0,1),material,256,256);
    emit(vec(b,0,c),vec(a,0,c),vec(a,h,c),vec(b,h,c),vec(0,0,-1),material,256,256);
    emit(vec(b,0,e),vec(b,0,c),vec(b,h,c),vec(b,h,e),vec(1,0,0),material,256,256);
    emit(vec(a,0,c),vec(a,0,e),vec(a,h,e),vec(a,h,c),vec(-1,0,0),material,256,256);
    emit(vec(a,h,e),vec(b,h,e),vec(b,h,c),vec(a,h,c),vec(0,1,0),2,256,256);
}
static void model_init(void)
{
    int i,j;face_count=0;
    emit(vec(-19,-.04f,19),vec(19,-.04f,19),vec(19,-.04f,-19),vec(-19,-.04f,-19),vec(0,1,0),2,2048,2048);
    for(j=0;j<3;++j)for(i=0;i<4;++i) {
        float x=-9+i*6.0f,z=-7+j*7.0f,h=4.3f+((i*3+j*5)%5)*1.15f;
        building(x,z,1.8f,h,2.0f,(i+j)&1);
    }
}
static void prepare_scene(void)
{
    int i,j;Vec look=vec(0,3.2f,0);
    eye=vec((float)sin(camera_yaw)*camera_radius,8.2f,(float)cos(camera_yaw)*camera_radius);
    forward=unit(sub(look,eye));right=unit(cross(forward,vec(0,1,0)));up=cross(right,forward);
    lamp=vec((float)sin(scene_time*.65f)*13,7,(float)cos(scene_time*.65f)*13);
    for(i=0;i<face_count;++i)for(j=0;j<4;++j) {
        CityFace *f=&faces[i];CityVertex *v=&prepared[i][j];Vec d=sub(f->p[j],eye),l=sub(lamp,f->p[j]);
        float dist=(float)sqrt(dot(l,l)),diff=dist>.01f?clamp(dot(f->normal,l)/dist,0,1):1;
        float shade=255*light_strength*(.14f+.86f*diff/(1+dist*.025f));
        v->x=dot(d,right);v->y=dot(d,up);v->z=dot(d,forward);
        v->u=(j==1||j==2)?f->u:0;v->v=j<2?f->v:0;v->r=shade*.80f;v->g=shade*.9f;v->b=shade;
    }
}
static void city_state(int enable,int preview)
{
    grTexFilterMode(GR_TMU0,preview?GR_TEXTUREFILTER_POINT_SAMPLED:GR_TEXTUREFILTER_BILINEAR,preview?GR_TEXTUREFILTER_POINT_SAMPLED:GR_TEXTUREFILTER_BILINEAR);
    grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
    grHints(GR_HINT_STWHINT,0);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXTRUE);
    grFogMode(GR_FOG_DISABLE);grCullMode(GR_CULL_DISABLE);grAlphaTestFunction(GR_CMP_ALWAYS);grDepthBiasLevel(0);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);grConstantColorValue(0xffffffffUL);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,preview?GR_COMBINE_FACTOR_TEXTURE_ALPHA:GR_COMBINE_FACTOR_LOCAL,
        GR_COMBINE_LOCAL_ITERATED,preview?GR_COMBINE_OTHER_CONSTANT:GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaControlsITRGBLighting(enable && !preview?FXTRUE:FXFALSE);
}
static float distance_plane(const CityVertex *v,int plane,float bx,float by)
{
    switch(plane){case 0:return v->z-.45f;case 1:return 100-v->z;case 2:return v->x+v->z*bx;case 3:return v->z*bx-v->x;case 4:return v->y+v->z*by;default:return v->z*by-v->y;}
}
static void draw_face(int id,float center)
{
    CityVertex a[12],b[12];GrVertex q[12];int count=4,plane,i;float bx=(split?160:320)/focal,by=172/focal;
    memcpy(a,prepared[id],sizeof(CityVertex)*4);
    for(plane=0;plane<6;++plane) {
        int n=0;
        for(i=0;i<count;++i) {
            CityVertex *p=&a[i],*r=&a[(i+1)%count];float dp=distance_plane(p,plane,bx,by),dr=distance_plane(r,plane,bx,by);
            if(dp>=0)b[n++]=*p;
            if((dp>=0)!=(dr>=0)) {
                float t=dp/(dp-dr);CityVertex *v=&b[n++];
                v->x=p->x+(r->x-p->x)*t;v->y=p->y+(r->y-p->y)*t;v->z=p->z+(r->z-p->z)*t;
                v->u=p->u+(r->u-p->u)*t;v->v=p->v+(r->v-p->v)*t;
                v->r=p->r+(r->r-p->r)*t;v->g=p->g+(r->g-p->g)*t;v->b=p->b+(r->b-p->b)*t;
            }
        }
        if(n<3)return;count=n;memcpy(a,b,count*sizeof(CityVertex));
    }
    for(i=0;i<count;++i) {
        float inv=1/a[i].z;memset(&q[i],0,sizeof(q[i]));
        /* Snap the shared local X before adding the viewport center, so the
           comparison halves use identical 12.4 screen coordinates. */
        q[i].x=center+(float)floor(a[i].x*focal*inv*16+.5f)/16;
        q[i].y=260-a[i].y*focal*inv;q[i].ooz=65535*.45f*inv;
        q[i].oow=q[i].tmuvtx[0].oow=inv;q[i].tmuvtx[0].sow=a[i].u*inv;q[i].tmuvtx[0].tow=a[i].v*inv;
        q[i].r=a[i].r;q[i].g=a[i].g;q[i].b=a[i].b;q[i].a=255;
        if(q[i].x!=q[i].x || q[i].y!=q[i].y || q[i].ooz<=0)finite_error=1;
    }
    grDrawPolygonVertexList(count,q);++draw_calls;
}
static void draw_scene(int fps,int hud)
{
    int pass,i,last,passes=split?2:1;char line[96];
    profile_begin();profile_stage(P_INPUT);grAlphaControlsITRGBLighting(FXFALSE);grClipWindow(0,0,640,480);
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0x00120b08,0,0);background();profile_stage(P_BACKGROUND);
    focal=split?260:520;prepare_scene();profile_stage(P_TRANSFORM);draw_calls=0;
    for(pass=0;pass<passes;++pass) {
        int feature=split?(pass^swapped):1;float center=split?(pass?480:160):320;
        grClipWindow(split?pass*320:0,88,split?(pass+1)*320:640,432);
        city_state(feature && mask_enabled,feature && mask_view);last=-1;
        for(i=0;i<face_count;++i) {
            CityFace *f=&faces[i];if(dot(f->normal,sub(eye,f->p[0]))<=0)continue;
            if(f->material!=last){last=f->material;grTexSource(GR_TMU0,address[last],GR_MIPMAPLEVELMASK_BOTH,&tex[last]);}
            draw_face(i,center);
        }
    }
    frame_calls=draw_calls;profile_stage(P_OPAQUE);
    local_color();grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);grDepthMask(FXFALSE);grClipWindow(0,0,640,480);
    if(hud) {
        text(20,18,"3DFX / ALPHA-CONTROLLED LIGHTING",1,176,157,122);text(20,39,"SELF-LIT CITY",2.5f,233,224,202);
        sprintf(line,"FPS %d",fps);text(535,20,line,1,135,209,202);
        sprintf(line,"E MASK %s  LIGHT %.0f  %s  %s",mask_enabled?"ON":"OFF",light_strength*100,moving?"MOVING LIGHT":"HELD LIGHT",touring?"FLYPAST":"MANUAL ORBIT");text(20,67,line,1,135,209,202);
        for(pass=0;pass<passes;++pass) {
            int feature=split?(pass^swapped):1;float left=split?pass*320.0f:0;
            text(left+20,96,feature?(mask_view?"TEXTURE ALPHA MASK":mask_enabled?"SELF-LIT WINDOWS / ONE PASS":"MASK OFF / VERTEX LIGHT") :"ORDINARY VERTEX LIGHTING",1,214,219,206);
        }
        text(20,447,"E MASK  C SPLIT  S SWAP  M SHOW MASK  +/- LIGHT  SPACE LIGHT MOTION",1,176,188,190);
        text(20,463,"F FLYPAST  LMB ORBIT  RMB ZOOM  R RESET  TAB PROFILER  ESC EXIT",1,116,140,148);
    }
    profile_stage(P_HUD);if(hud)profiler_panel();profile_stage(P_OVERLAY);
    while(grSstVRetraceOn()){}while(!grSstVRetraceOn()){}grBufferSwap(1);
    while(grBufferNumPending()>0){}grSstIdle();profile_stage(P_PRESENT);profile_end();
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
    GrHwConfiguration hw;union REGS regs;FILE *log=fopen("CITY.LOG","w");int ok=1,key,frames=0,fps=0,samples=0,stamp,synced=0;
    int test=argc>1 && (!strcmp(argv[1],"/test") || !strcmp(argv[1],"/softwarecheck")),bench=argc>1 && !strcmp(argv[1],"/bench");
    if(argc>1 && !strcmp(argv[1],"/softwarecheck"))save_captures=0;
    grGlideInit();if(!grSstQueryHardware(&hw) || hw.num_sst<1){ok=0;goto done;}grSstSelect(0);
    if(!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)){ok=0;goto done;}
    if(!load_textures()){ok=0;goto done;}
    grTexCombine(GR_TMU0,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,FXFALSE,FXFALSE);
    grDitherMode(GR_DITHER_4x4);profile_init();profiler.measure=1;model_init();
    if(log)fprintf(log,"Voodoo: framebuffer %d MB, %d TMUs, TMU0 %d MB, texture bytes %lu; timer %d; %d model faces\n",hw.SSTs[0].sstBoard.VoodooConfig.fbRam,hw.SSTs[0].sstBoard.VoodooConfig.nTexelfx,hw.SSTs[0].sstBoard.VoodooConfig.tmuConfig[0].tmuRam,(unsigned long)texture_bytes,profiler.available,face_count);
    if(test){ok=tests(log);goto done;}if(bench){ok=benchmark(log);goto done;}
    memset(&regs,0,sizeof(regs));int386(0x33,&regs,&regs);mouse_present=regs.w.ax==0xffff;stamp=rtc_seconds();
    for(;;) {
        float dt=profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
        while(kbhit()) {
            key=getch();if(key==27)goto done;if(key==0 || key==224){getch();continue;}
            if(key=='e'||key=='E')mask_enabled=!mask_enabled;if(key=='c'||key=='C')split=!split;
            if(key=='s'||key=='S')swapped=!swapped;if(key=='m'||key=='M')mask_view=!mask_view;
            if(key=='f'||key=='F')touring=!touring;if(key==' ')moving=!moving;
            if(key=='+'||key=='=')light_strength=clamp(light_strength+.1f,0,1);if(key=='-')light_strength=clamp(light_strength-.1f,0,1);
            if(key=='r'||key=='R'){camera_yaw=.65f;camera_radius=29;scene_time=0;light_strength=.7f;mask_view=swapped=0;mask_enabled=split=moving=touring=1;}
            if(key==9){profile_toggle();profiler_dirty=1;}
        }
        if(mouse_present) {
            int buttons,dx,dy;regs.w.ax=3;int386(0x33,&regs,&regs);buttons=regs.w.bx&3;
            regs.w.ax=11;int386(0x33,&regs,&regs);dx=(short)regs.w.cx;dy=(short)regs.w.dx;
            if(buttons&1){camera_yaw-=dx*.006f;touring=0;}if(buttons&2){camera_radius=clamp(camera_radius+dy*.09f,18,45);touring=0;}
        }
        if(moving)scene_time+=dt;if(touring)camera_yaw+=dt*.12f;draw_scene(fps,1);++frames;++samples;
        {int now=rtc_seconds(),elapsed=(now-stamp+60)%60;if(elapsed){if(synced)fps=samples/elapsed;synced=1;samples=0;stamp=now;}}
    }
done:
    if(log){fprintf(log,"%s: self-lit city, %d interactive frames\n",ok?"PASS":"FAIL",frames);fclose(log);}grGlideShutdown();return ok?0:1;
}
