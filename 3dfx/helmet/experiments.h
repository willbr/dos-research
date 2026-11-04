/* Moving-camera experiments; opt-in via /optbench, never affect normal runs.
   Each variant shares the reference topology/culling and Glide draw paths. */
#define OPT_SHARED 1
#define OPT_SPEC 2
#define OPT_COMPACT 4
#define OPT_TRIG 8
#define OPT_ASM 16
#define SPEC_SIZE 1024
#define TRIG_SIZE 2048
static float normal_lut[MAX_VERTS][3],normal_diff[MAX_VERTS],normal_shine[MAX_VERTS];
static int normal_material[MAX_VERTS],normal_index[MAX_VERTS],normal_count;
static float pos_stream[MAX_VERTS][3],uv_stream[MAX_VERTS][2];
static unsigned char material_stream[MAX_VERTS];
static float spec_lut[2][SPEC_SIZE+1],trig_lut[TRIG_SIZE+1];
static unsigned char ref_visible[MAX_FACES],ref_edges[MAX_FACES];

/* Inline x87 projection using the same float inputs as the compact C path.
   No SSE/MMX or reciprocal approximation. Stack empty on entry and exit. */
static void project_x87(const float *p,const float *matrix,float *out);
#pragma aux project_x87 = \
    "fld dword ptr [esi]" "fmul dword ptr [ebx]" \
    "fld dword ptr [esi+8]" "fmul dword ptr [ebx+4]" "faddp st(1),st" "fstp dword ptr [edi]" \
    "fld dword ptr [esi]" "fmul dword ptr [ebx+8]" \
    "fld dword ptr [esi+4]" "fmul dword ptr [ebx+12]" "faddp st(1),st" \
    "fld dword ptr [esi+8]" "fmul dword ptr [ebx+16]" "faddp st(1),st" "fstp dword ptr [edi+4]" \
    "fld dword ptr [esi]" "fmul dword ptr [ebx+20]" \
    "fld dword ptr [esi+4]" "fmul dword ptr [ebx+24]" "faddp st(1),st" \
    "fld dword ptr [esi+8]" "fmul dword ptr [ebx+28]" "faddp st(1),st" \
    "fsubr dword ptr [ebx+32]" "fdivr dword ptr [ebx+36]" "fstp dword ptr [edi+8]" \
    parm [esi] [ebx] [edi] modify exact [];

static void experiments_init(void)
{
    int i,j;normal_count=0;
    for(i=0;i<=SPEC_SIZE;++i) {
        float h=(float)i/SPEC_SIZE,h2=h*h,h4=h2*h2,h8=h4*h4;
        spec_lut[0][i]=h8*h8;spec_lut[1][i]=spec_lut[0][i]*spec_lut[0][i];
    }
    for(i=0;i<=TRIG_SIZE;++i)trig_lut[i]=(float)sin(i*(6.283185307179586/TRIG_SIZE));
    for(i=0;i<vertex_count;++i) {
        Vertex *v=&vertices[i];
        pos_stream[i][0]=v->p.x;pos_stream[i][1]=v->p.y-.58f;pos_stream[i][2]=v->p.z;
        uv_stream[i][0]=v->u;uv_stream[i][1]=v->v;material_stream[i]=(unsigned char)v->mat;
        /* Exact equality: share normals without quantizing their direction. */
        for(j=0;j<normal_count;++j)
            if(normal_material[j]==v->mat && normal_lut[j][0]==v->n.x && normal_lut[j][1]==v->n.y && normal_lut[j][2]==v->n.z)break;
        if(j==normal_count) {
            normal_lut[j][0]=v->n.x;normal_lut[j][1]=v->n.y;normal_lut[j][2]=v->n.z;
            normal_material[j]=v->mat;++normal_count;
        }
        normal_index[i]=j;
    }
}
static float lookup_spec(float h,int mat)
{
    float x=h*SPEC_SIZE,t;int i;
    if(x>=SPEC_SIZE)return 1;
    if(x<=0)return 0;
    i=(int)x;t=x-i;
    return spec_lut[mat==GLASS?0:1][i]*(1-t)+spec_lut[mat==GLASS?0:1][i+1]*t;
}
static float lookup_sin(float angle)
{
    float x=angle*(TRIG_SIZE/6.283185307179586f),t;int i=(int)floor(x);
    t=x-i;i&=TRIG_SIZE-1;
    return trig_lut[i]*(1-t)+trig_lut[i+1]*t;
}
/* Watcom leaves even __inline helpers with these parameters out of line.
   Expand this arithmetic so the benchmark does not measure 12 argument
   pushes and a function call per vertex. Inputs have no side effects. */
