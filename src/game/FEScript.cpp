#include "FEText.h"
#include "EXMemory.h"
#include "EXStdFuncs.h"
#include "EXItem.h"
#include "EXItemAnimator.h"
#include "EXItemAnimator_Entity.h"
#include "EXGeoFile.h"
#include "EXQuaternion.h"
#include "EXItemHandler.h"
#include "EXBounds.h"
#include "EXDatum.h"
#include "EXBaseDisplay.h"
#include "EXBaseRenderEnv.h"
#include "FEObjectScript.h"
#include "FEObject.h"

// TODO stand-in: game item (ItemUpdate.obj, game area).
class XItem : public EXItem {
public:
    static XItem *CreateItem(EXGeoFile *pFile, unsigned int hash, EXRuntimeClass *pClass, const EXVector &pos);
};

// TODO stand-in: script animator (EXItemAnimator_Script.obj, engine anim area).
class EXItemAnimator_Script : public EXItemAnimator {
public:
    EXItemAnimator_Script();
    unsigned int ScriptStatus() const { return m_ScriptStatus; }
    void SetScriptStatus(unsigned int s);
    void SetObjectFrame(float f);
    float ObjectFrame() const;
    u8 m_Unk104[0x110 - 0x104];   // TODO layout
    unsigned int m_ScriptStatus;  // 0x110
    u8 m_Unk114[0x148 - 0x114];   // TODO layout
};

// TODO stand-in: script event command (EXGeoScript, engine anim area).
struct GEOSCRIPT_EVENT {
    u32 m_Unk0;         // TODO layout
    short m_Frame;      // 0x04
    u16 m_Length;       // 0x06
    u32 m_Unk8;
    unsigned int m_Hash;  // 0x0c event hash
    u32 m_Unk10;
    union {
        unsigned int u;
        float f;
    } m_Value;             // 0x14
    float m_FParam;        // 0x18
    signed char m_BParam;  // 0x1c
};

EXRuntimeClass *FEObjectHandler::_GetBaseClass() { return EXRUNTIME_CLASS(EXItemHandler); }
const EXRuntimeClass FEObjectHandler::classFEObjectHandler = { EXRUNTIME_CLASS(EXItemHandler), "FEObjectHandler",
                                                                sizeof(FEObjectHandler),
                                                                (void *(__cdecl *)())FEObjectHandler::CreateObject };

static inline EXItemAnimator_Script *ScriptAnimator(EXItem *pItem) { return (EXItemAnimator_Script *)pItem->GetAnimator(); }

FEObjectScript *FEObjectScript::Play(unsigned int marker, unsigned int mode)
{
    if (m_pItem && marker < m_nMarkers) {
        m_State = (m_State & ~0x180000) | mode;
        m_Marker = marker;
        m_Frame = 0.0f;
        for (int i = 0; i < m_nSfx; i++) m_pSfx[i].Reset();
        if (m_State & 0x100000) {
            ScriptAnimator(m_pItem)->SetObjectFrame(m_pMarkers[marker].m_End - 1.0f);
            for (int j = m_nSfx - 1; j >= 0; j--) {
                if (m_pMarkers[marker].m_End - 1.0f >= m_pSfx[j].m_End) {
                    m_Sfx = j;
                    break;
                }
            }
        } else {
            ScriptAnimator(m_pItem)->SetObjectFrame(m_pMarkers[marker].m_Start + 1.0f);
            for (int j = 0; j < m_nSfx; j++) {
                if (m_pSfx[j].m_Start >= m_pMarkers[marker].m_Start) {
                    m_Sfx = j;
                    break;
                }
            }
        }
    }
    return this;
}

FEObjectScript *FEObjectScript::PlayEvent(FEObjEvent::Flags ev)
{
    for (int i = 0; i < m_nMarkers; i++) {
        if (ev == m_pMarkers[i].m_Event) {
            Play(i, 0);
            return this;
        }
    }
    return this;
}

