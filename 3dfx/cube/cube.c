/* Rainbow cube for DOS / Glide 2.x. Build with Open Watcom. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <conio.h>
#include <glide.h>

/* The RTC advances independently of BIOS timer interrupts, which Glide can
   delay. Measure complete seconds between RTC transitions. */
static int rtc_seconds(void)
{
    int sec, mode;
    outp(0x70,0x0b); mode=inp(0x71);
    outp(0x70,0); sec=inp(0x71);
    return (mode&4) ? sec : (sec>>4)*10+(sec&15);
}

/* Small 3x5 font, drawn by Glide so it appears on the Voodoo output. */
static void draw_fps(const char *text)
{
    static const unsigned short digits[10] = {
        075557,026227,071747,071717,055711,
        074717,074757,071111,075757,075717
    };
    GrVertex q[4];
    int i,row,col;
    memset(q,0,sizeof(q));
    for(i=0;i<4;++i) { q[i].r=160; q[i].g=245; q[i].b=255; q[i].a=255; }
    grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    for(i=0;text[i];++i) {
        unsigned short bits=0;
        if(text[i]>='0' && text[i]<='9') bits=digits[text[i]-'0'];
        else if(text[i]=='F') bits=074744;
        else if(text[i]=='P') bits=075744;
        else if(text[i]=='S') bits=074717;
        else if(text[i]=='-') bits=000700;
        for(row=0;row<5;++row) for(col=0;col<3;++col) {
            float x,y;
            if(!(bits & (1 << ((4-row)*3+2-col)))) continue;
            x=(float)(12+i*12+col*3); y=(float)(12+row*3);
            q[0].x=x; q[0].y=y; q[1].x=x+3; q[1].y=y;
            q[2].x=x+3; q[2].y=y+3; q[3].x=x; q[3].y=y+3;
            grDrawTriangle(&q[0],&q[1],&q[2]);
            grDrawTriangle(&q[0],&q[2],&q[3]);
        }
    }
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
}

static const float points[8][3] = {
    {-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
    {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}
};
static const float colours[8][3] = {
    {255,40,50},{255,145,20},{255,240,25},{45,240,70},
    {15,220,230},{40,85,255},{155,40,255},{255,40,175}
};
static const int triangles[12][3] = {
    {0,1,2},{0,2,3},{4,6,5},{4,7,6},
    {0,4,5},{0,5,1},{3,2,6},{3,6,7},
    {0,3,7},{0,7,4},{1,5,6},{1,6,2}
};

/* Save actual Voodoo framebuffer pixels for the finite verification run. */
static int capture(void)
{
    unsigned short *pixels;
    FILE *f;
    unsigned long i, lit = 0;
    pixels = malloc(640UL * 480 * 2);
    if (!pixels) return 0;
    if (!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,640,480,1280,pixels)) {
        free(pixels); return 0;
    }
    f = fopen("CUBE.PPM", "wb");
    if (!f) { free(pixels); return 0; }
    fprintf(f,"P6\n640 480\n255\n");
    for (i=0; i<640UL*480; ++i) {
        unsigned short p = pixels[i];
        unsigned char rgb[3];
        rgb[0] = ((p>>11)&31)*255/31;
        rgb[1] = ((p>>5)&63)*255/63;
        rgb[2] = (p&31)*255/31;
        if (rgb[0]>60 || rgb[1]>60 || rgb[2]>60) ++lit;
        fwrite(rgb,1,3,f);
    }
    fclose(f); free(pixels);
    return lit > 10000;
}

