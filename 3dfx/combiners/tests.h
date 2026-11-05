static void present_control(void)
{grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();}
static int probe(FILE *log,int mode,int a_mode,int ca,int texel,int blending,int *actual)
{
    static const float texture[3]={136,68,204},lighting[3]={96,160,224},back[3]={24,40,56};
    static const int ta_values[4]={0,85,170,255};
    GrVertex q[4];unsigned short pixel=0;int i,ok=1;float expected[3],ta=ta_values[texel],a;
    alpha_mode=a_mode;constant_alpha=ca;blend_enabled=blending;tint=0;material=0;
    grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0x00382818,0,0);
    combine_state(mode);grTexSource(GR_TMU0,probe_address,GR_MIPMAPLEVELMASK_BOTH,&probe_texture);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    memset(q,0,sizeof(q));
    for(i=0;i<4;++i) {
        q[i].x=(i==1||i==2)?400:240;q[i].y=i<2?160:320;q[i].ooz=30000;q[i].oow=q[i].tmuvtx[0].oow=1;
        q[i].tmuvtx[0].sow=(texel&1)?192:64;q[i].tmuvtx[0].tow=(texel&2)?192:64;
        q[i].r=lighting[0];q[i].g=lighting[1];q[i].b=lighting[2];q[i].a=a_mode?96:255;
    }
    grDrawPolygonVertexList(4,q);present_control();
    if(!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,320,240,1,1,2,&pixel))ok=0;
    actual[0]=((pixel>>11)&31)*255/31;actual[1]=((pixel>>5)&63)*255/63;actual[2]=(pixel&31)*255/31;
    /* Manual equations, using independent known source channels. */
    a=a_mode==0?255:a_mode==1?ta:a_mode==2?ta*ca/255:a_mode==3?clamp(ta+ca,0,255):ta*96/255+(255-ta)*ca/255;
    for(i=0;i<3;++i) {
        float c=constant_colors[0][i];
        expected[i]=mode==0?texture[i]:mode==1?texture[i]*lighting[i]/255:mode==2?clamp(texture[i]+lighting[i],0,255):ca*texture[i]/255+(255-ca)*c/255;
        if(blending)expected[i]=expected[i]*a/255+back[i]*(1-a/255);
        if(fabs(actual[i]-expected[i])>10)ok=0;
    }
    if(!ok && log)fprintf(log,"FAIL RGB mode %d alpha %d CA %d TA %.0f blend %d: RGB %d,%d,%d expected %.1f,%.1f,%.1f\n",mode,a_mode,ca,ta,blending,actual[0],actual[1],actual[2],expected[0],expected[1],expected[2]);
    return ok;
}
static unsigned long changes(const unsigned short *a,const unsigned short *b)
{
    unsigned long n=0;int i;if(!a || !b)return 0;
    for(i=130*640;i<430*640;++i)if(a[i]!=b[i])++n;return n;
}
static int gallery_reference(FILE *log,const unsigned short *gallery_image)
{
    int mode;unsigned long mismatch=0;
    for(mode=0;mode<4;++mode) {
        float x=mode%2?480:160,y=mode<2?202:366;unsigned short *reference;int row,col;
        grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0,0,0);background();
        checker_panel(x-128,y-57,256,114);combine_state(mode);draw_object(x,y,180);present_control();reference=capture(NULL);
        if(!reference)return 0;
        for(row=(int)y-55;row<(int)y+55;++row)for(col=(int)x-126;col<(int)x+126;++col)
            if(gallery_image[row*640+col]!=reference[row*640+col])++mismatch;
        free(reference);
    }
    if(log)fprintf(log,"Gallery versus four freshly cleared same-viewport references: %lu differing pixels: %s\n",mismatch,mismatch?"FAIL":"PASS");
    return !mismatch;
}
static int tests(FILE *log)
{
    int ok=1,mode,a_mode,level,texel,actual[3],baseline[3],i,calls=-1,checks=0;unsigned short *a,*b;char name[16];
    grDitherMode(GR_DITHER_DISABLE);
    for(mode=0;mode<4;++mode)for(level=0;level<3;++level){if(!probe(log,mode,0,level==0?0:level==1?128:255,2,0,actual))ok=0;++checks;}
    if(log)fprintf(log,"12 RGB probes: replace, modulate, saturating add and interpolation endpoints/midpoint: %s\n",ok?"PASS":"FAIL");
    for(a_mode=0;a_mode<5;++a_mode)for(level=0;level<3;++level)for(texel=0;texel<3;++texel) {
        if(!probe(log,0,a_mode,level==0?0:level==1?128:255,texel==0?0:texel==1?2:3,1,actual))ok=0;++checks;
    }
    if(log)fprintf(log,"45 alpha probes: opaque/replace/modulate/add/interpolate through SRC_ALPHA blending: %s\n",ok?"PASS":"FAIL");
    for(a_mode=0;a_mode<5;++a_mode) {
        if(!probe(log,3,a_mode,128,2,0,actual))ok=0;++checks;
        if(!a_mode)memcpy(baseline,actual,sizeof(baseline));else for(i=0;i<3;++i)if(actual[i]!=baseline[i])ok=0;
    }
    if(log)fprintf(log,"RGB interpolation retains the same constant-alpha factor across all output-alpha modes; %d probes total: %s\n",checks,ok?"PASS":"FAIL");
    grDitherMode(GR_DITHER_4x4);gallery=1;alpha_mode=blend_enabled=0;constant_alpha=128;phase=0;vertex_alpha=.7f;
    draw_scene(60,1);a=capture("GALLERY.PPM");if(!a || !visible_count || scene_draws!=visible_count*4)ok=0;
    if(a && !gallery_reference(log,a))ok=0;
    alpha_mode=4;blend_enabled=1;draw_scene(60,1);b=capture("ALPHA.PPM");if(!b || changes(a,b)<200)ok=0;free(b);
    alpha_mode=blend_enabled=0;draw_scene(60,1);b=capture(NULL);if(!a || !b || changes(a,b))ok=0;free(a);free(b);
    gallery=0;
    for(mode=0;mode<4;++mode) {
        color_mode=mode;draw_scene(60,1);sprintf(name,"MODE%d.PPM",mode);a=capture(name);if(!a)ok=0;free(a);
        if(calls<0)calls=scene_draws;else if(scene_draws!=calls)ok=0;
    }
    draw_scene(60,0);a=capture(NULL);phase=1.5f;draw_scene(60,0);b=capture(NULL);if(!a || !b || changes(a,b)<200)ok=0;free(a);free(b);
    if(log)fprintf(log,"Scene: four identical-geometry modes (%d visible cells per object), visible alpha blending, state restoration, motion and finite projection: %s\n",calls,ok?"PASS":"FAIL");
    gallery=1;color_mode=alpha_mode=blend_enabled=material=tint=0;constant_alpha=128;phase=0;return ok && !finite_error;
}
static int benchmark(FILE *log)
{
    int i,slow=0,max_draws=0;float sum=0,max_ms=0;FILE *mark;
    mark=fopen("START.MRK","w");if(!mark)return 0;fclose(mark);draw_scene(60,1);draw_scene(60,1);profile_reset();
    for(i=0;i<600;++i) {
        float ms;phase=i/60.0f;gallery=(i/75)%2==0;color_mode=(i/50)%4;alpha_mode=(i/120)%5;blend_enabled=(i/60)%2;
        constant_alpha=(i/100)%3==0?0:(i/100)%3==1?128:255;tint=(i/100)%3;material=(i/150)%2;
        if(i==300){profile_toggle();profiler_dirty=1;}
        draw_scene(60,1);ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;
        if(ms>max_ms)max_ms=ms;if(ms>18)++slow;if(scene_draws>max_draws)max_draws=scene_draws;
    }
    mark=fopen("END.MRK","w");if(!mark)return 0;fclose(mark);
    if(log)fprintf(log,"600 moving frames, gallery/single, RGB/alpha equations, blending, constants and textures, profiler for second half: %.2f guest FPS, max %.3f ms, %d over 18 ms, max %d cell draws\n",600000/sum,max_ms,slow,max_draws);
    return profiler.available && !slow && !finite_error;
}