// NEAR: block order/register choice of the reversed-play branch differ
FEObjectScript *FEObjectScript::SetFrame(float f)
{
    if (m_Marker >= m_nMarkers) {
        ScriptAnimator(m_pItem)->SetObjectFrame(f);
        return this;
    }
    EXItemAnimator_Script *pAnim = ScriptAnimator(m_pItem);
    FEScriptMarker *m = &m_pMarkers[m_Marker];
    unsigned int marker = m_Marker;
    if (m_State & 0x100000) {
        if (m->m_Start > m->m_End - f) pAnim->SetObjectFrame(m_pMarkers[marker].m_Start);
        else pAnim->SetObjectFrame(m_pMarkers[marker].m_End - f);
    } else {
        if (m->m_Start + f > m->m_End) pAnim->SetObjectFrame(m_pMarkers[marker].m_End);
        else pAnim->SetObjectFrame(m_pMarkers[marker].m_Start + f);
    }
    return this;
}

FEObjectScript *FEObjectScript::AddMarker(FEScriptMarker &m)
{
    if (m_pMarkers && m_nMarkers < 16) {
        m_pMarkers[m_nMarkers] = m;
        m_nMarkers++;
    }
    return this;
}

FEObjectScript *FEObjectScript::AddSfx(FESfx &s)
{
    if (m_pSfx && m_nSfx < 16) {
        m_pSfx[m_nSfx] = s;
        m_nSfx++;
    }
    return this;
}

FEObjectScript *FEObjectScript::MuteSfx(signed char mute)
{
    for (int i = 0; i < m_nSfx; i++) m_pSfx[i].Mute(mute);
    return this;
}

void FEObjectScript::ForceUpdate()
{
    EXWnd *pOld = EXBaseDisplay::ActiveWnd();
    EXBaseDisplay::SetActiveWnd((EXWnd *)m_pOwner->OwnerWnd());
    if (m_pItem) m_pItem->DoItemUpdate();
    EXBaseDisplay::SetActiveWnd(pOld);
}

void FEObjectScript::BoundsBox(EXBoundsBox &b)
{
    memset(&b, 0, sizeof(b));
    m_pItem->GetAnimatorBoundsBox(b, 0);
}

EXVector FEObjectScript::BoundsBox()
{
    EXBoundsBox b;
    memset(&b, 0, sizeof(b));
    m_pItem->GetAnimatorBoundsBox(b, 0);
    EXMaths::vec_xyzw d = b.vctMax() - b.vctMin();
    return EXVector(d.x, d.y, d.z, d.w);
}

FEObjectScript *FEObjectScript::SetScale(EXVector v)
{
    if (m_pItem) {
        if (EXBaseRenderEnv::WideScreen() && (m_State & 0x1000000)) v.x *= 1.33f;
        XItem *p = m_pItem;
        p->m_scale = v;
        p->Invalidate();
        m_pOwner->m_Flags &= ~0x20;
    }
    return this;
}

FEObjectScript *FEObjectScript::SetPosition(EXVector v)
{
    if (m_pItem) {
        m_pItem->SetPosition(v);
        m_pOwner->m_Flags &= ~0x20;
    }
    return this;
}

FEObjectScript *FEObjectScript::TweakPosition(EXVector v)
{
    if (m_pItem) {
        m_pItem->SetPosition(m_pItem->Position() + v);
        m_pOwner->m_Flags &= ~0x20;
    }
    return this;
}

FEObjectScript *FEObjectScript::TweakRotation(EXVector v)
{
    if (m_pItem) {
        XItem *p = m_pItem;
        p->m_rotation = p->Rotation() + v;
        p->Invalidate();
        m_pOwner->m_Flags &= ~0x20;
    }
    return this;
}

EXVector FEObjectScript::Position()
{
    if (!m_pItem) return EXVector(0.0f, 0.0f, 0.0f, 0.0f);
    m_pOwner->m_Flags &= ~0x20;
    const EXVector &p = m_pItem->Position();
    return EXVector(p.x, p.y, p.z, p.w);
}

void FEObjectScript::SetObjectAlign(EXW::TextAlign a)
{
    m_ObjectAlign = a;
    m_pOwner->m_Flags &= ~0x20;
}

float FEObjectScript::Length()
{
    if (m_Marker >= m_nMarkers) return 1.0f;
    return m_pMarkers[m_Marker].m_End - m_pMarkers[m_Marker].m_Start;
}

// NEAR: xmm0/xmm1 swapped between the clamp constant and f
FEObjectScript *FEObjectScript::SetAmountPlayed(float f)
{
    if (m_Marker < m_nMarkers) {
        if (f < 0.0f) f = 0.0f;
        else if (f > 1.0f) f = 1.0f;
        SetFrame(f * (m_pMarkers[m_Marker].m_End - m_pMarkers[m_Marker].m_Start));
    }
    return this;
}

