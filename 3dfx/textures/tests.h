#include "camera-tests.h"
static int run_tests(FILE *log,int soak)
{
    unsigned short *a=NULL,*b=NULL;int ok=1,n,count=soak?1440:600,slow=0,max_drawn=0,total_clipped=0;
    unsigned long changes=0,lit=0;float sum=0,max_ms=0,min_step=100,max_step=0;Camera last=drone_pose(0);
    profiler.measure=1;test_frames=count;
    if(!soak) {
        int controls=camera_switch_test();if(!controls)ok=0;
        if(log)fprintf(log,"Orbit/drone transitions preserve position; flight pause: %s\n",controls?"PASS":"FAIL");
        render(0,1);a=capture("CATHEDRL.PPM");if(!a || drawn<200)ok=0;free(a);a=NULL;
        checker=1;render(0,1);a=capture("PERSPECT.PPM");affine=1;render(0,1);b=capture("AFFINE.PPM");
        if(!a || !b)ok=0;
        else for(n=88*640;n<432*640;++n) {
            if(a[n]!=b[n])++changes;
            if(((a[n]>>11)&31)>8 || ((a[n]>>5)&63)>16)++lit;
        }
        if(changes<2000 || lit<15000)ok=0;
        if(log)fprintf(log,"Same-camera texture perspective: %lu changed pixels, %lu lit pixels; depth and geometry unchanged\n",changes,lit);
        free(a);free(b);a=b=NULL;affine=checker=0;
        compare=1;render(0,1);a=capture("SPLIT.PPM");if(!a)ok=0;free(a);compare=0;
        current_camera=drone_pose(0);render(0,1);a=capture("WEST.PPM");if(!a)ok=0;free(a);
        current_camera=drone_pose(FLIGHT_SECONDS*.48f);render(0,1);a=capture("EAST.PPM");if(!a)ok=0;free(a);
        current_camera.eye=vec(-1.9f,3,-22.5f);current_camera.look=vec(0,3,-15);render(0,1);
        a=capture("CLIP.PPM");if(!a || clipped==0)ok=0;free(a);
        if(log)fprintf(log,"Near/frustum clipping: %d clipped triangles, finite %s\n",clipped,finite_error?"FAIL":"PASS");
        {
            float split_sum=0,split_max=0;compare=1;profile_reset();
            for(n=0;n<120;++n) {
                float ms;current_camera=drone_pose(n*(FLIGHT_SECONDS/120));render(0,0);
                ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];split_sum+=ms;if(ms>split_max)split_max=ms;
            }
            if(log)fprintf(log,"Split comparison: 120 moving frames, %.2f emulated FPS, max %.3f ms\n",120000/split_sum,split_max);
            compare=0;
        }
    }
    flying=1;transition=1;current_camera=drone_pose(0);render(60,0);render(60,0);profile_reset();
    if(soak){FILE *mark=fopen("SOAKSTAR.MRK","w");if(mark)fclose(mark);else ok=0;}
    for(n=0;n<count;++n) {
        float ms;Vec delta;
        current_camera=drone_pose(n*(FLIGHT_SECONDS/count));if(!safe_drone(current_camera))ok=0;
        delta=sub(current_camera.eye,last.eye);
        if(n) {float step=(float)sqrt(dot(delta,delta));if(step<min_step)min_step=step;if(step>max_step)max_step=step;}
        last=current_camera;
        if(n==count/2){profile_toggle();profiler_dirty=1;}
        render(profiler.average>0?(int)(1000/profiler.average+.5f):0,0);ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;if(ms>max_ms)max_ms=ms;
        if(ms>18) {
            int j,slot=(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY;++slow;
            if(log && soak){fprintf(log,"Slow frame %d: %.3f ms, %d draws; stages",n,ms,drawn);for(j=0;j<P_STAGES;++j)fprintf(log," %.3f",profiler.stage[slot][j]);fprintf(log,"\n");}
        }
        if(drawn>max_drawn)max_drawn=drawn;total_clipped+=clipped;
    }
    if(soak){FILE *mark=fopen("SOAKEND.MRK","w");if(mark)fclose(mark);else ok=0;}
    {Camera start=drone_pose(0),end=drone_pose(FLIGHT_SECONDS);Vec delta=sub(start.eye,end.eye);if(dot(delta,delta)>.000001f || max_step>min_step*1.05f)ok=0;}
    if(log) {
        fprintf(log,"Drone path: %d frames, %.2f emulated FPS, max %.3f ms, %d frames over 18 ms; max %d draws; %d clipped triangles\n",count,count*1000/sum,max_ms,slow,max_drawn,total_clipped);
        fprintf(log,"Closed constant-speed path: step %.5f..%.5f units; camera clearance %s\n",min_step,max_step,ok?"PASS":"FAIL");log_profile(log);
    }
    if(!soak){a=capture("PROFILER.PPM");if(!a)ok=0;free(a);}
    if(soak && slow)ok=0;
    return ok && !finite_error;
}
