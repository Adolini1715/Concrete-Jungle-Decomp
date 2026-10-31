#include "FEText.h"
#include "EXMemory.h"
#include "EXStdFuncs.h"
#include "EXBaseDisplay.h"
#include "EXBaseSoundManager.h"
#include "XAppStandIn.h"
#include "FELayout.h"
#include "FEListener.h"
#include "FEHashCodes.h"

extern "C" int __cdecl sprintf(char *pBuf, const char *pFmt, ...);
extern "C" int __cdecl abs(int);
#pragma intrinsic(abs)
extern unsigned int gCurrentMap_GeoHash;

static const float FE_HALFPI = 1.5707964f;

FELayout::FELayout()
{
    m_pChildren = 0;
    m_pOwnerWnd = 0;
    m_ChildCount = 0;
    m_Rect.Set(0, 0, 0, 0);
    m_BaseRect.Set(0, 0, 0, 0);
    m_Flags &= ~8;
}

FELayout::~FELayout()
{
    Cleanup();
    for (unsigned int i = 0; i < m_ChildCount; i++) {
        delete m_pChildren[i];
        m_pChildren[i] = 0;
    }
    if (m_pChildren) EXFree(m_pChildren);
    m_pChildren = 0;
}

FEObject *FELayout::ObjectAdd(unsigned int i, FEObject *p)
{
    if (m_pChildren[i]) {
        delete m_pChildren[i];
        m_pChildren[i] = 0;
    }
    m_pChildren[i] = p;
    p->SetOwnerWnd(m_pOwnerWnd);
    p->SetParent(this);
    p->SetEventCallback(m_pEventCallback, 1);
    p->SetHelpText(m_HelpText);
    for (unsigned int j = 0; j < FEObjColour::OC_COUNT; j++) {
        p->SetMathmos((FEObjColour::Flags)j, m_Mathmos[j], 1);
        p->SetRemoteMathmos((FEObjColour::Flags)j, m_pRemoteMathmos[j], 1);
    }
    Resize();
    return p;
}

void FELayout::ObjectDel(unsigned int i)
{
    if (i < m_ChildCount && m_pChildren[i]) m_pChildren[i]->Delete();
}

FEObject *FELayout::Object(unsigned int i)
{
    if (i >= m_ChildCount) return 0;
    return m_pChildren[i];
}

void FELayout::Pause(signed char pause)
{
    for (unsigned int i = 0; i < m_ChildCount; i++)
        if (m_pChildren[i]) m_pChildren[i]->Pause(pause);
}

FEObject *FELayout::SetMathmos(FEObjColour::Flags which, FEMathmos m, signed char b)
{
    if (b) {
        for (unsigned int i = 0; i < m_ChildCount; i++)
            if (m_pChildren[i]) m_pChildren[i]->SetMathmos(which, m, 1);
    }
    FEObject::SetMathmos(which, m, 1);
    return this;
}

FEObject *FELayout::SetRemoteMathmos(FEObjColour::Flags which, FEMathmos *p, signed char b)
{
    if (b) {
        for (unsigned int i = 0; i < m_ChildCount; i++)
            if (m_pChildren[i]) m_pChildren[i]->SetRemoteMathmos(which, p, 1);
    }
    FEObject::SetRemoteMathmos(which, p, 1);
    return this;
}

FEObject *FELayout::MathmosGoto(FEMathmosPos::Flags pos)
{
    for (unsigned int i = 0; i < m_ChildCount; i++)
        if (m_pChildren[i]) m_pChildren[i]->MathmosGoto(pos);
    FEObject::MathmosGoto(pos);
    return this;
}

FEObject *FELayout::SetFont(unsigned int file, unsigned int hash, signed char b)
{
    if (b) {
        for (unsigned int i = 0; i < m_ChildCount; i++)
            if (m_pChildren[i]) m_pChildren[i]->SetFont(file, hash, 1);
    }
    m_Font[0] = file;
    m_Font[1] = hash;
    return this;
}

FEObject *FELayout::SetEventCallback(FEEventCallback p, signed char b)
{
    if (b) {
        for (unsigned int i = 0; i < m_ChildCount; i++)
            if (m_pChildren[i]) m_pChildren[i]->SetEventCallback(p, 1);
    }
    m_pEventCallback = p;
    return this;
}

signed char FELayout::DoEventCallback(FEObjEvent::Flags ev)
{
    if (m_pEventCallback) return m_pEventCallback(this, ev);
    FEObject *p = ChildSel();
    if (p) return p->DoEventCallback(ev);
    return 0;
}

FEObject *FELayout::SetFocus(signed char focus, signed char b, signed char c)
{
    if (!(m_Flags & 8)) {
        if (ChildSel()) ChildSel()->SetFocus(focus, b, c);
        return this;
    }
    for (unsigned int i = 0; i < m_ChildCount; i++)
        if (m_pChildren[i]) m_pChildren[i]->SetFocus(focus, b, 1);
    FEObject::SetFocus(focus, b, c);
    return this;
}

void FELayout::NotifyDisplay(signed char show)
{
    for (unsigned int i = 0; i < m_ChildCount; i++)
        if (m_pChildren[i]) m_pChildren[i]->NotifyDisplay(show);
    if (m_pEventCallback) {
        if (show) m_pEventCallback(this, FEObjEvent::OE_SHOW);
        else m_pEventCallback(this, FEObjEvent::OE_HIDE);
    }
}

void FELayout::BindCommit()
{
    for (unsigned int i = 0; i < m_ChildCount; i++)
        if (m_pChildren[i]) m_pChildren[i]->BindCommit();
    FEObject::BindCommit();
}

signed char FELayout::BindRecall()
{
    signed char r = 0;
    for (unsigned int i = 0; i < m_ChildCount; i++)
        if (m_pChildren[i]) r |= m_pChildren[i]->BindRecall();
    signed char r2 = FEObject::BindRecall();
    r2 |= r;
    return r2;
}

