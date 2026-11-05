/* Bounded weather pools, independent of the leaf random stream. */
#define RAIN_MAX 96
#define RAIN_HITS 16
typedef struct { Vec p,v;float target_x,target_z; } RainDrop;
typedef struct { float x,z,age;int active; } RainHit;
static RainDrop rain_drops[RAIN_MAX];
static RainHit rain_hits[RAIN_HITS];
static int rain_level=2,rain_quick,rain_next_hit,rain_impacts,rain_streaks,rain_rings,rain_splashes;
static unsigned long rain_random=701;
static float rain_accumulator,rain_impact_wait;
static int rain_tick_phase;
static GrTexInfo rain_texture;
static FxU32 rain_address;
static int rain_load(FxU32 *next,unsigned char *pixels)
{
    int x,y;FxU32 bytes;
    rain_texture.smallLod=rain_texture.largeLod=GR_LOD_32;
    rain_texture.aspectRatio=GR_ASPECT_1x1;rain_texture.format=GR_TEXFMT_ALPHA_8;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&rain_texture);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    for(y=0;y<32;++y)for(x=0;x<32;++x) {
        float dx=(x-15.5f)/15.5f,dy=(y-15.5f)/15.5f,r=(float)sqrt(dx*dx+dy*dy);
        pixels[y*32+x]=(unsigned char)(255*clamp(1-fabs(r-.78f)/.09f,0,1));
    }
    rain_address=*next;rain_texture.data=pixels;
    grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,&rain_texture);
    rain_texture.data=NULL;*next+=bytes;return 1;
}
static void rain_tick(float dt);
static int rain_key(int key)
{
    if(key=='j'||key=='J'){if(rain_level>0)--rain_level;return 1;}
    if(key=='k'||key=='K'){if(rain_level<3)++rain_level;return 1;}
    return 0;
}
