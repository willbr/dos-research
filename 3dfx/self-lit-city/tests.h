static void present_control(void)
{grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();}
static int probe(FILE *log,int enabled,int masked,int light,int constant,int *rgb)
{
    GrVertex q[4];unsigned short pixel=0;int i,ok=1;
    grAlphaControlsITRGBLighting(FXFALSE);grClipWindow(0,0,640,480);grDepthMask(FXTRUE);grBufferClear(0,0,0);
    city_state(enabled,0);grDepthBufferMode(GR_DEPTHBUFFER_DISABLE);
    grTexSource(GR_TMU0,probe_address,GR_MIPMAPLEVELMASK_BOTH,&probe_texture);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
    grConstantColorValue(0xff000000UL|(unsigned long)constant|((unsigned long)constant<<8)|((unsigned long)constant<<16));
    memset(q,0,sizeof(q));
    for(i=0;i<4;++i){q[i].x=(i==1||i==2)?400:240;q[i].y=i<2?160:320;q[i].oow=q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=masked?192:64;q[i].tmuvtx[0].tow=64;q[i].r=q[i].g=q[i].b=light;q[i].a=255;}
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);present_control();
    if(!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,320,240,1,1,2,&pixel))ok=0;
    rgb[0]=((pixel>>11)&31)*255/31;rgb[1]=((pixel>>5)&63)*255/63;rgb[2]=(pixel&31)*255/31;
    for(i=0;i<3;++i) {
        int expected=132*(enabled && masked?constant:light)/255;
        if(abs(rgb[i]-expected)>9)ok=0;
    }
    if(log)fprintf(log,"Mask %d, alpha bit %d, vertex %d, constant %d: RGB %d,%d,%d: %s\n",enabled,masked,light,constant,rgb[0],rgb[1],rgb[2],ok?"PASS":"FAIL");
    return ok;
}
static unsigned long changes(const unsigned short *a,const unsigned short *b)
{
    unsigned long n=0;int i;if(!a || !b)return 0;
    for(i=112*640;i<432*640;++i)if(a[i]!=b[i])++n;return n;
}
static int tests(FILE *log)
{
    int ok=1,enabled,masked,level,rgb[3],i,calls;unsigned short *a,*b;unsigned long changed,mismatch=0;
    grDitherMode(GR_DITHER_DISABLE);
    for(enabled=0;enabled<2;++enabled)for(masked=0;masked<2;++masked)for(level=0;level<3;++level)
        if(!probe(log,enabled,masked,level==0?0:level==1?96:255,255,rgb))ok=0;
    if(!probe(log,1,1,0,64,rgb))ok=0;
    if(!probe(log,1,0,96,64,rgb))ok=0;
    /* Explicitly disabling the selector must restore ordinary lighting. */
    if(!probe(log,0,1,0,255,rgb))ok=0;
    grDitherMode(GR_DITHER_4x4);split=0;mask_enabled=0;mask_view=0;light_strength=.25f;
    draw_scene(60,0);a=capture("OFF.PPM");calls=frame_calls;mask_enabled=1;
    draw_scene(60,0);b=capture("ON.PPM");changed=changes(a,b);
    if(!a || !b || changed<200 || frame_calls!=calls || !calls)ok=0;
    if(log)fprintf(log,"Self-lit comparison: %lu changed scene pixels; same %d face draws with mask on/off (one pass): %s\n",changed,calls,ok?"PASS":"FAIL");free(a);free(b);
    split=1;swapped=0;light_strength=.7f;draw_scene(60,1);a=capture("SPLIT.PPM");
    swapped=1;draw_scene(60,1);b=capture("SWAP.PPM");
    if(!a || !b)ok=0;
    /* Exclude the outermost clip-window pixel on each side; software Voodoo
       differs in its right-edge coverage there. Compare all interior pixels. */
    else for(i=112*640;i<432*640;++i){int x=i%640;if(x>0 && x<319 && a[i]!=b[i+320])++mismatch;}
    if(mismatch)ok=0;if(log)fprintf(log,"Swap comparison: %lu mismatched scene pixels: %s\n",mismatch,mismatch?"FAIL":"PASS");free(a);free(b);
    swapped=0;mask_view=1;draw_scene(60,1);a=capture("MASK.PPM");if(!a)ok=0;free(a);mask_view=0;
    draw_scene(60,0);a=capture(NULL);draw_scene(60,0);b=capture(NULL);
    if(!a || !b || changes(a,b))ok=0;free(b);
    scene_time=3;draw_scene(60,0);b=capture(NULL);changed=changes(a,b);if(!b || changed<200)ok=0;
    if(log)fprintf(log,"Moving CPU light: %lu changed pixels, repeatable fixed pose; %s\n",changed,ok?"PASS":"FAIL");free(a);free(b);
    scene_time=0;return ok && !finite_error;
}
static int benchmark(FILE *log)
{
    int i,slow=0,max_calls=0;float sum=0,max_ms=0;FILE *mark;
    mark=fopen("START.MRK","w");if(!mark)return 0;fclose(mark);
    draw_scene(60,1);draw_scene(60,1);profile_reset();
    for(i=0;i<600;++i) {
        float ms;scene_time=i/60.0f;camera_yaw=.65f+i*.006f;
        split=(i/100)%2==0;mask_enabled=(i/75)%2;mask_view=i>=450 && i<525;
        if(i==300){profile_toggle();profiler_dirty=1;}
        draw_scene(60,1);ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;
        if(ms>max_ms)max_ms=ms;if(ms>18)++slow;if(frame_calls>max_calls)max_calls=frame_calls;
    }
    mark=fopen("END.MRK","w");if(!mark)return 0;fclose(mark);
    if(log)fprintf(log,"600 moving orbit/light frames, split/single, mask on/off/preview, profiler for second half: %.2f guest FPS, max %.3f ms, %d over 18 ms, max %d face draws\n",600000/sum,max_ms,slow,max_calls);
    return profiler.available && !slow && !finite_error;
}
