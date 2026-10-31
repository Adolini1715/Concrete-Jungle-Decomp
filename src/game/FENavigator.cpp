#include "FEText.h"
#include "EXMemory.h"
#include "EXBaseGamePad.h"
#include "XAppStandIn.h"
#include "FEObject.h"
#include "FEObjectScript.h"
#include "FEListener.h"
#include "FEHud.h"
#include "FEPause.h"
#include "EXBaseSoundManager.h"

FEPad::Flags FEListener::m_PadOverride = (FEPad::Flags)0xf;
static const float s_FENavPadInit = 1.0f / 3.0f;
static float s_FENavThird = s_FENavPadInit;
static unsigned int s_AnyButton[1] = { 0x00fff00f };
unsigned char FEListener::m_LastPad;
float FEListener::m_LastInput;
unsigned int FEListener::m_ButtonMask[4];
unsigned int FEListener::m_PrevButtonMask[4];
signed char FEListener::ms_bInputDisabled;
FEListener *FEListener::m_pDialogListener;
FEKeyRepeater FEListener::m_KeyRepeater(FE_DEFAULT, FE_DEFAULT, FE_DEFAULT, FE_DEFAULT);
EXDList FEListener::m_Remap;

void FEMathmos::Init(ex::RGBA from, ex::RGBA to, float speed, FEWaveform::Flags wave, signed char enabled)
{
    m_From = from;
    m_Wave = wave;
    m_Colour = from;
    m_Time = 0.0f;
    m_Phase = 0.0f;
    m_To = to;
    m_Speed = speed;
    m_Enabled = enabled;
    m_Finished = from == to;
}

void FEMathmos::Goto(FEMathmosPos::Flags pos)
{
    if (m_Enabled) {
        switch (pos) {
        case FEMathmosPos::MP_START:
            m_Finished = 0;
            m_Colour = m_From;
            break;
        case FEMathmosPos::MP_END:
            m_Finished = 1;
            m_Colour = m_To;
            break;
        default:
            return;
        }
        m_Time = 0.0f;
        m_Phase = 0.0f;
    }
}

ex::RGBA FEMathmos::Validate(ex::RGBA c)
{
    if (c.m_R > 0x7f) c.m_R = 0x7f;
    if (c.m_G > 0x7f) c.m_G = 0x7f;
    if (c.m_B > 0x7f) c.m_B = 0x7f;
    if (c.m_A > 0x80) c.m_A = 0x80;
    return c;
}

FEKeyRepeater::FEKeyRepeater(unsigned int delay, unsigned int rate, unsigned int minRate, unsigned int accel)
{
    m_Count = 0;
    if (delay != FE_DEFAULT) m_Delay = delay;
    else m_Delay = EXBaseApp::NominalGameFrameRate();
    if (rate == FE_DEFAULT) m_Rate = EXBaseApp::NominalGameFrameRate() / 10;
    else m_Rate = rate;
    if (minRate == FE_DEFAULT) m_MinRate = m_Rate;
    else m_MinRate = minRate;
    m_Accel = accel == FE_DEFAULT ? 0 : accel;
    m_CurRate = m_Rate;
}

// NEAR: esi/edi swapped (this vs p); otherwise the same
void FEListener::SetFocus(FEObject *p, signed char b)
{
    if (m_pFocus) m_pFocus->SetFocus(0, b, 0);
    m_pFocus = p;
    if (p) p->SetFocus(1, b, 0);
    if (b) m_CursorMathmos.Goto(FEMathmosPos::MP_START);
    else m_CursorMathmos.Goto(FEMathmosPos::MP_END);
    if (m_pCursor) {
        m_pCursor->SetRect(p->Rect());
        m_pCursor->Script(0)->Play(0, 0);
    }
    if (m_pHelpObject && m_pFocus && m_pFocus->HelpText().Length() > 0) m_pHelpObject->SetText(m_pFocus->HelpText(), 0, 0);
}

void FEListener::SetDefaultFocus(FEObject *p)
{
    m_pDefaultFocus = p;
    SetFocus(p, 0);
}

