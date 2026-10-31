#include "EXGeoAnim.h"
#include "EXMaths.h"
#include "EXBit128.h"
#include "EXVector.h"
#include "EXScratchPad.h"
#include "EXGeoHeader.h"
#include "EXGeoEntity.h"
#include "EXMemory.h"
#include "xbd3d8.h"
#include <math.h>

// Animation stream decompression (XBGeoAnim.obj).
//
// Each animated channel is stored as a polynomial curve of up to degree 5 per key span. The
// decoder gathers the six coefficient rows of up to four channels at a time into a _RESULTS
// block (GetVal reads the coefficients the span's bit mask says are present, zeroing the rest),
// then evaluates sum(row[k] * t^k) with the powers of t from SetCoefs.
//

struct _RESULTS {
    float v[6][4];      // [coefficient][channel]
};

static float s_Coef0[6];            // second frame of an interpolated decode
static float s_Coef[6];             // 1, t, t^2 .. t^5
static int s_Chan[4];               // channel kinds of the gathered group (see Handle_nChannelsA1)
static _RESULTS s_Res0;
static _RESULTS s_Res1;

// A vertex buffer header over memory that is already physical (the pointer's top bit cleared).
inline void *CreatePhysicalVertexBuffer(void *pVB, void *pData, unsigned int *ppOut)
{
    ((u32 *)pVB)[0] = 1;
    ((u32 *)pVB)[1] = (u32)pData & 0x7fffffff;
    ((u32 *)pVB)[2] = 0;
    *(void **)ppOut = pVB;
    return pVB;
}

// Reads the coefficients present in mask (bit k: coefficient k stored) for channel j.
__forceinline void GetVal(const unsigned char *&p, const unsigned int &mask, _RESULTS *const r, unsigned int &j)
{
    const float *f = (const float *)p;
    switch (mask) {
    case 0x3f:
        r->v[0][j] = f[0];
        r->v[1][j] = f[1];
        r->v[2][j] = f[2];
        r->v[3][j] = f[3];
        r->v[4][j] = f[4];
        r->v[5][j] = f[5];
        break;
    case 0x1f:
        r->v[0][j] = f[0];
        r->v[1][j] = f[1];
        r->v[2][j] = f[2];
        r->v[3][j] = f[3];
        r->v[4][j] = f[4];
        r->v[5][j] = 0.0f;
        break;
    case 0xf:
        r->v[0][j] = f[0];
        r->v[1][j] = f[1];
        r->v[2][j] = f[2];
        r->v[3][j] = f[3];
        r->v[5][j] = 0.0f;
        r->v[4][j] = 0.0f;
        break;
    case 7:
        r->v[0][j] = f[0];
        r->v[1][j] = f[1];
        r->v[2][j] = f[2];
        r->v[5][j] = 0.0f;
        r->v[4][j] = 0.0f;
        r->v[3][j] = 0.0f;
        break;
    case 3:
        r->v[0][j] = f[0];
        r->v[1][j] = f[1];
        r->v[5][j] = 0.0f;
        r->v[4][j] = 0.0f;
        r->v[3][j] = 0.0f;
        r->v[2][j] = 0.0f;
        break;
    case 1:
        r->v[0][j] = f[0];
        r->v[5][j] = 0.0f;
        r->v[4][j] = 0.0f;
        r->v[3][j] = 0.0f;
        r->v[2][j] = 0.0f;
        r->v[1][j] = 0.0f;
        break;
    default: {
        unsigned int m = mask;
        for (int k = 0; k < 6; k++) {
            if (m & 1)
                r->v[k][j] = *f++;
            else
                r->v[k][j] = 0.0f;
            m >>= 1;
        }
        break;
    }
    }
}

__forceinline void ClrVal(_RESULTS *const r, unsigned int j)
{
    r->v[5][j] = 0.0f;
    r->v[4][j] = 0.0f;
    r->v[3][j] = 0.0f;
    r->v[2][j] = 0.0f;
    r->v[1][j] = 0.0f;
    r->v[0][j] = 0.0f;
}

inline unsigned int CountBitsSet8(unsigned int v)
{
    v = ((v >> 1) & 0x55555555) + (v & 0x55555555);
    v = ((v >> 2) & 0x33333333) + (v & 0x33333333);
    return ((v >> 4) & 0x0f0f0f0f) + (v & 0x0f0f0f0f);
}

