#include "WeaponTable.h"
#include "FEText.h"
#include "EXRand.h"

// TODO stand-ins: other areas.
void *AU_GetWorkSheet(int file, int sheet, int col, int &count);
extern unsigned int gCurrentMap_GeoHash;

static int s_PredWeapons[14] = {
    0x49000000, 0x49000001, 0x49000002, 0x49000004, 0x49000006, 0x49000007, 0x49000008,
    0x49000009, 0x4900000b, 0x4900000c, 0x4900000e, 0x4900000f,
};
static int s_NumPredWeapons = 14;

static inline float RandRange(float lo, float hi) { return g_EXRandClass.Randf() * (hi - lo) + lo; }

WeaponProfile *WeaponProfile::ms_pWeaponTable;
int WeaponProfile::ms_iWeaponTableCount;
ArmourProfile *ArmourProfile::ms_pArmourTable;
int ArmourProfile::ms_iArmourTableCount;

// NEAR: register allocation only (retail keeps the count in edi and the index in ebx).
void WeaponProfile::Init()
{
    ms_pWeaponTable = (WeaponProfile *)AU_GetWorkSheet(0x1000084, 0x1400000b, 0, ms_iWeaponTableCount);
    if (!ms_pWeaponTable) return;
    if (ms_iWeaponTableCount <= 0) return;
    int i;
    WeaponProfile *p = ms_pWeaponTable;
    for (i = 0; i < ms_iWeaponTableCount; i++, p++) {
        WeaponProfile &w = *p;
        w.m_DurationMin = (int)(EXBaseApp::NominalGameFrameRate() * *(float *)&w.m_DurationMin);
        w.m_DurationMax = (int)(EXBaseApp::NominalGameFrameRate() * *(float *)&w.m_DurationMax);
        w.m_WaitMin = (int)(EXBaseApp::NominalGameFrameRate() * *(float *)&w.m_WaitMin);
        w.m_WaitMax = (int)(EXBaseApp::NominalGameFrameRate() * *(float *)&w.m_WaitMax);
        w.m_Rate = (int)(EXBaseApp::NominalGameFrameRate() / *(float *)&w.m_Rate);
        w.m_LockFrames = (int)(EXBaseApp::NominalGameFrameRate() * *(float *)&w.m_LockFrames);
        w.m_LockTimer = 0.0f;
        if (w.m_DamageMin > w.m_DamageMax) w.m_DamageMax = w.m_DamageMin;
        w.m_NumSpecialAnims = 0;
        for (int j = 0; j < 3 && w.m_SpecialAnims[j]; j++) w.m_NumSpecialAnims++;
        if (w.m_DurationMax < w.m_DurationMin) w.m_DurationMax = w.m_DurationMin;
        if (w.m_WaitMax < w.m_WaitMin) w.m_WaitMax = w.m_WaitMin;
        if (w.m_Burst <= 0) w.m_Burst = 1;
    }
    for (i = 0; i < ms_iWeaponTableCount; i++) {
        if (ms_pWeaponTable[i].m_MapHash) {
            for (int j = 0; j < ms_iWeaponTableCount; j++)
                if (!ms_pWeaponTable[j].m_MapHash) break;
        }
    }
}

void WeaponProfile::RefreshBasePointer()
{
    ms_pWeaponTable = (WeaponProfile *)AU_GetWorkSheet(0x1000084, 0x1400000b, 0, ms_iWeaponTableCount);
}

WeaponProfile *WeaponProfile::GetWeaponDataFromEntity(int entity)
{
    if (!entity) return 0;
    int found = -1;
    for (int i = 0; i < ms_iWeaponTableCount; i++) {
        if (ms_pWeaponTable[i].m_Entity == entity) {
            if (ms_pWeaponTable[i].m_MapHash == gCurrentMap_GeoHash) { found = i; break; }
            if (ms_pWeaponTable[i].m_MapHash == 0) found = i;
        }
    }
    if (found == -1) return 0;
    return &ms_pWeaponTable[found];
}

WeaponProfile *WeaponProfile::GetWeaponData(int id)
{
    int found = -1;
    for (int i = 0; i < ms_iWeaponTableCount; i++) {
        if (ms_pWeaponTable[i].m_Id == id) {
            if (ms_pWeaponTable[i].m_MapHash == gCurrentMap_GeoHash) { found = i; break; }
            if (ms_pWeaponTable[i].m_MapHash == 0) found = i;
        }
    }
    if (found == -1) return 0;
    return &ms_pWeaponTable[found];
}

int WeaponProfile::GetWeaponDataIndexFromEntity(int entity)
{
    int found = -1;
    for (int i = 0; i < ms_iWeaponTableCount; i++) {
        if (ms_pWeaponTable[i].m_Entity == entity) {
            if (ms_pWeaponTable[i].m_MapHash == gCurrentMap_GeoHash) { found = i; break; }
            if (ms_pWeaponTable[i].m_MapHash == 0) found = i;
        }
    }
    return found;
}

