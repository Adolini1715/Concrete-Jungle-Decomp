#include "XNavMeshHandler.h"
#include "XNavLocation.h"
#include "XNavNetwork.h"
#include "EXMemory.h"
#include "FEText.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXPlane.h"

#include "AIControl.h"

s8 XNavMeshHandler::PostLoadInit()
{
    for (int i = 0; i < m_nMeshes; i++) m_pMeshes[i].SetMeshLighting();
    return 1;
}

// NEAR: clamp/compare block layout differs (retail keeps clamped values in xmm regs).
int XNavMeshHandler::GridSectorForPos(const EXVector &p)
{
    float x = p.x;
    float z = p.z;
    if (-600.0f > x) x = -600.0f;
    else if (x > 600.0f) x = 600.0f;
    if (-200.0f > z) z = -200.0f;
    else if (z > 200.0f) z = 200.0f;
    if (-600.0f > x || x > 600.0f || -200.0f > z || z > 200.0f) return -1;
    int ix = (int)(x * 0.005f) + (0.0f > x ? -1 : 0);
    int iz = (int)(z * 0.005f) + (0.0f > z ? -1 : 0);
    ix += 3;
    iz += 1;
    if (ix < 0) ix = 0;
    else if (ix > 5) ix = 5;
    if (iz < 0) iz = 0;
    else if (iz > 1) iz = 1;
    return iz * 6 + ix;
}

s8 XNavMeshHandler::CalcAStar(XNavLocation &a, XNavLocation &b, u32 flags)
{
    XNavMesh *m = a.GetMesh();
    if (!m) return 0;
    return m->NavNetwork()->CalcAStar(a.GetPolyID(), b.GetPolyID(), flags);
}

XNavLocation::~XNavLocation() {}

void XNavLocation::Reset()
{
    m_v00.SetZero();
    m_bValid = 0;
    m_vPos.SetZero();
    m_v20.SetZero();
    m_vFloorPos.SetZero();
    m_nMesh = 0;
    m_nPolyHandle = 0;
    m_vFloorNormal.Set(0.0f, 1.0f, 0.0f, 0.0f);
    m_wSurfaceFlags = 0;
    m_fMeshDistance = 0.0f;
    m_v50.SetZero();
}

static __forceinline float AnglePolar(float a)
{
    if (a >= 0.0f && a < 6.2831855f) return a;
    float r = (float)fmod(a, 6.2831855f);
    if (r >= 0.0f) return r;
    return r + 6.2831855f;
}

u16 XNavLocation::CalcBlockBits(const EXVector &d)
{
    float a = AnglePolar((float)atan2(d.x, d.z));
    int n = (int)(a * 2.546479f + 0.19634955f);
    if (n >= 16) n = 0;
    return 1 << n;
}

void XNavLocation::FloodPolyFlags(u16 a, u16 b, u16 c)
{
    if (IsValid()) {
        XNavMesh *m = GetMesh();
        if (m) m->FloodPolyFlags(GetPolyID(), a, b, c);
    }
}

s8 XNavMeshHandler::Init()
{
    m_b0 = 0;
    m_bDynamic = 0;
    m_nType = 0;
    m_nGeoFiles = 0;
    memset(m_GeoFiles, 0, sizeof(m_GeoFiles));
    m_nMeshes = 0;
    m_pMeshes = 0;
    m_Entities.Empty(1);
    m_Entities3D.Empty(1);
    m_pMaster = 0;
    XNavGridSector *pSector = m_Sectors;
    for (int i = 12; i; i--, pSector++) {
        pSector->m_pSubMap = 0;
        pSector->m_pMesh = 0;
    }
    m_wZones = 0;
    m_wSubMaps = 0;
    m_pSubMaps = 0;
    m_130 = 0;
    m_134 = 0;
    return 1;
}

s8 XNavMeshHandler::BootInit() { return Init(); }

s8 XNavMeshHandler::PreLoadInit()
{
    Init();
    m_nCmds = 0;
    if (m_pCmds) {
        EXFree(m_pCmds);
        m_pCmds = 0;
    }
    return 1;
}

s8 XNavMeshHandler::UnloadLevel()
{
    for (int i = 0; i < m_nMeshes; i++) m_pMeshes[i].CleanUp();
    m_nMeshes = 0;
    if (m_pMeshes) {
        delete[] m_pMeshes;
        m_pMeshes = 0;
    }
    return 1;
}

