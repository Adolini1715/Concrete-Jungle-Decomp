#include "EXBaseDisplay.h"
#include "EXWnd.h"
#include "EXMemory.h"

#include "EXMenu.h"
#include "EXWatcher.h"
#include "EXStdFuncs.h"


// TODO stand-in: texture (XBTexture area). Runtime textures are 0x50-byte entries with a
// virtual interface at +0xc (slot 5 creates the surface, slot 6 releases it).
class EXTextureImpl {
public:
    virtual void Unk0();
    virtual void Unk1();
    virtual void Unk2();
    virtual void Unk3();
    virtual void Unk4();
    virtual s8 Create(unsigned short w, unsigned short h, unsigned char fmt, unsigned int flags);
    virtual void Release();
};
class EXTexture {
public:
    u8 m_Unk00[0xc];
    EXTextureImpl m_Impl;  // 0x0c
    u8 m_Unk10[0x40];
};
// TODO stand-in: texture array (XBTexture area); entries in pages of 64.
class EXTextureArray {
public:
    EXTextureArray()
    {
        m_Unk00 = 0;
        m_Unk04 = 0;
        m_ppPages = 0;
        m_Unk0c = 0;
    }
    void Release();
    int NewTexture();
    s8 DeleteTexture(unsigned int i);
    int FindTextureIndex(const EXTexture *pTex) const;
    EXTexture *Texture(unsigned int i) { return &m_ppPages[i >> 6][i & 0x3f]; }
    u32 m_Unk00;
    u32 m_Unk04;
    EXTexture **m_ppPages;  // 0x08
    u32 m_Unk0c;
    u32 m_Unk10;
};

EXBaseDisplay *EXBaseDisplay::m_pDisplay;
EXWnd *EXBaseDisplay::ms_pActiveWnd;
EXPoint EXBaseDisplay::ms_Offset;

void EXBaseDisplay::RegisterWatcherVars()
{
    EXWatcher *pWatcher = EXWatcher::GetWatcher();
    unsigned int h = pWatcher->AddItem("VSync", "Display", EXWATCH::DT_8, &m_Params.m_VSync, 1);
    pWatcher->EnableItemEdit(h, 1);
}

s8 EXBaseDisplay::Init(EXBaseDisplay *pDisplay)
{
    m_pDisplay = pDisplay;
    return 1;
}

void EXBaseDisplay::Shutdown()
{
    if (m_pDisplay) delete m_pDisplay;
}

void EXBaseDisplay::RemoveWnd(EXWnd *pWnd)
{
    m_Wnds.Remove(pWnd);
    if (m_pFocusWnd == pWnd) m_pFocusWnd = 0;
    if (m_pMouseWnd == pWnd) m_pMouseWnd = 0;
}

s8 EXBaseDisplay::IsWndVisible(EXWnd *pWnd) const
{
    for (EXDListItem *p = m_Wnds.Head(); p; p = p->m_pNext) {
        if (p == pWnd) return 1;
        if (static_cast<EXWnd *>(p)->IsChild(pWnd)) return 1;
    }
    return 0;
}

s8 EXBaseDisplay::CalcPrimRectWindow(EXDListItem *pItem)
{
    static_cast<EXWnd *>(pItem)->CalcPrimRects();
    return 1;
}

s8 EXBaseDisplay::FlushWindowMem(EXDListItem *pItem)
{
    static_cast<EXWnd *>(pItem)->FlushMem();
    return 1;
}

void EXBaseDisplay::FlushAllWindowMem()
{
    m_Wnds.Apply(FlushWindowMem, 0);
}

s8 EXBaseDisplay::DeleteWindowMem(EXDListItem *pItem)
{
    static_cast<EXWnd *>(pItem)->DeleteMem();
    return 1;
}

void EXBaseDisplay::DeleteAllWindowMem()
{
    m_Wnds.Apply(DeleteWindowMem, 0);
}

EXTexture *EXBaseDisplay::NewRuntimeTexture(unsigned short w, unsigned short h, unsigned char fmt, unsigned int flags)
{
    if (!m_pRuntimeTextures) return 0;
    int i = m_pRuntimeTextures->NewTexture();
    if (i < 0) return 0;
    EXTexture *pTex = m_pRuntimeTextures->Texture(i);
    if (!pTex) return 0;
    if (!pTex->m_Impl.Create(w, h, fmt, flags)) {
        m_pRuntimeTextures->DeleteTexture(i);
        return 0;
    }
    m_NumRuntimeTextures++;
    return pTex;
}

void EXBaseDisplay::DelRuntimeTexture(EXTexture *pTex)
{
    int i = m_pRuntimeTextures->FindTextureIndex(pTex);
    pTex->m_Impl.Release();
    m_pRuntimeTextures->DeleteTexture(i);
    m_NumRuntimeTextures--;
}