FEObjectScript *FEObjectScript::SetRemoteMathmos(FEMathmos *p)
{
    m_pRemoteMathmos = p;
    if (p) m_State |= 0x400000;
    else m_State &= ~0x400000;
    return this;
}

// NEAR: register choice for the u64 id push in the 0x1600001e case
unsigned int FEObjectHandler::DoScriptCmdEvent(GEOSCRIPT_EVENT *pEvent, EXItemAnimator *pAnim)
{
    FEObjectHandler *pHandler = (FEObjectHandler *)((EXItem *)pAnim->GetBaseOwner())->ItemHandler();
    if (((EXItemAnimator_Script *)pAnim)->ScriptStatus() == 3) {
        switch (pEvent->m_Hash) {
        case 0x1600001d:
            m_Marker.m_Event = 9;
            m_Marker.m_Start = pEvent->m_Frame;
            break;
        case 0x1600001b:
            m_Marker.m_Event = 0xa;
        case 0x1600001f:
            m_Marker.m_Start = pEvent->m_Frame;
            break;
        case 0x16000020:
            m_Marker.m_Event = 0x1a;
            m_Marker.m_Start = pEvent->m_Frame;
            break;
        case 0x16000021:
            m_Marker.m_UnkC = pEvent->m_Frame;
            m_Marker.m_Unk10 = (int)pEvent->m_Value.f;
            break;
        case 0x1600001c:
            m_Marker.m_Unk8 = pEvent->m_Frame;
            break;
        case 0x1600001e:
            m_Marker.m_End = pEvent->m_Frame;
            pHandler->Owner()->Script(m_Id)->AddMarker(m_Marker);
            m_Marker.Reset();
            break;
        case 0x16000040:
            m_Sfx.m_Start = pEvent->m_Frame;
            m_Sfx.m_End = pEvent->m_Length + pEvent->m_Frame;
            m_Sfx.m_Hash = pEvent->m_Value.u;
            m_Sfx.m_Volume = (int)pEvent->m_FParam;
            m_Sfx.m_Unk11 = pEvent->m_BParam;
            pHandler->Owner()->Script(m_Id)->AddSfx(m_Sfx);
            break;
        case 0xffffffff:
            ((EXItemAnimator_Script *)pAnim)->SetScriptStatus(0);
            break;
        }
    }
    return 0;
}

FEObjectScript *FEObjectScript::SetTextureSwap(unsigned int a, unsigned int b, unsigned int c)
{
    m_pItem->m_renderInfo.SetTexSwapInfo(&m_TexSwap);
    m_TexSwap.SetSwap(a, b, c);
    return this;
}

EXVector FEObjectScript::GetDatumPos(unsigned int hash)
{
    EXDatumTemp temp;
    memset(&temp, 0, sizeof(temp));
    EXVector pos(-100.0f, -100.0f, -100.0f, 0.0f);
    if (!m_pItem) return pos;
    for (unsigned int i = 0; i < m_nDatums; i++)
        if (m_Datums[i].Hashcode() == hash) return m_Datums[i].m_Pos;
    temp.m_position = pos;
    m_pItem->GetAnimatorDatum(hash, temp, 0, 0);
    temp.Get(pos);
    if (m_nDatums < 32) {
        if (!((float)fabs(pos.x - -100.0f) < 0.001f && (float)fabs(pos.y - -100.0f) < 0.001f &&
              (float)fabs(pos.z - -100.0f) < 0.001f && (float)fabs(pos.w) < 0.001f)) {
            m_Datums[m_nDatums].Set(hash, pos);
            m_nDatums++;
        }
    }
    return pos;
}

void FEObjectScript::EnableRefraction(signed char b)
{
    if (b != m_Refraction) {
        m_pItem->m_renderInfo.EnableRefractionRender(b);
        if (b) m_ExtRenderInfo.SetRefractionStrength(0.5f);
        m_Refraction = b;
    }
}

FEObjectScript *FEObjectScript::SetScale(float f)
{
    float x = f;
    XItem *p = m_pItem;
    if (p) {
        if (EXBaseRenderEnv::WideScreen() && (m_State & 0x1000000)) x *= 1.33f;
        p->m_scale.x = x;
        p->m_scale.y = f;
        p->m_scale.z = f;
        p->m_scale.w = 0.0f;
        p->Invalidate();
        m_pOwner->m_Flags &= ~0x20;
    }
    return this;
}

