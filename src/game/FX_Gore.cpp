#include "FX_Gore.h"
#include "Actor.h"
#include "ActorProfile.h"
#include "XItem.h"
#include "XItemHandler_Player.h"
#include "EXVector.h"
#include "EXRand.h"
#include "EXHashCodeFile.h"
#include "EXTrace.h"
#include "FEText.h"
#include "XItemHandler_ScriptBase.h"
#include "EXBaseSoundManager.h"
#include "EXQuaternion.h"
#include "EXRayCastTemp.h"
#include "XItemHandler_TestCloth.h"
#include "EXItemAnimator.h"
#include "EXGeoFile.h"
#include <math.h>

// Effect script hashes for floor pools and wall splats, per blood group.
unsigned int PoolHash[10] = { 0x200012e, 0x2000130, 0x2000131, 0x2000132, 0x2000133, 0x2000134, 0x2000135, 0x200019c, 0x200019d, 0x200019e };
unsigned int SplatHash[9] = { 0x2000130, 0x2000131, 0x2000135, 0x2000136, 0x2000137, 0x200012f, 0x200019c, 0x200019d, 0x200019e };
unsigned int PredPoolHash[10] = { 0x200019f, 0x20001a0, 0x20001a1, 0x20001a2, 0x20001a3, 0x20001a4, 0x20001a5, 0x20001a9, 0x20001aa, 0x20001ab };
unsigned int PredSplatHash[9] = { 0x20001a0, 0x20001a1, 0x20001a5, 0x20001a6, 0x20001a7, 0x20001a8, 0x20001a9, 0x20001aa, 0x20001ab };
unsigned int OverkillPoolHash[1] = { 0x40004e4 };
unsigned int OverkillSplatHash[1] = { 0x4000327 };
unsigned int AcidPoolHash[8] = { 0x2000191, 0x2000197, 0x2000198, 0x2000199, 0x200019b, 0x200019a, 0x20001ac, 0x20001ad };
unsigned int AcidSplatHash[8] = { 0x2000191, 0x2000197, 0x2000198, 0x2000199, 0x200019b, 0x200019a, 0x20001ac, 0x20001ad };
unsigned int ScorchioHash[12] = { 0x2000216, 0x2000217, 0x2000219, 0x200021a, 0x200021b, 0x200021c, 0x2000218, 0x200021d, 0x200021e, 0x200021f, 0x2000220, 0x2000221 };

struct BTab_ {
    unsigned int *m_pPool;
    int m_nPool;
    unsigned int *m_pSplat;
    int m_nSplat;
};
BTab_ BloodTable[9] = {
    { PoolHash, 10, SplatHash, 9 },
    { PredPoolHash, 10, PredSplatHash, 9 },
    { OverkillPoolHash, 1, OverkillSplatHash, 1 },
    { PoolHash, 10, SplatHash, 9 },
    { PoolHash, 10, SplatHash, 9 },
    { AcidPoolHash, 8, AcidSplatHash, 8 },
    { PredPoolHash, 10, PredSplatHash, 9 },
    { PoolHash, 10, SplatHash, 9 },
    { ScorchioHash, 12, ScorchioHash, 12 },
};

short CurrentGore;
XItem *GoreList[16];
EXVector GoreColour[16];

int Gore_GetBloodGroup(Actor *pActor)
{
    switch (pActor->GetSkin()->m_BloodType) {
    case 5:
        return 6;
    case 2:
        return 7;
    case 1:
        return 1;
    case 3:
        return 5;
    case 4:
        return 4;
    case 0:
        break;
    default:
        ex::TRACE("Gore_GetBloodGroup() - unknown blood type %s %d",
                  EXHashCodeFile::Debug_HashCodeName(pActor->GetSkin()->m_BloodType), pActor->GetSkin()->m_BloodType);
        break;
    }
    return 0;
}

void Gore_Initialise()
{
    for (short i = 0; i < 16; i++) {
        GoreList[i] = 0;
        GoreColour[i].set(1.0f, 1.0f, 1.0f, 1.0f);
    }
    CurrentGore = 0;
}