// NEAR: the identity loop's pointer bias (retail +0x18, ours +8); the rest matches.
EXBaseDisplay::EXBaseDisplay()
{
    memset(&m_Params, 0, sizeof(m_Params));
    m_Params.m_VSync = 1;
    m_pFocusWnd = 0;
    m_pMouseWnd = 0;
    m_CaptureDir = "";
    m_DoCapture = 0;
    m_CaptureContinuous = 0;
    m_Unk33 = 0;
    m_Unk34 = 0;
    EXMaths::mat_44 *pMtx = ms_mtxArray;
    for (int i = 17; i != 0; i--) {
        pMtx->set_identity();
        pMtx++;
    }
    ms_UpdObjectToWorld = 0;
    ms_UpdWorldToView = 0;
    ms_UpdViewToClip = 0;
    ms_UpdViewToCull = 0;
    ms_UpdClipToScreen = 0;
    ms_UpdViewToScreen = 0;
    ms_pActiveWnd = 0;
    m_SafeFrameEnabled = 0;
    m_Unk36 = 0;
    m_Unk32 = 0;
    ms_TintRed = 1.0f;
    ms_TintGreen = 1.0f;
    ms_TintBlue = 1.0f;
    m_pRuntimeTextures = new EXTextureArray;
    EXWatcher *pWatcher = EXWatcher::GetWatcher();
    pWatcher->AddSection("Display", 0u, "-Engine-", 1);
    unsigned int h = pWatcher->AddItem("SafeFrame", "Display", EXWATCH::DT_8, &m_SafeFrameEnabled, 1);
    pWatcher->EnableItemEdit(h, 1);
    h = pWatcher->AddSection("Screen Capture", 0u, "-Engine-", 1);
    h = pWatcher->AddItem("Continuous", h, EXWATCH::DT_8, &m_CaptureContinuous);
    pWatcher->EnableItemEdit(h, 0);
    m_NumRuntimeTextures = 0;
}

EXBaseDisplay::~EXBaseDisplay()
{
    for (EXDListItem *p = m_Wnds.Tail(); p; p = p->m_pPrev) {
        EXWnd *pWnd = static_cast<EXWnd *>(p);
        if (pWnd->m_pMenu) pWnd->m_pMenu->Close();
    }
    for (EXDListItem *p = m_Wnds.Tail(); p; p = m_Wnds.Tail()) {
        m_Wnds.RemoveTail();
        delete static_cast<EXWnd *>(p);
    }
    EXTextureArray *pTextures = m_pRuntimeTextures;
    if (pTextures) {
        pTextures->Release();
        delete pTextures;
    }
}

// NEAR: block layout differs (retail tail-duplicates CalcPrimRects per branch and walks items without null checks).
EXWnd *EXBaseDisplay::AddWnd(EXWnd *pWnd)
{
    EXDListItem *pTop = m_Wnds.Tail();
    EXDListItem *p = pTop;
    if ((int)pWnd->m_Flags.m_flags < 0) {
        m_Wnds.AddTail(pWnd);
    } else if (pWnd->m_Flags & 0x1000) {
        for (; p; p = p->m_pPrev) {
            if ((int)static_cast<EXWnd *>(p)->m_Flags.m_flags >= 0) break;
        }
        if (p == pTop) m_Wnds.AddTail(pWnd);
        else p->Add(pWnd);
    } else if (pWnd->m_Flags & 0x2000) {
        m_Wnds.AddHead(pWnd);
    } else {
        for (; p; p = p->m_pPrev) {
            if (!(static_cast<EXWnd *>(p)->m_Flags & 0x80001000)) break;
        }
        if (!p) m_Wnds.AddHead(pWnd);
        else if (p == pTop) m_Wnds.AddTail(pWnd);
        else p->Add(pWnd);
    }
    pWnd->CalcPrimRects();
    return pWnd;
}

void EXBaseDisplay::DelWnd(EXWnd *pWnd)
{
    pWnd->m_Flags.m_flags |= 0x18000;
    if (m_pFocusWnd == pWnd) m_pFocusWnd = 0;
    if (m_pMouseWnd == pWnd) m_pMouseWnd = 0;
}

void EXBaseDisplay::WndToTop(EXWnd *pWnd)
{
    if (pWnd->m_Flags & 0x2000) return;
    if (pWnd->m_pParent) return;
    EXDListItem *p = m_Wnds.Tail();
    if (p && p == static_cast<EXDListItem *>(pWnd)) return;
    m_Wnds.Remove(pWnd);
    unsigned int flags = pWnd->m_Flags;
    if (flags & 0x80000000) {
        m_Wnds.AddTail(pWnd);
    } else if (flags & 0x1000) {
        for (; p; p = p->m_pPrev) {
            if (!(static_cast<EXWnd *>(p)->m_Flags & 0x80000000)) break;
        }
        if (p == m_Wnds.Tail()) m_Wnds.AddTail(pWnd);
        else p->Add(pWnd);
    } else {
        for (; p; p = p->m_pPrev) {
            if (!(static_cast<EXWnd *>(p)->m_Flags & 0x80001000)) break;
        }
        if (!p) m_Wnds.AddHead(pWnd);
        else if (p == m_Wnds.Tail()) m_Wnds.AddTail(pWnd);
        else p->Add(pWnd);
    }
}