#define lighting(nx,ny,nz,mat,lx,ly,lz,hx,hy,hz,diff_out,spec_out) do { \
    float ld=(nx)*(lx)+(ny)*(ly)+(nz)*(lz); \
    float lh=(nx)*(hx)+(ny)*(hy)+(nz)*(hz),lh2,lh4,lh8,lh16; \
    if(ld<0)ld=0;*(diff_out)=.34f+.66f*ld;if(lh<0)lh=0; \
    if(EXP_FLAGS&OPT_SPEC)lh16=lookup_spec(lh,mat); \
    else { lh2=lh*lh;lh4=lh2*lh2;lh8=lh4*lh4;lh16=lh8*lh8;if((mat)!=GLASS)lh16*=lh16; } \
    *(spec_out)=lh16*specular[mat]*255; \
} while(0)
#define EXP_FLAGS (0)
#define EXP_FUNCTION prepare_control
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

#define EXP_FLAGS (OPT_SHARED)
#define EXP_FUNCTION prepare_shared
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

#define EXP_FLAGS (OPT_SPEC)
#define EXP_FUNCTION prepare_spec
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

#define EXP_FLAGS (OPT_COMPACT)
#define EXP_FUNCTION prepare_compact
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

#define EXP_FLAGS (OPT_TRIG)
#define EXP_FUNCTION prepare_trig
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

#define EXP_FLAGS (OPT_COMPACT|OPT_ASM)
#define EXP_FUNCTION prepare_asm
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

#define EXP_FLAGS (OPT_SHARED|OPT_COMPACT)
#define EXP_FUNCTION prepare_shared_compact
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

#define EXP_FLAGS (OPT_SHARED|OPT_COMPACT|OPT_SPEC)
#define EXP_FUNCTION prepare_shared_lut
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

#define EXP_FLAGS (OPT_SHARED|OPT_COMPACT|OPT_ASM)
#define EXP_FUNCTION prepare_shared_asm
#include "experiment-kernel.h"
#undef EXP_FUNCTION
#undef EXP_FLAGS

static void prepare_experimental(void)
{
    switch(experiment_flags) {
    case OPT_SHARED:prepare_shared();break;
    case OPT_SPEC:prepare_spec();break;
    case OPT_COMPACT:prepare_compact();break;
    case OPT_TRIG:prepare_trig();break;
    case OPT_COMPACT|OPT_ASM:prepare_asm();break;
    case OPT_SHARED|OPT_COMPACT:prepare_shared_compact();break;
    case OPT_SHARED|OPT_COMPACT|OPT_SPEC:prepare_shared_lut();break;
    case OPT_SHARED|OPT_COMPACT|OPT_ASM:prepare_shared_asm();break;
    default:prepare_control();break;
    }
}

