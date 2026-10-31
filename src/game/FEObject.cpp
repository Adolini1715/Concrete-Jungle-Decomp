#include "FEText.h"
#include "EXMemory.h"
#include "EXBaseDisplay.h"
#include "XAppStandIn.h"
#include "FEObject.h"
#include "FEObjectScript.h"
#include "FEKeyframe.h"

const EXString FE_Blank = "";

void FEObject::InitRect(int x, int y, int w, int h)
{
    if (w && h) {
        m_Flags |= 2;
        m_Rect.Set(x, y, w, h);
        m_BaseRect.Set(x, y, w, h);
        if (x || y) m_Flags |= 0x10;
    } else {
        m_Flags &= ~2;
    }
    m_Flags &= ~0x20;
    Resize();
}

FEObjectScript *FEObject::Script(unsigned __int64 id)
{
    for (EXDListItem *p = m_Scripts.Head(); p; p = p->Next()) {
        FEObjectScript *pScript = static_cast<FEObjectScript *>(p);
        if (pScript->ID() == id) return pScript;
    }
    return 0;
}

void FEObject::Pause(signed char pause)
{
    m_Flags = pause ? (m_Flags | 0x800000) : (m_Flags & ~0x800000);
    for (EXDListItem *p = m_Scripts.Head(); p; p = p->Next()) {
        FEObjectScript *pScript = static_cast<FEObjectScript *>(p);
        pScript->MuteSfx(pause);
        pScript->m_State = pause ? (pScript->m_State | 0x800000) : (pScript->m_State & ~0x800000);
    }
}

FEObject *FEObject::ListenerAdd(FEListener *pListener)
{
    if (ChildCount() > 0) pListener->SetFocus(ChildSel(), 0);
    pListener->SetOwner(this);
    m_Listeners.AddTail(pListener);
    return this;
}

FEObject *FEObject::ListenerChildAdd(FEListener listener)
{
    for (unsigned int i = 0; i < ChildCount(); i++) {
        if (Object(i)) Object(i)->ListenerAdd(new FEListener(&listener));
    }
    return this;
}

FEListener *FEObject::Listener(unsigned __int64 id)
{
    for (EXDListItem *p = m_Listeners.Head(); p; p = p->Next()) {
        FEListener *pListener = static_cast<FEListener *>(p);
        if (pListener->ID() == id) return pListener;
    }
    if (m_pParent) return m_pParent->Listener(id);
    return static_cast<FEListener *>(m_Listeners.Head());
}

FEObject *FEObject::ListenerClear()
{
    while (m_Listeners.Head()) {
        FEListener *p = static_cast<FEListener *>(m_Listeners.RemoveHead());
        delete p;
    }
    return this;
}

void FEObject::NotifyDisplay(signed char show)
{
    if (show) {
        MathmosGoto(FEMathmosPos::MP_END);
        DoEventCallback(FEObjEvent::OE_SHOW);
    } else {
        DoEventCallback(FEObjEvent::OE_HIDE);
    }
}

void FEObject::PlayEvent(FEObjEvent::Flags ev)
{
    for (EXDListItem *p = m_Scripts.Head(); p; p = p->Next()) static_cast<FEObjectScript *>(p)->PlayEvent(ev);
}

FEObject *FEObject::LockTextRect(int w, int h, int padX, int padY)
{
    if (m_Text.Length()) {
        if (!w) m_Rect.w = 0x7fff;
        else if (w > 0) m_Rect.w = w;
        if (!h) m_Rect.h = 0x7fff;
        else if (h > 0) m_Rect.h = h;
        m_pOwnerWnd->SelectFont(m_Font[0], m_Font[1]);
        EXRect r = Rect();
        EXPoint size = m_pOwnerWnd->FontTextSize(m_Text, r, 1.0f);
        if (!w) m_Rect.w = size.x + padX + 1;
        if (!h) m_Rect.h = size.y + padY + 1;
        m_Flags |= 2;
    }
    return this;
}

FEObject *FEObject::LockTextRectToParent()
{
    LockTextRect(m_pParent->Rect().w, 0, 0, 0);
    return this;
}

FEObject *FEObject::SetMathmos(FEObjColour::Flags which, FEMathmos m, signed char b)
{
    if (which < FEObjColour::OC_COUNT) {
        m_Mathmos[which] = m;
        m_pRemoteMathmos[which] = 0;
        MathmosUpdate();
    }
    return this;
}