void Gore_GetBloodColour(int group, float &r, float &g, float &b)
{
    switch (group) {
    case 1:
        r = (float)(EXRand(8) + 37);
        g = (float)(EXRand(36) + 130);
        b = (float)(EXRand(1) + 0);
        break;
    case 7:
        r = (float)(EXRand(39) + 37);
        g = (float)(EXRand(8) + 7);
        b = (float)(EXRand(1) + 0);
        break;
    case 5:
        r = (float)(EXRand(89) + 167);
        g = (float)(EXRand(89) + 167);
        b = (float)(EXRand(39) + 90);
        break;
    case 6:
        r = (float)(EXRand(36) + 130);
        g = (float)(EXRand(25) + 24);
        b = (float)(EXRand(18) + 7);
        break;
    case 3:
        r = (float)(EXRand(98) + 90);
        g = (float)(EXRand(95) + 88);
        b = (float)(EXRand(69) + 63);
        break;
    case 4:
        r = (float)(EXRand(1) + 255);
        g = (float)(EXRand(1) + 255);
        b = (float)(EXRand(1) + 255);
        break;
    case 8:
        r = 0.0f;
        g = 0.0f;
        b = 0.0f;
        break;
    default:
        r = (float)(EXRand(46) + 45);
        g = (float)(EXRand(10) + 8);
        b = (float)(EXRand(1) + 0);
        break;
    }
    r *= 1.0f / 256.0f;
    g *= 1.0f / 256.0f;
    b *= 1.0f / 256.0f;
}

void Gore_GetVisionModeBloodColour(unsigned char mode, float &r, float &g, float &b)
{
    switch (mode) {
    case 1:
        r = 2.0f;
        g = 1.0f;
        b = 0.0f;
        break;
    case 2:
        r = 0.7f;
        g = 0.0f;
        b = 0.0f;
        break;
    case 3:
        r = 0.0f;
        g = 1.0f;
        b = 0.0f;
        break;
    }
}

void Gore_GetBloodColour(Actor *pActor, float &r, float &g, float &b)
{
    u8 mode = XItemHandler_Player::Instance()->m_Tint;
    switch (mode) {
    case 0:
        Gore_GetBloodColour(Gore_GetBloodGroup(pActor), r, g, b);
        break;
    case 1:
    case 2:
    case 3:
        Gore_GetVisionModeBloodColour(mode, r, g, b);
        break;
    }
}

void Gore_BloodPool(EXVector &pos, int group)
{
    EXVector down(0.0f, -1.0f, 0.0f, 0.0f);
    Gore_ShootSplat(pos, down, group);
}

void Gore_TintToBloodColour(XItem *pItem, int group)
{
    float r, g, b;
    if (pItem) {
        Gore_GetBloodColour(group, r, g, b);
        pItem->RenderInfo().RenderFlagsRef().BitSet(0xc);
        pItem->RenderInfo().SetRGBAMult(r, g, b, 1.0f);
    }
}

void Gore_TintToBloodColour(XItem *pItem, Actor *pActor)
{
    float r, g, b;
    int group = Gore_GetBloodGroup(pActor);
    if (pItem) {
        Gore_GetBloodColour(group, r, g, b);
        pItem->RenderInfo().RenderFlagsRef().BitSet(0xc);
        pItem->RenderInfo().SetRGBAMult(r, g, b, 1.0f);
    }
}

void Gore_TintToBloodColourRGB(XItem *pItem, float r, float g, float b)
{
    if (pItem) {
        pItem->RenderInfo().RenderFlagsRef().BitSet(0xc);
        pItem->RenderInfo().SetRGBAMult(r, g, b, 1.0f);
    }
}

void Gore_Bloodcleaner()
{
    for (CurrentGore = 0; CurrentGore < 16; CurrentGore++) {
        if (GoreList[CurrentGore]) GoreList[CurrentGore]->ItemFlags() = 0x10;
        GoreList[CurrentGore] = 0;
        GoreColour[CurrentGore].set(1.0f, 1.0f, 1.0f, 1.0f);
    }
    CurrentGore = 0;
}

void Gore_ShootSplat(EXVector &a, EXVector &b, Actor *pActor)
{
    Gore_ShootSplat(a, b, Gore_GetBloodGroup(pActor));
}

void Gore_BloodPool(EXVector &pos, Actor *pActor)
{
    EXVector down(0.0f, -1.0f, 0.0f, 0.0f);
    Gore_ShootSplat(pos, down, Gore_GetBloodGroup(pActor));
}

