#include <math.h>
#include "EXItemAnimator_Anim.h"
#include "EXItemAnimator_DynLight.h"
#include "EXItemAnimator_Camera.h"
#include "EXStdFuncs.h"

// Script-thread controller evaluation (EXItemAnimator_ScriptControllers.obj). Only the simpler
// members are decompiled; the spline/controller-info templates are not.


// TODO stand-in: self-relative 32-bit offset without a null check (geo script area).
template <class T> struct EXScriptRelPtr {
    T *Ptr() const { return (T *)((char *)this + m_off); }
    s32 m_off;
};

// TODO stand-in: script controller block header (geo script area).
struct EXGeoScriptControllerHeader {
    void *Data() const
    {
        const s16 *p = &m_dataOff;
        return (char *)p + *p;
    }
    template <class T> T *Keys() const { return ((const EXScriptRelPtr<T> *)&m_keysOff)->Ptr(); }
    s16 m_numKeys;      // +0 negative: spline with -n keys
    s16 m_dataOff;      // +2 self-relative offset of the controller data
    s32 m_keysOff;      // +4 self-relative offset of the keys (0: none)
};
// TODO stand-in: float key (geo script area).
struct EXGeoScript_FloatController {
    float m_time;
    float m_value;
};
// TODO stand-in: vertex/light set controller data (geo script area).
struct EXGeoScript_SetController {
    u16 m_rate;         // +0 frames per second when not key driven
    s8 m_count;         // +2 0: blend set n with n+1
    u8 m_mode;          // +3 CalcTime mode
    u16 m_sets[1];      // +4
};
struct EXGeoScript_TimeController {
    u16 m_mode;
    u16 m_02;
    float m_start;      // +4
    float m_end;        // +8
};
// TODO stand-in: texture-animation entry (geo script area); sizeof 0xc.
struct EXGeoScript_TextureAnim {
    u32 m_hash;
    u16 m_mode;         // +4
    u16 m_start;        // +6
    float m_rate;       // +8
};
struct EXGeoScript_TextureAnimList {
    u32 m_count;
    EXGeoScript_TextureAnim m_anims[1];     // +4
};
// TODO stand-ins: textures (render area).
class EXBaseGeoTexture {
public:
    u32 FrameRate() const { return m_frameRate; }
    u8 m_00[0xe];
    u8 m_frameRate;     // +0xe
};
class EXBaseTexture {
public:
    u8 FrameCount() const;
    void SetFrame(u8 f) { m_frame = f; }
    u8 m_00[8];
    EXBaseGeoTexture *m_pGeoTexture;    // +8
    u8 m_0c[4];
    u8 m_frame;                         // +0x10
};
class EXTexture {
public:
    u8 m_00[0xc];
    EXBaseTexture m_base;               // +0xc
};
// TODO stand-in: geo file header (geo area).
class EXGeoHeader {
public:
    EXTexture *Texture(u32 hash) const;
};
class EXGeoFile {
public:
    u8 m_00[0x20];
    EXGeoHeader *m_pHeader;             // +0x20
};

// TODO stand-in: per-controller runtime state (script area).
struct EXGeoScript_VectorController {
    float m_time;
    float m_x, m_y, m_z;
};

struct EXScriptController {
    u32 m_00;
    u32 m_key;          // +4 current key
    static float UpdateThread_CalcTime(float start, float end, u32 mode, float t);
    static float UpdateThread_TimeControllers(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, u32 flags);
    static u32 UpdateThread_FloatController(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, float &out);
    static u32 UpdateThread_VectorController(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, EXVector &out);
    static u32 UpdateThread_FloatController(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, float *pOut, int n);
    static u32 UpdateThread_QuatController(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, EXVector &out);
};

// TODO stand-in: texture swap entry (geo script area); sizeof 0x10.
struct EXGeoScript_TextureSwap {
    u32 *TextureLookupTble() const
    {
        if (!m_tableOff) return 0;
        return (u32 *)((char *)&m_tableOff + m_tableOff);
    }
    u32 m_hash;                             // +0
    s32 m_tableOff;                         // +4
    EXGeoScriptControllerHeader m_ctrl;     // +8
    u8 m_0c[4];
};
struct EXGeoScript_TextureSwapList {
    int m_count;
    EXGeoScript_TextureSwap m_swaps[1];     // +4
};

