/* SPDX-License-Identifier: MIT - shared by sectorforth and sectorlisp. */
#include <stdlib.h>
#include <string.h>
#include <glide.h>
#include "glide_demo.h"
static int active, initialized, test_mode, captures, triangles, failure;
static GrVertex vertices[32];
static unsigned char valid[32];
static FILE *logfile;

static void wait_retrace(void)
{
    while (grSstVRetraceOn()) { }
    while (!grSstVRetraceOn()) { }
}

int __cdecl gfx_init(void)
{
    GrHwConfiguration hw;
    if (active) return 1;
    memset(valid,0,sizeof(valid));
    if (initialized) {
        if (!grSstControl(GR_CONTROL_ACTIVATE)) return 0;
        /* DOSBox-X changes the host output mode asynchronously at retrace.
           Let activation finish before drawing into that output context. */
        wait_retrace();
        wait_retrace();
        active=1;
        return 1;
    }
    grGlideInit();
    if (!grSstQueryHardware(&hw) || hw.num_sst<1) {
        grGlideShutdown(); return 0;
    }
    grSstSelect(0);
    if (!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,
                     GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)) {
        grGlideShutdown(); return 0;
    }
    active=1;
    initialized=1;
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
                   GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    grCullMode(GR_CULL_DISABLE);
    grDitherMode(GR_DITHER_4x4);
    return 1;
}
void __cdecl gfx_close(void)
{
    /* Voodoo 1 is a separate accelerator: retain the context across REPL
       visits and switch back to VGA. Shut Glide down once, on process exit. */
    if (active) { grSstControl(GR_CONTROL_DEACTIVATE); active=0; }
}
void gd_shutdown(void)
{
    if (initialized) { grGlideShutdown(); initialized=0; active=0; }
}
int __cdecl gfx_clear(void)
{
    if (!active) return 0;
    grRenderBuffer(GR_BUFFER_BACKBUFFER);
    grClipWindow(0,0,640,480);
    grBufferClear(0x00160c08,0,0);
    return 1;
}
int __cdecl gfx_vertex(int x,int y,int r,int g,int b,int index)
{
    GrVertex *v;
    if (!active || index<0 || index>=32 || x<0 || x>=640 || y<0 || y>=480 ||
        r<0 || r>255 || g<0 || g>255 || b<0 || b>255) return 0;
    v=&vertices[index]; memset(v,0,sizeof(*v));
    v->x=(float)x; v->y=(float)y; v->r=(float)r;
    v->g=(float)g; v->b=(float)b; v->a=255; v->oow=1;
    valid[index]=1;
    return 1;
}
int __cdecl gfx_triangle(int a,int b,int c)
{
    if (!active || a<0 || a>=32 || b<0 || b>=32 || c<0 || c>=32 ||
        !valid[a] || !valid[b] || !valid[c]) return 0;
    grDrawTriangle(&vertices[a],&vertices[b],&vertices[c]);
    ++triangles;
    return 1;
}

/* Check real Voodoo framebuffer samples, not just successful API returns. */
static int capture(void)
{
    unsigned short *pixels=malloc(640UL*480*2);
    unsigned long i, bright=0;
    int ok=1;
    FILE *file;
    static const int samples[4][3]={{320,115,0},{135,385,1},{505,385,2},{320,280,3}};
    if (!pixels) return 0;
    if (!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,640,480,1280,pixels)) {
        free(pixels); return 0;
    }
    file=fopen("TRIANGLE.PPM","wb");
    if (!file) { free(pixels); return 0; }
    fprintf(file,"P6\n640 480\n255\n");
    for (i=0;i<640UL*480;++i) {
        unsigned short p=pixels[i];
        unsigned char rgb[3];
        rgb[0]=((p>>11)&31)*255/31;
        rgb[1]=((p>>5)&63)*255/63;
        rgb[2]=(p&31)*255/31;
        if (rgb[0]>60 || rgb[1]>60 || rgb[2]>60) ++bright;
        if (fwrite(rgb,1,3,file)!=3) ok=0;
    }
    if (fclose(file)) ok=0;
    for (i=0;i<4;++i) {
        unsigned short p=pixels[samples[i][1]*640UL+samples[i][0]];
        int rgb[3],channel=samples[i][2];
        rgb[0]=((p>>11)&31)*255/31; rgb[1]=((p>>5)&63)*255/63; rgb[2]=(p&31)*255/31;
        if (channel<3) {
            if (rgb[channel]<160 || rgb[channel]<=rgb[(channel+1)%3]+80 ||
                rgb[channel]<=rgb[(channel+2)%3]+80) ok=0;
        } else if (rgb[0]<40 || rgb[1]<40 || rgb[2]<40) ok=0;
    }
    if (bright<60000 || bright>100000) ok=0;
    free(pixels);
    if (logfile) fprintf(logfile,"Framebuffer RGB corners/interior and coverage: %s\n",ok?"PASS":"FAIL");
    return ok;
}
int __cdecl gfx_swap(void)
{
    if (!active) return 0;
    wait_retrace();
    grBufferSwap(1);
    while (grBufferNumPending()>0) { }
    grSstIdle();
    if (test_mode) {
        ++captures;
        if (!capture()) { failure=1; return 0; }
    }
    return 1;
}

void gd_begin_test(FILE *log) { logfile=log; test_mode=1; captures=triangles=failure=0; }
int gd_captures(void) { return captures; }
int gd_triangles(void) { return triangles; }
int gd_failed(void) { return failure; }