// NEAR: retail unrolls the sector loop 6x2 with pointer stepping; array loops differ.
s8 XNavMeshHandler::UpdatePtrs(XGeoNavMeshEntity *pOld, XGeoNavMeshEntity *pNew)
{
    pNew->UpdatePtrs();
    if (m_pMaster == pOld) m_pMaster = pNew;
    if (pNew->Is3D()) {
        for (int i = 0; i < m_Entities3D.m_nSize; i++)
            if (m_Entities3D[i] == pOld) m_Entities3D[i] = pNew;
    } else {
        for (int i = 0; i < m_Entities.m_nSize; i++)
            if (m_Entities[i] == pOld) m_Entities[i] = pNew;
    }
    for (int i = 0; i < 12; i++) {
        if (m_Sectors[i].m_pMesh == pOld) {
            m_Sectors[i].m_pMesh = pNew;
            pNew->SetSectorPtr(&m_Sectors[i]);
        }
    }
    for (int i = 0; i < m_nMeshes; i++) m_pMeshes[i].UpdatePtrs(pOld, pNew);
    return 1;
}

void XNavMeshHandler::RefreshMaps()
{
    if (!m_pMaster) return;
    int n = m_Entities.m_nSize;
    for (int i = 0; i < n; i++) {
        XGeoNavMeshEntity *e = m_Entities[i];
        if (!e) continue;
        if (e->IsMaster()) continue;
        if (e->HasData()) continue;
        XNavNetwork *mnet = m_pMaster->NavNetwork();
        XNavNetwork *enet = e->NavNetwork();
        for (int j = 0; j < m_wSubMaps; j++) {
            if (e->HashCode() == m_pSubMaps[j].HashCode()) {
                enet->SetNavData(mnet->NavData());
                enet->SetNavNodeData(mnet->NavNodeData());
                enet->SetNavRanges(mnet->NavRanges());
                enet->SetNavRangesCount(mnet->NavRangesCount());
                if (!m_pSubMaps[j].IsInit()) {
                    enet->NodeOffset() = e->NodeOffset() = m_pSubMaps[j].NodeOffset();
                    enet->LinkOffset() = e->LinkOffset() = m_pSubMaps[j].LinkOffset();
                    e->PolyOffset() = m_pSubMaps[j].PolyOffset();
                    e->VertOffset() = m_pSubMaps[j].VertOffset();
                    enet->DoSubMapOffsets(e->PolyOffset(), e->VertOffset());
                    m_pMeshes->AddNavMesh(e, 0, 0);
                    m_pSubMaps[j].SetInit(1);
                }
                break;
            }
        }
    }
}

void XNavMeshHandler::RemoveNavMesh(XGeoNavMeshEntity *pMesh)
{
    if (!pMesh) return;
    gAIControl.NavMeshDeleting(pMesh);
    if (pMesh->GridSector() < 12) m_Sectors[pMesh->GridSector()].SetMesh(0);
    for (int i = 0; i < m_nMeshes; i++) m_pMeshes[i].DelNavMesh(pMesh);
    if (pMesh->Is3D()) {
        for (int i = 0; i < m_Entities3D.m_nSize; i++) {
            if (pMesh == m_Entities3D.GetAt(i)) {
                m_Entities3D.RemoveAt(i);
                i--;
            }
        }
    } else {
        for (int i = 0; i < m_Entities.m_nSize; i++) {
            if (pMesh == m_Entities.GetAt(i)) {
                m_Entities.RemoveAt(i);
                i--;
            }
        }
    }
}

XNavMeshCmd *XNavMeshHandler::NewCmd()
{
    if (!m_nCmds) {
        m_pCmds = new XNavMeshCmd[20];
        if (!m_pCmds) return 0;
        m_nCmds = 20;
        memset(m_pCmds, 0, 20 * sizeof(XNavMeshCmd));
    }
    int n = m_nCmds;
    int i;
    for (i = 0; i < m_nCmds; i++)
        if (!m_pCmds[i].m_wType) break;
    if (i == n) {
        m_pCmds = (XNavMeshCmd *)_EXRealloc(m_pCmds, (n + 20) * sizeof(XNavMeshCmd));
        if (!m_pCmds) return 0;
        m_nCmds += 20;
    }
    XNavMeshCmd *c = &m_pCmds[i];
    memset(c, 0, sizeof(XNavMeshCmd));
    c->m_wIndex = (u16)i;
    return c;
}

