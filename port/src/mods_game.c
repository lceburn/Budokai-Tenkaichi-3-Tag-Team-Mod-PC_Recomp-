/*
 * Script mods, the game's side (the scripts themselves: port/src/gs/mods.c; docs/port/mods.md). Once per blank on the
 * game's thread this file reads what the scripts are given of a fight beyond the gauges (fight_panels.c reads those):
 * where each fighter is. And it is where the camera hook in src/battle/btl_cam.c (BtlCam_UpdateView) comes in: the
 * pose the game gives the battle view each frame is handed to the scripts' on_camera and the pose they return is
 * the one built and drawn.
 *
 * That pose is only drawn: nothing of the fight reads it back (include/battle/btl_char_cam.h: only
 * BtlCharApi_GetCamPose reads ChrCam.pos / rot; stick movement goes by ChrCam.yaw), and the game builds the view
 * from the fighter camera again at the start of the next frame. The other readers of the view are drawing, effects
 * and sound (include/battle/btl_cam.h). Blanks run again after a rollback, and self-check runs (BT3_SYNCTEST), get
 * the game's pose as it is. Cuts (techniques, transformations), fixed and frozen cameras are not handed over.
 *
 * Nothing here draws a game random number or writes anything but that view. This file's variables are not game
 * state (port/tools/make_state.py).
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "gs/mods.h"
#include "port_addr.h"
#include "../../include/battle/tag_capacity.h"

#if (defined(__x86_64__) || defined(__aarch64__)) && defined(__clang__)
#undef __ptr32 /* the Windows headers of mingw define these two away */
#undef __uptr
#define GAME_PTR *__ptr32 __uptr
#else
#define GAME_PTR *
#endif

extern int gPortResim; /* gs/state.c: blanks being run again */
extern void GAME_PTR gBtlChars;
extern void GAME_PTR gBtlSeq;
extern void GAME_PTR BtlChar_FindByObjId(int objId);
extern void GAME_PTR BtlChar_FindBySide(int side);
extern int BtlChar_TestFlag(void GAME_PTR chr, unsigned n);
extern int ChrCam_IsCutActive(void GAME_PTR chr);
extern int Battle_IsSplitScreen(void);
extern int TagTeam_Active(void),TagTeam_ScriptHudActor(int);

#define CHR_POSE 0x10        /* include/battle/btl_act_a.h: the pose block; pos at +0x00 */
#define POSE_YAW 0x94        /* ... heading yaw */
#define CHR_CAM_TARGET 0x450 /* include/battle/btl_char_cam.h: ChrCam target */

static ModsGame sGame;
static ModsCam sCam;
static int sCamOut;

static void rdf(float *out, uint8_t GAME_PTR p, int off, int n) {
    int i;
    uint32_t v;

    for (i = 0; i < n; i++) { /* (the bits as they are: no float operation in this software-float file) */
        v = *(uint32_t GAME_PTR)(p + off + i * 4);
        memcpy(&out[i], &v, 4);
    }
}

/* n floats at a game address (Port_Ptr) <-> here, as bits */
static void rda(float *out, uint32_t addr, int n) {
    memcpy(out, Port_Ptr(addr), (size_t)n * 4);
}
static void wra(uint32_t addr, const float *in, int n) {
    memcpy(Port_Ptr(addr), in, (size_t)n * 4);
}

void PortMods_GameTick(void) {
    int side;
    uint8_t GAME_PTR chr;

    if (gPortResim) {
        return;
    }
    memset(&sGame, 0, sizeof(sGame));
    if (gBtlChars != NULL && gBtlSeq != NULL) {
        for (side = 0; side < 2; side++) {
            int actor=TagTeam_ScriptHudActor(side);
            chr = actor>=0?(uint8_t GAME_PTR)BtlChar_FindBySide(actor):NULL;
            if (chr != NULL) {
                sGame.f[side].found = 1;
                rdf(sGame.f[side].pos, chr, CHR_POSE, 3);
                rdf(&sGame.f[side].yaw, chr, CHR_POSE + POSE_YAW, 1);
            }
        }
        sGame.battle = sGame.f[0].found && (sGame.f[1].found || TagTeam_Active());
        sGame.split = sGame.battle && Battle_IsSplitScreen();
    }
    Mods_Tick();
}

