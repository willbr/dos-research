static float projector_control_fbi_w=.125f;
static void projector_control_quad(float q,int camera_w)
{
    GrVertex p[4];int i;memset(p,0,sizeof(p));
    for(i=0;i<4;++i) {
        p[i].x=(i==1||i==2)?380:260;p[i].y=i<2?180:280;
        p[i].oow=projector_control_fbi_w;p[i].ooz=10000;p[i].a=255;
        p[i].tmuvtx[0].sow=127.5f*q;p[i].tmuvtx[0].tow=127.5f*q;p[i].tmuvtx[0].oow=camera_w?.125f:q;
    }
    grDrawTriangle(&p[0],&p[1],&p[2]);grDrawTriangle(&p[0],&p[2],&p[3]);
}
static void projector_control_state(void)
{
    grDepthMask(FXFALSE);grDepthBufferFunction(GR_CMP_GEQUAL);grDepthBiasLevel(1);grFogMode(GR_FOG_DISABLE);
    grTexSource(GR_TMU0,projector_address[0],GR_MIPMAPLEVELMASK_BOTH,&projector_texture);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grHints(GR_HINT_STWHINT,GR_STWHINT_W_DIFF_TMU0);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL_ALPHA,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static int projector_controls(FILE *log)
{
    int i,j,ok=1;GrFog_t table[GR_FOG_TABLE_SIZE];
    for(i=0;i<5;++i) {
        int f=i==3?128:i==4?255:0;
        control_clear();grFogMode(GR_FOG_DISABLE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        grDepthMask(FXTRUE);
        if(f) {
            for(j=0;j<GR_FOG_TABLE_SIZE;++j)table[j]=(GrFog_t)f;
            grFogTable(table);grFogMode(GR_FOG_WITH_TABLE);
        }
        control_rect(80,60,40,255,10000,0);projector_control_state();
        if(f)grFogMode(GR_FOG_WITH_TABLE|GR_FOG_ADD2);
        projector_control_quad(i==1?2:.25f,i==2);
        if(!control_sample(log,i==0?"projector independent Q=.25":i==1?"projector independent Q=2":i==2?"camera-W negative control":i==3?"projector plus lightmap fog once":"full fog unchanged by projector",
            i==2?80:i==3?107:i==4?52:162,i==2?60:i==3?93:i==4?65:120,i==2?40:i==3?71:i==4?78:64))ok=0;
    }
    control_clear();grFogMode(GR_FOG_DISABLE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    grDepthMask(FXTRUE);control_rect(0,200,0,255,30000,0);projector_control_state();projector_control_quad(.25f,0);
    if(!control_sample(log,"foreground rejects projected light",0,200,0))ok=0;
    control_clear();grFogMode(GR_FOG_DISABLE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    grDepthMask(FXTRUE);control_rect(80,60,40,255,10000,0);
    fog_curve=0;fog_update();projector_control_fbi_w=.025f;projector_control_state();
    grFogMode(GR_FOG_WITH_TABLE|GR_FOG_ADD2);projector_control_quad(.25f,0);
    if(!control_sample(log,"fog indexes camera W, texture indexes projector Q",99,74,46))ok=0;
    projector_control_fbi_w=.125f;fog_curve=2;
    grHints(GR_HINT_STWHINT,0);grFogMode(GR_FOG_DISABLE);fog_update();return ok;
}
static int projector_math_checks(FILE *log)
{
    int i,j,ok=1;float max_st=0,max_q=0;Vec fixed_eye,fixed_forward;
    projector_locked=0;current_camera=path_at(0);projector_time=.7f;projector_prepare(1);
    fixed_eye=projector_eye;fixed_forward=projector_forward;projector_locked=1;
    for(i=0;i<4;++i) {
        Vec forward,right,up;
        current_camera=path_at((float)i);projector_prepare(1);
        if(dot(sub(projector_eye,fixed_eye),sub(projector_eye,fixed_eye))>.0000001f || dot(projector_forward,fixed_forward)<.999999f)ok=0;
        forward=unit(sub(current_camera.look,current_camera.eye));right=unit(cross(forward,vec(0,1,0)));up=cross(right,forward);
        for(j=0;j<4;++j) {
            Vec p=add(current_camera.eye,add(mul(forward,4+j*2),add(mul(right,(j-1)*.8f),mul(up,.3f))));
            Vec cam=sub(p,current_camera.eye),lamp=sub(p,projector_eye);
            float w=1/dot(cam,forward),x=320+dot(cam,right)*focal*w,y=260-dot(cam,up)*focal*w;
            float s=projector_s[0]*x+projector_s[1]*y+projector_s[2]*w+projector_s[3];
            float t=projector_t[0]*x+projector_t[1]*y+projector_t[2]*w+projector_t[3];
            float q=projector_q[0]*x+projector_q[1]*y+projector_q[2]*w+projector_q[3];
            float eq=dot(lamp,projector_forward)*w,es=127.5f*(eq+dot(lamp,projector_right)*w/projector_tan),et=127.5f*(eq-dot(lamp,projector_up)*w/projector_tan);
            float ds=(float)fabs(s-es),dt=(float)fabs(t-et),dq=(float)fabs(q-eq);
            if(ds>max_st)max_st=ds;if(dt>max_st)max_st=dt;if(dq>max_q)max_q=dq;
            if(ds>.002f || dt>.002f || dq>.00005f)ok=0;
        }
    }
    if(log)fprintf(log,"Independent projector/camera coordinates and world hold: ST max %.7f, Q max %.7f; %s\n",max_st,max_q,ok?"PASS":"FAIL");
    projector_locked=0;return ok;
}
static int projector_scene_checks(FILE *log)
{
    int i,pixel,ok=projector_math_checks(log);unsigned short *lit,*other;
    for(i=0;i<3;++i) {
        unsigned long changes=0;
        current_camera.eye=i==0?vec(0,1.9f,4):vec(-3,2.8f,-33);
        current_camera.look=i==0?vec(0,2,-12):vec(-14,1,-47);
        projector_time=.7f;projector_enabled=1;projector_grid=i==2;render(0,1);lit=capture(i==0?"LANTERN.PPM":i==1?"GRAVES.PPM":"GRID.PPM");
        projector_enabled=0;render(0,1);other=capture(i==0?"DARKNAVE.PPM":"DARKYARD.PPM");projector_enabled=1;
        if(lit && other)for(pixel=88*640;pixel<432*640;++pixel)if(lit[pixel]!=other[pixel])++changes;
        if(!lit || !other || changes<100)ok=0;
        if(log)fprintf(log,"Projector %s: %lu changed pixels; %s\n",i==0?"nave":i==1?"graveyard":"grid",changes,ok?"PASS":"FAIL");
        free(lit);free(other);
    }
    projector_grid=0;projector_time=0;return ok;
}
