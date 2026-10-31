#ifndef XNAVROUTE_H
#define XNAVROUTE_H
#include "types.h"
#include "EXVector.h"
#include "XNavLocation.h"

// A route between nav locations (NavRoute.obj, NavRoute3D.obj). sizeof 0x11c.

class XNavMesh;
class EXMatrix;
class EXQuaternion;

// Poly traversal state passed to XNavMesh::TraversePolys (NavMesh.obj), sizeof 0xb0. The walk moves
// m_Pos towards m_Target poly by poly; a wall it cannot pass is slid along once, then recorded.
class XNavRoute;
struct NavTraverse {
    NavTraverse() {}
    EXVector m_Pos;         // +0x00 current position
    EXVector m_Target;      // +0x10
    EXVector m_Start;       // +0x20 position this step started from
    EXVector m_SavedTarget; // +0x30 target while sliding along a wall
    int m_nFromNode;        // +0x40 current poly
    int m_44;               // +0x44 poly this step started in
    int m_nNode;            // +0x48 neighbour being considered (route callback)
    int m_4c;               // +0x4c neighbour chosen (-1: none)
    u32 m_dwIgnore;         // +0x50 polys with these flags are walls
    int m_nSlides;          // +0x54 wall slides so far
    int m_nDepth;           // +0x58 recursion depth (stops at 1000)
    int m_5c;               // +0x5c poly this step started in
    u32 m_dwFlags;          // +0x60 1 crossed into another poly, 2 slid along a wall (caller's flags OR'd in)
    u8 m_64;                // +0x64 TODO
    u8 m_Pad65[3];
    XNavMesh *m_pMesh;      // +0x68
    XNavRoute *m_pRoute;    // +0x6c TraversePolyCallback picks the way (0: avoid m_dwIgnore polys)
    EXVector m_BlockPos;    // +0x70 where the walk was stopped
    EXVector m_BlockE0;     // +0x80 the blocking edge
    EXVector m_BlockE1;     // +0x90
    int m_nBlockPoly;       // +0xa0
    int m_nBlockEdge;       // +0xa4
    u8 m_Unka8[4];
    s8 m_bBlocked;          // +0xac
    u8 m_Padad[3];
};
class XNavRoute {
public:
    __forceinline signed char Status() { return m_Status; }
    __forceinline void SetDynamic(signed char b) { m_Flags.m_bDynamic = b; }
    __forceinline signed char IsDynamic() { return m_Flags.m_bDynamic; }
    __forceinline void SetNavigating(signed char b) { m_Flags.m_bNavigating = (u8)b; }
    __forceinline void SetNavIgnoreFlags(unsigned int f) { m_NavIgnoreFlags = f; }
    __forceinline EXVector &MoveVector() { return m_MoveVector; }
    __forceinline signed char JumpWaiting() { return m_TriggerJump != 0; }
    __forceinline const EXVector JumpFromPos() { return m_JumpFromPos; }
    __forceinline void SetJumpFromPos(const EXVector &v) { m_JumpFromPos = v; }
    __forceinline const EXVector JumpToPos() { return m_JumpToPos; }
    __forceinline void SetJumpToPos(const EXVector &v) { m_JumpToPos = v; }
    __forceinline signed char JumpDisabled() { return m_Flags.m_bNoJump; }
    __forceinline void DisableJumping(signed char b) { m_Flags.m_bNoJump = b; }
    __forceinline signed char MovementIs3D() { return m_b3D; }
    __forceinline EXVector &MoveToPoint() { return m_MoveToPoint; }
    __forceinline signed char IsNavigating() { return m_Flags.m_bNavigating; }
    __forceinline unsigned short JumpToPoly() { return m_JumpToPoly; }
    XNavRoute();
    ~XNavRoute();
    void Init(signed char, signed char);
    int GetRouteNodes(XNavLocation *, XNavLocation *, XNavMesh *);
    signed char CalcJourneyPos(const EXVector &, const EXVector &, int &, int &, float, float, EXVector &, int &,
                               float &, int);
    signed char RouteQuickTest(XNavLocation *, XNavLocation *);
    void InitJump(int, const EXVector &, int, const EXVector &, float);
    void EndJump();
    int TraversePolyCallback(NavTraverse &);
    void RecoverFromBrokenJump(XNavLocation &);
    signed char Collide(XNavLocation *, EXVector &);
    EXVector RandomPosition(XNavLocation *);
    static signed char ClampToPortal(const EXVector &, EXVector &, EXVector &, EXVector &, EXVector *, signed char,
                                     float);
    float GetFloorHeight(XNavMesh *, const EXVector &, int, int);
    signed char RouteQuickTest(XNavLocation *, const EXVector &, int);
    int CalcRouteEx(XNavLocation *, XNavLocation *);
    signed char CalcJumpToPos(XNavMesh *, const EXVector &, unsigned short *, const EXVector &, EXVector &);
    static signed char FloorInfo(const EXVector &, const EXVector &, EXVector &, EXVector &, int *, EXMatrix *,
                                 EXQuaternion *);
    int Update(XNavLocation *, XNavLocation *, float);
    signed char GetFleePosition(const EXVector &, const EXVector &, float, EXVector &);
    EXVector &CalcRoute(XNavLocation *, XNavLocation *);
    void LookAt2(const EXVector &, const EXVector &, EXMatrix &);
    signed char StandOnSurface(const EXVector &, XNavLocation *, const EXVector &, EXVector &, EXMatrix *,
                               EXQuaternion *);
    int CalcRoute3DEx(XNavLocation *, XNavLocation *);
    EXVector &CalcRoute3D(XNavLocation *, XNavLocation *);
    __forceinline void SetJumping() { m_Flags.m_bJumpCancel = 0; m_Flags.m_b7 = 1; }
    __forceinline void ClrJumpTrigger() { m_Flags.m_bTriggerJump = 0; }
    __forceinline void SetJumpToPoly(unsigned short p) { m_JumpToPoly = p; }
    signed char NoJump() { return m_Flags.m_bNoJump; }  // TODO real name (Actor.h)

