/* Synthetic Windows stop/capture test, no game assets or gameplay. */
#define PORT 1
#include "../src/effect_diag.c"
unsigned long long TagNet_BuildIdentity(void){return 0x12345678;}
int main(void){
#ifdef _WIN32
    SetErrorMode(SEM_NOGPFAULTERRORBOX|SEM_FAILCRITICALERRORS);
#endif
    enabled=1;
    EDiag_Match(4,1);EDiag_Roster(2,3,0,0,1,0);
    EDiag_Request(2,2);
    return 99;
}