#undef lighting
static void experiment_pose(int i,int count)
{
    yaw=-.6f+i*(6.28318530718f/count);
    pitch=-.05f+.45f*(float)sin(i*(6.28318530718f/count));
    zoom=1.15f+.15f*(float)cos(i*(6.28318530718f/count));
}
static int run_experiments(FILE *unused,int shared_only)
{
    /* 32 routes through the experimental kernel with no optimizations, to
       expose dispatcher/helper overhead separately from the original C. */
    static const int flags[]={0,32,OPT_SHARED,OPT_SPEC,OPT_COMPACT,OPT_TRIG,OPT_COMPACT|OPT_ASM,OPT_SHARED|OPT_COMPACT,OPT_SHARED|OPT_COMPACT|OPT_SPEC,OPT_SHARED|OPT_COMPACT|OPT_ASM};
    static const char *names[]={"Reference C","Experimental C control","Shared normals","Specular LUT 1024","Compact streams","Trig LUT 2048","Compact + x87","Shared + compact","Shared + compact + LUT","Shared + compact + x87"};
    GrVertex *reference=malloc(sizeof(projected));float *ref_shine=malloc(sizeof(shine));
    const char *logname=shared_only?"SHARED.LOG":"OPTBENCH.LOG";
    FILE *f=fopen(logname,"w");int quality,k,rep,i,j,ok=1;
    (void)unused;
    if(!f || !reference || !ref_shine || !profiler.available) { if(f)fclose(f);free(reference);free(ref_shine);return 0; }
    fprintf(f,"Moving camera, AA on, textures/highlights on, profiler hidden. Timer: emulated ms.\n");
    for(quality=0;quality<2;++quality) {
        detailed=quality;model_detail(quality);model_distance(0);if(!build_helmet()){ok=0;break;}experiments_init();
        fprintf(f,"\nQUALITY %s: %d vertices, %d faces, %d exact normal/material groups\n",quality?"DETAIL":"FAST",vertex_count,face_count,normal_count);fflush(f);
        for(k=0;k<(shared_only?2:(int)(sizeof(flags)/sizeof(flags[0])));++k) {
            int selected=shared_only?(k?64:0):flags[k];
            const char *name=shared_only?(k?"Production shared C":"Reference C"):names[k];
            float max_xy=0,max_rgb=0,max_spec=0,max_uv=0;
            double sums[3]={0,0,0},frame=0,work=0,transform=0;int mismatches=0;
            for(i=0;i<24;++i) {
                int visible,silhouettes;
                experiment_pose(i,24);experiment_flags=0;prepare();visible=visible_count;silhouettes=silhouette_count;
                memcpy(reference,projected,sizeof(GrVertex)*vertex_count);memcpy(ref_shine,shine,sizeof(float)*vertex_count);
                for(j=0;j<face_count;++j) { ref_visible[j]=(unsigned char)faces[j].visible;ref_edges[j]=aa_edges[j]; }
                experiment_flags=selected;prepare();if(visible!=visible_count || silhouettes!=silhouette_count)++mismatches;
                for(j=0;j<face_count;++j)if(ref_visible[j]!=faces[j].visible || (faces[j].visible && ref_edges[j]!=aa_edges[j]))++mismatches;
                for(j=0;j<vertex_count;++j) {
                    GrVertex *a=&reference[j],*b=&projected[j];float e;
                    if(b->x!=b->x || b->y!=b->y || b->r!=b->r || b->g!=b->g || b->b!=b->b || shine[j]!=shine[j] || b->oow!=b->oow || b->oow<=0)++mismatches;
                    e=(float)fabs(a->x-b->x);if(e>max_xy)max_xy=e;e=(float)fabs(a->y-b->y);if(e>max_xy)max_xy=e;
                    e=(float)fabs(a->r-b->r);if(e>max_rgb)max_rgb=e;e=(float)fabs(a->g-b->g);if(e>max_rgb)max_rgb=e;e=(float)fabs(a->b-b->b);if(e>max_rgb)max_rgb=e;
                    e=(float)fabs(ref_shine[j]-shine[j]);if(e>max_spec)max_spec=e;
                    e=(float)fabs(a->tmuvtx[0].sow-b->tmuvtx[0].sow);if(e>max_uv)max_uv=e;
                    e=(float)fabs(a->tmuvtx[0].tow-b->tmuvtx[0].tow);if(e>max_uv)max_uv=e;
                }
            }
            if(max_xy>.02f || max_rgb>.08f || max_spec>.08f || max_uv>.02f || mismatches)ok=0;
            for(rep=0;rep<3;++rep)for(i=0;i<360;++i) {
                unsigned long start,end;
                experiment_pose(i,360);start=profile_clock();prepare();end=profile_clock();sums[rep]+=(end-start)/65536.0;
            }
            profiler.measure=1;profile_reset();
            for(i=0;i<128;++i) {
                int slot;experiment_pose(i,128);render(60,0);slot=(profiler.next+PROFILE_HISTORY-1)%PROFILE_HISTORY;
                frame+=profiler.frame[slot];work+=profiler.frame[slot]-profiler.stage[slot][P_PRESENT];transform+=profiler.stage[slot][P_TRANSFORM];
            }
            fprintf(f,"%s | prepare ms %.6f %.6f %.6f | frame %.4f work %.4f transform %.4f | error xy %.6f rgb %.6f spec %.6f uv %.6f topology %d\n",name,sums[0]/360,sums[1]/360,sums[2]/360,frame/128,work/128,transform/128,max_xy,max_rgb,max_spec,max_uv,mismatches);
            /* Closing also flushes DOSBox-X's host-side file buffering so a
               running benchmark can be monitored from PowerShell. */
            fclose(f);f=fopen(logname,"a");if(!f){free(reference);free(ref_shine);return 0;}
        }
    }
    fprintf(f,"%s: all variants checked against reference over 24 moving poses per quality\n",ok?"PASS":"FAIL");fclose(f);free(reference);free(ref_shine);experiment_flags=0;return ok;
}
