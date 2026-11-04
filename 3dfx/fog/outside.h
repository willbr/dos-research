/* Original low-poly scenery, inspired by the references, not a site survey. */
int fog_nave_faces,fog_facade_first,fog_facade_last;
static void west_portal(void)
{
    int side,i;float z;
    fog_facade_first=face_count;
    for(side=0;side<2;++side) {
        Vec n=vec(0,0,side?-1:1);z=side?-14.18f:-14;
        quad(vec(-5.1f,0,z),vec(-1.45f,0,z),vec(-1.45f,7.8f,z),vec(-5.1f,7.8f,z),STONE,n);
        quad(vec(1.45f,0,z),vec(5.1f,0,z),vec(5.1f,7.8f,z),vec(1.45f,7.8f,z),STONE,n);
        for(i=0;i<12;++i) {
            float x0=-1.45f+i*2.9f/12,x1=-1.45f+(i+1)*2.9f/12;
            float h0=2.7f+1.2f*(1-(float)fabs(x0)/1.45f),h1=2.7f+1.2f*(1-(float)fabs(x1)/1.45f);
            quad(vec(x0,h0,z),vec(x1,h1,z),vec(x1,7.8f,z),vec(x0,7.8f,z),STONE,n);
            if(!side)quad(vec(x0,h0,-14),vec(x1,h1,-14),vec(x1,h1,-14.18f),vec(x0,h0,-14.18f),TRIM,vec(0,-1,0));
        }
    }
    box(-1.65f,0,-14.25f,-1.45f,2.7f,-13.95f,TRIM);
    box(1.45f,0,-14.25f,1.65f,2.7f,-13.95f,TRIM);
    fog_facade_last=face_count;
}
static void headstone(float x,float z,int style)
{
    int i;float w=.38f,h=.9f+(style%3)*.14f;
    box(x-.52f,0,z-.32f,x+.52f,.12f,z+.32f,STONE);
    if(style%4==0) {
        box(x-.12f,.12f,z-.12f,x+.12f,h+.6f,z+.12f,STONE);
        box(x-.43f,h,z-.12f,x+.43f,h+.22f,z+.12f,STONE);
    } else {
        box(x-w,.12f,z-.12f,x+w,h,z+.12f,STONE);
        for(i=0;i<4;++i) {
            float a=i*3.141593f/4,b=(i+1)*3.141593f/4;
            float x0=x+w*(float)cos(a),x1=x+w*(float)cos(b);
            float y0=h+w*(float)sin(a),y1=h+w*(float)sin(b);
            quad(vec(x0,h,z-.12f),vec(x1,h,z-.12f),vec(x1,y1,z-.12f),vec(x0,y0,z-.12f),STONE,vec(0,0,-1));
            quad(vec(x0,h,z+.12f),vec(x1,h,z+.12f),vec(x1,y1,z+.12f),vec(x0,y0,z+.12f),STONE,vec(0,0,1));
            quad(vec(x0,y0,z-.12f),vec(x1,y1,z-.12f),vec(x1,y1,z+.12f),vec(x0,y0,z+.12f),STONE,vec(0,1,0));
        }
        /* Inset inscription bars, geometry instead of an extra texture. */
        for(i=0;i<3;++i)quad(vec(x-.23f,.43f+i*.14f,z-.135f),vec(x+.23f,.43f+i*.14f,z-.135f),
            vec(x+.23f,.46f+i*.14f,z-.135f),vec(x-.23f,.46f+i*.14f,z-.135f),TRIM,vec(0,0,-1));
    }
    quad(vec(x-.45f,.012f,z+.25f),vec(x+.45f,.012f,z+.25f),vec(x+.45f,.012f,z+1.9f),vec(x-.45f,.012f,z+1.9f),STONE,vec(0,1,0));
}
static void tree(float x,float z,float height,int seed)
{
    int ring,i,start=vertex_count;float radius=height*.32f;
    cylinder(x,z,0,height*.72f,.22f,STONE);
    for(ring=0;ring<3;++ring)for(i=0;i<8;++i) {
        float a=i*6.2831853f/8,b=(i+1)*6.2831853f/8;
        float y0=height*(.45f+ring*.19f),y1=y0+height*.19f;
        float r0=ring==0?radius*.4f:ring==1?radius:radius*.82f,r1=ring==0?radius:ring==1?radius*.82f:0;
        float wobble=1+.12f*(float)sin(i*2.3561945f+seed),next=1+.12f*(float)sin((i+1)*2.3561945f+seed);
        quad(vec(x+r0*wobble*(float)cos(a),y0,z+r0*wobble*(float)sin(a)),
             vec(x+r0*next*(float)cos(b),y0,z+r0*next*(float)sin(b)),
             vec(x+r1*next*(float)cos(b),y1,z+r1*next*(float)sin(b)),
             vec(x+r1*wobble*(float)cos(a),y1,z+r1*wobble*(float)sin(a)),GRASS,vec((float)cos((a+b)*.5f),.35f,(float)sin((a+b)*.5f)));
    }
    for(i=start;i<vertex_count;++i) {
        vertices[i].r*=.56f;vertices[i].g*=.83f;vertices[i].b*=.68f;
    }
}
static void build_graveyard(void)
{
    int i,j,side,start=vertex_count;
    fog_nave_faces=face_count;
    quad(vec(-25,-.025f,-62),vec(25,-.025f,-62),vec(25,-.025f,-14.18f),vec(-25,-.025f,-14.18f),GRASS,vec(0,1,0));
    quad(vec(-1.35f,.005f,-55),vec(1.35f,.005f,-55),vec(1.35f,.005f,-14),vec(-1.35f,.005f,-14),PAVING,vec(0,1,0));
    for(side=-1;side<=1;side+=2) {
        for(j=0;j<5;++j)for(i=0;i<3;++i)headstone(side*(6+i*4.2f),-23-j*7.0f,(i+j*3+(side+1)*2));
        for(j=0;j<5;++j)tree(side*(19+(j%2)*2),-20-j*8.0f,7.2f+(j%3)*1.1f,j+side);
        /* Exterior nave walls and pitched roof make the return view solid. */
        quad(vec(side*5.13f,0,-14.18f),vec(side*5.13f,0,14),vec(side*5.13f,4.55f,14),vec(side*5.13f,4.55f,-14.18f),STONE,vec((float)side,0,0));
        quad(vec(side*5.3f,4.55f,-14.2f),vec(0,8.2f,-14.2f),vec(0,8.2f,14.2f),vec(side*5.3f,4.55f,14.2f),STONE,vec((float)side,1,0));
        box(side<0?-24:23,0,-62,side<0?-23:24,1.1f,-14.2f,STONE);
    }
    for(i=0;i<5;++i)tree(-20+i*10,-59,8+(i%2),i+11);
    box(-24,0,-62,24,1.1f,-61,STONE);
    for(i=start;i<vertex_count;++i) {
        vertices[i].r*=2.6f;vertices[i].g*=3.1f;vertices[i].b*=3.9f;
    }
}
