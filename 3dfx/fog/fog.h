#ifdef LIGHTMAP_DEMO
static int lightmaps_enabled=1;
#endif
static int fog_enabled=1,fog_curve=2,fog_color;
static float fog_density=.035f;
static GrFog_t fog_table[GR_FOG_TABLE_SIZE];
static const unsigned char fog_colors[3][3]={{52,65,78},{88,88,81},{29,40,58}};
static void fog_update(void)
{
    if(fog_curve==0)guFogGenerateLinear(fog_table,3,1.8f/fog_density);
    else if(fog_curve==1)guFogGenerateExp(fog_table,fog_density);
    else guFogGenerateExp2(fog_table,fog_density);
    grFogTable(fog_table);
    grFogColorValue((FxU32)fog_colors[fog_color][0]|((FxU32)fog_colors[fog_color][1]<<8)|((FxU32)fog_colors[fog_color][2]<<16));
}
static void fog_apply(int additive)
{
    grFogMode(fog_enabled?(GR_FOG_WITH_TABLE|(additive?GR_FOG_ADD2:0)):GR_FOG_DISABLE);
}
static void fog_key(int key)
{
    int changed=0;
    if(key=='h'||key=='H')fog_enabled=!fog_enabled;
    if(key>='1' && key<='3'){fog_curve=key-'1';changed=1;}
    if(key=='['){fog_density=clamp(fog_density-.005f,.005f,.12f);changed=1;}
    if(key==']'){fog_density=clamp(fog_density+.005f,.005f,.12f);changed=1;}
    if(key=='q'||key=='Q'){fog_color=(fog_color+1)%3;changed=1;}
    if(changed)fog_update();
}
static void fog_sky(GrVertex *sky)
{
    int i;
    for(i=0;i<4;++i) {
        float scale=i<2?.55f:1;
        sky[i].r=fog_colors[fog_color][0]*scale;sky[i].g=fog_colors[fog_color][1]*scale;sky[i].b=fog_colors[fog_color][2]*scale;
    }
}
static void fog_hud(int fps)
{
    char line[90];
#ifdef LIGHTMAP_DEMO
    text(21,19,"3DFX / LIGHTMAPS AND FOG",1,176,157,122);
#else
    text(21,19,"3DFX / NATIVE DISTANCE FOG",1,176,157,122);
#endif
    text(21,39,"NAVE TO GRAVEYARD",2.5f,233,224,202);
#ifdef LIGHTMAP_DEMO
    text(480,20,lightmaps_enabled?"B LIGHTMAPS ON":"B BASE ONLY",1,135,209,202);
#else
    text(480,20,fog_enabled?"H FOG ON":"H FOG OFF",1,135,209,202);
#endif
    text(480,36,firstperson_mode==1?"F2 FPS WALK":firstperson_mode==2?"F3 FPS FLY":flying?"F JOURNEY":"F4 MANUAL LOOK",1,176,188,190);
    if(fps>0)sprintf(line,"FPS %d",fps);else strcpy(line,"FPS --");text(480,53,line,1,176,157,122);
    sprintf(line,"%s  DENSITY %.3f  %s  /  %s",fog_curve==0?"LINEAR":fog_curve==1?"EXP":"EXP2",fog_density,
        fog_color==0?"BLUE HAZE":fog_color==1?"STONE MIST":"NIGHT",current_camera.eye.z < -14?"GRAVEYARD":"CANDLELIT NAVE");
    text(21,69,line,1,153,163,158);
}
