#include "EXItemRender.h"
#include "EXItem.h"
#include "EXMemory.h"

EX_IMPLEMENT_DYNCREATE_ROOT(EXItemRender)

EXItemRender::EXItemRender() { m_08 = 0; }

EXBaseItem *EXItemRender::GetBaseOwner() const
{
    if (!m_pOwner) return 0;
    return m_pOwner->GetBaseOwner();
}
