/* Classic sphere-map coordinates for a distant viewer. The environment is
   fixed around the turntable; geometry, lighting and material grain stay C. */
static int environment_enabled=1,environment_triangles;
static float environment_strength=.6f;
static float environment_u[MAX_VERTS],environment_v[MAX_VERTS];
static GrVertex environment_projected[MAX_VERTS];
static unsigned long environment_stamps[MAX_VERTS],environment_frame;
static GrTexInfo environment_texture;static FxU32 environment_address,environment_bytes;
static int environment_probe_u,environment_probe_v;static unsigned short environment_probe;
static int environment_hud_load(FxU32 *next);
static void environment_adjust(int step)
{ environment_strength=clamp((float)((int)(environment_strength*10+.5f)+step),0,10)/10; }
static float environment_soft(float value,float low,float high)
{ float t=clamp((value-low)/(high-low),0,1);return t*t*(3-2*t); }
static unsigned short environment_texel(int x,int y)
{
    float nx=(x-127.5f)/127.5f,ny=(127.5f-y)/127.5f,d=nx*nx+ny*ny,nz;
    float rx,ry,rz,tx,ty,tz,t,px,py,pz,r,g,b;
    if(d>1)d=1;nz=(float)sqrt(1-d);
    rx=2*nx*nz;ry=2*ny*nz;rz=1-2*d;
    tx=fabs(rx)>.00001f?6/(float)fabs(rx):1e10f;
    ty=fabs(ry)>.00001f?5/(float)fabs(ry):1e10f;
    tz=fabs(rz)>.00001f?8/(float)fabs(rz):1e10f;
    t=tx<ty?tx:ty;if(tz<t)t=tz;px=rx*t;py=ry*t;pz=rz*t;
    if(ty==t) {
        if(ry>0){float skylight=(1-environment_soft((float)fabs(px),1.8f,3))*(1-environment_soft((float)fabs(pz),2.5f,3.5f));r=160*skylight;g=181*skylight;b=205*skylight;}
        else r=g=b=0;
    } else {
        float along=tx==t?pz:px,width=tx==t?3.7f:3.0f;
        float local=along-width*(float)floor(along/width+.5f),edge=(float)fabs(local);
        float window=(1-environment_soft(edge,.62f,.93f))*environment_soft(py,-1.4f,-.8f)*(1-environment_soft(py,3.2f,4.0f));
        float mullion=environment_soft(edge,.045f,.12f);
        float horizontal=environment_soft((float)fabs(py-1.0f),.07f,.16f);
        window*=.45f+.55f*mullion*horizontal;
        r=window*205;g=window*211;b=window*214;
        if(tx==t && rx<0){r+=window*18;g+=window*6;b-=window*28;}
    }
    return (unsigned short)((((int)clamp(r,0,255)>>3)<<11)|(((int)clamp(g,0,255)>>2)<<5)|((int)clamp(b,0,255)>>3));
}
static int environment_load(FxU32 *next,unsigned short *pixels)
{
    int x,y;
    int best=-1;
    for(y=0;y<256;++y)for(x=0;x<256;++x) {
        unsigned short value=environment_texel(x,y);int energy=(value>>11)+((value>>5)&63)+(value&31);
        pixels[y*256+x]=value;
        if(energy>best){best=energy;environment_probe=value;environment_probe_u=x;environment_probe_v=y;}
    }
    environment_texture.smallLod=environment_texture.largeLod=GR_LOD_256;
    environment_texture.aspectRatio=GR_ASPECT_1x1;environment_texture.format=GR_TEXFMT_RGB_565;
    environment_texture.data=pixels;environment_bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&environment_texture);
    if(*next+environment_bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    environment_address=*next;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,&environment_texture);
    *next+=environment_bytes;environment_texture.data=NULL;return environment_hud_load(next);
}
static void environment_vertex(int index)
{
    GrVertex *p=&environment_projected[index],*s=&projected[index];int group=vertex_lighting_group[index];
    if(environment_stamps[index]==environment_frame)return;environment_stamps[index]=environment_frame;
    p->x=s->x;p->y=s->y;p->ooz=s->ooz;p->oow=s->oow;p->a=255;
    p->tmuvtx[0].oow=s->oow;p->tmuvtx[0].sow=environment_u[group]*s->oow;p->tmuvtx[0].tow=environment_v[group]*s->oow;
}
static void environment_state(void)
{
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GEQUAL);grDepthMask(FXFALSE);grDepthBiasLevel(0);
    grTexSource(GR_TMU0,environment_address,GR_MIPMAPLEVELMASK_BOTH,&environment_texture);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grColorCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static void environment_draw(void)
{
    static const float tint[3][3]={{1,.61f,.31f},{1,.87f,.55f},{.82f,.94f,1}};
    int mat,i;environment_triangles=0;
    if(!environment_enabled || environment_strength<=0)return;
    if(++environment_frame==0){memset(environment_stamps,0,sizeof(environment_stamps));environment_frame=1;}
    environment_state();
    for(mat=0;mat<GLASS;++mat) {
        unsigned long r=(unsigned long)(255*environment_strength*tint[mat][0]);
        unsigned long g=(unsigned long)(255*environment_strength*tint[mat][1]),b=(unsigned long)(255*environment_strength*tint[mat][2]);
        grConstantColorValue(0xff000000UL|r|(g<<8)|(b<<16));
        for(i=0;i<visible_per_mat[mat];++i) {
            int id=visible_faces[mat][i],flags=aa_edges[id];Face *f=&faces[id];
            GrVertex *a,*c,*d;
            environment_vertex(f->v[0]);environment_vertex(f->v[1]);environment_vertex(f->v[2]);
            a=&environment_projected[f->v[0]];c=&environment_projected[f->v[1]];d=&environment_projected[f->v[2]];
            if(antialias && flags)grAADrawTriangle(a,c,d,(flags&1)!=0,(flags&2)!=0,(flags&4)!=0);
            else grDrawTriangle(a,c,d);
            ++environment_triangles;
        }
    }
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
}
#include "../envmap/hud.h"
