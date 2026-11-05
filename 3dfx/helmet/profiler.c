/* Minimal DOSBox-X integration-timer client, written for this demo.
   Protocol: include/iglib.h and src/ints/bios.cpp in DOSBox-X.
   Register 4 is emulated milliseconds in unsigned 16.16 format.
   This measures guest CPU/API submission time, not GPU timestamp queries. */
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include "profiler.h"
FrameProfiler profiler;
static unsigned long start,last;
static float stage_sum[P_STAGES],frame_sum;
static unsigned long read_register(unsigned long index)
{
    unsigned long result=0;int i;
    outp(0x2a,0); /* reset the index/data byte latches */
    for(i=0;i<4;++i) { outp(0x28,(unsigned char)index);index>>=8; }
    for(i=0;i<4;++i) result|=(unsigned long)(unsigned char)inp(0x29)<<(i*8);
    return result;
}
void profile_init(void)
{
    memset(&profiler,0,sizeof(profiler));
    profiler.available=read_register(0)==0xd05b0740UL;
}
unsigned long profile_clock(void) { return read_register(4); }
void profile_reset(void)
{
    profiler.count=profiler.next=0;
    memset(profiler.frame,0,sizeof(profiler.frame));
    memset(profiler.stage,0,sizeof(profiler.stage));
    memset(profiler.mean,0,sizeof(profiler.mean));
    profiler.average=profiler.min=profiler.max=profiler.p95=0;
    memset(stage_sum,0,sizeof(stage_sum));frame_sum=0;
}
void profile_toggle(void)
{ profiler.show=!profiler.show;if(profiler.show)profile_reset(); }
void profile_begin(void)
{
    profiler.active=1;
    if(!profiler.available) return;
    memset(profiler.current,0,sizeof(profiler.current));
    start=last=read_register(4);
}
void profile_stage(int stage)
{
    unsigned long now;
    if(!profiler.available || (!profiler.show && !profiler.measure)) return;
    now=read_register(4);profiler.current[stage]=(now-last)/65536.0f;last=now;
}
void profile_end(void)
{
    float sorted[PROFILE_HISTORY],extra;
    int i,j,k,n,slot=profiler.next;
    unsigned long now,finished;
    profiler.active=0;
    if(!profiler.available) return;
    now=read_register(4);
    if(profiler.count==PROFILE_HISTORY) {
        frame_sum-=profiler.frame[slot];
        for(i=0;i<P_STAGES;++i) stage_sum[i]-=profiler.stage[slot][i];
    }
    profiler.frame[profiler.next]=(now-start)/65536.0f;
    frame_sum+=profiler.frame[slot];
    for(i=0;i<P_STAGES;++i) {
        profiler.stage[profiler.next][i]=profiler.current[i];
        stage_sum[i]+=profiler.current[i];
    }
    profiler.next=(profiler.next+1)%PROFILE_HISTORY;
    if(profiler.count<PROFILE_HISTORY) ++profiler.count;
    if(!profiler.show && !profiler.measure) return;
    n=profiler.count;
    for(j=0;j<P_STAGES;++j) profiler.mean[j]=stage_sum[j]/n;
    if((profiler.next&31)==0 || n==1) {
        for(i=0;i<n;++i) sorted[i]=profiler.frame[i];
        for(i=1;i<n;++i) { float value=sorted[i];k=i;while(k>0 && sorted[k-1]>value){sorted[k]=sorted[k-1];--k;}sorted[k]=value; }
        profiler.min=sorted[0];profiler.max=sorted[n-1];profiler.p95=sorted[(95*n+99)/100-1];
    }
    finished=read_register(4);
    extra=(finished-now)/65536.0f;
    profiler.frame[slot]=(finished-start)/65536.0f;
    frame_sum+=extra;profiler.stage[slot][P_OVERLAY]+=extra;stage_sum[P_OVERLAY]+=extra;
    profiler.mean[P_OVERLAY]=stage_sum[P_OVERLAY]/n;
    profiler.average=frame_sum/n;
    if(n==1 || profiler.frame[slot]<profiler.min)profiler.min=profiler.frame[slot];
    if(profiler.frame[slot]>profiler.max)profiler.max=profiler.frame[slot];
}
void profile_log(void *file)
{
    static const char *name[P_STAGES]={"Input","Background","Transform/cull","Opaque","Edge AA","Highlights","HUD","Profiler","Present/wait"
#ifdef PROJECTOR_DEMO
        ,"Projector"
#endif
#ifdef SHADOW_DEMO
        ,"Shadows"
#endif
#ifdef REFLECTION_DEMO
        ,"Reflection"
#endif
#ifdef ENVIRONMENT_DEMO
        ,"Environment"
#endif
    };
    FILE *f=(FILE *)file;int i;
    if(!f) return;
    fprintf(f,"Profiler: %s; %d frames; average %.3f ms, p95 %.3f, max %.3f\n",
        profiler.available?"DOSBox-X 16.16 emulated-ms timer":"timer unavailable",profiler.count,profiler.average,profiler.p95,profiler.max);
    for(i=0;i<P_STAGES;++i) fprintf(f,"  %s: %.3f ms\n",name[i],profiler.mean[i]);
}
