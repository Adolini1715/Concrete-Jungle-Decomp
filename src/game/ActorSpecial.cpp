#include "ActorSpecial.h"
#include "Actor.h"
#include "EXRand.h"
#include "FEText.h"
#include "ActorCarryable.h"
#include "ActorCombat.h"
#include "ActorLocation.h"
#include "ActorProfile.h"
#include "ActorUtil.h"
#include "WeaponTable.h"
#include "XApp.h"
#include "XNavLocation.h"

// TODO stand-ins: other areas.
void Honour_ScoreKill(Actor *pActor);
void Text_PrintObjective(unsigned int text, ex::RGBA col, int a, int b, signed char c, int d);

ActorSpecial SpecialList[10];
int SpecialCount;

signed char ActorSpecial_Add(int hash, int chance, int profile, StateFn state, int a, unsigned int text)
{
    if (SpecialCount >= 10) return 0;
    SpecialList[SpecialCount].m_ActorID = 0;
    SpecialList[SpecialCount].m_bEnabled = 0;
    SpecialList[SpecialCount].m_Chance = chance;
    SpecialList[SpecialCount].m_Hash = hash;
    SpecialList[SpecialCount].m_Profile = profile;
    SpecialList[SpecialCount].m_State = state;
    SpecialList[SpecialCount].m_Unk18 = a;
    SpecialList[SpecialCount].m_Text = text;
    SpecialCount++;
    return 1;
}

// NEAR: retail constructs the RGBA argument in its stack slot (mov ecx, esp; mov [ecx], -1).
signed char ActorSpecial_RegisterPlayerKill(Actor *pActor)
{
    Honour_ScoreKill(pActor);
    int id = pActor->m_RefID;
    for (int i = 0; i < 10; i++) {
        if (SpecialList[i].m_ActorID == id) {
            int tmp33 = ex::RGBA(0xffffffff);
Text_PrintObjective(SpecialList[i].m_Text, tmp33, 4, -1, 0, 0);
            SpecialList[i].m_bEnabled = 0;
            return 1;
        }
    }
    return 0;
}

signed char ActorSpecial_Unregister(Actor *pActor)
{
    int id = pActor->m_RefID;
    for (int i = 0; i < 10; i++) {
        if (SpecialList[i].m_ActorID == id) {
            SpecialList[i].m_ActorID = 0;
            return 1;
        }
    }
    return 0;
}

ActorSpecial *ActorSpecial_GetEntry(int hash)
{
    for (int i = 0; i < 10; i++)
        if (SpecialList[i].m_Hash == hash) return &SpecialList[i];
    return 0;
}

signed char ActorSpecial_IsSpecial(Actor *pActor)
{
    for (int i = 0; i < 10; i++)
        if (SpecialList[i].m_ActorID == pActor->m_RefID) return 1;
    return 0;
}

signed char Validate_KillerHooker(Actor *a, Actor *b) { return b->GetType() == 0; }

signed char Validate_YuppieMurderer(Actor *a, Actor *b)
{
    ActorProfile *pa = a->GetProfile();
    return b->GetProfile()->m_SubGroup == pa->m_SubGroup;
}

signed char Validate_SerialKiller(Actor *a, Actor *b) { return 1; }

signed char Validate_Zealot(Actor *a, Actor *b) { return b->GetProfile()->m_SubGroup == 0x4f000024; }

signed char STATE_Special_Killer(StateMachine &sm);
unsigned int GetMsgBitsForMsg(MsgType msg);

signed char STATE_Special_KillerHooker(StateMachine &sm)
{
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(MSG_ENTER), STATE_Special_Killer, 0x20000bf, (int)Validate_KillerHooker, 1, 0);
    return 1;
}

signed char STATE_Special_YuppieMurderer(StateMachine &sm)
{
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(MSG_ENTER), STATE_Special_Killer, 0x20000cf, (int)Validate_YuppieMurderer, 2, 0);
    return 1;
}

signed char STATE_Special_SerialKiller(StateMachine &sm)
{
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(MSG_ENTER), STATE_Special_Killer, 0x20000e2, (int)Validate_SerialKiller, 0x14, 0);
    return 1;
}

signed char STATE_Special_Zealot(StateMachine &sm)
{
    if (sm.m_Msg.m_Type != MSG_ENTER) return 0;
    sm.Goto(GetMsgBitsForMsg(MSG_ENTER), STATE_Special_Killer, 0x20000e2, (int)Validate_Zealot, 0x14, 0);
    return 1;
}

