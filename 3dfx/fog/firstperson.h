/* Held make/break states, chained to the normal BIOS keyboard handler. */
static volatile unsigned char firstperson_keys[256];
static volatile int firstperson_extended,firstperson_pause;
static volatile unsigned char firstperson_swallow;
static void (__interrupt __far *firstperson_old_irq)(void);
static int firstperson_keyboard_installed,firstperson_quick;
static Vec firstperson_velocity;
static void firstperson_scan(unsigned char scan)
{
    int code;
    if(firstperson_pause){--firstperson_pause;return;}
    if(scan==0xe1){firstperson_pause=5;return;}
    if(scan==0xe0){firstperson_extended=128;return;}
    code=(scan&127)+firstperson_extended;firstperson_extended=0;firstperson_keys[code]=(scan&128)?0:1;
    if(scan&128) {
        if(code==17)firstperson_swallow&=~1;
        if(code==30)firstperson_swallow&=~2;
        if(code==31)firstperson_swallow&=~4;
        if(code==32)firstperson_swallow&=~8;
        if(code==57)firstperson_swallow&=~16;
        if(code==16)firstperson_swallow&=~32;
        if(code==18)firstperson_swallow&=~64;
    }
}
static void firstperson_release_guard(void)
{
    _disable();
    if(firstperson_keys[17])firstperson_swallow|=1;
    if(firstperson_keys[30])firstperson_swallow|=2;
    if(firstperson_keys[31])firstperson_swallow|=4;
    if(firstperson_keys[32])firstperson_swallow|=8;
    if(firstperson_keys[57])firstperson_swallow|=16;
    if(firstperson_mode==2 && firstperson_keys[16])firstperson_swallow|=32;
    if(firstperson_mode==2 && firstperson_keys[18])firstperson_swallow|=64;
    _enable();
}
static int firstperson_key_consumed(int key)
{
    int mask=key=='w'||key=='W'?1:key=='a'||key=='A'?2:key=='s'||key=='S'?4:key=='d'||key=='D'?8:key==' '?16:key=='q'||key=='Q'?32:key=='e'||key=='E'?64:0;
    return mask && ((mask<32?firstperson_mode:firstperson_mode==2) || (firstperson_swallow&mask));
}
static void __interrupt __far firstperson_irq(void)
{ firstperson_scan((unsigned char)inp(0x60));_chain_intr(firstperson_old_irq); }
static void firstperson_keyboard_init(void)
{
    if(firstperson_keyboard_installed)return;
    memset((void *)firstperson_keys,0,sizeof(firstperson_keys));firstperson_extended=firstperson_pause=0;
    firstperson_swallow=0;
    firstperson_old_irq=_dos_getvect(9);_dos_setvect(9,firstperson_irq);firstperson_keyboard_installed=1;
}
static void firstperson_keyboard_shutdown(void)
{
    if(firstperson_keyboard_installed){_dos_setvect(9,firstperson_old_irq);firstperson_keyboard_installed=0;}
}
static int firstperson_walk_valid(float x,float z)
{
    int side,i,j;
    if(z>13.65f || z< -60.65f || fabs(x)>22.65f)return 0;
    if(z> -13.7f && fabs(x)>4.85f)return 0;
    if(z>=-14.5f && z<=-13.7f && fabs(x)>1.15f)return 0;
    if(z> -14) {
        for(side=-1;side<=1;side+=2)for(j=0;j<7;++j)
            if(fabs(x-side*3.1f)<.64f && fabs(z-(-12+j*4))<.64f)return 0;
    } else {
        for(side=-1;side<=1;side+=2) {
            for(j=0;j<5;++j)for(i=0;i<3;++i)
                if(fabs(x-side*(6+i*4.2f))<.76f && z> -23-j*7-.57f && z< -23-j*7+2.14f)return 0;
            for(j=0;j<5;++j) {
                float dx=x-side*(19+(j%2)*2),dz=z-(-20-j*8);
                if(dx*dx+dz*dz<.25f)return 0;
            }
        }
        for(i=0;i<5;++i){float dx=x-(-20+i*10),dz=z+59;if(dx*dx+dz*dz<.25f)return 0;}
    }
    return 1;
}
static float firstperson_floor(float x,float z)
{ return z>=-14?0:(fabs(x)<=1.35f && z>=-55)?.005f:-.025f; }
static void firstperson_set_mode(int mode)
{
    int i;Vec original;
    if(mode!=firstperson_mode && firstperson_mode)firstperson_release_guard();
    firstperson_mode=mode;if(flying)fly_mode(0);
    else {
        /* Catch the actual view, including any unfinished camera transition. */
        flying=1;fly_mode(0);
    }
    original=manual_anchor;
    if(mode==1 && !firstperson_walk_valid(manual_anchor.x,manual_anchor.z)) {
        for(i=0;i<32;++i) {
            float a=(i%8)*.785398163f,r=.5f*(1+i/8);
            float x=original.x+r*(float)cos(a),z=original.z+r*(float)sin(a);
            if(firstperson_walk_valid(x,z)){manual_anchor.x=x;manual_anchor.z=z;break;}
        }
        if(i==32)manual_anchor=vec(0,1.7f,6);
    }
    firstperson_mode=mode;firstperson_velocity=vec(0,0,0);transition=1;
    current_camera=orbit_pose();transition_from=current_camera;
}
static void firstperson_look_keys(float dt)
{
    if(!firstperson_mode)return;
    if(firstperson_keys[128+75])target_yaw+=1.5f*dt;
    if(firstperson_keys[128+77])target_yaw-=1.5f*dt;
    if(firstperson_keys[128+72])target_pitch-=.9f*dt;
    if(firstperson_keys[128+80])target_pitch+=.9f*dt;
    target_pitch=clamp(target_pitch,LOOK_PITCH_MIN,LOOK_PITCH_MAX);
}
static void firstperson_move(float dt)
{
    float heading=yaw+.6f,elevation=-pitch*1.5f-.1f;
    float forward=(float)firstperson_keys[17]-firstperson_keys[31],side=(float)firstperson_keys[32]-firstperson_keys[30];
    float vertical=firstperson_mode==2?(float)firstperson_keys[18]-firstperson_keys[16]:0;
    float speed=(firstperson_mode==1?3.0f:7.0f)*((firstperson_keys[42] || firstperson_keys[54])?2:1),length,blend;
    float cp=firstperson_mode==2?(float)cos(elevation):1;
    Vec direction=vec((float)sin(heading)*forward*cp-(float)cos(heading)*side,
        firstperson_mode==2?(float)sin(elevation)*forward+vertical:0,
        (float)cos(heading)*forward*cp+(float)sin(heading)*side),step;
    int i,count;
    if(dt<=0)return;dt=clamp(dt,.001f,.1f);
    length=(float)sqrt(dot(direction,direction));if(length>1)direction=mul(direction,1/length);
    blend=1-(float)exp(-12*dt);
    firstperson_velocity=add(mul(firstperson_velocity,1-blend),mul(direction,speed*blend));step=mul(firstperson_velocity,dt);
    if(firstperson_mode==2) {
        manual_anchor=add(manual_anchor,step);manual_anchor.x=clamp(manual_anchor.x,-80,80);
        manual_anchor.z=clamp(manual_anchor.z,-100,50);manual_anchor.y=clamp(manual_anchor.y,.3f,45);
    } else {
        length=(float)sqrt(step.x*step.x+step.z*step.z);count=(int)(length/.08f)+1;step=mul(step,1.0f/count);
        for(i=0;i<count;++i) {
            if(firstperson_walk_valid(manual_anchor.x+step.x,manual_anchor.z))manual_anchor.x+=step.x;else firstperson_velocity.x=0;
            if(firstperson_walk_valid(manual_anchor.x,manual_anchor.z+step.z))manual_anchor.z+=step.z;else firstperson_velocity.z=0;
        }
        manual_anchor.y+=(firstperson_floor(manual_anchor.x,manual_anchor.z)+1.7f-manual_anchor.y)*blend;
    }
}