inline unsigned int GetData(const unsigned char *&p) { return *(const u16 *)p; }
inline unsigned int GetData(const unsigned char *&p, unsigned int i) { return ((const u16 *)p)[i]; }

// Powers of t = frame / frames for the curve evaluation.
inline void SetCoefs(EXMaths::vec_xyzw *const c, int frame, int frames)
{
    float t = (float)frame / (float)frames;
    float *f = (float *)c;
    f[0] = 1.0f;
    float tt = t * t;
    f[2] = tt;
    tt *= t;
    f[3] = tt;
    tt *= t;
    f[4] = tt;
    tt *= t;
    f[1] = t;
    f[5] = tt;
}

// Evaluates the gathered morph channels and writes them out (n channels).
__forceinline void Handle_nChannelsM1(unsigned int &n, float *&pOut)
{
    for (unsigned int i = 0; i < n; i += 16) {
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[1][i] * s_Coef[1];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[2][i] * s_Coef[2];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[3][i] * s_Coef[3];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[4][i] * s_Coef[4];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[5][i] * s_Coef[5];
    }
    for (unsigned int i = 0; i < n; i++) *pOut++ = s_Res1.v[0][i];
    n = 0;
}

// As M1 for two frames (s_Res1 with s_Coef, s_Res0 with s_Coef0), blended by frac.
__forceinline void Handle_nChannelsM3(unsigned int &n, float *&pOut, float &frac)
{
    for (unsigned int i = 0; i < n; i += 16) {
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[1][i] * s_Coef[1];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[2][i] * s_Coef[2];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[3][i] * s_Coef[3];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[4][i] * s_Coef[4];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[5][i] * s_Coef[5];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[1][i] * s_Coef0[1];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[2][i] * s_Coef0[2];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[3][i] * s_Coef0[3];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[4][i] * s_Coef0[4];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[5][i] * s_Coef0[5];
    }
    for (unsigned int i = 0; i < n; i++) {
        float a = s_Res1.v[0][i];
        *pOut++ = (s_Res0.v[0][i] - a) * frac + a;
    }
    n = 0;
}

// Motion channels: evaluates the group (as M1) and routes each value by its kind: 5+ position x,
// 4 position y, 3 position z (w = 1, next position), 2 rotation x, 1 rotation y, 0 rotation z
// (w rebuilt from the unit length; z >= 2 is stored as z + 4 and means w is negative).
__forceinline void Handle_nChannelsA1(unsigned int &n, EXMaths::vec_xyzw *&pPos, EXMaths::vec_xyzw *&pRot)
{
    for (unsigned int i = 0; i < n; i += 16) {
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[1][i] * s_Coef[1];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[2][i] * s_Coef[2];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[3][i] * s_Coef[3];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[4][i] * s_Coef[4];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[5][i] * s_Coef[5];
    }
    for (unsigned int i = 0; i < n; i++) {
        float v = s_Res1.v[0][i];
        switch (s_Chan[i]) {
        case 4:
            pPos->y = v;
            break;
        case 3:
            pPos->z = v;
            pPos->w = 1.0f;
            pPos++;
            break;
        case 2:
            pRot->x = v;
            break;
        case 1:
            pRot->y = v;
            break;
        case 0:
            if (v >= 2.0f) {
                pRot->z = v - 4.0f;
                pRot->w = 0.0f;
                float w = 1.0f - pRot->Dot(*pRot);
                if (w > 0.0f) w = -(float)sqrt(w);
                pRot->w = w;
            } else {
                pRot->z = v;
                pRot->w = 0.0f;
                float w = 1.0f - pRot->Dot(*pRot);
                if (w > 0.0f) w = (float)sqrt(w);
                pRot->w = w;
            }
            pRot++;
            break;
        default:
            pPos->x = v;
            break;
        }
    }
    n = 0;
}

