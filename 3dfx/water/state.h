/* One small green ripple tile sampled twice with independent offsets. */
#include "../reflect/footprint.h"
static int water_quick,water_ripples=1,water_floating,water_layers;
static float water_energy=.18f,water_strength=.8f;
static float water_scroll_u[2],water_scroll_v[2];
static GrTexInfo water_texture;
static FxU32 water_address[2];
static int water_probe_rgb[3];
static unsigned short water_texels[32*32];
static const float water_uv_axes[2][4]={{44.8f,128,-128,44.8f},{44.8f,128,-128,44.8f}};
static void water_flow_velocity(int layer,float *x,float *z)
{
    float wind=.02f+.33f*water_energy*water_energy,cross=.006f+.010f*water_energy;
    if(layer)cross=-cross;
    *x=wind+.35f*cross;*z=-.35f*wind+cross;
}
static int water_probe[2],water_probe_u[2],water_probe_v[2];
static int water_key(int key)
{
    if(key=='v'||key=='V'){water_ripples=!water_ripples;return 1;}
    if(key=='+'||key=='='){water_strength=clamp(water_strength+.1f,0,1);return 1;}
    if(key=='-'){water_strength=clamp(water_strength-.1f,0,1);return 1;}
    return 0;
}
static int water_load(FxU32 *next,unsigned char *scratch)
{
    static const float centers[9][2]={{3,4},{14,2},{25,6},{7,14},{18,13},{29,16},{2,26},{14,28},{24,25}};
    int x,y,i,best=-1;FxU32 bytes;
    water_texture.smallLod=water_texture.largeLod=GR_LOD_32;
    water_texture.aspectRatio=GR_ASPECT_1x1;water_texture.format=GR_TEXFMT_ARGB_4444;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&water_texture);
    for(y=0;y<32;++y)for(x=0;x<32;++x) {
        float u=x*(6.28318530718f/32),v=y*(6.28318530718f/32);
        float wx=x+1.6f*(float)sin(v)+.7f*(float)sin(u+v),wz=y+1.4f*(float)sin(u)-.6f*(float)cos(u-v);
        float first=1e9f,second=1e9f,ridge,mottle;int r,g,b,alpha,score;
        for(i=0;i<9;++i) {
            float dx=wx-centers[i][0],dz=wz-centers[i][1],d;
            if(dx>16)dx-=32;if(dx< -16)dx+=32;if(dz>16)dz-=32;if(dz< -16)dz+=32;
            d=dx*dx+dz*dz;if(d<first){second=first;first=d;}else if(d<second)second=d;
        }
        ridge=clamp(1-((float)sqrt(second)-(float)sqrt(first))*.9f,0,1);ridge*=ridge;
        mottle=.5f+.5f*(float)(sin(u+v)*cos(2*v-u));
        r=(int)(25+20*mottle+150*ridge);g=(int)(65+28*mottle+137*ridge);b=(int)(43+20*mottle+105*ridge);
        alpha=(int)(9+6*ridge+.5f);water_texels[y*32+x]=(unsigned short)((alpha<<12)|((r>>4)<<8)|((g>>4)<<4)|(b>>4));
        score=r+g+b;if(score>best){best=score;water_probe_u[0]=water_probe_u[1]=x*8+4;water_probe_v[0]=water_probe_v[1]=y*8+4;
            water_probe[0]=water_probe[1]=alpha*17;water_probe_rgb[0]=(r>>4)*17;water_probe_rgb[1]=(g>>4)*17;water_probe_rgb[2]=(b>>4)*17;}
    }
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    water_address[0]=water_address[1]=*next;water_texture.data=water_texels;
    grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,&water_texture);*next+=bytes;
    water_texture.data=NULL;return 1;
}
