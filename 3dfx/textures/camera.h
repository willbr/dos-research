typedef struct { Vec eye,look; } Camera;
typedef struct { Vec eye,look; } Shot;
static const Shot shots[8]={
    {{-13,8,-35},{0,4,-18}},{{-12,10,-14},{0,4,-9}},
    {{-14,12,8},{0,6,3}},{{-9,16,25},{0,4,15}},
    {{14,14,23},{0,5,10}},{{16,10,-3},{0,5,0}},
    {{12,8,-25},{0,4,-18}},{{25,24,-40},{0,3,-2}}
};
#define PATH_SAMPLES 2048
#define FLIGHT_SECONDS 80.0f
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
    int i=(int)floor(u),a=(i+7)&7,b=i&7,c=(i+1)&7,d=(i+2)&7;
    Camera p;float t=u-i;
    p.eye=catmull(shots[a].eye,shots[b].eye,shots[c].eye,shots[d].eye,t);
    p.look=catmull(shots[a].look,shots[b].look,shots[c].look,shots[d].look,t);return p;
}
static void path_init(void)
{
    Camera last=path_at(0);int i;path_distance[0]=0;path_eye[0]=last.eye;
    for(i=1;i<=PATH_SAMPLES;++i) {
        Camera p=path_at(i*(8.0f/PATH_SAMPLES));Vec delta=sub(p.eye,last.eye);
        path_distance[i]=path_distance[i-1]+(float)sqrt(dot(delta,delta));path_eye[i]=p.eye;last=p;
    }
}
static Camera drone_pose(float seconds)
{
    float phase=seconds/FLIGHT_SECONDS,distance,t;int lo=0,hi=PATH_SAMPLES;
    phase-=(float)floor(phase);distance=phase*path_distance[PATH_SAMPLES];
    while(hi-lo>1){int m=(lo+hi)/2;if(path_distance[m]<distance)lo=m;else hi=m;}
    t=(distance-path_distance[lo])/(path_distance[hi]-path_distance[lo]);
    return path_at((lo+t)*(8.0f/PATH_SAMPLES));
}
static Camera orbit_pose(void)
{
    Camera p;float elevation=.55f+pitch*.65f,radius=58/zoom,flat=radius*(float)cos(elevation);
    p.look=vec(0,3.2f,-2);
    p.eye=add(p.look,vec((float)sin(yaw)*flat,(float)sin(elevation)*radius,-(float)cos(yaw)*flat));return p;
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
        Vec d=sub(current_camera.eye,vec(0,3.2f,-2));float radius=(float)sqrt(dot(d,d));
        yaw=target_yaw=(float)atan2(d.x,-d.z);
        pitch=target_pitch=clamp(((float)asin(d.y/radius)-.55f)/.65f,-.55f,.45f);
        zoom=target_zoom=clamp(58/radius,.65f,1.3f);
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