void FEListener::SetCursor(FEObject *p)
{
    m_pCursor = p;
    p->SetParent(m_pOwner);
    p->SetOwnerWnd(m_pOwner->OwnerWnd());
}

void FEListener::RemapAdd(unsigned int in, unsigned int out) { m_Remap.AddTail(new FEListenerRemap(in, out)); }

void FEListener::RemapClear()
{
    while (m_Remap.Head()) delete static_cast<FEListenerRemap *>(m_Remap.RemoveHead());
}

void FEListener::RemapModeSet(FERemap::Flags mode)
{
    RemapClear();
    switch (mode) {
    case 1:
        m_Remap.AddTail(new FEListenerRemap(0x40000, 0x2000));
        m_Remap.AddTail(new FEListenerRemap(0x80000, 0x8000));
        break;
    }
}

void FEListener::LastInputReset() { m_LastInput = theApp.Time(); }

signed char FEListener::QueryButtons(FEPad::Flags pad)
{
    for (int i = 0; i < 4; i++) {
        if ((pad & (1 << i)) && (m_PadOverride & (1 << i))) {
            for (unsigned int j = 0; j < sizeof(s_AnyButton) / sizeof(s_AnyButton[0]); j++) {
                if (!(s_AnyButton[j] & m_PrevButtonMask[i]) && (m_ButtonMask[i] & s_AnyButton[j])) return 1;
            }
        }
    }
    return 0;
}

// The one/two-colour constructors set the fields in this order (not through Init, whose order differs).
#define FEMATHMOS_SET(from, to, speed, wave, enabled) \
    m_From = from;                                    \
    m_To = to;                                        \
    m_Colour = m_From;                                \
    m_Time = 0.0f;                                    \
    m_Phase = 0.0f;                                   \
    m_Speed = speed;                                  \
    m_Wave = wave;                                    \
    m_Enabled = enabled;                              \
    m_Finished = m_From == m_To;

FEMathmos::FEMathmos()
{
    ex::RGBA from(0x7f, 0x7f, 0x7f, 0x80);
    ex::RGBA to(0x7f, 0x7f, 0x7f, 0x80);
    m_From = from;
    m_Colour = from;
    m_Time = 0.0f;
    m_Phase = 0.0f;
    m_To = to;
    m_Speed = 0.0f;
    m_Wave = FEWaveform::WF_NONE;
    m_Enabled = 1;
    m_Finished = from == to;
}

FEMathmos::FEMathmos(ex::RGBA c) { FEMATHMOS_SET(c, c, 0.0f, FEWaveform::WF_NONE, 1) }

FEMathmos::FEMathmos(ex::RGBA from, ex::RGBA to) { FEMATHMOS_SET(from, to, 60.0f, FEWaveform::WF_LINEAR, 0) }

FEMathmos::FEMathmos(ex::RGBA from, ex::RGBA to, float speed, FEWaveform::Flags wave, signed char enabled)
{
    Init(from, to, speed, wave, enabled);
}

FEMathmos FEMathmos::DefaultTint()
{
    return FEMathmos(FEHud::TintColour(XItemHandler_Player::Instance()->m_Tint));
}

FEMathmos FEMathmos::DefaultTintFocus()
{
    ex::RGBA c = FEHud::TintColour(XItemHandler_Player::Instance()->m_Tint);
    return FEMathmos(c, ex::RGBA(0, 0, 0, 0));
}

// NEAR: 26 bytes differ, scheduling of the inlined FEMathmos() and FEKeyRepeater temporaries
FEListener::FEListener(unsigned __int64 id, unsigned int flags, FEEventCallback pCallback, FEPad::Flags pad)
{
    m_Id = id;
    m_ListenFlags = flags;
    m_pCallback = pCallback;
    m_Pad = pad;
    m_CursorMathmos = FEMathmos(ex::RGBA(0x7f, 0, 0, 0x80));
    m_pFocus = 0;
    m_pDefaultFocus = 0;
    m_pOwner = 0;
    m_pCursor = 0;
    m_pHelpObject = 0;
    m_KeyRepeater = FEKeyRepeater(EXBaseApp::NominalGameFrameRate() / 2, EXBaseApp::NominalGameFrameRate() / 5,
                                  EXBaseApp::NominalGameFrameRate() / 10, 1);
    m_UpdateTimeStamp = 0;
    m_Disabled = 0;
}

