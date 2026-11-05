static void water_control_probe(int layer,float alpha,float depth)
{
    GrVertex p[4];int i;memset(p,0,sizeof(p));
    for(i=0;i<4;++i) {
        p[i].x=(i==1||i==2)?380:260;p[i].y=i<2?180:280;p[i].a=alpha;p[i].ooz=depth;
        p[i].oow=p[i].tmuvtx[0].oow=1;p[i].tmuvtx[0].sow=water_probe_u[layer];p[i].tmuvtx[0].tow=water_probe_v[layer];
    }
    grDrawTriangle(&p[0],&p[1],&p[2]);grDrawTriangle(&p[0],&p[2],&p[3]);
}
static int water_controls(FILE *log)
{
    int mode,ok=reflection_controls(log);GrFog_t table[GR_FOG_TABLE_SIZE];
    for(mode=0;mode<7;++mode) {
        int i,layers=mode==6?2:1,er=40,eg=60,eb=80;float alpha=mode==5?0:160;
        float expected[3]={40,60,80};
        control_clear();grFogMode(GR_FOG_DISABLE);grDepthMask(FXTRUE);
        grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        control_rect(mode==1?0:40,mode==1?200:60,mode==1?0:80,255,mode==1?30000:10000,0);
        for(i=0;i<GR_FOG_TABLE_SIZE;++i)table[i]=mode==3?255:mode==4?128:0;
        grFogTable(table);fog_enabled=mode==3 || mode==4;
        for(i=0;i<layers;++i) {
            water_state(i);grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
            water_control_probe(i,alpha,20000);
            {
                int channel;float amount=water_probe[i]/255.0f*alpha/255.0f*(mode==3?0:mode==4?127/255.0f:1);
                for(channel=0;channel<3;++channel)expected[channel]+=water_light[i][channel]*amount;
            }
        }
        if(mode==2) {
            grFogMode(GR_FOG_DISABLE);local_color();grDepthBiasLevel(0);grDepthMask(FXTRUE);
            grAlphaTestFunction(GR_CMP_ALWAYS);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
            control_rect(0,0,200,255,15000,0);
        }
        er=(int)clamp(expected[0],0,255);eg=(int)clamp(expected[1],0,255);eb=(int)clamp(expected[2],0,255);
        if(mode==1){er=eb=0;eg=200;}if(mode==2){er=eg=0;eb=200;}
        if(!control_sample(log,mode==0?"ripple adds texture-modulated highlight":mode==1?"foreground rejects ripple":mode==2?"ripple preserves scene depth":mode==3?"full fog removes ripple light":mode==4?"half fog attenuates ripple":mode==5?"zero ripple alpha":"two independent ripple textures",er,eg,eb))ok=0;
    }
    fog_enabled=1;fog_update();grFogMode(GR_FOG_DISABLE);grDepthBiasLevel(0);return ok;
}
static int water_physics_tests(FILE *log)
{
    int i,ok=1;float before,peak=0;WindLeaf *l=&wind_leaves[0];
    foliage_init();water_init();leaf_count=1;wind_enabled=0;
    l->p=vec(-5,1,-27.5f);l->v=vec(0,-1,0);l->floating=l->resting=0;
    for(i=0;i<180;++i)foliage_step(1/60.0f);
    if(!l->floating || l->p.y<WATER_Y+.025f || l->p.y>WATER_Y+.05f)ok=0;
    wind_enabled=1;before=l->p.x;for(i=0;i<180;++i)foliage_step(1/60.0f);
    if(!l->floating || l->p.x<before+.05f || !water_contains(l->p.x,l->p.z))ok=0;
    if(log)fprintf(log,"Falling leaf catches water, stays above it and drifts: %s\n",ok?"PASS":"FAIL");
    l->p=vec(-9,1,-27);l->v=vec(0,-1,0);l->floating=l->resting=0;wind_enabled=0;
    for(i=0;i<180;++i)foliage_step(1/60.0f);
    if(l->floating || !l->resting || fabs(l->p.y-foliage_ground(l->p.x,l->p.z))>.001f)ok=0;
    if(log)fprintf(log,"Dry-bank leaves retain ordinary ground settling: %s\n",ok?"PASS":"FAIL");
    l->p=vec(-3.6f,WATER_Y+.035f,-27);l->v=vec(2,0,0);l->floating=l->resting=1;
    for(i=0;i<30;++i)foliage_step(1/60.0f);
    if(l->floating || !l->resting || water_contains(l->p.x,l->p.z) || fabs(l->p.y-foliage_ground(l->p.x,l->p.z))>.001f)ok=0;
    if(log)fprintf(log,"Floating leaf strands on bank without sinking: %s\n",ok?"PASS":"FAIL");
    leaf_count=0;water_energy=.12f;wind_enabled=1;foliage_manual_gust=foliage_time;
    for(i=0;i<360;++i){foliage_step(1/60.0f);if(water_energy>peak)peak=water_energy;}
    wind_enabled=0;for(i=0;i<600;++i)foliage_step(1/60.0f);
    if(peak<.4f || water_energy>.14f)ok=0;
    if(log)fprintf(log,"Water roughens with gust and eases calm: peak %.3f calm %.3f; %s\n",peak,water_energy,ok?"PASS":"FAIL");
    for(i=0;i<20;++i)foliage_key('.');if(leaf_count!=128)ok=0;
    for(i=0;i<20;++i)foliage_key(',');if(leaf_count!=0)ok=0;
    for(i=0;i<20;++i)water_key('-');if(water_strength!=0)ok=0;
    for(i=0;i<20;++i)water_key('+');if(water_strength!=1)ok=0;
    if(log)fprintf(log,"Leaf volume and ripple strength clamp at bounds: %s\n",ok?"PASS":"FAIL");
    foliage_init();water_init();leaf_count=64;wind_enabled=1;water_strength=.8f;return ok;
}
static int water_scene_tests(FILE *log)
{
    unsigned short *still,*ripple,*zero,*next;int i,ok=1;unsigned long changed=0,outside=0,repeat=0;
    foliage_benchmark=1;foliage_enabled=leaves_enabled=0;firstperson_mode=0;flying=0;transition=1;
    current_camera.eye=vec(-1,2.8f,-23);current_camera.look=vec(-6,WATER_Y,-28);transition_from=current_camera;
    water_energy=.7f;foliage_time=2;water_ripples=0;render(0,1);still=capture("STILL.PPM");
    water_ripples=1;render(0,1);ripple=capture("RIPPLES.PPM");
    if(still && ripple)for(i=88*640;i<432*640;++i)if(still[i]!=ripple[i]) {
        ++changed;if(!reflection_pixel_inside(i%640+.5f,i/640+.5f,2))++outside;
    }
    if(!still || !ripple || changed<200 || outside || water_layers!=2)ok=0;
    if(log)fprintf(log,"Ripple surface: %lu changed, %lu outside footprint, %d layers; %s\n",changed,outside,water_layers,ok?"PASS":"FAIL");
    water_strength=0;render(0,1);zero=capture("ZERO.PPM");
    if(still && zero)for(i=88*640;i<432*640;++i)if(still[i]!=zero[i])++repeat;
    if(!zero || repeat)ok=0;
    if(log)fprintf(log,"Zero strength matches still water exactly: %lu errors; %s\n",repeat,ok?"PASS":"FAIL");
    free(zero);water_strength=.8f;render(0,1);zero=capture("REPEAT.PPM");repeat=foliage_image_changes(ripple,zero);
    if(!zero || repeat)ok=0;
    if(log)fprintf(log,"Fixed camera/time has no triangle shimmer: %lu errors; %s\n",repeat,ok?"PASS":"FAIL");
    for(i=0;i<84;++i)water_tick(1/60.0f);water_energy=.7f;
    foliage_time=3.4f;render(0,1);next=capture("SCROLL.PPM");changed=foliage_image_changes(ripple,next);
    if(!next || changed<200)ok=0;
    if(log)fprintf(log,"UV scrolling animates static textures: %lu changed pixels; %s\n",changed,ok?"PASS":"FAIL");
    free(still);free(ripple);free(zero);free(next);
    foliage_enabled=leaves_enabled=1;foliage_init();water_init();render(0,1);next=capture("FLOAT.PPM");if(!next || water_floating<8)ok=0;free(next);
    if(log)fprintf(log,"Floating leaves in native capture: %d; %s\n",water_floating,ok?"PASS":"FAIL");
    current_camera.eye=vec(-6,-.1f,-28);current_camera.look=vec(-6,.2f,-30);render(0,1);
    if(water_count || water_layers)ok=0;
    if(log)fprintf(log,"Below-water camera safely rejects puddle: %s\n",ok?"PASS":"FAIL");
    foliage_benchmark=0;return ok && !finite_error;
}
static int water_reflection_checks(FILE *log)
{
    int pose,i,ok=1,grass=0;unsigned long slab_changes=0,plant_changes=0,outside=0,cull_errors=0;
    foliage_benchmark=1;water_ripples=0;reflections_enabled=1;foliage_enabled=1;leaves_enabled=0;
    firstperson_mode=0;flying=0;foliage_time=2;
    for(pose=0;pose<5;++pose) {
        unsigned short *a,*b;
        current_camera.eye=pose==0?vec(-1,2.2f,-23):pose==1?vec(-3,1.7f,-24):pose==2?vec(-3,2,-33):pose==3?vec(0,1.9f,-9):vec(-8,1.7f,-26);
        current_camera.look=vec(-6,.02f,-28.5f);
        reflect_low_enabled=0;render(0,1);a=capture("LOWOFF.PPM");
        reflect_low_enabled=1;render(0,1);b=capture(pose==0?"SLABS.PPM":"SLABALT.PPM");
        if(!a || !b)ok=0;
        else for(i=88*640;i<432*640;++i)if(a[i]!=b[i]) {
            ++slab_changes;if(!reflection_pixel_inside(i%640+.5f,i/640+.5f,2))++outside;
        }
        free(a);free(b);
        reflect_vegetation_enabled=0;render(0,1);a=capture("PLANTOFF.PPM");
        reflect_vegetation_enabled=1;render(0,1);b=capture(pose==0?"PLANTS.PPM":pose==4?"REVIEW.PPM":"PLANTALT.PPM");grass+=reflect_grass_polygons;
        if(!a || !b)ok=0;
        else for(i=88*640;i<432*640;++i)if(a[i]!=b[i]) {
            ++plant_changes;if(!reflection_pixel_inside(i%640+.5f,i/640+.5f,2))++outside;
        }
        free(a);
        reflect_plant_culling=0;render(0,1);a=capture("PLANTREF.PPM");reflect_plant_culling=1;
        if(!a || !b)ok=0;else cull_errors+=foliage_image_changes(a,b);
        free(a);free(b);
    }
    if(cull_errors)ok=0;
    if(log)fprintf(log,"Mirrored plant culling matches unculled image: %lu errors; %s\n",cull_errors,ok?"PASS":"FAIL");
    if(slab_changes<10 || plant_changes<80 || !grass || outside)ok=0;
    if(log)fprintf(log,"Reflection completeness: %lu slab pixels, %lu vegetation pixels, %d grass polygons, %lu outside puddle; %s\n",slab_changes,plant_changes,grass,outside,ok?"PASS":"FAIL");
    foliage_benchmark=0;water_ripples=1;leaves_enabled=1;return ok;
}
static int water_tests(FILE *log,int controls_only)
{
    int ok=water_controls(log);if(!foliage_controls(log))ok=0;
    if(controls_only)return ok;
    if(!foliage_grass_anchor_checks(log))ok=0;
    if(!firstperson_tests(log))ok=0;
    if(!water_physics_tests(log))ok=0;
    if(!water_scene_tests(log))ok=0;
    if(!water_reflection_checks(log))ok=0;
    return ok;
}
