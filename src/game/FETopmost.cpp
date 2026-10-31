#include "FEText.h"
#include "EXMemory.h"
#include "FETopmost.h"
#include "FELayout.h"
#include "FEPause.h"
#include "FEObjectScript.h"
#include "XApp.h"
#include "EXBaseApp.h"

#include "GeoSectionManager.h"
#include "FELevels.h"
#include "ActorUtil.h"
#include "EXRand.h"

FELoading *FELoading::m_pInstance;
FEDialog *FEDialog::m_pInstance;
FECutscene *FECutscene::m_pInstance;

FELoading::FELoading()
{
    m_Texture = 0;
    m_Text = 0;
    m_HintCount = 0;
}

FELoading *FELoading::Instance(signed char create)
{
    if (!m_pInstance && create) {
        m_pInstance = new FELoading;
        FEWnd::FixupZOrder();
    }
    return m_pInstance;
}

FELoading::~FELoading()
{
    m_pInstance = 0;
    if (m_Texture) GeoSectionManager::Del(0x100004e, m_Texture);
}

FEDialog::FEDialog() { SetObject(new FELayoutLayered(1, 0, 0, 0, 0, (EXW::TextAlign)4)); }

FEDialog *FEDialog::Instance(signed char create)
{
    if (!m_pInstance && create) {
        m_pInstance = new FEDialog;
        FEWnd::FixupZOrder();
    }
    return m_pInstance;
}

FEDialog::~FEDialog() { m_pInstance = 0; }

FECutscene *FECutscene::Instance(signed char create)
{
    if (!m_pInstance && create) {
        m_pInstance = new FECutscene;
        FEWnd::FixupZOrder();
    }
    return m_pInstance;
}

FEMathmos FEMimicry::m_WhiteMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));
FEMathmos FECutscene::m_WhiteMathmos(ex::RGBA(0x7f, 0x7f, 0x7f, 0x80));

void FECutscene::SetSubtitle(unsigned int hash, unsigned int text, signed char a, signed char b, float time)
{
    m_SubtitleHash = hash;
    m_SubtitleA = a;
    m_SubtitleB = b;
    m_SubtitleTime = time;
    if (text > 0x51000000 && text < 0x510009b9) {
        FEObjectives *pObjectives = FEObjectives::Instance(1);
        pObjectives->Add(text, Text_Match(text), 8);
    }
}

void FECutscene::CreditsStart(float delay)
{
    m_CreditsTimer = 0.0f;
    m_CreditsLine = 0;
    m_CreditsDelay = delay + 2.0f;
    theApp.m_bPauseEnabled = 0;
}

void FECutscene::CreditsStop()
{
    m_CreditsLine = -1;
    theApp.m_bPauseEnabled = 1;
}

// NEAR: retail does the m_State |= 0x10 as load/or/store interleaved with the next call's argument setup.
void FECutscene::HUDScriptOn(unsigned int hash, float scale, EXVector pos)
{
    if (!m_pObject->Script(hash)) {
        m_pObject->ScriptAdd(hash, (FEObjType::Flags)4, m_HudFile, hash, (EXW::TextAlign)4);
        m_pObject->Script(hash)->m_State |= 0x10;
        m_pObject->Script(hash)->SetScale(scale);
        m_pObject->Script(hash)->SetRemoteMathmos(&m_WhiteMathmos);
        m_pObject->Script(hash)->TweakPosition(pos);
        m_pObject->Script(hash)->Play(0, 0);
    }
}

void FECutscene::HUDScriptOff(unsigned int hash)
{
    if (m_pObject->Script(hash)) m_pObject->ScriptDel(hash);
}

