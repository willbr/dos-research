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
#ifdef LIGHTMAP_DEMO
#include "../lightmap/tests.h"
#endif
#ifdef PROJECTOR_DEMO
#include "../project/tests.h"
#endif
#ifdef SHADOW_DEMO
#include "../shadows/tests.h"
#endif
static int submission_comparisons(FILE *log)
{
    int pose,mode,ok=1;unsigned short *a,*b;
#ifdef PROJECTOR_DEMO
    int old_grid=projector_grid;projector_grid=1;
#endif
    for(pose=0;pose<4;++pose)for(mode=0;mode<2;++mode) {
        int pixel,allowed=220;unsigned long significant=0;
#ifdef PROJECTOR_DEMO
        /* Projective grid edges amplify native fixed-point gradient rounding. */
        if(mode==1)allowed=660;
#endif
        current_camera=path_at(pose==0?0:pose==1?3:pose==2?8:14);
        flat_enabled=quad_enabled=1;render(0,1);a=capture(pose==0 && mode==1?"OPT0.PPM":"OPT.PPM");
        if(mode==0)flat_enabled=0;else quad_enabled=0;
        render(0,1);b=capture(pose==0 && mode==1?"REF0.PPM":"REF.PPM");flat_enabled=quad_enabled=1;
        if(a && b)for(pixel=88*640;pixel<432*640;++pixel) {
            int r=abs((int)(a[pixel]>>11)-(int)(b[pixel]>>11))*255/31;
            int g=abs((int)((a[pixel]>>5)&63)-(int)((b[pixel]>>5)&63))*255/63;
            int blue=abs((int)(a[pixel]&31)-(int)(b[pixel]&31))*255/31;
            if(r>16 || g>16 || blue>16)++significant;
        }
        /* Allow subpixel edge/diagonal rounding, never missing surfaces. */
        if(!a || !b || significant>(unsigned long)allowed)ok=0;
        if(log)fprintf(log,"Submission %s pose %d: %lu pixels over 16/255; %s\n",mode?"quad/triangle":"constant/iterated",pose,significant,ok?"PASS":"FAIL");
        free(a);free(b);
    }
#ifdef PROJECTOR_DEMO
    projector_grid=old_grid;
#endif
    return ok;
}
static int run_tests(FILE *log,int soak)
{
    int ok=1,i,count=soak==3?600:soak==2?6000:soak?1440:360,slow=0,inside=0,outside=0,portal=0;
    float max_ms=0,sum=0;unsigned short *a,*b;
    if(!soak) {
        ok=fog_controls(log);
#ifdef LIGHTMAP_DEMO
        if(!lightmap_controls(log))ok=0;
#endif
#ifdef PROJECTOR_DEMO
        if(!projector_controls(log))ok=0;
#endif
#ifdef SHADOW_DEMO
        if(!shadow_controls(log))ok=0;
#endif
        if(blend_control){
#ifdef LIGHTMAP_DEMO
            test_frames=20;
#ifdef PROJECTOR_DEMO
            test_frames+=7;
#ifdef SHADOW_DEMO
            test_frames+=4;
#endif
#endif
#else
            test_frames=16;
#endif
            return ok;
        }
#ifdef LIGHTMAP_DEMO
        if(!lightmap_scene_check(log))ok=0;
#endif
#ifdef PROJECTOR_DEMO
        if(!projector_scene_checks(log))ok=0;
#endif
#ifdef SHADOW_DEMO
        if(!shadow_scene_checks(log))ok=0;
#endif
        if(!submission_comparisons(log))ok=0;
        flying=0;camera_home();{
            int camera_ok=camera_switch_test();if(!camera_ok)ok=0;
            if(log)fprintf(log,"Camera switch/pause continuity: %s\n",camera_ok?"PASS":"FAIL");
        }
        for(i=0;i<3;++i) {
            const char *name=i==0?"NAVE.PPM":i==1?"PORTAL.PPM":"YARD.PPM";
            unsigned long changed=0;int pixel;
            current_camera.eye=i==0?vec(0,1.9f,4):i==1?vec(0,1.9f,-12):vec(-3,2.8f,-33);
            current_camera.look=i==2?vec(-14,2,-47):vec(0,2,-30);
            fog_enabled=1;render(0,1);
            if(log) {
                int n,transformed=0,eligible=0;for(n=0;n<vertex_count;++n)if(used[n])++transformed;
                for(n=0;n<MATERIALS;++n)eligible+=visible_count[n];
                fprintf(log,"Work %s: %d transformed vertices, %d eligible faces, %d drawn, %d clipped; %d apertures\n",name,transformed,eligible,drawn,clipped,fog_portal_count);
            }
            a=capture(name);fog_enabled=0;render(0,1);b=capture(i==2?"CLEAR.PPM":"NOFOG.PPM");fog_enabled=1;
            if(a && b)for(pixel=88*640;pixel<432*640;++pixel)if(a[pixel]!=b[pixel])++changed;
            if(!a || !b || changed<100)ok=0;
            if(log)fprintf(log,"Scene %s fog toggle: %lu changed pixels\n",name,changed);free(a);free(b);
        }
        for(i=0;i<16;++i) {
            int pixel;unsigned long changes=0;current_camera=path_at((float)i);
            fog_culling_enabled=1;render(0,1);a=capture(i==8?"CULL8.PPM":"CULLED.PPM");
            fog_culling_enabled=0;render(0,1);b=capture(i==8?"FULL8.PPM":"FULL.PPM");fog_culling_enabled=1;
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
#ifdef SHADOW_DEMO
    /* Marker disk I/O must precede the existing two retrace warmup frames.
       Include that warmup in the host interval, rather than disturbing the
       first measured guest frame after it has synchronized to retrace. */
    if(soak){FILE *f=fopen("SOAKSTAR.MRK","w");if(f)fclose(f);else ok=0;}
#endif
    test_frames=count;flying=1;transition=1;fog_enabled=1;current_camera=drone_pose(0);render(0,0);render(0,0);profile_reset();profiler.measure=1;
#ifndef SHADOW_DEMO
    if(soak){FILE *f=fopen("SOAKSTAR.MRK","w");if(f)fclose(f);else ok=0;}
#endif
    for(i=0;i<count;++i) {
        float ms,seconds=soak==3?94+i*(6.0f/count):i*(FLIGHT_SECONDS/count);profile_begin();current_camera=drone_pose(seconds);profile_stage(P_INPUT);
#ifdef PROJECTOR_DEMO
        projector_time=seconds;
#endif
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
#ifdef PROJECTOR_DEMO
    if(log)fprintf(log,"Projector final frame: %d cached polygons, %d clipped intersections, %d beam triangles\n",projector_count,projector_intersections,projector_triangles);
#endif
#ifdef SHADOW_DEMO
    if(log)fprintf(log,"Shadow volumes %d; final frame: %d polygons, %d triangles, piers %d graves %d\n",shadow_caster_count,shadow_polygons,shadow_triangles,shadow_pier_hits,shadow_grave_hits);
#endif
    if(soak && slow)ok=0;
    return ok && !finite_error;
}
