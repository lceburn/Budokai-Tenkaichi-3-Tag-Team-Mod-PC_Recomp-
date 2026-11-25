/* Render-only clearance for assembled custom geometry. Native camera state and
 * targetViews stay untouched. Bounds include imported head/hair/back pieces.
 * sqrtf must use port_libm.h's software-float bridge, never the compiler builtin. */
static s32 BtlCam_CustomBox(BtlObj *obj, BtlObjBound *bound, const Vec4 *origin,
                            const Vec4 *dir, f32 distance, f32 *exit) {
    extern BtlObjPart *BtlObj_GetNode(BtlObj *,s32);
    extern s32 BtlObj_IsNodeShown(BtlObj *,s32);
    BtlObjPart *part;
    f32 center[3], local[3], ray[3], ext[3], lo=0.0f, hi=10000.0f;
    s32 i,j,inside=1;
    if (!bound->enabled || !(part=BtlObj_GetNode(obj,bound->node)) ||
        !BtlObj_IsNodeShown(obj,part->unk08)) return 0;
    for (i=0;i<3;i++) {
        center[i]=part->mtx.m[3][i];
        for (j=0;j<3;j++) center[i]+=(((const f32 *)&bound->center)[j]-((const f32 *)&bound->unk10)[j])*part->mtx.m[j][i];
    }
    for (i=0;i<3;i++) {
        f32 q=0.0f,p=0.0f,d=0.0f;
        for (j=0;j<3;j++) {
            f32 axis=part->mtx.m[i][j];
            q+=axis*axis;p+=(((const f32 *)origin)[j]-center[j])*axis;d+=((const f32 *)dir)[j]*axis;
        }
        if (!__builtin_isfinite(q) || q<0.0001f) return 0;
        local[i]=p/q;ray[i]=d/q;
        ext[i]=__builtin_fabsf(((const f32 *)&bound->extent)[i])+2.0f/sqrtf(q);
        if (!__builtin_isfinite(local[i]) || !__builtin_isfinite(ray[i]) || !__builtin_isfinite(ext[i])) return 0;
        if (__builtin_fabsf(local[i]+ray[i]*distance)>ext[i]) inside=0;
        if (__builtin_fabsf(ray[i])<0.00001f) {
            if (__builtin_fabsf(local[i])>ext[i]) return 0;
        } else {
            f32 a=(-ext[i]-local[i])/ray[i],b=(ext[i]-local[i])/ray[i],swap;
            if(a>b){swap=a;a=b;b=swap;}
            if(a>lo)lo=a;if(b<hi)hi=b;
            if(lo>hi)return 0;
        }
    }
    if(hi>*exit)*exit=hi;
    return inside;
}

static s32 BtlCam_CustomRay(BtlObj *obj,const Vec4 *origin,const Vec4 *dir,f32 distance,f32 *exit) {
    BtlObjBound *bound=obj->mdl.bounds;
    s32 n,inside=0;
    *exit=0.0f;
    for(n=0;bound && n<640;n++) {
        inside|=BtlCam_CustomBox(obj,bound,origin,dir,distance,exit);
        if(bound->last || bound->next<0x40)break;
        bound=(BtlObjBound *)((u8 *)bound+bound->next);
    }
    return inside;
}

static void BtlCam_CustomClearance(BtlCamView *view,s32 objId) {
    extern void BtlCharApi_GetNodePos(s32,s32,Vec4 *);
    extern s32 gStgColMode;
    BtlObj *obj=BtlObj_Get(objId);
    Vec4 head,dir,candidate,at,best;
    f32 length=0.0f,exit,bestScore=1.0e30f;
    s32 i,attempt,mode;
    if(!obj || !PortGame_IsCustomCharacter(obj->chara))return;
    BtlCharApi_GetNodePos(objId,0x30,&head);
    for(i=0;i<3;i++){((f32 *)&dir)[i]=((f32 *)&view->pos)[i]-((f32 *)&head)[i];length+=((f32 *)&dir)[i]*((f32 *)&dir)[i];}
    if(!__builtin_isfinite(length))return;
    length=sqrtf(length);
    if(length<0.01f){dir.x=0;dir.y=0;dir.z=-1;length=0;}else for(i=0;i<3;i++)((f32 *)&dir)[i]/=length;
    dir.w=0;
    if(!BtlCam_CustomRay(obj,&head,&dir,length,&exit))return;
    mode=gStgColMode;gStgColMode=0;
    for(attempt=0;attempt<9;attempt++) {
        Vec4 ray=dir;
        f32 distance,score=0.0f,unused;
        /* Start along the requested view ray. Tight stage corners may require
           a side/up escape; choose the valid candidate nearest that view. */
        if(attempt) {
            static const f32 directions[8][3]={{1,0,0},{-1,0,0},{0,0,1},{0,0,-1},{0,-1,0},{1,-1,0},{-1,-1,0},{0,-1,-1}};
            f32 norm=0.0f;
            for(i=0;i<3;i++){((f32 *)&ray)[i]=directions[attempt-1][i];norm+=((f32 *)&ray)[i]*((f32 *)&ray)[i];}
            for(i=0;i<3;i++)((f32 *)&ray)[i]/=sqrtf(norm);
        }
        BtlCam_CustomRay(obj,&head,&ray,0,&exit);
        distance=exit+0.25f;if(distance<length)distance=length;
        if(distance>1000.0f)continue;
        for(i=0;i<3;i++)((f32 *)&candidate)[i]=((f32 *)&head)[i]+((f32 *)&ray)[i]*distance;
        candidate.w=1;
        if(BtlCam_TraceStage(&at,&candidate,&head,NULL,NULL)) {
            f32 available=0;
            for(i=0;i<3;i++){f32 d=((f32 *)&at)[i]-((f32 *)&head)[i];available+=d*d;}
            available=sqrtf(available)-2.0f;
            if(available<=0)continue;
            for(i=0;i<3;i++)((f32 *)&candidate)[i]=((f32 *)&head)[i]+((f32 *)&ray)[i]*available;
            distance=available;
        }
        if(BtlCam_CustomRay(obj,&head,&ray,distance,&unused))continue;
        for(i=0;i<3;i++){f32 d=((f32 *)&candidate)[i]-((f32 *)&view->pos)[i];score+=d*d;}
        if(score<bestScore){bestScore=score;best=candidate;}
        if(!attempt && bestScore<1.0e30f)break;
    }
    gStgColMode=mode;
    if(bestScore<1.0e30f)view->pos=best;
}