// NEAR: 14 bytes differ, scheduling of the inlined FEMathmos() RGBA temporaries
FEListener::FEListener(FEListener *p) { memcpy(this, p, sizeof(FEListener)); }

FEListener::~FEListener()
{
    if (m_pCursor) m_pCursor->Delete();
    SetFocus(0, 0);
}

// NEAR: retail calls RemapClear (ours inlines it) and keeps zero in eax/ecx
void FEListener::Initialise()
{
    m_KeyRepeater = FEKeyRepeater(FE_DEFAULT, FE_DEFAULT, FE_DEFAULT, 0);
    for (int i = 0; i < 4; i++) {
        m_ButtonMask[i] = 0;
        m_PrevButtonMask[i] = 0;
    }
    ms_bInputDisabled = 0;
    m_pDialogListener = 0;
    m_LastPad = 0;
    m_LastInput = 0.0f;
    RemapClear();
}

void FEListener::LastInputUpdate()
{
    EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
    for (int i = 0; i < 4; i++) {
        unsigned int b = pPad->Buttons(i);
        if (b != m_ButtonMask[i]) m_LastInput = theApp.Time();
        m_PrevButtonMask[i] = m_ButtonMask[i];
        m_ButtonMask[i] = b;
    }
}

// NEAR: remap loop and button bit tests (retail bitfield-style shifts)
signed char FEListener::QueryButton(unsigned int b, FEPad::Flags pad, signed char pressed)
{
    EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
    unsigned int i;
    for (i = 0; i < 4; i++) {
        if (!(pad & (1 << i))) continue;
        if (!GameControllerRemovedSys::ControllerIsValid(i)) continue;
        if (!(m_PadOverride & (1 << i))) continue;
        if (b == 4 || b == 8) {
            if (pPad->Buttons(i) & 1) return 0;
            if ((pPad->Buttons(i) >> 1) & 1) return 0;
        }
        if (b == 1 || b == 2) {
            if ((pPad->Buttons(i) >> 2) & 1) return 0;
            if ((pPad->Buttons(i) >> 3) & 1) return 0;
        }
        for (EXDListItem *p = m_Remap.Head(); p; p = p->Next()) {
            FEListenerRemap *pRemap = static_cast<FEListenerRemap *>(p);
            if (pressed) {
                if (b == pRemap->Output() && pPad->ButtonPressed(i, pRemap->Input())) goto found;
            } else {
                if (b == pRemap->Output() && pPad->ButtonDown(i, pRemap->Input())) goto found;
            }
        }
        if (pressed ? pPad->ButtonPressed(i, b) : pPad->ButtonDown(i, b)) goto found;
    }
    return 0;
found:
    m_LastPad = (unsigned char)i;
    return 1;
}

signed char FEListener::QueryButton(unsigned int b, int pad, signed char pressed)
{
    switch (pad) {
    case 0: return QueryButton(b, (FEPad::Flags)1, pressed);
    case 1: return QueryButton(b, (FEPad::Flags)2, pressed);
    case 2: return QueryButton(b, (FEPad::Flags)4, pressed);
    case 3: return QueryButton(b, (FEPad::Flags)8, pressed);
    default: return QueryButton(b, (FEPad::Flags)0xf, pressed);
    }
}

// NEAR: x87 compare and GameStateTimer reload order
signed char FEKeyRepeater::QueryButton(unsigned int b, FEPad::Flags pad)
{
    if (FEListener::QueryButton(b, pad, 1)) {
        m_CurRate = m_Rate;
        m_PressTime = theApp.GameStateTimer();
        m_LastRepeat = (unsigned int)m_PressTime;
        FEListener::LastInputReset();
        return 1;
    }
    if (FEListener::QueryButton(b, pad, 0)) {
        if (theApp.GameStateTimer() - m_PressTime > (float)m_Delay &&
            theApp.GameStateTimer() - (float)m_LastRepeat >= (float)m_CurRate) {
            if (++m_Count >= m_Accel && m_CurRate > m_MinRate) {
                m_CurRate--;
                m_Count = 0;
            }
            m_LastRepeat = (unsigned int)theApp.GameStateTimer();
            FEListener::LastInputReset();
            return 1;
        }
    }
    return 0;
}