signed char STATE_Special_MasterCriminal(StateMachine &sm) { return 0; }

signed char STATE_Special_CorruptCop(StateMachine &sm) { return 0; }

signed char STATE_Special_Sniper(StateMachine &sm) { return 0; }

signed char STATE_Special_Arsonist(StateMachine &sm) { return 0; }

signed char STATE_Special_Stalker(StateMachine &sm) { return 0; }

signed char ActorSpecial_Enable(int hash, signed char b)
{
    ActorSpecial *e = ActorSpecial_GetEntry(hash);
    if (!e) return 0;
    e->m_bEnabled = b;
    return 1;
}

StateFn ActorSpecial_Register(Actor *pActor)
{
    StateFn st = AP_GetStartState(pActor->GetProfile());
    for (int i = 0; i < 10; i++) {
        ActorSpecial *e = &SpecialList[i];
        if (e->m_Profile == pActor->GetProfileHash() && e->m_bEnabled && !e->m_ActorID && EXRand(100) + 1 <= e->m_Chance) {
            SpecialList[i].m_ActorID = pActor->m_RefID;
            pActor->SetAliveRadius(200.0f);
            pActor->CreationFlagsAdd(0x800);
            return SpecialList[i].m_State;
        }
    }
    return st;
}

signed char STATE_Special_TrampBasher(StateMachine &sm);

// Registers the ten special characters: hash, spawn chance, host profile, start state and objective text.
signed char ActorSpecial_PostLoadInit(void)
{
    for (int i = 0; i < 10; i++) {
        SpecialList[i].m_Hash = 0;
        SpecialList[i].m_bEnabled = 0;
        SpecialList[i].m_ActorID = 0;
        SpecialList[i].m_Chance = 0;
        SpecialList[i].m_Profile = 0;
        SpecialList[i].m_State = 0;
        SpecialList[i].m_Unk18 = 0;
        SpecialList[i].m_Text = 0;
    }
    struct Init { int hash; int profile; StateFn state; unsigned int text; };
    static const Init init[10] = {
        { 8, 0x47000005, STATE_Special_KillerHooker, 0x5100003e },
        { 4, 0x47000030, STATE_Special_YuppieMurderer, 0x5100003b },
        { 0, 0x47000030, STATE_Special_SerialKiller, 0x51000037 },
        { 2, 0x47000030, STATE_Special_Zealot, 0x51000039 },
        { 7, 0x47000030, STATE_Special_TrampBasher, 0x5100003d },
        { 1, 0x47000030, STATE_Special_MasterCriminal, 0x51000038 },
        { 3, 0x47000030, STATE_Special_CorruptCop, 0x5100003a },
        { 6, 0x47000030, STATE_Special_Sniper, 0x5100003c },
        { 9, 0x47000030, STATE_Special_Arsonist, 0x5100003f },
        { 5, 0x47000030, STATE_Special_Stalker, 0x5100003f },
    };
    for (int i = 0; i < 10; i++) {
        SpecialList[i].m_Hash = init[i].hash;
        SpecialList[i].m_bEnabled = 0;
        SpecialList[i].m_ActorID = 0;
        SpecialList[i].m_Chance = 100;
        SpecialList[i].m_Profile = init[i].profile;
        SpecialList[i].m_State = init[i].state;
        SpecialList[i].m_Unk18 = 10;
        SpecialList[i].m_Text = init[i].text;
    }
    SpecialCount = 10;
    return 1;
}

