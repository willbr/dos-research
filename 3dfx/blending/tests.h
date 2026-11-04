#include "../textures/camera-tests.h"
#include "control-tests.h"
static int camera_clear(Camera p)
{
    Vec d=sub(p.look,p.eye);
    return p.eye.x==p.eye.x && p.eye.y==p.eye.y && p.eye.z==p.eye.z &&
        fabs(p.eye.x)<1.9f && p.eye.y>1.2f && p.eye.y<3.5f && fabs(p.eye.z)<12.8f && dot(d,d)>.01f;
}
static unsigned long image_changes(unsigned short *a,unsigned short *b)
{
    unsigned long changed=0;int i;if(!a || !b)return 0;
    for(i=88*640;i<432*640;++i)if(a[i]!=b[i])++changed;return changed;
}
static int run_tests(FILE *log,int soak)
{
    int ok=1,i,count=soak?1440:360,slow=0,max_effects=0;float sum=0,max_ms=0;
    unsigned short *all=NULL,*other=NULL;
    if(!soak) {
        ok=blending_controls(log);if(blend_control){test_frames=6;return ok;}
        {
            int transitions;flying=0;camera_home();transitions=camera_switch_test();if(!transitions)ok=0;
            if(log)fprintf(log,"Tour/manual transition continuity and flight pause: %s\n",transitions?"PASS":"FAIL");
        }
        camera_home();yaw=target_yaw=-.6f;pitch=target_pitch=-.12f;zoom=target_zoom=1;
        current_camera=orbit_pose();blend_time=2;render(0,1);all=capture("NAVE.PPM");
        for(i=0;i<5;++i) {
            const char *name=i==0?"GLASS":i==1?"CANDLES":i==2?"MIST":i==3?"LIGHT":"SORTING";
            const char *path=i==0?"NOGLASS.PPM":i==1?"NOCANDLE.PPM":i==2?"NOMIST.PPM":i==3?"NOLIGHT.PPM":"WRONG.PPM";
            int *flag=i==0?&glass_enabled:i==1?&candles_enabled:i==2?&mist_enabled:i==3?&light_enabled:&sorted_blending;
            unsigned long changes;*flag=0;render(0,1);other=capture(path);changes=image_changes(all,other);*flag=1;
            if(!all || !other || changes<40)ok=0;
            if(log)fprintf(log,"Scene %s toggle: %lu changed pixels\n",name,changes);free(other);
        }
        free(all);current_camera.eye=vec(.3f,2.2f,-4.5f);current_camera.look=vec(5.085f,2.9f,-2.5f);
        render(0,1);other=capture("GLASS.PPM");if(!other)ok=0;free(other);
    }
    test_frames=count;flying=1;transition=1;blend_time=0;current_camera=drone_pose(0);render(0,0);render(0,0);profile_reset();profiler.measure=1;
    if(soak){FILE *f=fopen("SOAKSTAR.MRK","w");if(f)fclose(f);else ok=0;}
    for(i=0;i<count;++i) {
        float ms;current_camera=drone_pose(i*(FLIGHT_SECONDS/count));if(!camera_clear(current_camera))ok=0;
        if(i==count/2){profile_toggle();profiler_dirty=1;}
        render(profiler.average>0?(int)(1000/profiler.average+.5f):0,0);
        ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;if(ms>max_ms)max_ms=ms;
        if(ms>18) {
            int j,slot=(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY;++slow;
            if(log && soak){fprintf(log,"Slow frame %d: %.3f ms; stages",i,ms);for(j=0;j<P_STAGES;++j)fprintf(log," %.3f",profiler.stage[slot][j]);fprintf(log,"\n");}
        }
        if(effect_count>max_effects)max_effects=effect_count;
    }
    if(soak){FILE *f=fopen("SOAKEND.MRK","w");if(f)fclose(f);else ok=0;}
    if(log){fprintf(log,"Nave tour: %d frames, %.2f guest FPS, max %.3f ms, %d frames over 18 ms; max %d transparent quads\n",count,count*1000/sum,max_ms,slow,max_effects);log_profile(log);}
    if(!soak){other=capture("PROFILER.PPM");if(!other)ok=0;free(other);}
    if(soak && slow)ok=0;return ok && !finite_error;
}
