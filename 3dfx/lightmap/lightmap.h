/* One RGB565 atlas, independent lightmap UVs, native single-TMU two-pass draw. */
static int lightmap_count,lightmap_triangles;
static unsigned char lightmap_face[MAX_FACES];
static GrTexInfo lightmap_tex;
static FxU32 lightmap_address;
typedef struct { GrVertex p[3];int bias; } LightTriangle;
static LightTriangle lightmap_cache[1024];
static unsigned short lightmap_texel(int x,int y)
{
    int id=(x>=128?1:0)+(y>=128?2:0),i,side;
    float u=(x%128)/127.0f,v=(y%128)/127.0f,px,py,pz,r,g,b;
    if(id==0){px=-5.3f+10.6f*u;py=0;pz=-14+28*v;}
    else if(id==1 || id==3){px=id==1?-5.1f:5.1f;py=(1-v)*4.55f;pz=-14+28*u;}
    else {px=-25+50*u;py=0;pz=-62+48*v;}
    if(id==2) {
        float shadow=1;
        r=.47f;g=.57f;b=.78f;
        for(side=-1;side<=1;side+=2)for(i=0;i<15;++i) {
            float dx=px-side*(6+(i%3)*4.2f)-.7f,dz=pz-(-23-(i/3)*7)-.65f;
            float d=dx*dx/.55f+dz*dz/2.6f;
            if(d<1)shadow*=.43f+.57f*d;
        }
        r*=shadow;g*=shadow;b*=shadow;
    } else {
        r=.12f;g=.16f;b=.23f;
        for(i=0;i<12;++i) {
            float dx=px-candle_positions[i].x,dy=py-candle_positions[i].y,dz=pz-candle_positions[i].z;
            float falloff=1/(1+(dx*dx+dy*dy+dz*dz)*.6f);
            falloff*=falloff;r+=1.2f*falloff;g+=.65f*falloff;b+=.22f*falloff;
        }
        if(id==0)for(side=-1;side<=1;side+=2)for(i=0;i<6;++i) {
            float a=(side*px-.7f)/4.2f,c=(pz-(-11.5f+i*4.5f)+.6f)/2.5f;
            if(a>0 && a<1 && c>0 && c<1) {
                int column=(int)(a*6),row=(int)(c*9),color=(column*3+row*5)%8;
                float edge=clamp(a*12,0,1)*clamp((1-a)*12,0,1)*clamp(c*12,0,1)*clamp((1-c)*12,0,1);
                if(a*6-column<.065f || c*9-row<.065f)edge*=.2f;
                r+=glass_colors[color][0]/350.0f*edge;g+=glass_colors[color][1]/350.0f*edge;b+=glass_colors[color][2]/350.0f*edge;
            }
        }
        if(id==1 || id==3) {
            float stripe=(pz+11.5f)/4.5f;int nearest=(int)floor(stripe+.5f);
            float d=stripe-nearest,moon=.35f/(1+d*d*30+(py-2.4f)*(py-2.4f)*.8f);
            r+=moon*.35f;g+=moon*.65f;b+=moon;
        }
    }
    return rgb565((int)(clamp(r,0,1)*255),(int)(clamp(g,0,1)*255),(int)(clamp(b,0,1)*255));
}
static int lightmap_load(FxU32 *next,unsigned short *pixels)
{
    int x,y;FxU32 bytes;
    for(y=0;y<256;++y)for(x=0;x<256;++x)pixels[y*256+x]=lightmap_texel(x,y);
    lightmap_tex.smallLod=lightmap_tex.largeLod=GR_LOD_256;lightmap_tex.aspectRatio=GR_ASPECT_1x1;
    lightmap_tex.format=GR_TEXFMT_RGB_565;lightmap_tex.data=pixels;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&lightmap_tex);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    lightmap_address=*next;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,&lightmap_tex);*next+=bytes;lightmap_tex.data=NULL;
    return 1;
}
static void lightmap_mesh_init(void)
{
    unsigned char boosted[MAX_VERTS];int i,j;memset(boosted,0,sizeof(boosted));
    for(i=0;i<face_count;++i) {
        Face *f=&faces[i];Vec p=vertices[f->v[0]].p;int id=0;
        if(fabs(p.y)<.001f && f->normal.y>.99f && f->mat==PAVING)id=1;
        if(fabs(p.x+5.1f)<.001f && f->normal.x>.99f)id=2;
        if(fabs(p.x-5.1f)<.001f && f->normal.x<-.99f)id=4;
        if((fabs(p.y+.025f)<.001f && f->mat==GRASS)||(fabs(p.y-.005f)<.001f && f->mat==PAVING))id=3;
        lightmap_face[i]=(unsigned char)id;
        if(id)for(j=0;j<3;++j) {
            int v=f->v[j];float scale=id==3?1.5f:3.8f;
            if(!boosted[v]){vertices[v].r=clamp(vertices[v].r*scale,0,255);vertices[v].g=clamp(vertices[v].g*scale,0,255);vertices[v].b=clamp(vertices[v].b*scale,0,255);boosted[v]=1;}
        }
    }
}
static void lightmap_record(Face *f,const GrVertex *a,const GrVertex *b,const GrVertex *c)
{
    int i,id=lightmap_face[f-faces];LightTriangle *t;const GrVertex *src[3];
#ifdef PROJECTOR_DEMO
    if(!projector_in_quad)projector_record(f,a,b,c);
#endif
    if(!lightmaps_enabled || !id)return;
    if(lightmap_count==1024){finite_error=1;return;}
    t=&lightmap_cache[lightmap_count++];src[0]=a;src[1]=b;src[2]=c;
    t->bias=f->mat==GRASS?-4:f->mat==PAVING?-2:0;
    for(i=0;i<3;++i) {
        GrVertex *p=&t->p[i];float u=src[i]->tmuvtx[0].sow,v=src[i]->tmuvtx[0].tow,w=src[i]->oow;
        *p=*src[i];
        if(id==1){p->tmuvtx[0].sow=u*(126.0f/(128*10.6f))+63.5f*w;p->tmuvtx[0].tow=v*(126.0f/(128*28))+63.5f*w;}
        else if(id==3){p->tmuvtx[0].sow=u*(126.0f/(128*50))+63.5f*w;p->tmuvtx[0].tow=v*(126.0f/(128*48))+(128.5f+62*126.0f/48)*w;}
        else {p->tmuvtx[0].sow=u*(126.0f/(128*28))+191.5f*w;p->tmuvtx[0].tow=v*(126.0f/(128*4.55f))+(id==2?126.5f:254.5f)*w;}
    }
}
static void lightmap_cached_draw(void)
{
    int i,last_bias=999;
    for(i=0;i<lightmap_count;++i) {
        LightTriangle *t=&lightmap_cache[i];
        if(t->bias!=last_bias){grDepthBiasLevel(t->bias);last_bias=t->bias;}
        grDrawTriangle(&t->p[0],&t->p[1],&t->p[2]);++lightmap_triangles;
    }
}
static void lightmap_draw(void)
{
    lightmap_triangles=0;
    if(!lightmaps_enabled)return;
    grDepthMask(FXFALSE);grDepthBufferFunction(GR_CMP_EQUAL);grAlphaTestFunction(GR_CMP_ALWAYS);
    grTexSource(GR_TMU0,lightmap_address,GR_MIPMAPLEVELMASK_BOTH,&lightmap_tex);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_NONE,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_DST_COLOR,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
    grFogMode(GR_FOG_DISABLE);lightmap_cached_draw();
    if(fog_enabled) {
        /* DOSBox-X OpenGL omits PREFOG_COLOR. Resolve fog once with addition. */
        grConstantColorValue(0xff000000UL);
        grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
        grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
        grFogMode(GR_FOG_WITH_TABLE|GR_FOG_MULT2);lightmap_cached_draw();
    }
}