FEObject *FEObject::SetRemoteMathmos(FEObjColour::Flags which, FEMathmos *p, signed char b)
{
    if (which < FEObjColour::OC_COUNT) m_pRemoteMathmos[which] = p;
    return this;
}

FEMathmos *FEObject::Mathmos(FEObjColour::Flags which)
{
    return m_pRemoteMathmos[which] ? m_pRemoteMathmos[which] : &m_Mathmos[which];
}

FEMathmos *FEObject::RemoteMathmos(FEObjColour::Flags which) { return m_pRemoteMathmos[which]; }

FEObject *FEObject::MathmosGoto(FEMathmosPos::Flags pos)
{
    for (int i = 0; i < FEObjColour::OC_COUNT; i++) m_Mathmos[i].Goto(pos);
    return this;
}

// NEAR: retail reloads m_Flags after every store
FEObject *FEObject::SetFocus(signed char focus, signed char b, signed char c)
{
    if (m_Flags & 8) {
        m_Flags = focus ? (m_Flags | 4) : (m_Flags & ~4);
        m_Flags = c ? (m_Flags | 0x2000) : (m_Flags & ~0x2000);
        FEObjEvent::Flags ev = focus ? FEObjEvent::OE_FOCUSON : FEObjEvent::OE_FOCUSOFF;
        if (!(m_Flags & 0x2000) && focus) EnsureVisible(0, b);
        DoEventCallback(ev);
        for (EXDListItem *p = m_Scripts.Head(); p; p = p->Next()) static_cast<FEObjectScript *>(p)->PlayEvent(ev);
        (focus ? &m_Mathmos[FEObjColour::OC_FOCUS] : &m_Mathmos[FEObjColour::OC_NORMAL])
            ->Goto(b ? FEMathmosPos::MP_START : FEMathmosPos::MP_END);
        if (!(m_Flags & 0x2000) && (m_Flags & 0x1000) && b) BindUpdate(FE_DEFAULT);
    }
    return this;
}

signed char FEObject::RectOutside(EXRect r)
{
    EXRect rc = Rect();
    if (rc.x > r.x + r.w) return 1;
    if (rc.y > r.y + r.h) return 1;
    if (rc.x + rc.w < r.x) return 1;
    if (rc.y + rc.h < r.y) return 1;
    return 0;
}

// NEAR: register allocation (zero cached in ebx) and m_TextRect store addressing
void FEObject::CalculateTextRect()
{
    if (m_Text.Length() && m_pOwnerWnd) {
        EXRect r(0, 0, EXBaseDisplay::GetDisplay()->Width(), EXBaseDisplay::GetDisplay()->Height());
        m_pOwnerWnd->SelectFont(m_Font[0], m_Font[1]);
        EXPoint size = m_pOwnerWnd->FontTextSize(m_Text, r, 1.0f);
        m_TextRect = FEBaseWnd::AlignRectToRect(EXRect(0, 0, size.x, size.y), Rect(), m_TextAlign);
    }
}

signed char FEObject::HandleInput(signed char b)
{
    signed char r = 0;
    if (!(m_Flags & 0x10000)) return 0;
    if (b) {
        if (!m_pOwnerWnd) return 0;
        if (!(m_Flags & 0x40)) return 0;
        for (EXDListItem *p = m_Listeners.Head(); p; p = p->Next()) r |= static_cast<FEListener *>(p)->Update();
    }
    return r;
}

void FEObject::EnsureVisible(FEObject *p, signed char b)
{
    m_Flags |= 0x40;
    if (m_pParent) m_pParent->EnsureVisible(this, b);
}

void FEObject::BindUpdate(unsigned __int64 data)
{
    if ((unsigned int)data == FE_DEFAULT) data = m_Data;
    if (m_pBind) {
        m_BindValue = data;
        if (m_Flags & 0x800) BindCommit();
    } else if (m_pParent) {
        m_pParent->BindUpdate(data);
    }
}

void FEObject::BindCommit()
{
    if (m_pBind) {
        switch (m_BindType) {
        case FEDataType::DT_S8: *(signed char *)m_pBind = (signed char)m_BindValue; break;
        case FEDataType::DT_U8: *(unsigned char *)m_pBind = (unsigned char)m_BindValue; break;
        case FEDataType::DT_INT: *(int *)m_pBind = (int)m_BindValue; break;
        case FEDataType::DT_U32: *(unsigned int *)m_pBind = (unsigned int)m_BindValue; break;
        case FEDataType::DT_U64: *(unsigned __int64 *)m_pBind = m_BindValue; break;
        }
    }
}

