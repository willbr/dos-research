/* Cache glyph rasterization and static strings; update changed rows only. */
static unsigned char projector_hud_pixels[256*128];
static char projector_hud_text[11][41];
static GrTexInfo projector_hud_texture;
static FxU32 projector_hud_address;
static int projector_hud_uploaded;
static void projector_hud_update(int slot,const char *text)
{
    char clipped[41];int i,x,y,row=slot*8;
    strncpy(clipped,text,40);clipped[40]=0;
    if(!strcmp(projector_hud_text[slot],clipped))return;
    strcpy(projector_hud_text[slot],clipped);memset(projector_hud_pixels+row*256,0,256*8);
    for(i=0;clipped[i];++i) {
        int glyph=clipped[i]-32;
        if(glyph<0 || glyph>=96)continue;
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(panel_glyph[glyph][y]&(1<<(4-x)))projector_hud_pixels[(row+y)*256+i*6+x]=255;
    }
    if(projector_hud_uploaded)grTexDownloadMipMapLevelPartial(GR_TMU0,projector_hud_address,GR_LOD_256,GR_LOD_256,
        GR_ASPECT_2x1,GR_TEXFMT_ALPHA_8,GR_MIPMAPLEVELMASK_BOTH,projector_hud_pixels+row*256,row,row+7);
}
static void projector_hud_split(int slot,const char *text)
{
    projector_hud_update(slot,text);projector_hud_update(slot+1,strlen(text)>40?text+40:"");
}
static void projector_hud_refresh(int fps)
{
    char line[90];
    projector_hud_update(2,projector_enabled?"P LANTERN ON":"P LANTERN OFF");
    projector_hud_update(3,projector_locked?"O FIXED IN WORLD":"O MOVING LANTERN");
    if(fps>0)sprintf(line,"FPS %d",fps);else strcpy(line,"FPS --");projector_hud_update(4,line);
#ifdef SHADOW_DEMO
    sprintf(line,"S SHADOWS %s  %s  B LIGHTMAPS %s  H FOG %s  /  %s",shadows_enabled?"ON":"OFF",projector_grid?"X GRID":"WARM BEAM",lightmaps_enabled?"ON":"OFF",fog_enabled?"ON":"OFF",current_camera.eye.z< -14?"GRAVEYARD":"NAVE");
#else
    sprintf(line,"%s  LIGHTMAPS %s  FOG %s  /  %s",projector_grid?"X GRID":"X WARM BEAM",lightmaps_enabled?"ON":"OFF",fog_enabled?"ON":"OFF",current_camera.eye.z< -14?"GRAVEYARD":"CANDLELIT NAVE");
#endif
    projector_hud_split(5,line);
}
static int projector_hud_load(FxU32 *next)
{
    FxU32 bytes;
    projector_hud_texture.smallLod=projector_hud_texture.largeLod=GR_LOD_256;
    projector_hud_texture.aspectRatio=GR_ASPECT_2x1;projector_hud_texture.format=GR_TEXFMT_ALPHA_8;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&projector_hud_texture);
    if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    projector_hud_address=*next;*next+=bytes;
#ifdef SHADOW_DEMO
    projector_hud_update(0,"3DFX / PLANAR GROUND SHADOWS");projector_hud_update(1,"WINCHESTER / SHADOWS");
    projector_hud_split(7,"S SHADOWS  P LANTERN  O HOLD LIGHT  X GRID  TAB PROFILER");
    projector_hud_split(9,"B LIGHTMAPS  H FOG  F JOURNEY  SPACE PAUSE  LMB LOOK  WHEEL DOLLY");
#else
    projector_hud_update(0,"3DFX / PROJECTED TEXTURES");projector_hud_update(1,"WINCHESTER / LANTERN");
    projector_hud_split(7,"P LANTERN  O HOLD IN WORLD  X GRID  B LIGHTMAPS  H FOG  TAB PROFILER");
    projector_hud_split(9,"F JOURNEY  SPACE PAUSE  LMB LOOK  WHEEL DOLLY  1/2/3 FOG  ESC EXIT");
#endif
    projector_hud_refresh(0);projector_hud_texture.data=projector_hud_pixels;
    grTexDownloadMipMap(GR_TMU0,projector_hud_address,GR_MIPMAPLEVELMASK_BOTH,&projector_hud_texture);
    projector_hud_texture.data=NULL;projector_hud_uploaded=1;return 1;
}
static void projector_hud(int fps)
{
    static const struct { float x,y,scale;int r,g,b; } patch[11]={
        {21,19,1,176,157,122},{21,39,2.5f,233,224,202},{480,20,1,135,209,202},
        {480,36,1,176,188,190},{480,53,1,176,157,122},{21,69,1,153,163,158},
        {261,69,1,153,163,158},{21,452,1,176,188,190},{261,452,1,176,188,190},
        {21,466,1,116,140,148},{261,466,1,116,140,148}
    };
    int slot,i;GrVertex q[4];
    projector_hud_refresh(fps);
    grTexSource(GR_TMU0,projector_hud_address,GR_MIPMAPLEVELMASK_BOTH,&projector_hud_texture);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_NONE,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
    memset(q,0,sizeof(q));for(i=0;i<4;++i)q[i].a=255,q[i].oow=q[i].tmuvtx[0].oow=1;
    for(slot=0;slot<11;++slot) {
        float width=(float)strlen(projector_hud_text[slot])*6-1,scale=patch[slot].scale,x=patch[slot].x,y=patch[slot].y;
        if(width<0)continue;
        grConstantColorValue(0xff000000UL|(FxU32)patch[slot].r|((FxU32)patch[slot].g<<8)|((FxU32)patch[slot].b<<16));
        q[0].x=q[3].x=x;q[1].x=q[2].x=x+width*scale;q[0].y=q[1].y=y;q[2].y=q[3].y=y+7*scale;
        q[0].tmuvtx[0].sow=q[3].tmuvtx[0].sow=0;q[1].tmuvtx[0].sow=q[2].tmuvtx[0].sow=width;
        q[0].tmuvtx[0].tow=q[1].tmuvtx[0].tow=(float)(slot*8);q[2].tmuvtx[0].tow=q[3].tmuvtx[0].tow=(float)(slot*8+7);
        grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
    }
}