// Two frames blended by frac: positions linearly, rotations by slerp through qa / qb.
__forceinline void Handle_nChannelsA3(unsigned int &n, EXMaths::vec_xyzw *&pPos, EXMaths::vec_xyzw *&pRot,
                                      float &frac, EXMaths::vec_xyzw &qa, EXMaths::vec_xyzw &qb)
{
    for (unsigned int i = 0; i < n; i += 16) {
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[1][i] * s_Coef[1];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[2][i] * s_Coef[2];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[3][i] * s_Coef[3];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[4][i] * s_Coef[4];
        *(EXVector *)&s_Res1.v[0][i] += *(EXVector *)&s_Res1.v[5][i] * s_Coef[5];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[1][i] * s_Coef0[1];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[2][i] * s_Coef0[2];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[3][i] * s_Coef0[3];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[4][i] * s_Coef0[4];
        *(EXVector *)&s_Res0.v[0][i] += *(EXVector *)&s_Res0.v[5][i] * s_Coef0[5];
    }
    for (unsigned int i = 0; i < n; i++) {
        float a = s_Res1.v[0][i];
        float b = s_Res0.v[0][i];
        switch (s_Chan[i]) {
        case 4:
            pPos->y = (b - a) * frac + a;
            break;
        case 3:
            pPos->z = (b - a) * frac + a;
            pPos->w = 1.0f;
            pPos++;
            break;
        case 2:
            qa.x = a;
            qb.x = b;
            break;
        case 1:
            qa.y = a;
            qb.y = b;
            break;
        case 0: {
            if (a >= 2.0f) {
                qa.z = a - 4.0f;
                qa.w = 0.0f;
                float w = 1.0f - qa.DotXYZ(qa);
                if (w > 0.0f) w = -(float)sqrt(w);
                qa.w = w;
            } else {
                qa.z = a;
                qa.w = 0.0f;
                float w = 1.0f - qa.DotXYZ(qa);
                if (w > 0.0f) w = (float)sqrt(w);
                qa.w = w;
            }
            if (b >= 2.0f) {
                qb.z = b - 4.0f;
                qb.w = 0.0f;
                float w = 1.0f - qb.DotXYZ(qb);
                if (w > 0.0f) w = -(float)sqrt(w);
                qb.w = w;
            } else {
                qb.z = b;
                qb.w = 0.0f;
                float w = 1.0f - qb.DotXYZ(qb);
                if (w > 0.0f) w = (float)sqrt(w);
                qb.w = w;
            }
            *pRot = ((EXMaths::quat_xyzw &)qa).slerp((EXMaths::quat_xyzw &)qb, frac);
            pRot++;
            break;
        }
        default:
            pPos->x = (b - a) * frac + a;
            break;
        }
    }
    n = 0;
}

// Morph channel stream layout, per channel: u16 value count and u16 header length, then
// (frames - 1) >> 7 seek entries of three u16 (value offset, frame, span offset), then the spans:
// u16 (frames << 6 | coefficient mask). The coefficient floats for the channels follow the
// u16 table that starts the stream.
void EXGeoBaseAnim::DecompressMorphData(const unsigned char *pData, int numChannels, int frame,
                                        ex::EXBit128 &flags, float *pOut, const EXAnimStreamInfo &info,
                                        signed char) const
{
    EXScratchPad pad;
    int f = m_NumFrames - 1;
    if (frame < f) f = frame <= 0 ? 0 : frame;
    SetCoefs((EXMaths::vec_xyzw *)s_Coef, f, m_NumFrames);
    const unsigned char *pHdr = pData;
    const unsigned char *pVals = pData + GetData(pHdr) * 4;
    pHdr += 2;
    unsigned int j = 0;
    int numSeek = (info.m_NumFrames - 1) >> 7;
    int seek = info.m_SeekFrame >> 7;
    int m = 0;
    do {
        const unsigned char *pNextVals = pVals + GetData(pHdr) * 4;
        const unsigned char *pNextHdr = pHdr + GetData(pHdr, 1) * 2;
        if (!flags.IsSet(m)) {
            pOut++;
        } else {
            unsigned int end;
            const unsigned char *pSpan;
            if (seek) {
                pVals += GetData(pHdr, seek * 3 - 1) * 4;
                end = GetData(pHdr, seek * 3);
                pSpan = pHdr + GetData(pHdr, seek * 3 + 1) * 2;
            } else {
                end = info.m_StartFrame;
                pSpan = pHdr + numSeek * 6 + 4;
            }
            unsigned int w = GetData(pSpan);
            unsigned int mask = w & 0x3f;
            end += w >> 6;
            while ((unsigned int)f > end) {
                pVals += CountBitsSet8(mask) * 4;
                pSpan += 2;
                w = GetData(pSpan);
                mask = w & 0x3f;
                end += w >> 6;
            }
            GetVal(pVals, mask, &s_Res1, j);
            j++;
            if (j == 4) Handle_nChannelsM1(j, pOut);
        }
        pHdr = pNextHdr;
        pVals = pNextVals;
    } while (++m < numChannels);
    if (j) Handle_nChannelsM1(j, pOut);
}

