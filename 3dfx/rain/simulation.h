static float rain_random01(void)
{
    rain_random=rain_random*1664525UL+1013904223UL;
    return (rain_random&65535)/65535.0f;
}
static Vec rain_velocity(float gust)
{
    return vec(wind_enabled?.35f+3.2f*gust:0,-7.5f,wind_enabled?-.12f-1.12f*gust:0);
}
static void rain_spawn(int i,int initial)
{
    RainDrop *d=&rain_drops[i];float x,z,height,flight;
    /* A third of the pool covers the small feature area; the rest follows
       the camera. Spawn at the upwind position of the eventual landing. */
    if(i%3==0){x=-8.1f+4.6f*rain_random01();z=-31+5.7f*rain_random01();}
    else {x=current_camera.eye.x-12+24*rain_random01();z=clamp(current_camera.eye.z-12+24*rain_random01(),-65,-15.2f);}
    height=5+3*rain_random01();if(initial)height=.1f+height*rain_random01();
    d->v=rain_velocity(foliage_gust(x,z));flight=height/7.5f;
    d->target_x=d->v.x;d->target_z=d->v.z;
    d->p=vec(x-d->v.x*flight,foliage_ground(x,z)+height,z-d->v.z*flight);
}
static int rain_water_hit(float x,float z)
{
    return water_contains(x,z) && foliage_ground(x,z)<=WATER_Y+.035f;
}
static void rain_impact(float x,float z)
{
    RainHit *h;if(!rain_water_hit(x,z) || rain_impact_wait>0)return;
    rain_impact_wait=.035f;
    h=&rain_hits[rain_next_hit];rain_next_hit=(rain_next_hit+1)%RAIN_HITS;
    h->x=x;h->z=z;h->age=0;h->active=1;++rain_impacts;
}
static void rain_init(void)
{
    int i;rain_random=701;rain_next_hit=rain_impacts=rain_tick_phase=0;rain_accumulator=rain_impact_wait=0;
    memset(rain_hits,0,sizeof(rain_hits));
    for(i=0;i<RAIN_MAX;++i)rain_spawn(i,1);
}
static void rain_tick(float dt)
{
    int i,count=rain_level*(RAIN_MAX/3);
    rain_tick_phase=(rain_tick_phase+1)%6;rain_impact_wait-=dt;
    for(i=0;i<RAIN_HITS;++i)if(rain_hits[i].active) {
        rain_hits[i].age+=dt;if(rain_hits[i].age>=.8f)rain_hits[i].active=0;
    }
    for(i=0;i<count;++i) {
        RainDrop *d=&rain_drops[i];float ground;
        /* Stagger local gust samples at 10 Hz; velocity still eases at 60 Hz. */
        if(i%6==rain_tick_phase){Vec target=rain_velocity(foliage_gust(d->p.x,d->p.z));d->target_x=target.x;d->target_z=target.z;}
        d->v.x+=(d->target_x-d->v.x)*dt*3;d->v.z+=(d->target_z-d->v.z)*dt*3;
        d->p.x+=d->v.x*dt;d->p.y+=d->v.y*dt;d->p.z+=d->v.z*dt;
        /* All supported grave contacts are below 2.4; high drops need no lookup. */
        ground=d->p.y<=2.4f?foliage_ground(d->p.x,d->p.z):-100;
        if(d->p.y<=ground){rain_impact(d->p.x,d->p.z);rain_spawn(i,0);}
        else if(fabs(d->p.x-current_camera.eye.x)>28 || fabs(d->p.z-current_camera.eye.z)>30)rain_spawn(i,1);
    }
}
static void rain_update(int paused)
{
    int steps=0;if(paused || foliage_benchmark)return;
    rain_accumulator+=clamp(profiler.available && profiler.count?profiler.frame[(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY]/1000:1/60.0f,0,.1f);
    while(rain_accumulator>=1/60.0f && steps<6){rain_tick(1/60.0f);rain_accumulator-=1/60.0f;++steps;}
}