// TODO stand-in: texture swap table (render area).
class EXTexSwapInfo {
public:
    void SetSwap(u32 from, EXGeoFile *pFile, u32 to);
};

// TODO stand-in: running script thread (EXGeoScript area); only the animator and the active
// controller bits are declared.
class EXScriptThread {
protected:
    void UpdateThread_ExtraControllers(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t, u32 flags);
    void UpdateThread_ScreenFadeController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t);
    void UpdateThread_LightSetController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t);
    void UpdateThread_VertexSetController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t);
    void UpdateThread_CameraControllers(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t, u32 flags);
    void UpdateThread_TextureAnimController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t);
    void UpdateThread_TextureSwapController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t);
    void UpdateThread_AnimDepthController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t);
    void UpdateThread_LightRadiusController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t);
private:
    static u32 UpdateThread_AnimDepthController_Func(EXItemAnimator *p, void *pData);
public:
    u8 m_00[8];
    EXItemAnimator *m_pAnimator;    // +8
    u8 m_0c[0x10];
    u32 m_activeBits;               // +0x1c controllers still running
};

// b + (a - b) * f, operands by value.
static inline float ScriptLerp(float a, float b, float f) { return b + (a - b) * f; }

// Cubic a + b x + c x^2 + d x^3 in x = t - t0 (EXItemAnimator_ScriptControllers.obj copies).
class EXCubicSpline {
public:
    // Hermite segment from (t0, v0) to (t1, v1) with slopes s0, s1.
    EXCubicSpline(float t0, float v0, float t1, float v1, float s0, float s1)
    {
        float inv = 1.0f / (t1 - t0);
        m_t0 = t0;
        m_a = v0;
        m_b = s0;
        float m = (v1 - v0) * inv;
        m_c = (m * 3.0f - s0 * 2.0f - s1) * inv;
        m_d = (s0 + s1 - m * 2.0f) * inv * inv;
    }
    // Quadratic from (t0, v0) with slope s0 through (t1, v1).
    EXCubicSpline(float t1, float v1, float t0, float v0, float s0)
    {
        float span = t1 - t0;
        float inv = 1.0f / span;
        m_t0 = t0;
        m_a = v0;
        m_b = s0;
        m_c = ((v1 - v0) - span * s0) * inv * inv;
        m_d = 0.0f;
    }
    float Calc(float t) const
    {
        float x = t - m_t0;
        return ((m_d * x + m_c) * x + m_b) * x + m_a;
    }
    float m_a, m_b, m_c, m_d;   // +0
    float m_t0;                 // +0x10
};

// Spline keys are {time, m values}; value j of a key.
typedef EXGeoScript_FloatController SplineKey;  // {time, values...}: only the time is a fixed field
static inline float SplineKeyTime(const SplineKey *pKey) { return pKey->m_time; }
static inline float SplineKeyValue(const SplineKey *pKey, int j) { return (&pKey->m_value)[j]; }

// Two keys: an ease-in/ease-out cubic (zero slopes at both ends).
static __declspec(noinline) float UpdateThread_Get2PointSplineValue(const SplineKey *k0, const SplineKey *k1, float t, int j)
{
    return EXCubicSpline(SplineKeyTime(k0), SplineKeyValue(k0, j), SplineKeyTime(k1), SplineKeyValue(k1, j), 0.0f, 0.0f).Calc(t);
}

// Three keys, t in an end segment: a quadratic leaving `from` with the slope between `to` and
// `other` (the chord across the middle key), reaching `to`.
static __declspec(noinline) float Get3PointSplineValue(const SplineKey *from, const SplineKey *to, const SplineKey *other, float t, int j)
{
    float slope = (SplineKeyValue(other, j) - SplineKeyValue(to, j)) / (SplineKeyTime(other) - SplineKeyTime(to));
    return EXCubicSpline(SplineKeyTime(to), SplineKeyValue(to, j), SplineKeyTime(from), SplineKeyValue(from, j), slope).Calc(t);
}