XNavLocation::XNavLocation()
{
    Reset();
    m_dwIgnore = 8;
    m_b3D = 0;
    m_b2 = 0;
    m_60 = 0;
    m_wGeoBlockBits = 0;
    m_wObjBlockBits = 0;
    m_fPushRadius = 1.0f;
    m_70 = 0.0f;
}

XNavLocation::XNavLocation(EXVector p, int poly, int tri)
{
    m_b3D = 0;
    m_b2 = 0;
    m_60 = 0;
    Reset();
    XNavMesh *m = gNavMeshHandler.GetPtr(p, 0);
    m_dwIgnore = 0;
    m_vPos = p;
    SetPoly(m, poly, tri);
}

// NEAR: GetPtr/GetMesh comparison and PolyFlags inline shape differ.
s8 XNavLocation::SelectPolyByRaycast(EXVector &dir)
{
    XNavMesh *m = gNavMeshHandler.GetPtr(m_vPos, m_b3D);
    int poly = -1;
    if (m != GetMesh()) m_nMesh = m->Handle();
    else poly = GetPolyID();
    m->LocatePolygon3DRay(m_vPos, dir, &poly, 0, 0, 0);
    if (poly < 0) return 0;
    if (m_dwIgnore & m->PolyFlags(poly)) return 0;
    SetPoly(m, poly, -1);
    UpdateFloorInfo(m, poly);
    m_bValid = 1;
    return 1;
}

// NEAR: pos/in-pos copy scheduling differs.
void XNavLocation::SetPosPoly(const EXVector &p, int h, s8 b)
{
    if (h == -1) {
        SetPosSlow(p, b);
        return;
    }
    m_vPos = p;
    m_v00 = p;
    XNavMesh *m = gNavMeshHandler.GetPtr(m_vPos, m_b3D);
    if (m) m_nMesh = m->Handle();
    int poly = XNavMeshHandler::PolyID(h);
    if (m->PointInPolygon(p, poly)) {
        SetPoly(m, poly, -1);
        UpdateFloorInfo(m, poly);
        m_bValid = 1;
        return;
    }
    SetPosSlow(p, b);
}

XNavMeshHandler::~XNavMeshHandler()
{
    UnloadLevel();
    m_nCmds = 0;
    if (m_pCmds) {
        EXFree(m_pCmds);
        m_pCmds = 0;
    }
}

XGeoNavMeshEntity *XNavMeshHandler::Get(int i, int b3D)
{
    if (!b3D) {
        if (i >= m_Entities.m_nSize) return 0;
        return m_Entities.GetAt(i);
    }
    if (i >= m_Entities3D.m_nSize) return 0;
    return m_Entities3D.GetAt(i);
}

int XNavMeshHandler::SetUpRoute(XNavLocation &a, XNavLocation &b, float &cost, s8 flag)
{
    XGeoNavMeshEntity *e = Get(0, 0);
    if (!e) return -1;
    return e->NavNetwork()->SetupRoute(a.GetPolyID(), b.GetPolyID(), cost, flag);
}

s8 XNavMeshHandler::GetRoutePos(XNavLocation &a, XNavLocation &b, float dist, EXVector &pos, int &node)
{
    XGeoNavMeshEntity *e = Get(0, 0);
    if (!e) return 0;
    return e->NavNetwork()->GetRoutePos(a.GetPolyID(), b.GetPolyID(), dist, pos, node);
}

s8 XNavMeshHandler::DoCmd_SetBlocked(const EXVector &p)
{
    XNavLocation loc;
    loc.m_dwIgnore = 0;
    loc.SetPosSlow(p, 0);
    if (loc.IsValid()) {
        loc.FloodPolyFlags(0x1000, 8, 8);
        return 1;
    }
    return 0;
}

s8 XNavMeshHandler::DoCmd_SetOpen(const EXVector &p)
{
    XNavLocation loc;
    loc.m_dwIgnore = 0;
    loc.SetPosSlow(p, 0);
    if (loc.IsValid()) {
        loc.FloodPolyFlags(0x1000, 8, 0);
        return 1;
    }
    return 0;
}

s8 XNavMeshHandler::SetBlocked(const EXVector &p)
{
    if (DoCmd_SetBlocked(p)) return 1;
    XNavMeshCmd *c = NewCmd();
    c->m_wType = 1;
    c->m_vPos = p;
    return 0;
}

