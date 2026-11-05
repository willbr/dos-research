static unsigned long changes(const unsigned short *a,const unsigned short *b,int x,int y,int w,int h)
{
    int row,col;unsigned long count=0;if(!a || !b)return 0;
    for(row=y;row<y+h;++row)for(col=x;col<x+w;++col)if(a[row*640+col]!=b[row*640+col])++count;
    return count;
}
static void present_control(void)
{
    grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();
}
static int analytic_control(FILE *log)
{
    int filter,ok=1;unsigned short result[2];
    for(filter=0;filter<2;++filter) {
        GrVertex q[4];int i;memset(q,0,sizeof(q));
        grClipWindow(0,0,640,480);grBufferClear(0,0,0);sampler(0,filter,filter);
        grTexSource(GR_TMU0,probe_address,GR_MIPMAPLEVELMASK_BOTH,&probe_texture);
        grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
        for(i=0;i<4;++i){q[i].x=(i==1||i==2)?400:240;q[i].y=i<2?160:320;q[i].oow=q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=q[i].tmuvtx[0].tow=128;q[i].r=q[i].g=q[i].b=q[i].a=255;}
        grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);present_control();
        if(!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,320,240,1,1,2,&result[filter]))ok=0;
    }
    if(result[0]!=0xf800 && result[0]!=0x07e0 && result[0]!=0x001f && result[0]!=0xffff)ok=0;
    {int r=((result[1]>>11)&31)*255/31,g=((result[1]>>5)&63)*255/63,b=(result[1]&31)*255/31;
     if(abs(r-128)>12 || abs(g-128)>12 || abs(b-128)>12)ok=0;
     if(log)fprintf(log,"2x2 midpoint: nearest %04x is one source texel; bilinear RGB %d,%d,%d expected 128,128,128: %s\n",result[0],r,g,b,ok?"PASS":"FAIL");}
    return ok;
}
static unsigned short *derivative_control(int min_filter,int mag_filter,float extent)
{
    grClipWindow(0,0,640,480);grBufferClear(0,0,0);sampler(0,min_filter,mag_filter);
    sample_quad(120,120,400,240,extent,41.25f,75.25f,0);present_control();return capture(NULL);
}
static int tests(FILE *log)
{
    int ok=1,mode,i;unsigned short *point,*minonly,*magonly,*both,*a,*b;
    unsigned long cmin,cmag,cboth;
    grDitherMode(GR_DITHER_DISABLE);
    if(!analytic_control(log))ok=0;
    for(mode=0;mode<2;++mode) {
        float extent=mode?1024:32;int pass,known;
        point=derivative_control(0,0,extent);minonly=derivative_control(1,0,extent);
        magonly=derivative_control(0,1,extent);both=derivative_control(1,1,extent);
        cmin=changes(point,minonly,170,160,300,160);cmag=changes(point,magonly,170,160,300,160);cboth=changes(point,both,170,160,300,160);
        pass=point && minonly && magonly && both && (mode?(cmin>=1000 && !cmag && cboth==cmin):(cmag>=1000 && !cmin && cboth==cmag));
        known=software_check && mode && point && minonly && magonly && both && !cmin && cmag>=1000 && cboth==cmag;
        if(!pass && !known)ok=0;
        if(log)fprintf(log,"%s: min-only %lu, mag-only %lu, both %lu changed pixels; independent sampler selection: %s\n",mode?"Minification":"Magnification",cmin,cmag,cboth,pass?"PASS":known?"LIMIT":"FAIL");
        if(known && log)fprintf(log,"LIMIT: software backend selects magnification state in this single-level minification fixture; ordinary nearest/bilinear comparison still changes %lu pixels.\n",cboth);
        free(point);free(minonly);free(magonly);free(both);
    }
    grDitherMode(GR_DITHER_4x4);phase=0;span=32;split=1;swapped=oblique=0;
    draw_scene(60,1);a=capture("SPLIT.PPM");swapped=1;draw_scene(60,1);b=capture("SWAP.PPM");
    if(!a || !b)ok=0;
    else {
        unsigned long mismatch=0;
        for(i=132*640;i<410*640;++i){int x=i%640;if(x>=20 && x<300 && a[i]!=b[i+320])++mismatch;}
        if(mismatch)ok=0;
        if(log)fprintf(log,"Swap comparison: %lu mismatched scene pixels across columns: %s\n",mismatch,mismatch?"FAIL":"PASS");
    }
    free(a);free(b);swapped=0;
    draw_scene(60,0);a=capture(NULL);draw_scene(60,0);b=capture(NULL);
    if(!a || !b || changes(a,b,12,94,616,327))ok=0;free(b);
    phase=1.4f;draw_scene(60,0);b=capture(NULL);i=(int)changes(a,b,12,132,616,278);
    if(!a || !b || i<1000)ok=0;
    if(log)fprintf(log,"Fixed geometry UV motion: %d changed pixels; paused frame repeatable: %s\n",i,ok?"PASS":"FAIL");free(a);free(b);
    span=1024;draw_scene(60,1);a=capture("MINIFY.PPM");if(!a)ok=0;free(a);
    span=32;oblique=1;draw_scene(60,1);a=capture("OBLIQUE.PPM");if(!a)ok=0;free(a);
    span=32;oblique=0;phase=0;return ok;
}
static int benchmark(FILE *log)
{
    int i,slow=0;float max_ms=0,sum=0;FILE *mark;
    mark=fopen("START.MRK","w");if(!mark)return 0;fclose(mark);
    draw_scene(60,1);draw_scene(60,1);profile_reset();
    for(i=0;i<600;++i) {
        int mode=i/75;float ms;
        split=mode<4;bilinear=mode&1;oblique=mode==2 || mode==3;
        span=(mode==1 || mode==3 || mode>=6)?1024:32;
        phase=i/60.0f;
        if(i==300){profile_toggle();profiler_dirty=1;}
        draw_scene(60,1);ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;
        if(ms>max_ms)max_ms=ms;if(ms>18)++slow;
    }
    mark=fopen("END.MRK","w");if(!mark)return 0;fclose(mark);
    if(log)fprintf(log,"600 moving frames, eight comparison cases, profiler for second half: %.2f guest FPS, max %.3f ms, %d over 18 ms\n",600000/sum,max_ms,slow);
    return profiler.available && !slow;
}