// Four keys, t between k1 and k2: a Catmull-Rom style Hermite segment (slopes from the
// neighbouring chords).
static __declspec(noinline) float Get4PointSplineValue(const SplineKey *k0, const SplineKey *k1, const SplineKey *k2, const SplineKey *k3,
                                                       float t, int j)
{
    float s1 = (SplineKeyValue(k2, j) - SplineKeyValue(k0, j)) / (SplineKeyTime(k2) - SplineKeyTime(k0));
    float s2 = (SplineKeyValue(k3, j) - SplineKeyValue(k1, j)) / (SplineKeyTime(k3) - SplineKeyTime(k1));
    return EXCubicSpline(SplineKeyTime(k1), SplineKeyValue(k1, j), SplineKeyTime(k2), SplineKeyValue(k2, j), s1, s2).Calc(t);
}

// Spline controller: n keys of m values. Finds the first key after t (from the current key on),
// takes up to four keys around it (two before, one after) and evaluates the matching curve. The
// controller's current key becomes the first of those. Returns whether t is past the last key.
template <class V>
u32 UpdateThread_ControllerInfoSpline(EXGeoScript_FloatController *pKeys, V *pOut, EXScriptController *pCtrl, float t, u32 n, u32 m)
{
    const float *pK = (const float *)pKeys;
    u32 stride = m + 1;
    u32 i = pCtrl->m_key;
    for (; i < n && pK[i * stride] <= t; i++) {
    }
    int first = (int)i - 2 > 0 ? (int)i - 2 : 0;
    int last = (int)i + 1 > (int)n - 1 ? (int)n - 1 : (int)i + 1;
    int rel = (int)i - first;     // 2: t lies between pts[1] and pts[2]
    int count = last - first + 1;
    pCtrl->m_key = first;
    const SplineKey *pts[8];
    for (int k = 0; k < count; k++) pts[k] = (const SplineKey *)(pK + (first + k) * stride);
    switch (count) {
    case 0:
        for (u32 j = 0; j < m; j++) pOut[j] = 0;
        break;
    case 1:
        for (u32 j = 0; j < m; j++) pOut[j] = SplineKeyValue(pts[0], j);
        break;
    case 2:
        for (u32 j = 0; j < m; j++)
            pOut[j] = rel == 2 ? SplineKeyValue(pts[1], j) : UpdateThread_Get2PointSplineValue(pts[0], pts[1], t, j);
        break;
    case 3:
        for (u32 j = 0; j < m; j++)
            pOut[j] = rel == 2 ? Get3PointSplineValue(pts[1], pts[2], pts[0], t, j) : Get3PointSplineValue(pts[1], pts[0], pts[2], t, j);
        break;
    default:
        for (u32 j = 0; j < m; j++)
            pOut[j] = rel == 2 ? Get4PointSplineValue(pts[0], pts[1], pts[2], pts[3], t, j)
                               : Get3PointSplineValue(pts[1], pts[0], pts[2], t, j);
        break;
    }
    return n <= i;
}

// Finds the key pair around t, starting from the controller's current key.
// __forceinline: the original inlines it into the controller functions but still emits a copy;
// without it our compiler calls the copy (which then matches) and the callers drift further.
template <class T> __forceinline u32 UpdateThread_ControllerInfo(T *&pKey, T *&pNext, float &frac, EXScriptController *pCtrl, float t, u32 n)
{
    u32 i = pCtrl->m_key;
    pKey += i;
    for (; i < n - 1; i++) {
        if (pKey[1].m_time > t) break;
        pKey++;
    }
    pCtrl->m_key = i;
    if (i < n - 1) {
        pNext = pKey + 1;
        frac = (pNext->m_time - t) / (pNext->m_time - pKey->m_time);
        return 0;
    }
    pNext = pKey;
    frac = 1.0f;
    return 1;
}

float EXScriptController::UpdateThread_CalcTime(float start, float end, u32 mode, float t)
{
    if (mode == 0) return min(end, start + t);
    float len = end - start;
    if (0.0f >= len) return start;
    int n = (int)(t / len);
    float r = t - n * len;
    switch (mode) {
    case 1:
        return r + start;
    case 2:
        if (n & 1) return end - r;
        return r + start;
    }
    return start;
}

u32 EXScriptThread::UpdateThread_AnimDepthController_Func(EXItemAnimator *p, void *pData)
{
    if (p->IsKindOf(EXRUNTIME_CLASS(EXItemAnimator_Anim))) {
        ((EXItemAnimator_Anim *)p)->SetFrameDepth(*(float *)pData);
        return 0;
    }
    return 0;
}