// A killer who lurks in a dark alley, waits for a valid victim nearby, runs at them and strikes, then
// walks back to the alley. m_LocalData: 0..3 victim position, 4 phase, 5 victim id, 6 alley poi,
// 7 carried weapon id, 8 weapon entity, 9 victim validator, 10 strike distance, 11 wait timer.
static signed char Special_KillerUpdate(StateMachine &sm, int timerIdx, int poiType)
{
    sm.GetActorOwner().SetStatus(8);
    sm.GetActorOwner().SetWatch(7);
    int &phase = (int &)sm.m_LocalData[4];
    int &victimId = (int &)sm.m_LocalData[5];
    int &poi = (int &)sm.m_LocalData[6];
    int &carryId = (int &)sm.m_LocalData[7];
    float &timer = (float &)sm.m_LocalData[timerIdx];
    typedef signed char (__cdecl *ValidateFn)(Actor *, Actor *);

    if (sm.m_Msg.m_Type != MSG_EXT) return 0;

    Actor &me = sm.GetActorOwner();
    switch (phase) {
    case 0:
        phase = 1;
        poi = POI_GetNearestType(me.GetFeetPos(), poiType, 0, me.m_OrgLoc.NavZone(), me.CanJump(), 0);
        carryId = ActorCarryable::Create(0x100003c, sm.m_LocalData[8])->GetRefID();
        timer = 900.0f;
        me.SetCarry(carryId, 0);
        if (ActorCarryable::GetPtrFromID(carryId))
            ActorCarryable::GetPtrFromID(carryId)->SetHide(1);
        SM_GOSUB(STATE_GotoLocation, poi, 0x502, 0, 0);
        return 1;
    case 1:
        me.SetAnimType(0x4e000001, 0, 1);
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            for (Actor *pOther = Actor::GetFirst(me.GetFeetPos(), 20.0f, 0); pOther; pOther = pOther->GetNext(me.GetFeetPos(), 20.0f, 0)) {
                if (pOther->m_OrgLoc.NavZone() == me.m_OrgLoc.NavZone() && pOther->m_RefID != me.m_RefID
                    && ((ValidateFn)sm.m_LocalData[9])(&me, pOther)) {
                    MsgItem msg((MsgType)0x28, poi, 0, 0, 0);
                    victimId = pOther->SendMsg(msg, 0.0f);
                    if (victimId) {
                        phase = 2;
                        return 1;
                    }
                }
            }
        }
        return 1;
    case 2: {
        me.SetStatus(9);
        Actor *pVictim = Actor::GetPtrFromID(victimId);
        if (!pVictim) { phase = 1; return 1; }
        me.InvalidateGotoPosButNotTargetYRot();
        if (!me.IsFacingYRot(pVictim->GetHeadPos(), 5.0f))
            me.SetTargetYRot(pVictim->GetHeadPos(), 5.0f);
        else
            me.SetAnimType(0x4e000001, 0, 1);
        float dx = me.GetFeetPos().x - pVictim->GetFeetPos().x;
        float dz = me.GetFeetPos().z - pVictim->GetFeetPos().z;
        if (dx * dx + dz * dz >= 25.0f) return 1;
        phase = 3;
        if (ActorCarryable::GetPtrFromID(carryId))
            ActorCarryable::GetPtrFromID(carryId)->SetHide(0);
        return 1;
    }
    case 3: {
        me.SetStatus(9);
        Actor *pVictim = Actor::GetPtrFromID(victimId);
        if (!pVictim) { phase = 1; return 1; }
        me.SetAnimType(0x4e000003, 0, 1);
        me.GotoLoc(pVictim->m_OrgLoc);
        unsigned int strike = sm.m_LocalData[10];
        if (me.SqDistToTarget() < (float)(int)(strike * strike)) {
            phase = 4;
            MsgItem msg((MsgType)0x29, 0, 0, 0, 0);
            pVictim->SendMsg(msg, 0.0f);
            me.SetAnimType(GetNormalStrikeAnim(sm, WeaponProfile::GetWeaponDataFromEntity(sm.m_LocalData[8])), 0, 1);
            (EXVector &)sm.m_LocalData[0] = pVictim->GetFeetPos();
        }
        return 1;
    }
    case 4:
        me.SetStatus(9);
        if (me.HasAnimEnded()) {
            me.SetAnimType(GetNormalStrikeAnim(sm, WeaponProfile::GetWeaponDataFromEntity(sm.m_LocalData[8])), 0, 1);
            phase = 5;
        }
        return 1;
    case 5:
        me.SetStatus(9);
        if (me.HasAnimEnded())
            phase = 6;
        return 1;
    case 6:
        me.GotoPOI(poi);
        me.SetAnimType(0x4e000002, 0, 1);
        if (me.SqDistToTarget() >= 0.01f) return 1;
        phase = 7;
        if (ActorCarryable::GetPtrFromID(carryId))
            ActorCarryable::GetPtrFromID(carryId)->SetHide(1);
        return 1;
    case 7:
        me.InvalidateGotoPosButNotTargetYRot();
        if (!me.IsFacingYRot((EXVector &)sm.m_LocalData[0], 5.0f)) {
            me.SetTargetYRot((EXVector &)sm.m_LocalData[0], 5.0f);
            return 1;
        }
        timer = 1800.0f;
        phase = 1;
        return 1;
    }
    return 1;
}

