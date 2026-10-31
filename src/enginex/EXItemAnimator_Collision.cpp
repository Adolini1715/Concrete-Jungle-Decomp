#include "EXItemAnimator_Collision.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

// TODO stand-in: script collision command (geo script area).
struct GEOSCRIPT_COLLISION {
    u8 pad[0xc];
    s32 m_0c;           // +0xc
    u8 m_type;          // +0x10
    u8 pad11[3];
    EXVector3 m_size;   // +0x14
};

EX_IMPLEMENT_DYNCREATE(EXItemAnimator_Collision, EXItemAnimator)

EXItemAnimator_Collision::EXItemAnimator_Collision()
{
    memset(&m_datum, 0, sizeof(m_datum));
    m_134 = -1;
}

void EXItemAnimator_Collision::SetFromScriptCmd(const GEOSCRIPT_COLLISION *pCmd)
{
    m_datum.m_position.SetZero();
    m_datum.m_rotation.set_zero1();
    m_datum.m_2c = 0;
    m_datum.m_2e = 0;
    m_datum.m_type = pCmd->m_type;
    m_datum.m_size.x = pCmd->m_size.x;
    m_datum.m_size.y = pCmd->m_size.y;
    m_datum.m_size.z = pCmd->m_size.z;
    m_134 = pCmd->m_0c;
}

// NEAR: 8 bytes differ, load/store scheduling of the 3-dword scale copy.
void EXItemAnimator_Collision::SetObjectTime(float)
{
    m_renderFlags.BitClr(2);
    m_datum.m_size.x = m_scale.x;
    m_datum.m_size.y = m_scale.y;
    m_datum.m_size.z = m_scale.z;
    CalcMatrix_InPlaceParent();
}

// NEAR: register allocation (original keeps &pCtx in edi, pushes ebx late around the 3-dword copy).
void *EXItemAnimator_Collision::GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx)
{
    void *pResult = 0;
    if (pCtx != &m_datum) {
        d = m_datum;
        if (!(flags & 4)) {
            EXMatrix mtx;
            CalcMatrix(mtx);
            d.ApplyXForm(mtx);
        }
        pResult = &m_datum;
    } else if (flags & 0x80) {
        pResult = GetDatum_ChildLink(type, d, flags, pCtx);
    }
    return pResult;
}

void EXItemAnimator_Collision::GetDatum(u32 type, EXDatumTempArray &arr, u32 flags)
{
    arr.Add(m_datum, 1);
    if (flags & 0x80) GetDatum_ChildLink(type, arr, flags);
}