// NEAR: 37 bytes; the item and the count land in edi/ebx instead of ebx/edi.
void EXScriptThread::UpdateThread_TextureSwapController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t)
{
    EXPlacementItem *pItem = m_pAnimator;
    EXGeoScript_TextureSwapList *pList = (EXGeoScript_TextureSwapList *)pHdr->Data();
    int n = pList->m_count;
    EXTexSwapInfo *pSwap = pItem->SetTexSwapInfo();
    if (!pSwap) return;
    for (int i = 0; i < n; i++) {
        EXGeoScript_TextureSwap &s = pList->m_swaps[i];
        float f;
        EXScriptController::UpdateThread_FloatController(&s.m_ctrl, pCtrl, t, f);
        u32 from = s.m_hash | 0x86000000;
        u32 to = s.TextureLookupTble()[(int)f] | 0x86000000;
        pSwap->SetSwap(from, pItem->GeoFile(), to);
    }
}

void EXScriptThread::UpdateThread_AnimDepthController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t)
{
    float v[3];
    if (EXScriptController::UpdateThread_FloatController(pHdr, pCtrl, t, v, 3)) m_activeBits &= ~0x2000;
    m_pAnimator->ApplyFunction(UpdateThread_AnimDepthController_Func, v);
}

void EXScriptThread::UpdateThread_LightRadiusController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t)
{
    if (!m_pAnimator->IsKindOf(EXRUNTIME_CLASS(EXItemAnimator_DynLight))) for (;;) {}
    // The parameter doubles as the output.
    if (EXScriptController::UpdateThread_FloatController(pHdr, pCtrl, t, t)) m_activeBits &= ~0x4000;
    EXDynLight *pLight = ((EXItemAnimator_DynLight *)m_pAnimator)->DynLight();
    if (pLight) pLight->SetOuterRadius(t);
}

// NEAR: 154 bytes; the original evaluates the frame-rate product on the x87 stack (fild/fmul)
// where ours uses SSE.
void EXScriptThread::UpdateThread_TextureAnimController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t)
{
    EXGeoScript_TextureAnimList *pList = (EXGeoScript_TextureAnimList *)pHdr->Data();
    if (!m_pAnimator) return;
    EXGeoFile *pFile = m_pAnimator->GeoFile();
    if (!pFile) return;
    EXGeoHeader *pGeo = pFile->m_pHeader;
    if (!pGeo) return;
    for (u32 i = 0; i < pList->m_count; i++) {
        EXGeoScript_TextureAnim &a = pList->m_anims[i];
        EXTexture *pTex = pGeo->Texture(a.m_hash);
        if (pTex && pTex->m_base.m_pGeoTexture) {
            pTex->m_base.SetFrame((u8)EXScriptController::UpdateThread_CalcTime(a.m_start, pTex->m_base.FrameCount(), a.m_mode,
                pTex->m_base.m_pGeoTexture->FrameRate() * a.m_rate * t));
        }
    }
}

float EXScriptController::UpdateThread_TimeControllers(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, u32)
{
    float time = t;
    if (pHdr->m_keysOff) UpdateThread_FloatController(pHdr, pCtrl, t, time);
    const EXGeoScript_TimeController *p = (const EXGeoScript_TimeController *)pHdr->Data();
    return UpdateThread_CalcTime(p->m_start, p->m_end, p->m_mode, time);
}

// NEAR: 14 bytes; the key count is loaded into dx before the key-pointer add (ours: cx after it).
u32 EXScriptController::UpdateThread_FloatController(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, float &out)
{
    EXGeoScript_FloatController *pKey = pHdr->Keys<EXGeoScript_FloatController>();
    s16 n = pHdr->m_numKeys;
    s8 bDone;
    if (n < 0) {
        bDone = (s8)UpdateThread_ControllerInfoSpline<float>(pKey, &out, pCtrl, t, -(int)n, 1);
        return bDone;
    }
    EXGeoScript_FloatController *pNext;
    float frac;
    bDone = (s8)UpdateThread_ControllerInfo(pKey, pNext, frac, pCtrl, t, n);
    out = ScriptLerp(pKey->m_value, pNext->m_value, frac);
    return bDone;
}

