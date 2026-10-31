#include "Actor.h"
#include "ActorUtil.h"
#include "ActorTextureSwap.h"
#include "XItemHandler_Player.h"
#include "EXNew.h"
#include "FEText.h"


// The actor item's render flags (EXRenderInfo::m_renderFlags): 0x4 RGB mult, 0x8 TODO, 0x4000 heat render.
static inline ex::EXFlag<u16> &Flags(Actor *pActor) { return pActor->GetItem()->RenderInfo().RenderFlagsRef(); }

ActorTextureSwapArray ActorTextureSwap::m_Data;
int ActorTextureSwap::m_Count;

void ActorTextureSwap::ApplyAlternate(Actor *pActor)
{
    ActorTextureSwapItem *pItem = pActor->TextureSwapItem();
    EXTexSwapInfo *pSwap = pActor->TexSwapInfo();
    unsigned int file = pActor->GetSkin()->m_File;
    if (!pActor->IsLimbLost(ALL_HEAD) && pItem && pItem->Valid() && pItem->Head())
        pSwap->SetSwap(0x6000049, file, pItem->Head());
    if (pItem && pItem->Valid() && pItem->Body()) pSwap->SetSwap(0x6000078, file, pItem->Body());
    if (!pActor->IsLimbLost(ALL_BELT) && pItem && pItem->Valid() && pItem->Belt())
        pSwap->SetSwap(0x6000046, file, pItem->Belt());
    if (!pActor->IsLimbLost(ALL_LEFTARM) && pItem && pItem->Valid() && pItem->ArmLeft())
        pSwap->SetSwap(0x6000047, file, pItem->ArmLeft());
    if (!pActor->IsLimbLost(ALL_RIGHTARM) && pItem && pItem->Valid() && pItem->ArmRight())
        pSwap->SetSwap(0x6000048, file, pItem->ArmRight());
}

void ActorTextureSwap::ApplyLostLimbs(Actor *pActor)
{
    EXTexSwapInfo *pSwap = pActor->TexSwapInfo();
    ActorTextureSwapItem *pItem = pActor->TextureSwapItem();
    unsigned int file = pActor->GetSkin()->m_File;
    s8 lost = 0;
    if (pActor->IsLimbLost(ALL_HEAD)) {
        lost = 1;
        pSwap->SetSwap(0x6000049, 0x100004c, 0x6000045);
        if (pItem && pItem->Valid() && pItem->CutHead()) pSwap->SetSwap(0x60001a5, file, pItem->CutHead());
    }
    if (pActor->IsLimbLost(ALL_BELT)) {
        lost = 1;
        pSwap->SetSwap(0x6000046, 0x100004c, 0x6000045);
    }
    if (pActor->IsLimbLost(ALL_LEFTARM)) {
        lost = 1;
        pSwap->SetSwap(0x6000047, 0x100004c, 0x6000045);
        if (pItem && pItem->Valid() && pItem->CutArmLeft()) pSwap->SetSwap(0x60001a6, file, pItem->CutArmLeft());
    }
    if (pActor->IsLimbLost(ALL_RIGHTARM)) {
        pSwap->SetSwap(0x6000048, 0x100004c, 0x6000045);
        if (pItem && pItem->Valid() && pItem->CutArmRight()) pSwap->SetSwap(0x60001a7, file, pItem->CutArmRight());
    } else if (!lost) {
        pSwap->SetSwap(0x6000224, 0x100004c, 0x6000045);
        return;
    }
    pSwap->StopSwap(0x6000224);
}