float FEObjectScript::AmountPlayed()
{
    if (Length() < 1.0f) return 0.0f;
    return max(0.0f, min(1.0f, m_Frame / Length()));
}

float FEObjectScript::AmountPlayed(float f)
{
    if (Length() < 1.0f) return 0.0f;
    float frame = Length() * f;
    return max(0.0f, min(1.0f, frame / Length()));
}

FEObjectScript *FEObjectScript::MoveToAmountPlayed(float f)
{
    if (m_Marker >= m_nMarkers) return this;
    if (AmountPlayed(f) == AmountPlayed()) return this;
    if (f < 0.0f) f = 0.0f;
    else if (f > 1.0f) f = 1.0f;
    m_Unk3b0 = 1;
    m_Unk3b4 = AmountPlayed(f);
    m_State &= ~0x80000;
    if (AmountPlayed() > f) m_State |= 0x100000;
    else m_State &= ~0x100000;
    return this;
}

FEObjectHandler *FEObjectHandler::CreateObject() { return new FEObjectHandler; }

// NEAR: 1 byte, inlined EXStack dtor addresses [ecx+ebx] where retail has [ebx+ecx]
FEObjectScript::~FEObjectScript()
{
    for (int i = 0; i < m_nSfx; i++) m_pSfx[i].Reset();
    if (m_pMarkers) delete[] m_pMarkers;
    m_pMarkers = 0;
    delete[] m_pSfx;
    m_pSfx = 0;
    delete m_pItem;
    m_pItem = 0;
}

FEObjectScript::FEObjectScript(unsigned __int64 id, FEObject *pOwner, FEObjType::Flags type, unsigned int file,
                               unsigned int hash)
    : m_StateStack(0x80)
{
    m_Id = id;
    m_pMarkers = 0;
    m_nMarkers = 0;
    m_pSfx = 0;
    m_nSfx = 0;
    m_ObjectAlign = (EXW::TextAlign)4;
    m_AttachedTo = 0xfeffffff;
    m_pOwner = pOwner;
    m_Marker = 0xfeffffff;
    m_Sfx = 0xfeffffff;
    m_pItem = XItem::CreateItem(0, 0, (EXRuntimeClass *)&FEObjectHandler::classFEObjectHandler, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    m_nDatums = 0;
    m_pRemoteMathmos = 0;
    m_Refraction = 0;
    m_Mathmos = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    m_Unk3b0 = 0;
    m_Unk3b4 = 0.0f;
    m_State = 0x1000040;
    ((FEObjectHandler *)m_pItem->ItemHandler())->SetID(id);
    ((FEObjectHandler *)m_pItem->ItemHandler())->SetOwner(pOwner);
    m_pItem->SetPosition(EXVector(0.0f, 0.0f, 150.0f, 0.0f));
    XItem *pItem = m_pItem;
    pItem->m_rotation.x = 0.0f;
    pItem->m_rotation.y = 0.0f;
    pItem->m_rotation.z = 0.0f;
    pItem->m_rotation.w = 0.0f;
    pItem->Invalidate();
    if (file && hash) {
        if (type == 3) {
            EXItemAnimator_Entity *pEntity = new EXItemAnimator_Entity;
            EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
            pEntity->Initialise(pFile, hash);
            m_pItem->AddAnimator(pEntity);
            m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwap);
            m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
        }
        EXItemAnimator_Script *pScript = new EXItemAnimator_Script;
        EXGeoFile *pFile = EXGeoFile::LoadGeoFile(file, 0, 0);
        pScript->Initialise(pFile, hash);
        m_pItem->AddAnimator(pScript);
        m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwap);
        m_pItem->RenderInfo().SetExtRenderInfo(&m_ExtRenderInfo);
        m_State |= 0x200000;
        m_pMarkers = new FEScriptMarker[16];
        m_pSfx = new FESfx[16];
        pScript->SetScriptStatus(3);
        EXExtRenderInfo *pExt = &m_ExtRenderInfo;
        pExt->m_Light[0].set(0.0f, 0.0f, 0.0f, 0.0f);
        pExt->m_Light[1].set(0.0f, 0.0f, 0.0f, 0.0f);
        pExt->m_Light[2].set(0.0f, 0.0f, 0.0f, 0.0f);
        pExt->m_Light[3].set(0.0f, 0.0f, 0.0f, 0.0f);
        pExt->m_Light[4].set(0.0f, 0.0f, 0.0f, 0.0f);
        pExt->m_Light[5].set(0.0f, 0.0f, 0.0f, 0.0f);
        pExt->m_Ambient.set(0.5f, 0.5f, 0.5f, 0.0f);
    }
}

