#ifndef EFFECT_DIAG_H
#define EFFECT_DIAG_H
#ifdef PORT
int EDiag_Enabled(void);
void EDiag_Event(const char *,void *,void *,int,int,unsigned long long);
void EDiag_List(void *,void *,unsigned,unsigned,void *,unsigned,int,void *);
void EDiag_ListGone(void *);
void EDiag_Task(void *,void *,void *);
void EDiag_Kill(void *,const char *,unsigned long long);
void EDiag_Pool(int);
void EDiag_Char(int,int,const char *);
void EDiag_Bind(int,int,void *,void *,void *,void *,int,int);
void EDiag_Unbind(int,int,const char *);
int EDiag_Check(int,int,void *,void *,void *,void *);
int EDiag_Request(int,int);
void EDiag_Boundary(const char *);
void EDiag_Match(int,int);
void EDiag_Frame(unsigned,int);
void EDiag_Roster(int,int,int,int,int,int);
void EDiag_Crash(void);
void EDiag_Scene(int);
#define ED(...) do { if(EDiag_Enabled()) { __VA_ARGS__; } } while(0)
#else
#define ED(...) ((void)0)
#endif
#endif
