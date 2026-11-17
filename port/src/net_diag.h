#ifndef NET_DIAG_H
#define NET_DIAG_H
#include <stdint.h>
#include <stdio.h>
#define ND_HISTORY 512
#define ND_ACTORS 15
#define ND_PADS 10
#define ND_PAD_BYTES 18
int ND_Enabled(void);
void ND_Reset(unsigned);
void ND_Begin(unsigned,int,unsigned,unsigned,unsigned,unsigned);
void ND_Audit(int,const void *,unsigned);
void ND_End(uint64_t);
void ND_Inputs(unsigned,unsigned,unsigned,const unsigned char *,const unsigned char *);
int ND_Dump(unsigned,uint64_t,uint64_t,uint64_t,uint64_t,int,int,int,void (*)(FILE *));
#endif
