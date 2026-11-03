/* DOS/Glide host for the 32-bit sectorforth port. No geometry is defined here. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <conio.h>
#include <glide.h>

extern int __cdecl sf_eval(char *source);
extern void __cdecl sf_reset(void);
extern int __cdecl sf_depth(void);
extern int __cdecl sf_compiling(void);
static int active, initialized, test_mode, captures, triangles, failure;
static GrVertex vertices[32];
static unsigned char valid[32];
static FILE *logfile;

static void wait_retrace(void)
{
    while (grSstVRetraceOn()) { }
    while (!grSstVRetraceOn()) { }
}

int __cdecl host_number(const char *text, int length, long *value)
{
    unsigned long n=0, limit;
    int i=0, negative=0, base=10, digit;
    if (!length) return 0;
    if (text[i]=='-') { negative=1; if (++i==length) return 0; }
    if (text[i]=='$') { base=16; if (++i==length) return 0; }
    limit=negative?2147483648UL:2147483647UL;
    for (;i<length;++i) {
        digit=text[i]>='0' && text[i]<='9' ? text[i]-'0' :
              text[i]>='a' && text[i]<='f' ? text[i]-'a'+10 : -1;
        if (digit<0 || digit>=base || n>(limit-digit)/base) return 0;
        n=n*base+digit;
    }
    *value=negative ? (long)(0UL-n) : (long)n;
    return 1;
}
void __cdecl host_emit(int c) { putchar(c&255); }
int __cdecl host_key(void) { return test_mode?27:getch(); }
void __cdecl host_dot(long n) { printf("%ld ",n); }

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
static void gfx_shutdown(void)
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

static int evaluate(const char *text)
{
    char *copy=malloc(strlen(text)+1);
    int result;
    if (!copy) return 5;
    strcpy(copy,text); result=sf_eval(copy); free(copy);
    return result;
}
static int load(const char *path)
{
    FILE *file=fopen(path,"rb");
    long size;
    char *source;
    int result;
    if (!file) { printf("Cannot open %s\n",path); return 1; }
    fseek(file,0,SEEK_END); size=ftell(file); rewind(file);
    if (size<0 || size>262144) { fclose(file); return 1; }
    source=malloc(size+1);
    if (!source) { fclose(file); return 1; }
    if (fread(source,1,size,file)!=(size_t)size) { free(source); fclose(file); return 1; }
    fclose(file); source[size]=0;
    result=sf_eval(source);
    if (!result && sf_compiling()) result=4;
    free(source);
    return result;
}
static int selftest(void)
{
    int ok=1,i;
    char *stress;
    sf_reset();
    if (load("TESTS.F") || sf_depth()) ok=0;
    if (evaluate("not-a-word")!=1 || sf_depth()) ok=0;
    if (evaluate("drop")!=2) ok=0;
    if (evaluate("0 assert")!=6) ok=0;
    if (evaluate(": broken unknown")!=1 || sf_compiling()) ok=0;
    if (evaluate("broken")!=1) ok=0;
    if (evaluate("1") || evaluate("2 + 3 = assert") || sf_depth()) ok=0;
    if (evaluate(": across") || !sf_compiling() || evaluate("7 ; across 7 = assert")) ok=0;
    if (evaluate("vertex!")!=2 || evaluate("1 2 vertex!")!=2 ||
        evaluate("6 7 + 13 = assert")) ok=0;
    if (evaluate("2147483648")!=1) ok=0;
    if (evaluate("clear")!=7) ok=0;
    stress=malloc(70000);
    if (!stress) ok=0;
    else {
        for (i=0;i<1025;++i) { stress[i*2]='1'; stress[i*2+1]=' '; }
        stress[2050]=0;
        if (evaluate(stress)!=3 || sf_depth()) ok=0;
        strcpy(stress,": huge ");
        for (i=0;i<32768;++i) { stress[7+i*2]='1'; stress[8+i*2]=' '; }
        stress[65543]=0;
        if (evaluate(stress)!=5 || sf_compiling() || evaluate("huge")!=1) ok=0;
        free(stress);
    }
    if (evaluate("six 6 = assert") || sf_depth()) ok=0;
    if (logfile) fprintf(logfile,"Forth compiler, stacks, numbers and recovery: %s\n",ok?"PASS":"FAIL");
    sf_reset();
    return ok;
}
int main(int argc,char **argv)
{
    char line[1024];
    int result;
    test_mode=argc>1 && strcmp(argv[1],"/test")==0;
    if (test_mode) logfile=fopen("FORTH.LOG","w");
    puts("sectorforth / Glide - 32-bit DOS");
    puts("RGB triangle defined in TRIANGLE.F. Press a key to return to Forth.");
    if (test_mode && !selftest()) failure=1;
    result=load(test_mode || argc==1 ? "TRIANGLE.F" : argv[1]);
    gfx_close();
    if (result) { printf("Forth error %d\n",result); failure=1; }
    if (test_mode) {
        /* Exercise the same compiled word again after returning to text. */
        if (evaluate("show-triangle")) failure=1;
        if (evaluate("show-triangle")) failure=1;
        gfx_close();
        if (captures!=3 || triangles!=3 || sf_depth()) failure=1;
        if (logfile) {
            fprintf(logfile,"%s: sectorforth, 32-bit DOS, Voodoo Glide, RGB triangle\n",failure?"FAIL":"PASS");
            fclose(logfile);
        }
        gfx_shutdown();
        return failure?1:0;
    }
    puts("Ready. 'show-triangle' draws again; 'bye' exits. 'reset' clears the dictionary.");
    for (;;) {
        printf("\n%s> ",sf_compiling()?"compile":"ok");
        if (!fgets(line,sizeof(line),stdin)) break;
        if (!strcmp(line,"bye\n")) break;
        if (!strcmp(line,"reset\n")) { gfx_close(); sf_reset(); continue; }
        result=sf_eval(line);
        if (result) { gfx_close(); printf("error %d (stacks reset; incomplete definition discarded)\n",result); }
    }
    gfx_shutdown();
    return 0;
}
