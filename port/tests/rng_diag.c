/* Compare instrumented production generators with retained pre-trace sources.
   The same small arithmetic adapters serve both float implementations; this
   tests instrumentation equivalence, not PS2 float-model conformance. */
#include "../src/net_diag.c"
#include <assert.h>
extern unsigned long long gPortRandNext;
unsigned long long baselineLibc;
extern unsigned gPortVu0R,baselineVu0;
unsigned gRandState[624],baselineState[624],gRandMag01[2]={0,0x9908b0dfu},baselineMag[2]={0,0x9908b0dfu};
int gRandIndex,baselineIndex;
void Port_Srand(unsigned),baselineSrand(unsigned),Rand_SeedFloat(float),baselineSeedFloat(float),Rand_Seed(unsigned);
int Port_Rand(void),baselineRand(void),Rand_IntRange(int,int),baselineIntRange(int,int);
float Rand_Float01(void),baselineFloat01(void),Rand_FloatRange(float,float),baselineFloatRange(float,float);
unsigned Rand_Next(void),baselineNext(void),Rand_Range(unsigned),baselineRange(unsigned);
static unsigned fbits(float f){unsigned u;memcpy(&u,&f,4);return u;}
unsigned RefVu0_SubBits(unsigned a,unsigned b){float x,y;memcpy(&x,&a,4);memcpy(&y,&b,4);return fbits(x-y);}
unsigned RefVu0_MulBits(unsigned a,unsigned b){float x,y;memcpy(&x,&a,4);memcpy(&y,&b,4);return fbits(x*y);}
int main(void) {
    unsigned i,seed,ai[ND_AI_WORDS]={0};uint64_t saved;
    enabled=1;ND_Reset(5);ND_Begin(100,0,3,98,97,0);ND_End(0);ND_Phase(NP_EFFECTS_UPDATE);ND_View(2);
    assert(ND_AiActive());ai[0]=3;ai[15]=0x4000;
    ND_Ai(3,1,ai,ND_AI_WORDS);ai[15]=0;
    assert(current->aiPresent[1]==8 && current->ai[1][3][15]==0x4000);
    ND_Ai(-1,0,ai,ND_AI_WORDS);ND_Ai(ND_ACTORS,0,ai,ND_AI_WORDS);
    ND_Ai(3,ND_AI_STAGES,ai,ND_AI_WORDS);ND_Ai(3,0,ai,ND_AI_WORDS-1);
    assert(!current->aiPresent[0]);ND_Ai(-1,-1,NULL,0);
    ND_AiCheck35(3,ai,7);ai[0]=0;
    assert(current->check35Present==8&&current->check35[3][0]==3);
    ND_AiCheck35(-1,ai,7);ND_AiCheck35(ND_ACTORS,ai,7);ND_AiCheck35(3,ai,6);
    assert(current->check35Present==8);
    for(seed=0;seed<4;seed++) {
        Port_Srand(seed*1234567u);baselineSrand(seed*1234567u);
        for(i=0;i<1000;i++){assert(Port_Rand()==baselineRand());assert(gPortRandNext==baselineLibc);}
        for(i=0;i<100;i++){assert(Rand_IntRange(-15,99)==baselineIntRange(-15,99));assert(gPortRandNext==baselineLibc);}
    }
    assert(current->rngCount[NR_LIBC_NEXT]==4400 && current->rngCount[NR_LIBC_RANGE]==400);
    Rand_SeedFloat(0.1234141f);baselineSeedFloat(0.1234141f);
    assert(gPortVu0R==baselineVu0&&gPortRandNext==baselineLibc);
    ND_Phase(NP_DRAW_EFFECTS);ND_View(1);
    for(i=0;i<2000;i++){assert(fbits(Rand_Float01())==fbits(baselineFloat01()));assert(gPortVu0R==baselineVu0);}
    for(i=0;i<300;i++){assert(fbits(Rand_FloatRange(-4.0f,19.0f))==fbits(baselineFloatRange(-4.0f,19.0f)));assert(gPortVu0R==baselineVu0);}
    Rand_Seed(12345);memcpy(baselineState,gRandState,sizeof(gRandState));baselineIndex=gRandIndex;
    for(i=0;i<1000;i++){assert(Rand_Next()==baselineNext());assert(gRandIndex==baselineIndex);assert(!memcmp(gRandState,baselineState,sizeof(gRandState)));}
    for(i=0;i<100;i++){assert(Rand_Range(i%17)==baselineRange(i%17));assert(gRandIndex==baselineIndex);}
    assert(current->siteCount>0 && current->sites[0].caller && current->sites[0].phase==NP_EFFECTS_UPDATE);
    ND_Begin(101,0,3,99,98,0);ND_Rng(NR_LIBC_NEXT,1,2,0x20123456,3);
    ND_Begin(100,1,3,99,98,1);assert(!history[101%512].valid&&!current->firstEvent);
    ND_Phase(NP_CAMERA);ND_View(3);
    ND_Ai(3,0,ai,ND_AI_WORDS);
    ND_AiCheck35(3,ai,7);
    for(i=0;i<ND_RNG_EVENTS+100;i++)ND_Rng(NR_LIBC_NEXT,i,i+1,0x20001000+(i%(ND_RNG_SITES+16)),i);
    assert(current->rngCount[NR_LIBC_NEXT]==ND_RNG_EVENTS+100 && current->siteOverflow);
    assert(current->sites[0].actor==3 && rngEvents[(rngSequence-1)%ND_RNG_EVENTS].actor==3);
    assert(ND_Dump(100,0,0,1,2,1,0,1,NULL));
    saved=gPortRandNext;ND_Rng(NR_LIBC_SEED,1,2,3,4);assert(gPortRandNext==saved);
    assert(sizeof(history)+sizeof(rngEvents)+sizeof(initial)+sizeof(seedEvents)<16u*1024u*1024u);
    ND_Reset(6);assert(!initialValid&&!rngSequence&&!seedCount&&!ND_AiActive()&&rngActor==-1);
    enabled=0;Port_Srand(1);baselineSrand(1);for(i=0;i<50;i++)assert(Port_Rand()==baselineRand());assert(!rngSequence);
    puts("PASS: production libc/VU0/MT outputs and states match pre-trace baseline; wrappers, phases/views, replay invalidation, ring/site overflow, reset, opt-out and fixed memory.");
    return 0;
}