void FEMimicry::Create(FEObject *p, int n, unsigned int a, unsigned int b)
{
    if (p) {
        if (!(p->m_Flags & 0x40000000)) {
            p->ScriptAdd(0x5f5f5f67625f6d76, (FEObjType::Flags)4, 0x1000025, a, (EXW::TextAlign)2);
            p->Script(0x5f5f5f67625f6d76)->m_AttachedTo = 0;
            { FEObjectScript *q = p->Script(0x5f5f5f67625f6d76); unsigned int st = q->m_State; q->m_State = st | 0x2000000; }
            p->Script(0x5f5f5f67625f6d76)->Play(0, 0x80000);
            p->ScriptAdd(0x5f5f5f67665f6d76, (FEObjType::Flags)4, 0x1000025, b, (EXW::TextAlign)2);
            p->Script(0x5f5f5f67665f6d76)->m_AttachedTo = 0;
            { FEObjectScript *q = p->Script(0x5f5f5f67665f6d76); unsigned int st = q->m_State; q->m_State = st | 0x2000000; }
            p->Script(0x5f5f5f67665f6d76)->SetRemoteMathmos(&m_WhiteMathmos);
            p->Script(0x5f5f5f67665f6d76)->Play(0, 0);
            p->m_Flags |= 0x40000000;
        }
        p->m_Data = n;
        p->m_Flags |= 0x40;
    }
}

void FEMimicry::Delete(FEObject *p)
{
    if (p) {
        p->StateSub(0x40000000);
        p->ScriptDel(0x5f5f5f6c745f6d76);
        p->ScriptDel(0x5f5f5f67625f6d76);
        p->ScriptDel(0x5f5f5f67665f6d76);
    }
}

signed char FEMimicry::Update(FEObject *p, signed char paused)
{
    if (!p) return 0;
    if (!paused) {
        int t = (int)EXBaseApp::GameFrameRateMul();
        t = (int)p->m_Data - t;
        p->m_Data = t;
        if (t <= 0) {
            Delete(p);
            return 0;
        }
    }
    return 1;
}

#include "GameData.h"
#include "EXBaseDisplay.h"

extern unsigned int gCurrentMap_GeoHash;

// Credits lines: text hash and its alignment.
static const struct {
    unsigned int m_Text;
    int m_Align;
} s_CreditsLines[24] = {
    {0x51000869, 2}, {0x5100086a, 2}, {0x5100086b, 2}, {0x5100086c, 2}, {0x5100086d, 2}, {0x5100086e, 2},
    {0x5100087a, 2}, {0x5100099d, 2}, {0x5100099e, 4}, {0x5100086f, 2}, {0x51000870, 2}, {0x51000871, 2},
    {0x51000872, 2}, {0x51000873, 2}, {0x51000874, 2}, {0x51000875, 2}, {0x51000876, 2}, {0x51000877, 2},
    {0x51000878, 2}, {0x51000879, 2}, {0x510008d5, 2}, {0x510008d6, 2}, {0x510008d7, 2}, {0x510008d8, 2},
};

// Draws the current credits line (fading in/out) and steps through the list; sets the pause flag at the end.
void FECutscene::CreditsUpdate()
{
    if (m_CreditsLine < 0) return;
    ex::RGBA col(0x7f, 0x7f, 0x7f, 0x80);
    if (m_CreditsTimer < 1.0f) col.m_A = (u8)(int)(m_CreditsTimer * 128.0f);
    else if (m_CreditsTimer > m_CreditsDelay - 1.0f) col.m_A = (u8)(int)((m_CreditsDelay - m_CreditsTimer) * 128.0f);
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    int x = m_LetterTop.y, w = m_LetterTop.h, right = m_LetterBottom.y;
    EXRect rc(pDisp->m_SafeFrame.x + 0x10, pDisp->m_SafeFrame.w - 0x20, x + w, right - x - w);
    DrawText((FERenderPass::Flags)4, 0x100003a, 0x7000004, col, 0, rc, gText.Get(s_CreditsLines[m_CreditsLine].m_Text),
             (EXW::TextAlign)s_CreditsLines[m_CreditsLine].m_Align, 0);
    if (theApp.GameStateCurrent()->m_State == 9 && !GameControllerRemovedSys::Wnd()) {
        float dt = XApp::FrameLength();
        if (XApp::Frontend3DFile() == gCurrentMap_GeoHash) dt *= 0.5f;
        m_CreditsTimer += dt;
    }
    if (m_CreditsTimer >= m_CreditsDelay) {
        m_CreditsLine++;
        m_CreditsTimer = 0.0f;
    }
    if ((unsigned int)m_CreditsLine >= 0x18) {
        m_CreditsLine = -1;
        theApp.m_bPauseEnabled = 1;
    }
}