// Multi-value float keys: each key is {time, m values}. Linear keys interpolate between the pair
// around t (from the controller's current key on); negative counts are splines.
u32 EXScriptController::UpdateThread_FloatController(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, float *pOut, int m)
{
    float *pKeys = pHdr->Keys<float>();
    s16 n = pHdr->m_numKeys;
    if (n < 0) return (s8)UpdateThread_ControllerInfoSpline<float>((EXGeoScript_FloatController *)pKeys, pOut, pCtrl, t, -(int)n, m);
    u32 i = pCtrl->m_key;
    const float *pKey = pKeys + i * (m + 1);
    const float *pNext = pKey;
    for (; i < (u32)(n - 1); i++) {
        pNext = pKey + m + 1;
        if (t < pNext[0]) break;
        pKey = pNext;
    }
    pCtrl->m_key = i;
    const float *pB = pKey;
    float frac = 1.0f;
    if ((u32)(n - 1) > i) {
        pB = pNext;
        frac = (pNext[0] - t) / (pNext[0] - pKey[0]);
    }
    for (int j = 0; j < m; j++) pOut[j] = (pKey[1 + j] - pB[1 + j]) * frac + pB[1 + j];
    return (u32)(n - 1) <= i;
}

// Rotation keys {time, quaternion}: spherical interpolation between the pair around t; spline
// keys are normalised afterwards. The key count's sign is ignored for the linear search.
u32 EXScriptController::UpdateThread_QuatController(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, EXVector &out)
{
    float *pKeys = pHdr->Keys<float>();
    s16 n = pHdr->m_numKeys;
    if (n < 0) {
        float q[4];
        s8 bDone = (s8)UpdateThread_ControllerInfoSpline<float>((EXGeoScript_FloatController *)pKeys, q, pCtrl, t, -(int)n, 4);
        float lenSq = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
        out.Set(q[0], q[1], q[2], q[3]);
        if (lenSq >= 0.0f) {
            float len = sqrtf(lenSq);
            if (len > EX_EPSILON_UNIT) {
                float inv = 1.0f / len;
                out.Set(inv * q[0], inv * q[1], inv * q[2], inv * q[3]);
            }
        }
        return bDone;
    }
    u32 count = n < 0 ? -n : n;
    u32 i = pCtrl->m_key;
    const float *pKey = pKeys + i * 5;
    for (; i < count - 1; i++) {
        if (t < pKey[5]) break;
        pKey += 5;
    }
    pCtrl->m_key = i;
    const float *pB;
    float frac;
    if (count - 1 <= i) {
        frac = 1.0f;
        pB = pKey;
    } else {
        frac = (pKey[5] - t) / (pKey[5] - pKey[0]);
        pB = pKey + 5;
    }
    EXMaths::quat_xyzw a, b;
    a.x = pKey[1];
    a.y = pKey[2];
    a.z = pKey[3];
    a.w = pKey[4];
    b.x = pB[1];
    b.y = pB[2];
    b.z = pB[3];
    b.w = pB[4];
    EXMaths::quat_xyzw r = b.slerp(a, frac);
    out.Set(r.x, r.y, r.z, r.w);
    return count - 1 <= i;
}

// NEAR: 103 bytes; same count scheduling, and the original builds the result in two stack
// vec_xyzw temporaries (a by-value vector return) before storing it.
u32 EXScriptController::UpdateThread_VectorController(const EXGeoScriptControllerHeader *pHdr, EXScriptController *pCtrl, float t, EXVector &out)
{
    EXGeoScript_VectorController *pKey = pHdr->Keys<EXGeoScript_VectorController>();
    s16 n = pHdr->m_numKeys;
    s8 bDone;
    if (n < 0) {
        float v[3];
        bDone = (s8)UpdateThread_ControllerInfoSpline<float>((EXGeoScript_FloatController *)pKey, v, pCtrl, t, -n, 3);
        out.Set(v[0], v[1], v[2], 0.0f);
        return bDone;
    }
    EXGeoScript_VectorController *pNext;
    float frac;
    bDone = (s8)UpdateThread_ControllerInfo(pKey, pNext, frac, pCtrl, t, n);
    EXMaths::vec_xyzw r;
    r.x = ScriptLerp(pKey->m_x, pNext->m_x, frac);
    r.y = ScriptLerp(pKey->m_y, pNext->m_y, frac);
    r.z = ScriptLerp(pKey->m_z, pNext->m_z, frac);
    r.w = ScriptLerp(0.0f, 0.0f, frac);
    out = EXVector(r);
    return bDone;
}

