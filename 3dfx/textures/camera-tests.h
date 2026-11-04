static int safe_drone(Camera p)
{
    Vec d=sub(p.look,p.eye);
    if(p.eye.x!=p.eye.x || p.eye.y!=p.eye.y || p.eye.z!=p.eye.z || dot(d,d)<.01f || p.eye.y<5)return 0;
    if(fabs(p.eye.x)<3.7f && p.eye.z>-22.5f && p.eye.z<22.8f && p.eye.y<8.5f)return 0;
    if(fabs(p.eye.x)<7.8f && p.eye.z>-.5f && p.eye.z<6.4f && p.eye.y<8.5f)return 0;
    if(fabs(p.eye.x)<2.2f && p.eye.z>-.3f && p.eye.z<4.3f && p.eye.y<11.5f)return 0;
    return 1;
}
static int camera_switch_test(void)
{
    Vec old,delta;float seconds;int i,ok=1;
    current_camera=orbit_pose();old=current_camera.eye;fly_mode(1);update_camera(0,0);
    delta=sub(current_camera.eye,old);if(dot(delta,delta)>.000001f)ok=0;
    for(i=0;i<100;++i)update_camera(1/60.0f,0);
    old=current_camera.eye;seconds=drone_seconds;update_camera(1/60.0f,1);
    delta=sub(current_camera.eye,old);if(dot(delta,delta)>.000001f || seconds!=drone_seconds)ok=0;
    fly_mode(0);update_camera(0,1);delta=sub(current_camera.eye,old);if(dot(delta,delta)>.000001f)ok=0;
    yaw=target_yaw=-.6f;pitch=target_pitch=-.12f;zoom=target_zoom=1;
    yaw_speed=pitch_speed=zoom_speed=inertia_yaw=inertia_pitch=0;
    flying=0;transition=1;current_camera=orbit_pose();transition_from=current_camera;return ok;
}