s8 XNavMeshHandler::SetOpen(const EXVector &p)
{
    if (DoCmd_SetOpen(p)) return 1;
    XNavMeshCmd *c = NewCmd();
    c->m_wType = 2;
    c->m_vPos = p;
    return 0;
}

void XNavMeshHandler::UpdateCmds()
{
    XNavMeshCmd *c = m_pCmds;
    for (int i = 0; i < m_nCmds; i++, c++) {
        switch (c->m_wType) {
        case 1:
            if (DoCmd_SetBlocked(c->m_vPos)) c->m_wType = 0;
            break;
        case 2:
            if (DoCmd_SetOpen(c->m_vPos)) c->m_wType = 0;
            break;
        }
    }
}

s8 XNavMeshHandler::Update()
{
    RefreshMaps();
    UpdateCmds();
    return 1;
}

XNavMeshHandler::XNavMeshHandler()
{
    m_nCmds = 0;
    m_pCmds = 0;
    Init();
}

XNavMeshHandler gNavMeshHandler;

// Memory was compacted: re-find each loaded nav mesh entity and repoint everything at it.
// NEAR: retail searches the entity list with the loop index as the hash.
s8 XNavMeshHandler::HandleMemCompaction()
{
    for (int f = 0; f < m_nGeoFiles; f++) {
        EXGeoFile *pFile = EXGeoFile::LoadGeoFile(m_GeoFiles[f].m_dwHashCode, 0, 0);
        if (!pFile) continue;
        EXGeoHeader *pHdr = pFile->GeoHeader();
        if (!pHdr) continue;
        const EXGeoCommonArray<EXGeoEntityHeader> &list = pHdr->EntityList();
        int n = list.Size();
        for (int i = 0; i < n; i++) {
            EXGeoEntityHeader *pEntHdr = list.Find(i);
            if (!pEntHdr) continue;
            EXGeoEntity *pEnt = pEntHdr->Object(0);
            if (!pEnt) continue;
            if (pEnt->IsKindOf(EXRUNTIME_CLASS(XGeoNavMeshEntity))) {
                UpdatePtrs(m_GeoFiles[f].m_pEntity, (XGeoNavMeshEntity *)pEnt);
                m_GeoFiles[f].m_pEntity = (XGeoNavMeshEntity *)pEnt;
            }
        }
    }
    return 1;
}

// Direction masks for the 16 headings: bits within +-2 sectors of a heading, and the
// opposite arc.
static const u16 s_DirNear[16] = {0x3c, 0x78, 0xf0, 0x1e0, 0x3c0, 0x780, 0xf00, 0x1e00,
                                  0x3c00, 0x7800, 0xf000, 0xe001, 0xc003, 0x8007, 0xf, 0x1e};
static const u16 s_DirFar[16] = {0x7800, 0xf000, 0xe001, 0xc003, 0x8007, 0xf, 0x1e, 0x3c,
                                 0x78, 0xf0, 0x1e0, 0x3c0, 0x780, 0xf00, 0x1e00, 0x3c00};

static __forceinline u16 Rotl16(u16 v) { return (u16)(v << 1) ? (u16)(v << 1) : 1; }
static __forceinline u16 Rotr16(u16 v) { return (u16)(v >> 1) ? (u16)(v >> 1) : 0x8000; }

// Can the location head in direction d? 1: yes, -1: blocked, 0: need to steer round.
// "ignore" is a mask of block bits to disregard (byte swapped by retail).
int XNavLocation::NegotiateDir(const EXVector &d, u16 ignore)
{
    float a = AnglePolar((float)atan2(-d.x, -d.z));
    int n = (int)(a * 2.546479f + 0.19634955f);
    if (n >= 16) n = 0;
    u16 geo = m_wGeoBlockBits;
    int bNear = (s_DirNear[n] & geo) != 0;
    if ((s_DirFar[n] & geo) == 0) {
        if (bNear) return 1;
    } else if (!bNear) {
        return -1;
    }
    u16 all = (m_wObjBlockBits | geo) & ~(u16)((ignore << 8) | (ignore >> 8));
    bNear = (s_DirNear[n] & all) != 0;
    if ((s_DirFar[n] & all) == 0) return bNear;
    if (!bNear) return -1;
    u16 left = 1 << n;
    u16 right = left;
    for (int k = 0; k < 2; k++) {
        left = Rotl16(left);
        right = Rotr16(right);
    }
    for (int i = 0; i < 4; i++) {
        if (right & all) return -1;
        if (left & all) return 1;
        left = Rotl16(left);
        right = Rotr16(right);
    }
    return 0;
}

