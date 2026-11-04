/* Intentionally included repeatedly: compile each variant with constant flags. */
static void EXP_FUNCTION(void)
{
    float sy,cy,sp,cp,r10,r12,r20,r22,lx,ly,lz,hx,hy,hz,scale=660*zoom;
    Vec light=unit(vec(-.6f,.75f,1)),half=unit(add(light,vec(0,0,1)));
    float matrix[10];int i;
    if(EXP_FLAGS&OPT_TRIG) {
        sy=lookup_sin(yaw);cy=lookup_sin(yaw+1.57079632679f);
        sp=lookup_sin(pitch);cp=lookup_sin(pitch+1.57079632679f);
    } else { sy=(float)sin(yaw);cy=(float)cos(yaw);sp=(float)sin(pitch);cp=(float)cos(pitch); }
    r10=sp*sy;r12=-sp*cy;r20=-cp*sy;r22=cp*cy;
    lx=cy*light.x+r10*light.y+r20*light.z;ly=cp*light.y+sp*light.z;lz=sy*light.x+r12*light.y+r22*light.z;
    hx=cy*half.x+r10*half.y+r20*half.z;hy=cp*half.y+sp*half.z;hz=sy*half.x+r12*half.y+r22*half.z;
    matrix[0]=cy;matrix[1]=sy;matrix[2]=r10;matrix[3]=cp;matrix[4]=r12;
    matrix[5]=r20;matrix[6]=sp;matrix[7]=r22;matrix[8]=6.6f;matrix[9]=1;
    if(EXP_FLAGS&OPT_SHARED)for(i=0;i<normal_count;++i)
        lighting(normal_lut[i][0],normal_lut[i][1],normal_lut[i][2],normal_material[i],lx,ly,lz,hx,hy,hz,&normal_diff[i],&normal_shine[i]);
    for(i=0;i<vertex_count;++i) {
        Vertex *s=&vertices[i];GrVertex *v=&projected[i];
        float x,y,z,u,vv,xx,yy,inverse,diff,spec;int mat;
        if(EXP_FLAGS&OPT_COMPACT) {
            x=pos_stream[i][0];y=pos_stream[i][1];z=pos_stream[i][2];
            u=uv_stream[i][0];vv=uv_stream[i][1];mat=material_stream[i];
        } else { x=s->p.x;y=s->p.y-.58f;z=s->p.z;u=s->u;vv=s->v;mat=s->mat; }
        if(EXP_FLAGS&OPT_ASM) {
            float result[3];project_x87(pos_stream[i],matrix,result);xx=result[0];yy=result[1];inverse=result[2];
        } else { xx=cy*x+sy*z;yy=r10*x+cp*y+r12*z;inverse=1/(6.6f-(r20*x+sp*y+r22*z)); }
        if(EXP_FLAGS&OPT_SHARED) { diff=normal_diff[normal_index[i]];spec=normal_shine[normal_index[i]]; }
        else lighting(s->n.x,s->n.y,s->n.z,mat,lx,ly,lz,hx,hy,hz,&diff,&spec);
        shine[i]=spec;
        v->x=320+xx*scale*inverse;v->y=244-yy*scale*inverse;
        v->oow=inverse;v->ooz=65535*inverse;v->a=255;
        v->tmuvtx[0].sow=u*inverse;v->tmuvtx[0].tow=vv*inverse;v->tmuvtx[0].oow=inverse;
        if(textured && mat<GLASS) {
            if(detailed)v->r=v->g=v->b=diff*255;
            else { float intensity=diff*255+(highlights?spec*1.4f:0);if(intensity>255)intensity=255;v->r=v->g=v->b=intensity; }
        } else {
            float highlight=(!detailed && highlights)?spec:0;
            v->r=clamp(diff*base[mat][0]+highlight,0,255);
            v->g=clamp(diff*base[mat][1]+highlight*.94f,0,255);
            v->b=clamp(diff*base[mat][2]+highlight*.8f,0,255);
        }
    }
    finish_prepare(r20*6.6f,sp*6.6f+.58f,r22*6.6f);
}
