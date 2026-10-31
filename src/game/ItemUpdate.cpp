#include "XItem.h"
#include "XItemHandler.h"
#include "EXItemAnimator.h"
#include "EXGeoComplexRefObject.h"
#include "FEText.h"
#include "XAppStandIn.h"

// TODO stand-in: AI control (AIControl.obj, not this area).
class AIControl {
public:
    signed char CombatMusicPlaying() { return m_bCombatMusic; }
    u8 m_Unk0[0x28c];       // TODO layout
    signed char m_bCombatMusic;  // 0x28c
};
extern AIControl gAIControl;
extern unsigned int gCurrentMap_GeoHash;

XItem *gpPlayerItem[4];
XItem *gpCameraItem[4];

EX_IMPLEMENT_DYNCREATE(XItem, EXItem)

XItemEnv::~XItemEnv() {}
void XItemEnv::Initialise()
{
    m_MusicFlags = 0;
    m_MusicHash = 0;
}

XItem::XItem() {}
XItem::~XItem() {}

XItemList::XItemList() {}
XItemList::~XItemList() {}
void XItemList::Add(XItem *pItem)
{
    if (pItem) m_List.AddHead(pItem->m_ListItem, pItem);
}
void XItemList::Remove(XItem *pItem)
{
    if (pItem) m_List.Remove(pItem->m_ListItem);
}
XItem *XItemList::GetFirst()
{
    EXODListItem *p = m_List.Head();
    if (!p) return 0;
    return (XItem *)p->Object();
}
XItem *XItemList::GetNext(XItem *pItem)
{
    if (!pItem) return 0;
    EXODListItem *p = &pItem->m_ListItem;
    if (!p) return 0;
    p = p->Next();
    if (!p) return 0;
    return (XItem *)p->Object();
}

XItemEnv::XItemEnv() : EXItemEnv(0, 0)
{
    m_MusicFlags = 0;
    m_MusicHash = 0;
}

XItem *XItem::CreateItem(EXGeoFile *pFile, unsigned int hash, EXRuntimeClass *pHandlerClass, const EXVector &pos)
{
    XItem *pItem = NewItem();
    if (!pItem) return 0;
    pItem->SetGeoFile(pFile);
    pItem->SetRefObject(hash);
    pItem->SetPosition(pos);
    pItem->m_MapHash = gCurrentMap_GeoHash;
    if (pHandlerClass) {
        EXItemHandler *pHandler = (EXItemHandler *)pHandlerClass->m_pfnCreateObject();
        if (pHandler) {
            pItem->SetItemHandler(pHandler);
            pHandler->Initialise();
        }
    }
    return pItem;
}

void XItemEnv::ProcessSky(EXItemAnimator *pSky) const
{
    if (XItemHandler_Player::Instance()) {
        if (XItemHandler_Player::Instance()->m_Tint) pSky->ClrHasRender();
        else pSky->SetHasRender();
    }
    pSky->DoItemUpdate();
}

void XItemEnv::StartMusic(unsigned int hash, unsigned int flags, int a, int b)
{
    if (theApp.GameStateCurrent()->m_State != 9) return;
    if (theApp.PlayingDeathMusic()) return;
    if (gAIControl.CombatMusicPlaying() && !(flags & 4)) return;
    if ((m_MusicFlags & 8) && !(flags & 0x10)) return;
    EXItemEnv::StartMusic(hash, flags, a, b);
    m_MusicHash = hash;
    m_MusicFlags = flags;
}

XItemEnv theItemEnv;
XItemList gTestCarList;
XItemList gTestDroneList;
