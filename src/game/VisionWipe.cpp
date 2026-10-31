#include "VisionWipe.h"
#include "FEObject.h"
#include "FEHud.h"
#include "XApp.h"
#include "EXBaseDisplay.h"
#include "EXRand.h"
#include "GameData.h"
#include "XItemHandler_Player.h"
#include "XItemHandler_Camera.h"
#include "EXMemory.h"
#include "FEText.h"

VisionWipe *VisionWipe::m_pInstance;
EXSoundTag VisionWipe::m_SoundTag;

VisionWipe::VisionWipe()
{
    m_bWiping = 0;
    m_bCommitted = 0;
    m_bFlip = 0;
    m_Timer = 0.0f;
    m_Mode = 0;
    SetObject(new FEObject((FEObjType::Flags)0, 0, 0, 0xfdffffff, FE_Blank, FE_Blank, (EXW::TextAlign)4, 0, 0, 0, 0,
                           (EXW::TextAlign)4))
        ->MakeSafeRect();
}

VisionWipe *VisionWipe::Instance(signed char create)
{
    if (!m_pInstance && create) {
        m_pInstance = new VisionWipe;
        FEWnd::FixupZOrder();
    }
    return m_pInstance;
}

// NEAR: retail spills the (Rand32() & 1) value to a stack slot (push ecx frame); not reproduced.
void VisionWipe::Start(unsigned char mode)
{
    if (m_bWiping) return;
    m_bWiping = 1;
    m_bCommitted = 0;
    const int &r = g_EXRandClass.Rand32() & 1;
    m_bFlip = r == 0;
    m_Timer = 0.0f;
    m_Mode = mode;
    unsigned int soundOn, soundOff, soundOther;
    int level = (short)GameData::Instance(1)->Values()->m_Unk58;
    if (level >= -1 && level <= 5) {
        soundOn = 0x1af00207;
        soundOff = 0x1af00206;
        soundOther = 0x1af00208;
    } else {
        soundOn = 0x1af0055c;
        soundOff = 0x1af0055b;
        soundOther = 0x1af0055d;
    }
    switch (mode) {
    case 1:
    case 2:
    case 3:
    case 100:
    case 101:
        if (!XItemHandler_Player::Instance()->m_Tint && mode < 4) m_SoundTag.StartSound(soundOn);
        else m_SoundTag.StartSound(soundOther);
        break;
    default:
        m_SoundTag.RemoveAll(1);
        m_SoundTag.StartSound(soundOff);
        break;
    }
}

void VisionWipe::KillWipe()
{
    if (m_bWiping) m_bWiping = 0;
    m_SoundTag.RemoveAll(1);
    m_SoundTag.KillSoundPlayingLooping();
    Delete();
}

// NEAR: retail builds the EXRect/EXVector2 arguments in place (same issue as XApp::LoadingCutsceneDraw).
signed char VisionWipe::Update()
{
    if (!m_bWiping) return 0;
    if (theApp.GameStateCurrent()->m_State == 9) m_Timer += EXBaseApp::GameFrameRateMul();
    EXBaseDisplay *pDisplay = EXBaseDisplay::GetDisplay();
    unsigned short w = pDisplay->Width();
    int h = pDisplay->Height();
    int x = (int)((float)w - (float)(w + 0x578) * (m_Timer * 0.05f));
    DrawSprite((FERenderPass::Flags)2, 0x1000025, 0x600007a, FEHud::TintColour(m_Mode), 0, EXRect(x, 0, 0x578, h),
               EXVector2(0.0f, 0.0f), EXVector2(1.0f, 1.0f));
    if (m_Timer >= 10.0f && !m_bCommitted) {
        if (m_Mode < 4) {
            XItemHandler_Player::Instance()->CommitVisionMode(m_Mode);
            FEHud::Instance(1)->CommitVisionMode(m_Mode);
        } else if (m_Mode == 100) {
            XGetCamera(0)->SetFirstPerson();
        } else {
            XGetCamera(0)->SetThirdPerson();
        }
        m_bCommitted = 1;
    }
    if (m_Timer >= 20.0f) {
        m_bWiping = 0;
        Delete();
    }
    return 1;
}
