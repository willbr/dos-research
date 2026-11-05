#ifndef HELMET_PROFILER_H
#define HELMET_PROFILER_H
#define PROFILE_HISTORY 128
enum { P_INPUT,P_BACKGROUND,P_TRANSFORM,P_OPAQUE,P_AA,P_SPECULAR,P_HUD,P_OVERLAY,P_PRESENT,
#ifdef PROJECTOR_DEMO
    P_PROJECTOR,
#endif
#ifdef SHADOW_DEMO
    P_SHADOWS,
#endif
#ifdef REFLECTION_DEMO
    P_REFLECTION,
#endif
#ifdef ENVIRONMENT_DEMO
    P_ENVIRONMENT,
#endif
    P_STAGES };
typedef struct {
    int available,show,measure,count,next,active;
    float frame[PROFILE_HISTORY],stage[PROFILE_HISTORY][P_STAGES];
    float current[P_STAGES],mean[P_STAGES],average,min,max,p95;
} FrameProfiler;
extern FrameProfiler profiler;
void profile_init(void);
void profile_reset(void);
void profile_toggle(void);
void profile_begin(void);
void profile_stage(int stage);
void profile_end(void);
void profile_log(void *file);
unsigned long profile_clock(void);
#endif