signed char FELayout::HandleInput(signed char b)
{
    signed char r = 0;
    if (!(m_Flags & 0x10000)) return 0;
    for (unsigned int i = 0; i < m_ChildCount; i++) {
        if (m_pChildren[i]) {
            if (m_pChildren[i]->HandleInput(!r) || r) r = 1;
        }
    }
    signed char r2 = FEObject::HandleInput(b);
    r2 |= r;
    return r2;
}

void FELayout::Update()
{
    for (unsigned int i = 0; i < m_ChildCount; i++)
        if (m_pChildren[i]) m_pChildren[i]->Update();
    DoEventCallback(FEObjEvent::OE_UPDATE);
}

void FELayout::Draw()
{
    unsigned int i;
    for (i = 0; i < m_ChildCount; i++) {
        FEObject *p = m_pChildren[i];
        if (p && p->DeleteNeeded()) {
            delete p;
            m_pChildren[i] = 0;
        }
    }
    if (m_Flags & 0x40) {
        DoObjectAlign();
        UpdateKeyframes();
        MathmosUpdate();
        for (i = 0; i < m_ChildCount; i++)
            if (m_pChildren[i]) m_pChildren[i]->Draw();
    }
}

void FELayout::DebugDump(unsigned int n)
{
    for (unsigned int i = 0; i < m_ChildCount; i++)
        if (m_pChildren[i]) m_pChildren[i]->DebugDump(n + 1);
    DebugProperties(DebugName(), n);
}

FEObject *FELayout::FindChildObject(unsigned __int64 data)
{
    if (m_Data == data) return this;
    for (unsigned int i = 0; i < m_ChildCount; i++) {
        FEObject *p = m_pChildren[i];
        if (p) {
            if (p->Data() == data) return m_pChildren[i];
            p = m_pChildren[i]->FindChildObject(data);
            if (p) return p;
        }
    }
    return 0;
}

