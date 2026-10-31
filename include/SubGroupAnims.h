#ifndef SUBGROUPANIMS_H
#define SUBGROUPANIMS_H
#include "types.h"

class Actor;

// One row of the sub-group animation worksheet (HT_File_SubGroupAnimsTable, sizeof 0x48).
// Row 0 must be HT_ActorSubGroup_NONE (0x4f000000): the default animations.
class SubGroupAnims {
public:
    static SubGroupAnims &GetSubGroupAnims(int i) { return ms_pSubGroupAnimsTable[i]; }
    static void SetHash(int *pHash, int def);
    static void Init();
    static void RefreshBasePointer();
    static int GetSubGroupAnimsFromHash(int hash);
    static SubGroupAnims &GetSubGroupAnims(Actor &a);

    int m_Hash;             // 0x00 HT_ActorSubGroup_*
    int m_Anims[17];        // 0x04 animation hashes (0 = use the default row's, 0x9000053 = none)
private:
    static SubGroupAnims *ms_pSubGroupAnimsTable;
    static int ms_iSubGroupAnimsCount;
};
#endif