// Between two frames: both are decoded (the second frame's coefficients come from the next span
// when it starts after the first frame) and blended by the fraction.
void EXGeoBaseAnim::DecompressMorphData(const unsigned char *pData, int numChannels, float frame,
                                        ex::EXBit128 &flags, float *pOut, const EXAnimStreamInfo &info,
                                        signed char) const
{
    EXScratchPad pad;
    int i0 = (int)frame;
    float frac = frame - (float)i0;
    int f0 = m_NumFrames - 1;
    if (i0 < f0) f0 = i0 <= 0 ? 0 : i0;
    SetCoefs((EXMaths::vec_xyzw *)s_Coef, f0, m_NumFrames);
    int f1 = m_NumFrames - 1;
    if (f0 + 1 < f1) f1 = f0 + 1 <= 0 ? 0 : f0 + 1;
    SetCoefs((EXMaths::vec_xyzw *)s_Coef0, f1, m_NumFrames);
    const unsigned char *pHdr = pData;
    const unsigned char *pVals = pData + GetData(pHdr) * 4;
    pHdr += 2;
    unsigned int j = 0;
    int numSeek = (info.m_NumFrames - 1) >> 7;
    int seek = info.m_SeekFrame >> 7;
    int m = 0;
    do {
        const unsigned char *pNextVals = pVals + GetData(pHdr) * 4;
        const unsigned char *pNextHdr = pHdr + GetData(pHdr, 1) * 2;
        if (!flags.IsSet(m)) {
            pOut++;
        } else {
            unsigned int end;
            const unsigned char *pSpan;
            if (seek) {
                pVals += GetData(pHdr, seek * 3 - 1) * 4;
                end = GetData(pHdr, seek * 3);
                pSpan = pHdr + GetData(pHdr, seek * 3 + 1) * 2;
            } else {
                end = info.m_StartFrame;
                pSpan = pHdr + numSeek * 6 + 4;
            }
            unsigned int w = GetData(pSpan);
            unsigned int mask = w & 0x3f;
            end += w >> 6;
            while ((unsigned int)f0 > end) {
                pVals += CountBitsSet8(mask) * 4;
                pSpan += 2;
                w = GetData(pSpan);
                mask = w & 0x3f;
                end += w >> 6;
            }
            GetVal(pVals, mask, &s_Res1, j);
            if ((unsigned int)f1 > end) {
                pVals += CountBitsSet8(mask) * 4;
                mask = GetData(pSpan, 1) & 0x3f;
            }
            GetVal(pVals, mask, &s_Res0, j);
            j++;
            if (j == 4) Handle_nChannelsM3(j, pOut, frac);
        }
        pHdr = pNextHdr;
        pVals = pNextVals;
    } while (++m < numChannels);
    if (j) Handle_nChannelsM3(j, pOut, frac);
}

// Motion stream: per bone 3 rotation channels (6 with position when pos is set), laid out as the
// morph channels. Bones without animation (anim clear) keep zeroed coefficients, which gives an
// identity rotation and a zero position.
void EXGeoBaseAnim::DecompressData(const unsigned char *pData, int numBones, int frame, ex::EXBit128 &anim,
                                   ex::EXBit128 &pos, EXMaths::vec_xyzw *pPos, EXMaths::vec_xyzw *pRot,
                                   const EXAnimStreamInfo &info, signed char) const
{
    EXScratchPad pad;
    int f = m_NumFrames - 1;
    if (frame < f) f = frame <= 0 ? 0 : frame;
    SetCoefs((EXMaths::vec_xyzw *)s_Coef, f, m_NumFrames);
    const unsigned char *pHdr = pData;
    const unsigned char *pVals = pData + GetData(pHdr) * 4;
    pHdr += 2;
    unsigned int j = 0;
    int numSeek = (info.m_NumFrames - 1) >> 7;
    int seek = info.m_SeekFrame >> 7;
    int m = 0;
    do {
        int kind = pos.IsSet(m) ? 5 : 2;
        if (!anim.IsSet(m)) {
            do {
                ClrVal(&s_Res1, j);
                s_Chan[j] = kind;
                j++;
                pVals += GetData(pHdr) * 4;
                pHdr += GetData(pHdr, 1) * 2;
                if (j == 4) Handle_nChannelsA1(j, pPos, pRot);
            } while (kind--);
        } else {
            do {
                const unsigned char *pNextVals = pVals + GetData(pHdr) * 4;
                const unsigned char *pNextHdr = pHdr + GetData(pHdr, 1) * 2;
                unsigned int end;
                const unsigned char *pSpan;
                if (seek) {
                    pVals += GetData(pHdr, seek * 3 - 1) * 4;
                    end = GetData(pHdr, seek * 3);
                    pSpan = pHdr + GetData(pHdr, seek * 3 + 1) * 2;
                } else {
                    end = info.m_StartFrame;
                    pSpan = pHdr + numSeek * 6 + 4;
                }
                unsigned int w = GetData(pSpan);
                unsigned int mask = w & 0x3f;
                end += w >> 6;
                while ((unsigned int)f > end) {
                    pVals += CountBitsSet8(mask) * 4;
                    pSpan += 2;
                    w = GetData(pSpan);
                    mask = w & 0x3f;
                    end += w >> 6;
                }
                GetVal(pVals, mask, &s_Res1, j);
                s_Chan[j] = kind;
                j++;
                if (j == 4) Handle_nChannelsA1(j, pPos, pRot);
                pHdr = pNextHdr;
                pVals = pNextVals;
            } while (kind--);
        }
    } while (++m < numBones);
    if (j) Handle_nChannelsA1(j, pPos, pRot);
}