int WeaponProfile::GetWeaponDataIndex(int id)
{
    int found = -1;
    for (int i = 0; i < ms_iWeaponTableCount; i++) {
        if (ms_pWeaponTable[i].m_Id == id) {
            if (ms_pWeaponTable[i].m_MapHash == gCurrentMap_GeoHash) { found = i; break; }
            if (ms_pWeaponTable[i].m_MapHash == 0) found = i;
        }
    }
    return found;
}

WeaponProfile *WeaponProfile::GetWeaponDataFromIndex(int index)
{
    if (index >= 0 && index < ms_iWeaponTableCount) return &ms_pWeaponTable[index];
    return 0;
}

int WeaponProfile::GetWeaponryHashFromPredWeapon(int index)
{
    if (index >= 0 && index < s_NumPredWeapons) return s_PredWeapons[index];
    return 0x49000000;
}

float WeaponProfile::GetMeleeDamage() const
{
    if (m_MeleeDamage > 0.0f) return m_MeleeDamage;
    return RandRange(m_DamageMin, m_DamageMax);
}

float WeaponProfile::GetBulletDamage() const
{
    return RandRange(m_DamageMin, m_DamageMax);
}

float WeaponProfile::GetDuration() const
{
    return RandRange((float)m_DurationMin, (float)m_DurationMax);
}

float WeaponProfile::GetWait() const
{
    return RandRange((float)m_WaitMin, (float)m_WaitMax);
}

void WeaponProfile::Update()
{
    WeaponProfile *p = ms_pWeaponTable;
    for (int i = 0; i < ms_iWeaponTableCount; i++, p++) {
        if (p->m_LockTimer > 0.0f) p->m_LockTimer -= EXBaseApp::GameFrameRateMul();
    }
}

signed char WeaponProfile::IsPredatorProjectileWeapon() const
{
    if (m_Id == 0x49000007 || m_Id == 0x49000008 || m_Id == 0x49000009 || m_Id == 0x4900000a) return 1;
    return 0;
}

int WeaponProfile::GetSpecialAnimHash() const
{
    int n = m_NumSpecialAnims;
    if (!n) return 0;
    return m_SpecialAnims[g_EXRandClass.Rand32() % n];
}

float WeaponProfile::GetMineTriggerRadius() const
{
    return m_MineTriggerRadius * 0.6660000086f;
}

void ArmourProfile::Init()
{
    ms_pArmourTable = (ArmourProfile *)AU_GetWorkSheet(0x1000084, 0x14000012, 0, ms_iArmourTableCount);
    for (int i = 0; i < ms_iArmourTableCount; i++) {
        ms_pArmourTable[i].m_Damage[0] = (100.0f - ms_pArmourTable[i].m_Damage[0]) * 0.01f;
        ms_pArmourTable[i].m_Damage[1] = (100.0f - ms_pArmourTable[i].m_Damage[1]) * 0.01f;
        ms_pArmourTable[i].m_Damage[2] = (100.0f - ms_pArmourTable[i].m_Damage[2]) * 0.01f;
        ms_pArmourTable[i].m_Damage[3] = (100.0f - ms_pArmourTable[i].m_Damage[3]) * 0.01f;
        ms_pArmourTable[i].m_Damage[4] = (100.0f - ms_pArmourTable[i].m_Damage[4]) * 0.01f;
        ms_pArmourTable[i].m_Damage[5] = (100.0f - ms_pArmourTable[i].m_Damage[5]) * 0.01f;
        ms_pArmourTable[i].m_Damage[6] = (100.0f - ms_pArmourTable[i].m_Damage[6]) * 0.01f;
        ms_pArmourTable[i].m_Damage[7] = (100.0f - ms_pArmourTable[i].m_Damage[7]) * 0.01f;
        ms_pArmourTable[i].m_Damage[8] = (100.0f - ms_pArmourTable[i].m_Damage[8]) * 0.01f;
        ms_pArmourTable[i].m_Damage[9] = (100.0f - ms_pArmourTable[i].m_Damage[9]) * 0.01f;
        ms_pArmourTable[i].m_Damage[10] = (100.0f - ms_pArmourTable[i].m_Damage[10]) * 0.01f;
        ms_pArmourTable[i].m_Damage[11] = (100.0f - ms_pArmourTable[i].m_Damage[11]) * 0.01f;
    }
}

void ArmourProfile::RefreshBasePointer()
{
    ms_pArmourTable = (ArmourProfile *)AU_GetWorkSheet(0x1000084, 0x14000012, 0, ms_iArmourTableCount);
}

ArmourProfile *ArmourProfile::GetArmourData(int id)
{
    for (int i = 0; i < ms_iArmourTableCount; i++)
        if (ms_pArmourTable[i].m_Id == id) return &ms_pArmourTable[i];
    return 0;
}

int ArmourProfile::GetArmourDataIndex(int id)
{
    for (int i = 0; i < ms_iArmourTableCount; i++)
        if (ms_pArmourTable[i].m_Id == id) return i;
    return -1;
}

ArmourProfile *ArmourProfile::GetArmourDataFromIndex(int index)
{
    if (index >= 0 && index < ms_iArmourTableCount) return &ms_pArmourTable[index];
    return 0;
}

int WeaponProfile::GetWeaponryHashFromPredWeapon_Safe(int index)
{
    if (index >= 0x49000000) return index;
    return GetWeaponryHashFromPredWeapon(index);
}