// Spawns the blood or impact effect for a hit on an actor. a is unused, b = blocked hit, c = hit type
// (0 impact, 1 spurt, 2 combi, 3 sword, 4 pole), d = position, e = direction, f = play the hit sound.
void Gore_CreateActorBlood(Actor *pActor, signed char a, signed char b, int c, EXVector &d, EXVector &e, signed char f)
{
    float angle = (float)atan2(e.x, e.z);
    EXVector dir(e.x, e.y, e.z, e.w);
    int script = 0;
    unsigned int sound;
    int group = Gore_GetBloodGroup(pActor);

    if (!b) {
        switch (c) {
        default: sound = 0x1af00247; break;
        case 1: sound = 0x1af00211; break;
        case 2: sound = 0x1af00216; break;
        case 3:
        case 4: sound = 0x1af00226; break;
        }
        switch (group) {
        case 0: case 3: case 5: case 7:
            switch (c) {
            default: script = 0x40000e4; break;
            case 1: script = 0x40000e3; break;
            case 2: script = 0x400041d; break;
            case 3: script = 0x400041b; break;
            case 4: script = 0x400041c; break;
            }
            break;
        case 1: case 6:
            switch (c) {
            default: script = 0x40004d8; break;
            case 1: script = 0x40004d4; break;
            case 2: script = 0x40004d3; break;
            case 3: script = 0x40004d1; break;
            case 4: script = 0x40004d2; break;
            }
            break;
        case 4:
            script = 0x40004a2;
            sound = (c == 7) ? 0 : 0x1af0028d;
            break;
        }
        if (c != 0 && c != 2) {
            if (EXRand32() % 6 == 0) {
                dir.y = 0.0f;
                Gore_ShootSplat(d, dir, group);
            }
            if (EXRand32() % 6 == 0)
                Gore_BloodPool(d, group);
        }
        if (!script) goto sound;
    } else {
        switch (c) {
        case 0: script = 0x40000e4; sound = 0x1af005b5; break;
        default: sound = 0x1af00212; script = 0x400041e; break;
        case 2: script = 0x40000e4; sound = 0x1af00217; break;
        case 3:
        case 4: sound = 0x1af00227; script = 0x400041e; break;
        }
        if (pActor->m_ActorFlags.m_bUlysses)
            script = 0x40004a2;
        else if (!script)
            goto sound;
    }
    {
        XItem *pItem = XItemHandler_ScriptBase::FX_CreateBlood(d, script, group, 0x40);
        if (pItem)
            pItem->SetRotation(EXQuaternion(0.0f, angle, 0.0f, 0.0f));
    }
sound:
    if (f && sound != 0 && c < 0xe)
        EXBaseSoundManager::GetSoundManager()->StartSound(sound, d, -1, 0);
}

// Orients a splat on the surface the ray hit: slightly off the wall, facing along the normal. Flat
// floors get a random spin instead.
static void Gore_SplatPose(const EXRayCastTemp &r, EXVector &pos, EXVector &rot)
{
    EXVector hit = r.Intersect();
    EXVector n = r.m_normal;
    pos = EXVector(hit.x + n.x * 0.005f, hit.y + n.y * 0.005f, hit.z + n.z * 0.005f, hit.w + n.w * 0.005f);
    if (n.y != 1.0f) {
        float nx = -n.x, ny = -n.y, nz = -n.z;
        float yaw = (float)atan2(nx, nz);
        float lenSq = nx * nx + nz * nz;
        float len = lenSq < 0.0f ? 0.0f : (float)sqrt(lenSq);
        rot = EXVector(-(float)atan2(ny, len), yaw, 0.0f, 0.0f);
    } else {
        float spin = EXRandf() * 6.2831855f;
        rot = EXVector(1.5707964f, 0.0f, spin, 0.0f);
    }
}

// Frees the oldest gore item slot and returns the new current slot.
static short Gore_NextSlot()
{
    CurrentGore = (short)((CurrentGore + 1) % 16);
    if (GoreList[CurrentGore])
        GoreList[CurrentGore]->ItemFlags() = 0x10;
    return CurrentGore;
}

// Tints the splat in the current slot with the blood colour and, in a vision mode, the vision colour.
static void Gore_ApplyTint(float r, float g, float b)
{
    XItem *pItem = GoreList[CurrentGore];
    pItem->RenderInfo().RenderFlagsRef().BitSet(4);
    pItem->RenderInfo().SetRGBAMult(r, g, b, 1.0f);
    GoreColour[CurrentGore].set(r, g, b, 1.0f);
    unsigned char mode = XItemHandler_Player::Instance()->m_Tint;
    if (mode) {
        Gore_GetVisionModeBloodColour(mode, r, g, b);
        pItem->RenderInfo().SetRGBAMult(r, g, b, 1.0f);
        pItem->RenderInfo().RenderFlagsRef().BitSet(8);
    }
}