// Between two frames: both decoded, positions lerped and rotations slerped by the fraction.
void EXGeoBaseAnim::DecompressData(const unsigned char *pData, int numBones, float frame, ex::EXBit128 &anim,
                                   ex::EXBit128 &pos, EXMaths::vec_xyzw *pPos, EXMaths::vec_xyzw *pRot,
                                   const EXAnimStreamInfo &info, signed char) const
{
    EXScratchPad pad;
    EXMaths::vec_xyzw qa, qb;
    int i0 = (int)frame;
    float frac = frame - (float)i0;
    int f0 = m_NumFrames - 1;
    if (i0 < f0) f0 = i0 <= 0 ? 0 : i0;
    SetCoefs((EXMaths::vec_xyzw *)s_Coef, f0, m_NumFrames);
    int f1 = m_NumFrames - 1;
    if (f0 + 1 < f1) f1 = f0 + 1 <= 0 ? 0 : f0 + 1;
    SetCoefs((EXMaths::vec_xyzw *)s_Coef0, f1, m_NumFrames);
    const unsigned char *pHdr = pData;
    const unsigned char *pVals = pData + GetData(pHdr) * 4;
    pHdr += 2;
    unsigned int j = 0;
    int numSeek = (info.m_NumFrames - 1) >> 7;
    int seek = info.m_SeekFrame >> 7;
    int m = 0;
    do {
        int kind = pos.IsSet(m) ? 5 : 2;
        if (!anim.IsSet(m)) {
            do {
                ClrVal(&s_Res1, j);
                ClrVal(&s_Res0, j);
                s_Chan[j] = kind;
                j++;
                pVals += GetData(pHdr) * 4;
                pHdr += GetData(pHdr, 1) * 2;
                if (j == 4) Handle_nChannelsA3(j, pPos, pRot, frac, qa, qb);
            } while (kind--);
        } else {
            do {
                const unsigned char *pNextVals = pVals + GetData(pHdr) * 4;
                const unsigned char *pNextHdr = pHdr + GetData(pHdr, 1) * 2;
                unsigned int end;
                const unsigned char *pSpan;
                if (seek) {
                    pVals += GetData(pHdr, seek * 3 - 1) * 4;
                    end = GetData(pHdr, seek * 3);
                    pSpan = pHdr + GetData(pHdr, seek * 3 + 1) * 2;
                } else {
                    end = info.m_StartFrame;
                    pSpan = pHdr + numSeek * 6 + 4;
                }
                unsigned int w = GetData(pSpan);
                unsigned int mask = w & 0x3f;
                end += w >> 6;
                while ((unsigned int)f0 > end) {
                    pVals += CountBitsSet8(mask) * 4;
                    pSpan += 2;
                    w = GetData(pSpan);
                    mask = w & 0x3f;
                    end += w >> 6;
                }
                GetVal(pVals, mask, &s_Res1, j);
                if ((unsigned int)f1 > end) {
                    pVals += CountBitsSet8(mask) * 4;
                    mask = GetData(pSpan, 1) & 0x3f;
                }
                GetVal(pVals, mask, &s_Res0, j);
                s_Chan[j] = kind;
                j++;
                if (j == 4) Handle_nChannelsA3(j, pPos, pRot, frac, qa, qb);
                pHdr = pNextHdr;
                pVals = pNextVals;
            } while (kind--);
        }
    } while (++m < numBones);
    if (j) Handle_nChannelsA3(j, pPos, pRot, frac, qa, qb);
}

