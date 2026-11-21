#ifndef NET_RNG_DIAG_H
#define NET_RNG_DIAG_H
/* Diagnostic-only calls: no game structure, RNG algorithm or wire hash changes. */
enum { NR_LIBC_NEXT=1,NR_LIBC_SEED,NR_VU0_NEXT,NR_VU0_SEED,NR_MT_NEXT,
       NR_LIBC_WRAP,NR_LIBC_RANGE,NR_VU0_RANGE,NR_MT_RANGE,NR_KIND_COUNT };
enum { NP_IDLE,NP_NET_TICK,NP_SETUP,NP_AUDIO,NP_AI_INPUT,NP_FIGHTERS,
       NP_EFFECTS_UPDATE,NP_LOADERS,NP_STAGE_UPDATE,NP_CAMERA,NP_VISIBILITY,
       NP_MECHANICS,NP_VIEW_PREP,NP_DRAW_WORLD,NP_DRAW_FIGHTERS,NP_DRAW_EFFECTS,
       NP_HUD,NP_OVERLAY,NP_FRAME_END };
void ND_Rng(int,unsigned long long,unsigned long long,unsigned long long,unsigned);
int ND_Phase(int);
void ND_View(int);
/* Bounded, host-only AI observation: stages enter, sensed, thought, sent. */
void ND_Ai(int actor,int stage,const unsigned *words,unsigned count);
int ND_AiActive(void);
void ND_AiCheck35(int actor,const unsigned *words,unsigned count);
#ifdef PORT
#define NR_PHASE(p) ((void)ND_Phase(p))
#define NR_VIEW(v) ND_View(v)
#else
#define NR_PHASE(p) ((void)0)
#define NR_VIEW(v) ((void)0)
#endif
#endif
