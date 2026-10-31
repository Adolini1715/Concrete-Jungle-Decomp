#ifndef FERADAR_H
#define FERADAR_H
#include "FEWnd.h"
#include "FEHud.h"
#include "EXGeoTrigger.h"

// Bounds of the radar map of the current level (FERadar.obj); sizeof 0x48. TODO: m_Datum type
// (EXGeoTriggerDatum).
struct RadarMap {
    RadarMap()
    {
        m_Valid = 0;
        m_Hash = 0;
        m_W = 0;
        m_H = 0;
        m_X = 0;
        m_Y = 0;
    }
    ~RadarMap() {}
    signed char m_Valid;  // 0x00
    unsigned int m_Hash;  // 0x04
    int m_W;              // 0x08
    int m_H;              // 0x0c
    int m_X;              // 0x10
    int m_Y;              // 0x14
    EXGeoBaseDatum m_Datum;  // 0x18 radar volume
    RadarMap(unsigned int hash, EXGeoTriggerDatum d);
    RadarMap &operator=(const RadarMap &m)
    {
        m_Valid = m.m_Valid;
        m_Hash = m.m_Hash;
        m_W = m.m_W;
        m_H = m.m_H;
        m_X = m.m_X;
        m_Y = m.m_Y;
        m_Datum = m.m_Datum;
        return *this;
    }
};

// One thing drawn on the radar (FERadar.obj); sizeof 0x2c.
struct FERadarItem {
    FERadarItem();
    EXVector m_Pos;         // 0x00
    signed char m_Unk10;    // 0x10 =1
    signed char m_Unk11;    // 0x11
    u8 m_Unk12[2];
    ex::RGBA m_Colour;      // 0x14
    unsigned int m_Sprite;  // 0x18
    int m_Unk1c;            // 0x1c
    int m_Unk20;            // 0x20
    int m_Unk24;            // 0x24
    EXString m_Text;        // 0x28
};

// Radar window (FERadar.obj). Singleton, sizeof 0x434.
class FERadar : public FEWnd {
public:
    virtual ~FERadar() { m_pInstance = 0; }
    virtual signed char Draw();
    static FERadar *Instance(signed char create);
    static void DrawHints(FERadarInfo &info);
    static void DrawActors(FERadarInfo &info);
    static void DrawPoly2D(FERadarInfo &info, EXVector offset, EXVector centre, EXVector size, unsigned int file, unsigned int tex,
                           int blend, ex::RGBA col, float angle);
    static void DrawObjectives(FERadarInfo &info);
    static void DrawSpecialObjects(FERadarInfo &info);
    static void DrawPredator(FERadarInfo &info);
    static signed char DrawItemOnRadar(FERadarItem &item, FERadarInfo &info, FERenderPass::Flags pass);

    FERadarInfo m_Info;  // 0x3fc
    float m_Unk428;      // 0x428
    float m_Unk42c;      // 0x42c
    float m_Unk430;      // 0x430

private:
    FERadar();
    static FERadar *m_pInstance;
};
inline FERadar *FEGetRadar(signed char create) { return FERadar::Instance(create); }  // [fi] App.obj
#endif
