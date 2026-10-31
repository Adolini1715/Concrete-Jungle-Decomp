#include "EXWnd.h"
#include "EXEffectsSystems.h"
#include "EXMenu.h"
#include "EXBaseDisplay.h"
#include "EXBaseRenderEnv.h"
#include "EXFileSys.h"
#include "EXMemory.h"


// TODO stand-in: instance manager (XBTexture area).
class EXInstanceManager {
public:
    EXInstanceManager();
    ~EXInstanceManager();
    void DeleteManager();
    void ResetInstanceManager();
    u8 m_Data[0x34];
};
// TODO stand-in: item environment (EXItemEnv.obj).
class EXItemEnv {
public:
    EXItemEnvView *RegisterEnvView(s8 a);
    void ReleaseEnvView(EXItemEnvView *pView);
    void UpdateEnvView(EXItemEnvView *pView, EXCamera &cam, const EXMatrix &m);
    void Render(EXItemEnvView *pView);
    static EXItemEnv *m_pTheItemEnv;
};

void EXBaseWnd::FlushMem()
{
    DeleteSprite2DData();
    DeleteDbgMem();
    DeleteRenderMem();
}

// NEAR: 18 bytes differ, load/store interleaving of the second rect copy.
void EXBaseWnd::SetRect(const EXRect &r)
{
    m_Rect.x = r.x;
    m_Rect.y = r.y;
    m_Rect.w = r.w;
    m_Rect.h = r.h;
    if (m_Sprite2DClipRect.x == 0 && m_Sprite2DClipRect.y == 0 && m_Sprite2DClipRect.w == 0 && m_Sprite2DClipRect.h == 0) {
        m_Sprite2DClipRect.x = m_Rect.x;
        m_Sprite2DClipRect.y = m_Rect.y;
        m_Sprite2DClipRect.w = m_Rect.w;
        m_Sprite2DClipRect.h = m_Rect.h;
    }
    CreateDbgMem();
}

void EXBaseWnd::DelChild(EXWnd *pWnd)
{
    m_Children.Remove(pWnd);
    EXBaseDisplay *pDisplay = EXBaseDisplay::GetDisplay();
    if (pDisplay->m_pFocusWnd == pWnd) pDisplay->m_pFocusWnd = 0;
    if (EXBaseDisplay::GetDisplay()->m_pMouseWnd == pWnd) EXBaseDisplay::GetDisplay()->m_pMouseWnd = 0;
    if (pWnd) delete pWnd;
}

void EXBaseWnd::HideChild(EXWnd *pWnd)
{
    m_Children.Remove(pWnd);
    EXBaseDisplay *pDisplay = EXBaseDisplay::GetDisplay();
    if (pDisplay->m_pFocusWnd == pWnd) pDisplay->m_pFocusWnd = 0;
    if (EXBaseDisplay::GetDisplay()->m_pMouseWnd == pWnd) EXBaseDisplay::GetDisplay()->m_pMouseWnd = 0;
}

s8 EXBaseWnd::IsChild(EXWnd *pWnd) const
{
    for (EXDListItem *p = m_Children.Head(); p; p = p->m_pNext) {
        if (p == pWnd) return 1;
        if (static_cast<EXWnd *>(p)->IsChild(pWnd)) return 1;
    }
    return 0;
}

// NEAR: rect copies use a base register (lea) where retail stores through this+offset.
void EXBaseWnd::CalcPrimRects()
{
    m_PrimRect = m_Rect;
    if (m_pParent) m_PrimRect.Offset(m_pParent->m_PrimRect.TopLeft());
    else m_PrimRect.Offset(EXBaseDisplay::Offset());
    if (m_pParent) {
        m_PrimClipRect = IntersectRect(m_PrimRect, m_pParent->m_PrimClipRect);
    } else {
        EXBaseDisplay *pDisplay = EXBaseDisplay::GetDisplay();
        EXRect disp(EXBaseDisplay::Offset().x, EXBaseDisplay::Offset().y, pDisplay->Width(), pDisplay->Height());
        m_PrimClipRect = IntersectRect(m_PrimRect, disp);
    }
    m_Children.Apply(CalcChildPrimRects, 0);
}

