/* Water contact extends the existing bounded leaf pool. No new allocation. */
static void water_tick(float dt)
{
    float target=.12f+.88f*foliage_gust(-5.8f,-28);
    /* Roughness builds quickly and decays more slowly after the gust. */
    float rate=target>water_energy?1.4f:.48f;
    water_energy+=(target-water_energy)*(1-(float)exp(-rate*dt));
    {
        int layer;
        for(layer=0;layer<2;++layer) {
            float x,z;const float *axis=water_uv_axes[layer];water_flow_velocity(layer,&x,&z);
            /* A pattern F(M*p+offset) moves with velocity -inverse(M)*offset'. */
            water_scroll_u[layer]-=dt*(axis[0]*x+axis[1]*z);
            water_scroll_v[layer]-=dt*(axis[2]*x+axis[3]*z);
            if(water_scroll_u[layer]>=256)water_scroll_u[layer]-=256;
            if(water_scroll_u[layer]<0)water_scroll_u[layer]+=256;
            if(water_scroll_v[layer]>=256)water_scroll_v[layer]-=256;
            if(water_scroll_v[layer]<0)water_scroll_v[layer]+=256;
        }
    }
}
static void water_float_leaf(WindLeaf *l,float dt,float gust)
{
    float nx,nz;
    l->v.x+=((wind_enabled?.045f+.18f*gust:0)-l->v.x)*dt*1.8f;
    l->v.z+=((wind_enabled?-.015f-.06f*gust:0)-l->v.z)*dt*1.8f;
    nx=l->p.x+l->v.x*dt;nz=l->p.z+l->v.z*dt;
    l->p.x=nx;l->p.z=nz;
    if(!water_contains(nx,nz) || foliage_ground(nx,nz)>WATER_Y+.035f) {
        l->floating=0;l->resting=1;l->v=vec(0,0,0);l->p.y=foliage_ground(nx,nz);l->age=0;return;
    }
    l->angle+=dt*(.08f+gust*.2f);
    l->p.y=WATER_Y+.035f+(water_ripples?water_energy*.006f*foliage_sin(foliage_time*2.1f+l->angle):0);
}
static void water_init(void)
{
    int i;water_energy=.18f;water_floating=0;
    water_scroll_u[0]=water_scroll_v[0]=0;water_scroll_u[1]=37;water_scroll_v[1]=101;
    /* Keep the water/leaf interaction visible immediately on entering the demo. */
    for(i=0;i<16;++i) {
        WindLeaf *l=&wind_leaves[i];
        l->p=vec(-7.5f+(i%4)*.8f+.12f*foliage_sin(i*2.5f),WATER_Y+.035f,-26.2f-(i/4)*.66f+.18f*foliage_cos(i*1.8f));
        l->floating=i<12;l->resting=i<12;l->v=vec(0,0,0);l->age=0;
        if(!l->floating)l->p.y=1+(i%4)*.3f;
    }
}
