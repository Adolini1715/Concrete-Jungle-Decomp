#include "SubGroupAnims.h"
#include "ActorProfile.h"
#include "Actor.h"
#include "ActorUtil.h"
#include "FEText.h"


SubGroupAnims *SubGroupAnims::ms_pSubGroupAnimsTable;
int SubGroupAnims::ms_iSubGroupAnimsCount;

void SubGroupAnims::SetHash(int *pHash, int def)
{
    if (*pHash == 0) *pHash = def;
    else if (*pHash == 0x9000053) *pHash = 0;
}

void SubGroupAnims::Init()
{
    ms_pSubGroupAnimsTable = (SubGroupAnims *)AU_GetWorkSheet(0x1000096, 0x1400000c, 0, ms_iSubGroupAnimsCount);
    if (ms_iSubGroupAnimsCount <= 0) return;
    if (ms_pSubGroupAnimsTable->m_Hash != 0x4f000000) return;
    SubGroupAnims *pDef = ms_pSubGroupAnimsTable;
    SubGroupAnims *p = ms_pSubGroupAnimsTable;
    for (int i = 1; i < ms_iSubGroupAnimsCount; i++, p++) SetHash(&p->m_Anims[5], pDef->m_Anims[5]);
}

void SubGroupAnims::RefreshBasePointer()
{
    ms_pSubGroupAnimsTable = (SubGroupAnims *)AU_GetWorkSheet(0x1000096, 0x1400000c, 0, ms_iSubGroupAnimsCount);
}

int SubGroupAnims::GetSubGroupAnimsFromHash(int hash)
{
    SubGroupAnims *p = ms_pSubGroupAnimsTable;
    for (int i = 0; i < ms_iSubGroupAnimsCount; i++, p++)
        if (p->m_Hash == hash) return i;
    return 0;
}

SubGroupAnims &SubGroupAnims::GetSubGroupAnims(Actor &a)
{
    ActorProfile *p = AP_GetPtrFromIndex(a.m_ProfileIndex);
    if (!p) return *ms_pSubGroupAnimsTable;
    return ms_pSubGroupAnimsTable[GetSubGroupAnimsFromHash(p->m_SubGroup)];
}
