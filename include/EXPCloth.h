#ifndef EXPCLOTH_H
#define EXPCLOTH_H
#include "EXItemPhysics.h"
#include "EXVector.h"
#include "EXArray.h"
#include "EXFlag.h"
#include "EXMatrix.h"

class EXQuaternion;
class EXWnd;
class EXGeoEntity;
class EXGeoBaseEntity;
class EXGeoSplitEntity;
class EXGDiData;
class EXCollisionType;
struct EXGeoEntity_VtxData;
namespace ex { class RGBA; }

// Verlet particle (sizeof 0x3c). Flags: bit 0 fixed, bit 1 grabbed.
class EXPParticle {
public:
    EXPParticle();
    void SetPosition(const EXVector &v)
    {
        m_pos = v;
        m_oldPos = v;
    }
    void SetCurrentPosition(const EXVector &v) { m_pos = v; }
    void SetOldPosition(const EXVector &v) { m_oldPos = v; }
    void SetAcceleration(const EXVector &v) { m_accel = v; }
    void AddToPositionAndOld(const EXVector &v)
    {
        m_pos += v;
        m_oldPos += v;
    }
    void SetDrag(float f) { m_drag = f; }
    ex::EXFlag<u32> &Flags() { return m_flags; }
    const EXVector &GetPosition() const { return m_pos; }
    const EXVector &GetOldPosition() const { return m_oldPos; }
    const EXVector &GetAcceleration() const { return m_accel; }
    float GetDrag() const { return m_drag; }
    EXVector &GetPositionRef() { return m_pos; }
    void SetFixed(s8 b) { m_flags = b ? (m_flags | 1) : (m_flags & ~1); }
    void SetGrabbed(s8 b) { m_flags = b ? (m_flags | 3) : (m_flags & ~3); }
    void ApplyXForm(const EXMatrix &m, s8 bOld)
    {
        m_pos.ApplyXForm(m);
        if (bOld) m_oldPos.ApplyXForm(m);
    }
    void QuickUpdatePosition();
    void VariableUpdatePosition(float t);
    float CheckCollision(const EXVector &p, const EXVector &n, float d, float f);
    void Reset();
    void KeepAwayFrom(EXCollisionType &c, s8 bOld);

    EXVector m_pos;             // +0x00
    EXVector m_oldPos;          // +0x10
    EXVector m_accel;           // +0x20
    float m_drag;               // +0x30
    float m_34;                 // +0x34
    ex::EXFlag<u32> m_flags;    // +0x38
};

// Distance constraint between two particles (sizeof 0x10).
class EXPClothEdge {
public:
    EXPClothEdge();
    void SetRigidity(float f) { m_rigidity = f; }
    void SetLength(float f) { m_lengthSq = f * f; }
    void SetLengthSq(float f) { m_lengthSq = f; }
    float Length() { return EXSqrt(m_lengthSq); }
    void SetParticles(EXPParticle *a, EXPParticle *b);
    void Draw(EXWnd *pWnd, ex::RGBA col, const EXVector &v) const;
    EXVector GetVector() const;
    void Draw(EXWnd *pWnd, const EXMatrix &m) const;
    void SatisfyConstraints();

    EXPParticle *m_p1;          // +0x00
    EXPParticle *m_p2;          // +0x04
    float m_lengthSq;           // +0x08
    float m_rigidity;           // +0x0c
};

// Edge whose rest length may vary between a min and a max (sizeof 0x20).
class EXPVarClothEdge : public EXPClothEdge {
public:
    EXPVarClothEdge()
    {
        m_minSq = 0.0f;
        m_maxSq = 0.0f;
        m_bActive = 1;
    }
    void Activate(s8 b) { m_bActive = b; }
    u32 Hash() const { return m_hash; }
    void Set(float fMin, float fMax, float fRigidity, u32 hash);
    void Debug_DrawConstraint(const EXVector *pPos, const EXQuaternion *pRot) const;
    void SatisfyConstraints();

    float m_maxSq;              // +0x10
    float m_minSq;              // +0x14
    u32 m_hash;                 // +0x18
    s8 m_bActive;               // +0x1c
};

// Plane constraint holding a particle on the positive side (sizeof 0x18).
class EXPClothCollisionConstraint {
public:
    EXPClothCollisionConstraint() { m_pParticle = 0; }
    void Set(const EXVector &n, EXPParticle *p, float d)
    {
        m_normal = n;
        m_pParticle = p;
        m_d = d;
    }
    EXPClothCollisionConstraint &operator=(const EXPClothCollisionConstraint &c)
    {
        m_normal = c.m_normal;
        m_pParticle = c.m_pParticle;
        m_d = c.m_d;
        return *this;
    }
    void SatisfyConstraints();

    EXVector m_normal;              // +0x00
    EXPParticle *m_pParticle;       // +0x10
    float m_d;                      // +0x14
};

