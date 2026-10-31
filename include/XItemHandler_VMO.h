#ifndef XITEMHANDLER_VMO_H
#define XITEMHANDLER_VMO_H
#include "EXItemHandler.h"
#include "EXTexSwapInfo.h"
#include "EXExtRenderInfoStandIn.h"

class XItem;
class XItemList;
class FEObject;
struct ScanInfo;

// Vision-mode object: an item whose look changes with the Predator's vision mode (FX_VMO.obj). sizeof 0xc0.
class XItemHandler_VMO : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_VMO)
public:
    virtual ~XItemHandler_VMO();
    virtual s8 Initialise();
    virtual s8 DoUpdate();
    XItemHandler_VMO() {}
    static XItemHandler_VMO *CreateObject();
    static signed char ScanCallback(signed char b, FEObject *pObj, ScanInfo *pInfo);
    signed char GetScanInfo(int type, ScanInfo *pInfo);
    EXTexSwapInfo m_TexSwapInfo;        // 0x08
    EXExtRenderInfo m_ExtRenderInfo;    // 0x1c
    EXVector m_Colour;                  // 0x9c trigger colour / 128 (alpha 1)
    unsigned int m_VMOFlags;            // 0xac per-vision-mode visibility bits (Update_VMO_Generic)
    int m_DeleteFlagChannel;            // 0xb0
    int m_ShowFlagChannel;              // 0xb4
    u32 m_ScanText;                     // 0xb8
    u32 m_ScanText2;                    // 0xbc
};

extern XItemList gVMOList;
void Update_VMO_Generic(unsigned char mode, unsigned int flags, XItem *pItem, EXTexSwapInfo &tex);
void Update_VMO(unsigned char mode);
#endif