FEListener *FELayout::FindChildListener(unsigned __int64 id)
{
    FEListener *p = Listener(id);
    if (p) return p;
    for (unsigned int i = 0; i < m_ChildCount; i++) {
        if (m_pChildren[i]) {
            p = m_pChildren[i]->FindChildListener(id);
            if (p) return p;
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------
// FELayoutLayered

FELayoutLayered::FELayoutLayered() {}

void FELayoutLayered::InitLayered(unsigned int count, unsigned int x, unsigned int y, unsigned int w,
                                  unsigned int h, EXW::TextAlign align)
{
    m_ChildCount = count;
    m_pChildren = (FEObject **)_EXAlloc(count * sizeof(FEObject *), 0);
    m_ObjectAlign = align;
    memset(m_pChildren, 0, count * sizeof(FEObject *));
    InitRect(x, y, w, h);
}

// NEAR: inline Rect() expansion computes x before y and keeps the child pointer live (register allocation)
FEObject *FELayoutLayered::Resize()
{
    if (!(m_Flags & 0x100)) {
        for (unsigned int i = 0; i < m_ChildCount; i++)
            if (m_pChildren[i]) m_pChildren[i]->SetRect(Rect());
    }
    return this;
}

// ---------------------------------------------------------------------------------------------
// FELayoutPaged

// NEAR: same as FELayoutLayered::Resize (Rect() expansion order, child pointer kept in a register)
FEObject *FELayoutPaged::Resize()
{
    if (!(m_Flags & 0x100)) {
        for (unsigned int i = 0; i < m_ChildCount; i++)
            if (m_pChildren[i]) m_pChildren[i]->SetRect(Rect());
    }
    return this;
}

signed char FELayoutPaged::HandleInput(signed char b)
{
    if (!(m_Flags & 0x10000)) return 0;
    signed char r = FEObject::HandleInput(b);
    if (m_pPage) {
        if (m_pPage->HandleInput(!r) || r) r = 1;
    }
    if (!r) {
        unsigned int sound = 0;
        if (FEListener::QueryButton(0x8000, (FEPad::Flags)0xf, 1)) {
            if (m_pPage && m_pPage->DoEventCallback((FEObjEvent::Flags)0x16))
                sound = gCurrentMap_GeoHash != XApp::Frontend3DFile() ? HT_Sfx_FE_Select : HT_Sfx_FE_Back;
            else if (PageBack())
                sound = HT_Sfx_FE_Back;
            r = 1;
        }
        if (FEListener::QueryButton(0x2000, (FEPad::Flags)0xf, 1)) {
            if (m_pPage && m_pPage->DoEventCallback((FEObjEvent::Flags)0x17)) sound = HT_Sfx_FE_Select;
            r = 1;
        }
        if (sound && (m_pOwnerWnd->m_State & 0x8000000)) EXBaseSoundManager::GetSoundManager()->StartSound(sound);
    }
    return r;
}

void FELayoutPaged::Update()
{
    if (m_pPage) m_pPage->Update();
}

void FELayoutPaged::Draw()
{
    unsigned int flags = m_Flags;
    m_Flags = 0;
    FELayout::Draw();
    if (m_pListener) {
        unsigned __int64 cur = m_pPage ? m_pPage->Data() : 0xfefffffe;
        FEObject *pFocus = m_pListener->Focus();
        unsigned __int64 want = pFocus ? pFocus->Data() : 0xff000000;
        if (cur != want) PageSelect(want, 0, 1);
    }
    if (m_pPage) m_pPage->Draw();
    m_Flags = flags;
}

FEObject *FELayoutPaged::Page(unsigned __int64 id, int *pIndex)
{
    for (unsigned int i = 0; i < m_ChildCount; i++) {
        if (m_pChildren[i] && id == m_pChildren[i]->Data()) {
            if (pIndex) *pIndex = i;
            return m_pChildren[i];
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------
// FELayoutFixedGrid

FELayoutFixedGrid::FELayoutFixedGrid(unsigned int cols, unsigned int rows, unsigned int x, unsigned int y,
                                     unsigned int w, unsigned int h, EXW::TextAlign align)
{
    m_Cols = cols;
    m_Rows = rows;
    m_ChildCount = cols * rows;
    m_pChildren = (FEObject **)_EXAlloc(cols * rows * sizeof(FEObject *), 0);
    m_ObjectAlign = align;
    memset(m_pChildren, 0, cols * rows * sizeof(FEObject *));
    InitRect(x, y, w, h);
}

FEObject *FELayoutFixedGrid::ObjectAdd(unsigned int i, unsigned int j, FEObject *p)
{
    if (m_pChildren[m_Cols * j + i]) {
        delete m_pChildren[m_Cols * j + i];
        m_pChildren[m_Cols * j + i] = 0;
    }
    m_pChildren[m_Cols * j + i] = p;
    p->SetOwnerWnd(m_pOwnerWnd);
    p->SetParent(this);
    p->SetEventCallback(m_pEventCallback, 1);
    p->SetHelpText(m_HelpText);
    for (unsigned int k = 0; k < FEObjColour::OC_COUNT; k++) {
        p->SetMathmos((FEObjColour::Flags)k, m_Mathmos[k], 1);
        p->SetRemoteMathmos((FEObjColour::Flags)k, m_pRemoteMathmos[k], 1);
    }
    Resize();
    return p;
}

void FELayoutFixedGrid::ObjectDel(unsigned int i, unsigned int j)
{
    if (i < m_Cols && j < m_Rows && m_pChildren[m_Cols * j + i]) m_pChildren[m_Cols * j + i]->Delete();
}

FEObject *FELayoutFixedGrid::Object(unsigned int i, unsigned int j)
{
    if (i < m_Cols && j < m_Rows) return m_pChildren[m_Cols * j + i];
    return 0;
}

// NEAR: child pointer kept in a register; retail re-reads m_pChildren[] after building the rect
FEObject *FELayoutFixedGrid::Resize()
{
    if (!(m_Flags & 0x100)) {
        unsigned int w = m_Rect.w / m_Rows;
        unsigned int h = m_Rect.h / m_Cols;
        unsigned int x = 0;
        for (unsigned int j = 0; j < m_Rows; j++) {
            unsigned int y = 0;
            for (unsigned int i = 0; i < m_Cols; i++) {
                EXPoint o = Origin();
                if (m_pChildren[m_Cols * j + i]) m_pChildren[m_Cols * j + i]->SetRect(EXRect(x + o.x, y + o.y, w, h));
                y += h;
            }
            x += w;
        }
    }
    return this;
}

// ---------------------------------------------------------------------------------------------
// FELayoutFlexGrid

void FELayoutFlexGrid::Init(unsigned int cols, unsigned int rows, EXW::TextAlign align)
{
    m_Cols = cols;
    m_Rows = rows;
    m_ChildCount = cols * rows;
    m_pChildren = (FEObject **)_EXAlloc(cols * rows * sizeof(FEObject *), 0);
    m_ObjectAlign = align;
    m_Flags |= 2;
    memset(m_pChildren, 0, cols * rows * sizeof(FEObject *));
}

// NEAR: loop register allocation / no 8-byte stack alignment in retail form
FEObject *FELayoutFlexGrid::Resize()
{
    if (!(m_Flags & 0x100)) {
        int hMax[128];
        int wMax[128];
        memset(hMax, 0, sizeof(hMax));
        memset(wMax, 0, sizeof(wMax));
        for (int pass = 0; pass < 2; pass++) {
            unsigned int i, j;
            for (i = 0; i < m_Cols; i++) {
                for (j = 0; j < m_Rows; j++) {
                    FEObject *p = m_pChildren[m_Cols * j + i];
                    if (p) {
                        if (p->m_Rect.h > hMax[i]) hMax[i] = p->m_Rect.h;
                        if (p->m_Rect.w > wMax[j]) wMax[j] = p->m_Rect.w;
                    }
                }
            }
            int y = m_Rect.y + m_BaseRect.y;
            int x = m_BaseRect.x + m_Rect.x;
            for (i = 0; i < m_Cols; i++) {
                x = m_BaseRect.x + m_Rect.x;
                for (j = 0; j < m_Rows; j++) {
                    if (m_pChildren[m_Cols * j + i]) m_pChildren[m_Cols * j + i]->SetRect(EXRect(x, y, wMax[j], hMax[i]));
                    x += wMax[j];
                }
                y += hMax[i];
            }
            m_Rect.w = x - (m_BaseRect.x + m_Rect.x);
            m_Rect.h = y - (m_Rect.y + m_BaseRect.y);
            m_Flags &= ~0x20;
            DoObjectAlign();
        }
    }
    return this;
}

// ---------------------------------------------------------------------------------------------
// FELayoutScrollGrid

// NEAR: register allocation (arrow y / colour spill slots)
signed char FELayoutScrollGrid::HandleInput(signed char b)
{
    if (!(m_Flags & 0x10000)) return FELayout::HandleInput(b);
    FEListener *pListener = Listener(FE_NAV_LISTENER);
    if (b && (m_Flags & 0x40) && pListener && FEListener::IsChildOf(pListener->Focus(), this)) {
        ex::RGBA c = MultiplyMathmos(m_ArrowMathmos.Update(), 2);
        int y = m_LeftRightArrowY;
        if (y < -100) y = Rect().h / 2 + 1;
        y += Rect().y;
        if (m_Visible.y < (int)m_Cols) {
            int arrowY = m_UpDownArrowY;
            if (arrowY < -100) arrowY = 8;
            FEObject *p = Object(m_Top.y, m_Top.x);
            if (m_Top.y > 0 || !p || (p->m_Flags & 0x4000))
                m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollUp, c, 0,
                                        EXPoint(Rect().x + Rect().w / 2, Rect().y + arrowY), EXPoint(8, 8),
                                        (EXW::TextAlign)4);
            p = Object(m_Cols - 1, m_Top.x);
            if ((unsigned int)(m_Top.y + m_Visible.y - 1) < m_Cols - 1 || !p || (p->m_Flags & 0x4000))
                m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollDown, c, 0,
                                        EXPoint(Rect().x + Rect().w / 2, Rect().h + Rect().y - arrowY), EXPoint(8, 8),
                                        (EXW::TextAlign)4);
        }
        if (m_Top.x > 0)
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollLeft, c, 0,
                                    EXPoint(Rect().x + 8, y), EXPoint(8, 8), (EXW::TextAlign)4);
        if (m_Top.x + m_Visible.x < (int)m_Rows)
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollRight, c, 0,
                                    EXPoint(Rect().x + Rect().w - 8, y), EXPoint(8, 8), (EXW::TextAlign)4);
    }
    return FELayout::HandleInput(b);
}

void FELayoutScrollGrid::EnsureVisible(FEObject *p, signed char b)
{
    m_Flags |= 0x40;
    int x = 0;
    int y = 0;
    for (unsigned int i = 0; i < m_Cols; i++) {
        for (unsigned int j = 0; j < m_Rows; j++) {
            if (m_pChildren[m_Cols * j + i] == p) {
                x = j;
                y = i;
                break;
            }
        }
    }
    signed char moved = 0;
    if (m_ScrollAnchor) {
        if (x > m_Top.x + (m_Visible.x >> 1)) {
            m_Top.x = x - (m_Visible.x >> 1);
            moved = 1;
        } else if (x < m_Top.x + (m_Visible.x >> 1)) {
            m_Top.x = x - (m_Visible.x >> 1);
            moved = 1;
        }
        if (y > m_Top.y + (m_Visible.y >> 1)) {
            m_Top.y = y - (m_Visible.y >> 1);
            moved = 1;
        } else if (y < m_Top.y + (m_Visible.y >> 1)) {
            m_Top.y = y - (m_Visible.y >> 1);
            moved = 1;
        }
    } else {
        if (x >= m_Top.x + m_Visible.x) {
            m_Top.x = x - m_Visible.x + 1;
            moved = 1;
        } else if (x < m_Top.x) {
            m_Top.x = x;
            moved = 1;
        }
        if (y >= m_Top.y + m_Visible.y) {
            m_Top.y = y - m_Visible.y + 1;
            moved = 1;
        } else if (y < m_Top.y) {
            m_Top.y = y;
            moved = 1;
        }
    }
    if (moved) {
        m_Scrolling = b && m_SmoothScroll;
        Resize();
    }
    if (m_pParent) m_pParent->EnsureVisible(this, b);
}

FEObject *FELayoutScrollGrid::ChildSel() { return m_pChildren[m_Top.x * m_Cols + m_Top.y]; }

// ---------------------------------------------------------------------------------------------
// FELayoutFlexScrollGrid

// NEAR: Rect() call order in the arrow tests and bool materialisation differ
signed char FELayoutFlexScrollGrid::HandleInput(signed char b)
{
    FEObject *pLast = 0;
    if (!(m_Flags & 0x10000)) return FELayout::HandleInput(b);
    FEListener *pListener = Listener(FE_NAV_LISTENER);
    if (b && (m_Flags & 0x40) && pListener && FEListener::IsChildOf(pListener->Focus(), this)) {
        pLast = m_pChildren[m_Rows * m_Cols - 1];
        FEObject *pFirst = m_pChildren[0];
        ex::RGBA c = MultiplyMathmos(m_ArrowMathmos.Update(), 2);
        if (pFirst->Rect().y < Rect().y || pLast->Rect().y > Rect().h + Rect().y || (pFirst->m_Flags & 0x4000))
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollUp, c, 0,
                                    EXPoint(Rect().x + Rect().w / 2, Rect().y + 8), EXPoint(8, 8), (EXW::TextAlign)4);
        if (pLast->Rect().h + pLast->Rect().y > Rect().h + Rect().y || pLast->Rect().y > Rect().h + Rect().y ||
            (pLast->m_Flags & 0x4000))
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollDown, c, 0,
                                    EXPoint(Rect().x + Rect().w / 2, Rect().h + Rect().y - 8), EXPoint(8, 8),
                                    (EXW::TextAlign)4);
        signed char left = pFirst->Rect().x < Rect().x;
        if (left)
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollLeft, c, 0,
                                    EXPoint(Rect().x + 8, Rect().h / 2 + Rect().y), EXPoint(8, 8), (EXW::TextAlign)4);
        signed char right = pLast->Rect().x + pLast->Rect().w > Rect().x + Rect().w;
        if (right)
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollRight, c, 0,
                                    EXPoint(Rect().x + Rect().w - 8, Rect().h / 2 + Rect().y), EXPoint(8, 8),
                                    (EXW::TextAlign)4);
    }
    return FELayout::HandleInput(b);
}