s8 EXBaseWnd::CalcChildPrimRects(EXDListItem *pItem)
{
    static_cast<EXWnd *>(pItem)->CalcPrimRects();
    return 1;
}

s8 EXBaseWnd::IsAtFront() const
{
    return this == EXBaseDisplay::GetDisplay()->TopWnd();
}

s8 EXBaseWnd::IsAtBack() const
{
    return this == EXBaseDisplay::GetDisplay()->BottomWnd();
}

s8 EXBaseWnd::HasFocus() const
{
    return this == EXBaseDisplay::GetDisplay()->m_pFocusWnd;
}

s8 EXBaseWnd::IsVisible()
{
    return EXBaseDisplay::GetDisplay()->IsWndVisible((EXWnd *)this);
}

EXParticleSysMgr *EXBaseWnd::ParticleSysMgr()
{
    if (m_pActiveEffects) return m_pActiveEffects->ParticleSysMgr();
    return 0;
}

void EXBaseWnd::DeleteParticleSysMem()
{
    if (m_pActiveEffects) m_pActiveEffects->DeleteParticleSysMem();
}

void EXBaseWnd::DeleteAllParticleSystems()
{
    if (m_pActiveEffects) m_pActiveEffects->DeleteAllParticleSystems();
}

void EXBaseWnd::StopParticleUpdateAndRender(s8 stop)
{
    if (m_pActiveEffects) m_pActiveEffects->StopParticleUpdateAndRender(stop);
}

void EXBaseWnd::PauseParticleSystems(s8 pause)
{
    if (m_pActiveEffects) m_pActiveEffects->PauseParticleSystems(pause);
}

EXSwooshSysMgr *EXBaseWnd::SwooshSysMgr()
{
    if (m_pActiveEffects) return m_pActiveEffects->SwooshSysMgr();
    return 0;
}

void EXBaseWnd::DeleteSwooshSysMem()
{
    if (m_pActiveEffects) m_pActiveEffects->DeleteSwooshSysMem();
}

void EXBaseWnd::DeleteAllSwooshSystems()
{
    if (m_pActiveEffects) m_pActiveEffects->DeleteAllSwooshSystems();
}

void EXBaseWnd::StopSwooshUpdateAndRender(s8 stop)
{
    if (m_pActiveEffects) m_pActiveEffects->StopSwooshUpdateAndRender(stop);
}

// Retail tests the active system but calls through the owned one.
void EXBaseWnd::PauseSwooshSystems(s8 pause)
{
    if (m_pActiveEffects) m_pEffectsSystems->PauseSwooshSystems(pause);
}

EXInstanceManager *EXBaseWnd::GetInstanceManager()
{
    EXInstanceManager *p = m_pInstanceManager;
    if (!p) {
        p = new EXInstanceManager;
        m_pInstanceManager = p;
    }
    return p;
}

s8 EXBaseWnd::RequestTextureCapture(EXTexture *pTex)
{
    m_CaptureRequested = 1;
    m_pCaptureTexture = pTex;
    return 1;
}

void EXBaseWnd::MovieOpen(char *, int, int, int, int, s8)
{
    m_MovieFinished = 1;
}

void EXBaseWnd::MovieUpdate() {}

void EXBaseWnd::MovieRender() {}

void EXBaseWnd::MovieClose() {}

s8 EXBaseWnd::MovieCheckFreeMem()
{
    return 1;
}

void EXBaseWnd::RegisterItemEnvView(s8 a)
{
    if (!m_pItemEnvView && EXItemEnv::m_pTheItemEnv) m_pItemEnvView = EXItemEnv::m_pTheItemEnv->RegisterEnvView(a);
}

void EXBaseWnd::ReleaseItemEnvView()
{
    if (m_pItemEnvView && EXItemEnv::m_pTheItemEnv) {
        EXItemEnv::m_pTheItemEnv->ReleaseEnvView(m_pItemEnvView);
        m_pItemEnvView = 0;
    }
}

s8 EXBaseWnd::UpdateItemEnvView(EXCamera &cam, const EXMatrix &m)
{
    if (m_pItemEnvView && EXItemEnv::m_pTheItemEnv) {
        EXItemEnv::m_pTheItemEnv->UpdateEnvView(m_pItemEnvView, cam, m);
        return 1;
    }
    return 0;
}

