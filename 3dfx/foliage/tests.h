static void foliage_probe(float depth,int solid)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i) {
        q[i].x=(i==1||i==2)?380:260;q[i].y=i<2?180:280;q[i].r=q[i].g=q[i].b=255;
        q[i].a=foliage_mode==1?205:255;q[i].ooz=depth;q[i].oow=q[i].tmuvtx[0].oow=1;
        q[i].tmuvtx[0].sow=q[i].tmuvtx[0].tow=solid?128:0;
    }
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static int foliage_controls(FILE *log)
{
    int mode,ok=1;GrFog_t table[GR_FOG_TABLE_SIZE];
    for(mode=0;mode<7;++mode) {
        int i,er=65,eg=131,eb=32;foliage_mode=mode==5?1:mode==6?2:0;fog_enabled=mode==4;
        for(i=0;i<GR_FOG_TABLE_SIZE;++i)table[i]=255;
        control_clear();grFogMode(GR_FOG_DISABLE);grDepthMask(FXTRUE);
        grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        control_rect(0,mode==2?200:0,mode==2?0:200,255,mode==2?40000:10000,0);
        grFogTable(table);foliage_state(0);grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
        foliage_probe(30000,mode!=0 && mode!=6);
        if(mode==0 || mode==1) {
            grFogMode(GR_FOG_DISABLE);local_color();grAlphaTestFunction(GR_CMP_ALWAYS);
            grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);grDepthMask(FXTRUE);
            control_rect(200,0,0,255,20000,0);
        }
        if(mode==0){er=200;eg=eb=0;}
        if(mode==2){er=eb=0;eg=200;}
        if(mode==4){er=52;eg=65;eb=78;}
        if(mode==5){er=52;eg=105;eb=65;}
        if(mode==6){er=eg=eb=0;}
        if(!control_sample(log,mode==0?"cutout hole preserves color and depth":mode==1?"solid leaf writes depth":mode==2?"foreground occludes foliage":mode==3?"ARGB1555 textured leaf":mode==4?"full fog on alpha cutout":mode==5?"sorted blend comparator":"solid card exposes transparent texels",er,eg,eb))ok=0;
    }
    foliage_mode=0;fog_enabled=1;fog_update();grFogMode(GR_FOG_DISABLE);return ok;
}
static unsigned long foliage_image_changes(unsigned short *a,unsigned short *b)
{
    unsigned long count=0;int i;
    if(!a || !b)return 0;
    for(i=88*640;i<432*640;++i)if(a[i]!=b[i])++count;
    return count;
}
static int foliage_tests(FILE *log,int controls_only)
{
    int ok=foliage_controls(log),i,rest=0,lifted=0;unsigned short *a,*b;unsigned long changes;
    if(controls_only)return ok;
    if(!firstperson_tests(log))ok=0;
    /* Fixed-step settle, then a traveling gust must lift leaves again. */
    foliage_init();
    {
        float max_error=0;
        for(i=-8192;i<=8192;++i) {
            float angle=i*.037f,error=fabs(foliage_sin(angle)-(float)sin(angle));
            if(error>max_error)max_error=error;
        }
        if(max_error>.00006f)ok=0;
        if(log)fprintf(log,"Wind LUT versus full sine: max error %.7f; %s\n",max_error,ok?"PASS":"FAIL");
    }
    leaf_count=LEAF_MAX;wind_enabled=0;
    for(i=0;i<60*24;++i)foliage_step(1/60.0f);
    for(i=0;i<leaf_count;++i)if(wind_leaves[i].resting && fabs(wind_leaves[i].p.y-foliage_ground(wind_leaves[i].p.x,wind_leaves[i].p.z))<.001f)++rest;
    if(rest!=leaf_count)ok=0;
    wind_enabled=1;foliage_manual_gust=foliage_time;
    for(i=0;i<300;++i){foliage_step(1/60.0f);if(foliage_airborne>lifted)lifted=foliage_airborne;}
    if(lifted<20)ok=0;
    if(log)fprintf(log,"Leaf physics: %d/%d settle exactly on receiver; gust lifts %d; %s\n",rest,leaf_count,lifted,ok?"PASS":"FAIL");
    foliage_time=2;foliage_manual_gust=-100;
    if(foliage_gust(-22,-30)<foliage_gust(22,-30)+.3f)ok=0;
    if(log)fprintf(log,"Gust wave reaches west trees before east trees: %s\n",ok?"PASS":"FAIL");
    for(i=0;i<20;++i)foliage_key(',');if(leaf_count!=0)ok=0;
    for(i=0;i<20;++i)foliage_key('.');if(leaf_count!=LEAF_MAX)ok=0;
    if(log)fprintf(log,"Leaf volume clamps at zero and %d: %s\n",LEAF_MAX,ok?"PASS":"FAIL");
    foliage_init();foliage_benchmark=1;leaves_enabled=0;firstperson_mode=0;flying=0;transition=1;
    current_camera.eye=vec(-2,2.1f,-25);current_camera.look=vec(-18,4.8f,-40);transition_from=current_camera;
    foliage_enabled=0;render(0,1);a=capture("BARE.PPM");
    foliage_enabled=1;render(0,1);b=capture("CUTOUT.PPM");changes=foliage_image_changes(a,b);free(a);free(b);
    if(changes<1500)ok=0;
    if(log)fprintf(log,"Native tree/grass cutouts: %lu changed scene pixels, %d cards; %s\n",changes,foliage_card_count,ok?"PASS":"FAIL");
    wind_enabled=0;render(0,1);a=capture("CALM.PPM");wind_enabled=1;render(0,1);b=capture("GUST.PPM");changes=foliage_image_changes(a,b);free(a);free(b);
    if(changes<100)ok=0;
    if(log)fprintf(log,"Traveling gust bends crowns and grass: %lu changed pixels; %s\n",changes,ok?"PASS":"FAIL");
    foliage_mode=1;render(0,1);a=capture("BLENDED.PPM");foliage_mode=2;render(0,1);b=capture("SOLID.PPM");changes=foliage_image_changes(a,b);free(a);free(b);
    if(changes<1500)ok=0;
    if(log)fprintf(log,"Blend/opaque card comparison: %lu changed pixels; %s\n",changes,ok?"PASS":"FAIL");
    foliage_mode=0;current_camera.eye=vec(0,1.7f,-35);current_camera.look=vec(-4,.1f,-40);
    leaves_enabled=0;render(0,1);a=capture("NOLEAVES.PPM");
    leaves_enabled=1;for(i=0;i<leaf_count;++i){wind_leaves[i].p=vec(-6+(i%16)*.65f,1.0f+(i/16)*.4f,-37-(i/16));wind_leaves[i].resting=0;}
    render(0,1);b=capture("LEAVES.PPM");changes=foliage_image_changes(a,b);free(a);free(b);
    if(changes<100)ok=0;
    if(log)fprintf(log,"Loose leaves visible independently of foliage: %lu changed pixels; %s\n",changes,ok?"PASS":"FAIL");
    foliage_init();leaf_count=64;foliage_benchmark=0;return ok && !finite_error;
}