FEObject *FEListener::FindObjectByData(FEObject *pRoot, unsigned __int64 data)
{
    if ((unsigned int)data == FE_DEFAULT) return 0;
    if ((unsigned int)data == 0xfdffffff) return 0;
    FEObject *list[256] = { 0 };
    unsigned int n = 0;
    GetChildren((FEObjEvent::Flags)0xf, pRoot, pRoot, list, n, 8, 1);
    for (unsigned int i = 0; i < n; i++) {
        if (i >= 256) return 0;
        if (list[i]->Data() == data) return list[i];
    }
    return 0;
}

signed char FEListener::IsParentOf(FEObject *pParent, FEObject *pChild)
{
    FEObject *list[256] = { 0 };
    unsigned int n = 0;
    GetChildren((FEObjEvent::Flags)0xf, pParent, pParent, list, n, 0x20000000, 1);
    for (unsigned int i = 0; i < n; i++)
        if (pChild == list[i]) return 1;
    return 0;
}

signed char FEListener::IsChildOf(FEObject *pChild, FEObject *pParent) { return IsParentOf(pParent, pChild); }

signed char FEListener::SetFocusByData(unsigned __int64 data, signed char b)
{
    FEObject *p = FindObjectByData(m_pOwner, data);
    if (p) {
        SetFocus(p, b);
        return 1;
    }
    return 0;
}

signed char FEListener::SetDefaultFocusByData(unsigned __int64 data)
{
    FEObject *p = FindObjectByData(m_pOwner, data);
    if (p) {
        SetDefaultFocus(p);
        return 1;
    }
    return 0;
}

// Blends the end colours: retail weights the START colour by f and the END colour by 1 - f.
ex::RGBA FEMathmos::Interpolate(float f)
{
    ex::RGBA c;
    c.m_R = (u8)(m_From.m_R * f) + (u8)(m_To.m_R * (1.0f - f));
    c.m_G = (u8)(m_From.m_G * f) + (u8)(m_To.m_G * (1.0f - f));
    c.m_B = (u8)(m_From.m_B * f) + (u8)(m_To.m_B * (1.0f - f));
    c.m_A = (u8)(m_From.m_A * f) + (u8)(m_To.m_A * (1.0f - f));
    return Validate(c);
}

// Steps the colour animation by one frame. m_Time runs 0..1 (or an angle for the cosine wave);
// m_Phase counts frames and ends the animation once it reaches the wave's length.
ex::RGBA FEMathmos::Update()
{
    float rate = EXBaseApp::GameFrameRateMul();
    ex::RGBA c = m_Colour;
    if (m_Finished) return c;
    m_Phase = rate + m_Phase;
    float phase = m_Phase;
    if (m_Speed == 0.0f) {
        c = m_To;
        m_Finished = 1;
        m_Colour = c;
        return c;
    }
    switch (m_Wave) {
    case 0:
    case 6:
        break;
    case 1:
        m_Time = (6.2831855f / m_Speed) * rate + m_Time;
        break;
    default:
        m_Time = rate / m_Speed + m_Time;
        if (1.0f < m_Time) m_Time = 0.0f;
        break;
    case 3:
        m_Time = m_Time - rate / m_Speed;
        if (m_Time < 0.0f) m_Time = 1.0f;
        break;
    }
    float f;
    switch (m_Wave) {
    case 0:
        c = m_From;
        goto done;
    case 1:
        f = (float)cos(m_Time) * 0.5f + 0.5f;
        break;
    case 2:
    case 6:
        f = m_Time;
        break;
    case 3:
        f = m_Time;
        break;
    case 4:
        f = m_Time >= 0.5f ? 0.0f : 1.0f;
        break;
    case 5:
        if (m_Time >= 0.5f) f = 1.0f - (m_Time - 0.5f) * 2.0f;
        else f = m_Time * 2.0f;
        break;
    default:
        goto done;
    }
    c = Interpolate(f);
done:
    if (m_Enabled) {
        float len;
        switch (m_Wave) {
        case 0:
            m_Finished = 1;
            goto finish;
        case 1:
        case 4:
        case 5:
            len = m_Speed * 0.5f;
            break;
        case 2:
        case 3:
            len = m_Speed;
            break;
        default:
            goto finish;
        }
        if (len <= phase) m_Finished = 1;
    }
finish:
    if (m_Finished) {
        m_Colour = m_To;
        return c;
    }
    m_Colour = c;
    return c;
}