s8 EXBaseWnd::RenderItemEnvView(EXCamera &cam, const EXMatrix &m)
{
    if (m_pItemEnvView && EXItemEnv::m_pTheItemEnv) {
        EXItemEnv::m_pTheItemEnv->UpdateEnvView(m_pItemEnvView, cam, m);
        EXItemEnv::m_pTheItemEnv->Render(m_pItemEnvView);
        return 1;
    }
    return 0;
}

void EXBaseWnd::DeleteMem()
{
    DeleteSprite2DData();
    DeleteDbgMem();
    DeleteAllParticleSystems();
    DeleteAllSwooshSystems();
    DeleteRenderMem();
    if (m_pInstanceManager) m_pInstanceManager->DeleteManager();
}

// NEAR: block order differs (retail duplicates the null-checked item conversion per branch).
EXWnd *EXBaseWnd::AddChild(EXWnd *pWnd)
{
    EXDListItem *pTail = m_Children.Tail();
    EXDListItem *p = pTail;
    for (; p; p = p->m_pPrev) {
        if (!(static_cast<EXWnd *>(p)->m_Flags & 0x1000)) break;
    }
    if (!p) {
        m_Children.AddHead(pWnd);
    } else if (p == pTail) {
        m_Children.AddTail(pWnd);
    } else {
        p->Add(pWnd);
    }
    pWnd->m_pParent = (EXWnd *)this;
    return pWnd;
}

s8 EXBaseWnd::UpdateChild(EXDListItem *pItem)
{
    if (static_cast<EXWnd *>(pItem)->m_Flags & 0x8000) return 1;
    return static_cast<EXWnd *>(pItem)->UpdateWindow();
}

void EXBaseWnd::DrawWindowShadow()
{
    if (m_Flags & 0x40000) {
        EXBaseDisplay::SetActiveWnd((EXWnd *)this);
        EXBaseRenderEnv::SetShadowRender(1);
        EXBaseRenderEnv::SetRenderState(3);
        BeginDrawShadow();
        DrawShadow();
        EndDrawShadow();
        EXBaseRenderEnv::SetShadowRender(0);
    }
}

s8 EXBaseWnd::DrawChild(EXDListItem *pItem)
{
    if (static_cast<EXWnd *>(pItem)->m_Flags & 0x8000) return 1;
    return static_cast<EXWnd *>(pItem)->DrawWindow();
}

// NEAR: loop setup scheduling (retail loads the 0x8000 mask before the empty-list test) and inlined ContainsPoint.
EXWnd *EXBaseWnd::ContainsPrimPoint(const EXPoint &p)
{
    EXWnd *pHit = 0;
    if (m_PrimRect.ContainsPoint(p)) pHit = (EXWnd *)this;
    for (EXDListItem *pItem = m_Children.Head(); pItem; pItem = pItem->m_pNext) {
        EXWnd *pWnd = static_cast<EXWnd *>(pItem);
        if (!(pWnd->m_Flags & 0x8000)) {
            EXWnd *pChild = pWnd->ContainsPrimPoint(p);
            if (pChild) pHit = pChild;
        }
    }
    return pHit;
}

EXWnd *EXBaseWnd::ContainsDispPoint(const EXPoint &p)
{
    return ContainsPrimPoint(EXBaseDisplay::DispToPrim(p));
}

void EXBaseWnd::DeleteInstanceManager()
{
    if (m_pInstanceManager) {
        delete m_pInstanceManager;
        m_pInstanceManager = 0;
    }
}

EXBaseWnd::~EXBaseWnd()
{
    if (m_pMenu) delete m_pMenu;
    DeleteMem();
    ReleaseItemEnvView();
    if (m_pEffectsSystems) {
        delete m_pEffectsSystems;
        m_pEffectsSystems = 0;
    }
    DeleteInstanceManager();
}

s8 EXBaseWnd::UpdateWindow()
{
    if (m_Flags & 0x10) {
        DbgClear();
        if (m_pMenu && m_pMenu->IsOpen() && m_pMenu->TopLevel()) DbgPrint("\n");
    }
    EXBaseDisplay::SetActiveWnd((EXWnd *)this);
    if (!m_Initialised) {
        m_Initialised = 1;
        if (!InitialUpdate()) return 0;
    }
    if (m_pInstanceManager) m_pInstanceManager->ResetInstanceManager();
    if (!Update()) return 0;
    if (m_pEffectsSystems) m_pEffectsSystems->Update();
    return m_Children.Apply(UpdateChild, 0);
}

