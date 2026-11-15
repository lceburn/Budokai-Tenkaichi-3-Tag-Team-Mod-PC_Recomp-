#ifndef TAG_DAMAGE_H
#define TAG_DAMAGE_H
/* Disc costumes supply one normal and one damaged model. Apply damage once,
   at or below half health; healing does not repair clothing mid-match. */
static inline int TagDamage_ShouldChange(int health,int maximum,int damaged,int sturdy) {
    return maximum>0 && health>0 && health<=maximum/2 && !damaged && !sturdy;
}
#define BTL_CHANGE_DAMAGE_SLOT (-1)
#endif
