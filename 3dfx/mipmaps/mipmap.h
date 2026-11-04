/* Nine RGB565 mip levels, uploaded once. Alpha cutouts/UI stay at level 0. */
static int mip_enabled=1,mip_colored,mip_dither,mip_uploads,mip_quick,mip_bench;
static float lod_bias=.5f;
static GrTexInfo debug_mips;
static FxU32 debug_address;
static const unsigned char level_colors[9][3]={
    {240,65,55},{240,135,40},{230,220,55},{65,210,90},{55,205,215},
    {70,105,230},{155,85,225},{225,75,170},{190,190,190}
};
static void mip_reduce(unsigned short *p,int size)
{
    int x,y,small=size/2;
    for(y=0;y<small;++y)for(x=0;x<small;++x) {
        unsigned short a=p[(y*2)*size+x*2],b=p[(y*2)*size+x*2+1];
        unsigned short c=p[(y*2+1)*size+x*2],d=p[(y*2+1)*size+x*2+1];
        int r=((a>>11)+(b>>11)+(c>>11)+(d>>11)+2)/4;
        int g=(((a>>5)&63)+((b>>5)&63)+((c>>5)&63)+((d>>5)&63)+2)/4;
        int blue=((a&31)+(b&31)+(c&31)+(d&31)+2)/4;
        p[y*small+x]=(unsigned short)((r<<11)|(g<<5)|blue);
    }
}
static int mip_download(int texture,unsigned short *pixels,FxU32 *next)
{
    FxU32 bytes;int lod,size=256;
    tex[texture].smallLod=GR_LOD_1;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&tex[texture]);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    address[texture]=*next;
    for(lod=GR_LOD_256;lod<=GR_LOD_1;++lod) {
        grTexDownloadMipMapLevel(GR_TMU0,*next,lod,GR_LOD_256,GR_ASPECT_1x1,
            GR_TEXFMT_RGB_565,GR_MIPMAPLEVELMASK_BOTH,pixels);++mip_uploads;
        if(size>1)mip_reduce(pixels,size);size/=2;
    }
    *next+=bytes;tex[texture].data=NULL;return 1;
}
static int mip_debug_download(unsigned short *pixels,FxU32 *next)
{
    int lod,size=256,i;FxU32 bytes;
    debug_mips.largeLod=GR_LOD_256;debug_mips.smallLod=GR_LOD_1;
    debug_mips.aspectRatio=GR_ASPECT_1x1;debug_mips.format=GR_TEXFMT_RGB_565;debug_mips.data=NULL;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&debug_mips);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;debug_address=*next;
    for(lod=GR_LOD_256;lod<=GR_LOD_1;++lod) {
        unsigned short color=rgb565(level_colors[lod][0],level_colors[lod][1],level_colors[lod][2]);
        for(i=0;i<size*size;++i)pixels[i]=color;
        grTexDownloadMipMapLevel(GR_TMU0,*next,lod,GR_LOD_256,GR_ASPECT_1x1,
            GR_TEXFMT_RGB_565,GR_MIPMAPLEVELMASK_BOTH,pixels);++mip_uploads;size/=2;
    }
    *next+=bytes;return 1;
}
static void mip_source(int texture,int pass)
{
    int eligible=texture!=3 && texture!=5 && texture!=6;
    int enabled=eligible && (compare?pass:mip_enabled);
    GrTexInfo info=(mip_colored && eligible)?debug_mips:tex[texture];
    FxU32 base=(mip_colored && eligible)?debug_address:address[texture];
    if(!enabled)info.smallLod=info.largeLod;
    grTexMipMapMode(GR_TMU0,enabled?(mip_dither?GR_MIPMAP_NEAREST_DITHER:GR_MIPMAP_NEAREST):GR_MIPMAP_DISABLE,FXFALSE);
    grTexSource(GR_TMU0,base,GR_MIPMAPLEVELMASK_BOTH,&info);
    grTexLodBiasValue(GR_TMU0,enabled?lod_bias:0);
}
static void mip_hud(int fps)
{
    char line[48];int i;
    grTexMipMapMode(GR_TMU0,GR_MIPMAP_DISABLE,FXFALSE);grTexLodBiasValue(GR_TMU0,0);
    text(21,19,"3DFX / MIPMAPPING AND LOD STUDY",1,176,157,122);
    text(21,39,"WINCHESTER CATHEDRAL",2.6f,233,224,202);
    text(480,20,compare?"OFF / ON":mip_enabled?"MIPMAPS ON":"MIPMAPS OFF",1,135,209,202);
    text(480,36,flying?"F DRONE FLIGHT":"F ORBIT CAMERA",1,176,188,190);
    if(fps>0)sprintf(line,"EMU FPS %d",fps);else strcpy(line,"EMU FPS --");text(480,53,line,1,176,157,122);
    sprintf(line,"BIAS %+.2f",lod_bias);text(480,70,line,1,176,157,122);
    if(mip_colored) {
        local_color();grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);
        for(i=0;i<9;++i)color_quad(21+i*47,80,39,3,level_colors[i][0],level_colors[i][1],level_colors[i][2]);
        for(i=0;i<9;++i){sprintf(line,"%d",256>>i);text(21+i*47,69,line,1,level_colors[i][0],level_colors[i][1],level_colors[i][2]);}
    } else text(21,69,mip_dither?"N DITHERED LOD   0 RESET BIAS":"N NEAREST LOD   0 RESET BIAS",1,153,163,158);
    if(compare){local_color();grAlphaBlendFunction(GR_BLEND_ONE,GR_BLEND_ZERO,GR_BLEND_ONE,GR_BLEND_ZERO);color_quad(319,88,2,344,30,40,40);text(18,96,"MIPMAPS OFF",1,245,231,184);text(338,96,"MIPMAPS ON",1,245,231,184);}
}
