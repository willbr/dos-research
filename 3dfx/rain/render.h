/* One untextured alpha-blend state for all small weather primitives. */
static void rain_state(void)
{
    local_color();grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);
    grDepthMask(FXFALSE);grDepthBiasLevel(0);grAlphaTestFunction(GR_CMP_ALWAYS);grCullMode(GR_CULL_DISABLE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);fog_apply(0);
}
static void rain_quad(Vec a,Vec b,Vec c,Vec d,float alpha)
{
    EffectQuad q;q.p[0]=a;q.p[1]=b;q.p[2]=c;q.p[3]=d;
    q.r=160;q.g=195;q.b=205;q.alpha=alpha;draw_effect(&q);
}
static int rain_project_drop(Vec p,GrVertex *v,float alpha)
{
    Vec d;float depth,x,y,inv;
    d.x=p.x-current_camera.eye.x;d.y=p.y-current_camera.eye.y;d.z=p.z-current_camera.eye.z;
    depth=FOG_DOT(d,blend_forward);if(depth<.45f || depth>80)return 0;
    inv=1/depth;x=320+FOG_DOT(d,blend_right)*focal*inv;y=260-FOG_DOT(d,blend_up)*focal*inv;
    if(x<0 || x>=640 || y<88 || y>=432)return 0;
    memset(v,0,sizeof(*v));v->x=x;v->y=y;v->ooz=65535*.45f*inv;v->oow=inv;
    v->r=160;v->g=195;v->b=205;v->a=alpha;return 1;
}
static void rain_draw(void)
{
    int i,j;rain_streaks=rain_rings=rain_splashes=0;
    if(!rain_level)return;
    rain_state();
    for(i=0;i<rain_level*(RAIN_MAX/3);++i) {
        RainDrop *d=&rain_drops[i];Vec tail= sub(d->p,mul(d->v,.035f));
        float depth=FOG_DOT(sub(d->p,current_camera.eye),blend_forward);
        float alpha=95*((i%3)?clamp((12-depth)/6,0,1):1);
        GrVertex a,b;
        if(alpha<3)continue;
        /* A one-pixel line avoids quad clipping/setup for each thin streak. */
        if(!rain_project_drop(d->p,&a,alpha) || !rain_project_drop(tail,&b,alpha*.3f))continue;
        grDrawLine(&a,&b);
        ++rain_streaks;
    }
    /* A single small alpha tile replaces eight separate ring strips. */
    grDepthBiasLevel(2);
    grTexSource(GR_TMU0,rain_address,GR_MIPMAPLEVELMASK_BOTH,&rain_texture);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    for(i=0;i<RAIN_HITS;++i)if(rain_hits[i].active) {
        RainHit *h=&rain_hits[i];float radius=.035f+h->age*.5f,alpha=145*(1-h->age/.8f);
        float depth=FOG_DOT(sub(vec(h->x,WATER_Y,h->z),current_camera.eye),blend_forward);
        ReflectVertex v[24];GrVertex p[24];int count;unsigned int mask=0,common=(1U<<14)-1;
        if(i%4)alpha*=clamp((12-depth)/6,0,1);
        if(alpha<3)continue;
        if(!foliage_visible(vec(h->x,WATER_Y,h->z),.4f))continue;
        if(!water_count)continue;
        for(j=0;j<4;++j) {
            int k;unsigned int code=0;float u=(j==1||j==2)?256:0,t=j<2?0:256;
            v[j]=reflect_view(vec(h->x+(u?radius:-radius),WATER_Y,h->z+(t?radius:-radius)),u,t);
            for(k=0;k<14;++k)if(reflect_distance(&v[j],k)<0)code|=1U<<k;
            mask|=code;common&=code;
        }
        if(common)continue;count=reflect_clip(v,4,14,mask);if(count<3)continue;
        for(j=0;j<count;++j){reflect_project(&v[j],&p[j]);p[j].r=160;p[j].g=195;p[j].b=205;p[j].a=alpha;}
        grDrawPolygonVertexList(count,p);++rain_rings;
    }
    local_color();grDepthBiasLevel(0);
    for(i=0;i<RAIN_HITS;++i)if(rain_hits[i].active) {
        RainHit *h=&rain_hits[i];
        float depth=FOG_DOT(sub(vec(h->x,WATER_Y,h->z),current_camera.eye),blend_forward);
        float fade=clamp((12-depth)/4,0,1);
        if(fade<.02f)continue;
        if(!foliage_visible(vec(h->x,WATER_Y,h->z),.4f))continue;
        if(h->age<.18f)for(j=-1;j<=1;j+=2) {
            Vec p=vec(h->x+j*h->age*.55f,WATER_Y+.015f+1.8f*h->age-10*h->age*h->age,h->z);
            Vec tip=add(p,vec(j*.015f,.035f,0)),w=mul(blend_right,.008f);
            rain_quad(sub(p,w),add(p,w),add(tip,w),sub(tip,w),fade*140*(1-h->age/.18f));++rain_splashes;
        }
    }
    grDepthBiasLevel(0);grFogMode(GR_FOG_DISABLE);
}
