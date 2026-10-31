#ifndef FEOBJECTSCRIPT_H
#define FEOBJECTSCRIPT_H
#include "types.h"
#include "EXStdFuncs.h"
#include "EXList.h"
#include "EXRect.h"
#include "EXVector.h"
#include "EXBaseWnd.h"
#include "EXStack.h"
#include "EXSoundTagStandIn.h"
#include "RGBA.h"
#include "FEFlags.h"
#include "FEMathmos.h"
#include "EXItemHandler.h"

class FEObject;
class XItem;
struct EXBoundsBox;

#include "EXExtRenderInfoStandIn.h"

// TODO stand-in: texture swap table (EXRenderEnv.obj, engine render area). sizeof 0x14.
#include "EXTexSwapInfo.h"

// Frame range of one scripted event (FEScript.obj); sizeof 0x18.
class FEScriptMarker {
public:
    FEScriptMarker() { Reset(); }
    void Reset()
    {
        m_Start = -1.0f;
        m_End = -1.0f;
        m_Unk8 = -1.0f;
        m_UnkC = -1.0f;
        m_Unk10 = -1;
        m_Event = 0;
    }
    float m_Start;         // 0x00 first frame
    float m_End;           // 0x04 last frame
    float m_Unk8;          // 0x08
    float m_UnkC;          // 0x0c
    int m_Unk10;           // 0x10
    unsigned int m_Event;  // 0x14 FEObjEvent that plays it
};

// Sound attached to a script frame range (FEScript.obj); sizeof 0x1c.
class FESfx {
public:
    FESfx()
    {
        m_Start = -1.0f;
        m_End = -1.0f;
        m_Hash = 0;
        m_Volume = -1;
        m_Playing = 0;
        m_Unk11 = 0;
    }
    ~FESfx() { Stop(); }
    void Play()
    {
        if (!m_Playing) {
            m_Tag.StartSound(m_Hash, EXVector(0.0f, 0.0f, 0.0f, 0.0f), m_Volume, 0);
            m_Playing = 1;
        }
    }
    void Stop()
    {
        m_Tag.RemoveAll(1);
        m_Tag.KillSoundPlayingLooping();
    }
    void Mute(signed char b)
    {
        if (b) m_Volume = 0;
        else m_Volume = -1;
        m_Tag.SetVolume(m_Volume);
    }
    void Reset()
    {
        Stop();
        m_Playing = 0;
    }
    float m_Start;          // 0x00
    float m_End;            // 0x04
    unsigned int m_Hash;    // 0x08
    int m_Volume;           // 0x0c
    signed char m_Playing;  // 0x10
    signed char m_Unk11;    // 0x11
    EXSoundTag m_Tag;       // 0x14
};

// Cached datum position (FEScript.obj); sizeof 0x14.
class FEDatumCache {
public:
    FEDatumCache()
    {
        m_Hash = 0;
        m_Pos.set(-100.0f, -100.0f, -100.0f, 0.0f);
    }
    void Set(unsigned int hash, EXVector v)
    {
        m_Hash = hash;
        m_Pos.x = v.x;
        m_Pos.y = v.y;
        m_Pos.z = v.z;
        m_Pos.w = v.w;
    }
    unsigned int Hashcode() { return m_Hash; }
    EXVector Position() { return EXVector(m_Pos.x, m_Pos.y, m_Pos.z, m_Pos.w); }
    unsigned int m_Hash;  // 0x00
    EXVector m_Pos;       // 0x04
};

// Animated 3D script attached to an FEObject (FEScript.obj). The EXDListItem base (at +8, after the
// vptr and alignment for m_Id) links it into the owner's script list. sizeof 0x3b8.
// m_State: 0x40 visible, 0x100000 play backwards, 0x80000/0x100000 play mode, 0x200000 has script,
// 0x400000 remote mathmos, 0x1000000 widescreen-corrected scale.
class FEObjectScript : public EXDListItem {
public:
    virtual ~FEObjectScript();
    FEObjectScript(unsigned __int64 id, FEObject *pOwner, FEObjType::Flags type, unsigned int file, unsigned int hash);