// Switch the location between the ground and 3D nav meshes and re-find its polygon.
void XNavLocation::Set3D(s8 b3D)
{
    m_b3D = b3D;
    if (!m_bValid || m_b3D != m_b2) {
        m_bValid = 0;
        m_b2 = m_b3D;
        XNavMesh *pMesh = gNavMeshHandler.GetPtr(m_vPos, m_b3D);
        if (pMesh) {
            m_b3D = pMesh->Is3D();
            int poly = -1;
            int tri = -1;
            if (pMesh == gNavMeshHandler.GetPtr((u32)m_nMesh)) {
                poly = XNavMeshHandler::PolyID(m_nPolyHandle);
                tri = XNavMeshHandler::PolyTri(m_nPolyHandle);
            } else {
                m_nMesh = pMesh->Handle();
            }
            if (m_nMesh != 0) {
                pMesh->LocatePolygon(m_vPos, &poly, &tri, 0);
                if (poly < 0 || (m_dwIgnore & pMesh->PolyFlags(poly))) {
                    EXVector fixed;
                    pMesh->ValidatePos(m_vPos, m_dwIgnore, fixed, &poly);
                    m_vPos = fixed;
                }
                SetPoly(pMesh, poly, tri);
                UpdateFloorInfo(pMesh, poly);
                if (poly >= 0) m_bValid = 1;
            }
        }
    }
}

// Make sure the location sits on a legal polygon, moving it if it does not.
// bForce re-locates even when already valid; bCheckHeight also rejects a position more
// than a unit above or below the floor of its polygon.
s8 XNavLocation::Validate(s8 bForce, s8 bCheckHeight)
{
    if (m_bValid && !bForce && m_b3D == m_b2) return 1;
    m_bValid = 0;
    m_b2 = m_b3D;
    XNavMesh *pMesh = gNavMeshHandler.GetPtr(m_vPos, m_b3D);
    if (!pMesh) return 1;
    m_b3D = pMesh->Is3D();
    int poly = -1;
    int tri = -1;
    if (pMesh == gNavMeshHandler.GetPtr((u32)m_nMesh)) {
        poly = XNavMeshHandler::PolyID(m_nPolyHandle);
        tri = XNavMeshHandler::PolyTri(m_nPolyHandle);
    } else {
        m_nMesh = pMesh->Handle();
    }
    if (m_nMesh == 0) return 0;
    pMesh->LocatePolygon(m_vPos, &poly, &tri, 0);
    float heightDiff = 100000.0f;
    int startPoly = -1;
    bool bFix = false;
    if (bCheckHeight && poly >= 0) {
        float y = m_vPos.y;
        startPoly = poly;
        heightDiff = (float)fabs(y - pMesh->GetHeight(m_vPos, poly, 0, &m_vFloorNormal));
    }
    if (poly < 0 || (bCheckHeight && heightDiff > 1.0f) || (m_dwIgnore & pMesh->PolyFlags(poly))) bFix = true;
    if (bFix) {
        EXVector fixed;
        pMesh->ValidatePos(m_vPos, m_dwIgnore, fixed, &poly);
        if (bCheckHeight && poly >= 0 && poly == startPoly) {
            poly = pMesh->NearestPolygon(m_vPos, &fixed, m_dwIgnore);
            float dx = fixed.x - m_vPos.x;
            float dy = fixed.y - m_vPos.y;
            float dz = fixed.z - m_vPos.z;
            float dw = fixed.w - m_vPos.w;
            float d = (float)sqrt(dy * dy + dz * dz + dw * dw + dx * dx);
            if (heightDiff < d) {
                fixed = m_vPos;
                poly = startPoly;
            }
        }
        m_vPos = fixed;
    }
    SetPoly(pMesh, poly, tri);
    UpdateFloorInfo(pMesh, poly);
    if (poly >= 0) {
        m_bValid = 1;
        return 1;
    }
    return 0;
}

