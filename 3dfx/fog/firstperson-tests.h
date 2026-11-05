static void firstperson_test_pose(Vec eye,Vec look,int mode)
{
    memset((void *)firstperson_keys,0,sizeof(firstperson_keys));firstperson_extended=firstperson_pause=0;
    firstperson_swallow=0;
    current_camera.eye=eye;current_camera.look=look;flying=0;firstperson_set_mode(mode);
}
static void firstperson_test_step(int count,float dt)
{ int i;for(i=0;i<count;++i){firstperson_look_keys(dt);camera_step(dt);update_camera(dt,0);} }
static int firstperson_irq_check(FILE *log)
{
    int i,made=0,released=0,bios=0,ok;firstperson_keyboard_init();
    outp(0x64,0xd2);outp(0x60,0x11);
    for(i=0;i<100000 && !firstperson_keys[17];++i){}made=firstperson_keys[17]!=0;
    if(kbhit())bios=getch();
    outp(0x64,0xd2);outp(0x60,0x91);
    for(i=0;i<100000 && firstperson_keys[17];++i){}released=firstperson_keys[17]==0;
    firstperson_keyboard_shutdown();ok=made && released && bios=='w';
    if(log)fprintf(log,"Native IRQ1 make/break and BIOS chaining: make %d release %d BIOS %d; %s\n",made,released,bios,ok?"PASS":"FAIL");
    return ok;
}
static int firstperson_tests(FILE *log)
{
    int ok=1,paused=0,i;Camera saved=current_camera,before,after;Vec right,delta;float walk_distance,diagonal;
    int old_flying=flying;
    firstperson_test_pose(vec(0,1.7f,0),vec(0,1.7f,8),0);before=orbit_pose();right=unit(cross(unit(sub(before.look,before.eye)),vec(0,1,0)));
    previous_buttons=0;mouse_apply(20,0,1,0,1/60.0f,&paused);yaw=target_yaw;after=orbit_pose();
    if(dot(unit(sub(after.look,after.eye)),right)<=0)ok=0;
    firstperson_test_pose(vec(0,1.7f,0),vec(0,1.7f,8),1);before=orbit_pose();
    firstperson_mouse(20,0,0);yaw=target_yaw;after=orbit_pose();
    if(dot(unit(sub(after.look,after.eye)),right)<=0 || inertia_yaw!=0)ok=0;
    if(log)fprintf(log,"Mouse-right turns view right in drag and FPS look: %s\n",ok?"PASS":"FAIL");
    firstperson_test_pose(vec(0,1.7f,0),vec(0,1.7f,8),1);firstperson_scan(17);firstperson_test_step(30,1/60.0f);walk_distance=manual_anchor.z;
    firstperson_scan(0x91);firstperson_test_step(60,1/60.0f);if(fabs(firstperson_velocity.z)>.001f)ok=0;
    firstperson_test_pose(vec(0,1.7f,0),vec(0,1.7f,8),1);firstperson_scan(17);firstperson_scan(32);firstperson_test_step(30,1/60.0f);
    diagonal=(float)sqrt(manual_anchor.x*manual_anchor.x+manual_anchor.z*manual_anchor.z);
    if(manual_anchor.x>=0 || manual_anchor.z<=0 || fabs(diagonal-walk_distance)>.02f)ok=0;
    firstperson_test_pose(vec(0,1.7f,0),vec(0,1.7f,8),1);firstperson_scan(17);firstperson_scan(42);firstperson_test_step(30,1/60.0f);
    if(manual_anchor.z<walk_distance*1.9f)ok=0;
    firstperson_scan(0xe0);firstperson_scan(77);if(!firstperson_keys[205])ok=0;firstperson_scan(0xe0);firstperson_scan(0xcd);if(firstperson_keys[205])ok=0;
    if(log)fprintf(log,"Held movement, right strafe, normalized diagonals, Shift and release braking: %s\n",ok?"PASS":"FAIL");
    firstperson_test_pose(vec(0,1.7f,-12.5f),vec(0,1.7f,-20),1);firstperson_scan(17);firstperson_test_step(120,1/60.0f);if(manual_anchor.z> -17 || fabs(manual_anchor.y-firstperson_floor(manual_anchor.x,manual_anchor.z)-1.7f)>.01f)ok=0;
    firstperson_test_pose(vec(1.8f,1.7f,-12.5f),vec(1.8f,1.7f,-20),1);firstperson_scan(17);firstperson_test_step(120,1/60.0f);if(manual_anchor.z< -13.71f)ok=0;
    firstperson_test_pose(vec(1.8f,1.7f,0),vec(5,1.7f,0),1);firstperson_scan(17);firstperson_test_step(60,1/60.0f);if(manual_anchor.x>2.47f)ok=0;
    firstperson_test_pose(vec(-6,1.7f,-20),vec(-6,1.7f,-25),1);firstperson_scan(17);firstperson_test_step(60,1/60.0f);if(manual_anchor.z< -20.9f)ok=0;
    if(log)fprintf(log,"Walking eye height, doorway clearance, wall/pier/grave collisions: %s\n",ok?"PASS":"FAIL");
    firstperson_test_pose(vec(0,5,-10),vec(0,5,-20),2);firstperson_scan(57);firstperson_test_step(60,1/60.0f);if(manual_anchor.y<11)ok=0;
    firstperson_test_pose(vec(0,5,-10),vec(0,5,-20),2);firstperson_scan(29);firstperson_test_step(30,1/60.0f);if(manual_anchor.y>2.2f)ok=0;
    firstperson_test_pose(vec(4,5,-1),vec(10,5,-1),2);firstperson_scan(17);firstperson_test_step(60,1/60.0f);if(manual_anchor.x<10)ok=0;
    firstperson_test_pose(vec(0,5,-10),vec(0,9,-14),2);firstperson_scan(17);firstperson_test_step(60,1/60.0f);if(manual_anchor.y<9)ok=0;
    if(log)fprintf(log,"FPS flight: climb, descend, pitched forward movement and noclip: %s\n",ok?"PASS":"FAIL");
    before=current_camera;firstperson_set_mode(1);update_camera(0,0);delta=sub(current_camera.eye,before.eye);if(dot(delta,delta)>.000001f)ok=0;
    for(i=0;i<8;++i){before=path_at(i*2.0f);current_camera=before;flying=1;firstperson_set_mode(2);update_camera(0,0);delta=sub(current_camera.eye,before.eye);if(dot(delta,delta)>.000001f)ok=0;}
    if(log)fprintf(log,"FPS mode/tour entry preserves the current position: %s\n",ok?"PASS":"FAIL");
    firstperson_test_pose(vec(0,1.7f,0),vec(0,1.7f,8),1);firstperson_scan(32);firstperson_set_mode(0);
    if(!firstperson_key_consumed('d'))ok=0;firstperson_scan(0xa0);if(firstperson_key_consumed('d'))ok=0;
    if(log)fprintf(log,"Held movement does not trigger feature hotkeys on mode exit: %s\n",ok?"PASS":"FAIL");
    {
        unsigned short *image;
        firstperson_test_pose(vec(0,1.7f,6),vec(0,1.7f,-8),1);render(0,1);image=capture("FPSWALK.PPM");if(!image || drawn<50)ok=0;free(image);
        firstperson_test_pose(vec(4,12,-25),vec(0,3,-14),2);render(0,1);image=capture("FPSFLY.PPM");if(!image || drawn<50)ok=0;free(image);
        firstperson_test_pose(vec(0,14,0),vec(0,0,-30),2);render(0,1);image=capture("FPSROOF.PPM");if(!image || drawn<50 || fog_inside)ok=0;free(image);
        if(log)fprintf(log,"Walking and flying native framebuffer captures: %s\n",ok?"PASS":"FAIL");
    }
    if(!firstperson_irq_check(log))ok=0;
    memset((void *)firstperson_keys,0,sizeof(firstperson_keys));firstperson_mode=0;firstperson_velocity=vec(0,0,0);
    firstperson_swallow=0;
    current_camera=saved;flying=1;fly_mode(0);flying=old_flying;transition=1;current_camera=saved;transition_from=saved;
    return ok;
}
