/* Orbit input and damped motion shared in design with the helmet demo. */
static float clamp(float x,float low,float high) { return x<low?low:x>high?high:x; }
/* Exact critically damped spring for a fixed target during this time step.
   Position and velocity are continuous when keyboard/mouse targets change. */
static void spring(float *value,float *speed,float target,float dt,float omega)
{
    float decay=(float)exp(-omega*dt),offset=*value-target;
    float impulse=(*speed+omega*offset)*dt;
    *value=target+(offset+impulse)*decay;
    *speed=(*speed-omega*impulse)*decay;
}
static void camera_step(float dt)
{
    if(!(previous_buttons&1)) {
        float decay=(float)exp(-4.5f*dt),travel=(1-decay)/4.5f;
        float next_pitch=target_pitch+inertia_pitch*travel;
        target_yaw+=inertia_yaw*travel;
        target_pitch=clamp(next_pitch,-.55f,.45f);
        if(target_pitch!=next_pitch) inertia_pitch=0;
        inertia_yaw*=decay;
        inertia_pitch*=decay;
        if(fabs(inertia_yaw)<.001f) inertia_yaw=0;
        if(fabs(inertia_pitch)<.001f) inertia_pitch=0;
    }
    spring(&yaw,&yaw_speed,target_yaw,dt,16);
    spring(&pitch,&pitch_speed,target_pitch,dt,16);
    spring(&zoom,&zoom_speed,target_zoom,dt,18);
    pitch=clamp(pitch,-.55f,.45f);zoom=clamp(zoom,.65f,1.3f);
}
static void reset_target(void)
{
    target_yaw=-.6f;target_pitch=-.12f;target_zoom=1;
    inertia_yaw=inertia_pitch=drag_yaw_speed=drag_pitch_speed=0;
}
static void mouse_apply(int dx,int dy,int buttons,int wheel,float dt,int *paused)
{
    if(buttons&1) {
        float blend=1-(float)exp(-25*dt);
        if(!(previous_buttons&1)) {
            target_yaw=yaw;target_pitch=pitch;
            inertia_yaw=inertia_pitch=drag_yaw_speed=drag_pitch_speed=0;
            yaw_speed=pitch_speed=0;
        }
        target_yaw+=dx*.007f;
        target_pitch=clamp(target_pitch+dy*.005f,-.55f,.45f);
        drag_yaw_speed+=(dx*.007f/dt-drag_yaw_speed)*blend;
        drag_pitch_speed+=(dy*.005f/dt-drag_pitch_speed)*blend;
        *paused=1;
    } else if(previous_buttons&1) {
        inertia_yaw=clamp(drag_yaw_speed*.8f,-5,5);
        inertia_pitch=clamp(drag_pitch_speed*.8f,-2,2);
    }
    if(buttons&2) {
        target_zoom=clamp(target_zoom-dy*.004f,.65f,1.3f);*paused=1;
        inertia_yaw=inertia_pitch=0;
    }
    if(wheel) target_zoom=clamp(target_zoom-wheel*.08f,.65f,1.3f);
    if((buttons&4) && !(previous_buttons&4)) { reset_target();*paused=1; }
    previous_buttons=buttons;
}
static void mouse_init(void)
{
    union REGS r;
    memset(&r,0,sizeof(r));int386(0x33,&r,&r);
    mouse_present=r.w.ax==0xffff;
    if(!mouse_present) return;
    memset(&r,0,sizeof(r));r.w.ax=0x11;int386(0x33,&r,&r);
    wheel_present=r.w.ax==0x574d && (r.w.cx&1);
    memset(&r,0,sizeof(r));r.w.ax=7;r.w.dx=639;int386(0x33,&r,&r);
    memset(&r,0,sizeof(r));r.w.ax=8;r.w.dx=479;int386(0x33,&r,&r);
    memset(&r,0,sizeof(r));r.w.ax=0x0b;int386(0x33,&r,&r); /* discard old motion */
}
static void mouse_poll(float dt,int *paused)
{
    union REGS r;
    int buttons,wheel,dx,dy;
    if(!mouse_present) return;
    memset(&r,0,sizeof(r));r.w.ax=3;int386(0x33,&r,&r);
    buttons=r.h.bl&7;wheel=wheel_present?(signed char)r.h.bh:0;
    memset(&r,0,sizeof(r));r.w.ax=0x0b;int386(0x33,&r,&r);
    dx=(short)r.w.cx;dy=(short)r.w.dx;
    if((buttons&7) || wheel)fly_mode(0);
    mouse_apply(dx,dy,buttons,wheel,dt,paused);
}
