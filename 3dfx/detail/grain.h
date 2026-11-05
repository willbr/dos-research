/* A small alpha texture modulates opaque masonry. Fog color is restored
   separately so (F*f + B*(1-f)) becomes F*f + B*(1-f)*(1-detail). */
#include "../fog/visibility.h"
#define DETAIL_MAX 512
typedef struct {GrVertex p[16];int count;} DetailSurface;
static DetailSurface detail_cache[DETAIL_MAX];
static int detail_count,detail_polygons,detail_triangles,detail_quick;
static float detail_far_w,detail_near_w;
static GrTexInfo detail_texture;static FxU32 detail_address;
static int detail_face_group[MAX_FACES],detail_group_active[FOG_MAX_GROUPS+1],detail_record_enabled,detail_culling=1,detail_probe;
static float detail_range;
#define DETAIL_RECORD(f,list,ids,count) do {if(detail_record_enabled && (!detail_culling || detail_group_active[detail_face_group[(f)-faces]]))detail_record_polygon(f,list,ids,count);}while(0)
static void detail_mesh_init(void)
{
    int group,face;
    for(group=0;group<fog_group_count;++group)for(face=fog_groups[group].first;face<fog_groups[group].last;++face)
        detail_face_group[face]=(faces[face].mat==STONE || faces[face].mat==TRIM)?group:FOG_MAX_GROUPS;
}
static float detail_noise(int x,int y)
{
    unsigned long h=(unsigned long)(x&31)*374761393UL+(unsigned long)(y&31)*668265263UL;
    h=(h^(h>>13))*1274126177UL;h^=h>>16;return (h&255)/255.0f;
}
static int detail_load(FxU32 *next,unsigned short *pixels)
{
    unsigned char *alpha=(unsigned char *)pixels;int x,y;FxU32 bytes;
    for(y=0;y<128;++y)for(x=0;x<128;++x) {
        int ix=x/4,iy=y/4;float tx=(x&3)*.25f,ty=(y&3)*.25f,a,b,value;
        tx=tx*tx*(3-2*tx);ty=ty*ty*(3-2*ty);
        a=detail_noise(ix,iy)*(1-tx)+detail_noise(ix+1,iy)*tx;
        b=detail_noise(ix,iy+1)*(1-tx)+detail_noise(ix+1,iy+1)*tx;
        value=a*(1-ty)+b*ty;
        alpha[y*128+x]=(unsigned char)(12+135*value*value+12*detail_noise(x/2,y/2));
    }
    detail_texture.smallLod=detail_texture.largeLod=GR_LOD_128;
    detail_probe=alpha[64*128+64];
    detail_texture.aspectRatio=GR_ASPECT_1x1;detail_texture.format=GR_TEXFMT_ALPHA_8;detail_texture.data=alpha;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&detail_texture);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    detail_address=*next;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,&detail_texture);
    *next+=bytes;detail_texture.data=NULL;return 1;
}
static void detail_reset(void)
{
    detail_range=6.0f*(detail_tiles>4?4.0f/detail_tiles:1);
    detail_count=detail_polygons=detail_triangles=0;detail_far_w=1/detail_range;detail_near_w=3/detail_range;
    detail_record_enabled=detail_enabled && detail_strength>0;
}
static void detail_adjust(int step)
{ detail_strength=clamp((float)((int)(detail_strength*10+.5f)+step),0,10)/10; }
static void detail_record_polygon(Face *face,const GrVertex *list,const int *ids,int count)
{
    GrVertex a[16],b[16];int i,n=0,nearby=0,outside=0;DetailSurface *s;
    if(!detail_enabled || detail_strength<=0 || (face->mat!=STONE && face->mat!=TRIM))return;
    for(i=0;i<count;++i) {
        const GrVertex *p=&list[ids?ids[i]:i];
        if(p->oow>detail_far_w)nearby=1;else outside=1;
    }
    if(!nearby)return;
    if(count>12){finite_error=1;return;}
    for(i=0;i<count;++i)a[i]=list[ids?ids[i]:i];
    if(outside) {
        for(i=0;i<count;++i) {
            GrVertex *p=&a[i],*q=&a[(i+1)%count];float dp=p->oow-detail_far_w,dq=q->oow-detail_far_w;
            if(dp>=0)b[n++]=*p;
            if((dp>=0)!=(dq>=0)) {
                float t=dp/(dp-dq);GrVertex *r=&b[n++];memset(r,0,sizeof(*r));
                r->x=p->x+(q->x-p->x)*t;r->y=p->y+(q->y-p->y)*t;r->ooz=p->ooz+(q->ooz-p->ooz)*t;
                r->oow=detail_far_w;r->tmuvtx[0].sow=p->tmuvtx[0].sow+(q->tmuvtx[0].sow-p->tmuvtx[0].sow)*t;
                r->tmuvtx[0].tow=p->tmuvtx[0].tow+(q->tmuvtx[0].tow-p->tmuvtx[0].tow)*t;
            }
        }
        count=n;for(i=0;i<count;++i)a[i]=b[i];
    }
    if(count<3)return;if(detail_count==DETAIL_MAX){finite_error=1;return;}
    s=&detail_cache[detail_count++];s->count=count;
    for(i=0;i<count;++i) {
        GrVertex *p=&s->p[i];*p=a[i];
        p->a=255*detail_strength*clamp((p->oow-detail_far_w)/(detail_near_w-detail_far_w),0,1);
        p->tmuvtx[0].sow*=detail_tiles;p->tmuvtx[0].tow*=detail_tiles;p->tmuvtx[0].oow=p->oow;
    }
}
static void detail_state(int correction)
{
    grDepthBufferMode(GR_DEPTHBUFFER_ZBUFFER);grDepthBufferFunction(GR_CMP_GEQUAL);grDepthMask(FXFALSE);grDepthBiasLevel(1);
    grHints(GR_HINT_STWHINT,0);grAlphaTestFunction(GR_CMP_ALWAYS);grConstantColorValue(0xff000000UL);
    grTexSource(GR_TMU0,detail_address,GR_MIPMAPLEVELMASK_BOTH,&detail_texture);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_BILINEAR,GR_TEXTUREFILTER_BILINEAR);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_LOCAL,GR_COMBINE_LOCAL_ITERATED,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grFogMode(correction?GR_FOG_WITH_TABLE|GR_FOG_MULT2:GR_FOG_DISABLE);
    grAlphaBlendFunction(correction?GR_BLEND_SRC_ALPHA:GR_BLEND_ZERO,correction?GR_BLEND_ONE:GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
}
static void detail_draw(void)
{
    int pass,i;if(!detail_enabled || detail_strength<=0 || !detail_count)return;
    for(pass=0;pass<(fog_enabled?2:1);++pass) {
        detail_state(pass);
        for(i=0;i<detail_count;++i) {
            DetailSurface *s=&detail_cache[i];grDrawPolygonVertexList(s->count,s->p);
            ++detail_polygons;detail_triangles+=s->count-2;
        }
    }
    grFogMode(GR_FOG_DISABLE);grDepthBiasLevel(0);
}