int PortMods_Read(ModsGame *out) {
    memcpy(out, &sGame, sizeof(*out));
    return sGame.battle;
}

int PortMods_CamGet(ModsCam *out) {
    memcpy(out, &sCam, sizeof(*out));
    return 1;
}

void PortMods_CamSet(const ModsCam *cam) {
    memcpy(sCam.pos, cam->pos, sizeof(sCam.pos));
    memcpy(sCam.rot, cam->rot, sizeof(sCam.rot));
    sCam.zoom = cam->zoom;
    sCam.collide = cam->collide;
    sCamOut = 1;
}

/* From BtlCam_UpdateView (src/battle/btl_cam.c): `pos` and `rot` are the addresses of the view's pose in game
   memory, just copied from the side's fighter camera. */
/* What the scripts asked of this side's camera besides its pose, for src/battle/btl_cam.c: the zoom (thousandths,
   1000 the game's) and whether the pose is to be kept out of the stage. The game's values when no mod changed it. */
static int sZoom[TAG_ACTOR_CAPACITY], sCollide[TAG_ACTOR_CAPACITY];
int Port_ModsCamZoom(int side) {
    return side >= 0 && side < TAG_ACTOR_CAPACITY && sZoom[side] ? sZoom[side] : 1000;
}
int Port_ModsCamCollide(int side) {
    return side >= 0 && side < TAG_ACTOR_CAPACITY ? sCollide[side] : 0;
}

void Port_ModsCamera(int side, int objId, int pos, int rot) {
    static int synctest = -1;
    uint8_t GAME_PTR chr;

    if (side >= 0 && side < TAG_ACTOR_CAPACITY) {
        sZoom[side] = 1000;
        sCollide[side] = 0;
    }
    if (synctest < 0) {
        synctest = getenv("BT3_SYNCTEST") != NULL;
    }
    if (gPortResim || synctest || side < 0 || side >= TAG_ACTOR_CAPACITY || !Mods_WantCamera()) {
        return;
    }
    chr = (uint8_t GAME_PTR)BtlChar_FindByObjId(objId);
    if (chr == NULL) {
        return;
    }
    memset(&sCam, 0, sizeof(sCam));
    sCam.side = side;
    sCam.view = side;
    {
        extern int TagTeam_Active(void), TagTeam_ViewActor(int), TagTeam_CameraActor(int);
        extern int TagTeam_Team(int), TagTeam_Target(int), BattleSide_GetObjId(int);
        extern int BtlCharApi_GetHp(int);
        int foe;
        sCam.actor = TagTeam_Active() ? TagTeam_CameraActor(TagTeam_ViewActor(side)) : side;
        if (TagTeam_Active()) sCam.side = TagTeam_Team(sCam.actor);
        foe = TagTeam_Active() ? TagTeam_Target(sCam.actor) : 1-side;
        sCam.hp = BtlCharApi_GetHp(objId);
        sCam.foeHp = foe >= 0 ? BtlCharApi_GetHp(BattleSide_GetObjId(foe)) : -1;
    }
    /* the game's own camera work: cuts, fixed and frozen cameras (fighter flags 0xCC, 0xCD, 0xDC, 0xB8) */
    sCam.cut = ChrCam_IsCutActive(chr) || BtlChar_TestFlag(chr, 0xCC) || BtlChar_TestFlag(chr, 0xDC) ||
               BtlChar_TestFlag(chr, 0xB8) || BtlChar_TestFlag(chr, 0xCD);
    rda(sCam.pos, (uint32_t)pos, 3);
    rda(sCam.rot, (uint32_t)rot, 3);
    rdf(sCam.target, chr, CHR_CAM_TARGET, 3);
    sCam.zoom = 1.0f;
    sCam.collide = 1;
    sCamOut = 0;
    Mods_Camera();
    if (sCamOut) {
        wra((uint32_t)pos, sCam.pos, 3);
        wra((uint32_t)rot, sCam.rot, 3);
        sZoom[side] = (int)(sCam.zoom * 1000.0f + 0.5f);
        sCollide[side] = sCam.collide;
    }
}