// Verlet cloth driven from a geo entity (sizeof 0xd8).
class EXPCloth : public EXItemPhysicsExt {
public:
    virtual s8 DoUpdate();
    virtual s8 Initialise();
    EX_DECLARE_DYNAMIC(EXPCloth)
public:
    virtual ~EXPCloth();
    virtual void DoCollision(EXItemPhysics *pOther, EXCScoop_FaceData *pData);
    virtual void SyncWithVerts(EXGeoBaseEntity *pEnt, EXGDiData *pGDi);
    virtual void SetWind(EXVector v);
    void SetIterations(u32 n) { m_iterations = n; }
    void SetGravCoef(float f) { m_bc = f * 0.002725f; }  // appended ([fi] in TestCloth.obj)
    ex::EXFlag<u32> &ClothFlags() { return m_clothFlags; }
    void MoveGrabbedVert(const EXVector &v);
    void UpdateGrabbedVert(const EXVector &v, u32 i);
    void SetDrag(float f);
    void SetElasticity(float f);
    void SetElasticity(float f, u32 i);
    void ActivateExtraEdges(u32 hash);
    void DeactivateExtraEdges(u32 hash);
    void PauseSync();
    void Grab(const EXVector &v);
    void ReleaseGrabbedVert();
    void GrabVerts(u32 n, const u32 *p);
    void ReleaseGrabbedVerts(u32 n, const u32 *p);
    static void RenderModifier_Callback(u32 a, void *p);
    s8 SplitEntityInitialise(const EXGeoSplitEntity *pEnt, u8 flags);
    s8 Initialise(const EXGeoEntity *pEnt, u8 flags);
    s8 ClothInit(u8 flags);
    void DoCollision(EXGeoEntity *pEnt, EXCScoop_FaceData *pData);
    void ResetClothLengths(const EXGeoEntity *pEnt, float f);
    void DoClothCollision(EXCollisionType *pCol, u32 n);
    EXPCloth();
    static EXPCloth *CreateObject();
    void FindNormals(const EXGeoEntity *pEnt, EXGeoEntity_VtxData *pVerts);
protected:
    float ScoreEdges(int a, int b, const EXVector &v) const;
    s8 VerletUpdate(float t);
    s8 InitialiseExtraEdges(const EXGeoEntity *pEnt);
    void DoClothCollision(EXItem *pItem, EXCollisionType *pCol, u32 n, EXGeoBaseEntity *pEnt, EXGDiData *pGDi);
public:
    EXVector m_wind;                // +0x44 normalised wind
    EXVector m_windRaw;             // +0x54
    EXPParticle *m_pParticles;      // +0x64
    EXPClothEdge *m_pEdges;         // +0x68
    void *m_6c;                     // +0x6c
    EXDataArray<EXPVarClothEdge, EXPVarClothEdge &> m_varEdges;                     // +0x70
    EXDataArray<EXPClothCollisionConstraint, EXPClothCollisionConstraint &> m_collisions;  // +0x80
    u32 m_90;                       // +0x90
    void *m_94;                     // +0x94
    void *m_98;                     // +0x98
    u32 m_9c;                       // +0x9c
    u32 m_numParticles;             // +0xa0
    u32 m_numEdges;                 // +0xa4
    u32 m_iterations;               // +0xa8
    int m_grabbed;                  // +0xac
    u32 m_b0;                       // +0xb0
    u32 m_b4;                       // +0xb4
    u32 m_b8;                       // +0xb8
    float m_bc;                     // +0xbc
    float m_c0;                     // +0xc0
    float m_c4;                     // +0xc4
    float m_c8;                     // +0xc8
    float m_cc;                     // +0xcc
    ex::EXFlag<u32> m_clothFlags;   // +0xd0
    s8 m_d4;                        // +0xd4
    s8 m_bOwnArrays;                // +0xd5
    s8 m_d6;                        // +0xd6
};

// Cloth attached to its item in world space (sizeof 0x12c).
class EXPAttachedClothWS : public EXPCloth {
public:
    virtual s8 DoUpdate();
    virtual s8 Initialise();
    EX_DECLARE_DYNAMIC(EXPAttachedClothWS)
public:
    virtual void DoCollision(EXItemPhysics *pOther, EXCScoop_FaceData *pData);
    virtual void SyncWithVerts(EXGeoBaseEntity *pEnt, EXGDiData *pGDi);
    virtual void SetWind(EXVector v);
    void GetEntity(EXGeoBaseEntity *&pEnt, EXGDiData *&pGDi);
    s8 Initialise(u32 hash, u8 flags);
    void Syncronise();
    void DoClothCollision(EXCollisionType *pCol, u32 n);
    void PauseSync();
    EXPAttachedClothWS();
    static EXPAttachedClothWS *CreateObject();

    EXMatrix m_invMtx;              // +0xd8 inverse of the attachment frame at initialisation
    u32 m_118;                      // +0x118 item owning the attachment bone (0: the cloth's own)
    u32 m_11c;                      // +0x11c item whose entity the cloth uses
    u32 m_120;                      // +0x120 attachment datum hash
    u32 m_124;                      // +0x124
    float m_128;                    // +0x128
};
#endif
