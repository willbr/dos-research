static int reflection_controls(FILE *log)
{
    int mode,ok=1;
    for(mode=0;mode<4;++mode) {
        control_clear();grDepthMask(FXTRUE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        control_rect(0,200,0,255,mode==1?30000:9000,0);reflection_state();control_rect(80,120,160,255,10000,0);
        if(mode==2){grDepthBiasLevel(0);control_rect(0,0,200,255,9500,0);}
        if(mode==3){grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);control_rect(23,39,47,85,10000,0);}
        if(!control_sample(log,mode==0?"water plane overlays ground":mode==1?"foreground rejects reflection":mode==2?"reflection preserves scene depth":"water tint over reflection",
            mode==1 || mode==2?0:mode==3?61:80,mode==1?200:mode==2?0:mode==3?93:120,mode==1?0:mode==2?200:mode==3?122:160))ok=0;
    }
    grDepthBiasLevel(0);return ok;
}
static int reflection_pixel_inside(float x,float y,float margin)
{
    int i;float sign=0;
    for(i=0;i<water_count;++i) {
        GrVertex *a=&water_polygon[i],*b=&water_polygon[(i+1)%water_count];float dx=b->x-a->x,dy=b->y-a->y;
        float d=dx*(y-a->y)-dy*(x-a->x),length=(float)sqrt(dx*dx+dy*dy);
        if(fabs(d)<=margin*length)continue;
        if(sign && sign*d<0)return 0;sign=d;
    }
    return water_count>=3;
}
static int reflection_scene_checks(FILE *log)
{
    int pose,pixel,ok=1;unsigned short *on,*off;
    for(pose=0;pose<4;++pose) {
        unsigned long changed=0,outside=0;int polygons;
        current_camera.eye=pose==0?vec(-1,3.4f,-22):pose==1?vec(-3,3.6f,-35):vec(0,1.9f,4);
        current_camera.look=pose==0?vec(-6,.2f,-28):pose==1?vec(-6,.2f,-28):vec(0,2,-14);
        if(pose==3)current_camera=path_at(4);
        reflections_enabled=1;render(0,1);polygons=reflect_polygons;on=capture(pose==0?"PUDDLE.PPM":pose==1?"RETURN.PPM":pose==2?"INON.PPM":"FLIGHT.PPM");
        reflections_enabled=0;render(0,1);off=capture(pose==0?"STILL.PPM":pose==1?"RETSTILL.PPM":pose==2?"INOFF.PPM":"FLATFLY.PPM");
        if(on && off)for(pixel=88*640;pixel<432*640;++pixel)if(on[pixel]!=off[pixel]) {
            ++changed;if(!reflection_pixel_inside(pixel%640+.5f,pixel/640+.5f,2))++outside;
        }
        if(!on || !off || outside || (pose!=2 && (changed<50 || polygons<1)) || (pose==2 && (changed || polygons)))ok=0;
        if(log)fprintf(log,"Reflection pose %d: %lu changed, %lu outside puddle, %d polygons; %s\n",pose,changed,outside,polygons,ok?"PASS":"FAIL");
        free(on);free(off);
    }
    reflections_enabled=1;return ok;
}
