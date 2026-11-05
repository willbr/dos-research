/* Bitmap font and incremental profiler dashboard, adapted from helmet. */
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
 {'+',{0,4,4,31,4,4,0}},
 {'[',{14,8,8,8,8,8,14}},{']',{14,2,2,2,2,2,14}}
};
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
    GrVertex q[4];int i;
    grTexSource(GR_TMU0,address[3],GR_MIPMAPLEVELMASK_BOTH,&tex[3]);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
    local_color();
    grConstantColorValue(0xff000000UL|(FxU32)r|((FxU32)g<<8)|((FxU32)b<<16));
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,
        GR_COMBINE_LOCAL_NONE,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
    /* Color, depth and row bounds are constant for the whole string. */
    memset(q,0,sizeof(q));
    for(i=0;i<4;++i) { q[i].r=r;q[i].g=g;q[i].b=b;q[i].a=255;q[i].oow=1;q[i].tmuvtx[0].oow=1; }
    q[0].y=q[1].y=y;q[2].y=q[3].y=y+7*size;
    for(;*s;++s,x+=6*size) {
        int index=*s-32;float u=(float)(index%32*8),v=(float)(index/32*8);
        if(index<0 || index>95 || *s==' ') continue;
        q[0].x=q[3].x=x;q[1].x=q[2].x=x+5*size;
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
#ifdef DETAIL_DEMO
static void detail_profiler_template(void)
{
    static const char *labels[P_STAGES]={"INPUT","DARK","XFORM","OPAQUE","BLEND","LIGHTMAP","HUD","PROF","WAIT","PROJECT","SHADOW",
#ifdef FOLIAGE_DEMO
"FOLIAGE"
#else
"DETAIL"
#endif
};
    unsigned short muted=rgb565(131,164,169),gold=rgb565(200,167,104),background=rgb565(8,16,22);int i;
    for(i=0;i<128*256;++i)profiler_pixels[i]=background;
    panel_line(0,0,127,0,muted);panel_line(0,255,127,255,muted);panel_line(0,0,0,255,muted);panel_line(127,0,127,255,muted);
    panel_text(4,5,"FRAME PROFILER",gold);panel_text(4,16,"EMU MS / TAB CLOSE",muted);
    if(profiler.available)for(i=0;i<P_STAGES;++i)panel_text(4,42+i*8,labels[i],muted);
    panel_text(4,148,"FPS 0-90 / 60",muted);panel_text(4,245,"16.7 MS = 60 FPS",gold);
    memcpy(profiler_template,profiler_pixels,sizeof(profiler_template));
}
#endif
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
#ifdef DETAIL_DEMO
    static const char *labels[P_STAGES]={"INPUT","DARK","XFORM","OPAQUE","BLEND","LIGHTMAP","HUD","PROF","WAIT","PROJECT","SHADOW",
#ifdef FOLIAGE_DEMO
"FOLIAGE"
#else
"DETAIL"
#endif
};
#elif defined(REFLECTION_DEMO)
    static const char *labels[P_STAGES]={"INPUT","DARK","XFORM","OPAQUE","BLEND","LIGHTMAP","HUD","PROF","WAIT","PROJECT","SHADOW","REFLECT"};
#elif defined(SHADOW_DEMO)
    static const char *labels[P_STAGES]={"INPUT","DARK","XFORM","OPAQUE","BLEND","LIGHTMAP","HUD","PROF","WAIT","PROJECT","SHADOW"};
#elif defined(PROJECTOR_DEMO)
    static const char *labels[P_STAGES]={"INPUT","DARK","XFORM","OPAQUE","BLEND","LIGHTMAP","HUD","PROF","WAIT","PROJECT"};
#elif defined(LIGHTMAP_DEMO)
    static const char *labels[P_STAGES]={"INPUT","DARK","XFORM","OPAQUE","BLEND","LIGHTMAP","HUD","PROF","WAIT"};
#elif defined(BLEND_DEMO)
    static const char *labels[P_STAGES]={"INPUT","DARK","XFORM","OPAQUE","BLEND","STATE","HUD","PROF","WAIT"};
#else
    static const char *labels[P_STAGES]={"INPUT","SKY","XFORM","DRAW","COMPARE","EXTRA","HUD","PROF","WAIT"};
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
#ifdef SHADOW_DEMO
        /* Four clear slices and four row slices leave room for moving shadows. */
        if(phase<=4) {
            unsigned short clear_color=rgb565(8,16,22);
            int start=(phase-1)*8192;
#ifdef DETAIL_DEMO
            memcpy(profiler_pixels+start,profiler_template+start,8192*sizeof(unsigned short));
#else
            for(i=start;i<start+8192;i+=8) {
                profiler_pixels[i]=clear_color;profiler_pixels[i+1]=clear_color;
                profiler_pixels[i+2]=clear_color;profiler_pixels[i+3]=clear_color;
                profiler_pixels[i+4]=clear_color;profiler_pixels[i+5]=clear_color;
                profiler_pixels[i+6]=clear_color;profiler_pixels[i+7]=clear_color;
            }
#endif
            ++profiler_build;goto profiler_publish;
        }
        phase=phase==5?1:phase<=8?phase-4:phase==9?4:phase-5;
#elif defined(PROJECTOR_DEMO)
        if(phase==1) {
            unsigned short clear_color=rgb565(8,16,22);
            for(i=0;i<128*128;i+=8) {
                profiler_pixels[i]=clear_color;profiler_pixels[i+1]=clear_color;
                profiler_pixels[i+2]=clear_color;profiler_pixels[i+3]=clear_color;
                profiler_pixels[i+4]=clear_color;profiler_pixels[i+5]=clear_color;
                profiler_pixels[i+6]=clear_color;profiler_pixels[i+7]=clear_color;
            }
            ++profiler_build;goto profiler_publish;
        }
        --phase;
#endif
        if(phase==1) {
            unsigned short background_color=rgb565(8,16,22);
            int clear_start=0;
#ifdef PROJECTOR_DEMO
            clear_start=128*128;
#endif
            /* Keep the dashboard's largest raster job below the frame
               budget on the Pentium target; eight stores per loop. */
#ifndef SHADOW_DEMO
            for(i=clear_start;i<128*256;i+=8) {
                profiler_pixels[i]=background_color;profiler_pixels[i+1]=background_color;
                profiler_pixels[i+2]=background_color;profiler_pixels[i+3]=background_color;
                profiler_pixels[i+4]=background_color;profiler_pixels[i+5]=background_color;
                profiler_pixels[i+6]=background_color;profiler_pixels[i+7]=background_color;
            }
#endif
#ifndef DETAIL_DEMO
            panel_line(0,0,127,0,muted);panel_line(0,255,127,255,muted);
            panel_line(0,0,0,255,muted);panel_line(127,0,127,255,muted);
            panel_text(4,5,"FRAME PROFILER",gold);panel_text(4,16,"EMU MS / TAB CLOSE",muted);
#endif
            if(!panel_snapshot.available) {
                panel_text(4,40,"TIMER UNAVAILABLE",ink);panel_text(4,52,"USE SUPPLIED CONF",muted);
            } else if(panel_snapshot.average>0) {
                int fps10=(int)(10000/panel_snapshot.average+.5f),ms10=(int)(panel_snapshot.average*10+.5f);
                sprintf(line,"FPS %d.%d  %d.%dMS",fps10/10,fps10%10,ms10/10,ms10%10);
                panel_text(4,29,line,ink);
            } else panel_text(4,29,"COLLECTING FRAMES",ink);
        } else if(phase>=2 && phase<=4 && panel_snapshot.available) {
            int first=(phase-2)*3;
            int end=first+3,row_step=10,bar_step=8;
#ifdef PROJECTOR_DEMO
            if(phase==4)end=P_STAGES;row_step=9;
#ifdef SHADOW_DEMO
            row_step=8;bar_step=7;
            if(profiler_build==9)first=9;
            end=first+3;
#endif
#endif
            for(i=first;i<end && i<P_STAGES;++i) {
                int y=42+i*row_step,width=panel_snapshot.average>0?(int)(panel_snapshot.mean[i]/panel_snapshot.average*116):0;
                int value=(int)(panel_snapshot.mean[i]*100+.5f);
#ifndef DETAIL_DEMO
                panel_text(4,y,labels[i],muted);
#endif
                sprintf(line,"%2d.%02d",value/100,value%100);panel_text(80,y,line,ink);
                for(j=0;j<width && j<116;++j) panel_pixel(4+j,y+bar_step,i==P_PRESENT?muted:gold);
            }
            if(phase==4
#ifdef SHADOW_DEMO
                && profiler_build==9
#endif
            ) {
                int p=(int)(panel_snapshot.p95*10+.5f),m=(int)(panel_snapshot.max*10+.5f);
                sprintf(line,"P95 %d.%d MAX %d.%d",p/10,p%10,m/10,m%10);
#if defined(REFLECTION_DEMO) || defined(DETAIL_DEMO)
                panel_text(4,139,line,ink);
#else
                panel_text(4,135,line,ink);
#endif
            }
        } else if(phase==5 && panel_snapshot.available) {
#ifndef DETAIL_DEMO
            panel_text(4,148,"FPS 0-90 / 60",muted);
#endif
            panel_graph(159,1);
        } else if(phase==6 && panel_snapshot.available) {
            panel_graph(207,0);
#ifndef DETAIL_DEMO
            panel_text(4,245,"16.7 MS = 60 FPS",gold);
#endif
        }
        ++profiler_build;
        if(profiler_build==
#ifdef SHADOW_DEMO
            12
#else
            8
#endif
        ) { profiler_build=0;profiler_row=0;profiler_upload=1; }
    }
#ifdef PROJECTOR_DEMO
profiler_publish:
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
