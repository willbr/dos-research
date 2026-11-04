static int lightmap_controls(FILE *log)
{
    int i,j,ok=1;GrFog_t table[GR_FOG_TABLE_SIZE];
    for(i=0;i<4;++i) {
        int f=i==0?0:i==1?128:255;
        for(j=0;j<GR_FOG_TABLE_SIZE;++j)table[j]=(GrFog_t)f;
        control_clear();grFogTable(table);grFogMode(i==3?GR_FOG_DISABLE:GR_FOG_WITH_TABLE|GR_FOG_ADD2);
        grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);grDepthMask(FXTRUE);
        control_rect(200,100,40,255,10000,0);
        grDepthMask(FXFALSE);grDepthBufferFunction(GR_CMP_EQUAL);
        grFogMode(GR_FOG_DISABLE);
        grAlphaBlendFunction(GR_BLEND_DST_COLOR,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        control_rect(128,192,64,255,10000,0);
        if(i!=3) {
            grFogMode(GR_FOG_WITH_TABLE|GR_FOG_MULT2);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
            control_rect(128,192,64,255,10000,0);
        }
        if(!control_sample(log,i==0?"lightmap multiply":i==1?"lightmap fog composed once":i==2?"full fog ignores lightmap tint":"lightmap with fog disabled",
            i==3?100:(int)((1-f/255.0f)*200*128/255+f/255.0f*52),
            i==3?75:(int)((1-f/255.0f)*100*192/255+f/255.0f*65),
            i==3?10:(int)((1-f/255.0f)*40*64/255+f/255.0f*78)))ok=0;
    }
    grFogMode(GR_FOG_DISABLE);fog_update();return ok;
}
static int lightmap_scene_check(FILE *log)
{
    int ok=1,i;unsigned short *a,*b;
    for(i=0;i<2;++i) {
        unsigned long changed=0;int pixel;
        current_camera.eye=i?vec(-3,2.8f,-33):vec(0,1.9f,4);
        current_camera.look=i?vec(-14,1,-47):vec(0,2,-12);
        lightmaps_enabled=1;render(0,1);a=capture(i?"YARDLIT.PPM":"LIT.PPM");
        lightmaps_enabled=0;render(0,1);b=capture(i?"YARDBASE.PPM":"BASE.PPM");lightmaps_enabled=1;
        if(a && b)for(pixel=88*640;pixel<432*640;++pixel)if(a[pixel]!=b[pixel])++changed;
        if(!a || !b || changed<1000)ok=0;
        if(log)fprintf(log,"Lightmap scene %s: %lu changed pixels; %s\n",i?"graveyard":"nave",changed,ok?"PASS":"FAIL");
        free(a);free(b);
    }
    return ok;
}
