static void control_rect(float r,float g,float b,float alpha,float depth,int textured)
{
    GrVertex q[4];int i;memset(q,0,sizeof(q));
    for(i=0;i<4;++i) {
        q[i].x=(i==1||i==2)?380:260;q[i].y=i<2?180:280;
        q[i].r=r;q[i].g=g;q[i].b=b;q[i].a=alpha;q[i].ooz=depth;q[i].oow=1;
        q[i].tmuvtx[0].oow=1;q[i].tmuvtx[0].sow=textured==2?80:textured?128:0;q[i].tmuvtx[0].tow=textured==2?180:textured?128:0;
    }
    grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
}
static void control_clear(void)
{
    grClipWindow(0,0,640,480);grDepthMask(FXTRUE);grDepthBiasLevel(0);grBufferClear(0,0,0);
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);
    grAlphaTestFunction(GR_CMP_ALWAYS);local_color();grDepthMask(FXFALSE);
}
static int control_sample(FILE *log,const char *name,int er,int eg,int eb)
{
    unsigned short pixels[256];int i,r=0,g=0,b=0,ok;
    grBufferSwap(1);while(grBufferNumPending()>0){}grSstIdle();
    if(!grLfbReadRegion(GR_BUFFER_FRONTBUFFER,310,220,16,16,32,pixels))return 0;
    for(i=0;i<256;++i){r+=((pixels[i]>>11)&31)*255/31;g+=((pixels[i]>>5)&63)*255/63;b+=(pixels[i]&31)*255/31;}
    r/=256;g/=256;b/=256;ok=abs(r-er)<=12 && abs(g-eg)<=12 && abs(b-eb)<=12;
    if(log)fprintf(log,"Control %s: RGB %d,%d,%d expected %d,%d,%d: %s\n",name,r,g,b,er,eg,eb,ok?"PASS":"FAIL");
    return ok;
}
static int blending_controls(FILE *log)
{
    int ok=1;
    control_clear();grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
    control_rect(0,0,255,128,10000,0);control_rect(255,0,0,128,20000,0);
    if(!control_sample(log,"far-blue then near-red",128,0,64))ok=0;
    control_clear();control_rect(255,0,0,128,20000,0);control_rect(0,0,255,128,10000,0);
    if(!control_sample(log,"reversed alpha order",64,0,128))ok=0;
    control_clear();grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
    control_rect(255,0,0,128,10000,0);control_rect(255,0,0,128,10000,0);
    if(!control_sample(log,"additive accumulation",255,0,0))ok=0;
    control_clear();grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);grDepthMask(FXTRUE);
    control_rect(0,200,0,255,30000,0);grDepthMask(FXFALSE);grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
    control_rect(0,0,255,255,10000,0);
    if(!control_sample(log,"opaque foreground rejects glow",0,200,0))ok=0;
    control_clear();grTexSource(GR_TMU0,address[7],GR_MIPMAPLEVELMASK_BOTH,&tex[7]);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
    control_rect(200,100,50,128,10000,1);
    if(!control_sample(log,"texture alpha times vertex alpha",100,50,25))ok=0;
    {
        unsigned short pixel=blend_texel(5,80,180);int alpha=((pixel>>12)&15)*17;
        int er=((pixel>>8)&15)*17*alpha*128/(255*255),eg=((pixel>>4)&15)*17*alpha*128/(255*255),eb=(pixel&15)*17*alpha*128/(255*255);
        control_clear();grTexSource(GR_TMU0,address[5],GR_MIPMAPLEVELMASK_BOTH,&tex[5]);
        grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
        grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
        control_rect(255,255,255,128,10000,2);
        if(!control_sample(log,"ARGB4444 colored glass",er,eg,eb))ok=0;
    }
    return ok;
}