// NEAR: retail pops the arguments of consecutive calls separately (add esp, 4 each); block layout otherwise matches.
void ActorTextureSwap::Apply(Actor *pActor)
{
    if (!XItemHandler_Player::Instance()) return;
    EXTexSwapInfo *pSwap = pActor->TexSwapInfo();
    pSwap->StopAllSwaps();
    unsigned int tex;
    unsigned int headTex;
    switch (XItemHandler_Player::Instance()->GetVisionMode()) {
    case 1:
        Flags(pActor).BitSet(0x4000);
        Flags(pActor).BitSet(8);
        Flags(pActor).BitClr(4);
        if (pActor->IsCloaked()) {
            tex = 0x60000c7;
            headTex = tex;
        } else {
            switch (pActor->GetType()) {
            case 2:
            case 8:
                if (pActor->GetProfile()->m_SubGroup == 0x4f000021) {
                    tex = 0x6000063;
                    headTex = 0x6000063;
                } else {
                    tex = 0x6000061;
                    headTex = 0x6000062;
                }
                break;
            case 3:
            case 7:
                tex = 0x6000061;
                headTex = 0x6000063;
                break;
            case 4:
                tex = 0x6000257;
                headTex = tex;
                break;
            default:
                tex = 0x6000062;
                headTex = 0x6000063;
                break;
            }
        }
        pSwap->SetSwap(0x6000078, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_LEFTARM)) pSwap->SetSwap(0x6000047, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_RIGHTARM)) pSwap->SetSwap(0x6000048, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_HEAD)) pSwap->SetSwap(0x6000049, 0x1000016, headTex);
        if (!pActor->IsLimbLost(ALL_BELT)) pSwap->SetSwap(0x6000046, 0x1000016, tex);
        if (pActor->IsCloaked()) {
            pSwap->SetSwap(0x6000075, 0x1000016, 0x60000c7);
            pSwap->SetSwap(0x6000077, 0x1000016, 0x60000c7);
            pSwap->SetSwap(0x6000076, 0x1000016, 0x60000c7);
            pSwap->SetSwap(0x6000074, 0x1000016, 0x60000c7);
            tex = 0x60000c7;
        } else {
            pSwap->SetSwap(0x6000075, 0x1000016, 0x6000072);
            pSwap->SetSwap(0x6000077, 0x1000016, 0x6000061);
            pSwap->SetSwap(0x6000076, 0x1000016, 0x6000062);
            pSwap->SetSwap(0x6000074, 0x1000016, 0x6000063);
            tex = 0x6000072;
        }
        break;
    case 2: {
        pActor->UpdateBrainScanColour(0);
        tex = 0x6000073;
        if (pActor->IsCloaked()) tex = 0x6000122;
        int type = pActor->GetType();
        if (type >= 3 && (type <= 4 || type == 7)) goto noSwap;
        Flags(pActor).BitSet(8);
        Flags(pActor).BitSet(4);
        Flags(pActor).BitSet(0x4000);
        pSwap->SetSwap(0x6000078, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_LEFTARM)) pSwap->SetSwap(0x6000047, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_RIGHTARM)) pSwap->SetSwap(0x6000048, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_HEAD)) pSwap->SetSwap(0x6000049, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_BELT)) pSwap->SetSwap(0x6000046, 0x1000016, tex);
        pSwap->SetSwap(0x6000075, 0x1000016, tex);
        pSwap->SetSwap(0x6000077, 0x1000016, tex);
        pSwap->SetSwap(0x6000076, 0x1000016, tex);
        pSwap->SetSwap(0x6000074, 0x1000016, tex);
        break;
    }
    case 3:
        tex = 0;
        switch (pActor->GetType()) {
        case 5:
            if (pActor->GetProfile()->m_SubGroup == 0x4f000017) goto noSwap;
        case 2:
        case 3:
        case 7:
        case 8:
            tex = 0x6000073;
            break;
        }
        if (pActor->IsCloaked() || pActor->CanChangeSkinEver()) tex = 0x60000bc;
        if (pActor->GetProfile()->m_Hash == 0x4700037c) tex = 0x6000073;
        else if (!tex) goto noSwap;
        Flags(pActor).BitSet(0x4000);
        pSwap->SetSwap(0x6000078, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_LEFTARM)) pSwap->SetSwap(0x6000047, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_RIGHTARM)) pSwap->SetSwap(0x6000048, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_HEAD)) pSwap->SetSwap(0x6000049, 0x1000016, tex);
        if (!pActor->IsLimbLost(ALL_BELT)) pSwap->SetSwap(0x6000046, 0x1000016, tex);
        pSwap->SetSwap(0x6000075, 0x1000016, tex);
        pSwap->SetSwap(0x6000077, 0x1000016, tex);
        pSwap->SetSwap(0x6000076, 0x1000016, tex);
        pSwap->SetSwap(0x6000074, 0x1000016, tex);
        pSwap->SetSwap(0x600008b, 0x1000016, tex);
        Flags(pActor).BitSet(8);
        Flags(pActor).BitSet(4);
        pActor->GetItem()->RenderInfo().m_rgbaMult.Set(0.0f, 1.0f, 1.0f, 1.0f);
        ApplyLostLimbs(pActor);
        return;
    noSwap:
        Flags(pActor).BitClr(0x4000);
        Flags(pActor).BitClr(8);
        Flags(pActor).BitClr(4);
        ApplyAlternate(pActor);
        ApplyLostLimbs(pActor);
        return;
    default:
        Flags(pActor).BitClr(0x4000);
        Flags(pActor).BitClr(8);
        ApplyAlternate(pActor);
        ApplyLostLimbs(pActor);
        return;
    }
    pSwap->SetSwap(0x600008b, 0x1000016, tex);
    ApplyLostLimbs(pActor);
}

void ActorTextureSwap::Initialise(Actor *pActor)
{
    unsigned int skin = pActor->GetSkin()->m_Hash;
    for (int i = 0; i < m_Count; i++) {
        if (m_Data[i].Skin() == skin) {
            pActor->SetTextureSwapItem(m_Data[i].Pick());
            break;
        }
    }
    Apply(pActor);
}

// NEAR: 20 bytes differ (inlined item constructor: lea edi scheduled before mov ecx, 9).
void ActorTextureSwap::PostLoadInit()
{
    int n = 0;
    ActorTextureSwapRow *pRow = (ActorTextureSwapRow *)AU_GetWorkSheet(0x100004c, 0x1400000e, 0, n);
    m_Count = 0;
    m_Data.SetSize(n, -1);
    for (int i = 0; i < n; i++, pRow++) {
        int j;
        for (j = 0; j < m_Count; j++) {
            if (m_Data[j].Skin() == pRow->m_Skin) {
                m_Data[j].Add(pRow);
                goto next;
            }
        }
        m_Data[m_Count].SetSkin(pRow->m_Skin);
        m_Data[m_Count].Add(pRow);
        m_Count++;
    next:;
    }
    m_Data.SetSize(m_Count, -1);
    ActorTextureSwapEntry tmp(0);
    for (int a = 0; a < m_Count - 1; a++) {
        for (int b = 0; b < m_Count - 1; b++) {
            if (m_Data[b].Skin() > m_Data[b + 1].Skin()) {
                tmp = m_Data[b];
                m_Data[b] = m_Data[b + 1];
                m_Data[b + 1] = tmp;
            }
        }
    }
    tmp.Invalidate();
}

void ActorTextureSwap::Shutdown()
{
    m_Data.Empty(1);
    m_Count = 0;
}

void ActorTextureSwap::PreLoadInit()
{
    m_Data.Empty(1);
    m_Count = 0;
}
