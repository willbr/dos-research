/* Clip each coplanar quad once, then use the regular native polygon API. */
static int quad_vertices[MAX_FACES][4];
static unsigned char quad_kind[MAX_FACES];
static int quad_enabled=1;
static void quad_init(void)
{
    int i,j,k,group=0;
    for(i=0;i+1<face_count;++i) {
        Face *a=&faces[i],*b=&faces[i+1];int extra=-1,shared=0,q[4];Vec n,d;
        while(i>=fog_groups[group].last)++group;
        if(i+1>=fog_groups[group].last)continue;
        if(a->mat!=b->mat || a->v[0]!=b->v[0])continue;
        for(j=0;j<3;++j){int found=0;for(k=0;k<3;++k)if(b->v[j]==a->v[k])found=1;if(found)++shared;else extra=b->v[j];}
        if(shared!=2 || extra<0)continue;
        n=cross(sub(vertices[a->v[1]].p,vertices[a->v[0]].p),sub(vertices[a->v[2]].p,vertices[a->v[0]].p));
        d=sub(vertices[extra].p,vertices[a->v[0]].p);
        if(fabs(dot(n,d))>.00001f*(float)sqrt(dot(n,n)))continue;
        q[0]=a->v[0];
        if(b->v[1]==a->v[2]){q[1]=a->v[1];q[2]=a->v[2];q[3]=extra;}
        else if(b->v[2]==a->v[1]){q[1]=extra;q[2]=a->v[1];q[3]=a->v[2];}
        else continue;
        for(j=0;j<4;++j)quad_vertices[i][j]=q[j];
        quad_kind[i]=1;quad_kind[i+1]=2;++i;
    }
}
static void quad_draw(Face *face,int *ids)
{
    ViewVertex a[12],b[12];GrVertex q[12];unsigned char mask=0,common=63;int count=4,plane,i,j;
    for(i=0;i<4;++i){unsigned char code=view[ids[i]].code;mask|=code;common&=code;}
    if(common)return;
    if(!mask) {
        grDrawPolygon(4,ids,projected[0]);drawn+=2;
#ifdef LIGHTMAP_DEMO
        lightmap_record(face,&projected[0][ids[0]],&projected[0][ids[1]],&projected[0][ids[2]]);
        lightmap_record(face,&projected[0][ids[0]],&projected[0][ids[2]],&projected[0][ids[3]]);
#endif
        return;
    }
    for(i=0;i<4;++i)a[i]=view[ids[i]];
    for(plane=0;plane<6;++plane)if(mask&(1<<plane)) {
        int n=0;
        for(i=0;i<count;++i) {
            ViewVertex *p=&a[i],*r=&a[(i+1)%count];float dp=plane_distance(p,plane),dr=plane_distance(r,plane);
            if(dp>=0)b[n++]=*p;
            if((dp>=0)!=(dr>=0))interpolate(p,r,dp/(dp-dr),&b[n++]);
        }
        if(n<3)return;count=n;for(i=0;i<count;++i)a[i]=b[i];
    }
    for(i=0;i<count;++i)project(&a[i],&q[i],0,320);
    grDrawPolygonVertexList(count,q);drawn+=count-2;++clipped;
#ifdef LIGHTMAP_DEMO
    for(j=1;j<count-1;++j)lightmap_record(face,&q[0],&q[j],&q[j+1]);
#endif
}