// Move the location by a delta, sliding it across the nav mesh. Flag bits in *pFlags:
// in: 8 and 0x10 ask for 3D handling; out: 4 means the move ended on the floor.
void XNavLocation::Move(const EXVector &delta, u32 *pFlags)
{
    if (!m_bValid) return;
    float lenSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    if (lenSq == 0.0f) return;
    XNavMesh *pMesh = gNavMeshHandler.GetPtr((u32)m_nMesh);
    int bFlagA = 0;
    int bFlagB = 0;
    if (pFlags) {
        bFlagA = (*pFlags >> 3) & 1;
        bFlagB = (*pFlags >> 4) & 1;
    }
    int poly = XNavMeshHandler::PolyID(m_nPolyHandle);
    if (!(poly < (int)pMesh->PolyCount())) {
        pMesh->LocatePolygon(m_vPos, &poly, 0, 0);
        if (poly < 0) return;
    }
    EXVector target = m_vPos + delta;
    EXVector dest(m_v50.x + target.x, m_v50.y + target.y, m_v50.z + target.z, m_v50.w + target.w);
    m_v50.SetZero();
    float dy = m_vPos.y - dest.y;
    if (!m_b3D || (!bFlagA && !bFlagB)) {
        pMesh->TraversePolys(m_vPos, poly, dest, m_dwIgnore, pFlags, (XNavRoute *)m_60);
        m_vPos = dest;
        SetPoly(pMesh, poly, 0);
        UpdateFloorInfo(pMesh, poly);
        if (poly >= 0) {
            if (!m_b3D && dest.y - m_vFloorPos.y < 0.1f && (!bFlagA || dy > 0.0f)) {
                dest.y = m_vFloorPos.y;
                if (pFlags) *pFlags |= 4;
            }
            m_vPos = dest;
            SetPoly(pMesh, poly, 0);
        }
    } else {
        float rayLen = 10000.0f;
        if (pMesh->LocatePolygon3DRay(m_vPos, delta, &poly, m_dwIgnore & 0xfffff7f7, &rayLen, 0)) {
            float x = delta.x, y = delta.y, z = delta.z, w = delta.w;
            float l2 = w * w + z * z + y * y + x * x;
            if (l2 >= 0.0f) {
                float l = (float)sqrt(l2);
                if (l > 1e-5f) {
                    float inv = 1.0f / l;
                    x *= inv;
                    y *= inv;
                    z *= inv;
                    w *= inv;
                }
            }
            m_vFloorPos = EXVector(m_vPos.x + x * rayLen, m_vPos.y + y * rayLen, m_vPos.z + z * rayLen, m_vPos.w + w * rayLen);
            SetPoly(pMesh, poly, -1);
            UpdateFloorInfo(pMesh, poly);
            if (rayLen * rayLen <= lenSq) {
                dest = m_vFloorPos;
                if (pFlags) *pFlags |= 4;
            }
        }
        m_vPos = dest;
    }
    m_v00 = m_vPos;
}

// Retail inlines the "first mesh of this level" setup in every branch of AddNavMesh.
static XNavMesh *NewMeshes(XNavMeshHandler *h, int n)
{
    h->UnloadLevel();
    h->m_pMeshes = new XNavMesh[n];
    h->m_nMeshes = n;
    return h->m_pMeshes;
}

