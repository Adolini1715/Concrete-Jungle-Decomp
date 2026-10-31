#include "EXItemAnimator_DynLight.h"
#include "EXBaseRenderEnv.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

EX_IMPLEMENT_DYNCREATE(EXItemAnimator_DynLight, EXItemAnimator)

EXItemAnimator_DynLight::EXItemAnimator_DynLight() { m_pDynLight = 0; }

void EXItemAnimator_DynLight::CleanInitialise()
{
    if (m_pDynLight) {
        m_pDynLight->SetActive(0);
        m_pDynLight = 0;
    }
}

s8 EXItemAnimator_DynLight::Initialise(EXGeoFile *, u32) { return 1; }

void EXItemAnimator_DynLight::AttachDynLight(EXDynLight *pLight)
{
    CleanInitialise();
    m_pDynLight = pLight;
    m_dir = pLight->Dir();
}

EXItemAnimator_DynLight::~EXItemAnimator_DynLight() { CleanInitialise(); }

// NEAR: VC7.1 refuses to inline mat_44::operator* and the EXRenderInfo(a, b) constructor here
// (the original inlines both: D3DXVec4Transform/Merge called directly, 0x80-byte frame). Small test
// functions inline them; inlining stops once the function has enough nesting/inline expansions.
// Also the original tests the RGB-mult flag as a bitfield (shr 2 / test 1).
u32 EXItemAnimator_DynLight::DoRender(const EXMatrix &parent, const EXRenderInfo &info, u32 flags)
{
    u32 result = 0;
    if (EXBaseRenderEnv::RenderState() & 1) CalcMatrix_InPlace(parent, flags);
    const EXMatrix &m = mtx();
    if (EXBaseRenderEnv::RenderState() & 2) {
        if (EXBaseRenderEnv::ShadowRender()) return 0;
        if (m_pDynLight) {
            m_pDynLight->SetPos(m.GetTranslation());
            if (m_pDynLight->Directional()) {
                EXVector dir(m_dir.x, m_dir.y, m_dir.z, 0.0f);
                m_pDynLight->SetDir(EXVector((const EXMaths::mat_44 &)m * dir));
            }
            if (m_renderInfo.RGBMultEnabled()) {
                EXRenderInfo ri(m_renderInfo, info);
                EXMaths::vec_xyzw c = ri.RGBAMult();
                float r = limit(c.x, 0.0f, 255.0f);
                float g = limit(c.y, 0.0f, 255.0f);
                float b = limit(c.z, 0.0f, 255.0f);
                float a = limit(c.w, 0.0f, 255.0f);
                m_pDynLight->SetColor(ex::XRGBA((u8)r, (u8)g, (u8)b, (u8)a));
            }
        }
        result = 1;
    }
    return RenderNodes(m, info, flags) | result;
}