signed char STATE_Special_Killer(StateMachine &sm)
{
    if (sm.m_Msg.m_Type == MSG_ENTER) {
        sm.GetActorOwner().SetStatus(8);
        sm.GetActorOwner().SetWatch(7);
        sm.m_LocalData[4] = 0;
        sm.m_LocalData[8] = sm.m_Msg.m_Data[0];
        sm.m_LocalData[9] = sm.m_Msg.m_Data[1];
        sm.m_LocalData[10] = sm.m_Msg.m_Data[2];
        return 1;
    }
    return Special_KillerUpdate(sm, 11, 0x420000a7);
}

// The same killer, but the lurking spot type is passed in (data[3]) and the timer lives one slot later.
signed char STATE_Special_KillerLure(StateMachine &sm)
{
    if (sm.m_Msg.m_Type == MSG_ENTER) {
        sm.GetActorOwner().SetStatus(8);
        sm.GetActorOwner().SetWatch(7);
        sm.m_LocalData[4] = 0;
        sm.m_LocalData[8] = sm.m_Msg.m_Data[0];
        sm.m_LocalData[9] = sm.m_Msg.m_Data[1];
        sm.m_LocalData[10] = sm.m_Msg.m_Data[2];
        sm.m_LocalData[11] = sm.m_Msg.m_Data[3];
        return 1;
    }
    return Special_KillerUpdate(sm, 12, sm.m_LocalData[11]);
}

// Picks the nearest suitable victim around center for the tramp basher: same nav zone, alive, not
// the basher's own kind and not the last victim. bTrampOnly restricts it to tramps. Writes the id to
// LocalData[6] when one is closer (by XZ distance) than the best so far.
static void TrampBasher_FindTarget(StateMachine &sm, const EXVector &center, float radius, bool bTrampOnly)
{
    Actor &me = sm.GetActorOwner();
    float best = 9999999.0f;
    for (Actor *p = Actor::GetFirst(center, radius, 0); p; p = p->GetNext(center, radius, 0)) {
        if (p->m_OrgLoc.NavZone() != me.m_OrgLoc.NavZone()) continue;
        if (bTrampOnly && AP_GetPtrFromIndex(p->m_ProfileIndex)->m_SubGroup != 0x4f000001) continue;
        if (p->m_RefID == me.m_RefID || p->m_RefID == (int)sm.m_LocalData[7]) continue;
        if (!(p->m_Health > 0.0f)) continue;
        if (AP_GetPtrFromIndex(p->m_ProfileIndex)->m_SubGroup == AP_GetPtrFromIndex(me.m_ProfileIndex)->m_SubGroup) continue;
        float dx = me.GetFeetPos().x - p->GetFeetPos().x;
        float dz = me.GetFeetPos().z - p->GetFeetPos().z;
        float d = dz * dz + dx * dx;
        if (d < best - 1.0f) {
            sm.m_LocalData[6] = p->m_RefID;
            best = d;
        }
    }
}