signed char FEObject::BindRecall()
{
    signed char r = 0;
    if (m_pBind) {
        switch (m_BindType) {
        case FEDataType::DT_S8: m_BindValue = *(signed char *)m_pBind; break;
        case FEDataType::DT_U8: m_BindValue = *(unsigned char *)m_pBind; break;
        case FEDataType::DT_U32: m_BindValue = *(unsigned int *)m_pBind; break;
        case FEDataType::DT_INT: m_BindValue = *(int *)m_pBind; break;
        case FEDataType::DT_U64: m_BindValue = *(unsigned __int64 *)m_pBind; break;
        default: m_BindValue = 0; break;
        }
        FEObject *p = FEListener::FindObjectByData(this, m_BindValue);
        if (p) {
            p->EnsureVisible(0, 0);
            r = 1;
        }
    }
    return r;
}

void FEObject::Bind(FEDataType::Flags type, void *p)
{
    m_BindType = type;
    m_pBind = p;
    BindRecall();
}

void FEObject::Update()
{
    MathmosUpdate();
    DoEventCallback(FEObjEvent::OE_UPDATE);
}

FEObject *FEObject::SetOwnerWnd(FEWnd *p)
{
    m_pOwnerWnd = p;
    return this;
}

void FEObject::DebugProperties(char *pBuf, unsigned int n) {}

ex::RGBA FEObject::MultiplyColour(ex::RGBA a, ex::RGBA b)
{
    ex::RGBA r;
    r.m_R = (u8)(a.m_R * ((float)b.m_R / 128.0f));
    r.m_G = (u8)(a.m_G * ((float)b.m_G / 128.0f));
    r.m_B = (u8)(a.m_B * ((float)b.m_B / 128.0f));
    r.m_A = (u8)(a.m_A * ((float)b.m_A / 128.0f));
    return r;
}

// NEAR: 49 bytes differ, retail converts both target coords before storing and copies the start origin as floats (movss)
FEKeyframe::FEKeyframe(FEObject *pObj, EXPoint target, float time)
{
    m_Target.x = (float)target.x;
    m_Target.y = (float)target.y;
    m_pObject = pObj;
    m_Start = pObj->OriginFloat();
    m_Time = time;
}

FEObjectScript *FEObject::ScriptAdd(unsigned __int64 id, FEObjType::Flags type, unsigned int file, unsigned int hash,
                                    EXW::TextAlign align)
{
    m_Scripts.AddTail(new FEObjectScript(id, this, type, file, hash));
    FEObjectScript *pScript = Script(id);
    pScript->SetObjectAlign(align);
    pScript->ForceUpdate();
    if (m_Flags & 4) pScript->PlayEvent(FEObjEvent::OE_FOCUSON);
    else pScript->PlayEvent(FEObjEvent::OE_FOCUSOFF);
    m_Flags &= ~0x20;
    return pScript;
}

signed char FEObject::ScriptDel(unsigned __int64 id)
{
    FEObjectScript *pScript = Script(id);
    if (!pScript) return 0;
    m_Scripts.Remove(pScript);
    delete pScript;
    return 1;
}

// NEAR: inlined FEKeyframe ctor (see FEKeyframe::FEKeyframe)
FEObject *FEObject::KeyframeAdd(EXPoint target, float time)
{
    m_Keyframes.AddTail(new FEKeyframe(this, target, time));
    return this;
}

void FEObject::MathmosUpdate()
{
    if (m_Flags & 4) {
        if (m_pRemoteMathmos[FEObjColour::OC_FOCUS]) m_Colour = m_pRemoteMathmos[FEObjColour::OC_FOCUS]->Colour();
        else m_Colour = m_Mathmos[FEObjColour::OC_FOCUS].Update();
    } else {
        if (m_pRemoteMathmos[FEObjColour::OC_NORMAL]) m_Colour = m_pRemoteMathmos[FEObjColour::OC_NORMAL]->Colour();
        else m_Colour = m_Mathmos[FEObjColour::OC_NORMAL].Update();
    }
    if (!m_pRemoteMathmos[FEObjColour::OC_TINT]) m_Mathmos[FEObjColour::OC_TINT].Update();
    if (!m_pRemoteMathmos[FEObjColour::OC_ALPHA]) m_Mathmos[FEObjColour::OC_ALPHA].Update();
    m_Colour = MultiplyMathmos(MultiplyMathmos(m_Colour, FEObjColour::OC_TINT), FEObjColour::OC_ALPHA);
}

