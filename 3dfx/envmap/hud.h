/* Rasterize changed strings once, upload one small row, draw one quad/row. */
static unsigned char environment_hud_pixels[256*128];
static char environment_hud_text[11][41];
static GrTexInfo environment_hud_texture;static FxU32 environment_hud_address;
static int environment_hud_uploaded;
static void environment_hud_update(int slot,const char *text)
{
    char line[41];int i,x,y,row=slot*8;
    strncpy(line,text,40);line[40]=0;if(!strcmp(environment_hud_text[slot],line))return;
    strcpy(environment_hud_text[slot],line);memset(environment_hud_pixels+row*256,0,256*8);
    for(i=0;line[i];++i) {
        int glyph=line[i]-32;if(glyph<0 || glyph>=96)continue;
        for(y=0;y<7;++y)for(x=0;x<5;++x)if(panel_glyph[glyph][y]&(1<<(4-x)))environment_hud_pixels[(row+y)*256+i*6+x]=255;
    }
    if(environment_hud_uploaded)grTexDownloadMipMapLevelPartial(GR_TMU0,environment_hud_address,GR_LOD_256,GR_LOD_256,
        GR_ASPECT_2x1,GR_TEXFMT_ALPHA_8,GR_MIPMAPLEVELMASK_BOTH,environment_hud_pixels+row*256,row,row+7);
}
static void environment_hud_split(int slot,const char *text)
{ environment_hud_update(slot,text);environment_hud_update(slot+1,strlen(text)>40?text+40:""); }
static int environment_hud_load(FxU32 *next)
{
    FxU32 bytes;
    environment_hud_texture.smallLod=environment_hud_texture.largeLod=GR_LOD_256;
    environment_hud_texture.aspectRatio=GR_ASPECT_2x1;environment_hud_texture.format=GR_TEXFMT_ALPHA_8;
    bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&environment_hud_texture);if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
    environment_hud_address=*next;*next+=bytes;
    environment_hud_update(0,"WINCHESTER / ENVIRONMENT MAPPING");environment_hud_update(1,"DIVING HELMET");
    environment_hud_split(7,"E REFLECTION -/+ STRENGTH LMB TURN WHEEL/RMB ZOOM A AA");
    environment_hud_split(9,"SPACE PAUSE ARROWS TURN T TEXTURE Q QUALITY TAB PROFILER ESC EXIT");
    environment_hud_texture.data=environment_hud_pixels;
    grTexDownloadMipMap(GR_TMU0,environment_hud_address,GR_MIPMAPLEVELMASK_BOTH,&environment_hud_texture);
    environment_hud_texture.data=NULL;environment_hud_uploaded=1;return 1;
}
static void environment_hud(int fps)
{
    static const struct {float x,y,scale;int r,g,b;} patch[11]={
        {22,19,1,176,157,122},{21,39,3,233,224,202},{510,22,2,135,209,202},{510,48,1,176,188,190},
        {510,63,1,176,157,122},{22,72,1,135,209,202},{262,72,1,135,209,202},
        {22,452,1,176,188,190},{262,452,1,176,188,190},{22,466,1,116,140,148},{262,466,1,116,140,148}
    };
    char line[80];GrVertex q[4];int i,slot;
    environment_hud_update(2,antialias?"AA ON":"AA OFF");
    if(fps>0)sprintf(line,"FPS %d",fps);else strcpy(line,"FPS --");environment_hud_update(3,line);
    environment_hud_update(4,detailed?"Q DETAIL":model_distant?"Q FAST FAR":"Q FAST");
    sprintf(line,"E REFLECTION %s / STRENGTH %d",environment_enabled?"ON":"OFF",(int)(environment_strength*100+.5f));environment_hud_split(5,line);
    grTexSource(GR_TMU0,environment_hud_address,GR_MIPMAPLEVELMASK_BOTH,&environment_hud_texture);
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_CLAMP,GR_TEXTURECLAMP_CLAMP);
    grTexFilterMode(GR_TMU0,GR_TEXTUREFILTER_POINT_SAMPLED,GR_TEXTUREFILTER_POINT_SAMPLED);
    grColorCombine(GR_COMBINE_FUNCTION_LOCAL,GR_COMBINE_FACTOR_NONE,GR_COMBINE_LOCAL_CONSTANT,GR_COMBINE_OTHER_NONE,FXFALSE);
    grAlphaCombine(GR_COMBINE_FUNCTION_SCALE_OTHER,GR_COMBINE_FACTOR_ONE,GR_COMBINE_LOCAL_NONE,GR_COMBINE_OTHER_TEXTURE,FXFALSE);
    grAlphaBlendFunction(GR_BLEND_SRC_ALPHA,GR_BLEND_ONE_MINUS_SRC_ALPHA,GR_BLEND_ONE,GR_BLEND_ZERO);
    memset(q,0,sizeof(q));for(i=0;i<4;++i)q[i].a=255,q[i].oow=q[i].tmuvtx[0].oow=1;
    for(slot=0;slot<11;++slot) {
        float width=(float)strlen(environment_hud_text[slot])*6-1,scale=patch[slot].scale;
        if(width<0)continue;
        grConstantColorValue(0xff000000UL|patch[slot].r|((FxU32)patch[slot].g<<8)|((FxU32)patch[slot].b<<16));
        q[0].x=q[3].x=patch[slot].x;q[1].x=q[2].x=patch[slot].x+width*scale;
        q[0].y=q[1].y=patch[slot].y;q[2].y=q[3].y=patch[slot].y+7*scale;
        q[0].tmuvtx[0].sow=q[3].tmuvtx[0].sow=0;q[1].tmuvtx[0].sow=q[2].tmuvtx[0].sow=width;
        q[0].tmuvtx[0].tow=q[1].tmuvtx[0].tow=(float)(slot*8);q[2].tmuvtx[0].tow=q[3].tmuvtx[0].tow=(float)(slot*8+7);
        grDrawTriangle(&q[0],&q[1],&q[2]);grDrawTriangle(&q[0],&q[2],&q[3]);
    }
    grTexClampMode(GR_TMU0,GR_TEXTURECLAMP_WRAP,GR_TEXTURECLAMP_WRAP);
}
