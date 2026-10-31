#include "EXItemCtrl.h"
#include "EXItemCtrl_SnapShot.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

template <class T> T EXEvaluateSpline_CatmullRom(const T &p0, const T &p1, const T &p2, const T &p3, float t);
template <class T> T EXSplinePos(const T *pts, int n, int i, float t);

EX_IMPLEMENT_DYNCREATE(EXItemCtrl_SnapShot, EXItemCtrl)

void EXItemCtrlStack::KillStack(EXItemCtrl *pCtrl)
{
    if (!pCtrl) pCtrl = FirstCtrl();
    while (pCtrl) {
        EXItemCtrl *pNext = pCtrl->Next();
        m_list.Remove(pCtrl);
        delete pCtrl;
        pCtrl = pNext;
    }
}

EXItemCtrl *EXItemCtrl::operator=(const EXItemCtrl *p)
{
    Init();
    m_pCtrlStack = 0;
    m_geoRef = p->m_geoRef;
    m_ctrlFlags = p->m_ctrlFlags;
    m_blendSpline = p->m_blendSpline;
    m_frame = p->m_frame;
    m_frameRateMult = p->m_frameRateMult;
    m_blend = p->m_blend;
    m_blendRate = p->m_blendRate;
    return this;
}

s8 EXItemCtrl::Initialise(EXItemAnimator_Anim *) { return 1; }
float EXItemCtrl::GetFrame() const { return m_frame; }
void EXItemCtrl::SetFrame(float f) { m_frame = f; }
void EXItemCtrl::SetObjectFrame(float f) { m_frame = f; }
void EXItemCtrl::SetObjectTime(float, s8) {}
void EXItemCtrl::SetFrameRateMult(float f) { m_frameRateMult = f; }
void EXItemCtrl::SetBlendSpline(u32 s) { m_blendSpline = s; }
s8 EXItemCtrl::UpdateCtrlOnStack(EXItemAnimator_Anim *, EXItemCtrlStack *, EXRotPosInfo *, int) { return 0; }
u8 EXItemCtrl::CalcMotionData(EXMotionData *, const EXCalcMotionTemp &) { return 8; }
void *EXItemCtrl::GetFrameDatum(EXItemAnimator_Anim *, u32, EXDatumTemp &, u32) { return 0; }
void EXItemCtrl::GetBoundsBox(EXBoundsBox &, u32) {}

EX_IMPLEMENT_DYNCREATE_ROOT(EXItemCtrl)

EXItemCtrlStack::EXItemCtrlStack()
{
    m_flags = 0;
    m_stackSize = 1;
    m_blend = 1.0f;
    m_sectionBits.SetAll();
}

void EXItemCtrlStack::SetSelectiveSectionBits(const ex::EXBit128 &bits)
{
    m_sectionBits = bits;
    m_flags.BitSet(2);
}

void EXItemCtrlStack::UpdateFlushStack()
{
    EXItemCtrl *pCtrl = FirstCtrl();
    s8 bFlush = 0;
    u32 count = 0;
    while (pCtrl) {
        EXItemCtrl *pNext = pCtrl->Next();
        if (count >= m_stackSize) bFlush = 1;
        if ((pCtrl->m_ctrlFlags.m_flags & 0x40000) || bFlush) {
            m_list.Remove(pCtrl);
            delete pCtrl;
        } else {
            if (pCtrl->m_blend >= 1.0f) bFlush = 1;
            count++;
        }
        pCtrl = pNext;
    }
}

// NEAR: 6 bytes, the original loads 1.0f after `mov eax, ecx` (instruction scheduling only).
EXItemCtrl::EXItemCtrl(u32 hash)
{
    m_geoRef.Set(0, hash);
    m_ctrlFlags = 0;
    m_pCtrlStack = 0;
    m_blendSpline = 0;
    m_frame = 0.0f;
    Blend() = 1.0f;
    m_blendRate = 0.0f;
    m_frameRateMult = 1.0f;
}

void EXItemCtrl::UpdateBlendFracs(float dt)
{
    if (m_blendRate == 0.0f) return;
    float b = m_blendRate * dt + m_blend;
    if (b > 1.0f) {
        m_blendRate = 0.0f;
        b = 1.0f;
    } else if (0.0f > m_blend) {
        b = 0.0f;
        m_blendRate = 0.0f;
    }
    m_blend = b;
}

void EXItemCtrl::SetBlendRateFrames(u32 n)
{
    float r;
    if (n) {
        float f = (float)n;
        if (f != 0.0f) r = 1.0f / f;
        else r = 0.0f;
    } else {
        r = 1.0f;
    }
    m_blendRate = r;
}

EX_IMPLEMENT_DYNCREATE(EXItemCtrl_Anim, EXItemCtrl)
EX_IMPLEMENT_DYNCREATE(EXItemCtrl_AnimScript, EXItemCtrl_Anim)

void EXItemCtrlStack::UpdateStack()
{
    if (m_flags.m_flags & 1) UpdateFlushStack();
}

template <class T> T EXSplinePos(const T *pts, int n, int i, float t)
{
    if (n < 4) return T(0.0f, 0.0f);
    int i0 = i - 1;
    int lo = 0;
    int a = max(lo, i0);
    int i3 = i + 2;
    int last = n - 1;
    int b = min(last, i3);
    return EXEvaluateSpline_CatmullRom(pts[a], pts[i], pts[i + 1], pts[b], t);
}

template <class T> T EXEvaluateSpline_CatmullRom(const T &p0, const T &p1, const T &p2, const T &p3, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    T a = (p3 + (p1 * 3.0f - p2 * 3.0f) - p0) * t3;
    T b = (p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2;
    T c = (p2 - p0) * t;
    return (p1 * 2.0f + c + b + a) * 0.5f;
}

float EXItemCtrl::GetSplineBlend() const
{
    float b = m_blend;
    if (!m_blendSpline) return b;
    if (0.0f >= b) return b;
    if (b >= 1.0f) return b;
    const u8 *p = (const u8 *)&m_blendSpline;
    ms_BlendSpline[1].Set(p[0] * 0.01f, p[1] * 0.01f);
    ms_BlendSpline[2].Set(p[2] * 0.01f, p[3] * 0.01f);
    int i;
    for (i = 0; i < 2; i++)
        if (ms_BlendSpline[i + 1].x > b) break;
    float t = (b - ms_BlendSpline[i].x) / (ms_BlendSpline[i + 1].x - ms_BlendSpline[i].x);
    EXVector2 v = EXSplinePos(ms_BlendSpline, 4, i, t);
    b = limit(v.y, 0.0f, 1.0f);
    return b;
}

s8 EXItemCtrl::ms_ForceUpdateObjectTime;
EXVector2 EXItemCtrl::ms_BlendSpline[4] = {
    EXVector2(0.0f, 0.0f), EXVector2(1.0f / 3.0f, 1.0f / 3.0f), EXVector2(2.0f / 3.0f, 2.0f / 3.0f), EXVector2(1.0f, 1.0f)
};
ex::EXBit128 EXItemCtrlStack::ms_DeformerUsedBits;
ex::EXBit128 EXItemCtrlStack::ms_BoneUsedBits;
u32 EXItemCtrlStack::ms_UpdateFlags;
