static void present_control(void)
{grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();}
static int probe(FILE *log,int enabled,int paint,int spec,int constant)
{
    GrVertex q[4];unsigned short pixel=0;int i,rgb[3],ok=1,expected=132+(enabled && paint?constant:spec);
    expected=expected>255?255:expected;grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0,0,0);
    paint_state(enabled,0);grTexSource(GR_TMU0,probe_address,GR_MIPMAPLEVELMASK_BOTH,&probe_texture);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
    grConstantColorValue(0xff000000UL|(FxU32)constant|((FxU32)constant<<8)|((FxU32)constant<<16));memset(q,0,sizeof(q));
    for(i=0;i<4;++i){q[i].x=(i==1||i==2)?400:240;q[i].y=i<2?160:320;q[i].ooz=30000;q[i].oow=q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=paint?192:64;q[i].tmuvtx[0].tow=64;q[i].r=q[i].g=q[i].b=spec;q[i].a=255;}
    grDrawPolygonVertexList(4,q);present_control();if(!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,320,240,1,1,2,&pixel))ok=0;
    rgb[0]=((pixel>>11)&31)*255/31;rgb[1]=((pixel>>5)&63)*255/63;rgb[2]=(pixel&31)*255/31;
    for(i=0;i<3;++i)if(abs(rgb[i]-expected)>9)ok=0;
    if(log)fprintf(log,"Selector %d paint %d spec %d constant %d: RGB %d,%d,%d expected %d: %s\n",enabled,paint,spec,constant,rgb[0],rgb[1],rgb[2],expected,ok?"PASS":"FAIL");return ok;
}
static unsigned long changes(const unsigned short *a,const unsigned short *b)
{
    unsigned long n=0;int i;if(!a || !b)return 0;
    for(i=120*640;i<390*640;++i)if(a[i]!=b[i])++n;return n;
}
static int tests(FILE *log)
{
    int ok=1,enabled,paint,level,i,gloss,calls;unsigned short *base,*masked,*unmasked,*mask,*a,*b;
    unsigned long paint_pixels=0,paint_changed=0,unmasked_paint=0,bare_changed=0,mismatch=0,rounding=0;
    grDitherMode(GR_DITHER_DISABLE);
    for(enabled=0;enabled<2;++enabled)for(paint=0;paint<2;++paint)for(level=0;level<3;++level)
        if(!probe(log,enabled,paint,level==0?0:level==1?96:224,0))ok=0;
    if(!probe(log,1,1,0,64) || !probe(log,1,0,96,64) || !probe(log,0,1,96,0))ok=0;
    for(gloss=0;gloss<4;++gloss) {
        gloss_index=gloss;strength=1.2f;prepare_panel();
        for(i=0;i<NV;++i){float ref=lit[i].ndotl>0?clamp(strength*(float)pow(lit[i].ndoth,gloss_values[gloss]),0,1):0;if(fabs(lit[i].r-245*ref)>.02f)ok=0;}
    }
    if(log)fprintf(log,"Repeated-squaring specular matches pow reference at all %d vertices and four gloss levels: %s\n",NV,ok?"PASS":"FAIL");
    grDitherMode(GR_DITHER_4x4);split=0;point_sampling=1;gloss_index=2;strength=0;light_time=phase=0;
    mask_enabled=1;draw_scene(60,0);base=capture("BASE.PPM");calls=draw_calls;
    strength=1;draw_scene(60,0);masked=capture("PAINT.PPM");if(draw_calls!=calls)ok=0;
    mask_enabled=0;draw_scene(60,0);unmasked=capture("UNMASKED.PPM");if(draw_calls!=calls)ok=0;
    mask_enabled=1;mask_view=1;draw_scene(60,0);mask=capture("MASK.PPM");mask_view=0;
    if(!base || !masked || !unmasked || !mask)ok=0;
    else for(i=120*640;i<390*640;++i) {
        if(mask[i]==0xffff){++paint_pixels;if(masked[i]!=base[i])++paint_changed;if(unmasked[i]!=base[i])++unmasked_paint;}
        else if(mask[i]==0 && masked[i]!=base[i])++bare_changed;
    }
    if(paint_pixels<1000 || paint_changed || unmasked_paint<200 || bare_changed<200)ok=0;
    if(log)fprintf(log,"Paint mask: %lu paint pixels, %lu changed under masked highlight, %lu change with mask off; %lu bare-metal highlight pixels; %d draws on/off: %s\n",paint_pixels,paint_changed,unmasked_paint,bare_changed,calls,ok?"PASS":"FAIL");
    free(base);free(masked);free(unmasked);free(mask);
    split=1;point_sampling=0;swapped=0;draw_scene(60,1);a=capture("SPLIT.PPM");swapped=1;draw_scene(60,1);b=capture("SWAP.PPM");
    if(!a || !b)ok=0;
    else for(i=120*640;i<390*640;++i) {
        int x=i%640;if(x>0 && x<319 && a[i]!=b[i+320]) {
            unsigned short p=a[i],q=b[i+320];
            if(abs((int)((p>>11)&31)-(int)((q>>11)&31))<=1 && abs((int)((p>>5)&63)-(int)((q>>5)&63))<=1 && abs((int)(p&31)-(int)(q&31))<=1)++rounding;
            else ++mismatch;
        }
    }
    if(mismatch || rounding>16)ok=0;
    if(log)fprintf(log,"Swapped viewport interiors: %lu larger differences, %lu one-level RGB565 roundings (limit 16): %s\n",mismatch,rounding,mismatch || rounding>16?"FAIL":"PASS");free(a);free(b);swapped=0;
    draw_scene(60,0);a=capture(NULL);draw_scene(60,0);b=capture(NULL);if(!a || !b || changes(a,b))ok=0;free(b);
    light_time=2;draw_scene(60,0);b=capture(NULL);if(!a || !b || changes(a,b)<200)ok=0;free(a);free(b);
    if(log)fprintf(log,"Moving light, repeatable fixed pose, one-pass masked addition and finite projection: %s\n",ok?"PASS":"FAIL");
    light_time=0;return ok && !finite_error;
}
static int benchmark(FILE *log)
{
    int i,slow=0,max_draws=0;float sum=0,max_ms=0;FILE *mark;
    mark=fopen("START.MRK","w");if(!mark)return 0;fclose(mark);draw_scene(60,1);draw_scene(60,1);profile_reset();
    for(i=0;i<600;++i) {
        float ms;phase=i/60.0f;light_time=phase;orbiting=1;split=(i/100)%2==0;mask_enabled=(i/75)%2;mask_view=i>=450 && i<500;
        gloss_index=(i/150)%4;specular_enabled=i%100<85;strength=(i/100)%2?1.5f:1;
        if(i==300){profile_toggle();profiler_dirty=1;}
        draw_scene(60,1);ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;
        if(ms>max_ms)max_ms=ms;if(ms>18)++slow;if(draw_calls>max_draws)max_draws=draw_calls;
    }
    mark=fopen("END.MRK","w");if(!mark)return 0;fclose(mark);
    if(log)fprintf(log,"600 moving light/orbit frames, mask on/off/preview, gloss and strength, split/single, profiler for second half: %.2f guest FPS, max %.3f ms, %d over 18 ms, max %d cell draws\n",600000/sum,max_ms,slow,max_draws);
    return profiler.available && !slow && !finite_error;
}