FEObject *FELayoutFlexScrollGrid::ChildSel() { return m_pChildren[0]; }

// ---------------------------------------------------------------------------------------------
// FELayoutCurve

FELayoutCurve::FELayoutCurve(unsigned int count, EXW::TextAlign curve, EXW::TextAlign childAlign, unsigned int x,
                             unsigned int y, unsigned int w, unsigned int h, EXW::TextAlign align)
{
    m_ChildCount = count;
    m_Curve = curve;
    m_ChildAlign = childAlign;
    m_pChildren = (FEObject **)_EXAlloc(count * sizeof(FEObject *), 0);
    m_ObjectAlign = align;
    memset(m_pChildren, 0, count * sizeof(FEObject *));
    InitRect(x, y, w, h);
}

// ---------------------------------------------------------------------------------------------
// FELayoutSpin

FEObject *FELayoutSpin::SetRange(int lo, int hi)
{
    if (lo <= hi) {
        m_Min = lo;
        m_Max = hi;
        Resize();
    }
    return this;
}

int FELayoutSpin::Range(int &lo, int &hi)
{
    lo = m_Min;
    hi = m_Max;
    return m_Value;
}

FEObject *FELayoutSpin::SetValue(int v)
{
    m_Value = v;
    Resize();
    return this;
}

FEObject *FELayoutSpin::Resize()
{
    if (m_Value < m_Min) m_Value = m_Min;
    else if (m_Value > m_Max) m_Value = m_Max;
    char buf[16];
    sprintf(buf, "%d", m_Value);
    m_Text = buf;
    return this;
}

void FELayoutSpin::BindUpdate(unsigned __int64 data)
{
    if ((unsigned int)data == FE_DEFAULT) data = m_Value;
    if (m_pBind) {
        m_BindValue = data;
        if (m_Flags & 0x800) BindCommit();
    } else if (m_pParent) {
        m_pParent->BindUpdate(data);
    }
}

