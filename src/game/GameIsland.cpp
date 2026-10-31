#include "XGameWnd.h"
#include "XItem.h"
#include "EXGeoFile.h"
#include "FEText.h"

extern EXGeoFile *pMapGeoFile;

// District bounds table (TODO field names).
struct _DistrictData {
    float x0, x1, z0, z1;
    unsigned int hash;
};

struct _IslandDistrictState2 {
    unsigned int hash;       // +0 section group hash
    int state;               // +4
    int state2;              // +8
    int unkC;                // +0xc
    unsigned int hash2;      // +0x10
    unsigned int section;    // +0x14
};

int Island0Map[9][10] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 1, 2, 3, 4, 5, 6, 0, 0 },
    { 0, 0, 7, 8, 9, 10, 0, 11, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
};

_DistrictData Island_0_Data[13] = {
    { -600.0f, -400.0f, 0.0f, 200.0f, 0x5100005a },
    { -400.0f, -200.0f, 0.0f, 200.0f, 0x5100005b },
    { -200.0f, 0.0f, 0.0f, 200.0f, 0x5100005c },
    { 0.0f, 200.0f, 0.0f, 200.0f, 0x5100005d },
    { 200.0f, 400.0f, 0.0f, 200.0f, 0x5100005e },
    { 400.0f, 600.0f, 0.0f, 200.0f, 0x5100005f },
    { -600.0f, -400.0f, -200.0f, 0.0f, 0x5100005a },
    { -400.0f, -200.0f, -200.0f, 0.0f, 0x51000060 },
    { -200.0f, 0.0f, -200.0f, 0.0f, 0x51000064 },
    { 0.0f, 200.0f, -200.0f, 0.0f, 0x51000065 },
    { 200.0f, 400.0f, -200.0f, 0.0f, 0x5100005a },
    { 400.0f, 600.0f, -200.0f, 0.0f, 0x51000066 },
    { 0.0f, 0.0f, 0.0f, 0.0f, 0x5100005a },
};

_IslandDistrictState2 Island_0[12] = {
    { 0x08000065, 0, 0, 0, 0x060000b6, 0x14000008 },
    { 0x08000066, 0, 0, 0, 0x060000a9, 0x14000009 },
    { 0x08000067, 0, 0, 0, 0x060000ad, 0x14000008 },
    { 0x08000068, 0, 0, 0, 0x060000ae, 0x14000009 },
    { 0x08000069, 0, 0, 0, 0x060000af, 0x14000008 },
    { 0x0800006a, 0, 0, 0, 0x060000b0, 0x14000009 },
    { 0x0800006b, 0, 0, 0, 0x060000b7, 0x14000008 },
    { 0x0800006c, 0, 0, 0, 0x060000b1, 0x14000009 },
    { 0x0800006d, 0, 0, 0, 0x060000b2, 0x14000008 },
    { 0x0800006e, 0, 0, 0, 0x060000b3, 0x14000009 },
    { 0x08000060, 0, 0, 0, 0x060000b5, 0x14000008 },
    { 0, 0, 0, 0, 0, 0 },
};

signed char InsideDistrictBounds(EXVector &pos, float minX, float maxX, float maxZ, float minZ)
{
    if (minX > pos.x + 100.0f) return 0;
    if (pos.x - 100.0f > maxX) return 0;
    if (minZ > pos.z + 100.0f) return 0;
    if (pos.z - 100.0f > maxZ) return 0;
    return 1;
}

signed char PointInsideDistrict(EXVector &pos, float minX, float maxX, float maxZ, float minZ)
{
    if (minX > pos.x) return 0;
    if (pos.x > maxX) return 0;
    if (minZ > pos.z) return 0;
    if (pos.z > maxZ) return 0;
    return 1;
}

void XGameWnd::Island_Initialise()
{
    if (m_Unk400 == -1) return;
    EXGeoHeader *pHdr = pMapGeoFile->GeoHeader();
    for (_IslandDistrictState2 *p = Island_0; p->hash; p++) {
        if (pHdr->IsSectionLoaded(p->hash, 1) == 3) {
            p->state = 2;
            p->state2 = 2;
        } else {
            p->state = 0;
            p->state2 = 0;
        }
    }
    m_Unk404 = 0;
}

// NEAR: 24 bytes differ, shl+[eax+4] vs scaled [eax*8+Island_0+4] addressing
signed char XGameWnd::GetIslandSectionInfo(EXVector &pos, unsigned int &section)
{
    for (int x = 0; x < 10; x++) {
        for (int z = 0; z < 9; z++) {
            int d = Island0Map[z][x] - 1;
            if (d < 0) continue;
            float fx = (float)(x * 200 - 1000);
            float fz = (float)(600 - z * 200);
            if (PointInsideDistrict(pos, fx, fx + 200.0f, fz + 200.0f, fz)) {
                if (Island_0[d].state != 2) return 0;
                section = Island_0[d].section;
                return 1;
            }
        }
    }
    return 0;
}

// NEAR: 10 bytes differ, spill store scheduled after the loop init
void XGameWnd::UpdateIslandRadar()
{
    EXVector pos = gpPlayerItem[0]->m_position;
    int n = 0;
    for (int x = -1000; x < 1000; x += 200) {
        float fx = (float)x;
        for (int z = 600; z > -1200; z -= 200) {
            float fz = (float)z;
            if (InsideDistrictBounds(pos, fx, fx + 200.0f, fz + 200.0f, fz)) {
                if (++n == 4) return;
            }
        }
    }
}

// TODO: XGameWnd::UpdateDistrictLoading (1632 bytes) not yet written.