// NEAR: 21 bytes; the viewport conversions are interleaved with the stores differently.
void EXScriptThread::UpdateThread_CameraControllers(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t, u32 flags)
{
    EXItemAnimator_Camera *pCam = (EXItemAnimator_Camera *)m_pAnimator;
    float v[4];
    if (flags & 0x8000) {
        EXScriptController::UpdateThread_FloatController(pHdr++, pCtrl++, t, v[0]);
        pCam->SetFocalLength(v[0]);
    }
    if (flags & 0x10000) {
        EXScriptController::UpdateThread_FloatController(pHdr++, pCtrl++, t, v, 2);
        pCam->m_apertureWidth = v[0];
        pCam->m_apertureHeight = v[1];
    }
    if (flags & 0x20000) {
        EXScriptController::UpdateThread_FloatController(pHdr++, pCtrl++, t, v[0]);
        pCam->SetZoom(v[0]);
    }
    if (flags & 0x40000) {
        EXScriptController::UpdateThread_FloatController(pHdr++, pCtrl++, t, v, 4);
        int x = (int)v[0];
        int y = (int)v[1];
        int w = (int)v[2];
        int h = (int)v[3];
        pCam->SetViewport(x, y, w, h);
    }
}

void EXScriptThread::UpdateThread_ExtraControllers(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t, u32 flags)
{
    if (flags & 0x100) {
        if (m_activeBits & 0x100) UpdateThread_ScreenFadeController(pHdr, pCtrl, t);
        pHdr++;
        pCtrl++;
    }
    if (flags & 0x200) {
        if (m_activeBits & 0x200) UpdateThread_LightSetController(pHdr, pCtrl, t);
        pHdr++;
        pCtrl++;
    }
    if (flags & 0x400) {
        if (m_activeBits & 0x400) UpdateThread_VertexSetController(pHdr, pCtrl, t);
        pHdr++;
        pCtrl++;
    }
    if (flags & 0x800) {
        int n = ((EXGeoScript_TextureSwapList *)pHdr->Data())->m_count;
        if (m_activeBits & 0x800) UpdateThread_TextureSwapController(pHdr, pCtrl, t);
        pHdr++;
        pCtrl += n;
    }
    if (flags & 0x1000) {
        if (m_activeBits & 0x1000) UpdateThread_TextureAnimController(pHdr, pCtrl, t);
        pHdr++;
        pCtrl++;
    }
    if (flags & 0x2000) {
        if (m_activeBits & 0x2000) UpdateThread_AnimDepthController(pHdr, pCtrl, t);
        pHdr++;
        pCtrl++;
    }
    if (flags & 0x4000) {
        if (m_activeBits & 0x4000) UpdateThread_LightRadiusController(pHdr, pCtrl, t);
        pHdr++;
        pCtrl++;
    }
}

// NEAR: 166 bytes; x87 temporaries and the m_pAnimator reload differ (SetVertexSet's float
// argument is evaluated on the x87 stack in the original).
void EXScriptThread::UpdateThread_VertexSetController(EXGeoScriptControllerHeader *&pHdr, EXScriptController *&pCtrl, float t)
{
    EXGeoScriptControllerHeader *h = pHdr;
    EXGeoScript_SetController *d = (EXGeoScript_SetController *)h->Data();
    if (h->m_numKeys == 0) {
        float rate = d->m_rate;
        float f = 0.0f;
        if (rate != 0.0f) f = 1.0f / rate;
        t = f * t;
    } else if (EXScriptController::UpdateThread_FloatController(h, pCtrl, t, t)) {
        m_activeBits &= ~0x200;
    }
    if (d->m_count == 0) {
        int i = (int)t;
        m_pAnimator->m_renderInfo.SetVertexSet((short)i, (short)(i + 1), t - (short)i);
        m_pAnimator->m_renderInfo.RenderFlagsRef().BitSet(0x1000);
        return;
    }
    int last = d->m_count - 1;
    t = EXScriptController::UpdateThread_CalcTime(0.0f, (float)last, d->m_mode, t);
    int i = (int)t;
    t -= i;
    int j = min(last, i + 1);
    EXPlacementItem *pItem = m_pAnimator;
    pItem->m_renderInfo.SetVertexSet(d->m_sets[i], d->m_sets[j], t);
    pItem->m_renderInfo.RenderFlagsRef().BitSet(0x1000);
}
