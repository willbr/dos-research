typedef struct { Vec p[4];float depth,r,g,b,alpha;int texture,add,bias; } EffectQuad;
static EffectQuad effect_quads[96];
static int effect_order[96],effect_count,blended_triangles,blend_clipped;
static Vec blend_right,blend_up,blend_forward;
static void emit_quad(Vec a,Vec b,Vec c,Vec d,int texture,int additive,float r,float g,float blue,float alpha,int bias)
{
    EffectQuad *q;Vec center;
    if(effect_count==96){finite_error=1;return;}q=&effect_quads[effect_count++];
    q->p[0]=a;q->p[1]=b;q->p[2]=c;q->p[3]=d;q->texture=texture;q->add=additive;
    q->r=r;q->g=g;q->b=blue;q->alpha=alpha;q->bias=bias;
    center=mul(add(add(a,b),add(c,d)),.25f);q->depth=dot(sub(center,current_camera.eye),blend_forward);
}
static void billboard(Vec center,float width,float height,int texture,int additive,float r,float g,float b,float alpha)
{
    Vec right=mul(blend_right,width*.5f),up=mul(blend_up,height*.5f);
    emit_quad(sub(sub(center,right),up),sub(add(center,right),up),add(add(center,right),up),add(sub(center,right),up),texture,additive,r,g,b,alpha,0);
    /* Fade camera-facing sprites before the near plane crosses them. */
    if(effect_count)effect_quads[effect_count-1].alpha*=clamp((effect_quads[effect_count-1].depth-.45f)/1.5f,0,1);
}
static void collect_effects(void)
{
    int i;effect_count=0;
    if(glass_enabled)for(i=0;i<glass_count;++i) {
        GlassWindow *g=&glass_windows[i];
        if(dot(g->normal,sub(current_camera.eye,g->center))>0)
            emit_quad(g->p[0],g->p[1],g->p[2],g->p[3],5,0,255,255,255,255,1);
    }
    if(light_enabled)for(i=0;i<12;++i) {
        GlassWindow *g=&glass_windows[i];float sign=g->center.x<0?-1.0f:1.0f,z=g->center.z;
        emit_quad(vec(sign*4.9f,.045f,z-1),vec(sign*4.9f,.045f,z+1),
            vec(sign*.95f,.045f,z+2.2f),vec(sign*.95f,.045f,z+.2f),2,1,220,205,190,110,1);
    }
    if(candles_enabled)for(i=0;i<12;++i) {
        float flicker=.85f+.15f*(float)sin(blend_time*8.3f+i*1.7f);
        Vec p=add(candle_positions[i],vec(.014f*(float)sin(blend_time*6+i),.15f,0));
        billboard(p,1.25f,1.25f,7,1,255,146,58,47*flicker);
        billboard(p,.22f*flicker,.34f/flicker,6,1,255,247,220,245);
    }
    if(mist_enabled) {
        billboard(vec(-.3f,.8f,-4.5f),5.8f,1.65f,7,0,120,151,186,72);
        billboard(vec(.2f,.85f,1.5f),5.6f,1.7f,7,0,185,151,113,67);
        billboard(vec(-.2f,.8f,7.5f),5.8f,1.65f,7,0,119,148,181,72);
    }
}
static void draw_effect(EffectQuad *q)
{
    ViewVertex a[12],b[12];GrVertex projected_quad[12];unsigned char mask=0,common=63;int count=4,i,j,plane;
    for(i=0;i<4;++i) {
        float x=q->p[i].x-current_camera.eye.x,y=q->p[i].y-current_camera.eye.y,z=q->p[i].z-current_camera.eye.z;
        ViewVertex *v=&a[i];float xb,yb;unsigned char code=0;
        v->x=x*blend_right.x+y*blend_right.y+z*blend_right.z;
        v->y=x*blend_up.x+y*blend_up.y+z*blend_up.z;
        v->z=x*blend_forward.x+y*blend_forward.y+z*blend_forward.z;
        v->u=(i==1||i==2)?255:0;v->v=i<2?255:0;v->r=q->r;v->g=q->g;v->b=q->b;
        xb=v->z*frustum_x;yb=v->z*(172/focal);
        if(v->z<.45f)code|=1;if(v->z>160)code|=2;
        if(v->x < -xb)code|=4;if(v->x>xb)code|=8;if(v->y < -yb)code|=16;if(v->y>yb)code|=32;
        v->code=code;mask|=code;common&=code;
    }
    if(common)return;
    for(plane=0;plane<6;++plane)if(mask&(1<<plane)) {
        int n=0;
        for(i=0;i<count;++i) {
            ViewVertex *p=&a[i],*r=&a[(i+1)%count];float dp=plane_distance(p,plane),dr=plane_distance(r,plane);
            if(dp>=0)b[n++]=*p;
            if((dp>=0)!=(dr>=0))interpolate(p,r,dp/(dp-dr),&b[n++]);
        }
        if(n<3)return;count=n;for(i=0;i<count;++i)a[i]=b[i];
    }
    for(i=0;i<count;++i){project(&a[i],&projected_quad[i],0,320);projected_quad[i].a=q->alpha;}
    for(j=1;j<count-1;++j){grDrawTriangle(&projected_quad[0],&projected_quad[j],&projected_quad[j+1]);++blended_triangles;++drawn;}
    if(mask)++blend_clipped;
}
static void blending_effects(int paused)
{
    int i,j,last_texture=-1,last_add=-1,last_bias=-999;blended_triangles=blend_clipped=0;
    if(!paused)blend_time+=profiler.available && profiler.count?clamp(profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000,.004f,.1f):1/60.0f;
    blend_forward=unit(sub(current_camera.look,current_camera.eye));blend_right=unit(cross(blend_forward,vec(0,1,0)));blend_up=cross(blend_right,blend_forward);
    collect_effects();
    for(i=0;i<effect_count;++i)effect_order[i]=i;
    for(i=1;i<effect_count;++i) {
        int item=effect_order[i];j=i-1;
        while(j>=0 && (sorted_blending?effect_quads[effect_order[j]].depth<effect_quads[item].depth:effect_quads[effect_order[j]].depth>effect_quads[item].depth)) {
            effect_order[j+1]=effect_order[j];--j;
        }
        effect_order[j+1]=item;
    }
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXFALSE);
    grAlphaTestFunction(GR_CMP_GREATER);grAlphaTestReferenceValue(0);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    for(i=0;i<effect_count;++i) {
        EffectQuad *q=&effect_quads[effect_order[i]];
        if(q->texture!=last_texture) {
            grTexSource(GR_TMU0,address[q->texture],GR_MIPMAPLEVELMASK_BOTH,&tex[q->texture]);
            grColorCombine(q->texture==7?GR_COMBINE_FUNCTION_LOCAL:GR_COMBINE_FUNCTION_SCALE_OTHER,
                q->texture==7?GR_COMBINE_FACTOR_NONE:GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,
                q->texture==7?GR_COMBINE_OTHER_NONE:GR_COMBINE_OTHER_TEXTURE,FXFALSE);
            last_texture=q->texture;
        }
        if(q->add!=last_add){grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,q->add?GR_BLEND_ONE:GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);last_add=q->add;}
        if(q->bias!=last_bias){grDepthBiasLevel(q->bias);last_bias=q->bias;}
        draw_effect(q);
    }
}
