#include "../blending/control-tests.h"
#include "../textures/camera-tests.h"
static void fog_control_rect(float distance)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i) {
        q[i].x=(i==1||i==2)?380:260;q[i].y=i<2?180:280;
        q[i].r=200;q[i].g=100;q[i].b=40;q[i].a=128;q[i].ooz=10000;q[i].oow=1/distance;
    }
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static int fog_controls(FILE *log)
{
    int ok=1,i,j;GrFog_t table[GR_FOG_TABLE_SIZE];
    for(i=0;i<3;++i) {
        int f=i==0?0:i==1?128:255;
        for(j=0;j<GR_FOG_TABLE_SIZE;++j)table[j]=(GrFog_t)f;
        control_clear();grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        grFogTable(table);fog_apply(0);fog_control_rect(16);
        if(!control_sample(log,i==0?"zero fog":i==1?"half fog":"full fog",200+(52-200)*f/255,100+(65-100)*f/255,40+(78-40)*f/255))ok=0;
    }
    control_clear();grFogTable(table);fog_apply(1);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);fog_control_rect(16);
    if(!control_sample(log,"full fog removes additive light",0,0,0))ok=0;
    control_clear();grFogMode(GR_FOG_DISABLE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);fog_control_rect(16);
    if(!control_sample(log,"disabled fog bypasses full table",200,100,40))ok=0;
    for(j=0;j<GR_FOG_TABLE_SIZE;++j)table[j]=128;
    control_clear();grFogTable(table);fog_apply(0);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);fog_control_rect(16);
    if(!control_sample(log,"fog preserves independent transparency",63,41,29))ok=0;
    control_clear();fog_apply(1);grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);fog_control_rect(16);
    if(!control_sample(log,"additive light attenuates without fog color",50,25,10))ok=0;
    for(i=0;i<3;++i)for(j=0;j<3;++j) {
        float w=j==0?4:j==1?16:48,amount;
        int er,eg,eb;fog_curve=i;fog_update();
        amount=i==0?clamp((w-3)/(1.8f/fog_density-3),0,1):i==1?1-(float)exp(-fog_density*w):1-(float)exp(-fog_density*fog_density*w*w);
        er=(int)(200+(52-200)*amount);eg=(int)(100+(65-100)*amount);eb=(int)(40+(78-40)*amount);
        control_clear();grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);fog_apply(0);fog_control_rect(w);
        if(log)fprintf(log,"Curve %d at W %.1f: ",i,w);
        if(!control_sample(log,"depth table",er,eg,eb))ok=0;
    }
    fog_curve=2;fog_update();grFogMode(GR_FOG_DISABLE);return ok;
}
static int journey_clear(Camera p)
{
    Vec d=sub(p.look,p.eye);int side,i,j;
    if(p.eye.x!=p.eye.x || p.eye.y<1.2f || p.eye.y>5 || p.eye.z!=p.eye.z || dot(d,d)<.01f)return 0;
    if(p.eye.z>-14.5f && p.eye.z<-13.6f && (fabs(p.eye.x)>1.25f || p.eye.y>2.7f))return 0;
    if(p.eye.z>=-14 && (fabs(p.eye.x)>1.9f || p.eye.z>12.8f))return 0;
    if(p.eye.z< -14 && (fabs(p.eye.x)>16 || p.eye.z< -55))return 0;
    for(side=-1;side<=1;side+=2)for(j=0;j<5;++j)for(i=0;i<3;++i)
        if(fabs(p.eye.x-side*(6+i*4.2f))<.85f && p.eye.z> -23-j*7-.7f && p.eye.z< -23-j*7+2.3f)return 0;
    return 1;
}
static int run_tests(FILE *log,int soak)
{
    int ok=1,i,count=soak?1440:360,slow=0,inside=0,outside=0,portal=0;
    float max_ms=0,sum=0;unsigned short *a,*b;
    if(!soak) {
        ok=fog_controls(log);if(blend_control){test_frames=16;return ok;}
        flying=0;camera_home();if(!camera_switch_test())ok=0;
        for(i=0;i<3;++i) {
            const char *name=i==0?"NAVE.PPM":i==1?"PORTAL.PPM":"YARD.PPM";
            unsigned long changed=0;int pixel;
            current_camera.eye=i==0?vec(0,1.9f,4):i==1?vec(0,1.9f,-12):vec(-3,2.8f,-33);
            current_camera.look=i==2?vec(-14,2,-47):vec(0,2,-30);
            fog_enabled=1;render(0,1);a=capture(name);fog_enabled=0;render(0,1);b=capture(i==2?"CLEAR.PPM":"NOFOG.PPM");fog_enabled=1;
            if(a && b)for(pixel=88*640;pixel<432*640;++pixel)if(a[pixel]!=b[pixel])++changed;
            if(!a || !b || changed<100)ok=0;
            if(log)fprintf(log,"Scene %s fog toggle: %lu changed pixels\n",name,changed);free(a);free(b);
        }
        for(i=0;i<16;++i) {
            int pixel;unsigned long changes=0;current_camera=path_at((float)i);
            fog_culling_enabled=1;render(0,1);a=capture("CULLED.PPM");
            fog_culling_enabled=0;render(0,1);b=capture("FULL.PPM");fog_culling_enabled=1;
            if(a && b)for(pixel=88*640;pixel<432*640;++pixel)if(a[pixel]!=b[pixel])++changes;
            if(!a || !b || changes){ok=0;if(log)fprintf(log,"Culling comparison shot %d: %lu changed pixels\n",i,changes);}
            free(a);free(b);
        }
        if(log)fprintf(log,"Culled/full framebuffer comparisons: %s\n",ok?"PASS":"FAIL");
        for(i=0;i<16;++i) {
            Camera old=path_at((float)i);Vec delta,heading;
            current_camera=old;flying=1;fly_mode(0);update_camera(2,1);
            delta=sub(old.eye,current_camera.eye);heading=unit(sub(old.look,old.eye));
            if(dot(delta,delta)>.000001f || dot(heading,unit(sub(current_camera.look,current_camera.eye)))<.999f)ok=0;
        }
        if(log)fprintf(log,"Manual look preserves outdoor/indoor position and direction: %s\n",ok?"PASS":"FAIL");
    }
    /* Dense route sampling checks doorway clearance independently of frame count. */
    for(i=0;i<8192;++i) {
        Camera p=drone_pose(i*(FLIGHT_SECONDS/8192));
        if(!journey_clear(p)){if(log)fprintf(log,"Blocked path at sample %d: %.3f %.3f %.3f\n",i,p.eye.x,p.eye.y,p.eye.z);ok=0;break;}
        if(p.eye.z> -13.6f)++inside;else if(p.eye.z< -14.5f)++outside;else ++portal;
    }
    if(!inside || !outside || !portal)ok=0;
    if(log)fprintf(log,"Path samples: nave %d, doorway %d, graveyard %d; %s\n",inside,portal,outside,ok?"PASS":"FAIL");
    test_frames=count;flying=1;transition=1;fog_enabled=1;current_camera=drone_pose(0);render(0,0);render(0,0);profile_reset();profiler.measure=1;
    if(soak){FILE *f=fopen("SOAKSTAR.MRK","w");if(f)fclose(f);else ok=0;}
    for(i=0;i<count;++i) {
        float ms;current_camera=drone_pose(i*(FLIGHT_SECONDS/count));
        if(i==count/2){profile_toggle();profiler_dirty=1;}
        render(profiler.average>0?(int)(1000/profiler.average+.5f):0,0);
        ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;if(ms>max_ms)max_ms=ms;
        if(ms>18) {
            ++slow;if(log && soak && slow<=12) {
                int j,slot=(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY;
                fprintf(log,"Slow frame %d, position %.2f %.2f %.2f; stages",i,current_camera.eye.x,current_camera.eye.y,current_camera.eye.z);
                for(j=0;j<P_STAGES;++j)fprintf(log," %.3f",profiler.stage[slot][j]);fprintf(log,"\n");
            }
        }
    }
    if(soak){FILE *f=fopen("SOAKEND.MRK","w");if(f)fclose(f);else ok=0;}
    if(log){fprintf(log,"Journey: %d frames, %.2f guest FPS, max %.3f ms, %d over 18 ms\n",count,count*1000/sum,max_ms,slow);log_profile(log);}
    return ok && !finite_error;
}
