#include "scene.h"
static int glass_enabled=1,candles_enabled=1,mist_enabled=1,light_enabled=1,sorted_blending=1,blend_control;
static float blend_time;
static const unsigned char glass_colors[8][3]={
    {225,39,61},{35,77,224},{239,173,39},{39,182,106},
    {147,53,216},{45,174,207},{235,104,35},{102,151,225}
};
static unsigned short rgba4444(int r,int g,int b,int a)
{return (unsigned short)(((a>>4)<<12)|((r>>4)<<8)|((g>>4)<<4)|(b>>4));}
static unsigned char soft_alpha(int x,int y)
{
    float u=(x-127.5f)/127.5f,v=(y-127.5f)/127.5f,r=1-u*u-v*v;
    return r>0?(unsigned char)(255*r*r*r):0;
}
static unsigned short blend_texel(int texture,int x,int y)
{
    float u=x/255.0f,v=1-y/255.0f;int r,g,b,a;
    if(texture==6) {
        float center=.14f*(float)sin(v*5.4f),width=.6f*(1-v)*(float)sqrt(v+.035f);
        float d=(float)fabs((u-.5f)*2-center)/(width+.0001f),edge=1-d*d;
        if(edge<=0 || v>.985f)return 0;
        a=(int)(255*edge*clamp(v*12,0,1));r=255;g=(int)(115+125*edge);b=(int)(20+110*edge*(1-v));
        return rgba4444(r,g,b,a);
    }
    {
        float d=(float)fabs(u-.5f)+.5f,top=.54f+.46f*(float)sqrt(1-d*d)/.8660254f;
        float cx=u*6-(float)floor(u*6),cy=v*9-(float)floor(v*9);
        int column=(int)(u*6),row=(int)(v*9),index=(column*3+row*5)%8;
        int rim=u<.045f || u>.955f || v<.035f || v>top-.045f;
        int lead=cx<.035f || cy<.035f || fabs(cx-.5f)+fabs(cy-.5f)>.955f;
        float dx=u-.5f,dy=(v-.73f)*.8f,rr=dx*dx+dy*dy;
        if(v>top || u<.008f || u>.992f)return 0;
        if(rr<.0225f) {
            float angle=(float)atan2(dy,dx);index=((int)((angle+3.141593f)*8/6.283186f))&7;
            lead=(float)fabs(sin(angle*4))<.065f || rr>.020f || rr<.0004f;
        }
        if(texture==2 && (rim || lead))return 0;
        if(rim){r=179;g=165;b=134;a=255;}
        else if(lead){r=20;g=23;b=27;a=255;}
        else {
            int variation=(x*29+y*13+(x*y)%37)%25;
            r=glass_colors[index][0]-variation;g=glass_colors[index][1];b=glass_colors[index][2];a=160+variation;
        }
    }
    return rgba4444(r,g,b,a);
}
static void blending_hud(int fps)
{
    char line[80];
    text(21,19,"3DFX / ALPHA AND ADDITIVE BLENDING",1,176,157,122);
    text(21,39,"WINCHESTER / CANDLELIGHT",2.5f,233,224,202);
    text(480,20,sorted_blending?"BACK TO FRONT":"WRONG ORDER",1,sorted_blending?135:240,sorted_blending?209:120,150);
    text(480,36,flying?"F NAVE TOUR":"F MANUAL LOOK",1,176,188,190);
    if(fps>0)sprintf(line,"FPS %d",fps);else strcpy(line,"FPS --");text(480,53,line,1,176,157,122);
    sprintf(line,"GLASS %s  CANDLES %s  MIST %s  LIGHT %s",glass_enabled?"ON":"OFF",candles_enabled?"ON":"OFF",mist_enabled?"ON":"OFF",light_enabled?"ON":"OFF");
    text(21,69,line,1,153,163,158);
}