void FEObjectScript::Render(ex::RGBA c)
{
    if (!(m_State & 0x40)) return;
    if ((m_State & 0x400000) && m_pRemoteMathmos) c = m_pRemoteMathmos->m_Colour;
    EXRenderInfo &ri = m_pItem->m_renderInfo;
    ri.EnableRGBMult(1);
    float k = 1.0f / 128.0f;
    float a = (float)c.m_A * k;
    ri.EnableAlphaMult(a < 0.95f);
    ri.m_rgbaMult.x = (float)c.m_R * k;
    ri.m_rgbaMult.y = (float)c.m_G * k;
    ri.m_rgbaMult.z = (float)c.m_B * k;
    ri.m_rgbaMult.w = a;
    EXMatrix mtx;
    mtx.SetIdentity();
    m_pItem->DoItemRender(mtx, ri);
}

// Scales the script so its projected bounds are w x h pixels (uniformly to fit when b is set).
FEObjectScript *FEObjectScript::SetSize(int w, int h, signed char b)
{
    if (m_pOwner) {
        m_State &= ~0x1000000;
        m_pOwner->m_pOwnerWnd->SetupCamera(0);
        EXBoundsBox box;
        memset(&box, 0, sizeof(box));
        m_pItem->GetAnimatorBoundsBox(box, 0);
        EXPoint p0 = m_pOwner->m_pOwnerWnd->WorldToWnd(EXVector(box.vctMin().x, box.vctMin().y, box.vctMin().z, box.vctMin().w));
        EXPoint p1 = m_pOwner->m_pOwnerWnd->WorldToWnd(EXVector(box.vctMax().x, box.vctMax().y, box.vctMax().z, box.vctMax().w));
        int sw = (p0.x > p1.x ? p0.x : p1.x) - (p0.x < p1.x ? p0.x : p1.x);
        int sh = (p0.y > p1.y ? p0.y : p1.y) - (p0.y < p1.y ? p0.y : p1.y);
        if (b) {
            int m = sw > sh ? sh : sw;
            float k = 1.0f / (float)m;
            SetScale(EXVector((float)w * k, (float)h * k, 1.0f, 0.0f));
        } else {
            SetScale(EXVector((float)w / (float)sw, (float)h / (float)sh, 1.0f, 0.0f));
        }
    }
    return this;
}

// TODO stand-ins: only what Update needs of the app state (XApp.h conflicts with the XItem stand-in above).
class XGameState {
public:
    int m_State;
};
class XApp {
public:
    XGameState *GameStateCurrent();
};
extern XApp theApp;

