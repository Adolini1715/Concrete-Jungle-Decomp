#include "XItemHandler_Hint.h"
#include "XItem.h"
#include "EXMemory.h"
#include "FEText.h"

XItemList gHintList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Hint)

XItemHandler_Hint::~XItemHandler_Hint() { gHintList.Remove((XItem *)m_pItem); }

s8 XItemHandler_Hint::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gHintList.Add((XItem *)m_pItem);
    return 1;
}

// NEAR: TODO (1408 bytes: radius/volume test, camera zoom, on-screen marker and objective text). Stub.
s8 XItemHandler_Hint::DoUpdate() { return 1; }