// True when the two objects overlap on the axis perpendicular to the movement direction (events 0xb/0xc
// move along y so test x; 0xd/0xe move along x so test y). Each centre must lie inside the other's span.
signed char FEListener::IsOrthogonal(FEObjEvent::Flags ev, FEObject *a, FEObject *b)
{
    switch ((int)ev) {
    case 0xb:
    case 0xc: {
        EXRect ra = a->Rect();
        EXRect rb = b->Rect();
        if (rb.x <= a->Centre().x && a->Centre().x <= rb.x + rb.w && ra.x <= b->Centre().x &&
            b->Centre().x <= ra.x + ra.w)
            return 1;
        break;
    }
    case 0xd:
    case 0xe: {
        EXRect ra = a->Rect();
        EXRect rb = b->Rect();
        if (rb.y <= a->Centre().y && a->Centre().y <= rb.y + rb.h && ra.y <= b->Centre().y &&
            b->Centre().y <= ra.y + ra.h)
            return 1;
        break;
    }
    }
    return 0;
}

// Is a direction pressed on any enabled pad (or remapped onto it)? Records which pad did it.
static signed char FE_DirPressed(unsigned int bit, FEPad::Flags pad, EXDList &remap, FEPad::Flags padOverride,
                                 unsigned char &lastPad)
{
    EXBaseGamePad *pPad = EXBaseGamePad::GetGamePad();
    for (unsigned int i = 0; i < 4; i++) {
        unsigned int mask = 1 << i;
        if (!(pad & mask)) continue;
        if (!GameControllerRemovedSys::ControllerIsValid(i)) continue;
        if (!(padOverride & mask)) continue;
        for (EXDListItem *p = remap.Head(); p; p = p->Next()) {
            FEListenerRemap *pRemap = static_cast<FEListenerRemap *>(p);
            if (pRemap->Output() == (1u << bit) && (pRemap->Input() & pPad->Buttons(i))) {
                lastPad = (unsigned char)i;
                return 1;
            }
        }
        if ((pPad->Buttons(i) >> bit) & 1) {
            lastPad = (unsigned char)i;
            return 1;
        }
    }
    return 0;
}

// Reads the d-pad as one of eight directions (0 up, then clockwise; 8 none).
FEDirection::Flags FEListener::QueryDirection(FEPad::Flags pad)
{
    signed char up = FE_DirPressed(4, pad, m_Remap, m_PadOverride, m_LastPad);
    signed char down = FE_DirPressed(5, pad, m_Remap, m_PadOverride, m_LastPad);
    signed char left = FE_DirPressed(6, pad, m_Remap, m_PadOverride, m_LastPad);
    signed char right = FE_DirPressed(7, pad, m_Remap, m_PadOverride, m_LastPad);
    if (up && right) return (FEDirection::Flags)1;
    if (down) {
        if (right) return (FEDirection::Flags)3;
        if (left) return (FEDirection::Flags)5;
    }
    if (up) return (FEDirection::Flags)(left ? 7 : 0);
    if (right) return (FEDirection::Flags)2;
    if (down) return (FEDirection::Flags)4;
    return (FEDirection::Flags)(left ? 6 : 8);
}