s8 EXBaseWnd::DrawWindow()
{
    if (m_PrimClipRect.w == 0 && m_PrimClipRect.h == 0) return 1;
    ClearSprite2DData();
    m_pSprite2DTexture = 0;
    m_pFont = 0;
    m_pFontTexture = 0;
    m_FontColor = ex::RGBA(0x80808080);
    EXBaseDisplay::SetActiveWnd((EXWnd *)this);
    EXBaseRenderEnv::SetShadowRender(0);
    EXBaseRenderEnv::SetRenderState(3);
    BeginDraw();
    if (m_Flags & 0x200000) g_EXFileLoadActivity.Update((EXWnd *)this);
    s8 ok = Draw();
    EndDraw();
    if (!ok) return 0;
    s8 r = m_Children.Apply(DrawChild, 0);
    m_CaptureRequested = 0;
    return r;
}

// NEAR: store scheduling (retail keeps the member stores in source order; VC groups ours by value).
EXBaseWnd::EXBaseWnd()
{
    m_pParent = 0;
    m_Rect.Set(0, 0, 0, 0);
    m_Sprite2DClipRect.Set(0, 0, 0, 0);
    m_Unk28[0] = 0;
    m_Unk28[1] = 0;
    m_Unk30[0] = 12;
    m_Unk30[1] = 12;
    m_PrimRect.Set(0, 0, 0, 0);
    m_PrimClipRect.Set(0, 0, 0, 0);
    m_Flags.Set(0);
    m_Effects.Set(0);
    SetBkColor(ex::RGBA(0, 0, 0, 0));
    m_Unk7c = ex::RGBA(0x50, 0x50, 0x50, 0xff);
    m_pDbgText = 0;
    m_DbgAttr = 0xff00;
    m_DbgWidth = 0;
    m_DbgHeight = 0;
    m_Unk80 = ex::RGBA(0x80, 0x80, 0x80, 0xff);
    m_DbgX = 0;
    m_DbgY = 0;
    m_Sprite2DData.SetGrowBy(16);
    m_Sprite2DBlocks.SetGrowBy(16);
    m_pMenu = 0;
    m_pSprite2DTexture = 0;
    m_pFont = 0;
    m_pFontTexture = 0;
    m_pInstanceManager = 0;
    m_pItemEnvView = 0;
    m_pCaptureTexture = 0;
    m_pEffectsSystems = new EXEffectsSystems;
    m_pActiveEffects = m_pEffectsSystems;
    m_GlowIntensity = 0.5f;
    m_MotionBlurFactor = 0.0f;
    m_Unk8c = 0;
    m_Initialised = 0;
    m_CaptureRequested = 0;
    m_Unk100 = 0;
    m_MovieFinished = 1;
    m_FocusBlurFactor = 2;
    m_FocusBlurDistance = 1.0f;
    m_Unk104.x = 0.0f;
    m_Unk104.y = 0.0f;
    m_Unk104.w = 1.0f;
    m_Unk104.h = 1.0f;
    m_MotionBlurDrawRect.x = 0.0f;
    m_MotionBlurDrawRect.y = 0.0f;
    m_MotionBlurDrawRect.w = 1.0f;
    m_MotionBlurDrawRect.h = 1.0f;
    m_ExpansionStart = 2.75f;
    m_ExpansionStep = 1.8f;
    m_pExpansionMask = 0;
    m_ExpansionPasses = 3;
    m_ExpansionAlpha = 1.0f;
    m_RefractionScale[0] = 0.333f;
    m_RefractionScale[1] = 0.333f;
    m_RefractionScale[2] = 0.333f;
    m_RefractionScale[3] = 0.333f;
    m_BloomColor = ex::RGBA(0x80, 0x80, 0x80, 0x80);
    m_RefractionPasses = 14;
    m_NoiseIntensity = 1.0f;
    m_NoiseSampleW = 1.0f;
    m_NoiseSampleH = 1.0f;
}
