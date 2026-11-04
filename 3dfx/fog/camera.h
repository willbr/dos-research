typedef struct { Vec eye,look; } Camera;
typedef struct { Vec eye,look; } Shot;
#define PATH_KEYS 16
static const Shot shots[PATH_KEYS]={
 {{0,1.9f,6},{0,2,-14}}, {{0,1.9f,-2},{0,2,-18}},
 {{0,1.9f,-10},{0,2,-23}}, {{0,1.9f,-16},{0,2,-30}},
#ifdef REFLECTION_DEMO
 {{0,2.2f,-24},{-5.8f,.2f,-28}}, {{-3,2.8f,-33},{-10,3,-43}},
#else
 {{0,2.2f,-24},{-5,2,-36}}, {{-3,2.8f,-33},{-10,3,-43}},
#endif
 {{-4,3.5f,-44},{0,3,-52}}, {{0,4.1f,-49},{8,3,-43}},
 {{4,3.5f,-44},{8,2.5f,-31}}, {{3,2.8f,-33},{0,2,-20}},
#ifdef REFLECTION_DEMO
 {{0,2.2f,-24},{-5.8f,.2f,-28}}, {{0,1.9f,-16},{0,3,0}},
#else
 {{0,2.2f,-24},{0,2,-14}}, {{0,1.9f,-16},{0,3,0}},
#endif
 {{0,1.9f,-10},{0,4,12}}, {{0,2.1f,-2},{0,4,14}},
 {{0,2.5f,7},{1.5f,4,14}}, {{.7f,2.3f,10},{3,3,4}}
};
#define PATH_SAMPLES 2048
#define FLIGHT_SECONDS 100.0f
static float path_distance[PATH_SAMPLES+1],drone_seconds,transition=1;
static Vec path_eye[PATH_SAMPLES+1];
static int flying;
static Camera current_camera,transition_from;
static Vec catmull(Vec a,Vec b,Vec c,Vec d,float t)
{
    float t2=t*t,t3=t2*t;
    return add(add(mul(a,(-t+2*t2-t3)*.5f),mul(b,1-2.5f*t2+1.5f*t3)),
               add(mul(c,(t+4*t2-3*t3)*.5f),mul(d,(-t2+t3)*.5f)));
}
static Camera path_at(float u)
{
    int i=(int)floor(u),a=(i+PATH_KEYS-1)%PATH_KEYS,b=i%PATH_KEYS,c=(i+1)%PATH_KEYS,d=(i+2)%PATH_KEYS;
    Camera p;float t=u-i;
    p.eye=catmull(shots[a].eye,shots[b].eye,shots[c].eye,shots[d].eye,t);
    p.look=catmull(shots[a].look,shots[b].look,shots[c].look,shots[d].look,t);return p;
}
static void path_init(void)
{
    Camera last=path_at(0);int i;path_distance[0]=0;path_eye[0]=last.eye;
    for(i=1;i<=PATH_SAMPLES;++i) {
        Camera p=path_at(i*((float)PATH_KEYS/PATH_SAMPLES));Vec delta=sub(p.eye,last.eye);
        path_distance[i]=path_distance[i-1]+(float)sqrt(dot(delta,delta));path_eye[i]=p.eye;last=p;
    }
}
static Camera drone_pose(float seconds)
{
    float phase=seconds/FLIGHT_SECONDS,distance,t;int lo=0,hi=PATH_SAMPLES;
    phase-=(float)floor(phase);distance=phase*path_distance[PATH_SAMPLES];
    while(hi-lo>1){int m=(lo+hi)/2;if(path_distance[m]<distance)lo=m;else hi=m;}
    t=(distance-path_distance[lo])/(path_distance[hi]-path_distance[lo]);
    return path_at((lo+t)*((float)PATH_KEYS/PATH_SAMPLES));
}
static Vec manual_anchor={0,1.9f,6};
static void camera_home(void) { manual_anchor=vec(0,1.9f,6); }
static Camera orbit_pose(void)
{
    Camera p;float heading=yaw+.6f,dx=(float)sin(heading),dz=(float)cos(heading);
    float elevation=-pitch*1.5f-.1f,flat=8*(float)cos(elevation);
    p.eye=add(manual_anchor,vec(dx*(zoom-1)*8,0,dz*(zoom-1)*8));
    /* Manual look preserves the current position when leaving the tour. */
    p.look=add(p.eye,vec(dx*flat,8*(float)sin(elevation),dz*flat));return p;
}
static void fly_mode(int enabled)
{
    int i;float closest=1e30f;
    if(enabled==flying)return;
    transition_from=current_camera;transition=0;
    if(enabled) {
        for(i=0;i<PATH_SAMPLES;i+=4) {
            float x=path_eye[i].x-current_camera.eye.x,y=path_eye[i].y-current_camera.eye.y,z=path_eye[i].z-current_camera.eye.z;
            float distance=x*x+y*y+z*z;
            if(distance<closest){closest=distance;drone_seconds=FLIGHT_SECONDS*path_distance[i]/path_distance[PATH_SAMPLES];}
        }
    } else {
        Vec d=sub(current_camera.look,current_camera.eye);float flat=(float)sqrt(d.x*d.x+d.z*d.z);
        manual_anchor=current_camera.eye;
        yaw=target_yaw=(float)atan2(d.x,d.z)-.6f;
        pitch=target_pitch=clamp(-((float)atan2(d.y,flat)+.1f)/1.5f,-.55f,.45f);
        zoom=target_zoom=1;
        yaw_speed=pitch_speed=zoom_speed=inertia_yaw=inertia_pitch=0;
    }
    flying=enabled;
}
static void update_camera(float dt,int paused)
{
    Camera desired;float t;
    if(flying && !paused)drone_seconds+=dt;
    desired=flying?drone_pose(drone_seconds):orbit_pose();
    transition=clamp(transition+dt/1.5f,0,1);t=transition*transition*(3-2*transition);
    current_camera.eye=add(mul(transition_from.eye,1-t),mul(desired.eye,t));
    current_camera.look=add(mul(transition_from.look,1-t),mul(desired.look,t));
}
