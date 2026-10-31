#include "XItemHandler_Mimicry.h"
#include "XItem.h"
#include "XItemHandler_Camera.h"
#include "ActorOptions.h"
#include "EXMemory.h"
#include "FEText.h"

XItemList gMimicryList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Mimicry)

XItemHandler_Mimicry::~XItemHandler_Mimicry()
{
    gMimicryList.Remove((XItem *)m_pItem);
    if (XGetCamera(0)) {
        int type;
        if (XGetCamera(0)->GetLockOn(type) == m_pItem) XGetCamera(0)->ClearLockonX();
    }
}

s8 XItemHandler_Mimicry::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gMimicryList.Add((XItem *)m_pItem);
    return 1;
}

XItemList gVoiceOverList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_VoiceOver)

XItemHandler_VoiceOver::~XItemHandler_VoiceOver()
{
    gVoiceOverList.Remove((XItem *)m_pItem);
    m_SoundTag.RemoveAll(1);
}

s8 XItemHandler_VoiceOver::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gVoiceOverList.Add((XItem *)m_pItem);
    return 1;
}

s8 XItemHandler_Mimicry::DoUpdate()
{
    if (m_ShowFlagChannel) {
        m_pItem->ItemFlags().BitClr(1);
        if (FlagChannels[m_ShowFlagChannel]) {
            m_bActive = 1;
            m_pItem->SetHasRender();
            m_ShowFlagChannel = 0;
        }
    }
    if (m_HideFlagChannel && FlagChannels[m_HideFlagChannel]) {
        m_bActive = 0;
        m_pItem->ItemFlags() = 0x10;
    }
    return 1;
}

unsigned int XItemHandler_Mimicry::Play()
{
    m_bActive = 0;
    m_ShowFlagChannel = 0;
    m_HideFlagChannel = 0;
    FlagChannels[m_PlayFlagChannel] = 1;
    m_pItem->ItemFlags() = 0x10;
    return m_Sound;
}

s8 XItemHandler_VoiceOver::DoUpdate()
{
    if (m_StartFlagChannel) {
        if (FlagChannels[m_StartFlagChannel]) {
            m_StartFlagChannel = 0;
            m_SoundTag.StartSound(m_Sound);
        }
    } else if (!m_SoundTag.IsPlaying(m_Sound)) {
        m_pItem->ItemFlags() = 0x10;
    }
    if (m_StopFlagChannel && FlagChannels[m_StopFlagChannel]) m_pItem->ItemFlags() = 0x10;
    return 1;
}

void XItemHandler_VoiceOver::Stop()
{
    m_SoundTag.RemoveAll(1);
    m_pItem->ItemFlags() = 0x10;
}

void StopAllVoiceOvers()
{
    for (XItem *p = gVoiceOverList.GetFirst(); p; p = gVoiceOverList.GetNext(p)) {
        XItemHandler_VoiceOver *pVO = (XItemHandler_VoiceOver *)p->ItemHandler();
        if (!pVO->m_StartFlagChannel) pVO->Stop();
    }
}