// Throws a ray from pos along dir; if it finds a surface, leaves a splat of the given blood group there.
void Gore_ShootSplat(EXVector &a, EXVector &b, int group)
{
    if (group == 0 && EXRand32() % 100 < 60) {
        for (XItem *pCloth = gClothList.GetFirst(); pCloth; pCloth = gClothList.GetNext(pCloth)) {
            EXVector d = pCloth->Position() - a;
            if (d.LengthXYZSquared() < 9.0f)
                ((XItemHandler_TestCloth *)pCloth->ItemHandler())->BloodStain();
        }
    }

    EXVector dir(b.x, b.y, b.z, b.w);
    float lenSq = dir.x * dir.x + dir.w * dir.w + dir.z * dir.z + dir.y * dir.y;
    if (lenSq >= 0.0f) {
        float len = (float)sqrt(lenSq);
        if (len > 1e-5f) {
            float inv = 1.0f / len;
            dir.y *= inv; dir.x *= inv; dir.z *= inv; dir.w *= inv;
        }
    }
    dir.x *= 3.0f; dir.y *= 3.0f; dir.z *= 3.0f; dir.w *= 3.0f;

    EXVector start(a.x + 0.5f, a.y + 0.5f, a.z + 0.5f, a.w);
    EXRayCastTemp r;
    r.Set(start, dir, 0, 0, 0, 0);
    r.m_mask = 0x4088;
    theItemEnv.DoRayCastToMap(r, 0);
    if (!r.m_bHit) return;
    EXVector n1 = r.m_normal;

    r.Set(a, dir, 0, 0, 0, 0);
    r.m_mask = 0x4088;
    theItemEnv.DoRayCastToMap(r, 0);
    if (!r.m_bHit) return;
    if (!(fabs(r.m_normal.x - n1.x) < 0.001f && fabs(r.m_normal.y - n1.y) < 0.001f
          && fabs(r.m_normal.z - n1.z) < 0.001f && fabs(r.m_normal.w - n1.w) < 0.001f))
        return;

    if (group == 5 || group == 1) {
        EXVector hit = r.Intersect();
        for (int i = 0; i < 16; i++) {
            if (GoreList[i] && (GoreList[i]->Position() - hit).LengthXYZSquared() < 0.25f)
                return;
        }
    }

    Gore_NextSlot();
    GoreList[CurrentGore] = XItem::CreateItem(0, 0, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000016, 0, 0);

    unsigned int hash;
    if (fabs(dir.y) >= 0.5f) {
        const BTab_ &t = BloodTable[group];
        short idx = t.m_nPool == 0 ? 0 : (short)(EXRand32() % (unsigned int)t.m_nPool);
        hash = t.m_pPool[idx];
    } else {
        const BTab_ &t = BloodTable[group];
        if (t.m_nSplat == 0)
            hash = t.m_pSplat[0];
        else
            hash = t.m_pSplat[(short)(EXRand32() % (unsigned int)t.m_nSplat)];
    }

    EXVector pos, rot;
    Gore_SplatPose(r, pos, rot);
    XItem *pItem = GoreList[CurrentGore];
    pItem->SetPosition(pos);
    pItem->SetRotation(EXQuaternion(rot.x, rot.y, rot.z, 0.0f));
    pItem->AddAnimator(EXItemAnimator::CreateFrom(pFile, hash));
    theItemEnv.AddItem(pItem, 0);

    if (group == 5) {
        pItem->AddAnimator(EXItemAnimator::CreateFrom(pFile, 0x1100000e));
        return;
    }
    float cr, cg, cb;
    Gore_GetBloodColour(group, cr, cg, cb);
    Gore_ApplyTint(cr, cg, cb);
}

// Leaves a pool under an overkilled actor's head.
void Gore_CreateOverkillBlood(Actor *pActor)
{
    EXVector dir(0.0f, -3.0f, 0.0f, 0.0f);
    EXVector head = pActor->GetHeadPos();
    EXVector start(head.x + 0.5f, head.y + 0.5f, head.z + 0.5f, head.w);
    EXRayCastTemp r;
    r.Set(start, dir, 0, 0, 0, 0);
    r.m_mask = 0x4088;
    theItemEnv.DoRayCastToMap(r, 0);
    if (!r.m_bHit) return;
    EXVector n1 = r.m_normal;

    r.Set(pActor->GetHeadPos(), dir, 0, 0, 0, 0);
    r.m_mask = 0x4088;
    theItemEnv.DoRayCastToMap(r, 0);
    if (!r.m_bHit) return;
    if (!(fabs(r.m_normal.x - n1.x) < 0.001f && fabs(r.m_normal.y - n1.y) < 0.001f
          && fabs(r.m_normal.z - n1.z) < 0.001f && fabs(r.m_normal.w - n1.w) < 0.001f))
        return;

    Gore_NextSlot();
    EXVector pos, rot;
    Gore_SplatPose(r, pos, rot);
    EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x1000016, 0, 0);
    XItem *pItem = XItem::CreateItem(0, 0, 0, EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    GoreList[CurrentGore] = pItem;
    pItem->AddAnimator(EXItemAnimator::CreateFrom(pFile, pActor->m_ActorFlags2.m_bPredator ? 0x40004e6 : 0x40004e4));
    pItem->SetPosition(pos);
    pItem->SetRotation(EXQuaternion(rot.x, rot.y, rot.z, 0.0f));
    pItem->RenderInfo().RenderFlagsRef().BitSet(8);
    pItem->RenderInfo().RenderFlagsRef().BitSet(4);
    float cr, cg, cb;
    Gore_GetBloodColour(pActor, cr, cg, cb);
    Gore_ApplyTint(cr, cg, cb);
    theItemEnv.AddItem(pItem, 0);
}