// NEAR: outer script loop; retail null-checks the static_cast inside the loop
void FEObject::DoObjectAlign()
{
    if (!(m_Flags & 0x20)) {
        if (!(m_Flags & 0x10)) {
            m_Rect = FEBaseWnd::AlignRectToRect(m_Rect, EXRect(0, 0, m_BaseRect.w, m_BaseRect.h), m_ObjectAlign);
            m_OriginFloat.x = (float)(m_BaseRect.x + m_Rect.x);
            m_OriginFloat.y = (float)(m_BaseRect.y + m_Rect.y);
            CalculateTextRect();
        }
        EXDListItem *p;
        for (p = m_Scripts.Head(); p; p = p->Next()) static_cast<FEObjectScript *>(p)->StateSub(0x20);
        for (p = m_Scripts.Head(); p; p = p->Next()) {
            FEObjectScript *pScript = static_cast<FEObjectScript *>(p);
            if (!(pScript->State() & 0x20)) pScript->DoObjectAlign(Rect());
            for (EXDListItem *q = m_Scripts.Head(); q; q = q->Next()) {
                FEObjectScript *pAttached = static_cast<FEObjectScript *>(q);
                if (pAttached->AttachedTo() == pScript->ID()) {
                    pAttached->SetPosition(pScript->Position());
                    pAttached->StateAdd(0x20);
                }
            }
            pScript->StateAdd(0x20);
        }
    }
    m_Flags |= 0x20;
}

FEObject *FEObject::SetRect(EXRect r)
{
    if (!(m_Flags & 0x200)) m_BaseRect.Set(r.x, r.y, r.w, r.h);
    if (!(m_Flags & 2)) m_Rect.Set(0, 0, r.w, r.h);
    m_Flags &= ~0x20;
    DoObjectAlign();
    Resize();
    return this;
}

// NEAR: 9 bytes differ, retail tests the result with test/sete, ours neg/sbb/inc
signed char FEObject::RectInside(EXRect r)
{
    return RectOutside(r) == 0;
}

// NEAR: 8 bytes differ, SafeFrame fields loaded in reverse order (h, w, y, x)
FEObject *FEObject::MakeSafeRect()
{
    m_Flags = (m_Flags & ~0x20) | 0x12;
    const EXRect &r = EXBaseDisplay::GetDisplay()->m_SafeFrame;
    m_Rect.Set(r.x, r.y, r.w, r.h);
    return this;
}

// NEAR: x87 compare/store scheduling of the GetDatumPos result
EXVector FEObject::FindDatum(unsigned int hash)
{
    for (EXDListItem *p = m_Scripts.Head(); p; p = p->Next()) {
        EXVector v = static_cast<FEObjectScript *>(p)->GetDatumPos(hash);
        if (!(v == EXVector(-100.0f, -100.0f, -100.0f, 0.0f))) return v;
    }
    return EXVector(-100.0f, -100.0f, -100.0f, 0.0f);
}

// NEAR: x87 compare scheduling against the -100 sentinel
EXPoint FEObject::FindDatum2D(unsigned int hash)
{
    EXVector v = FindDatum(hash);
    if (v == EXVector(-100.0f, -100.0f, -100.0f, 0.0f)) {
        EXPoint r;
        r.x = 0;
        r.y = 0;
        return r;
    }
    EXPoint p = m_pOwnerWnd->WorldToWnd(v);
    EXPoint r;
    r.x = p.x - (m_BaseRect.x + m_Rect.x);
    r.y = p.y - (m_BaseRect.y + m_Rect.y);
    return r;
}

// NEAR: SetOriginFloat inlined in ours, called in retail; clamp compare order
float FEKeyframe::Update(float dt)
{
    m_Time -= dt;
    EXVector2 cur = m_pObject->OriginFloat();
    if (m_Time <= 0.0f) {
        m_pObject->SetOriginFloat(m_Target);
        return -m_Time;
    }
    float f = dt / m_Time;
    EXVector2 p;
    p.x = (m_Target.x - cur.x) * f + cur.x;
    p.y = (m_Target.y - cur.y) * f + cur.y;
    if ((m_Target.x > m_Start.x && p.x > m_Target.x) || (m_Start.x > m_Target.x && m_Target.x > p.x)) p.x = m_Target.x;
    if ((m_Target.y > m_Start.y && p.y > m_Target.y) || (m_Start.y > m_Target.y && m_Target.y > p.y)) p.y = m_Target.y;
    m_pObject->SetOriginFloat(p);
    return -m_Time;
}