    EXVector m_v00;             // 0x00
    EXVector m_MoveToPoint;     // 0x10
    EXVector m_v20;             // 0x20
    EXVector m_MoveVector;      // 0x30
    EXVector m_v40;             // 0x40
    EXVector m_v50;             // 0x50
    EXVector m_v60;             // 0x60
    EXVector m_v70;             // 0x70
    EXVector m_v80;             // 0x80
    EXVector m_v90;             // 0x90
    EXVector m_JumpFromPos;     // 0xa0
    EXVector m_JumpToPos;       // 0xb0
    s8 m_bInit;                 // 0xc0
    s8 m_c1;                    // 0xc1
    s8 m_Status;                // 0xc2
    s8 m_b3D;                   // 0xc3
    int m_c4;                   // 0xc4
    u32 m_NavIgnoreFlags;       // 0xc8
    float m_fcc;                // 0xcc
    int m_d0;                   // 0xd0
    int m_d4;                   // 0xd4
    int m_d8;                   // 0xd8
    int m_dc;                   // 0xdc
    int m_e0;                   // 0xe0
    int m_e4;                   // 0xe4
    s8 m_e8;                    // 0xe8
    u16 m_JumpFromNode;         // 0xea
    u16 m_JumpNode;             // 0xec
    u16 m_JumpToPoly;           // 0xee
    float m_fJumpTime;          // 0xf0
    int m_nNodes;               // 0xf4
    u16 m_Nodes[6];             // 0xf8
    u16 m_Links[6];             // 0x104
    struct {
        u32 m_bNavigating : 1;  // bit 0
        u32 m_bCollided : 1;    // bit 1
        u32 m_b2 : 1;           // bit 2
        u32 m_b3 : 1;
        u32 m_bDynamic : 1;     // bit 4
        u32 m_b5 : 1;
        u32 m_bNoJump : 1;      // bit 6 (Actor::DisableJumping)
        u32 m_b7 : 1;           // bit 7 jumping
        u32 m_bTriggerJump : 1; // bit 8 (Actor::InitTriggerJump)
        u32 m_bJumpCancel : 1;  // bit 9
        u32 m_b10 : 22;
    } m_Flags;                  // 0x110
    int m_TriggerJump;          // 0x114
    struct {
        u32 m_b0 : 1;
        u32 m_b1 : 1;
        u32 m_b2 : 30;
    } m_Flags2;                 // 0x118
};
#endif
