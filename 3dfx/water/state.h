/* Native one-TMU water: two independent scrolling ALPHA8 layers. */
#include "../reflect/footprint.h"
static int water_quick,water_ripples=1,water_floating,water_layers;
static float water_energy=.18f,water_strength=.8f;
static GrTexInfo water_texture;
static FxU32 water_address[2];
static int water_probe[2],water_probe_u[2],water_probe_v[2];
static int water_key(int key)
{
    if(key=='v'||key=='V'){water_ripples=!water_ripples;return 1;}
    if(key=='+'||key=='='){water_strength=clamp(water_strength+.1f,0,1);return 1;}
    if(key=='-'){water_strength=clamp(water_strength-.1f,0,1);return 1;}
    return 0;
}
static int water_load(FxU32 *next,unsigned char *pixels)
{
    int kind,x,y;FxU32 bytes;
    water_texture.smallLod=water_texture.largeLod=GR_LOD_256;
    water_texture.aspectRatio=GR_ASPECT_1x1;water_texture.format=GR_TEXFMT_ALPHA_8;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&water_texture);
    for(kind=0;kind<2;++kind) {
        water_probe[kind]=0;
        for(y=0;y<256;++y)for(x=0;x<256;++x) {
            float u=x*(6.28318530718f/256),v=y*(6.28318530718f/256);
            float wave=(float)sin(u*(kind?2:3)+.8*sin(v*(kind?3:2))+.4*sin(v-u));
            float ridge=clamp((wave-.55f)/.45f,0,1);
            int value=(int)(255*ridge*ridge*(.65f+.35f*(float)cos(v*2-u)));
            pixels[y*256+x]=(unsigned char)value;
            if(value>water_probe[kind]){water_probe[kind]=value;water_probe_u[kind]=x;water_probe_v[kind]=y;}
        }
        if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
        water_address[kind]=*next;water_texture.data=pixels;
        grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,&water_texture);*next+=bytes;
    }
    water_texture.data=NULL;return 1;
}
