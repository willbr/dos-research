static int shadow_controls(FILE *log)
{
    int i,ok=1;
    for(i=0;i<4;++i) {
        control_clear();grFogMode(GR_FOG_DISABLE);grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        grDepthMask(FXTRUE);control_rect(i==3?0:80,i==3?200:60,i==3?0:40,255,i==3?10040:10000,0);
        shadow_mask_state();grDepthBiasLevel(SHADOW_DEPTH_BIAS);control_rect(0,0,0,255,10000,0);
        if(i==2)control_rect(0,0,0,255,10000,0);
        if(i==1 || i==2){projector_control_state();projector_control_quad(.25f,0);}
        if(i==3) {
            local_color();grDepthMask(FXFALSE);grDepthBiasLevel(0);grDepthBufferFunction(GR_CMP_GREATER);
            grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);control_rect(255,0,0,255,10032,0);
        }
        if(!control_sample(log,i==0?"shadow mask preserves scene RGB":i==1?"depth mask blocks lantern only":i==2?"overlapping masks form a union":"shadow mask preserves foreground depth",
            i==3?0:80,i==3?200:60,i==3?0:40))ok=0;
    }
    grDepthBiasLevel(0);grDepthMask(FXFALSE);grHints(GR_HINT_STWHINT,0);return ok;
}
static int shadow_math_checks(FILE *log)
{
    ShadowPoint a,b,hull[64];ShadowCaster moved;Vec saved=projector_eye;int i,count,ok=1;
    projector_eye=vec(0,2,0);shadow_project_point(vec(1,1,2),0,&a);
    if(fabs(a.x-2)>.00001f || fabs(a.z-4)>.00001f)ok=0;
    shadow_project_point(vec(1,1,2),.005f,&b);if(fabs(b.x-1.995f)>.00001f || fabs(b.z-3.99f)>.00001f)ok=0;
    projector_eye.x=1;shadow_project_point(vec(1,1,2),0,&b);
    if(fabs(b.x-1)>.00001f || fabs(b.z-4)>.00001f || fabs(a.x-b.x)<.9f)ok=0;
    projector_eye=vec(0,2,0);moved=shadow_casters[0];
    for(i=0;i<moved.points;++i)moved.point[i].y+=5;moved.low.y+=5;moved.high.y+=5;
    if(shadow_project_caster(&moved,0,hull))ok=0;
    moved=shadow_casters[0];for(i=0;i<moved.points;++i)moved.point[i].y-=5;moved.low.y-=5;moved.high.y-=5;
    if(shadow_project_caster(&moved,0,hull))ok=0;
    count=shadow_project_caster(&shadow_casters[1],0,hull);if(count<3)ok=0;
    for(i=0;i<count;++i)if(hull[i].x!=hull[i].x || hull[i].z!=hull[i].z || fabs(hull[i].x)>100000 || fabs(hull[i].z)>100000)ok=0;
    projector_eye=saved;
    if(log)fprintf(log,"Planar projection, moving light, raised ground and horizon clipping: %s\n",ok?"PASS":"FAIL");
    return ok;
}
static void shadow_test_lamp(Vec eye,Vec target)
{
    projector_locked=1;projector_eye=eye;projector_forward=unit(sub(target,eye));
    projector_right=unit(cross(projector_forward,vec(0,1,0)));projector_up=cross(projector_right,projector_forward);
}
static int shadow_scene_checks(FILE *log)
{
    int scene,pixel,ok=shadow_math_checks(log);unsigned short *lit,*masked,*base,*no_light;
    for(scene=0;scene<2;++scene) {
        unsigned long changes=0,darker=0,below_base=0,off_changes=0;int pier_hits,grave_hits;
        current_camera.eye=scene?vec(-1,3.2f,-30):vec(0,2.4f,5);
        current_camera.look=scene?vec(-6.7f,.4f,-39):vec(4.3f,.4f,-2);
        shadow_test_lamp(scene?vec(-4,2.2f,-33):vec(0,2,3.5f),scene?vec(-8.5f,0,-41):vec(4.6f,0,-1.5f));
        projector_enabled=1;shadows_enabled=0;render(0,1);lit=capture(scene?"NOSHADG.PPM":"NOSHADN.PPM");
        shadows_enabled=1;render(0,1);pier_hits=shadow_pier_hits;grave_hits=shadow_grave_hits;masked=capture(scene?"GRAVESH.PPM":"NAVESH.PPM");
        projector_enabled=0;render(0,1);base=capture(scene?"BASEG.PPM":"BASEN.PPM");
        shadows_enabled=0;render(0,1);no_light=capture(scene?"OFFG.PPM":"OFFN.PPM");
        if(lit && masked && base && no_light)for(pixel=88*640;pixel<432*640;++pixel) {
            int mr=(masked[pixel]>>11)&31,mg=(masked[pixel]>>5)&63,mb=masked[pixel]&31;
            if(lit[pixel]!=masked[pixel])++changes;
            if(((lit[pixel]>>11)&31)>mr+1 || ((lit[pixel]>>5)&63)>mg+1 || (lit[pixel]&31)>mb+1)++darker;
            if(mr+1<((base[pixel]>>11)&31) || mg+1<((base[pixel]>>5)&63) || mb+1<(base[pixel]&31))++below_base;
            if(base[pixel]!=no_light[pixel])++off_changes;
        }
        if(!lit || !masked || !base || !no_light || darker<50 || below_base || off_changes || (scene?!grave_hits:!pier_hits))ok=0;
        if(log)fprintf(log,"Shadows %s: %lu changed, %lu darker; %lu below baked base; lamp-off %lu changes; piers %d graves %d; %s\n",
            scene?"graveyard":"nave",changes,darker,below_base,off_changes,pier_hits,grave_hits,ok?"PASS":"FAIL");
        free(lit);free(masked);free(base);free(no_light);
    }
    shadows_enabled=1;projector_enabled=1;projector_locked=0;projector_grid=0;projector_time=0;return ok;
}
