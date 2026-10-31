#ifndef EXCLOTH2_H
#define EXCLOTH2_H
#include "EXItemPhysics.h"
#include "EXVector.h"
#include "EXArray.h"
#include "EXFlag.h"

class EXMatrix;
class EXGeoEntity;
class EXGDiData;
struct EXGeoEntity_VtxData;

// Second-generation cloth particle (sizeof 0x38). Flags: bit 0 fixed.
class EXCloth2Particle {
public:
    const EXVector &Position() const { return m_pos; }
    void SetPosition(const EXVector &v) { m_pos = v; }
    void SetFullPosition(const EXVector &v)
    {
        m_pos = v;
        m_oldPos = v;
    }
    const EXVector &OldPosition() const { return m_oldPos; }
    void SetOldPosition(const EXVector &v) { m_oldPos = v; }
    void AddToPosition(const EXVector &v) { m_pos += v; }
    const EXVector &Acceleration() const { return m_accel; }
    float Drag() const { return m_drag; }
    ex::EXFlag<u32> &Flags() { return m_flags; }
    void Init()
    {
        m_accel.Set(0.0f, -9.8f, 0.0f, 0.0f);
        m_flags = 0;
        m_drag = 0.0f;
    }
    void SetFixed(s8 b) { m_flags = b ? (m_flags | 1) : (m_flags & ~1); }

    EXVector m_pos;             // +0x00
    EXVector m_oldPos;          // +0x10
    EXVector m_accel;           // +0x20
    float m_drag;               // +0x30
    ex::EXFlag<u32> m_flags;    // +0x34
};

// Distance constraint between two particle indices (sizeof 0x10).
class EXCloth2Edge {
public:
    void Init()
    {
        m_lengthSq = 0.0f;
        m_ref1 = 0;
        m_ref2 = 0;
        m_rigidity = 1.0f;
    }
    u32 Ref1() const { return m_ref1; }
    void SetRef1(const u32 &r) { m_ref1 = r; }
    u32 Ref2() const { return m_ref2; }
    void SetRef2(const u32 &r) { m_ref2 = r; }
    float LengthSq() const { return m_lengthSq; }
    void SetLengthSq(const float &f) { m_lengthSq = f; }
    float Rigidity() const { return m_rigidity; }

    u32 m_ref1;                 // +0x00
    u32 m_ref2;                 // +0x04
    float m_lengthSq;           // +0x08
    float m_rigidity;           // +0x0c
};

// Per-entity vertex mapping (allocated by InitialiseEntity). TODO layout.
struct EXCloth2EntityInfo {
    u32 m_00;                   // +0x00
    void *m_pData;              // +0x04 freed with the info
};

class EXCloth2Base : public EXItemPhysics {
    EX_DECLARE_DYNAMIC(EXCloth2Base)
protected:
    virtual ~EXCloth2Base();
public:
    void Syncronise(EXGeoEntity *pEnt, EXGDiData *pGDi, const EXMatrix &m);
    EXCloth2Base()
    {
        m_50 = 1.0f / 60.0f;
        m_58 = 0;
        m_54 = 180.0f;
    }
    static EXCloth2Base *CreateObject();
protected:
    void EntityNormals(const EXGeoEntity *pEnt, EXGeoEntity_VtxData *pVerts, const EXCloth2EntityInfo *pInfo);
    void UpdatePositions(float t);
    void SatisfyConstraints(u32 n);
    void PhysicsUpdate(float t);
    void SyncroniseEntity(EXGeoEntity *pEnt, EXGDiData *pGDi, const EXCloth2EntityInfo *pInfo, const EXMatrix &m);
    s8 InitialiseEntity(const EXGeoEntity *pEnt);
    s8 PhysicsInitialise(const EXGeoEntity *pEnt);
public:
    EXDataArray<EXCloth2Particle, EXCloth2Particle &> m_particles;  // +0x10
    EXDataArray<EXVector, EXVector &> m_20;                         // +0x20 TODO
    EXDataArray<EXCloth2Edge, EXCloth2Edge &> m_edges;              // +0x30
    EXPtrArray<EXCloth2EntityInfo> m_entities;                      // +0x40
    float m_50;                 // +0x50 last step
    float m_54;                 // +0x54 iterations per second
    ex::EXFlag<u32> m_58;       // +0x58
};

class EXCloth2 : public EXCloth2Base {
    EX_DECLARE_DYNAMIC(EXCloth2)
public:
    void DoStep(float t);
    void Initialise(u8 flags);
    EXCloth2() {}
    static EXCloth2 *CreateObject();
protected:
    void GetEntity(EXGeoEntity *&pEnt, EXGDiData *&pGDi) const;
public:
    u8 m_flags;                 // +0x5c
};

// Steps every registered EXCloth2 (owned by EXItemEnv).
class EXCloth2Manager {
public:
    EXCloth2Manager() {}
    ~EXCloth2Manager() {}
    void RemoveCloth(const EXCloth2 *p);
    void DoStep(float t);
    void AddCloth(EXCloth2 *p);

    EXPtrArray<EXCloth2> m_cloths;  // +0x00
};
#endif
