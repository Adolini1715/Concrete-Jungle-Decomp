#include "XItemHandler_VMO.h"
#include "XItem.h"
#include "ScanInfo.h"
#include "ActorOptions.h"
#include "EXMemory.h"
#include "FEText.h"

XItemList gVMOList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_VMO)

XItemHandler_VMO::~XItemHandler_VMO() { gVMOList.Remove((XItem *)m_pItem); }

s8 XItemHandler_VMO::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gVMOList.Add((XItem *)m_pItem);
    return 1;
}

signed char XItemHandler_VMO::ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo) { return 1; }

s8 XItemHandler_VMO::DoUpdate()
{
    if (m_DeleteFlagChannel && FlagChannels[m_DeleteFlagChannel]) m_pItem->ItemFlags() = 0x10;
    return 1;
}

signed char XItemHandler_VMO::GetScanInfo(int type, ScanInfo *pInfo)
{
    switch (type) {
    case 2:
        pInfo->m_a0 = m_ScanText;
        pInfo->m_Text[0][0] = m_ScanText2;
        break;
    default:
        pInfo->m_a0 = 0;
        break;
    }
    pInfo->m_a4 = 1.0f;
    s8 r = pInfo->m_a0 > 0;
    pInfo->m_ac = r;
    pInfo->m_pCallback = ScanCallback;
    return r;
}

void Update_VMO_Generic(unsigned char mode, unsigned int flags, XItem *pItem, EXTexSwapInfo &tex)
{
    switch (mode) {
    case 1:
        if (flags & 1) {
            pItem->RenderInfo().RenderFlagsRef().BitSet(2);
            pItem->RenderInfo().RenderFlagsRef().BitClr(0xc);
            pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 0.75f);
            pItem->RenderInfo().RenderFlagsRef().BitClr(0x4000);
            tex.StopAllSwaps();
        } else if (flags & 2) {
            pItem->RenderInfo().RenderFlagsRef().BitClr(6);
            pItem->RenderInfo().RenderFlagsRef().BitSet(8);
            pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 1.0f);
            pItem->RenderInfo().RenderFlagsRef().BitSet(0x4000);
            tex.SetSwap(0x6000075, 0x1000016, 0x6000072);
            tex.SetSwap(0x6000077, 0x1000016, 0x6000061);
            tex.SetSwap(0x6000076, 0x1000016, 0x6000062);
            tex.SetSwap(0x6000074, 0x1000016, 0x6000063);
            tex.SetSwap(0x60001ec, 0x100004c, 0x6000045);
            tex.SetSwap(0x60000b8, 0x1000016, 0x6000061);
        } else {
            pItem->RenderInfo().RenderFlagsRef().BitClr(0xe);
            pItem->RenderInfo().RenderFlagsRef().BitClr(0x4000);
            tex.StopAllSwaps();
        }
        if (flags & 0x28) pItem->ItemFlags().BitClr(1);
        if (flags & 0x10) pItem->SetHasRender();
        break;
    case 3:
        if (flags & 0x30) pItem->ItemFlags().BitClr(1);
        if (flags & 0xc) {
            pItem->RenderInfo().RenderFlagsRef().BitClr(2);
            pItem->RenderInfo().RenderFlagsRef().BitSet(0xc);
            pItem->RenderInfo().SetRGBAMult(0.0f, 3.0f, 3.0f, 1.0f);
            pItem->SetHasRender();
            tex.StopAllSwaps();
            pItem->RenderInfo().RenderFlagsRef().BitClr(0x4000);
        } else {
            pItem->RenderInfo().RenderFlagsRef().BitClr(0xe);
            pItem->RenderInfo().RenderFlagsRef().BitClr(0x4000);
            tex.StopAllSwaps();
        }
        break;
    default:
        pItem->RenderInfo().RenderFlagsRef().BitClr(0xe);
        pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 1.0f);
        pItem->RenderInfo().RenderFlagsRef().BitClr(0x4000);
        tex.StopAllSwaps();
        if (flags & 0x18) pItem->ItemFlags().BitClr(1);
        if (flags & 0x20) pItem->SetHasRender();
        break;
    }
}

// TODO: Update_VMO (1632 bytes: walks every item and dispatches on its handler class) not written yet.
