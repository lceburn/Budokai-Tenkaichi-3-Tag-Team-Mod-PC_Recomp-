/* Synthetic captures; the production module owns all history/write logic. */
#include "../src/net_diag.c"
#include <assert.h>
static int client,teamMode;
static void record(unsigned f,int rewind) {
    uint32_t actor[29]={0},mt[624],extra[4]={9,10,11,12},header[6]={15,3,f,0,5,2};
    uint32_t rng[2]={13,14},lobby[2]={15,16};unsigned a,i;
    ND_Begin(f,rewind,3,f>2?f-2:0,f>3?f-3:0,rewind?1:0);
    ND_Audit(-1,header,teamMode?24:16);ND_Audit(-2,rng,sizeof(rng));ND_Audit(-3,lobby,sizeof(lobby));
    for(i=0;i<624;i++)mt[i]=i;ND_Audit(-4,mt,sizeof(mt));ND_Audit(-5,extra,sizeof(extra));
    for(a=0;a<15;a++){actor[0]=a;actor[teamMode?20:2]=(client&&f>=599&&a==7)?1:0;ND_Audit(a,actor,teamMode?116:112);}
    { uint64_t h=14695981039346656037ull;const unsigned char *p;unsigned n;
      p=(const unsigned char *)current->header;for(n=0;n<current->headerWords*4;n++)h=(h^p[n])*1099511628211ull;
      for(a=0;a<15;a++){p=(const unsigned char *)current->actor[a];for(n=0;n<current->words[a]*4;n++)h=(h^p[n])*1099511628211ull;}
      p=(const unsigned char *)current->rng;for(n=0;n<8;n++)h=(h^p[n])*1099511628211ull;
      ND_End(h^((uint64_t)lobby[0]|(uint64_t)lobby[1]<<32));
    }
}
int main(int argc,char **argv) {
    unsigned f;unsigned char input[10][18]={{0}};
    if(argc>1&&!strcmp(argv[1],"disabled")){assert(!ND_Enabled());ND_Reset(1);ND_Begin(0,0,3,0,0,0);ND_Audit(-1,NULL,0);ND_End(1);assert(!ND_Dump(0,1,2,3,4,0,0,1,NULL));return 0;}
    enabled=1;client=argc>1&&strstr(argv[1],"client")!=NULL;teamMode=argc>1&&strstr(argv[1],"teams-")!=NULL;ND_Reset(41);
    for(f=0;f<=600;f++)record(f,0);
    assert(history[0].stamp==512 && history[600%512].mt[623]==623);
    record(595,1);assert(!history[600%512].valid&&history[594%512].valid);
    for(f=596;f<=600;f++)record(f,1);
    assert(branches==1&&history[600%512].words[14]==(teamMode?29:28));
    assert(history[600%512].headerWords==(teamMode?6:4));
    if(teamMode)assert(history[600%512].header[4]==5&&history[600%512].header[5]==2);
    for(f=540;f<=600;f++)ND_Inputs(f,1023,1023,&input[0][0],&input[0][0]);
    assert(history[599%512].usedMask==3&&history[599%512].actualMask==3);
    if(argc>1&&!strcmp(argv[1],"fail")){assert(!ND_Dump(599,599,600,1,2,1,0,1,NULL));return 0;}
    assert(ND_Dump(599,history[599%512].hash,0,1,2,!client,client,client?0:1,NULL));
    assert(!ND_Dump(599,1,2,3,4,0,0,1,NULL));
    ND_Reset(42);assert(!captured&&!history[599%512].valid&&!haveFrame);
    puts("PASS: bounded wrap, rollback replacement/future invalidation, all actors and RNG, input masks, first capture and round reset.");
    return 0;
}
