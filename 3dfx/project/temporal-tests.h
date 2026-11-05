static int projector_temporal_check(FILE *log)
{
    int pose,mode,frame,pixel,ok=projector_depth_bias==8;unsigned long original[3]={0,0,0},corrected[3]={0,0,0};
    unsigned short *previous=malloc(640UL*480*2),*current=malloc(640UL*480*2);
    if(!previous || !current){free(previous);free(current);return 0;}
#ifdef SHADOW_DEMO
    shadows_enabled=0;
#endif
#ifdef DETAIL_DEMO
    detail_enabled=0;
#endif
    projector_grid=0;projector_locked=0;projector_enabled=1;
    for(pose=0;pose<3;++pose)for(mode=0;mode<3;++mode) {
        unsigned long pops=0;int maximum=0;projector_depth_bias=mode==0?1:mode==1?8:32;
        current_camera.eye=pose==0?vec(0,1.9f,6):pose==1?vec(1.5f,1.9f,6):vec(-1,2.4f,-25);
        current_camera.look=pose==0?vec(0,.3f,-2):pose==1?vec(3.1f,1.8f,4):vec(-6,.5f,-31);
        for(frame=0;frame<60;++frame) {
            projector_time=frame/60.0f;render(0,1);
            if(!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,0,0,640,480,1280,current)){free(previous);free(current);return 0;}
            if(frame)for(pixel=88*640;pixel<432*640;pixel+=4) {
                int r,g,b,delta;if((pixel/640)&1)continue;
                r=abs((current[pixel]>>11)-(previous[pixel]>>11))*8;
                g=abs(((current[pixel]>>5)&63)-((previous[pixel]>>5)&63))*4;
                b=abs((current[pixel]&31)-(previous[pixel]&31))*8;delta=r>g?r:g;if(b>delta)delta=b;
                if(delta>24)++pops;if(delta>maximum)maximum=delta;
            }
            {unsigned short *swap=previous;previous=current;current=swap;}
        }
        if(log){fprintf(log,"Torch temporal pose %d bias %d: %lu jumps over 24, max %d\n",pose,projector_depth_bias,pops,maximum);fflush(log);}
        if(mode==0)original[pose]=pops;if(mode==1)corrected[pose]=pops;
    }
    for(pose=0;pose<2;++pose)if(original[pose]>10 && corrected[pose]>original[pose]/10)ok=0;
    if(log)fprintf(log,"Torch clipping-depth stability regression: %s\n",ok?"PASS":"FAIL");
    free(previous);free(current);projector_depth_bias=8;return ok;
}