int main(int argc, char **argv)
{
    GrHwConfiguration hw;
    GrVertex v[8];
    double camera[8][3];
    double angle = 0.45;
    int i, key, paused=0, frames=0, ok=1;
    int fps_second, fps_frames=0, fps_synced=0, fps_samples=0;
    char fps_text[32]="FPS --";
    int benchmark = argc>1 && strcmp(argv[1],"/bench")==0;
    int test = benchmark || (argc>1 && strcmp(argv[1],"/test")==0);
    FILE *log = fopen("CUBE.LOG","w");
    grGlideInit();
    if (!grSstQueryHardware(&hw) || hw.num_sst < 1) {
        grGlideShutdown();
        puts("No Voodoo detected. Use the supplied DOSBox-X configuration.");
        if (log) { fputs("FAIL: No Voodoo\n",log); fclose(log); }
        return 1;
    }
    grSstSelect(0);
    if (!grSstWinOpen(0,GR_RESOLUTION_640x480,GR_REFRESH_60Hz,
                     GR_COLORFORMAT_ABGR,GR_ORIGIN_UPPER_LEFT,2,1)) {
        grGlideShutdown();
        if (log) { fputs("FAIL: Video mode\n",log); fclose(log); }
        return 1;
    }
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,
                   GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grCullMode(GR_CULL_DISABLE);
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);
    grDepthBufferFunction(GR_CMP_GREATER);
    grDepthMask(FXTRUE);
    grDitherMode(GR_DITHER_4x4);
    memset(v,0,sizeof(v));
    fps_second=rtc_seconds();
    if (benchmark) { FILE *mark=fopen("START.MRK","w"); if(mark) fclose(mark); }
    for (;;) {
        while (kbhit()) {
            key=getch();
            if (key==27) goto done;
            if (key==' ') paused=!paused;
        }
        /* One smooth animation step per 60 Hz Voodoo refresh. BIOS clock()
           only advances at about 18 Hz and visibly stair-steps rotation. */
        if (!paused) angle+=0.8/60.0;
        for (i=0;i<8;++i) {
            double x=points[i][0], y=points[i][1], z=points[i][2];
            double a=angle, b=angle*0.73, c=angle*0.31;
            double xx=x*cos(a)+z*sin(a), zz=-x*sin(a)+z*cos(a);
            double yy=y*cos(b)-zz*sin(b);
            double depth=y*sin(b)+zz*cos(b)+5.3;
            x=xx*cos(c)-yy*sin(c); y=xx*sin(c)+yy*cos(c);
            camera[i][0]=x; camera[i][1]=y; camera[i][2]=depth;
            v[i].x=(float)(320+x*520/depth);
            v[i].y=(float)(240-y*520/depth);
            v[i].ooz=(float)(65535-depth*6000);
            v[i].oow=(float)(1.0/depth);
            v[i].r=colours[i][0]; v[i].g=colours[i][1];
            v[i].b=colours[i][2]; v[i].a=255;
        }
        grBufferClear(0x00160c08,0,GR_ZDEPTHVALUE_FARTHEST);
        for (i=0;i<12;++i) {
            int j, a=triangles[i][0], b=triangles[i][1], c=triangles[i][2];
            double ux=camera[b][0]-camera[a][0], uy=camera[b][1]-camera[a][1];
            double uz=camera[b][2]-camera[a][2], vx=camera[c][0]-camera[a][0];
            double vy=camera[c][1]-camera[a][1], vz=camera[c][2]-camera[a][2];
            double nx=uy*vz-uz*vy, ny=uz*vx-ux*vz, nz=ux*vy-uy*vx;
            double light=0.4+0.6*fabs((-0.3*nx+0.4*ny-0.866*nz)/sqrt(nx*nx+ny*ny+nz*nz));
            GrVertex face[3];
            for (j=0;j<3;++j) {
                face[j]=v[triangles[i][j]];
                face[j].r*=light; face[j].g*=light; face[j].b*=light;
            }
            grDrawTriangle(&face[0],&face[1],&face[2]);
        }
        draw_fps(fps_text);
        /* DOSBox-X's OpenGL Voodoo backend may execute swap commands
           immediately, regardless of swap_interval. Explicitly wait for
           the next rising vertical-retrace edge before presenting. */
        while (grSstVRetraceOn()) { }
        while (!grSstVRetraceOn()) { }
        grBufferSwap(1);
        /* A queued swap is not a displayed frame. Keep only one in flight. */
        while (grBufferNumPending() > 0) { }
        grSstIdle();
        ++frames;
        ++fps_frames;
        {
            int second=rtc_seconds();
            int seconds=(second-fps_second+60)%60;
            if(seconds) {
                /* Discard the partial second at startup. */
                if(fps_synced) {
                    sprintf(fps_text,"FPS %d",fps_frames/seconds);
                    ++fps_samples;
                }
                fps_synced=1; fps_frames=0; fps_second=second;
            }
        }
        if (test && (benchmark ? frames>=600 : (frames>=120 && fps_samples>=2))) {
            if (benchmark) { FILE *mark=fopen("END.MRK","w"); if(mark) fclose(mark); }
            ok=capture(); break;
        }
    }
done:
    grGlideShutdown();
    if (log) {
        fprintf(log,"HUD: %s\n",fps_text);
        fprintf(log,"%s: Voodoo Glide, 640x480, %d frames%s\n",
                ok?"PASS":"FAIL",frames,test?", framebuffer capture":"");
        fclose(log);
    }
    puts("Rainbow cube - thanks for watching!");
    return ok?0:1;
}