// Collects objects under a (flags: 0x20000000 also lists the containers) that lie in direction ev
// from b: 0xb below, 0xc above, 0xd right, 0xe left, 0xf anywhere. Unless ev is 0xf the final list
// is bubble-sorted by distance from b's centre (bNearestLast picks the order). Retail never clears the
// "swapped" flag inside a pass, so once a pair swaps, every later pair in the pass swaps too.
unsigned int FEListener::GetChildren(FEObjEvent::Flags ev, FEObject *a, FEObject *b, FEObject **pList,
                                     unsigned int &n, unsigned int flags, signed char bNearestLast)
{
    if (!a) return n;
    if (n > 0xff) return 0;
    static unsigned int s_Depth = n;
    s_Depth++;
    unsigned int count = a->ChildCount();
    if (count != 0 && ((a->State() & flags) != flags || (flags & 0x20000000))) {
        if (flags & 0x20000000) pList[n++] = a;
        for (unsigned int i = 0; i < count; i++) GetChildren(ev, a->Object(i), b, pList, n, flags, bNearestLast);
    } else if ((a->State() & flags) == flags || (flags & 0x20000000)) {
        bool bAdd = false;
        switch ((int)ev) {
        case 0xb:
            if ((b->State() & 0x400) && !IsOrthogonal(ev, a, b)) break;
            bAdd = b->Centre().y > a->Centre().y;
            break;
        case 0xc:
            if ((b->State() & 0x400) && !IsOrthogonal(ev, a, b)) break;
            bAdd = a->Centre().y > b->Centre().y;
            break;
        case 0xd:
            if ((b->State() & 0x400) && !IsOrthogonal(ev, a, b)) break;
            bAdd = b->Centre().x > a->Centre().x;
            break;
        case 0xe:
            if ((b->State() & 0x400) && !IsOrthogonal(ev, a, b)) break;
            bAdd = a->Centre().x > b->Centre().x;
            break;
        case 0xf:
            bAdd = true;
            break;
        }
        if (bAdd) pList[n++] = a;
    }
    s_Depth--;
    if ((int)ev != 0xf && s_Depth == 0) {
        if (n == 0) return 0;
        float cy = (float)b->Centre().y;
        int cx = b->Centre().x;
        bool bSwapped;
        do {
            bSwapped = false;
            if (n == 1) break;
            for (unsigned int i = 0; i < n - 1; i++) {
                FEObject *pa = pList[i];
                FEObject *pb = pList[i + 1];
                float dxa = (float)cx - (float)pa->Centre().x;
                float dya = cy - (float)pa->Centre().y;
                float da = dya * dya + dxa * dxa;
                float dxb = (float)cx - (float)pb->Centre().x;
                float dyb = cy - (float)pb->Centre().y;
                float db = dyb * dyb + dxb * dxb;
                if (!bNearestLast) {
                    if (db > da) bSwapped = true;
                } else {
                    if (db < da) bSwapped = true;
                }
                if (bSwapped) {
                    pList[i] = pb;
                    pList[i + 1] = pa;
                }
            }
        } while (bSwapped);
    }
    return n;
}

