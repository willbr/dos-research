static void present_control(void)
{grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();}
static unsigned short *palette_image(void)
{
    grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0,0,0);
    sample_state(2);image_quad(192,120,256,32,0,0,256,32);present_control();return capture(NULL);
}
static int verify_palette(FILE *log,const char *label)
{
    unsigned short *p=palette_image();int i,bad=0;if(!p)return 0;
    for(i=0;i<256;++i) {
        unsigned short pixel=p[136*640+192+i];FxU32 c=palette.data[i];
        int r=((pixel>>11)&31)*255/31,g=((pixel>>5)&63)*255/63,b=(pixel&31)*255/31;
        if(abs(r-(int)((c>>16)&255))>9 || abs(g-(int)((c>>8)&255))>9 || abs(b-(int)(c&255))>9)++bad;
    }
    free(p);if(log)fprintf(log,"%s: all 256 GPU palette codes, %d mismatches: %s\n",label,bad,bad?"FAIL":"PASS");return !bad;
}
static unsigned long hash_bytes(unsigned long hash,const unsigned char *p,unsigned long bytes)
{unsigned long i;for(i=0;i<bytes;++i)hash=(hash^p[i])*16777619UL;return hash;}
static unsigned long texture_hash(void)
{
    unsigned long h=hash_bytes(2166136261UL,lava_pixels,sizeof(lava_pixels));
    h=hash_bytes(h,logo_pixels,sizeof(logo_pixels));return hash_bytes(h,strip_pixels,sizeof(strip_pixels));
}
static unsigned long region_changes(const unsigned short *a,const unsigned short *b,int x,int y,int width,int height)
{
    unsigned long count=0;int row,col;if(!a || !b)return 0;
    for(row=y;row<y+height;++row)for(col=x;col<x+width;++col)if(a[row*640+col]!=b[row*640+col])++count;return count;
}
static int tests(FILE *log)
{
    int ok=1,i;unsigned long before,hash=texture_hash(),old_full,old_partial,changed=0,guards=0,logo,frozen,bytes_partial,bytes_full;
    unsigned short *a,*b;bilinear=0;cycle_enabled=0;grDitherMode(GR_DITHER_DISABLE);
    if(!verify_palette(log,"Initial full upload"))ok=0;
    before=palette_bytes;palette.data[73]=packed_rgb(30,220,180);palette_partial(73,73);
    if(palette_bytes-before!=4 || !verify_palette(log,"Single entry 73, neighbours preserved"))ok=0;
    before=palette_bytes;
    for(i=71;i<=78;++i)palette.data[i]=packed_rgb(240-(i-71)*7,50+(i-71)*13,20+(i-71)*19);
    palette_partial(71,78);
    if(palette_bytes-before!=32 || !verify_palette(log,"Unaligned inclusive 71-78 range"))ok=0;
    before=palette_bytes;palette.data[0]=packed_rgb(220,40,130);palette_partial(0,0);palette.data[255]=packed_rgb(20,210,240);palette_partial(255,255);
    if(palette_bytes-before!=8 || !verify_palette(log,"First/last entries 0 and 255"))ok=0;
    before=palette_bytes;for(i=0;i<256;++i)palette.data[i]=packed_rgb(i,255-i,(i*3)&255);palette_partial(0,255);
    if(palette_bytes-before!=1024 || !verify_palette(log,"Entire inclusive 0-255 through partial API"))ok=0;
    palette_restore();partial_updates=1;narrow_band=0;palette_phase=27;before=palette_bytes;cycle_tick(0);bytes_partial=palette_bytes-before;a=palette_image();
    palette_restore();partial_updates=0;palette_phase=27;before=palette_bytes;cycle_tick(0);bytes_full=palette_bytes-before;b=palette_image();
    if(!a || !b || region_changes(a,b,192,120,256,32) || bytes_partial!=512 || bytes_full!=1024)ok=0;
    if(log)fprintf(log,"Partial versus full palette: same pixels, %lu versus %lu uploaded bytes: %s\n",bytes_partial,bytes_full,ok?"PASS":"FAIL");free(a);free(b);
    palette_restore();partial_updates=1;palette_phase=11;cycle_tick(0);
    {GuTexPalette previous=palette;narrow_band=1;palette_phase=29;force_update=1;before=palette_bytes;cycle_tick(0);
     for(i=0;i<256;++i)if((i<96 || i>159) && previous.data[i]!=palette.data[i])ok=0;
     if(palette_bytes-before!=256 || !verify_palette(log,"Narrow 96-159 band, other CPU/GPU entries preserved"))ok=0;}
    narrow_band=0;palette_restore();grDitherMode(GR_DITHER_4x4);cycle_enabled=0;
    draw_scene(60,0,0);a=capture("INITIAL.PPM");before=palette_bytes;old_full=full_calls;old_partial=partial_calls;
    for(i=0;i<60;++i)cycle_tick(1/60.0f);
    if(palette_bytes!=before || full_calls!=old_full || partial_calls!=old_partial)ok=0;
    draw_scene(60,0,0);b=capture(NULL);if(!a || !b || region_changes(a,b,0,96,640,336))ok=0;free(b);
    cycle_enabled=1;for(i=0;i<8;++i)cycle_tick(.1f);draw_scene(60,0,0);b=capture("CYCLED.PPM");
    if(!a || !b)ok=0;
    else for(i=0;i<256*256;++i) {
        int p=(i/256+124)*640+i%256+32;
        if(a[p]!=b[p]){if(lava_pixels[i]>=64 && lava_pixels[i]<=191)++changed;else ++guards;}
    }
    logo=region_changes(a,b,352,114,256,128);frozen=region_changes(a,b,416,262,128,128);
    if(changed<500 || guards || logo<500 || frozen || full_calls!=old_full || texture_hash()!=hash || p8_uploads!=3)ok=0;
    if(log)fprintf(log,"Palette-only motion: %lu active lava pixels, %lu static-range changes, %lu shared-logo changes, %lu frozen RGB565 changes; paused uploads zero; P8 hashes/three uploads unchanged: %s\n",changed,guards,logo,frozen,ok?"PASS":"FAIL");free(a);free(b);
    palette_restore();bilinear=cycle_enabled=1;draw_scene(60,1,0);a=capture("LAVA.PPM");if(!a)ok=0;free(a);
    if(log)fprintf(log,"Full palette %lu calls; partial palette %lu calls; %lu table bytes including test/reset uploads.\n",full_calls,partial_calls,palette_bytes);
    return ok;
}
static int benchmark(FILE *log)
{
    int i,slow=0;unsigned long h=texture_hash(),p8_start=p8_uploads;float sum=0,max_ms=0;FILE *mark;
    mark=fopen("START.MRK","w");if(!mark)return 0;fclose(mark);draw_scene(60,1,0);draw_scene(60,1,0);profile_reset();
    for(i=0;i<600;++i) {
        float ms;partial_updates=(i/100)%2==0;narrow_band=(i/150)%2;direction=i<300?1:-1;speed=i<300?24:48;
        bilinear=(i/100)%2;zoomed=(i/200)%2;
        if(i%100==0 || i%150==0)force_update=1;
        if(i==300){profile_toggle();profiler_dirty=1;}
        draw_scene(60,1,1/60.0f);ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;if(ms>max_ms)max_ms=ms;if(ms>18)++slow;
    }
    mark=fopen("END.MRK","w");if(!mark)return 0;fclose(mark);
    if(log)fprintf(log,"600 cycling frames, partial/full, broad/narrow bands, speed/reverse/filter/crop, profiler for second half: %.2f guest FPS, max %.3f ms, %d over 18 ms; runtime P8 uploads %lu, source hash %s; palette bytes %lu\n",600000/sum,max_ms,slow,p8_uploads-p8_start,h==texture_hash()?"unchanged":"CHANGED",palette_bytes);
    return profiler.available && !slow && p8_uploads==p8_start && h==texture_hash();
}