// File a loaded nav mesh geometry entity: pick (or create) the XNavMesh it belongs to,
// register it in the sector grid, and tell the AI about it.
s8 XNavMeshHandler::AddNavMesh(XGeoNavMeshEntity *pEnt, u32 hash)
{
    if (!pEnt) return 0;
    int sector = GridSectorForPos(pEnt->m_vBoundsMid);
    pEnt->SetGridSector((u16)sector);
    XNavMesh *pMesh;
    XNavSubMap *pSubMaps;
    u16 nSubMaps;
    if (!pEnt->IsMaster()) {
        if (!pEnt->HasData()) {
            if (m_nType == 0) {
                NewMeshes(this, 1);
                m_pMeshes->Init();
                m_pMeshes->m_dwHandle = 1;
            }
            pMesh = m_pMeshes;
            m_nType = 3;
        } else {
            if (pEnt->IsSingle()) {
                if (m_nType == 0) NewMeshes(this, 2);
                m_nType = 1;
                if (pEnt->Is3D()) {
                    m_pMeshes[1].Init();
                    m_pMeshes[1].AddNavMesh(pEnt, 0, 0);
                    m_pMeshes[1].m_dwHandle = 2;
                } else {
                    m_pMeshes[0].Init();
                    m_pMeshes[0].AddNavMesh(pEnt, 0, 0);
                    m_pMeshes[0].m_dwHandle = 1;
                }
                goto tail;
            }
            if (m_nType == 0) {
                NewMeshes(this, 12);
                for (int i = 0; i < 12; i++) {
                    m_pMeshes[i].Init();
                    m_pMeshes[i].m_dwHandle = i + 1;
                }
            }
            m_nType = 2;
            pMesh = m_pMeshes + sector;
        }
        pSubMaps = 0;
        nSubMaps = 0;
    } else {
        if (m_nType == 0) {
            NewMeshes(this, 1);
            m_pMeshes->Init();
            m_pMeshes->m_dwHandle = 1;
        }
        m_nType = 3;
        if (m_pSubMaps) {
            EXFree(m_pSubMaps);
            m_pSubMaps = 0;
        }
        m_wSubMaps = pEnt->SubMaps();
        if (m_wSubMaps) {
            m_pMaster = pEnt;
            XNavSubMapData *src = pEnt->SubMapData();
            m_pSubMaps = (XNavSubMap *)_EXAlloc(m_wSubMaps * sizeof(XNavSubMap), 0);
            for (int i = 0; i < m_wSubMaps; i++, src++) {
                m_pSubMaps[i].m_Data = *src;
                int s = GridSectorForPos(m_pSubMaps[i].m_Data.m_vBoundsMid);
                m_pSubMaps[i].m_wSector = (u16)s;
                m_Sectors[s].m_pSubMap = &m_pSubMaps[i];
            }
            pEnt->NavNetwork()->m_Section.m_wLinkOffset = pEnt->LinkOffset();
        }
        pSubMaps = m_pSubMaps;
        nSubMaps = m_wSubMaps;
        pMesh = m_pMeshes;
        m_b0 = 1;
    }
    pMesh->AddNavMesh(pEnt, nSubMaps, pSubMaps);
tail:
    m_b0 = !pEnt->IsSingle();
    if (pEnt->Is3D()) m_Entities3D.Add(pEnt);
    else m_Entities.Add(pEnt);
    RefreshMaps();
    if (sector >= 0 && sector < 12) {
        pEnt->SetID((u16)sector);
        m_Sectors[sector].m_pMesh = pEnt;
        pEnt->SetSectorPtr(&m_Sectors[sector]);
        gAIControl.NavMeshAdded(pEnt);
        m_bDynamic = pEnt->IsDynamic();
        m_GeoFiles[m_nGeoFiles].m_00 = 0x10000 | (u16)m_nGeoFiles;
        m_GeoFiles[m_nGeoFiles].m_dwHashCode = hash;
        m_GeoFiles[m_nGeoFiles].m_pEntity = pEnt;
        m_nGeoFiles++;
        pEnt->NavNetwork()->RemoveBadLinks();
        return 1;
    }
    gAIControl.NavMeshAdded(pEnt);
    return 0;
}

static __forceinline bool NearSame(const EXVector &a, const EXVector &b)
{
    return fabs(a.x - b.x) < 0.001f && fabs(a.y - b.y) < 0.001f && fabs(a.z - b.z) < 0.001f && fabs(a.w - b.w) < 0.001f;
}

// Put the location at a new position. Small steps are slid along the mesh with
// ValidateLine; big jumps (or a bad slide) throw the old state away and re-find the polygon.
void XNavLocation::SetPosSlow(const EXVector &p, s8 bCheckHeight)
{
    EXVector target(m_v50.x + p.x, m_v50.y + p.y, m_v50.z + p.z, m_v50.w + p.w);
    if (m_bValid) {
        if (NearSame(m_v00, target)) return;
        EXVector diff(m_v00.x - target.x, m_v00.y - target.y, m_v00.z - target.z, m_v00.w - target.w);
        float lenSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z + diff.w * diff.w;
        if (lenSq < 0.0f || (float)sqrt(lenSq) < 0.5f) {
            if (fabs(m_v00.x - m_vPos.x) >= 0.001f) return;
            if (fabs(m_v00.y - m_vPos.y) >= 0.001f) return;
            if (fabs(m_v00.z - m_vPos.z) >= 0.001f) return;
            if (fabs(m_v00.w - m_vPos.w) >= 0.001f) return;
            XNavMesh *pMesh = GetMesh();
            int poly = -1;
            EXVector out;
            pMesh->ValidateLine(*this, target, poly, out, m_dwIgnore);
            m_vPos = out;
            SetPoly(pMesh, poly, 0);
            UpdateFloorInfo(pMesh, poly);
            m_v00 = p;
            if (!bCheckHeight) return;
            if (m_fMeshDistance <= 1.0f) return;
            m_bValid = 0;
        }
    }
    if (m_bValid && NearSame(m_v00, target)) return;
    m_bValid = 0;
    m_vPos.SetZero();
    m_v20.SetZero();
    m_vFloorPos.SetZero();
    m_nMesh = 0;
    m_nPolyHandle = 0;
    m_vFloorNormal.Set(0.0f, 1.0f, 0.0f, 0.0f);
    m_wSurfaceFlags = 0;
    m_fMeshDistance = 0.0f;
    m_v50.SetZero();
    m_v00 = target;
    m_vPos = target;
    Validate(0, bCheckHeight);
}

