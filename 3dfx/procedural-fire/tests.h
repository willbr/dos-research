static void present_control(void)
{grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();}
static int close_rgb(unsigned short actual,unsigned short expected)
{
    int r=((actual>>11)&31)-((expected>>11)&31),g=((actual>>5)&63)-((expected>>5)&63),b=(actual&31)-(expected&31);
    return abs(r)<=1 && abs(g)<=1 && abs(b)<=1;
}
static int hot_bound_valid(void)
{
    FireState *s=active_state();unsigned char *heat=s->heat[s->front];int y,x;
    for(y=0;y<FH;++y)for(x=0;x<FW;++x)if(heat[y*FW+x])return y==s->top && y>=s->first;
    return 0;
}
static int verify_textures(FILE *log,const char *label)
{
    unsigned short *p;int path,x,y,bad=0,different=0;
    grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0,0,0);
    for(path=0;path<2;++path){sample_state(path);image_quad(path?384:64,160,128);}present_control();p=capture(NULL);if(!p)return 0;
    for(y=0;y<FH;++y)for(x=0;x<FW;++x) {
        unsigned short a=p[(y+160)*640+x+64],b=p[(y+160)*640+x+384],expected=pixels[y*FW+x];
        if(!close_rgb(a,expected) || !close_rgb(b,expected))++bad;if(a!=b)++different;
    }
    if(log)fprintf(log,"%s: 16384 texels, %d CPU-decode errors, %d full/row pixel differences: %s\n",label,bad,different,bad || different?"FAIL":"PASS");free(p);return !bad && !different;
}
static int tests(FILE *log)
{
    static const int ranges[6][2]={{0,0},{127,127},{7,12},{31,32},{63,64},{0,127}};
    int ok=1,i,x,y,test,mode,old_front,old_steps;unsigned long before0,before1,old_calls0,old_calls1;unsigned short *a,*b;char label[64];
    grDitherMode(GR_DITHER_DISABLE);bilinear=zoomed=0;running=0;
    if((address[0]&7) || (address[1]&7) || ((address[1]&0x1fffffUL)+LEVEL_BYTES)>0x200000UL)ok=0;
    for(y=0;y<FH;++y)for(x=0;x<FW;++x)pixels[y*FW+x]=rgb565((y*5+41)&255,(x*3+17)&255,(x+y*7+53)&255);
    whole_upload(0);whole_upload(1);memcpy(uploaded,pixels,sizeof(pixels));
    for(test=0;test<6;++test) {
        int first=ranges[test][0],last=ranges[test][1];
        for(y=first;y<=last;++y)for(x=0;x<FW;++x)pixels[y*FW+x]=rgb565((test*29+x*5+y*3)&255,(test*61+y*7+x)&255,(test*31+x+y*11)&255);
        before1=path_bytes[1];whole_upload(0);rows_upload(first,last);memcpy(uploaded,pixels,sizeof(pixels));
        if(path_bytes[1]-before1!=(unsigned long)(last-first+1)*ROW_BYTES)ok=0;
        sprintf(label,"Inclusive rows %d-%d, other rows preserved",first,last);if(!verify_textures(log,label))ok=0;
    }
    initialize_states();band_mode=0;sync_pending=1;fire_tick(0);if(!verify_textures(log,"Initial fire synchronization"))ok=0;
    for(mode=0;mode<4;++mode) {
        band_mode=mode!=0;if(band_mode)band_index=mode-1;sync_pending=1;fire_tick(0);
        for(i=0;i<24;++i) {
            before0=path_bytes[0];before1=path_bytes[1];manual_steps=1;fire_tick(0);
            if(!hot_bound_valid())ok=0;
            if(path_bytes[0]-before0!=LEVEL_BYTES)ok=0;
            if(dirty_first>=0 && path_bytes[1]-before1!=(unsigned long)(dirty_last-dirty_first+1)*ROW_BYTES)ok=0;
            if(band_mode && dirty_first>=0 && dirty_first<active_state()->first)ok=0;
            if(!band_mode && (changed_rows<2 || dirty_first>=FH-1))ok=0;
        }
        sprintf(label,"%s fire, active rows %d-127",band_mode?"Controlled-band":"Full",active_state()->first);
        if(!verify_textures(log,label))ok=0;
        if(log)fprintf(log,"  Last update: %d rows change, span %d-%d; full %lu B, partial %lu B\n",changed_rows,dirty_first,dirty_last,path_bytes[0]-before0,path_bytes[1]-before1);
    }
    old_calls0=path_calls[0];old_calls1=path_calls[1];old_front=active_state()->front;old_steps=active_state()->steps;
    for(i=0;i<60;++i)fire_tick(1/60.0f);
    if(path_calls[0]!=old_calls0 || path_calls[1]!=old_calls1 || active_state()->front!=old_front || active_state()->steps!=old_steps)ok=0;
    manual_steps=1;fire_tick(0);if(path_calls[0]!=old_calls0+1 || active_state()->steps!=old_steps+1)ok=0;
    if(log)fprintf(log,"Paused simulation/upload counts unchanged; manual step advances both GPU paths: %s\n",ok?"PASS":"FAIL");
    /* Recreate the selected state and prove its seeded warm-up is repeatable. */
    initialize_states();band_mode=0;sync_pending=1;fire_tick(0);a=malloc(sizeof(pixels));if(a)memcpy(a,pixels,sizeof(pixels));
    initialize_states();sync_pending=1;fire_tick(0);if(!a || memcmp(a,pixels,sizeof(pixels)))ok=0;free(a);
    grDitherMode(GR_DITHER_4x4);split=1;swapped=0;bilinear=1;draw_scene(60,1,0);a=capture("FULLFIRE.PPM");if(!a)ok=0;free(a);
    band_mode=1;band_index=1;sync_pending=1;draw_scene(60,1,0);a=capture("BAND.PPM");if(!a)ok=0;free(a);
    running=0;draw_scene(60,0,0);a=capture(NULL);draw_scene(60,0,0);b=capture(NULL);
    if(!a || !b)ok=0;else for(y=132;y<388;++y)for(x=32;x<608;++x)if(a[y*640+x]!=b[y*640+x])ok=0;
    free(a);free(b);
    if(log)fprintf(log,"Aligned mip bases, cold-prefix bounds, changes above fuel row, bounded band rows, seeded repeatability and fixed captures: %s\n",ok?"PASS":"FAIL");
    band_mode=0;sync_pending=1;running=1;return ok;
}
static int benchmark(FILE *log)
{
    int i,slow=0;float sum=0,max_ms=0;unsigned long bytes0=path_bytes[0],bytes1=path_bytes[1];FILE *mark;
    mark=fopen("START.MRK","w");if(!mark)return 0;fclose(mark);draw_scene(60,1,0);draw_scene(60,1,0);profile_reset();
    for(i=0;i<600;++i) {
        float ms;int mode=(i/150)%4;running=1;band_mode=mode!=0;if(band_mode)band_index=mode-1;
        if(i%150==0)sync_pending=1;step_rate=mode%2?30:60;split=(i/100)%2==0;single_path=(i/50)%2;bilinear=(i/100)%2;zoomed=(i/200)%2;swapped=(i/75)%2;
        if(i==300){profile_toggle();profiler_dirty=1;}
        draw_scene(60,1,1/60.0f);ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;if(ms>max_ms)max_ms=ms;if(ms>18)++slow;
    }
    mark=fopen("END.MRK","w");if(!mark)return 0;fclose(mark);
    if(log)fprintf(log,"600 frames, full/band16/32/64, 30/60 simulation Hz, split/single/filter/crop, profiler second half: %.2f guest FPS, max %.3f ms, %d over 18 ms; full %lu B, row path %lu B, palette 0 B\n",600000/sum,max_ms,slow,path_bytes[0]-bytes0,path_bytes[1]-bytes1);
    return profiler.available && !slow;
}