void *XBHeap1Alloc(int size, int flags);

// Skin weights of one piece of a skinned entity: the vertex buffer data sits behind a relptr at +8.
struct EXGeoSkinWeights {
    u32 m_00[2];
    ex::EXRelPtr<u8> m_Data;    // 0x08
};

// A weight vertex buffer header for one skinned entity piece (0x18).
struct XBSkinWeightVB {
    u32 m_Index;                    // 0x00 piece within the group
    u32 m_Group;                    // 0x04 entity group
    xb::D3DVertexBuffer *m_pVB;     // 0x08
    xb::D3DVertexBuffer m_VB;       // 0x0c
};

// The weight relptr table of an entity group (relptr at +0, count at +4).
static inline ex::EXRelPtr<EXGeoSkinWeights> *SkinWeights(const EXGeoSkinEntityRef &r)
{
    return *(const ex::EXRelPtr<ex::EXRelPtr<EXGeoSkinWeights> > *)&r.m_00[0];
}

xb::D3DVertexBuffer *EXGeoAnimSkin::GetWeightVertexBuffer(unsigned int i)
{
    EXGeoSkinWeights *pW = SkinWeights(m_EntityGroups[0].Data()[m_pWeightVB[i].m_Group])[m_pWeightVB[i].m_Index];
    CreatePhysicalVertexBuffer(&m_pWeightVB[i].m_VB, pW->m_Data, (unsigned int *)&m_pWeightVB[i].m_pVB);
    return m_pWeightVB[i].m_pVB;
}

void EXGeoAnimSkin::OnCreate(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    EXGeoCommonObject::OnCreate(pHdr, pObj);
    m_pWeightVB = 0;
    EXGeoEntityHeader *pEnts = pHdr->EntityList().Find(0);
    EXGeoSkinEntityRef *pRefs = m_EntityGroups[0].Data();
    int numGroups = LinkedSkinCount() + SkinCount();
    int count = 0;
    for (int n = 0; n < numGroups; n++) count += pRefs[n].m_00[1];
    m_pWeightVB = (XBSkinWeightVB *)XBHeap1Alloc(count * sizeof(XBSkinWeightVB), 0);
    memset(m_pWeightVB, 0, count * sizeof(XBSkinWeightVB));
    int base = 0;
    for (int g = 0; g < numGroups; g++) {
        EXGeoSkinEntityRef &ref = pRefs[g];
        EXGeoEntity *pEnt = (&pEnts[ref.m_EntityHash & 0xffffff])->Object(0);
        s8 bSplit = pEnt->IsKindOf(&EXGeoSplitEntity::classEXGeoSplitEntity);
        int num = ref.m_00[1];
        for (int k = 0; k < num; k++) {
            EXGeoEntity *pSub = pEnt;
            if (bSplit) pSub = ((ex::EXRelPtr<EXGeoEntity> *)((u8 *)pEnt + 0x5c))[k];
            EXGeoSkinWeights *pW = SkinWeights(ref)[k];
            if (pSub->VertexCount()) {
                pSub->CreatePhysicalVertexBuffer(&m_pWeightVB[base + k].m_VB, pW->m_Data, (unsigned int *)&m_pWeightVB[base + k].m_pVB);
                m_pWeightVB[base + k].m_Index = k;
                m_pWeightVB[base + k].m_Group = g;
            }
        }
        base += num;
    }
}

void EXGeoAnimSkin::OnDelete(EXGeoHeader *pHdr, EXGeoObjectHeader *pObj)
{
    EXGeoCommonObject::OnDelete(pHdr, pObj);
    if (m_pWeightVB) {
        int numGroups = LinkedSkinCount() + SkinCount();
        EXGeoSkinEntityRef *pRefs = m_EntityGroups[0].Data();
        int count = 0;
        for (int n = 0; n < numGroups; n++) count += pRefs[n].m_00[1];
        for (int i = 0; i < count; i++) EXGeoEntity::ReleasePhysicalVertexBuffer((unsigned int *)m_pWeightVB[i].m_pVB);
        EXFree(m_pWeightVB);
    }
}