// NEAR: inlined SetOriginFloat ordering (see FEObject::SetOriginFloat)
void FEKeyframe::MoveToTarget() { m_pObject->SetOriginFloat(m_Target); }

// NEAR: 153 bytes differ, store scheduling only (all fields and calls agree)
void FEObject::Init(FEObjType::Flags type, unsigned int file, unsigned int hash, unsigned __int64 data, EXString text,
                    EXString help, EXW::TextAlign textAlign, unsigned int x, unsigned int y, unsigned int w,
                    unsigned int h, EXW::TextAlign objAlign)
{
    m_Rect.x = 0;
    m_Rect.y = 0;
    m_Rect.w = 0;
    m_Rect.h = 0;
    m_BaseRect.x = 0;
    m_BaseRect.y = 0;
    m_BaseRect.w = 0;
    m_BaseRect.h = 0;
    m_TextRect.x = 0;
    m_TextRect.y = 0;
    m_TextRect.w = 0;
    m_TextRect.h = 0;
    m_Text = text;
    m_HelpText = help;
    m_Data = data;
    m_ObjectAlign = objAlign;
    m_Font[0] = 0x100003a;
    m_Font[1] = 0x7000004;
    m_Texture[0] = 0;
    m_Texture[1] = 0;
    m_AltData = 0;
    m_TextAlign = textAlign;
    m_TexCoord0.x = 0.0f;
    m_TexCoord0.y = 0.0f;
    m_TexCoord1.x = 1.0f;
    m_TexCoord1.y = 1.0f;
    m_OriginFloat.x = 0.0f;
    m_OriginFloat.y = 0.0f;
    m_ScrollPos = 0.0f;
    m_BlendMode = 0;
    m_pOwnerWnd = 0;
    m_pEventCallback = 0;
    m_BorderWidth = 0;
    m_pParent = 0;
    m_Colour = ex::RGBA(0x7f, 0x7f, 0x7f, 0x80);
    m_DeleteNeeded = 0;
    m_CleanedUp = 0;
    m_TypeChars = 0;
    m_TypeSpeed = 0;
    m_TypeCounter = 0;
    m_TypeDelay = 0;
    m_pBind = 0;
    m_BindType = FEDataType::DT_NONE;
    m_BindValue = 0;
    m_pMathmosLink = 0;
    m_ScrollDelay = 2.0f;
    m_Flags = 0x8015c48;
    m_Mathmos[FEObjColour::OC_FOCUS] = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    m_Mathmos[FEObjColour::OC_NORMAL] = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    m_Mathmos[FEObjColour::OC_TINT] = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    m_Mathmos[FEObjColour::OC_ALPHA] = FEMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
    MathmosGoto(FEMathmosPos::MP_END);
    m_pRemoteMathmos[0] = 0;
    m_pRemoteMathmos[1] = 0;
    m_pRemoteMathmos[2] = 0;
    m_pRemoteMathmos[3] = 0;
    switch (type) {
    case FEObjType::OT_SPRITE:
        m_Texture[0] = file;
        m_Texture[1] = hash;
        break;
    case FEObjType::OT_TEXT:
        m_Font[0] = file;
        m_Font[1] = hash;
        break;
    case FEObjType::OT_SCRIPT:
    case FEObjType::OT_SCRIPT2:
        ScriptAdd(0, type, file, hash, objAlign);
        break;
    }
    InitRect(x, y, w, h);
}

// NEAR: inlined MoveToTarget/SetOriginFloat ordering (see SetOriginFloat)
FEObject *FEObject::KeyframeClear()
{
    while (m_Keyframes.Head()) {
        static_cast<FEKeyframe *>(m_Keyframes.Head())->MoveToTarget();
        delete static_cast<FEKeyframe *>(m_Keyframes.RemoveHead());
    }
    return this;
}

// NEAR: retail inlines FEKeyframe::Update (with SetOriginFloat/SetRect); ours calls it
void FEObject::UpdateKeyframes()
{
    float dt = XApp::GameFrameRateMul();
    while (m_Keyframes.Count() > 0 && dt > 0.0f) {
        dt = static_cast<FEKeyframe *>(m_Keyframes.Head())->Update(dt);
        if (dt > 0.0f) {
            delete static_cast<FEKeyframe *>(m_Keyframes.RemoveHead());
            if (m_Keyframes.Count() == 0) DoEventCallback(FEObjEvent::OE_KEYFRAMESDONE);
        }
    }
}