signed char FELayoutSpin::BindRecall()
{
    if (m_pBind) {
        switch (m_BindType) {
        case FEDataType::DT_S8: m_BindValue = *(signed char *)m_pBind; break;
        case FEDataType::DT_U8: m_BindValue = *(unsigned char *)m_pBind; break;
        case FEDataType::DT_U32: m_BindValue = *(unsigned int *)m_pBind; break;
        case FEDataType::DT_INT: m_BindValue = *(int *)m_pBind; break;
        case FEDataType::DT_U64: m_BindValue = *(unsigned __int64 *)m_pBind; break;
        default: m_BindValue = 0; break;
        }
        SetValue((int)m_BindValue);
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------
// FELayoutCheckbox

FELayoutCheckbox::FELayoutCheckbox(signed char b, unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                                   EXW::TextAlign align)
{
    m_Checked = 0;
    m_Glyph = b;
    m_ObjectAlign = align;
    m_Flags |= 8;
    InitRect(x, y, w, h);
}

signed char FELayoutCheckbox::HandleInput(signed char b)
{
    signed char r = 0;
    if (!(m_Flags & 0x10000)) return 0;
    FEListener *pListener = Listener(FE_NAV_LISTENER);
    if (b && (m_Flags & 0x40) && pListener && FEListener::IsChildOf(pListener->Focus(), this)) {
        r = DoEventCallback((FEObjEvent::Flags)0x28);
        if (!r && FEListener::QueryButton(0x2000, (FEPad::Flags)0xf, 1)) {
            m_Checked = !m_Checked;
            BindUpdate(FE_DEFAULT);
            r = DoEventCallback((FEObjEvent::Flags)0x19);
            if (m_Flags & 0x8000000) EXBaseSoundManager::GetSoundManager()->StartSound(HT_Sfx_FE_Select);
        }
    }
    return r;
}

FEObject *FELayoutCheckbox::SetValue(signed char b)
{
    m_Checked = b;
    return this;
}

void FELayoutCheckbox::BindUpdate(unsigned __int64 data)
{
    if ((unsigned int)data == FE_DEFAULT) data = m_Checked;
    if (m_pBind) {
        m_BindValue = data;
        if (m_Flags & 0x800) BindCommit();
    } else if (m_pParent) {
        m_pParent->BindUpdate(data);
    }
}

signed char FELayoutCheckbox::BindRecall()
{
    if (m_pBind) {
        switch (m_BindType) {
        case FEDataType::DT_S8: m_BindValue = *(signed char *)m_pBind; break;
        default: m_BindValue = 0; break;
        }
        SetValue((signed char)m_BindValue);
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------
// FELayoutSlider

signed char FELayoutSlider::HandleInput(signed char b)
{
    signed char r = 0;
    if (!(m_Flags & 0x10000)) return 0;
    FEListener *pListener = Listener(FE_NAV_LISTENER);
    if (b && (m_Flags & 0x40) && pListener && FEListener::IsChildOf(pListener->Focus(), this)) {
        if (m_Value > m_Min &&
            (m_KeyRepeater.QueryButton(4, (FEPad::Flags)0xf) || m_KeyRepeater.QueryButton(0x40, (FEPad::Flags)0xf))) {
            m_Value -= m_Delta;
            if (m_Value < m_Min) m_Value = m_Min;
            BindUpdate(FE_DEFAULT);
            Resize();
            r = DoEventCallback((FEObjEvent::Flags)0x19);
            if (m_Flags & 0x8000000) EXBaseSoundManager::GetSoundManager()->StartSound(HT_Sfx_FE_Change);
        }
        if (m_Value < m_Max &&
            (m_KeyRepeater.QueryButton(8, (FEPad::Flags)0xf) || m_KeyRepeater.QueryButton(0x80, (FEPad::Flags)0xf))) {
            m_Value += m_Delta;
            if (m_Value > m_Max) m_Value = m_Max;
            BindUpdate(FE_DEFAULT);
            Resize();
            r = DoEventCallback((FEObjEvent::Flags)0x19);
            if (m_Flags & 0x8000000) EXBaseSoundManager::GetSoundManager()->StartSound(HT_Sfx_FE_Change);
        }
    }
    return r;
}

FEObject *FELayoutSlider::SetRange(int lo, int hi)
{
    if (lo <= hi) {
        m_Min = lo;
        m_Max = hi;
        Resize();
    }
    return this;
}

int FELayoutSlider::Range(int &lo, int &hi)
{
    lo = m_Min;
    hi = m_Max;
    return m_Value;
}

FEObject *FELayoutSlider::SetValue(int v)
{
    m_Value = v;
    Resize();
    return this;
}

FEObject *FELayoutSlider::Resize()
{
    if (m_Value < m_Min) m_Value = m_Min;
    else if (m_Value > m_Max) m_Value = m_Max;
    m_BarRect = m_Rect;
    if (m_BarRect.h > 8) {
        m_BarRect.y += (m_BarRect.h - 8) / 2;
        m_BarRect.h = 8;
    }
    if (m_BarRect.w > 32) {
        m_BarRect.x += 16;
        m_BarRect.w -= 32;
    }
    return this;
}

void FELayoutSlider::BindUpdate(unsigned __int64 data)
{
    if ((unsigned int)data == FE_DEFAULT) data = m_Value;
    if (m_pBind) {
        m_BindValue = data;
        if (m_Flags & 0x800) BindCommit();
    } else if (m_pParent) {
        m_pParent->BindUpdate(data);
    }
}

signed char FELayoutSlider::BindRecall()
{
    if (m_pBind) {
        switch (m_BindType) {
        case FEDataType::DT_S8: m_BindValue = *(signed char *)m_pBind; break;
        case FEDataType::DT_U32: m_BindValue = *(unsigned int *)m_pBind; break;
        case FEDataType::DT_INT: m_BindValue = *(int *)m_pBind; break;
        case FEDataType::DT_U64: m_BindValue = *(unsigned __int64 *)m_pBind; break;
        default: m_BindValue = 0; break;
        }
        SetValue((int)m_BindValue);
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------
// Constructors and remaining members (retail emits these after the inline helpers).

FELayoutLayered::FELayoutLayered(unsigned int count, unsigned int x, unsigned int y, unsigned int w,
                                 unsigned int h, EXW::TextAlign align)
{
    InitLayered(count, x, y, w, h, align);
}

FELayoutFlexGrid::FELayoutFlexGrid(unsigned int cols, unsigned int rows, EXW::TextAlign align)
{
    Init(cols, rows, align);
}

// NEAR: member stores interleave differently with the inlined DefaultScroll() argument setup
FELayoutScrollGrid::FELayoutScrollGrid(unsigned int cols, unsigned int rows, unsigned int x, unsigned int y,
                                       unsigned int w, unsigned int h, EXW::TextAlign align)
{
    m_ChildCount = cols * rows;
    m_Cols = cols;
    m_Rows = rows;
    m_pChildren = (FEObject **)_EXAlloc(cols * rows * sizeof(FEObject *), 0);
    m_ObjectAlign = align;
    m_Visible.x = rows;
    m_Visible.y = cols;
    m_Top.x = 0;
    m_Top.y = 0;
    m_Scrolling = 0;
    m_SmoothScroll = 1;
    m_ScrollAnchor = 0;
    m_ArrowMathmos = FEMathmos::DefaultScroll();
    m_LeftRightArrowY = -101;
    m_UpDownArrowY = -101;
    m_pGridWidths = 0;
    memset(m_pChildren, 0, cols * rows * sizeof(FEObject *));
    InitRect(x, y, w, h);
}

// NEAR: register allocation and StateSet/RectInside ordering
FEObject *FELayoutScrollGrid::Resize()
{
    if (!(m_Flags & 0x100)) {
        int h = m_Rect.h / m_Visible.y;
        int w = m_Rect.w / m_Visible.x;
        for (int i = 0; i < (int)m_Cols; i++) {
            int x = 0;
            for (int j = 0; j < (int)m_Rows; j++) {
                if (m_pGridWidths) w = (int)(m_Rect.w * m_pGridWidths[j]);
                else x = (j - m_Top.x) * w;
                FEObject *p = m_pChildren[m_Cols * j + i];
                if (p) {
                    EXPoint o = Origin();
                    EXRect r(o.x + x, (i - m_Top.y) * h + o.y, w, h);
                    p->KeyframeClear();
                    if (m_pGridWidths && (p->m_Flags & 0x100)) {
                        EXRect a = FEBaseWnd::AlignRectToRect(p->Rect(), r, p->ObjectAlign());
                        r.x = a.x;
                        r.y = a.y;
                        r.w = p->m_Rect.w;
                        r.h = p->m_Rect.h;
                    }
                    if (m_Scrolling) {
                        p->SetSize(EXPoint(r.w, r.h));
                        p->KeyframeAdd(EXPoint(r.x, r.y), 15.0f);
                    } else {
                        p->SetRect(r);
                    }
                    p->StateSet(RectInside(r) || RectInside(p->Rect()) ? p->m_Flags & ~0x80 : p->m_Flags | 0x80);
                }
                x += w;
            }
        }
        m_Scrolling = 0;
    }
    return this;
}

// NEAR: two byte stores scheduled around the DefaultScroll() argument pushes
FELayoutFlexScrollGrid::FELayoutFlexScrollGrid(unsigned int cols, unsigned int rows, unsigned int x,
                                               unsigned int y, unsigned int w, unsigned int h,
                                               EXW::TextAlign align)
{
    m_Cols = cols;
    m_ChildCount = cols * rows;
    m_Rows = rows;
    m_pChildren = (FEObject **)_EXAlloc(cols * rows * sizeof(FEObject *), 0);
    m_ObjectAlign = align;
    m_Scrolling = 0;
    m_SmoothScroll = 1;
    m_Scroll.x = 0;
    m_Scroll.y = 0;
    m_ArrowMathmos = FEMathmos::DefaultScroll();
    memset(m_pChildren, 0, cols * rows * sizeof(FEObject *));
    InitRect(x, y, w, h);
}

void FELayoutFlexScrollGrid::EnsureVisible(FEObject *p, signed char b)
{
    m_Flags |= 0x40;
    if (p) {
        p->KeyframeClear();
        EXPoint old = m_Scroll;
        EXRect r = Rect();
        EXRect pr = p->Rect();
        if (pr.x < r.x) m_Scroll.x += r.x - pr.x;
        else if (pr.x + pr.w > r.x + r.w) m_Scroll.x += r.w - pr.w - pr.x + r.x;
        if (pr.y < r.y) m_Scroll.y += r.y - pr.y;
        else if (pr.y + pr.h > r.y + r.h) m_Scroll.y += r.h - pr.h - pr.y + r.y;
        if (m_Scroll != old) {
            m_Scrolling = b && m_SmoothScroll;
            Resize();
        }
    }
    if (m_pParent) m_pParent->EnsureVisible(this, b);
}

static inline signed char FEGreater(int a, int b) { return a > b; }

// NEAR: register allocation / stack alignment
FEObject *FELayoutFlexScrollGrid::Resize()
{
    if (!(m_Flags & 0x100)) {
        int hMax[128];
        int wMax[128];
        memset(hMax, 0, sizeof(hMax));
        memset(wMax, 0, sizeof(wMax));
        unsigned int i, j;
        for (i = 0; i < m_Cols; i++) {
            for (j = 0; j < m_Rows; j++) {
                FEObject *p = m_pChildren[m_Cols * j + i];
                if (p) {
                    p->KeyframeClear();
                    if (FEGreater(p->m_Rect.h, hMax[i])) hMax[i] = p->m_Rect.h;
                    if (FEGreater(p->m_Rect.w, wMax[j])) wMax[j] = p->m_Rect.w;
                }
            }
        }
        int y = m_BaseRect.y + m_Rect.y;
        for (i = 0; i < m_Cols; i++) {
            int x = m_Rect.x + m_BaseRect.x;
            for (j = 0; j < m_Rows; j++) {
                FEObject *p = m_pChildren[m_Cols * j + i];
                if (p) {
                    int sx = m_Scroll.x + x;
                    int sy = m_Scroll.y + y;
                    if (m_Scrolling) {
                        p->SetSize(EXPoint(wMax[j], hMax[i]));
                        p->KeyframeAdd(EXPoint(sx, sy), (float)(max(abs(m_Scroll.x), abs(m_Scroll.y)) >> 2));
                    } else {
                        p->SetRect(EXRect(sx, sy, wMax[j], hMax[i]));
                    }
                    p->StateSet(RectInside(EXRect(sx, sy, wMax[j], hMax[i])) || RectInside(p->Rect()) ? p->m_Flags & ~0x80
                                                                                                    : p->m_Flags | 0x80);
                }
                x += wMax[j];
            }
            y += hMax[i];
            m_Flags &= ~0x20;
            DoObjectAlign();
        }
    }
    return this;
}

// NEAR: empty `case 2` of the child-align switch is dropped; `again` flag spilled instead of bl
FEObject *FELayoutCurve::Resize()
{
    if (!(m_Flags & 0x100)) {
        unsigned int n = 0;
        int lastH = 0;
        unsigned int i;
        for (i = 0; i < m_ChildCount; i++) {
            if (m_pChildren[i]) {
                n++;
                m_pChildren[i]->LockTextRect(0, 0, 0, 0);
                lastH = m_pChildren[i]->m_Rect.h;
            }
        }
        if (n) {
            int step = m_Rect.h / n - 1;
            int range = m_Rect.h - lastH;
            int x0 = m_Rect.x;
            int amp = m_Rect.w;
            signed char again;
            do {
                again = 0;
                int y = 0;
                unsigned int k = 0;
                for (i = 0; i < m_ChildCount; i++) {
                    if (!m_pChildren[i]) continue;
                    int x = 0;
                    switch (m_Curve) {
                    case 6: x = (int)(sin((float)y / range * FE_HALFPI) * amp) + x0 + 1; break;
                    case 0: x = (int)((1.0f - sin((float)y / range * FE_HALFPI)) * amp) + x0 + 1; break;
                    case 1: x = (int)((1.0f - cos((float)y / range * FE_HALFPI)) * amp) + x0 + 1; break;
                    case 8: x = (int)(cos((float)y / range * FE_HALFPI) * amp) + x0 + 1; break;
                    }
                    switch (m_ChildAlign) {
                    case 2: break;
                    case 4: x -= m_pChildren[k]->m_Rect.w / 2; break;
                    case 7: x -= m_pChildren[k]->m_Rect.w; break;
                    }
                    FEObject *p = m_pChildren[k];
                    if (FEGreater(p->m_Rect.w + x, m_Rect.w)) {
                        amp += m_Rect.w - p->m_Rect.w - x;
                        again = 1;
                    } else if (x < 0) {
                        x0 -= x;
                        again = 1;
                    } else {
                        if (p) p->SetOrigin(EXPoint(Origin().x + x, Origin().y + y), (EXW::TextAlign)0);
                        y += step;
                        k++;
                    }
                }
            } while (again);
        }
    }
    return this;
}

FELayoutSpin::FELayoutSpin(unsigned int lo, unsigned int hi, unsigned int x, unsigned int y, unsigned int w,
                           unsigned int h, EXW::TextAlign align)
{
    m_Min = 0;
    m_Max = 0;
    m_Value = lo;
    m_Text = EXString("0");
    m_ObjectAlign = align;
    m_Unk1cc = 0;
    m_Delta = 1;
    m_KeyRepeater = FEKeyRepeater(EXBaseApp::NominalGameFrameRate() / 2, 10, 2, 4);
    m_ArrowMathmos = FEMathmos::DefaultScroll();
    SetRange(lo, hi);
    m_Flags |= 8;
    InitRect(x, y, w, h);
}

// NEAR: r spilled to the stack (0x8000000 kept in ebx); arrow drawing register allocation
signed char FELayoutSpin::HandleInput(signed char b)
{
    signed char r = 0;
    if (!(m_Flags & 0x10000)) return 0;
    FEListener *pListener = Listener(FE_NAV_LISTENER);
    if (b && (m_Flags & 0x40) && pListener && FEListener::IsChildOf(pListener->Focus(), this)) {
        if (m_Value > m_Min &&
            (m_KeyRepeater.QueryButton(4, (FEPad::Flags)0xf) || m_KeyRepeater.QueryButton(0x40, (FEPad::Flags)0xf))) {
            m_Value -= m_Delta;
            if (m_Value < m_Min) m_Value = m_Min;
            BindUpdate(FE_DEFAULT);
            Resize();
            r = DoEventCallback((FEObjEvent::Flags)0x19);
            if (m_Flags & 0x8000000) EXBaseSoundManager::GetSoundManager()->StartSound(HT_Sfx_FE_Change);
        }
        if (m_Value < m_Max &&
            (m_KeyRepeater.QueryButton(8, (FEPad::Flags)0xf) || m_KeyRepeater.QueryButton(0x80, (FEPad::Flags)0xf))) {
            m_Value += m_Delta;
            if (m_Value > m_Max) m_Value = m_Max;
            BindUpdate(FE_DEFAULT);
            Resize();
            r = DoEventCallback((FEObjEvent::Flags)0x19);
            if (m_Flags & 0x8000000) EXBaseSoundManager::GetSoundManager()->StartSound(HT_Sfx_FE_Change);
        }
        ex::RGBA c = MultiplyMathmos(m_ArrowMathmos.Update(), 2);
        if (m_Value > m_Min)
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollLeft, c, 0,
                                    EXPoint(Rect().x + 8, Rect().h / 2 + Rect().y), EXPoint(8, 8), (EXW::TextAlign)4);
        if (m_Value < m_Max)
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollRight, c, 0,
                                    EXPoint(Rect().x + Rect().w - 8, Rect().h / 2 + Rect().y), EXPoint(8, 8),
                                    (EXW::TextAlign)4);
    }
    DrawText((FERenderPass::Flags)2, m_Font[0], m_Font[1], m_Colour, m_BlendMode, m_Rect, m_Text, m_TextAlign, 0);
    return r;
}

// NEAR: colour select compiles to sbb instead of the retail branch; m_Rect copied through a pointer
void FELayoutCheckbox::Draw()
{
    FELayout::Draw();
    u8 v;
    if (m_Flags & 4) v = 0xff;
    else v = 0x40;
    ex::RGBA c(v, v, v, 0x80);
    if ((m_Checked && !m_Glyph) || (!m_Checked && m_Glyph))
        DrawText((FERenderPass::Flags)2, m_Font[0], m_Font[1], c, m_BlendMode, m_Rect, "~I", m_TextAlign, 0);
    else
        DrawText((FERenderPass::Flags)2, m_Font[0], m_Font[1], c, m_BlendMode, m_Rect, "~G", m_TextAlign, 0);
}

FELayoutSlider::FELayoutSlider(unsigned int lo, unsigned int hi, unsigned int x, unsigned int y, unsigned int w,
                               unsigned int h, EXW::TextAlign align)
{
    m_Min = 0;
    m_Max = 0;
    m_Value = lo;
    m_ObjectAlign = align;
    m_Unk1cc = 0;
    m_Delta = 1;
    m_KeyRepeater = FEKeyRepeater(EXBaseApp::NominalGameFrameRate() / 2, 10, 2, 4);
    m_BarRect.x = 0;
    m_BarRect.y = 0;
    m_BarRect.w = 0;
    m_BarRect.h = 0;
    m_ArrowMathmos = FEMathmos::DefaultScroll();
    SetRange(lo, hi);
    m_Flags |= 8;
    InitRect(x, y, w, h);
}

signed char FELayoutPaged::PageBack()
{
    FEObject *p = 0;
    m_History.Pop(p);
    if (p) {
        PageSelect(p->Data(), 0, 1);
        return 1;
    }
    return 0;
}

void FELayoutPaged::PageFlush()
{
    FEObject *p;
    while (m_History.Pop(p)) {
    }
}

FELayoutPaged::~FELayoutPaged()
{
    if (m_pPage) {
        m_pPage->DoEventCallback((FEObjEvent::Flags)0x14);
        m_pPage->NotifyDisplay(0);
    }
    if (m_pPageData) EXFree(m_pPageData);
    m_pPageData = 0;
}

void FELayoutPaged::PageSelect(unsigned __int64 id, signed char push, signed char notify)
{
    if (m_pPage) {
        if (push) m_History.Push(m_pPage);
        if (notify) m_pPage->NotifyDisplay(0);
        m_pPage->DoEventCallback((FEObjEvent::Flags)0x14);
    }
    int index = 0;
    m_pPage = Page(id, &index);
    if (m_pPage) {
        m_pPage->DoEventCallback((FEObjEvent::Flags)0x13);
        m_pPage->Update();
        if (notify) m_pPage->NotifyDisplay(1);
    }
}

// NEAR: inline Rect() expansion order (x before y)
void FELayoutScrollGrid::Draw()
{
    if (!(m_Flags & 0x40000)) m_pOwnerWnd->AddClipRect(Rect());
    FELayout::Draw();
    if (!(m_Flags & 0x40000)) m_pOwnerWnd->RestoreClipRect();
}

// NEAR: inline Rect() expansion order (x before y)
void FELayoutFlexScrollGrid::Draw()
{
    if (!(m_Flags & 0x40000)) m_pOwnerWnd->AddClipRect(Rect());
    FELayout::Draw();
    if (!(m_Flags & 0x40000)) m_pOwnerWnd->RestoreClipRect();
}

// NEAR: register allocation around the arrow sprites and the bar colours
void FELayoutSlider::Draw()
{
    m_pOwnerWnd->SetClipRect(EXRect(0, 0, EXBaseDisplay::GetDisplay()->Width(), EXBaseDisplay::GetDisplay()->Height()));
    FELayout::Draw();
    u8 v;
    if (m_Flags & 4) {
        ex::RGBA c = MultiplyMathmos(m_ArrowMathmos.Update(), 2);
        if (m_Value > m_Min)
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollLeft, c, 0,
                                    EXPoint(Rect().x + 8, Rect().h / 2 + Rect().y), EXPoint(8, 8), (EXW::TextAlign)4);
        if (m_Value < m_Max)
            m_pOwnerWnd->DrawSprite((FERenderPass::Flags)3, HT_File_HUD, HT_Texture_FEScrollRight, c, 0,
                                    EXPoint(Rect().x + Rect().w - 8, Rect().h / 2 + Rect().y), EXPoint(8, 8),
                                    (EXW::TextAlign)4);
        v = 0xff;
    } else {
        v = 0x40;
    }
    ex::RGBA c(v, v, v, 0x80);
    DrawSprite((FERenderPass::Flags)2, HT_File_HUD, HT_Texture_FEWhite, MultiplyColour(ex::RGBA(0x5f, 0x5f, 0x5f, 0x80), c),
               0, m_BarRect, EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    int w = (int)((float)m_Value / (float)m_Max * (float)m_BarRect.w);
    DrawSprite((FERenderPass::Flags)2, HT_File_HUD, HT_Texture_FEWhite, MultiplyColour(ex::RGBA(0, 0x7f, 0, 0x80), c),
               0, EXRect(m_BarRect.x, m_BarRect.y, w, m_BarRect.h), EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    m_pOwnerWnd->RestoreClipRect();
}

FELayoutPaged::FELayoutPaged(unsigned int count, FEListener *pListener, unsigned int x, unsigned int y,
                             unsigned int w, unsigned int h, EXW::TextAlign align)
    : m_History(0x80)
{
    m_ChildCount = count;
    m_pChildren = (FEObject **)_EXAlloc(count * sizeof(FEObject *), 0);
    m_pPageData = (void **)_EXAlloc(count * sizeof(void *), 0);
    m_ObjectAlign = align;
    m_pListener = pListener;
    m_Unk1cc = 0;
    m_pPage = 0;
    m_Unk1c8 = 0xfdffffff;
    memset(m_pChildren, 0, count * sizeof(FEObject *));
    memset(m_pPageData, 0, count * sizeof(void *));
    InitRect(x, y, w, h);
}
