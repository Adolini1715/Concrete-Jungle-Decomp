#ifndef SCANINFO_H
#define SCANINFO_H
#include "types.h"
#include "EXString.h"

class EXItem;
class FEObject;

// HUD scan page description, filled by the locked-on object's GetScanInfo (FEHud::GetScanInfo).
struct ScanInfo {
    u32 m_0;                    // 0x00 scan page (0 thermal, 1 neuro, 2 tech)
    EXItem *m_pItem;            // 0x04 locked-on item the page describes
    u32 m_ModelEntity;          // 0x08 entity shown spinning on the tech page (FEHud::UpdateTechScan)
    u32 m_ModelFile;            // 0x0c its file
    u32 m_Text[4][8];           // 0x10 text hashes: four columns of up to eight lines (blank hash = empty line)
    int m_id90;                 // 0x90 actor id (Actor::GetScanInfo)
    u8 m_94[0xa0 - 0x94];
    u32 m_a0;                   // 0xa0 scan script played as the page background
    float m_a4;                 // 0xa4 health fraction shown on the thermal page gauge
    EXString m_text;            // 0xa8 summary line (thermal page)
    s8 m_ac;                    // 0xac page has content
    signed char (__cdecl *m_pCallback)(signed char, FEObject *, ScanInfo *);  // 0xb0 called on open (1) and every frame (0)
};
#endif
