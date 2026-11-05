/* Cutouts use ordinary Z testing/writes, unlike sorted translucent cards. */
static EffectQuad foliage_cards[FOLIAGE_CARDS];
static int foliage_order[FOLIAGE_CARDS],foliage_card_count,foliage_triangles;
#ifdef WATER_DEMO
static int foliage_mirrored;
#endif
static void foliage_state(int kind)
{
    grTexSource(GR_TMU0,foliage_address[kind],GR_MIPMAPLEVELMASK_BOTH,&foliage_texture[kind]);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);grHints(GR_HINT_STWHINT,0);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grConstantColorValue((foliage_mode==1?0xcd000000UL:0xff000000UL)|0xffffffUL);
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(foliage_mode==1?FXFALSE:FXTRUE);
    grDepthBiasLevel(0);grCullMode(GR_CULL_DISABLE);
    grAlphaTestReferenceValue(127);grAlphaTestFunction(foliage_mode==0?GR_CMP_GREATER:GR_CMP_ALWAYS);
    grAlphaBlendFunction(foliage_mode==1?GR_BLEND_SRC_ALPHA:GR_BLEND_ONE,foliage_mode==1?GR_BLEND_ONE_MINUS_SRC_ALPHA:GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    fog_apply(0);
}
static int foliage_visible(Vec center,float radius)
{
    Vec d;
#ifdef WATER_DEMO
    if(foliage_mirrored)return reflection_plant_visible(center,radius);
#endif
    d.x=center.x-current_camera.eye.x;d.y=center.y-current_camera.eye.y;d.z=center.z-current_camera.eye.z;
    if(!fog_sphere_visible(d,radius,blend_forward,blend_right,blend_up,frustum_x,172/focal,1.8f,1.8f))return 0;
    if(fog_inside && !fog_through_portal(d,radius))return 0;
    return 1;
}
static void foliage_card(Vec center,Vec right,Vec up,int texture,float light)
{
    EffectQuad *q;Vec reflected=center,d;float depth,radius;
#ifdef WATER_DEMO
    if(foliage_mirrored)reflected.y=2*WATER_Y-reflected.y;
#endif
    d.x=reflected.x-current_camera.eye.x;d.y=reflected.y-current_camera.eye.y;d.z=reflected.z-current_camera.eye.z;
    depth=FOG_DOT(d,blend_forward);radius=(float)sqrt(FOG_DOT(right,right)+FOG_DOT(up,up));
#ifdef WATER_DEMO
    if(foliage_mirrored && !reflection_plant_visible(center,radius*1.5f))return;
#endif
    if(fog_inside && !fog_through_portal(d,radius*1.5f))return;
    if(depth+radius<.45f || depth-radius>95 || fabs(FOG_DOT(d,blend_right))>depth*frustum_x+radius*1.8f || fabs(FOG_DOT(d,blend_up))>depth*(172/focal)+radius*1.8f)return;
    if(foliage_card_count>=FOLIAGE_CARDS){finite_error=1;return;}
    q=&foliage_cards[foliage_card_count++];
    q->p[0].x=center.x-right.x-up.x;q->p[0].y=center.y-right.y-up.y;q->p[0].z=center.z-right.z-up.z;
    q->p[1].x=center.x+right.x-up.x;q->p[1].y=center.y+right.y-up.y;q->p[1].z=center.z+right.z-up.z;
    q->p[2].x=center.x+right.x+up.x;q->p[2].y=center.y+right.y+up.y;q->p[2].z=center.z+right.z+up.z;
    q->p[3].x=center.x-right.x+up.x;q->p[3].y=center.y-right.y+up.y;q->p[3].z=center.z-right.z+up.z;
    q->texture=texture;q->depth=depth;
    q->r=q->g=q->b=light;q->alpha=foliage_mode==1?205:255;q->bias=0;
}
static void foliage_grass(float x,float z,Vec right,float half_height,float sway,float light)
{
    /* Center moves by half the tip offset, cancelling it at both roots. */
    foliage_card(vec(x+sway*.5f,half_height-.02f,z-sway*.15f),right,
        vec(sway*.5f,half_height,-sway*.15f),1,light);
}
static void foliage_tree(float x,float z,float height,int seed)
{
    int j,k;float gust,sway,lod,dx=x-current_camera.eye.x,dz=z-current_camera.eye.z;
    if(!foliage_visible(vec(x,height*.78f,z),height*.7f))return;
    gust=foliage_gust(x,z);sway=wind_enabled?(.04f+.38f*gust)*foliage_sin(foliage_time*2.4f+seed*.4f):0;
#ifdef WATER_DEMO
    if(foliage_mirrored) {
        /* A camera-facing crown proxy bounds tree reflection work; grass uses
           its exact wind-bent cards. Match the full crown's overall extent. */
        Vec center=vec(x+sway,height*.78f,z-sway*.35f);float size=height*.32f;
        foliage_card(center,mul(blend_right,size),vec(sway*.35f,height*.34f,-sway*.12f),0,206);
        return;
    }
#endif
    /* Distant crowns retain three sprays; two smaller sprays shrink away
       over ten metres instead of consuming fill/setup behind distant windows. */
    lod=clamp(((float)sqrt(dx*dx+dz*dz)-20)/10,0,1);
    for(j=0;j<5;++j) {
        float a=j*2.399963f+seed,spread=j==0?0:height*.14f;
        Vec center=vec(x+spread*foliage_cos(a)+sway,height*(j==0?.91f:.67f+(j%2)*.12f),z+spread*foliage_sin(a)-sway*.35f);
        float size=height*(j==0?.20f:.235f),flutter=wind_enabled?(.018f+gust*.1f)*foliage_sin(foliage_time*4+j+seed):0;
        if(j>=3){size*=1-lod;if(size<.025f)continue;}
        for(k=0;k<2;++k) {
            float angle=a+k*1.5707963f;
            foliage_card(center,vec(size*foliage_cos(angle),flutter,size*foliage_sin(angle)),vec(sway*.35f,size*1.05f,-sway*.12f),0,185+(j%3)*21);
        }
    }
}
static int foliage_depth_order(const void *aa,const void *bb)
{
    float a=foliage_cards[*(const int *)aa].depth,b=foliage_cards[*(const int *)bb].depth;
    return a<b?1:a>b?-1:0;
}
static void foliage_collect(void)
{
    int i,j,side;foliage_card_count=0;
    blend_forward=unit(sub(current_camera.look,current_camera.eye));blend_right=unit(cross(blend_forward,vec(0,1,0)));blend_up=cross(blend_right,blend_forward);
    if(foliage_enabled) {
        for(side=-1;side<=1;side+=2) {
            for(j=0;j<5;++j)foliage_tree(side*(19+(j%2)*2),-20-j*8.0f,7.2f+(j%3)*1.1f,j+side);
            for(j=0;j<5;++j)for(i=0;i<5;++i) {
                float x=side*(3.7f+i*3.8f),z=-21-j*7.5f,dx=x-current_camera.eye.x,dy=.3f-current_camera.eye.y,dz=z-current_camera.eye.z,dist=dx*dx+dy*dy+dz*dz;
                float sway;
                if(dist>32*32 || !foliage_visible(vec(x,.4f,z),1.1f))continue;
                sway=wind_enabled?(.025f+foliage_gust(x,z)*.18f)*foliage_sin(foliage_time*3+i+j):0;
                foliage_grass(x,z,vec(.58f,0,.15f),.33f,sway,195);
                foliage_grass(x,z,vec(-.13f,0,.53f),.30f,sway,175);
            }
            /* Small branch sprays hugging the lower exterior wall as ivy. */
            for(j=0;j<7;++j)foliage_card(vec(side*(1.85f+j*.43f),.55f+j%3*.28f,-14.23f),vec(.45f,0,0),vec(0,.65f,0),0,170);
        }
        for(i=0;i<5;++i)foliage_tree(-20+i*10,-59,8+(i%2),i+11);
    }
    if(leaves_enabled)for(i=0;i<leaf_count;++i) {
        WindLeaf *l=&wind_leaves[i];float c,s,tilt,ct;Vec right,up;
        if(!foliage_visible(l->p,l->size*1.5f))continue;
        c=foliage_cos(l->angle);s=foliage_sin(l->angle);tilt=l->resting?0:.8f*foliage_sin(l->angle*1.7f);ct=foliage_cos(tilt);
        right=vec(l->size*c,0,l->size*s);up=vec(-l->size*s*ct,l->size*foliage_sin(tilt),l->size*c*ct);
        foliage_card(l->p,right,up,2,225);
    }
}
static void foliage_draw(void)
{
    int i,last=-1,start=drawn;FxU32 last_color=0;foliage_collect();
    for(i=0;i<foliage_card_count;++i)foliage_order[i]=i;
    if(foliage_mode==1)qsort(foliage_order,foliage_card_count,sizeof(int),foliage_depth_order);
    for(i=0;i<foliage_card_count;++i) {
        EffectQuad *q=&foliage_cards[foliage_order[i]];
        FxU32 color=((FxU32)q->alpha<<24)|(FxU32)q->r|((FxU32)q->g<<8)|((FxU32)q->b<<16);
        if(q->texture!=last){foliage_state(q->texture);last=q->texture;last_color=0;}
        if(color!=last_color){grConstantColorValue(color);last_color=color;}
        draw_effect(q);
    }
    foliage_triangles=drawn-start;
    grDepthMask(FXFALSE);grDepthBiasLevel(0);grAlphaTestFunction(GR_CMP_ALWAYS);
}
