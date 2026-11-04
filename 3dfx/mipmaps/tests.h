#include "../textures/camera-tests.h"
static int classify_level(unsigned short pixel)
{
    float r=(float)((pixel>>11)&31)/31,g=(float)((pixel>>5)&63)/63,b=(float)(pixel&31)/31;
    float peak=r>g?r:g,best=100;int i,index=-1;
    if(b>peak)peak=b;if(peak<.06f)return -1;r/=peak;g/=peak;b/=peak;
    for(i=0;i<9;++i) {
        float cr=level_colors[i][0],cg=level_colors[i][1],cb=level_colors[i][2],m=cr>cg?cr:cg,err;
        if(cb>m)m=cb;cr/=m;cg/=m;cb/=m;
        err=(r-cr)*(r-cr)+(g-cg)*(g-cg)+(b-cb)*(b-cb);
        if(err<best){best=err;index=i;}
    }
    return best<.04f?index:-1;
}
static int run_tests(FILE *log,int soak)
{
    unsigned short *off=NULL,*on=NULL,*mask=NULL,*negative=NULL,*positive=NULL,*shot;
    int ok=1,n,count=soak?1440:600,slow=0;float sum=0,max_ms=0,min_step=100,max_step=0;
    Camera last=drone_pose(0);profiler.measure=1;test_frames=count;
    if(!soak) {
        unsigned short block[4]={0xf800,0x07e0,0x001f,0xffff};
        unsigned long changes=0,lit=0,samples=0;double neg_sum=0,pos_sum=0;
        int controls=camera_switch_test();mip_reduce(block,2);
        if(block[0]!=0x8410 || mip_uploads!=54 || !controls)ok=0;
        if(log)fprintf(log,"RGB565 reduction: %04x expected 8410; %d mip-level uploads expected 54; camera transitions %s\n",block[0],mip_uploads,controls?"PASS":"FAIL");
        mip_enabled=0;render(0,1);off=capture("OFF.PPM");mip_enabled=1;render(0,1);on=capture("ON.PPM");
        if(!off || !on)ok=0;
        else for(n=88*640;n<432*640;++n) {
            if(off[n]!=on[n])++changes;
            if(((on[n]>>11)&31)>8 || ((on[n]>>5)&63)>16)++lit;
        }
        if(changes<3000 || lit<15000)ok=0;
        if(log)fprintf(log,"Same-camera mipmaps OFF/ON: %lu changed scene pixels, %lu lit pixels\n",changes,lit);
        free(off);free(on);
        compare=1;render(0,1);shot=capture("SPLIT.PPM");if(!shot)ok=0;free(shot);compare=0;
        mip_colored=1;mip_enabled=0;render(0,1);mask=capture("BASECOL.PPM");
        mip_enabled=1;render(0,1);on=capture("LEVELS.PPM");if(!on)ok=0;
        lod_bias=-2;render(0,1);negative=capture("BIASNEG.PPM");lod_bias=2;render(0,1);positive=capture("BIASPOS.PPM");
        if(!mask || !negative || !positive)ok=0;
        else for(n=88*640;n<432*640;n+=2) {
            int r=(mask[n]>>11)&31,g=((mask[n]>>5)&63)/2,b=mask[n]&31,a,c;
            if(r<6 || r<g*1.8f || r<b*1.8f)continue;
            a=classify_level(negative[n]);c=classify_level(positive[n]);
            if(a<0 || c<0)continue;neg_sum+=a;pos_sum+=c;++samples;
        }
        if(samples<1000 || pos_sum<=neg_sum+samples)ok=0;
        if(log)fprintf(log,"Colored LOD bias: %lu matched pixels; mean level at -2 %.3f / +2 %.3f (larger = coarser)\n",samples,samples?neg_sum/samples:0,samples?pos_sum/samples:0);
        free(mask);free(negative);free(positive);lod_bias=.5f;mip_dither=1;
        render(0,1);shot=capture("DITHER.PPM");changes=0;
        if(!on || !shot)ok=0;
        else for(n=88*640;n<432*640;++n)if(on[n]!=shot[n])++changes;
        if(changes<500)ok=0;
        if(log)fprintf(log,"Nearest/dither colored levels: %lu changed scene pixels\n",changes);
        free(on);free(shot);mip_dither=mip_colored=0;
        if(mip_quick){test_frames=8;return ok && !finite_error;}
        current_camera.eye=vec(-1.9f,3,-22.5f);current_camera.look=vec(0,3,-15);render(0,1);
        if(!clipped || finite_error)ok=0;
        if(log)fprintf(log,"Near/frustum clipping: %d clipped triangles, finite %s\n",clipped,finite_error?"FAIL":"PASS");
        for(n=0;n<3;++n) {
            int i;float work=0,frame=0;
            mip_enabled=n!=0;mip_dither=n==2;profile_reset();
            for(i=0;i<120;++i) {
                int slot;current_camera=drone_pose(i*(FLIGHT_SECONDS/120));render(0,0);slot=(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY;
                work+=profiler.frame[slot]-profiler.stage[slot][P_PRESENT];frame+=profiler.frame[slot];
            }
            if(log)fprintf(log,"Moving mode %s: %.2f emulated FPS; work %.3f ms\n",n==0?"OFF":n==1?"NEAREST":"DITHER",120000/frame,work/120);
        }
    }
    mip_enabled=1;mip_dither=mip_colored=compare=0;lod_bias=.5f;flying=1;transition=1;
    current_camera=drone_pose(0);render(0,0);render(0,0);profile_reset();
    if(soak){FILE *f=fopen("SOAKSTAR.MRK","w");if(f)fclose(f);else ok=0;}
    for(n=0;n<count;++n) {
        float ms;Vec delta;current_camera=drone_pose(n*(FLIGHT_SECONDS/count));if(!safe_drone(current_camera))ok=0;
        delta=sub(current_camera.eye,last.eye);
        if(n){float step=(float)sqrt(dot(delta,delta));if(step<min_step)min_step=step;if(step>max_step)max_step=step;}last=current_camera;
        if(n==count/2){profile_toggle();profiler_dirty=1;}
        render(profiler.average>0?(int)(1000/profiler.average+.5f):0,0);
        ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;if(ms>max_ms)max_ms=ms;if(ms>18)++slow;
    }
    if(soak){FILE *f=fopen("SOAKEND.MRK","w");if(f)fclose(f);else ok=0;}
    {Camera a=drone_pose(0),b=drone_pose(FLIGHT_SECONDS);Vec d=sub(a.eye,b.eye);if(dot(d,d)>.000001f || max_step>min_step*1.05f)ok=0;}
    if(log){fprintf(log,"Mipmap drone tour: %d frames, %.2f emulated FPS, max %.3f ms, %d frames over 18 ms; steps %.5f..%.5f\n",count,count*1000/sum,max_ms,slow,min_step,max_step);log_profile(log);}
    if(!soak){shot=capture("PROFILER.PPM");if(!shot)ok=0;free(shot);}
    if(soak && slow)ok=0;return ok && !finite_error;
}
static int run_mip_benchmark(FILE *log)
{
    int phase,i,ok=1;test_frames=720;profiler.measure=1;
    compare=mip_colored=0;lod_bias=.5f;
    for(phase=0;phase<3;++phase) {
        char name[20];FILE *f;double work=0,elapsed=0;
        mip_enabled=phase!=0;mip_dither=phase==2;current_camera=drone_pose(0);
        render(0,0);render(0,0);profile_reset();
        sprintf(name,"P%dSTART.MRK",phase);f=fopen(name,"w");if(f)fclose(f);else ok=0;
        for(i=0;i<240;++i) {
            int slot;current_camera=drone_pose(i*(FLIGHT_SECONDS/240));render(0,0);
            slot=(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY;
            elapsed+=profiler.frame[slot];work+=profiler.frame[slot]-profiler.stage[slot][P_PRESENT];
        }
        sprintf(name,"P%dEND.MRK",phase);f=fopen(name,"w");if(f)fclose(f);else ok=0;
        if(log)fprintf(log,"Benchmark %s: 240 frames, %.2f guest FPS, %.4f ms guest CPU/API work\n",phase==0?"OFF":phase==1?"NEAREST":"DITHER",240000/elapsed,work/240);
    }
    return ok && !finite_error;
}
