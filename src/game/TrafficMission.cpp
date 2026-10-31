#include "FEText.h"
#include "Vehicle.h"
#include "Actor.h"
#include "XItemHandler_Objective.h"

extern unsigned short Mission_State;

static Vehicle *s_pVehicle;
static unsigned int s_hash;
static XItemHandler_Objective *s_pObjective;
static int s_actorId;

unsigned int TrafficMissionGetHash() { return s_hash; }
XItemHandler_Objective *TrafficMissionGetObjective() { return s_pObjective; }
int TrafficMissionGetActorId() { return s_actorId; }
Vehicle *TrafficMissionGetvehicle() { return s_pVehicle; }
signed char TrafficMissionIsActive() { return s_hash != 0; }

signed char TrafficMissionInit(unsigned int hash)
{
    s_pObjective = 0;
    s_actorId = 0;
    switch (hash) {
    case 0x45000006: s_hash = hash; break;
    default: s_hash = 0; break;
    }
    return s_hash != 0;
}

// NEAR: same code, but retail places the initRoute block inline after the state test; ours moves it
// to the end of the function (tried switch, else-if, inverted test, nested ifs).
void TrafficMissionUpdate()
{
    if (!s_hash) return;
    if (Mission_State == 0 || Mission_State == 2) {
        s_pVehicle->getMove().initRoute(0);
        s_hash = 0;
        return;
    }
    if (s_hash == 0x45000006 && s_pVehicle && s_pVehicle->m_pObjective->m_Hash == 0x46000002 && s_actorId &&
        Actor::GetPtrFromID(s_actorId) && !s_pVehicle->m_objectiveState)
        s_pVehicle->m_objectiveState = 1;
}

void TrafficObjectiveTriggered(XItemHandler_Objective *pObjective)
{
    if (s_hash) s_pObjective = pObjective;
}

void TrafficObjectiveActorSpawned(int id, XItemHandler_Objective *)
{
    if (s_hash) s_actorId = id;
}

void TrafficObjectiveVehicleSpawned(Vehicle *pVehicle, XItemHandler_Objective *)
{
    if (s_hash) s_pVehicle = pVehicle;
}
