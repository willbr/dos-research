/* Integration check and AA submission benchmark for prebuilt fitting LODs. */
static int run_lod_test(FILE *log)
{
    unsigned short *near_image=NULL,*far_image=NULL;
    Vertex *near_mesh=NULL,*far_mesh=NULL;
    int phase,i,ok=1,near_faces=0,near_vertices=0;
    unsigned long changed=0,lit=0;
    double near_work=0,far_work=0;
    automatic_lod=0;detailed=0;model_detail(0);experiment_flags=64;profiler.measure=1;
    for(phase=0;phase<2;++phase) {
        double work=0,aa=0,transform=0;
        model_distance(phase);if(!build_helmet())return 0;
        if(!phase) { near_mesh=vertices;near_faces=face_count;near_vertices=vertex_count; }
        else { far_mesh=vertices;if(face_count>=near_faces || vertex_count>=near_vertices)ok=0; }
        zoom=.72f;render(60,0);render(60,0);profile_reset();
        for(i=0;i<240;++i) {
            int slot;
            yaw=-.6f+i*(6.28318530718f/240);pitch=-.05f+.45f*(float)sin(i*(6.28318530718f/240));
            render(60,0);slot=(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY;
            work+=profiler.frame[slot]-profiler.stage[slot][P_PRESENT];
            aa+=profiler.stage[slot][P_AA];transform+=profiler.stage[slot][P_TRANSFORM];
        }
        if(log)fprintf(log,"Fitting LOD %s: %d vertices, %d faces; 240 moving AA frames at zoom .72; work %.4f / AA %.4f / transform %.4f ms\n",phase?"FAR":"NEAR",vertex_count,face_count,work/240,aa/240,transform/240);
        if(phase)far_work=work;else near_work=work;
        yaw=-.6f;pitch=-.12f;render(60,1);
        if(phase)far_image=capture("FAR.PPM");else near_image=capture("NEAR.PPM");
    }
    if(!near_image || !far_image)ok=0;
    else for(i=90*640;i<410*640;++i) {
        if(i%640<80 || i%640>560)continue;
        if(near_image[i]!=far_image[i])++changed;
        if(((far_image[i]>>11)&31)>8 || ((far_image[i]>>5)&63)>16)++lit;
    }
    if(lit<15000 || changed==0 || far_work>=near_work)ok=0;
    for(i=0;i<1000;++i) {
        model_distance(i&1);if(!build_helmet() || vertices!=((i&1)?far_mesh:near_mesh))ok=0;
    }
    /* Hysteresis: .84 enters distant LOD; .92 returns to the near mesh. */
    zoom=.80f;if(!select_view_mesh() || !model_distant)ok=0;
    zoom=.88f;if(!select_view_mesh() || !model_distant)ok=0;
    zoom=.96f;if(!select_view_mesh() || model_distant)ok=0;
    zoom=.88f;if(!select_view_mesh() || model_distant)ok=0;
    if(log)fprintf(log,"LOD captures: %lu changed pixels, %lu lit pixels; 1000 cached switches + hysteresis: %s\n",changed,lit,ok?"PASS":"FAIL");
    free(near_image);free(far_image);automatic_lod=1;zoom=1;return ok;
}
