static void environment_control_rect(float r,float g,float b,float alpha,float depth,float w)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i) {
        q[i].x=(i==1 || i==2)?380:260;q[i].y=i<2?180:280;
        q[i].r=r;q[i].g=g;q[i].b=b;q[i].a=alpha;q[i].ooz=depth;q[i].oow=q[i].tmuvtx[0].oow=w;
        q[i].tmuvtx[0].sow=environment_probe_u*w;q[i].tmuvtx[0].tow=environment_probe_v*w;
    }
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static int environment_controls(FILE *log)
{
    int mode,ok=1;float saved_strength=environment_strength;
    for(mode=0;mode<20;++mode)environment_adjust(1);if(environment_strength!=1)ok=0;
    for(mode=0;mode<20;++mode)environment_adjust(-1);if(environment_strength!=0)ok=0;
    environment_adjust(1);environment_adjust(-1);if(environment_strength!=0)ok=0;
    environment_strength=saved_strength;
    if(log)fprintf(log,"Strength controls: exact zero and 10-step clamps; %s\n",ok?"PASS":"FAIL");
    for(mode=0;mode<5;++mode) {
        unsigned short pixels[256];int i,r=0,g=0,b=0,er,eg,eb,pass;
        float gain=(64/255.0f)*(mode==3?128/255.0f:1);
        grClipWindow(0,0,640,480);grDepthMask(FXTRUE);grDepthBiasLevel(0);grBufferClear(0,0,0);
        grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);local_color();
        grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        environment_control_rect(mode==1?0:80,mode==1?200:60,mode==1?0:40,255,mode==1?30000:10000,1);
        environment_state();grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
        grConstantColorValue(0xff404040UL);environment_control_rect(0,0,0,mode==3?128:255,mode==2?15000:10000,mode==4?2:.25f);
        if(mode==2){local_color();grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);environment_control_rect(0,200,0,255,12000,1);}
        er=mode==1 || mode==2?0:80+(int)(((environment_probe>>11)&31)*255/31*gain);
        eg=mode==1 || mode==2?200:60+(int)(((environment_probe>>5)&63)*255/63*gain);
        eb=mode==1 || mode==2?0:40+(int)((environment_probe&31)*255/31*gain);
        grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();
        if(!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,310,220,16,16,32,pixels)){ok=0;continue;}
        for(i=0;i<256;++i){r+=((pixels[i]>>11)&31)*255/31;g+=((pixels[i]>>5)&63)*255/63;b+=(pixels[i]&31)*255/31;}
        r/=256;g/=256;b/=256;pass=abs(r-er)<=12 && abs(g-eg)<=12 && abs(b-eb)<=12;if(!pass)ok=0;
        if(log)fprintf(log,"Environment control %d: RGB %d,%d,%d expected %d,%d,%d; %s\n",mode,r,g,b,er,eg,eb,pass?"PASS":"FAIL");
    }
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);return ok;
}
static int environment_normal_check(FILE *log)
{
    int pose,i,checked=0,ok=1;float error=0;
    for(pose=0;pose<4;++pose) {
        float sy,cy,sp,cp;
        yaw=-.6f+pose*.73f;pitch=-.2f+pose*.13f;environment_enabled=1;environment_strength=.6f;prepare();
        sy=(float)sin(yaw);cy=(float)cos(yaw);sp=(float)sin(pitch);cp=(float)cos(pitch);
        for(i=0;i<lighting_group_count;++i) {
            Vec n=lighting_groups[i].normal;float nx=cy*n.x+sy*n.z;
            float ny=sp*sy*n.x+cp*n.y-sp*cy*n.z,nz=-cp*sy*n.x+sp*n.y+cp*cy*n.z;
            float rx,ry,rz,m,u,v,du,dv;
            if(nz<.02f || lighting_groups[i].mat>=GLASS)continue;
            rx=2*nx*nz;ry=2*ny*nz;rz=2*nz*nz-1;m=2*(float)sqrt(rx*rx+ry*ry+(rz+1)*(rz+1));
            u=127.5f+255*rx/m;v=127.5f-255*ry/m;du=(float)fabs(u-environment_u[i]);dv=(float)fabs(v-environment_v[i]);
            if(du>error)error=du;if(dv>error)error=dv;++checked;
        }
    }
    if(checked<100 || error>.03f)ok=0;
    if(log)fprintf(log,"Full reflection-vector/sphere-map reference: %d normal groups, max UV error %.6f; %s\n",checked,error,ok?"PASS":"FAIL");
    return ok;
}
static int environment_checks(FILE *log)
{
    int pose,pixel,ok=environment_controls(log),old_aa=antialias;float old_yaw=yaw,old_pitch=pitch,old_zoom=zoom;
    float saved_strength=environment_strength;int saved_enabled=environment_enabled;
    if(!environment_normal_check(log))ok=0;
    for(pose=0;pose<4;++pose) {
        unsigned short *off,*zero,*half,*full;unsigned long changed=0,zero_changes=0,nonmonotone=0,darker=0;int triangles;
        yaw=pose==1?1.1f:-.6f;pitch=pose==1?.2f:-.12f;zoom=pose==3?.72f:1;antialias=pose!=2;
        environment_enabled=0;render(0,1);off=capture(pose==0?"ENVOFF.PPM":"OFF.PPM");
        environment_enabled=1;environment_strength=0;render(0,1);zero=capture("ZERO.PPM");
        environment_strength=.3f;render(0,1);half=capture("HALF.PPM");
        environment_strength=.6f;render(0,1);triangles=environment_triangles;full=capture(pose==0?"ENVON.PPM":pose==1?"ENVSIDE.PPM":pose==2?"NOAA.PPM":"ENVFAR.PPM");
        if(off && zero && half && full)for(pixel=90*640;pixel<410*640;++pixel) {
            int x=pixel%640,channel;unsigned short mask[3]={31,63,31};int shift[3]={11,5,0};
            if(x<80 || x>560)continue;
            if(off[pixel]!=zero[pixel])++zero_changes;if(off[pixel]!=full[pixel])++changed;
            for(channel=0;channel<3;++channel) {
                int a=(off[pixel]>>shift[channel])&mask[channel],b=(half[pixel]>>shift[channel])&mask[channel],c=(full[pixel]>>shift[channel])&mask[channel];
                if(c+1<a)++darker;if(c+1<b || b+1<a)++nonmonotone;
            }
        }
        if(!off || !zero || !half || !full || changed<500 || zero_changes || nonmonotone || darker || triangles<100)ok=0;
        if(log)fprintf(log,"Environment pose %d: %lu changed, zero-strength %lu, darker %lu, nonmonotone %lu, %d triangles; %s\n",pose,changed,zero_changes,darker,nonmonotone,triangles,ok?"PASS":"FAIL");
        free(off);free(zero);free(half);free(full);
    }
    antialias=old_aa;yaw=old_yaw;pitch=old_pitch;zoom=old_zoom;environment_strength=saved_strength;environment_enabled=saved_enabled;
    return ok;
}
