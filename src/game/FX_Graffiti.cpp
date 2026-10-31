#include "XItemHandler_Graffiti.h"
#include "XItem.h"
#include "EXMemory.h"
#include "FEText.h"

XItemList gGraffitiList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Graffiti)

XItemHandler_Graffiti::~XItemHandler_Graffiti() { gGraffitiList.Remove((XItem *)m_pItem); }

s8 XItemHandler_Graffiti::DoUpdate() { return 1; }

// NEAR: as Recharge_GetNearest (copy of the difference vector scheduled differently).
XItem *Graffiti_GetNearest(EXVector &pos)
{
    float best = 999999.9f;
    XItem *p = gGraffitiList.GetFirst();
    XItem *pBest = 0;
    for (; p; p = gGraffitiList.GetNext(p)) {
        EXVector d = p->Position() - pos;
        float dist = d.LengthXYZSquared();
        if (best > dist) {
            best = dist;
            pBest = p;
        }
    }
    return pBest;
}

s8 XItemHandler_Graffiti::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gGraffitiList.Add((XItem *)m_pItem);
    m_pItem->RenderInfo().SetTexSwapInfo(&m_TexSwapInfo);
    m_pItem->RenderInfo().RenderFlagsRef().BitSet(2);
    m_pItem->RenderInfo().SetRGBAMult(1.0f, 1.0f, 1.0f, 0.0f);
    return 1;
}