FECutscene::FECutscene()
{
    m_Unk400[0] = 0;
    m_SubtitleHash = 0;
    m_SubtitleA = 0;
    m_SubtitleTime = 0.0f;
    m_Unk410[0] = 0;
    m_Unk410[1] = 0;
    m_CreditsLine = -1;
    m_CreditsTimer = 0.0f;
    m_CreditsDelay = 0.0f;
    m_BarMathmos = FEMathmos(ex::RGBA(0, 0, 0, 0), ex::RGBA(0, 0, 0, 0x80), (float)EXBaseApp::NominalGameFrameRate() * 2.0f,
                             (FEWaveform::Flags)1, 1);
    SetObject(new FEObject((FEObjType::Flags)0, 0, 0, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0, (EXW::TextAlign)4))
        ->MakeSafeRect();
    m_pObject->m_Flags &= ~0x10;
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    int h = pDisp->Height();
    int w = pDisp->Width();
    int bar = h / 6;
    m_LetterTop = EXRect(0, 0, w, bar - 0x18);
    m_LetterBottom = EXRect(0, h - (bar + 0x1c), w, bar + 0x1c);
}

// Swaps the loading-screen texture section and picks a hint text: a random line from the general hints
// (and, for non-training levels, the level's own).
unsigned int FELoading::SetTextureGetText(unsigned int tex, unsigned int level)
{
    if (m_Texture == tex) return m_Text;
    if (m_Texture) GeoSectionManager::Del(0x100004e, m_Texture);
    m_Texture = tex;
    GeoSectionManager::SetAsyncMode(0);
    GeoSectionManager::Add(0x100004e, m_Texture);
    GeoSectionManager::DetachFromGameflow(0x100004e, 1);
    GeoSectionManager::SetAsyncMode(1);
    if (level == XApp::Frontend2DFile() || level == XApp::Frontend3DFile() || level == 0x1000187) {
        m_Text = 0x5100005a;
        return m_Text;
    }
    struct Row {
        unsigned int m_Key;
        unsigned int m_Text;
    };
    Row *rows = (Row *)AU_GetWorkSheet(0x100004e, 0x14000019, 0, m_HintCount);
    if (m_HintCount <= 0) {
        m_Text = 0x5100005a;
        return m_Text;
    }
    int catStart = -1, catEnd = -1, lvlStart = -1, lvlEnd = -1;
    unsigned int catKey = 0x100018f + (FELevelManager::Instance(1)->IsRitual(level) ? 1 : 0);
    for (int i = 0; i < m_HintCount; i++) {
        if (catStart < 0) {
            if (rows[i].m_Key == catKey) catStart = i;
        } else if (catEnd < 0) {
            if (rows[i].m_Key != catKey) catEnd = i - 1;
        }
        if (lvlStart < 0) {
            if (rows[i].m_Key == level) lvlStart = i;
        } else if (lvlEnd < 0) {
            if (rows[i].m_Key != level) lvlEnd = i - 1;
        }
    }
    if (FELevelManager::Instance(1)->LevelFromHash(level, 0)->m_Type == 2) catStart = -1;
    if (lvlStart >= 0 && lvlEnd < 0) lvlEnd = m_HintCount - 1;
    if (catStart >= 0 && catEnd < 0) catEnd = m_HintCount - 1;
    int catCount = catEnd - catStart;
    int lvlCount = lvlEnd - lvlStart;
    if (catCount <= 0 && lvlCount <= 0) {
        m_Text = 0x5100005a;
        return m_Text;
    }
    bool useCat;
    if (catCount > 0) {
        if (lvlCount <= 0) {
            useCat = true;
        } else {
            unsigned int total = lvlCount + catCount + 1;
            unsigned int r = total ? g_EXRandClass.Rand32() % total : 0;
            useCat = (int)r < catCount;
        }
    } else {
        useCat = false;
    }
    if (useCat) {
        unsigned int n = catCount + 1;
        unsigned int r = n ? g_EXRandClass.Rand32() % n : 0;
        m_Text = rows[r + catStart].m_Text;
    } else {
        unsigned int n = lvlCount + 1;
        unsigned int r = n ? g_EXRandClass.Rand32() % n : 0;
        m_Text = rows[r + lvlStart].m_Text;
    }
    return m_Text;
}