    unsigned int State() { return m_State; }
    void StateSet(unsigned int s) { m_State = s; }
    void StateAdd(unsigned int s) { m_State |= s; }
    void StateSub(unsigned int s) { m_State &= ~s; }
    void Stop() { m_State |= 0x80000; }
    unsigned __int64 ID() { return m_Id; }
    unsigned __int64 AttachedTo() { return m_AttachedTo; }
    void SetAttachedTo(unsigned __int64 id) { m_AttachedTo = id; }
    XItem *GetXItem() { return m_pItem; }

    FEObjectScript *Play(unsigned int marker, unsigned int mode);
    FEObjectScript *PlayEvent(FEObjEvent::Flags ev);
    FEObjectScript *SetFrame(float f);
    FEObjectScript *AddMarker(FEScriptMarker &m);
    FEObjectScript *AddSfx(FESfx &s);
    FEObjectScript *MuteSfx(signed char mute);
    void ForceUpdate();
    void BoundsBox(EXBoundsBox &b);
    EXVector BoundsBox();
    FEObjectScript *SetScale(EXVector v);
    FEObjectScript *SetPosition(EXVector v);
    FEObjectScript *TweakPosition(EXVector v);
    FEObjectScript *TweakRotation(EXVector v);
    EXVector Position();
    void SetObjectAlign(EXW::TextAlign a);
    float Length();
    FEObjectScript *SetAmountPlayed(float f);
    FEObjectScript *SetRemoteMathmos(FEMathmos *p);
    // TODO name: copies a tint into the script's own mathmos and uses it (inlined in Actor.obj).
    FEObjectScript *SetMathmos(FEMathmos m)
    {
        m_Mathmos = m;
        return SetRemoteMathmos(&m_Mathmos);
    }
    FEObjectScript *SetTextureSwap(unsigned int a, unsigned int b, unsigned int c);
    EXVector GetDatumPos(unsigned int hash);
    void EnableRefraction(signed char b);
    void Render(ex::RGBA c);
    FEObjectScript *SetScale(float f);
    FEObjectScript *SetSize(int w, int h, signed char b);
    void DoObjectAlign(EXRect r);
    float AmountPlayed();
    float AmountPlayed(float f);
    FEObjectScript *MoveToAmountPlayed(float f);
    void Update();

    unsigned __int64 m_Id;               // 0x010
    signed char m_Refraction;            // 0x018
    FEObject *m_pOwner;                  // 0x01c
    XItem *m_pItem;                      // 0x020
    FEScriptMarker *m_pMarkers;          // 0x024 [16]
    int m_nMarkers;                      // 0x028
    FESfx *m_pSfx;                       // 0x02c [16]
    int m_nSfx;                          // 0x030
    EXW::TextAlign m_ObjectAlign;        // 0x034
    unsigned __int64 m_AttachedTo;       // 0x038
    unsigned int m_Marker;               // 0x040 current marker
    unsigned int m_Sfx;                  // 0x044 next sound to trigger
    unsigned int m_State;                // 0x048
    EXStack<unsigned int> m_StateStack;  // 0x04c
    float m_Frame;                       // 0x068 time into the current marker
    FEDatumCache m_Datums[32];           // 0x06c
    unsigned int m_nDatums;              // 0x2ec
    EXExtRenderInfo m_ExtRenderInfo;     // 0x2f0
    EXTexSwapInfo m_TexSwap;             // 0x370
    FEMathmos *m_pRemoteMathmos;         // 0x384
    FEMathmos m_Mathmos;                 // 0x388
    signed char m_Unk3b0;                // 0x3b0
    float m_Unk3b4;                      // 0x3b4
};
// Item handler of an FEObjectScript's XItem: collects markers and sounds from script events
// (FEScript.obj). sizeof 0x48.
class FEObjectHandler : public EXItemHandler {
    EX_DECLARE_DYNAMIC(FEObjectHandler)
public:
    virtual signed char Initialise() { return 1; }
    virtual signed char DoUpdate() { return 1; }
    virtual unsigned int DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim);
    FEObjectHandler() {}
    static FEObjectHandler *CreateObject();
    void SetOwner(FEObject *p) { m_pOwner = p; }
    void SetID(unsigned __int64 id) { m_Id = id; }
    FEObject *Owner() { return m_pOwner; }

    unsigned __int64 m_Id;     // 0x08
    FEObject *m_pOwner;        // 0x10
    FEScriptMarker m_Marker;   // 0x14 marker being built from events
    FESfx m_Sfx;               // 0x2c
};

#endif
