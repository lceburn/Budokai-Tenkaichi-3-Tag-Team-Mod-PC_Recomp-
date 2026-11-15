#ifndef TAG_CAMERA_CONTROLS_H
#define TAG_CAMERA_CONTROLS_H
typedef struct TagSpectatorChoice { int actor,mode; } TagSpectatorChoice;
/* Native character views, free flight, overview, then the first character.
   Integer-only state belongs to the match; floating camera poses remain local. */
static inline TagSpectatorChoice TagSpectator_Next(int count,int actor,int mode,unsigned watchable) {
    TagSpectatorChoice next={actor,mode};int i;
    if(mode==1){next.mode=3;return next;}
    if(mode==3){next.mode=0;actor=-1;}
    for(i=actor+1;i<count;i++)if(watchable&(1u<<i)){next.actor=i;next.mode=0;return next;}
    next.mode=1;return next;
}
static inline float TagCamera_Pitch(float pitch,float axis,int sensitivity) {
    pitch-=axis*sensitivity*(0.0523598776f/100.0f);
    return pitch<-1.2f?-1.2f:pitch>1.2f?1.2f:pitch;
}
#endif
