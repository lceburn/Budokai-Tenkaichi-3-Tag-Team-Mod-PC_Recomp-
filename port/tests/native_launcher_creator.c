/* Stub child for the installed Windows launcher migration regression. */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
int wmain(int argc,wchar_t **argv){
 if(argc>1){FILE *f=fopen("launcher-helper.txt","w");for(int i=1;i<argc;i++)fwprintf(f,L"%ls\n",argv[i]);fclose(f);return 0;}
 FILE *f=fopen("launcher-engine.txt","w");const char *a=getenv("BT3_WARRIOR_MODELS"),*b=getenv("BT3_WARRIOR_NATIVE_HEAD_TEST");fprintf(f,"models=%s\nhead=%s\n",a?a:"off",b?b:"off");fclose(f);return 0;
}
