#ifndef XITEMHANDLER_SCRIPTBASE_H
#define XITEMHANDLER_SCRIPTBASE_H
#include "EXItemHandler.h"

class XItem;
class XItemList;
class EXVector;
class EXItemAnimator;
struct GEOSCRIPT_EVENT;

// One-shot script effect item (Bullet_Explosion.obj). sizeof 0x28.
// m_Flags: 1 in gBloodTintList (blood colour tint), 2 vision-mode tint, 4 linked to a CSMF animator.
class XItemHandler_ScriptBase : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_ScriptBase)
public:
    virtual ~XItemHandler_ScriptBase();
    virtual signed char Initialise();
    virtual signed char DoUpdate();
    virtual unsigned int DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
    XItemHandler_ScriptBase();
    void SetBloodTintColour(int mode);
    static void TintAllBloodToVisionMode(int mode);
    static XItemHandler_ScriptBase *CreateObject();
    void DeleteIfCSMF();
    static XItem *FX_CreateLink(unsigned int script, int actor, unsigned int datum, int life, unsigned int file);
    static XItem *FX_CreateLinkBlood(unsigned int script, int actor, unsigned int datum, int group, int life);
    static XItem *FX_Create(EXVector &pos, unsigned int script, int life, unsigned int file);
    static XItem *FX_CreateBlood(EXVector &pos, unsigned int script, int group, int life);
    static XItem *FX_CreateCSMF(unsigned int script, EXItemAnimator *pAnim, unsigned int datum, int life, unsigned int file);
    static XItem *Explosion_Create(EXVector &pos, unsigned int a, int b, int c, int d, unsigned int e, signed char f, int g, signed char h);
    int m_Life;                 // 0x08 frames left (<= 0: forever)
    int m_ActorID;              // 0x0c actor to follow (<= 0: the player)
    EXItemAnimator *m_pAnim;    // 0x10 CSMF animator to follow
    unsigned int m_DatumHash;   // 0x14
    u32 m_Flags;                // 0x18
    float m_R;                  // 0x1c blood colour
    float m_G;                  // 0x20
    float m_B;                  // 0x24
};

extern XItemList gBloodTintList;
void Remove_CSMF();
signed char CheckLineOfSight(const EXVector &a, const EXVector &b);
#endif
