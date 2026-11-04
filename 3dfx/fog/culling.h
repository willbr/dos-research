/* Static bounds for small runs of faces; never reject a partially visible run. */
#define FOG_CHUNK 32
typedef struct { Vec center;float radius; } FogBounds;
static FogBounds fog_bounds[(MAX_FACES+FOG_CHUNK-1)/FOG_CHUNK];
extern int fog_nave_faces,fog_facade_first,fog_facade_last;
static float portal_distance,portal_sign,portal_left,portal_right,portal_bottom,portal_top,portal_radius[4];
static int fog_culling_enabled=1;
static void fog_portal_prepare(void)
{
    float d;
    portal_sign=current_camera.eye.z> -14?-1.0f:1.0f;
    portal_distance=d=(current_camera.eye.z+14)*-portal_sign;
    portal_left=-1.45f-current_camera.eye.x;portal_right=1.45f-current_camera.eye.x;
    portal_bottom=-current_camera.eye.y;portal_top=3.9f-current_camera.eye.y;
    portal_radius[0]=(float)sqrt(d*d+portal_left*portal_left);
    portal_radius[1]=(float)sqrt(d*d+portal_right*portal_right);
    portal_radius[2]=(float)sqrt(d*d+portal_bottom*portal_bottom);
    portal_radius[3]=(float)sqrt(d*d+portal_top*portal_top);
}
static void fog_bounds_init(void)
{
    int first,i,j;
    for(first=0;first<face_count;first+=FOG_CHUNK) {
        Vec lo=vec(1e20f,1e20f,1e20f),hi=vec(-1e20f,-1e20f,-1e20f),delta;
        FogBounds *b=&fog_bounds[first/FOG_CHUNK];
        for(i=first;i<face_count && i<first+FOG_CHUNK;++i)for(j=0;j<3;++j) {
            Vec p=vertices[faces[i].v[j]].p;
            if(p.x<lo.x)lo.x=p.x;if(p.y<lo.y)lo.y=p.y;if(p.z<lo.z)lo.z=p.z;
            if(p.x>hi.x)hi.x=p.x;if(p.y>hi.y)hi.y=p.y;if(p.z>hi.z)hi.z=p.z;
        }
        b->center=mul(add(lo,hi),.5f);delta=sub(hi,lo);b->radius=.5f*(float)sqrt(dot(delta,delta))+.001f;
    }
}
static int fog_chunk_visible(int first,Vec forward,Vec right,Vec up,float bx,float by,float rx,float ry)
{
    FogBounds *b=&fog_bounds[first/FOG_CHUNK];Vec d=sub(b->center,current_camera.eye);
    float z=dot(d,forward),x=dot(d,right),y=dot(d,up),r=b->radius;
    if(!fog_culling_enabled)return 1;
    if(first+FOG_CHUNK<=fog_nave_faces && current_camera.eye.z< -14.5f &&
       fabs(current_camera.eye.x)<5.1f &&
       (first+FOG_CHUNK<=fog_facade_first || first>=fog_facade_last)) {
        /* From the yard within the west facade's width, opaque stone hides the
           nave except through this portal. Inside, retain side-window views. */
        float depth=d.z*portal_sign,distance=portal_distance;
        if(distance*d.x-portal_left*depth < -r*portal_radius[0] ||
           portal_right*depth-distance*d.x < -r*portal_radius[1] ||
           distance*d.y-portal_bottom*depth < -r*portal_radius[2] ||
           portal_top*depth-distance*d.y < -r*portal_radius[3])return 0;
    }
    return z+r>=.45f && z-r<=160 && fabs(x)<=z*bx+r*rx && fabs(y)<=z*by+r*ry;
}
