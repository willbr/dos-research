static void detail_control_rect(float alpha,float depth)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i) {
        q[i].x=(i==1 || i==2)?380:260;q[i].y=i<2?180:280;q[i].a=alpha;
        q[i].ooz=depth;q[i].oow=q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=q[i].tmuvtx[0].tow=129;
    }
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static int detail_controls(FILE *log)
{
    int mode,ok=1,saved_fog=fog_enabled;float saved_strength=detail_strength;
    GrFog_t table[GR_FOG_TABLE_SIZE];
    for(mode=0;mode<20;++mode)detail_adjust(1);if(detail_strength!=1)ok=0;
    for(mode=0;mode<20;++mode)detail_adjust(-1);if(detail_strength!=0)ok=0;
    detail_strength=saved_strength;
    for(mode=0;mode<8;++mode) {
        int i,f=mode==1?128:mode==2 || mode==3?255:0;
        int er,eg,eb,alpha=mode==6?0:mode==7?255:128;
        float amount=detail_probe/255.0f*(alpha/255.0f),fog;
        for(i=0;i<GR_FOG_TABLE_SIZE;++i)table[i]=(GrFog_t)f;
        control_clear();grFogTable(table);fog_enabled=mode!=3;fog_apply(0);
        grDepthMask(FXTRUE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        control_rect(mode==4?0:200,mode==4?200:100,mode==4?0:40,255,mode==4?30000:10000,0);
        detail_state(0);grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
        detail_control_rect(alpha,mode==5?15000:10000);
        if(fog_enabled){detail_state(1);grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);detail_control_rect(alpha,mode==5?15000:10000);}
        if(mode==5) {
            grFogMode(GR_FOG_DISABLE);local_color();grDepthBiasLevel(0);grDepthBufferFunction(GR_CMP_GREATER);
            grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);control_rect(200,0,0,255,12000,0);
        }
        fog=fog_enabled?f/255.0f:0;
        er=mode==4?0:mode==5?200:(int)(52*fog+200*(1-fog)*(1-amount));
        eg=mode==4?200:mode==5?0:(int)(65*fog+100*(1-fog)*(1-amount));
        eb=mode==4 || mode==5?0:(int)(78*fog+40*(1-fog)*(1-amount));
        if(!control_sample(log,mode==0?"detail with zero fog":mode==1?"detail with half fog":mode==2?"detail preserves full fog":mode==3?"detail with fog disabled":mode==4?"detail rejects foreground":mode==5?"detail preserves depth":mode==6?"zero detail alpha":"full detail alpha",er,eg,eb))ok=0;
    }
    fog_enabled=saved_fog;fog_update();grFogMode(GR_FOG_DISABLE);grDepthBiasLevel(0);return ok;
}
static int detail_compare_check(FILE *log)
{
    unsigned short *base,*full,*split;int pixel,ok=1;unsigned long left=0,right=0,visible=0;
    current_camera.eye=vec(2.9f,2,1.5f);current_camera.look=vec(5.1f,1.9f,-1);
    detail_strength=1;detail_tiles=2;detail_compare=0;detail_enabled=0;render(0,1);base=capture("COMPAREB.PPM");
    detail_enabled=1;render(0,1);full=capture("COMPAREF.PPM");
    detail_compare=1;render(0,1);split=capture("COMPARE.PPM");
    if(base && full && split)for(pixel=88*640;pixel<432*640;++pixel) {
        int x=pixel%640;if(x>=318 && x<=321)continue;
        if(x<318 && base[pixel]!=split[pixel])++left;
        if(x>321 && (abs((full[pixel]>>11)-(split[pixel]>>11))>1 || abs(((full[pixel]>>5)&63)-((split[pixel]>>5)&63))>1 || abs((full[pixel]&31)-(split[pixel]&31))>1))++right;
        if(x>321 && base[pixel]!=split[pixel])++visible;
    }
    if(!base || !full || !split || left || right || visible<2000)ok=0;
    if(log)fprintf(log,"Detail comparison: left base errors %lu, right detail errors %lu, %lu visible detail pixels; %s\n",left,right,visible,ok?"PASS":"FAIL");
    free(base);free(full);free(split);detail_compare=0;return ok;
}
static int detail_scene_checks(FILE *log)
{
    int pose,pixel,ok=1;float saved_strength=detail_strength;int saved_tiles=detail_tiles;
    for(pose=0;pose<5;++pose) {
        unsigned short *off,*zero,*on,*reference;unsigned long changes=0,zero_changes=0,cull_changes=0,fog_errors=0;
        int cached,triangles;
        current_camera.eye=pose==1?vec(-4.5f,2,-27):pose==2?vec(0,22,-32):vec(2.9f,2,1.5f);
        current_camera.look=pose==1?vec(-6,.7f,-30):pose==2?vec(0,0,-29):vec(5.1f,1.9f,-1);
        fog_enabled=pose!=4;fog_update();
        if(pose==3){GrFog_t full[GR_FOG_TABLE_SIZE];int i;for(i=0;i<GR_FOG_TABLE_SIZE;++i)full[i]=255;grFogTable(full);}
        detail_tiles=4;detail_enabled=0;render(0,1);off=capture(pose==0?"BASE.PPM":"OFF.PPM");
        detail_enabled=1;detail_strength=0;render(0,1);zero=capture("ZERO.PPM");
        detail_strength=.7f;detail_culling=1;render(0,1);cached=detail_count;triangles=detail_triangles;
        on=capture(pose==0?"GRAIN.PPM":pose==1?"GRAVES.PPM":pose==2?"FAR.PPM":pose==3?"FULLFOG.PPM":"NOFOG.PPM");
        detail_culling=0;render(0,1);reference=capture("REFERENCE.PPM");detail_culling=1;
        if(off && zero && on && reference)for(pixel=88*640;pixel<432*640;++pixel) {
            if(off[pixel]!=on[pixel])++changes;if(off[pixel]!=zero[pixel])++zero_changes;if(on[pixel]!=reference[pixel])++cull_changes;
            if(pose==3 && (abs((off[pixel]>>11)-(on[pixel]>>11))>1 || abs(((off[pixel]>>5)&63)-((on[pixel]>>5)&63))>1 || abs((off[pixel]&31)-(on[pixel]&31))>1))++fog_errors;
        }
        if(!off || !zero || !on || !reference || zero_changes || cull_changes || fog_errors || ((pose==0 || pose==1 || pose==4) && (changes<100 || !cached)) || (pose==2 && (changes || cached)))ok=0;
        if(log)fprintf(log,"Detail pose %d: %lu changed, zero strength %lu, cull difference %lu, fog errors %lu; %d cached, %d triangles; %s\n",pose,changes,zero_changes,cull_changes,fog_errors,cached,triangles,ok?"PASS":"FAIL");
        free(off);free(zero);free(on);free(reference);
    }
    fog_enabled=1;fog_update();if(!detail_compare_check(log))ok=0;
    detail_enabled=1;detail_strength=saved_strength;detail_tiles=saved_tiles;return ok;
}
