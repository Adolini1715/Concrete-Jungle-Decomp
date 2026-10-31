#ifndef EXBASESWOOSHSYS_H
#define EXBASESWOOSHSYS_H
#include "EXEffectsSys.h"
#include "EXVector.h"
#include "RGBA.h"

class EXItem;
class EXItemAnimator_Entity;
class EXSwooshSysMgr;

// One trail vertex per segment and hash (32 bytes; rows of m_numSegments per hash).
struct SwooshVTX_Data {
    EXVector m_pos;
    float m_u;          // +0x10
    float m_v;          // +0x14
    float m_18;
    ex::RGBA m_colour;  // +0x1c
};

// Trail effect attached to an item or entity (EXSwoosh.obj), sizeof 0xcc.
// Vertex ring buffer: m_numSegments rows of m_numHashs EXVectors, m_curSegment is the write row.
class EXBaseSwooshSys : public EXEffectsSys {
    EX_DECLARE_DYNAMIC(EXBaseSwooshSys)
    friend class EXSwooshSysMgr;

public:
    virtual ~EXBaseSwooshSys();
    EXBaseSwooshSys();
    static EXBaseSwooshSys *CreateObject();
    void SetSwooshSysMgr(EXSwooshSysMgr *p) { m_pMgr = (EXEffectsSysMgr *)p; }
    u32 NumSegments() const { return m_numSegments; }
    void SetNumSegments(u32 n) { m_numSegments = n; }
    u32 NumHashs() const { return m_numHashs; }
    void SetNumHashs(u32 n) { m_numHashs = n; }
    void SetSwooshAlpha(float a) { m_alpha = a; }
    void SetManualUpdate(s8 b) { m_bManualUpdate = b; }
    void SetTessalationLevel(u32 level);
    void SetVertexPosition(const EXVector &pos, u32 hash);
    s8 Finished() const;
    void KillSwoosh();
    u32 GetHashEntry(u32 i);
    s8 Create(EXGeoFile *pFile, u32 hash, int datum, s8 loop, int segments);
    s8 UpdateSwoosh();
    s8 Initialize(EXGeoFile *pFile, u32 hash, EXItem *pItem, EXItemAnimator_Entity *pEntity, int datum, s8 loop, int segments);
    s8 Update(const EXSwooshSysMgr *pMgr);
protected:
    void Detatch();
    void CalculateSwoosh();
public:
    u32 m_hash;                 // +0x54
    EXItem *m_pItem;            // +0x58
    EXItemAnimator_Entity *m_pEntity;   // +0x5c
    float m_life;               // +0x60
    float m_ageRate;            // +0x64 percent per frame
    u32 m_numSegments;          // +0x68
    float m_6c;                 // +0x6c live segment count
    float m_70;                 // +0x70
    u32 m_numHashs;             // +0x74
    u32 m_curSegment;           // +0x78
    SwooshVTX_Data **m_pRows;   // +0x7c per-hash rows
    EXVector *m_pVerts;         // +0x80
    EXVector *m_pVerts2;        // +0x84
    void *m_pMem;               // +0x88
    float m_age;                // +0x8c 0..100
    int m_datum;                // +0x90
    u32 m_tessalation;          // +0x94
    s8 m_bManualUpdate;         // +0x98
    s8 m_bKill;                 // +0x99
    s8 m_bLoop;                 // +0x9a
    s8 m_bDirty;                // +0x9b
    s8 m_bDetached;             // +0x9c
    u8 m_9d[3];
    u32 m_id;                   // +0xa0
    u32 m_controllers;          // +0xa4 1 taper, 2 alpha, 4 colour
    float m_alpha;              // +0xa8
    float m_endR, m_endG, m_endB;       // +0xac
    float m_endAlpha;           // +0xb8
    float m_startR, m_startG, m_startB; // +0xbc
    float m_startAlpha;         // +0xc8
};
#endif
