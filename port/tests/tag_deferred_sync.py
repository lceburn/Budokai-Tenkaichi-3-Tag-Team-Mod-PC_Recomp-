"""Check production deferred-choice code and the real all-ready gate."""
from pathlib import Path
import subprocess
ROOT=Path(__file__).resolve().parents[2]

def extract(source,name):
    at=source.index('static int '+name+'(');opening=source.index('{',at);end=opening+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[at:end]

def main():
    source=(ROOT/'port/src/gs/tag_net.c').read_text();out=ROOT/'port/build/tag-deferred-sync.c'
    out.write_text(r'''
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "../../port/include/plat_warriors.h"
enum {TN_FIGHTERS=10,TN_PLAYERS=10,TN_OBSERVER=10,PEERS=19,TN_CUSTOM_PICK=1,TN_CUSTOM_SYNC=2,TL_GAUNTLET=3};
typedef struct {int chara,costume,items[8];} TlLoadout;
typedef struct {int size[2],owner[10],chara[10],costume[10],items[10][8],cpuLevel[10],revision,party,practice,mode;} TagNetConfig;
typedef struct {int set,owner,local,cpu;TlLoadout selection;char name[192];} CustomPick;
typedef struct {struct {unsigned state,size,received;} customRx;struct {unsigned state,size,offset;} customTx;int hello,role;} Peer;
static Peer peers[PEERS];static CustomPick customPicks[10];static TagNetConfig config;
static int host=1,role=0,phase=2,customSync,customGrant=-1,customCatalog,customCatalogReceived=1;
static int ready[10],localReady=1,catalogReady=1,shares,failures,packets;static unsigned roundId;
static struct {int active;} tournament;static char message[192];
static uint32_t get32(const void *p){const unsigned char *b=p;return (uint32_t)b[0]<<24|(uint32_t)b[1]<<16|(uint32_t)b[2]<<8|b[3];}
static void put32(void *p,uint32_t v){unsigned char *b=p;b[0]=v>>24;b[1]=v>>16;b[2]=v>>8;b[3]=v;}
static void custom_pick_config(const TagNetConfig *next);
static void apply_edit(const TagNetConfig *next){custom_pick_config(next);config=*next;config.revision++;if(!customSync)memset(ready,0,sizeof(ready));}
static int valid(const TagNetConfig *c){return c->chara[0]>=0 && c->chara[0]<250;}
static int custom_fighter_owned(int owner,unsigned fighter,unsigned rev){return phase==2 && !config.practice && rev==(unsigned)config.revision && fighter<2 && (config.owner[fighter]==owner || (owner==0 && config.owner[fighter]<0));}
static void send_packet(Peer *p,int type,const void *b,unsigned n){(void)p;(void)type;(void)b;(void)n;packets++;}
static void broadcast(int type,const void *b,unsigned n,Peer *p){send_packet(p,type,b,n);}
static void say(const char *s){snprintf(message,sizeof(message),"%s",s);}
static void failed(const char *s){say(s);failures++;}
int PortWarriors_LocalCount(void){return 1;}
int PortWarriors_LocalCopy(int n,PortWarriorLocal *out){if(n)return 0;memset(out,0,sizeof(*out));strcpy(out->name,"Local Fighter");return 1;}
static int custom_local_ready(void){return localReady;}
static int Wnc_CanStart(const void *c){(void)c;return catalogReady;}
static unsigned player_mask(void){return 3;}
static int popcount(unsigned m){return __builtin_popcount(m);}
#include "../../port/src/gs/tag_net_deferred.h"
static int custom_share(int local,unsigned fighter,const TlLoadout *s,int cpu){
 (void)s;(void)cpu;assert(customSync && ready[0] && ready[1] && local==0);shares++;
 custom_pick_clear(fighter);return 1;
}
''' + extract(source,'all_ready') + r'''
int main(void) {
 TlLoadout f={0};unsigned char wire[248];
 config.size[0]=config.size[1]=1;config.owner[0]=0;config.owner[1]=1;config.revision=1;
 assert(custom_choose(0,0,&f,-1));assert(shares==0 && customPicks[0].set && config.revision==2);
 assert(!all_ready() && shares==0);ready[0]=1;
 assert(!all_ready() && shares==0);ready[1]=1;
 catalogReady=0;assert(!all_ready() && shares==0 && customSync);
 catalogReady=1;assert(!all_ready() && shares==1);assert(all_ready());
 // Returning to choices: same stock fighter must explicitly clear a pending custom.
 customSync=0;assert(custom_choose(0,0,&f,-1));assert(customPicks[0].set);
 assert(custom_choose(-2,0,&f,-1));assert(!customPicks[0].set);
 assert(custom_choose(0,0,&f,-1));custom_pick_wire(0,wire);
 config.revision++;assert(!custom_pick_apply(0,wire,sizeof(wire),0));
 assert(!custom_pick_apply(0,wire,1,0));
 // During syncing the transfer percentage comes from actual received bytes.
 customSync=1;peers[0].customRx.state=1;peers[0].customRx.size=1000;peers[0].customRx.received=600;
 custom_sync_progress();assert(strstr(message,"60%"));
 assert(!failures && packets>0);
 puts("Deferred sync: no package work before all ready, catalog gating, stale revision rejection, stock replacement and progress passed.");
}
''')
    exe=out.with_suffix('')
    subprocess.run(['cc','-std=c99','-O1','-g','-fsanitize=address,undefined',str(out),'-o',str(exe)],cwd=ROOT,check=True)
    subprocess.run([str(exe)],cwd=ROOT,check=True)

if __name__=='__main__':main()