// Advances the script's frame, fires its sounds and markers, and handles looping/stopping.
// NB: the early-out paths leave the owner's window as the active window, as retail does.
void FEObjectScript::Update()
{
    EXWnd *pOld = EXBaseDisplay::ActiveWnd();
    EXBaseDisplay::SetActiveWnd((EXWnd *)m_pOwner->OwnerWnd());
    if (m_State & 0x880000) return;
    if ((m_State & 0x2000000) && theApp.GameStateCurrent()->m_State != 9) return;
    if (m_Marker == 0xfeffffff) return;
    if (m_pItem) m_pItem->DoItemUpdate();
    EXItemAnimator_Script *pAnim = ScriptAnimator(m_pItem);
    float dt = pAnim->FrameRateMult() * 2.0f * EXBaseApp::GameFrameRateMul();
    float dtEps = dt + 0.001f;
    m_Frame += dt;
    m_Mathmos.Update();
    if (m_State & 0x100000) {
        pAnim->SetObjectFrame(m_pMarkers[m_Marker].m_End - m_Frame + dtEps);
        if (m_pMarkers[m_Marker].m_UnkC >= 0.0f && m_pMarkers[m_Marker].m_UnkC >= pAnim->ObjectFrame())
            Play(m_pMarkers[m_Marker].m_Unk10, 0);
        for (int i = (int)m_Sfx; i >= 0; i--) {
            if (m_pMarkers[m_Marker].m_Start > (float)i) break;
            float f = pAnim->ObjectFrame();
            if (f <= m_pSfx[i].m_End) m_pSfx[i].Play();
            if (f <= m_pSfx[i].m_Start) m_pSfx[i].Stop();
        }
        if (m_Unk3b0) {
            if (m_Unk3b4 >= AmountPlayed()) {
                SetAmountPlayed(m_Unk3b4);
                m_State |= 0x80000;
                m_Unk3b4 = 0.0f;
                m_Unk3b0 = 0;
            }
        }
        if (pAnim->ObjectFrame() < m_pMarkers[m_Marker].m_Start) {
            if (m_pMarkers[m_Marker].m_Unk8 >= 0.0f) {
                pAnim->SetObjectFrame(m_pMarkers[m_Marker].m_Unk8 - 1.0f);
            } else {
                pAnim->SetObjectFrame(m_pMarkers[m_Marker].m_Start);
                m_State |= 0x80000;
            }
            if (m_pMarkers[m_Marker].m_Unk8 >= 0.0f) m_Frame = m_pMarkers[m_Marker].m_End - m_pMarkers[m_Marker].m_Unk8 - 1.0f;
        }
    } else {
        pAnim->SetObjectFrame(m_pMarkers[m_Marker].m_Start + m_Frame - dtEps);
        if (m_pMarkers[m_Marker].m_UnkC >= 0.0f && pAnim->ObjectFrame() >= m_pMarkers[m_Marker].m_UnkC)
            Play(m_pMarkers[m_Marker].m_Unk10, 0);
        for (unsigned int i = m_Sfx; i < (unsigned int)m_nSfx; i++) {
            if ((float)i > m_pMarkers[m_Marker].m_End) break;
            float f = pAnim->ObjectFrame();
            if (f >= m_pSfx[i].m_Start) m_pSfx[i].Play();
            if (f >= m_pSfx[i].m_End) m_pSfx[i].Stop();
        }
        if (m_Unk3b0) {
            if (AmountPlayed() >= m_Unk3b4) {
                SetAmountPlayed(m_Unk3b4);
                m_State |= 0x80000;
                m_Unk3b4 = 0.0f;
                m_Unk3b0 = 0;
            }
        }
        if (pAnim->ObjectFrame() > m_pMarkers[m_Marker].m_End) {
            if (m_pMarkers[m_Marker].m_Unk8 >= 0.0f) {
                pAnim->SetObjectFrame(m_pMarkers[m_Marker].m_Unk8);
            } else {
                pAnim->SetObjectFrame(m_pMarkers[m_Marker].m_End);
                m_State |= 0x80000;
            }
            if (m_pMarkers[m_Marker].m_Unk8 >= 0.0f) m_Frame = m_pMarkers[m_Marker].m_Unk8 - m_pMarkers[m_Marker].m_Start;
        }
    }
    EXBaseDisplay::SetActiveWnd(pOld);
}

