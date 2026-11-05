static int rain_tests(FILE *log,int controls_only)
{
    int ok=water_controls(log),i,hits=0;unsigned short *dry,*wet,*repeat;unsigned long changed,same;
    Vec calm,storm;float saved_time;RainDrop saved;
    if(!foliage_controls(log))ok=0;
    /* Weather obeys the same foreground and depth rules as other particles. */
    control_clear();grFogMode(GR_FOG_DISABLE);local_color();grDepthMask(FXTRUE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    control_rect(0,200,0,255,30000,0);rain_state();grFogMode(GR_FOG_DISABLE);
    control_rect(160,195,205,128,20000,0);
    if(!control_sample(log,"rain rejected by foreground",0,200,0))ok=0;
    control_clear();rain_state();grFogMode(GR_FOG_DISABLE);control_rect(160,195,205,128,20000,0);
    local_color();grDepthMask(FXTRUE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    control_rect(0,0,200,255,15000,0);
    if(!control_sample(log,"rain preserves opaque depth",0,0,200))ok=0;
    if(controls_only)return ok;
    current_camera.eye=vec(-2,1.7f,-23.7f);current_camera.look=vec(-6,WATER_Y,-28);
    foliage_init();water_init();rain_init();foliage_benchmark=1;
    for(i=0;i<10;++i)rain_key('j');if(rain_level!=0)ok=0;
    for(i=0;i<10;++i)rain_key('k');if(rain_level!=3)ok=0;
    wind_enabled=1;calm=rain_velocity(0);storm=rain_velocity(1);
    if(storm.x<=calm.x || storm.z>=calm.z || storm.y!=calm.y)ok=0;
    wind_enabled=0;calm=rain_velocity(1);if(calm.x || calm.z)ok=0;wind_enabled=1;
    saved=rain_drops[0];saved_time=rain_accumulator;foliage_benchmark=0;rain_update(1);foliage_benchmark=1;
    if(memcmp(&saved,&rain_drops[0],sizeof(saved)) || saved_time!=rain_accumulator)ok=0;
    for(i=0;i<600;++i)rain_tick(1/60.0f);
    for(i=0;i<RAIN_HITS;++i)if(rain_hits[i].active){++hits;if(!rain_water_hit(rain_hits[i].x,rain_hits[i].z) || rain_hits[i].age<0 || rain_hits[i].age>=.8f)ok=0;}
    if(rain_impacts<20 || !hits || rain_next_hit<0 || rain_next_hit>=RAIN_HITS)ok=0;
    if(log)fprintf(log,"Rain intensity clamps, gust direction, pause, bounded impacts: %d impacts, %d active; %s\n",rain_impacts,hits,ok?"PASS":"FAIL");
    i=rain_impacts;rain_impact_wait=0;rain_impact(0,0);rain_impact_wait=0;rain_impact(-6,-23);
    if(rain_impacts!=i)ok=0;
    rain_level=0;render(0,1);dry=capture("DRY.PPM");
    rain_level=3;render(0,1);wet=capture("RAIN.PPM");
    changed=foliage_image_changes(dry,wet);
    render(0,1);repeat=capture("REPEAT.PPM");same=foliage_image_changes(wet,repeat);
    if(!dry || !wet || !repeat || changed<100 || same || !rain_rings || !rain_streaks)ok=0;
    if(log)fprintf(log,"Weather capture: %lu changed pixels, %lu paused differences, %d streaks, %d ring sprites, %d splashes; %s\n",changed,same,rain_streaks,rain_rings,rain_splashes,ok?"PASS":"FAIL");
    free(dry);free(wet);free(repeat);
    rain_level=0;for(i=0;i<60;++i)rain_tick(1/60.0f);
    for(i=0;i<RAIN_HITS;++i)if(rain_hits[i].active)ok=0;
    rain_level=2;foliage_benchmark=0;rain_init();return ok && !finite_error;
}
