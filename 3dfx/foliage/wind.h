/* Deterministic CPU wind/leaf simulation; original procedural texture assets. */
#define LEAF_MAX 128
#define FOLIAGE_CARDS 640
static int foliage_enabled=1,foliage_mode,wind_enabled=1,leaves_enabled=1;
static int leaf_count=64,foliage_airborne,foliage_quick,foliage_benchmark;
static float foliage_time,foliage_accumulator,foliage_manual_gust=-100;
static float foliage_phase,foliage_phase_stamp=-1;
static GrTexInfo foliage_texture[3];
static FxU32 foliage_address[3];
typedef struct { Vec p,v;float age,angle,spin,size;int resting;
#ifdef WATER_DEMO
    int floating;
#endif
} WindLeaf;
static WindLeaf wind_leaves[LEAF_MAX];
static float foliage_sines[1025];
static float foliage_sin(float angle)
{
    float t=angle*(1024/6.28318530718f),f;int i=(int)t;
    f=t-i;if(f<0){--i;f+=1;}i&=1023;
    return foliage_sines[i]+f*(foliage_sines[i+1]-foliage_sines[i]);
}
static float foliage_cos(float angle) { return foliage_sin(angle+1.57079632679f); }
static unsigned long foliage_random=173;
static unsigned long foliage_rand(void)
{ foliage_random=foliage_random*1664525UL+1013904223UL;return foliage_random; }
static float foliage_random01(void) { return (foliage_rand()&65535)/65535.0f; }
static float foliage_pulse(float t)
{ if(t<=0 || t>=4.5f)return 0;return (float)(.5-.5*foliage_cos(t*(6.2831853f/4.5f))); }
static float foliage_gust(float x,float z)
{
    float delay=(x+24)*.065f+(-z-14)*.015f,t;
    if(!wind_enabled)return 0;
    if(foliage_phase_stamp!=foliage_time){foliage_phase=foliage_time-14*(float)floor(foliage_time/14);foliage_phase_stamp=foliage_time;}
    t=foliage_phase-delay;if(t<0)t+=14;
    return clamp(foliage_pulse(t)+foliage_pulse(foliage_time-foliage_manual_gust-delay),0,1);
}
static float foliage_ground(float x,float z)
{
    int side,i,j,style;float ax=fabs(x),y=firstperson_floor(x,z)+.025f,dx,dz;
    if(ax<5.4f || ax>15 || z> -19.4f || z< -55)return y;
    i=(int)((ax-6+2.1f)/4.2f);j=(int)((-23-z+3.5f)/7);
    if(i<0 || i>2 || j<0 || j>4)return y;
    side=x<0?-1:1;dx=ax-(6+i*4.2f);dz=z-(-23-j*7);style=i+j*3+(side+1)*2;
    if(fabs(dx)<.56f && dz>-.36f && dz<.36f) {
        if(fabs(dx)<.4f && fabs(dz)<.17f)y=.9f+(style%3)*.14f+(style%4==0?.84f:.4f);
        else y=.145f;
    } else if(fabs(dx)<.48f && dz>.2f && dz<1.94f)y=.045f;
    return y;
}
#ifdef WATER_DEMO
#include "../water/floating.h"
#endif
static void foliage_leaf_spawn(int i,int resting)
{
    WindLeaf *l=&wind_leaves[i];float x=-21+42*foliage_random01(),z=-18-41*foliage_random01();
#ifdef WATER_DEMO
    l->floating=0;
#endif
    l->p=vec(x,resting?foliage_ground(x,z):2.5f+4*foliage_random01(),z);
    l->v=vec(.5f,0,-.2f);l->age=foliage_random01()*5;l->angle=foliage_random01()*6.2831853f;
    l->spin=1+foliage_random01()*3;l->size=.13f+foliage_random01()*.12f;l->resting=resting;
}
static void foliage_init(void)
{
    int i;for(i=0;i<=1024;++i)foliage_sines[i]=(float)sin(i*(6.28318530718f/1024));
    foliage_random=173;foliage_time=2;foliage_accumulator=0;foliage_manual_gust=-100;
    for(i=0;i<LEAF_MAX;++i)foliage_leaf_spawn(i,i%3!=0);
}
static void foliage_tick(float dt)
{
    int i;foliage_time+=dt;foliage_airborne=0;
#ifdef WATER_DEMO
    water_tick(dt);
#endif
    for(i=0;i<leaf_count;++i) {
        WindLeaf *l=&wind_leaves[i];float gust=foliage_gust(l->p.x,l->p.z),floor;
        l->age+=dt;
#ifdef WATER_DEMO
        if(l->floating){water_float_leaf(l,dt,gust);continue;}
#endif
        if(l->resting) {
            if(gust>.76f+(i%5)*.025f && l->age>3) {
                l->resting=0;l->v=vec(2+gust*3,1.0f+gust*.8f,-.5f);l->age=0;l->p.y+=.025f;
            } else continue;
        }
        ++foliage_airborne;
        l->v.x+=((wind_enabled?.45f+gust*6:0)-l->v.x)*dt*2;
        l->v.z+=((wind_enabled?-.2f-gust*1.4f:0)-l->v.z)*dt*2;
        l->v.y-=dt*.9f;if(l->v.y< -1.15f)l->v.y=-1.15f;
        l->angle+=dt*l->spin;
        l->p=add(l->p,mul(l->v,dt));
        l->p.x+=dt*.16f*foliage_sin(l->angle*2);l->p.z+=dt*.1f*foliage_cos(l->angle*1.7f);
        l->p.x=clamp(l->p.x,-22.8f,22.8f);l->p.z=clamp(l->p.z,-60.7f,-15);
        floor=foliage_ground(l->p.x,l->p.z);
#ifdef WATER_DEMO
        if(l->v.y<=0 && l->p.y<=WATER_Y+.035f && floor<=WATER_Y+.035f && water_contains(l->p.x,l->p.z)) {
            l->floating=l->resting=1;l->v=vec(0,0,0);l->p.y=WATER_Y+.035f;l->age=0;--foliage_airborne;continue;
        }
#endif
        if(l->p.y<=floor) {l->p.y=floor;l->v=vec(0,0,0);l->resting=1;l->age=0;--foliage_airborne;}
    }
}
static void foliage_step(float dt)
{
    int ticks=0;foliage_accumulator+=clamp(dt,0,.1f);
    while(foliage_accumulator>=1/60.0f && ticks++<6){foliage_tick(1/60.0f);foliage_accumulator-=1/60.0f;}
}
static void foliage_update(int paused)
{
    int i;
    if(!paused && !foliage_benchmark)foliage_step(profiler.available && profiler.count?profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000:1/60.0f);
    foliage_airborne=0;for(i=0;i<leaf_count;++i)if(!wind_leaves[i].resting)++foliage_airborne;
}
static int foliage_key(int key)
{
    switch(key) {
    case 'g':case 'G':foliage_enabled=!foliage_enabled;return 1;
    case 'c':case 'C':foliage_mode=(foliage_mode+1)%3;return 1;
    case 'z':case 'Z':wind_enabled=!wind_enabled;return 1;
    case 'l':case 'L':leaves_enabled=!leaves_enabled;return 1;
    case 'm':case 'M':foliage_manual_gust=foliage_time;return 1;
    case ',':case '<':leaf_count=(int)clamp(leaf_count-16,0,LEAF_MAX);return 1;
    case '.':case '>':leaf_count=(int)clamp(leaf_count+16,0,LEAF_MAX);return 1;
    }
    return 0;
}
static void foliage_ellipse(unsigned short *p,int cx,int cy,int rx,int ry,int angle,int r,int g,int b)
{
    int x,y,lo=cx-rx-ry,hi=cx+rx+ry,top=cy-rx-ry,bottom=cy+rx+ry;
    float a=angle*.017453293f,c=(float)cos(a),s=(float)sin(a);
    if(lo<1)lo=1;if(hi>254)hi=254;if(top<1)top=1;if(bottom>254)bottom=254;
    for(y=top;y<=bottom;++y)for(x=lo;x<=hi;++x) {
        float u=((x-cx)*c+(y-cy)*s)/rx,v=(-(x-cx)*s+(y-cy)*c)/ry;
        if(u*u+v*v<1) {
            float shade=.8f+.2f*(1-v);int rr=(int)(r*shade),gg=(int)(g*shade),bb=(int)(b*shade);
            if(fabs(v)<.10f){rr+=20;gg+=12;bb+=2;}
            p[y*256+x]=(unsigned short)(0x8000|((rr>>3)<<10)|((gg>>3)<<5)|(bb>>3));
        }
    }
}
static int foliage_load(FxU32 *next,unsigned short *p)
{
    int kind,i,x,y;FxU32 bytes;
    for(kind=0;kind<3;++kind) {
        memset(p,0,256UL*256*2);foliage_random=319+kind*17;
        if(kind==0) {
            /* A branch spray with individual overlapping leaves and real gaps. */
            for(y=40;y<243;++y)for(x=0;x<256;++x)if(abs(x-(118+(243-y)/9))<2)p[y*256+x]=0x8000|(11<<10)|(9<<5)|4;
            for(i=0;i<175;++i) {
                float a=foliage_random01()*6.2831853f,d=(float)sqrt(foliage_random01());
                int cx=128+(int)(100*d*cos(a)),cy=119+(int)(103*d*sin(a));
                int autumn=i%7==0;
                foliage_ellipse(p,cx,cy,7+(int)(foliage_random01()*8),3+(int)(foliage_random01()*4),(int)(foliage_random01()*180),autumn?164:60+(i%5)*14,autumn?100:102+(i%4)*15,autumn?30:35+(i%3)*9);
            }
            /* Known samples for the independent native alpha/depth controls. */
            for(y=123;y<=133;++y)for(x=123;x<=133;++x)p[y*256+x]=0x8000|(8<<10)|(16<<5)|4;
        } else if(kind==1) {
            for(i=0;i<42;++i) {
                float base=10+foliage_random01()*236,lean=-60+120*foliage_random01(),height=60+foliage_random01()*175;
                for(y=0;y<(int)height;++y) {
                    float t=y/height;int cx=(int)(base+lean*t*t),w=1+(int)(3*(1-t));
                    for(x=cx-w;x<=cx+w;++x)if(x>0 && x<255)p[(254-y)*256+x]=(unsigned short)(0x8000|((8+i%6)<<10)|((13+i%9)<<5)|(3+i%4));
                }
            }
        } else {
            foliage_ellipse(p,128,128,91,47,-35,192,112,35);
            foliage_ellipse(p,128,128,86,39,35,166,79,25);
        }
        foliage_texture[kind].smallLod=foliage_texture[kind].largeLod=GR_LOD_256;
        foliage_texture[kind].aspectRatio=GR_ASPECT_1x1;foliage_texture[kind].format=GR_TEXFMT_ARGB_1555;
        foliage_texture[kind].data=p;bytes=grTexTextureMemRequired(GR_MIPMAPLEVELMASK_BOTH,&foliage_texture[kind]);
        if(*next+bytes>grTexMaxAddress(GR_TMU0)+1)return 0;
        foliage_address[kind]=*next;grTexDownloadMipMap(GR_TMU0,*next,GR_MIPMAPLEVELMASK_BOTH,&foliage_texture[kind]);
        *next+=bytes;foliage_texture[kind].data=NULL;
    }
    foliage_random=173;return 1;
}