// Positions the script so its projected bounds sit at the m_ObjectAlign anchor of r. Objects flagged 0x80000000
// are moved iteratively (halving the step on overshoot) until the anchor edge lines up.
void FEObjectScript::DoObjectAlign(EXRect r)
{
    if (!m_pOwner) return;
    if (m_State & 0x10) return;
    m_nDatums = 0;
    if ((int)m_pOwner->m_Flags >= 0) {
        EXBoundsBox box;
        BoundsBox(box);
        EXPoint p0 = m_pOwner->m_pOwnerWnd->WorldToWnd(EXVector(box.vctMin().x, box.vctMin().y, box.vctMin().z, box.vctMin().w));
        EXPoint p1 = m_pOwner->m_pOwnerWnd->WorldToWnd(EXVector(box.vctMax().x, box.vctMax().y, box.vctMax().z, box.vctMax().w));
        int minX = p0.x <= p1.x ? p0.x : p1.x, minY = p0.y <= p1.y ? p0.y : p1.y;
        int maxX = p0.x >= p1.x ? p0.x : p1.x, maxY = p0.y >= p1.y ? p0.y : p1.y;
        int halfW = (maxX - minX) / 2, halfH = (maxY - minY) / 2;
        int x = 0, y = 0;
        switch ((int)m_ObjectAlign) {
        case 0: x = halfW + r.x; y = halfH + r.y; break;
        case 1: x = halfW + r.x; y = r.h - halfH + r.y; break;
        case 2: x = halfW + r.x; y = r.h / 2 + r.y; break;
        case 3: x = r.w / 2 + r.x; y = halfH + r.y; break;
        case 4: x = r.w / 2 + r.x; y = r.h / 2 + r.y; break;
        case 5: x = r.w / 2 + r.x; y = r.h - halfH + r.y; break;
        case 6: x = r.w - halfW + r.x; y = halfH + r.y; break;
        case 7: x = r.w - halfW + r.x; y = r.h / 2 + r.y; break;
        case 8: x = r.w - halfW + r.x; y = r.h - halfH + r.y; break;
        }
        EXVector w = m_pOwner->m_pOwnerWnd->WndToWorld(EXPoint(x, y));
        EXVector cur = Position();
        SetPosition(EXVector(w.x, w.y, cur.z, w.w));
        return;
    }
    float stepX = 1.0f, stepY = 1.0f;
    unsigned int n = 0;
    m_pOwner->m_pOwnerWnd->SetupCamera(0);
    do {
        EXBoundsBox box;
        memset(&box, 0, sizeof(box));
        n++;
        m_pItem->GetAnimatorBoundsBox(box, 0);
        EXPoint p0 = m_pOwner->m_pOwnerWnd->WorldToWnd(EXVector(box.vctMin().x, box.vctMin().y, box.vctMin().z, box.vctMin().w));
        EXPoint p1 = m_pOwner->m_pOwnerWnd->WorldToWnd(EXVector(box.vctMax().x, box.vctMax().y, box.vctMax().z, box.vctMax().w));
        int minX = p0.x <= p1.x ? p0.x : p1.x, minY = p0.y <= p1.y ? p0.y : p1.y;
        int maxX = p0.x >= p1.x ? p0.x : p1.x, maxY = p0.y >= p1.y ? p0.y : p1.y;
        int halfW = (maxX - minX) / 2, halfH = (maxY - minY) / 2;
        int cx = 0, cy = 0, tx = 0, ty = 0;
        switch ((int)m_ObjectAlign) {
        case 0: cx = minX; cy = minY; tx = r.x; ty = r.y; break;
        case 1: cx = minX; cy = maxY; tx = r.x; ty = r.y + r.h; break;
        case 2: cx = minX; cy = halfH + minY; tx = r.x; ty = r.h / 2 + r.y; break;
        case 3: cx = halfW + minX; cy = minY; tx = r.w / 2 + r.x; ty = r.y; break;
        case 4: cx = halfW + minX; cy = halfH + minY; tx = r.w / 2 + r.x; ty = r.h / 2 + r.y; break;
        case 5: cx = halfW + minX; cy = maxY; tx = r.w / 2 + r.x; ty = r.y + r.h; break;
        case 6: cx = maxX; cy = minY; tx = r.w + r.x; ty = r.y; break;
        case 7: cx = maxX; cy = halfH + minY; tx = r.w + r.x; ty = r.h / 2 + r.y; break;
        case 8: cx = maxX; cy = maxY; tx = r.w + r.x; ty = r.y + r.h; break;
        }
        if (cx < tx) {
            if (stepX > 0.0f) TweakPosition(EXVector(stepX, 0.0f, 0.0f, 0.0f));
            else stepX *= -0.5f;
        }
        if (cx > tx) {
            if (0.0f > stepX) TweakPosition(EXVector(stepX, 0.0f, 0.0f, 0.0f));
            else stepX *= -0.5f;
        }
        if (cx == tx) stepX = 0.0f;
        if (cy < ty) {
            if (stepY > 0.0f) TweakPosition(EXVector(0.0f, -stepY, 0.0f, 0.0f));
            else stepY *= -0.5f;
        }
        if (cy > ty) {
            if (0.0f > stepY) TweakPosition(EXVector(0.0f, -stepY, 0.0f, 0.0f));
            else stepY *= -0.5f;
        }
        if (cy == ty) stepY = 0.0f;
    } while ((stepX != 0.0f || stepY != 0.0f) && n < 0x32);
}
