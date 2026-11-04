/* These meshes use constant per-face lighting. Avoid redundant RGB gradients. */
static FxU32 flat_color[MAX_FACES],flat_palette[MAX_FACES];
static int flat_id[MAX_FACES],flat_next[MAX_FACES],flat_head[MAX_FACES],flat_material[MAX_FACES],flat_count;
static int flat_enabled=1;
static void flat_init(void)
{
    int i,j;
    for(i=0;i<face_count;++i) {
        Face *f=&faces[i];Vertex *v=&vertices[f->v[0]];
        FxU32 color=(FxU32)(v->r+.5f)|((FxU32)(v->g+.5f)<<8)|((FxU32)(v->b+.5f)<<16)|0xff000000UL;
        flat_color[i]=color;
        for(j=0;j<flat_count;++j)if(flat_palette[j]==color && flat_material[j]==f->mat)break;
        if(j==flat_count){flat_palette[j]=color;flat_material[j]=f->mat;++flat_count;}
        flat_id[i]=j;
    }
}
