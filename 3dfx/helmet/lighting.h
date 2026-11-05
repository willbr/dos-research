/* Production C renderer: recompute lighting per exact normal/material group. */
static float group_diff[MAX_VERTS],group_shine[MAX_VERTS];
static void prepare_lit(void)
{
    float sy=(float)sin(yaw),cy=(float)cos(yaw),sp=(float)sin(pitch),cp=(float)cos(pitch);
    float r10=sp*sy,r12=-sp*cy,r20=-cp*sy,r22=cp*cy;
    Vec light=unit(vec(-.6f,.75f,1)),half=unit(add(light,vec(0,0,1)));
    float lx=cy*light.x+r10*light.y+r20*light.z,ly=cp*light.y+sp*light.z,lz=sy*light.x+r12*light.y+r22*light.z;
    float hx=cy*half.x+r10*half.y+r20*half.z,hy=cp*half.y+sp*half.z,hz=sy*half.x+r12*half.y+r22*half.z;
    float camx=r20*6.6f,camy=sp*6.6f+.58f,camz=r22*6.6f,scale=660*zoom;
    int i;
    for(i=0;i<lighting_group_count;++i) {
        LightingGroup *g=&lighting_groups[i];
        float diff=g->normal.x*lx+g->normal.y*ly+g->normal.z*lz;
        float h=g->normal.x*hx+g->normal.y*hy+g->normal.z*hz,h2,h4,h8,h16;
#ifdef ENVIRONMENT_DEMO
        if(environment_enabled && environment_strength>0) {
            environment_u[i]=127.5f*(1+cy*g->normal.x+sy*g->normal.z);
            environment_v[i]=127.5f*(1-r10*g->normal.x-cp*g->normal.y-r12*g->normal.z);
        }
#endif
        if(diff<0)diff=0;group_diff[i]=.34f+.66f*diff;if(h<0)h=0;
        h2=h*h;h4=h2*h2;h8=h4*h4;h16=h8*h8;
        group_shine[i]=(g->mat==GLASS?h16:h16*h16)*specular[g->mat]*255;
    }
    for(i=0;i<vertex_count;++i) {
        Vertex *s=&vertices[i];GrVertex *v=&projected[i];
        float x=s->p.x,y=s->p.y-.58f,z=s->p.z;
        float inverse=1/(6.6f-(r20*x+sp*y+r22*z));
        int group=vertex_lighting_group[i];
        float diff=group_diff[group];
        shine[i]=group_shine[group];
        v->x=320+(cy*x+sy*z)*scale*inverse;v->y=244-(r10*x+cp*y+r12*z)*scale*inverse;
        v->oow=inverse;v->ooz=65535*inverse;v->a=255;
        v->tmuvtx[0].sow=s->u*inverse;v->tmuvtx[0].tow=s->v*inverse;v->tmuvtx[0].oow=inverse;
        if(textured && s->mat<GLASS) {
            if(detailed) v->r=v->g=v->b=diff*255;
            else { float light=diff*255+(highlights?shine[i]*1.4f:0);if(light>255)light=255;v->r=v->g=v->b=light; }
        } else {
            float spec=(!detailed && highlights)?shine[i]:0;
            v->r=clamp(diff*base[s->mat][0]+spec,0,255);
            v->g=clamp(diff*base[s->mat][1]+spec*.94f,0,255);
            v->b=clamp(diff*base[s->mat][2]+spec*.8f,0,255);
        }
    }
    finish_prepare(camx,camy,camz);
}