// Per-frame input: d-pad/stick navigation moves the focus (wrapping to the far end when nothing lies
// that way and the object allows it), then face-button presses are passed to the callback as events.
// Event numbers: 1-8 button presses, 0x1a/0x1b the pause/back buttons; 0xb-0xe up/down/right/left.
signed char FEListener::Update()
{
    m_UpdateTimeStamp++;
    if (m_pCursor) m_pCursor->Update();
    if (ms_bInputDisabled) return 0;
    if (m_Disabled) return 0;
    if (m_pDialogListener && m_pDialogListener != this) return 0;

    FEObject *list[256] = { 0 };
    unsigned int n = 0;
    signed char result = 0;
    int ev = 0;
    if (m_ListenFlags & 0x400) {
        if (m_KeyRepeater.QueryButton(0x10, m_Pad)) ev = 0xb;
        else if (m_KeyRepeater.QueryButton(0x20, m_Pad)) ev = 0xc;
    }
    if (m_ListenFlags & 0x800) {
        if (m_KeyRepeater.QueryButton(0x40, m_Pad)) ev = 0xd;
        else if (m_KeyRepeater.QueryButton(0x80, m_Pad)) ev = 0xe;
    }
    if (m_ListenFlags & 1) {
        if (m_KeyRepeater.QueryButton(1, m_Pad)) ev = 0xb;
        else if (m_KeyRepeater.QueryButton(2, m_Pad)) ev = 0xc;
    }
    if (m_ListenFlags & 2) {
        if (m_KeyRepeater.QueryButton(4, m_Pad)) ev = 0xd;
        else if (m_KeyRepeater.QueryButton(8, m_Pad)) ev = 0xe;
    }
    if (m_ListenFlags & 0x1000) {
        if (m_KeyRepeater.QueryButton(0x100, m_Pad)) ev = 0xb;
        else if (m_KeyRepeater.QueryButton(0x200, m_Pad)) ev = 0xc;
    }
    if (m_ListenFlags & 0x2000) {
        if (m_KeyRepeater.QueryButton(0x400, m_Pad)) ev = 0xd;
        else if (m_KeyRepeater.QueryButton(0x800, m_Pad)) ev = 0xe;
    }
    if (m_ListenFlags & 0x4000) {
        if (m_KeyRepeater.QueryButton(0x100000, m_Pad)) ev = 0xd;
        else if (m_KeyRepeater.QueryButton(0x400000, m_Pad)) ev = 0xe;
    }
    // With the pause menu up, these presses skip navigation this frame.
    if (FEPause::Instance(0)) {
        if (QueryButton(0x2000, m_Pad, 1) || QueryButton(0x8000, m_Pad, 1)) goto buttons;
    }
    if (ev != 0) {
        GetChildren((FEObjEvent::Flags)ev, m_pOwner, m_pFocus, list, n, 0x48, 1);
        if (m_pCallback) m_pCallback(m_pOwner, (FEObjEvent::Flags)ev);
        if (n != 0 && m_pFocus != list[0]) {
            EXBaseSoundManager::GetSoundManager()->StartSound((ev == 0xd || ev == 0xe) ? 0x1af00548 : 0x1af00547);
            SetFocus(list[0], 1);
            goto buttons;
        }
        int opposite;
        unsigned int canWrap;
        switch (ev) {
        case 0xb:
            opposite = 0xc;
            canWrap = m_pFocus->State() >> 0xe;
            break;
        case 0xc:
            opposite = 0xb;
            canWrap = m_pFocus->State() >> 0xe;
            break;
        case 0xd:
            opposite = 0xe;
            canWrap = m_pFocus->State() >> 0xf;
            break;
        case 0xe:
            opposite = 0xd;
            canWrap = m_pFocus->State() >> 0xf;
            break;
        default:
            goto buttons;
        }
        if ((canWrap & 1) && (GetChildren((FEObjEvent::Flags)opposite, m_pOwner, m_pFocus, list, n, 0x48, 1), n != 0) &&
            m_pFocus != list[n - 1]) {
            SetFocus(list[n - 1], 0);
            EXBaseSoundManager::GetSoundManager()->StartSound(0x1af00155);
        }
    }
buttons:
    if (!m_pCallback || !(m_pOwner->State() & 4)) return 0;
    if ((m_ListenFlags & 4) && QueryButton(0x1000, m_Pad, 1)) result = m_pCallback(m_pOwner, (FEObjEvent::Flags)1);
    if ((m_ListenFlags & 8) && QueryButton(0x2000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)2);
    if ((m_ListenFlags & 0x10) && QueryButton(0x4000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)3);
    if ((m_ListenFlags & 0x20) && QueryButton(0x8000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)4);
    if ((m_ListenFlags & 0x40) && QueryButton(0x10000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)5);
    if ((m_ListenFlags & 0x80) && QueryButton(0x20000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)6);
    if ((m_ListenFlags & 0x100) && QueryButton(0x100000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)7);
    if ((m_ListenFlags & 0x200) && QueryButton(0x400000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)8);
    if ((m_ListenFlags & 0x8000) && QueryButton(0x2000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)0x1a);
    if ((m_ListenFlags & 0x10000) && QueryButton(0x8000, m_Pad, 1)) result |= m_pCallback(m_pOwner, (FEObjEvent::Flags)0x1b);
    if (result) EXBaseSoundManager::GetSoundManager()->StartSound(0x1af0015c);
    return result;
}
