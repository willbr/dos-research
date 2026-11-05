#ifndef GRAVEYARD_PUDDLE_FOOTPRINT_H
#define GRAVEYARD_PUDDLE_FOOTPRINT_H
#define WATER_Y (-.020f)
static const float water_outline[8][2]={
    {-7.5f,-25},{-4.3f,-25.3f},{-3.5f,-26.7f},{-3.9f,-29.8f},
    {-5,-31},{-7.2f,-30.7f},{-8,-29},{-8.1f,-26.5f}
};
#ifdef WATER_DEMO
static int water_contains(float x,float z)
{
    int i;
    if(x< -8.1f || x> -3.5f || z< -31 || z> -25)return 0;
    for(i=0;i<8;++i) {
        int j=(i+1)%8;
        float dx=water_outline[j][0]-water_outline[i][0],dz=water_outline[j][1]-water_outline[i][1];
        if(dx*(z-water_outline[i][1])-dz*(x-water_outline[i][0])>0)return 0;
    }
    return 1;
}
#endif
#endif
