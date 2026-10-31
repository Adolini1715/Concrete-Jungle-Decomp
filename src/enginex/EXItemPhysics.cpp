#include "EXItemPhysics.h"
#include "EXItem.h"
#include "EXItemAnimator.h"
#include "EXMemory.h"

// TODO stand-in: collision face data (collision area). Only the item reached through +0x34 is used.
struct EXCScoopHit {
    u8 pad[0x48];
    EXPlacementItem *m_pItem;   // +0x48
};
class EXCScoop_FaceData {
public:
    u8 pad[0x34];
    EXCScoopHit *m_34;          // +0x34
};

EX_IMPLEMENT_DYNCREATE_ROOT(EXItemPhysics)
// NEAR: EXItemPhysicsExt::CreateObject, 5 bytes differ: inlined ctor stores m_40 before the vptr
// (original stores the vptr first); the out-of-line ctor matches.
EX_IMPLEMENT_DYNCREATE(EXItemPhysicsExt, EXItemPhysics)

EXItemPhysics::EXItemPhysics()
{
    m_physicsFlags = 0;
    m_collisionFlags = 0;
    m_interObjectGroup = 0;
    m_pItem = 0;
}

s8 EXItemPhysics::Initialise() { return 1; }
s8 EXItemPhysics::DoUpdate() { return 1; }

void EXItemPhysics::DoMapCollisionCallBack(EXCScoop_FaceData *pData)
{
    EXPlacementItem *pItem = pData->m_34->m_pItem;
    while (pItem) {
        if (pItem->IsKindOf(EXRUNTIME_CLASS(EXItemAnimator))) {
            pItem = ((EXItemAnimator *)pItem)->GetOwnerItem();
            continue;
        }
        if (pItem->IsKindOf(EXRUNTIME_CLASS(EXItem))) {
            EXItemPhysics *pPhys = ((EXItem *)pItem)->ItemPhysics();
            if (pPhys) pPhys->DoCollision(this, pData);
        }
        return;
    }
}

EXItemPhysicsExt::EXItemPhysicsExt()
{
    m_40 = 0;
    m_10.SetZero();
    m_20.SetZero();
}
