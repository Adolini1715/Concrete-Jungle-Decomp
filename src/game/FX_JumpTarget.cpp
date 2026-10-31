#include "XItemHandler_JumpTarget.h"
#include "XItem.h"
#include "EXMemory.h"
#include "FEText.h"

XItemList gJumpTargetList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_JumpTarget)

XItemHandler_JumpTarget::~XItemHandler_JumpTarget() { gJumpTargetList.Remove((XItem *)m_pItem); }

s8 XItemHandler_JumpTarget::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gJumpTargetList.Add((XItem *)m_pItem);
    return 1;
}

s8 XItemHandler_JumpTarget::DoUpdate() { return 1; }
