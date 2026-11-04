/* Hard edges retain separate UV/colors; coincident positions transform once. */
typedef struct { float x,y,z,sx,sy,inv,depth;unsigned long stamp;unsigned char code; } PositionCache;
static PositionCache position_cache[MAX_VERTS];
static int position_id[MAX_VERTS],position_hash[32768],position_count;
static unsigned long position_frame;
static void position_init(void)
{
    int i;memset(position_hash,255,sizeof(position_hash));
    for(i=0;i<vertex_count;++i) {
        union { float f;unsigned long u; } x,y,z;unsigned long slot;int old;
        x.f=vertices[i].p.x;y.f=vertices[i].p.y;z.f=vertices[i].p.z;
        slot=((x.u*73856093UL)^(y.u*19349663UL)^(z.u*83492791UL));slot=(slot^(slot>>16))&32767;
        for(;;) {
            old=position_hash[slot];
            if(old<0){position_hash[slot]=i;position_id[i]=i;++position_count;break;}
            if(vertices[old].p.x==x.f && vertices[old].p.y==y.f && vertices[old].p.z==z.f){position_id[i]=old;break;}
            slot=(slot+1)&32767;
        }
    }
}
static void position_transform(Vec forward,Vec right,Vec up,float bx,float by)
{
    int n;float ex=current_camera.eye.x,ey=current_camera.eye.y,ez=current_camera.eye.z;
    if(++position_frame==0){memset(position_cache,0,sizeof(position_cache));position_frame=1;}
    for(n=0;n<active_count;++n) {
        int i=active_vertices[n];
        PositionCache *c=&position_cache[position_id[i]];ViewVertex *v=&view[i];Vertex *s=&vertices[i];
        if(c->stamp!=position_frame) {
            float x=s->p.x-ex,y=s->p.y-ey,z=s->p.z-ez,xb,yb;unsigned char code=0;
            c->stamp=position_frame;
            c->x=x*right.x+y*right.y+z*right.z;c->y=x*up.x+y*up.y+z*up.z;c->z=x*forward.x+y*forward.y+z*forward.z;
            xb=c->z*bx;yb=c->z*by;
            if(c->z<.45f)code|=1;if(c->z>160)code|=2;
            if(c->x< -xb)code|=4;if(c->x>xb)code|=8;if(c->y< -yb)code|=16;if(c->y>yb)code|=32;c->code=code;
            if(!code){c->inv=1/c->z;c->sx=320+c->x*focal*c->inv;c->sy=260-c->y*focal*c->inv;c->depth=65535*.45f*c->inv;}
        }
        v->code=c->code;
        if(!c->code) {
            GrVertex *p=&projected[0][i];p->x=c->sx;p->y=c->sy;p->oow=c->inv;p->ooz=c->depth;
            p->tmuvtx[0].sow=s->u*c->inv;p->tmuvtx[0].tow=s->v*c->inv;p->tmuvtx[0].oow=c->inv;
        }
    }
}
static void cached_view(int index,ViewVertex *v)
{
    PositionCache *p=&position_cache[position_id[index]];
    *v=view[index];v->x=p->x;v->y=p->y;v->z=p->z;
}