signed char FECutscene::Draw()
{
    if (!theApp.m_bCutscene) {
        m_DeleteNeeded = 1;
        return 1;
    }
    EXBaseDisplay *pDisp = EXBaseDisplay::GetDisplay();
    int w = pDisp->Width();
    int h = pDisp->Height();
    if (m_Unk400[0]) {
        // letterbox bars (each drawn twice to darken)
        DrawSprite((FERenderPass::Flags)4, 0x1000025, 0x6000054, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 3, m_LetterTop, EXVector2(0.0f, 0.0f),
                   EXVector2(1.0f, 1.0f));
        DrawSprite((FERenderPass::Flags)4, 0x1000025, 0x6000054, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 3, m_LetterTop, EXVector2(0.0f, 0.0f),
                   EXVector2(1.0f, 1.0f));
        DrawSprite((FERenderPass::Flags)4, 0x1000025, 0x6000054, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 3, m_LetterBottom, EXVector2(0.0f, 0.0f),
                   EXVector2(1.0f, 1.0f));
        DrawSprite((FERenderPass::Flags)4, 0x1000025, 0x6000054, ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 3, m_LetterBottom, EXVector2(0.0f, 0.0f),
                   EXVector2(1.0f, 1.0f));
        int bar = h / 6;
        m_LetterTop = EXRect(0, 0, w, bar - 0x18);
        m_LetterBottom = EXRect(0, h - (bar + 0x1c), w, bar + 0x1c);
    } else {
        m_LetterTop = EXRect(0, 0, w, 0);
        m_LetterBottom = EXRect(0, h, w, 0);
    }
    if (!FEPause::Instance(0)) {
        if (GameData::Instance(1)->m_Values.m_Unk1c[2] || m_SubtitleB) {
            if (m_SubtitleHash && theApp.GameStateCurrent()->m_State == 9) m_SubtitleTime -= XApp::GameFrameRateMul();
            if (m_SubtitleHash && m_SubtitleTime >= 0.0f) {
                EXRect r = m_pObject->Rect();
                EXRect rc(r.x, m_LetterBottom.y, r.w, r.h + (r.y - m_LetterBottom.y));
                DrawText((FERenderPass::Flags)4, 0x100003a, 0x7000004 + (m_SubtitleA ? 8 : 0), ex::RGBA(0x7f, 0x7f, 0x7f, 0x80), 0, rc,
                         Text_Get(m_SubtitleHash), (EXW::TextAlign)4, 0);
            } else {
                m_SubtitleHash = 0;
                m_SubtitleTime = 0.0f;
                m_SubtitleA = 0;
            }
        }
        if (m_Unk410[1] && !m_Unk410[0]) {
            FEMimicry::Create(m_pObject, 999, 0x4000744, 0x4000745);
            m_Unk410[0] = 1;
        }
    }
    if (m_BarMathmos.m_Colour.m_A) {
        DrawSprite((FERenderPass::Flags)4, 0x1000025, 0x6000054, m_BarMathmos.Update(), 0,
                   EXRect(0, 0, pDisp->Width(), pDisp->Height()), EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    }
    if (m_Unk410[0]) {
        if (m_Unk410[1]) m_pObject->SetMathmos((FEObjColour::Flags)2, FEMathmos::DefaultTint(), 1);
        else FEMimicry::Delete(m_pObject);
    }
    CreditsUpdate();
    return FEWnd::Draw();
}