void FEObject::Cleanup()
{
    if (!m_CleanedUp) {
        while (m_Scripts.Head()) {
            FEObjectScript *p = static_cast<FEObjectScript *>(m_Scripts.RemoveHead());
            delete p;
        }
        KeyframeClear();
        ListenerClear();
        m_CleanedUp = 1;
    }
}

FEObject::~FEObject() { Cleanup(); }

// NEAR: retail calls Rect() out of line and puts the typing branch after the others
void FEObject::Draw()
{
    if (!m_pOwnerWnd || !(m_Flags & 0x40)) return;
    DoObjectAlign();
    UpdateKeyframes();
    if (m_Flags & 0x80) return;
    if (!(m_Flags & 0x40000)) m_pOwnerWnd->AddClipRect(Rect());
    if (m_pMathmosLink) m_Colour = m_pMathmosLink->MathmosLinkColour();
    int w = m_Rect.w;
    int h = m_Rect.h;
    if (m_Text.Length() > 0) {
        if (m_TypeSpeed > 0) {
            if (m_TypeDelay > 0) {
                m_TypeDelay--;
            } else {
                if (m_TypeCounter++ % m_TypeSpeed == 0) m_TypeChars++;
                DrawText(FERenderPass::RP_2, m_Font[0], m_Font[1], m_Colour, m_BlendMode, EXRect(0, 0, w, h), m_Text,
                         m_TextAlign, m_TypeChars);
                if (m_TypeChars >= m_Text.Length()) m_TypeSpeed = 0;
            }
        } else if (m_Flags & 0x20000) {
            EXRect r = Rect();
            m_pOwnerWnd->SelectFont(m_Font[0], m_Font[1]);
            r.w = 0x7fff;
            EXPoint size = m_pOwnerWnd->FontTextSize(m_Text, r, 1.0f);
            size.x++;
            size.y++;
            if (size.y < m_Rect.h + 4) {
                DrawText(FERenderPass::RP_2, m_Font[0], m_Font[1], m_Colour, m_BlendMode, EXRect(0, 0, w, h), m_Text,
                         m_TextAlign, m_TypeChars);
            } else {
                if (!theApp.DialogOpen() && !(m_Flags & 0x800000)) {
                    m_ScrollDelay -= XApp::FrameLength();
                    if (m_ScrollDelay < 0.0f) {
                        m_ScrollPos -= 0.5f;
                        if ((float)-size.y > m_ScrollPos) m_ScrollPos = (float)m_Rect.h;
                    }
                }
                DrawText(FERenderPass::RP_2, m_Font[0], m_Font[1], m_Colour, m_BlendMode,
                         EXRect(0, (int)m_ScrollPos, w, size.y), m_Text, m_TextAlign, m_TypeChars);
            }
        } else {
            DrawText(FERenderPass::RP_2, m_Font[0], m_Font[1], m_Colour, m_BlendMode, EXRect(0, 0, w, h), m_Text,
                     m_TextAlign, 0);
        }
    }
    if (m_Texture[0] && m_Texture[1]) {
        if (!m_BorderWidth)
            DrawSprite(FERenderPass::RP_2, m_Texture[0], m_Texture[1], m_Colour, m_BlendMode, EXRect(0, 0, w, h),
                       m_TexCoord0, m_TexCoord1);
        else
            DrawBorderSprite(FERenderPass::RP_2, m_Texture[0], m_Texture[1], m_Colour, m_BlendMode, EXRect(0, 0, w, h),
                             m_BorderWidth);
    }
    m_pOwnerWnd->SetupCamera(0);
    for (EXDListItem *p = m_Scripts.Head(); p; p = p->Next())
        m_pOwnerWnd->DrawScript(FERenderPass::RP_1, m_Colour, static_cast<FEObjectScript *>(p));
    if (!(m_Flags & 0x40000)) m_pOwnerWnd->RestoreClipRect();
}

FEObject::FEObject() : m_StateStack(0x80)
{
    Init(FEObjType::OT_NONE, 0, 0, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4);
    m_Flags &= ~8;
}

FEObject::FEObject(FEObjType::Flags type, unsigned int file, unsigned int hash, unsigned __int64 data, EXString text,
                   EXString help, EXW::TextAlign textAlign, unsigned int x, unsigned int y, unsigned int w,
                   unsigned int h, EXW::TextAlign objAlign)
    : m_StateStack(0x80)
{
    Init(type, file, hash, data, text, help, textAlign, x, y, w, h, objAlign);
}
