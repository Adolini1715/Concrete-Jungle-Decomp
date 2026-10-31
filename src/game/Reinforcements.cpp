#include "XItemHandler_Reinforcements.h"
#include "XItem.h"
#include "Actor.h"
#include "EXMemory.h"
#include "FEText.h"

XItemList gReinforceList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Reinforcements)

XItemHandler_Reinforcements::~XItemHandler_Reinforcements() { gReinforceList.Remove((XItem *)m_pItem); }

signed char XItemHandler_Reinforcements::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gReinforceList.Add((XItem *)m_pItem);
    m_RepeatTimer = 0;
    m_bSpawning = 0;
    return 1;
}

void XItemHandler_Reinforcements::SpawnActors()
{
    if (m_RepeatTimer) return;
    if (m_bSpawning) return;
    m_bSpawning = 1;
    m_Spawned = 0;
    m_SpawnDelay = 0;
}

signed char XItemHandler_Reinforcements::DoUpdate()
{
    if (m_RepeatTimer) m_RepeatTimer--;
    if (m_SpawnDelay) m_SpawnDelay--;
    if (m_bSpawning && m_SpawnDelay <= 0) {
        m_SpawnDelay = 10;
        Actor::Create(m_Profile, m_pItem->Position(), 0, 0.0f, 0, 0x100, 0);
        if (++m_Spawned >= m_Count) {
            m_bSpawning = 0;
            if (m_RepeatDelay) {
                m_RepeatTimer = m_RepeatDelay;
                return 1;
            }
            m_pItem->ItemFlags() = 0x10;
        }
    }
    return 1;
}
