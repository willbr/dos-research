static void present_control(void)
{grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();}
static void control_clear(void)
{grClipWindow(0,0,640,480);grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthMask(FXTRUE);grBufferClear(0x00404040,0,0);}
static int rgb_matches(unsigned short pixel,const int *rgb)
{
    int actual[3],i;actual[0]=((pixel>>11)&31)*255/31;actual[1]=((pixel>>5)&63)*255/63;actual[2]=(pixel&31)*255/31;
    for(i=0;i<3;++i)if(abs(actual[i]-rgb[i])>9)return 0;return 1;
}
static int image_control(FILE *log,int image,int format)
{
    Sample *s=&samples[image][format];unsigned short *p;int x,y,rgb[3],bad=0;
    control_clear();sample_state(image,format,0);image_quad(192,120,256,0,0,256);present_control();p=capture(NULL);if(!p)return 0;
    for(y=3;y<256;y+=8)for(x=3;x<256;x+=8) {
        unsigned long i=(y*256UL+x)*3;rgb[0]=s->rgb[i];rgb[1]=s->rgb[i+1];rgb[2]=s->rgb[i+2];if(!rgb_matches(p[(y+120)*640+x+192],rgb))++bad;
    }
    if(log)fprintf(log,"Image %c %s: 1024 GPU samples vs CPU decode, %d mismatches: %s\n",'A'+image,format_names[format],bad,bad?"FAIL":"PASS");free(p);return !bad;
}
static int code_check(const unsigned short *p,int x,int y,int image,int format)
{
    int code,rgb[3],bad=0;
    if(!p)return 256;
    for(code=0;code<256;++code){table_rgb(&samples[image][format],code,format,rgb);if(!rgb_matches(p[(y+code/16*16+8)*640+x+code%16*16+8],rgb))++bad;}
    return bad;
}
static int code_controls(FILE *log)
{
    int image,format,ok=1,bad;unsigned short *p;
    for(image=0;image<2;++image)for(format=1;format<3;++format) {
        control_clear();sample_state(image,format,0);grTexSource(GR_TMU0,code_address[format-1],GR_MIPMAPLEVELMASK_BOTH,&code_info[format-1]);image_quad(192,120,256,0,0,256);present_control();p=capture(NULL);
        bad=code_check(p,192,120,image,format);
        if(opengl_diagnostic && format==2 && bad && !code_check(p,192,120,1-image,format)) {
            ++diagnostic_limits;if(log)fprintf(log,"LIMIT: image %c NCC codebook retains the other slot's exact 256 decoded colors after a slot-only switch.\n",'A'+image);
        } else {
            if(bad)ok=0;if(log)fprintf(log,"Image %c %s: all 256 table codes, %d mismatches: %s\n",'A'+image,format_names[format],bad,bad?"FAIL":"PASS");
        }
        free(p);
    }
    return ok;
}
static unsigned long changes(const unsigned short *a,const unsigned short *b)
{
    unsigned long n=0;int x,y;if(!a || !b)return 0;
    for(y=120;y<376;++y)for(x=192;x<448;++x)if(a[y*640+x]!=b[y*640+x])++n;return n;
}
static int tests(FILE *log)
{
    int image,format,ok=1,uploads_before,bad,i;unsigned short *a,*b;GuTexPalette zero_alpha;unsigned long changed;
    grDitherMode(GR_DITHER_DISABLE);bilinear=wrong_ncc=wrong_palette=0;
    for(image=0;image<2;++image)for(format=0;format<3;++format)if(!image_control(log,image,format))ok=0;
    if(!code_controls(log))ok=0;
    /* Both tables must work in one frame, without re-downloading either. */
    uploads_before=ncc_uploads;control_clear();
    for(image=0;image<2;++image){sample_state(image,2,0);grTexSource(GR_TMU0,code_address[1],GR_MIPMAPLEVELMASK_BOTH,&code_info[1]);image_quad(image?350:30,120,256,0,0,256);}
    present_control();a=capture(NULL);bad=code_check(a,30,120,0,2)+code_check(a,350,120,1,2);
    if(opengl_diagnostic && bad && ncc_uploads==uploads_before && a &&
       ((!code_check(a,30,120,0,2) && !code_check(a,350,120,0,2)) || (!code_check(a,30,120,1,2) && !code_check(a,350,120,1,2)))) {
        ++diagnostic_limits;if(log)fprintf(log,"LIMIT: both NCC codebook views retain the same slot's colors in OpenGL; %d additional NCC uploads.\n",ncc_uploads-uploads_before);
    } else {
        if(bad || ncc_uploads!=uploads_before)ok=0;
        if(log)fprintf(log,"Two resident NCC slots, 512 codes in one frame: %d mismatches, %d additional NCC uploads: %s\n",bad,ncc_uploads-uploads_before,bad || ncc_uploads!=uploads_before?"FAIL":"PASS");
    }
    free(a);
    for(format=1;format<3;++format) {
        control_clear();sample_state(0,format,0);image_quad(192,120,256,0,0,256);present_control();a=capture(NULL);
        if(format==1)wrong_palette=1;else wrong_ncc=1;
        control_clear();sample_state(0,format,0);image_quad(192,120,256,0,0,256);present_control();b=capture(NULL);changed=changes(a,b);
        if(opengl_diagnostic && format==2 && a && b && !changed && memcmp(samples[0][2].table.nccTable.packed_data,samples[1][2].table.nccTable.packed_data,48)) {
            ++diagnostic_limits;if(log)fprintf(log,"LIMIT: wrong NCC slot leaves the cached YIQ image unchanged in OpenGL.\n");
        } else {
            if(!a || !b || changed<500)ok=0;
            if(log)fprintf(log,"Wrong %s comparison: %lu changed pixels: %s\n",format==1?"palette":"NCC slot",changed,changed>=500?"PASS":"FAIL");
        }
        free(b);
        wrong_palette=wrong_ncc=0;control_clear();sample_state(0,format,0);image_quad(192,120,256,0,0,256);present_control();b=capture(NULL);
        if(!b || changes(a,b))ok=0;free(a);free(b);
    }
    control_clear();sample_state(0,1,0);zero_alpha=samples[0][1].table.palette;
    for(i=0;i<256;++i)zero_alpha.data[i]&=0x00ffffffUL;
    grTexDownloadTable(GR_TMU0,GR_TEXTABLE_PALETTE,&zero_alpha);++palette_uploads;
    grTexSource(GR_TMU0,code_address[0],GR_MIPMAPLEVELMASK_BOTH,&code_info[0]);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_NONE,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
    image_quad(192,120,256,0,0,256);present_control();a=capture(NULL);bad=code_check(a,192,120,0,1);free(a);
    if(bad)ok=0;if(log)fprintf(log,"P8 ignores palette-entry alpha under texture-alpha blending: %d mismatches: %s\n",bad,bad?"FAIL":"PASS");
    grTexDownloadTable(GR_TMU0,GR_TEXTABLE_PALETTE,&samples[0][1].table.palette);palette_loaded=0;++palette_uploads;
    grDitherMode(GR_DITHER_4x4);gallery=1;image_selected=table_view=error_view=zoomed=moving=0;
    draw_scene(60,1);a=capture("GALLERY.PPM");draw_scene(60,1);b=capture(NULL);if(!a || !b || changes(a,b))ok=0;free(a);free(b);
    image_selected=1;draw_scene(60,1);a=capture("COOL.PPM");if(!a)ok=0;free(a);
    image_selected=0;zoomed=error_view=1;draw_scene(60,1);a=capture("ERRORS.PPM");if(!a)ok=0;free(a);
    error_view=zoomed=0;table_view=1;draw_scene(60,1);a=capture("SLOTS.PPM");if(!a)ok=0;free(a);
    if(ncc_uploads!=2 || codec_uploads!=6 || texture_bytes>2097152UL)ok=0;
    if(log)fprintf(log,"Six TexUS texture payloads, two startup NCC uploads, restored tables and repeatable captures: %s\n",ok?"PASS":"FAIL");
    table_view=0;return ok;
}
static int benchmark(FILE *log)
{
    int i,slow=0,start_ncc=ncc_uploads,start_codec=codec_uploads;float sum=0,max_ms=0;FILE *mark;
    mark=fopen("START.MRK","w");if(!mark)return 0;fclose(mark);draw_scene(60,1);draw_scene(60,1);profile_reset();
    for(i=0;i<600;++i) {
        float ms;phase=i/60.0f;moving=zoomed=1;image_selected=(i/75)%2;gallery=(i/100)%2==0;format_selected=(i/50)%3;
        table_view=i>=400 && i<500;error_view=(i/150)%2;bilinear=(i/100)%2;
        if(i==300){profile_toggle();profiler_dirty=1;}
        draw_scene(60,1);ms=profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY];sum+=ms;if(ms>max_ms)max_ms=ms;if(ms>18)++slow;
    }
    mark=fopen("END.MRK","w");if(!mark)return 0;fclose(mark);
    if(log)fprintf(log,"600 panning frames, formats/images/errors/slots/filters, profiler for second half: %.2f guest FPS, max %.3f ms, %d over 18 ms; runtime NCC uploads %d, sample texel uploads %d, palette uploads %d\n",600000/sum,max_ms,slow,ncc_uploads-start_ncc,codec_uploads-start_codec,palette_uploads);
    return profiler.available && !slow && ncc_uploads==start_ncc && codec_uploads==start_codec;
}
