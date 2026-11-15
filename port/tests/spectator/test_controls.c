#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../../../include/battle/tag_camera_controls.h"
int main(void) {
    TagSpectatorChoice c={0,0};int i;float pitch=0;
    c=TagSpectator_Next(4,c.actor,c.mode,15);assert(c.actor==1 && !c.mode);
    c=TagSpectator_Next(4,c.actor,c.mode,11);assert(c.actor==3 && !c.mode);
    c=TagSpectator_Next(4,c.actor,c.mode,15);assert(c.actor==3 && c.mode==1);
    c=TagSpectator_Next(4,c.actor,c.mode,15);assert(c.mode==3);
    c=TagSpectator_Next(4,c.actor,c.mode,15);assert(c.actor==0 && !c.mode);
    c=TagSpectator_Next(4,3,3,12);assert(c.actor==2 && !c.mode);
    assert(TagCamera_Pitch(0,-1,100)>0 && TagCamera_Pitch(0,1,100)<0);
    assert(fabsf(TagCamera_Pitch(0,-1,200)-2*TagCamera_Pitch(0,-1,100))<.0001f);
    for(i=0;i<300;i++)pitch=TagCamera_Pitch(pitch,-1,300);
    assert(pitch==1.2f);
    for(i=0;i<300;i++)pitch=TagCamera_Pitch(pitch,1,300);
    assert(pitch==-1.2f);
    puts("PASS: actor cycle with skipped/fallen actors, free camera, overview, wrap; vertical direction, sensitivity and pitch limits.");
}
