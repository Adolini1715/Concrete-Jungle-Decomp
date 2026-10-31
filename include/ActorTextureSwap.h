#ifndef ACTORTEXTURESWAP_H
#define ACTORTEXTURESWAP_H
#include "types.h"
#include "EXList.h"
#include "EXArray.h"
#include "EXMemory.h"
#include <string.h>

class Actor;

// One row of the texture swap worksheet (HT_File_ActorProfiles / 0x1400000e, sizeof 0x24).
struct ActorTextureSwapRow {
    unsigned int m_Skin;        // 0x00 skin hash the row applies to
    unsigned int m_Head;        // 0x04
    unsigned int m_ArmLeft;     // 0x08
    unsigned int m_ArmRight;    // 0x0c
    unsigned int m_Body;        // 0x10
    unsigned int m_Belt;        // 0x14
    unsigned int m_CutHead;     // 0x18
    unsigned int m_CutArmLeft;  // 0x1c
    unsigned int m_CutArmRight; // 0x20
};

// A texture set for one skin (sizeof 0x30), listed in its ActorTextureSwapEntry.
class ActorTextureSwapItem : public EXDListItem {
public:
    ActorTextureSwapItem()
    {
        m_bValid = 0;
        memset(&m_Row, 0, sizeof(m_Row));
    }
    // NEAR: 8 bytes differ (retail loads the count before lea edi when emitted alone).
    ActorTextureSwapItem(ActorTextureSwapRow *pRow)
    {
        m_bValid = 1;
        m_Row = *pRow;
    }
    unsigned int Head() { return m_Row.m_Head; }
    unsigned int ArmLeft() { return m_Row.m_ArmLeft; }
    unsigned int ArmRight() { return m_Row.m_ArmRight; }
    unsigned int Body() { return m_Row.m_Body; }
    unsigned int Belt() { return m_Row.m_Belt; }
    unsigned int CutHead() { return m_Row.m_CutHead; }
    unsigned int CutArmLeft() { return m_Row.m_CutArmLeft; }
    unsigned int CutArmRight() { return m_Row.m_CutArmRight; }
    signed char Valid() { return m_bValid; }
    s8 m_bValid;                // 0x08
    ActorTextureSwapRow m_Row;  // 0x0c
};

// All texture sets of one skin (sizeof 0x18); Pick() cycles through them.
class ActorTextureSwapEntry {
public:
    ActorTextureSwapEntry(signed char bDefault = 1)
    {
        m_bValid = 1;
        m_Skin = 0;
        m_Count = 1;
        m_Pick = 0;
        m_Items.Init();
        if (bDefault) m_Items.AddTail(new ActorTextureSwapItem);
    }
    ~ActorTextureSwapEntry()
    {
        if (m_bValid) {
            while (m_Items.Head()) EXFree(m_Items.RemoveHead());
            m_Count = 0;
        }
    }
    void Add(ActorTextureSwapRow *pRow)
    {
        m_Items.AddTail(new ActorTextureSwapItem(pRow));
        m_Count++;
    }
    ActorTextureSwapItem *Pick()
    {
        ActorTextureSwapItem *p = (ActorTextureSwapItem *)m_Items.Head();
        for (int n = 0; n < m_Pick; n++) p = (ActorTextureSwapItem *)p->Next();
        m_Pick = (m_Pick + 1) % m_Count;
        return p;
    }
    void SetSkin(unsigned int skin) { m_Skin = skin; }
    unsigned int Skin() { return m_Skin; }
    void Invalidate() { m_bValid = 0; }
    s8 m_bValid;                // 0x00 owns its items
    unsigned int m_Skin;        // 0x04
    int m_Count;                // 0x08
    EXDList m_Items;            // 0x0c
    int m_Pick;                 // 0x14
};

class ActorTextureSwapArray : public EXArray<ActorTextureSwapEntry, ActorTextureSwapEntry &> {
};

// Per-skin texture variations and vision-mode texture swaps (ActorTextureSwap.obj).
class ActorTextureSwap {
public:
    static void ApplyAlternate(Actor *pActor);
    static void ApplyLostLimbs(Actor *pActor);
    static void Apply(Actor *pActor);
    static void Initialise(Actor *pActor);
    static void PostLoadInit();
    static void Shutdown();
    static void PreLoadInit();
private:
    static ActorTextureSwapArray m_Data;
    static int m_Count;
};
#endif
