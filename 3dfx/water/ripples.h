static void water_state(int layer)
{
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GREATER);grDepthMask(FXFALSE);grDepthBiasLevel(1);
    grHints(GR_HINT_STWHINT,0);grAlphaTestFunction(GR_CMP_ALWAYS);
    grTexSource(GR_TMU0,water_address[layer],GR_MIPMAPLEVELMASK_BOTH,&water_texture);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
    grConstantColorValue(0xff000000UL|(FxU32)water_light[layer][0]|((FxU32)water_light[layer][1]<<8)|((FxU32)water_light[layer][2]<<16));
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
    fog_apply(1);
}
static void water_uv(float x,float z,int layer,float *u,float *v)
{
    float t=foliage_time,scroll_s=t*(layer?-6:3.5f),scroll_t=t*(layer?4:1.5f);
    scroll_s-=256*(float)floor(scroll_s/256);scroll_t-=256*(float)floor(scroll_t/256);
    *u=(layer?x*67+z*49:x*153+z*21)+scroll_s;
    *v=(layer?-x*53+z*65:-x*18+z*147)+scroll_t;
}
static void water_draw(void)
{
    int layer,i;ReflectVertex v[24];GrVertex p[24];
    float alpha;
    water_layers=0;
    if(!water_ripples || water_strength<=0 || water_count<3)return;
    for(layer=0;layer<2;++layer) {
        int count;unsigned int mask=0,common=63;
        alpha=(layer?8+105*water_energy:10+48*water_energy)*water_strength;
        for(i=0;i<8;++i) {
            int j;unsigned int code=0;float u,t;
            water_uv(water_outline[i][0],water_outline[i][1],layer,&u,&t);
            v[i]=reflect_view(vec(water_outline[i][0],WATER_Y,water_outline[i][1]),u,t);
            for(j=0;j<6;++j)if(reflect_distance(&v[i],j)<0)code|=1U<<j;
            mask|=code;common&=code;
        }
        if(common)continue;count=reflect_clip(v,8,6,mask);if(count<3)continue;
        for(i=0;i<count;++i){reflect_project(&v[i],&p[i]);p[i].a=alpha;}
        water_state(layer);grDrawPolygonVertexList(count,p);++water_layers;
    }
    grFogMode(GR_FOG_DISABLE);grDepthBiasLevel(0);
}