// Refresh everything derived from the polygon the location stands on: the floor point and
// normal, the distance to the mesh, the surface flags, and (for ground actors with a push
// radius) the wall block bits and the sideways push away from solid edges.
void XNavLocation::UpdateFloorInfo(XNavMesh *pMesh, int poly)
{
    if (!pMesh || poly < 0) {
        m_wGeoBlockBits = 0;
        return;
    }
    if (!m_b3D) {
        m_vFloorPos = m_vPos;
        m_vFloorPos.y = pMesh->GetHeight(m_vPos, poly, 0, &m_vFloorNormal);
        float dx = m_vPos.x - m_vFloorPos.x;
        float dy = m_vPos.y - m_vFloorPos.y;
        float dz = m_vPos.z - m_vFloorPos.z;
        float dw = m_vPos.w - m_vFloorPos.w;
        float lenSq = dx * dx + dy * dy + dz * dz + dw * dw;
        m_fMeshDistance = lenSq >= 0.0f ? (float)sqrt(lenSq) : 0.0f;
    } else {
        EXPlane plane;
        pMesh->PolyPlane(poly, plane);
        m_vFloorNormal = EXVector(plane.A, plane.B, plane.C, 0.0f);
        float x = m_vPos.x, y = m_vPos.y, z = m_vPos.z;
        float dist = 0.0f - (plane.A * x + y * plane.B + z * plane.C + 1.0f * plane.D);
        m_vFloorPos = EXVector(plane.A * dist + x, plane.B * dist + y, plane.C * dist + z, dist * 0.0f);
        m_fMeshDistance = dist;
    }
    m_wSurfaceFlags = poly < (int)pMesh->PolyCount() ? (u16)pMesh->PolyFlags(poly) : 0;
    m_wGeoBlockBits = 0;
    if (!m_b3D && m_fPushRadius > 0.0f) {
        int count = 0;
        EXVector verts[16];
        pMesh->PolyVerts(poly, verts, &count);
        u8 usage[16];
        pMesh->NavNetwork()->Node((u16)poly)->GetEdgeUsage(usage);
        m_v50.SetZero();
        int next = 1;
        for (int i = 0; i < count; i++, next++) {
            if (count <= next) next = 0;
            if (usage[i] != 0) continue;
            const EXVector &a = verts[i];
            const EXVector &b = verts[next];
            // edge a->b crossed with straight up gives the wall normal
            float ex = a.x - b.x, ey = a.y - b.y, ez = a.z - b.z;
            float nx = ey * 0.0f - ez * 1.0f;
            float ny = ez * 0.0f - 0.0f * ex;
            float nz = 1.0f * ex - ey * 0.0f;
            float lenSq = nx * nx + ny * ny + nz * nz;
            float len = lenSq < 0.0f ? 0.0f : (float)sqrt(lenSq);
            float d = 0.0f;
            if (len != 0.0f) {
                float inv = 1.0f / len;
                nz *= inv;
                ny *= inv;
                nx *= inv;
                d = 0.0f - (nz * b.z + ny * b.y + b.x * nx);
            } else {
                nx = 1.0f;
                ny = 0.0f;
                nz = 0.0f;
            }
            float dist = nx * m_vPos.x + 1.0f * d + nz * m_vPos.z + m_vPos.y * ny;
            if (dist <= m_fPushRadius) {
                EXVector out(0.0f - nx, 0.0f - ny, 0.0f - nz, 0.0f);
                m_wGeoBlockBits |= CalcBlockBits(out);
            }
            if (dist < m_70) {
                float push = m_70 - dist;
                m_v50.x += nx * push;
                m_v50.y += ny * push;
                m_v50.z += nz * push;
                m_v50.w += push * 0.0f;
            }
        }
        m_v50.y = 0.0f;
        m_v50.x *= 0.1f;
        m_v50.y *= 0.1f;
        m_v50.z *= 0.1f;
        m_v50.w *= 0.1f;
    }
}
