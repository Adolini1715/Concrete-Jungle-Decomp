#ifndef XITEM_H
#define XITEM_H
#include "EXItem.h"
#include "EXList.h"
#include "EXItemEnvStandIn.h"

class EXGeoFile;
class EXTrigger;
struct EXGeoTrigger;
class EXGeoMap;

// Game item (ItemUpdate.obj): EXItem plus a game list link and the map it was created on. sizeof 0x174.
class XItem : public EXItem {
    EX_DECLARE_DYNAMIC(XItem)
public:
    virtual ~XItem();
    EX_DECLARE_ITEMTYPE(XItem)
    XItem();
    static XItem *CreateObject();
    static XItem *NewItem() { return new XItem; }
    static XItem *CreateItem(EXGeoFile *pFile, unsigned int hash, EXRuntimeClass *pHandlerClass, const EXVector &pos);
    // Game-physics view of EXItem::m_pItemPhysics ([fi] copy in App.obj).
    class XItemPhysics *ItemPhysics() const { return (class XItemPhysics *)m_pItemPhysics; }
    u32 m_Unk160;               // 0x160 TODO
    EXODListItem m_ListItem;    // 0x164 XItemList link
    unsigned int m_MapHash;     // 0x170 gCurrentMap_GeoHash at creation
};

// List of game items through XItem::m_ListItem.
class XItemList {
public:
    XItemList();
    ~XItemList();
    void Add(XItem *pItem);
    void Remove(XItem *pItem);
    XItem *GetFirst();
    XItem *GetNext(XItem *pItem);
    EXODList m_List;
};

// Game item environment: sky handling and music priorities. sizeof 0xac.
class XItemEnv : public EXItemEnv {
public:
    virtual ~XItemEnv();
    virtual void Initialise();
    virtual void StartMusic(unsigned int hash, unsigned int flags, int a, int b);
    virtual void ProcessSky(EXItemAnimator *pSky) const;
    XItemEnv();
    void InitTriggers(EXItemAnimator_Map *pMap,
                      unsigned int (__cdecl *pFunc)(EXTrigger *, EXGeoTrigger *, EXGeoMap *, EXMapOn, unsigned short));
    unsigned int MusicFlags() { return m_MusicFlags; }  // appended by the actor agent (AIControl.obj)
    unsigned int MusicHash() { return m_MusicHash; }
    void UnlockMusic() { m_MusicFlags &= ~8; }  // [fi] copy in CameraScript.obj
    unsigned int m_MusicFlags;  // 0xa4
    unsigned int m_MusicHash;   // 0xa8
};
extern XItemEnv theItemEnv;
extern XItem *gpPlayerItem[4];
extern XItem *gpCameraItem[4];
extern XItemList gTestCarList;
extern XItemList gTestDroneList;
inline XItemEnv *XGetWorld() { return &theItemEnv; }
#endif