void EXBaseDisplay::WndToBottom(EXWnd *pWnd)
{
    if (pWnd->m_Flags & 0x1000) return;
    if (pWnd->m_pParent) return;
    EXDListItem *p = m_Wnds.Head();
    if (p && p == static_cast<EXDListItem *>(pWnd)) return;
    m_Wnds.Remove(pWnd);
    for (; p; p = p->m_pNext) {
        if (!(static_cast<EXWnd *>(p)->m_Flags & 0x2000)) break;
    }
    if (!p) m_Wnds.AddTail(pWnd);
    else if (p == m_Wnds.Head()) m_Wnds.AddHead(pWnd);
    else p->Insert(pWnd);
}

s8 EXBaseDisplay::RedrawWindow(EXDListItem *pItem)
{
    if (static_cast<EXWnd *>(pItem)->m_Flags & 0x8000) return 1;
    static_cast<EXWnd *>(pItem)->DrawWindowShadow();
    return static_cast<EXWnd *>(pItem)->DrawWindow();
}

void EXBaseDisplay::RedrawWindows()
{
    m_Wnds.Apply(CalcPrimRectWindow, 0);
    m_Wnds.Apply(RedrawWindow, 0);
}

s8 EXBaseDisplay::UpdateWindow(EXDListItem *pItem)
{
    if (static_cast<EXWnd *>(pItem)->m_Flags & 0x8000) return 1;
    return static_cast<EXWnd *>(pItem)->UpdateWindow();
}

EXWnd *EXBaseDisplay::WindowUnder(const EXPoint &pt, EXWnd *pIgnore) const
{
    EXDListItem *pItem = m_Wnds.Tail();
    EXPoint p = DispToPrim(pt);
    s8 found = pIgnore == 0;
    for (; pItem; pItem = pItem->m_pPrev) {
        EXWnd *pWnd = static_cast<EXWnd *>(pItem);
        if (pWnd->m_Flags & 0x8000) continue;
        if (found) {
            EXWnd *pHit = pWnd->ContainsPrimPoint(p);
            if (pHit) return pHit;
        } else if (pWnd == pIgnore || pWnd->IsChild(pIgnore)) {
            found = 1;
        }
    }
    return 0;
}

// NEAR: retail does not fold the clamps after zeroing the frame (reads back memory), u8 mode in ebx.
void EXBaseDisplay::SetupSafeFrame()
{
    m_SafeFrame.x = 0;
    m_SafeFrame.y = 0;
    m_SafeFrame.w = 0;
    m_SafeFrame.h = 0;
    u8 mode = m_Unk36;
    if (!mode) mode = 2;
    if (mode & 1) {
        if (m_SafeFrame.x < 16) m_SafeFrame.x = 16;
        if (m_SafeFrame.y < 16) m_SafeFrame.y = 16;
        if (m_SafeFrame.w < 16) m_SafeFrame.w = 16;
        if (m_SafeFrame.h < 16) m_SafeFrame.h = 16;
    }
    if (mode & 2) {
        if (m_SafeFrame.x < 50) m_SafeFrame.x = 50;
        if (m_SafeFrame.y < 35) m_SafeFrame.y = 35;
        if (m_SafeFrame.w < 50) m_SafeFrame.w = 50;
        if (m_SafeFrame.h < 37) m_SafeFrame.h = 37;
    }
    if (mode & 4) {
        if (m_SafeFrame.x < 16) m_SafeFrame.x = 16;
        if (m_SafeFrame.y < 16) m_SafeFrame.y = 16;
        if (m_SafeFrame.w < 16) m_SafeFrame.w = 16;
        if (m_SafeFrame.h < 16) m_SafeFrame.h = 16;
    }
    m_SafeFrame.w = Width() - m_SafeFrame.w - m_SafeFrame.x;
    m_SafeFrame.h = Height() - m_SafeFrame.y - m_SafeFrame.h;
}

// NEAR: 19 bytes differ, retail keeps a null check before deleting the window.
void EXBaseDisplay::UpdateWindows()
{
    m_Wnds.Apply(UpdateWindow, 0);
    EXDListItem *p = m_Wnds.Tail();
    while (p) {
        EXWnd *pWnd = (EXWnd *)((char *)p - 4);
        EXDListItem *pPrev = p->m_pPrev;
        if (pWnd->m_Flags & 0x10000) {
            m_Wnds.Remove(p);
            if (pWnd) delete pWnd;
        }
        p = pPrev;
    }
}

// A world point in display pixels: through WorldToScreen, divided by w, less the display offset
// (0, 0 when w is 0).
EXVector2 EXBaseDisplay::WorldToDisp(const EXVector &p)
{
    EXMaths::vec_xyzw v = p;
    v.w = 1.0f;
    EXMaths::mat_44 m = WorldToScreen();
    EXVector s = m * v;
    if (s.w == 0.0f) return EXVector2(0.0f, 0.0f);
    s *= 1.0f / s.w;
    return EXVector2(s.x - (float)ms_Offset.x, s.y - (float)ms_Offset.y);
}
