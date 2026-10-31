#ifndef FX_GORE_H
#define FX_GORE_H
#include "types.h"

class Actor;
class XItem;
class EXVector;

// Blood and gore effects (FX_Gore.obj).
int Gore_GetBloodGroup(Actor *pActor);
void Gore_Initialise();
void Gore_GetBloodColour(int group, float &r, float &g, float &b);
void Gore_GetVisionModeBloodColour(unsigned char mode, float &r, float &g, float &b);
void Gore_GetBloodColour(Actor *pActor, float &r, float &g, float &b);
void Gore_ShootSplat(EXVector &a, EXVector &b, int group);
void Gore_ShootSplat(EXVector &a, EXVector &b, Actor *pActor);
void Gore_BloodPool(EXVector &pos, int group);
void Gore_BloodPool(EXVector &pos, Actor *pActor);
void Gore_TintToBloodColour(XItem *pItem, int group);
void Gore_TintToBloodColour(XItem *pItem, Actor *pActor);
void Gore_TintToBloodColourRGB(XItem *pItem, float r, float g, float b);
void Gore_CreateOverkillBlood(Actor *pActor);
void Gore_Bloodcleaner();
void Gore_CreateActorBlood(Actor *pActor, signed char a, signed char b, int c, EXVector &d, EXVector &e, signed char f);
#endif