// A tramp who beats up nearby people with a baseball bat, then walks home. m_LocalData: 0..3 home
// position, 4 home poly, 5 phase (1 going home, 2 waiting, 3 chasing, 4 striking), 6 target id,
// 7 last target id, 8 carried bat id, 9 bat entity, 10 timer, 11 home facing.
signed char STATE_Special_TrampBasher(StateMachine &sm)
{
    Actor &me = sm.GetActorOwner();
    me.SetStatus(8);
    me.SetWatch(7);
    switch (sm.m_Msg.m_Type) {
    case MSG_SOMEONE_SEEN_PREDATOR:
        if (!me.IsMsgOnStack(MSG_SOMEONE_SEEN_PREDATOR)) AU_HandleSomeoneSeenPredator(sm);
        return 1;
    case MSG_LOUD_NOISE:
        if (!me.IsMsgOnStack(MSG_LOUD_NOISE)) AU_HandleLoudNoise(sm);
        return 1;
    case MSG_ATTACKED_BY_ACTOR:
        if (!me.IsMsgOnStack(MSG_ATTACKED_BY_ACTOR)) AU_HandleActorAttacks(sm, (StructMsg_AttackedByActor *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_ATTRACT:
        if (me.IsMsgOnStack(MSG_ATTRACT)) return 1;
        if (!me.Holographic()) AU_HandleAttractNoise(sm);
        return 1;
    case MSG_FIGHT_ME:
        if (!me.IsMsgOnStack(MSG_FIGHT_ME)) AU_HandleFightMe(sm);
        return 1;
    case MSG_DRIVE_BY:
        if (!me.IsMsgOnStack(MSG_DRIVE_BY)) AU_HandleDriveBy(sm);
        return 1;
    case MSG_ATTACKED_BY_PREDATOR:
        if (!me.IsMsgOnStack(MSG_ATTACKED_BY_PREDATOR)) AU_HandlePlayerAttacks(sm, (StructMsg_AttackedByPredator *)sm.m_Msg.m_Data[0]);
        return 1;
    case MSG_MUGGING:
        if (!me.IsMsgOnStack(MSG_MUGGING)) AU_HandleMuggingAttacks(sm);
        return 1;
    case MSG_ENTER: {
        sm.m_LocalData[5] = 2;
        (EXVector &)sm.m_LocalData[0] = me.GetFeetPos();
        sm.m_LocalData[4] = me.m_OrgLoc.GetPolyHandle();
        (float &)sm.m_LocalData[11] = me.GetRotation().y;
        sm.m_LocalData[9] = 0x20000e7;
        sm.m_LocalData[8] = ActorCarryable::Create(0x100003c, 0x20000e7)->GetRefID();
        me.SetCarry(sm.m_LocalData[8], 0);
        me.SetAnimType(0x4e000001, 0, 1);
        return 1;
    }
    case MSG_EXT:
        break;
    case MSG_EXIT:
        return 1;
    default:
        return 0;
    }

    float &timer = (float &)sm.m_LocalData[10];
    int &phase = (int &)sm.m_LocalData[5];
    int &targetId = (int &)sm.m_LocalData[6];
    switch (phase - 1) {
    case 0:
        me.GotoPosPoly((EXVector &)sm.m_LocalData[0], sm.m_LocalData[4]);
        if (me.SqDistToTarget() >= 0.01f) {
            me.SetAnimType(0x4e000002, 0, 1);
        } else {
            phase = 2;
            timer = 300.0f;
            me.SetAnimType(0x4e000001, 0, 1);
            me.SetRotY((float &)sm.m_LocalData[11]);
        }
        timer -= XApp::GameFrameRateMul();
        if (0.0f < timer) return 1;
        TrampBasher_FindTarget(sm, (EXVector &)sm.m_LocalData[0], 50.0f, true);
        if (targetId == 0) {
            timer = 60.0f;
            return 1;
        }
        phase = 3;
        break;
    case 1:
        targetId = 0;
        me.SetAnimType(0x4e000001, 0, 1);
        timer -= XApp::GameFrameRateMul();
        if (0.0f < timer) return 1;
        TrampBasher_FindTarget(sm, (EXVector &)sm.m_LocalData[0], 50.0f, true);
        if (targetId == 0) {
            timer = 60.0f;
            return 1;
        }
        phase = 3;
        break;
    case 2: {
        timer -= XApp::GameFrameRateMul();
        if (timer <= 0.0f) {
            TrampBasher_FindTarget(sm, me.GetFeetPos(), 5.0f, false);
            timer = 60.0f;
        }
        Actor *pTarget = Actor::GetPtrFromID(targetId);
        if (!pTarget) {
            phase = 1;
            return 1;
        }
        me.SetAnimType(0x4e000003, 0, 1);
        me.GotoLoc(pTarget->m_OrgLoc);
        if (me.SqDistToTarget() < 1.0f) {
            phase = 4;
            sm.m_LocalData[7] = targetId;
            me.SetAnimType(GetNormalStrikeAnim(sm, WeaponProfile::GetWeaponDataFromEntity(sm.m_LocalData[9])), 0, 1);
            EXVector zero(0.0f, 0.0f, 0.0f, 0.0f);
            SendMsg_AttackedByActor(pTarget, WeaponProfile::GetWeaponData(0x49000001), zero, zero, 1000.0f, &me, 0);
            MsgItem msg(MSG_SOMEONE_UNDER_ATTACK, 0, 0, 0, 0);
            Actor::SendMsg(msg, me.GetHeadPos(), 10.0f, &me, 0.0f);
        }
        return 1;
    }
    case 3:
        me.SetStatus(9);
        if (me.HasAnimEnded())
            phase = 2;
        return 1;
    }
    return 1;
}
