#include "EXTexture.h"
#include "EXMemory.h"
#include "EXNew.h"
#include "EXStdFuncs.h"

extern "C" double __cdecl sin(double);
extern "C" double __cdecl cos(double);
#pragma intrinsic(sin, cos)

static const float kScrollScale = 30000.0f;
static const float kInvScrollScale = 1.0f / 30000.0f;

EXBaseTexture::EXBaseTexture()
{
    m_pHeader = 0;
    m_pGeoTexture = 0;
    m_HashCode = 0;
    m_Frame = 0;
    m_11 = 0;
    m_UOffset = 0.0f;
    m_VOffset = 0.0f;
    m_ScrollRateU = 0.0f;
    m_ScrollRateV = 0.0f;
    m_ScrollSet = 0;
    m_Paused = 0;
    m_Playing = 0;
    m_OnFirstFrame = 1;
    m_OnLastFrame = 0;
    m_13 = 0;
}

EXBaseTexture::~EXBaseTexture() {}

float EXBaseTexture::ScrollRateU() const
{
    float r = (float)(int)(m_ScrollRateU * kScrollScale);
    return (r != 0.0f ? 1.0f / r : 0.0f) * 500.0f;
}

float EXBaseTexture::ScrollRateV() const
{
    float r = (float)(int)(m_ScrollRateV * kScrollScale);
    return (r != 0.0f ? 1.0f / r : 0.0f) * 500.0f;
}

void EXBaseTexture::StopScroll()
{
    m_ScrollRateU = 0.0f;
    m_ScrollRateV = 0.0f;
    m_ScrollSet = 1;
}

void EXBaseTexture::ResetScrollRate()
{
    EXGeoTexture *pTex = m_pGeoTexture;
    if (pTex) {
        if (pTex->m_ScrollU || pTex->m_ScrollV) {
            m_ScrollRateU = pTex->m_ScrollU * kInvScrollScale;
            m_ScrollRateV = pTex->m_ScrollV * kInvScrollScale;
        }
        m_ScrollSet = 0;
    }
}

void EXBaseTexture::ReverseScroll()
{
    m_ScrollRateU = -m_ScrollRateU;
    m_ScrollRateV = -m_ScrollRateV;
    m_ScrollSet = 1;
}

unsigned char EXBaseTexture::FrameCount() const
{
    if (!m_pGeoTexture) return 1;
    return m_pGeoTexture->m_FrameCount;
}

void EXBaseTexture::PauseAnimation() { m_Paused = 1; }

void EXBaseTexture::ResumeAnimation()
{
    m_Paused = 0;
    m_Playing = 1;
}

void EXTextureArray::Release()
{
    if (m_ppBlocks) {
        for (u32 b = 0; b < m_NumBlocks; b++) {
            u32 n = b == m_NumBlocks - 1 ? m_LastBlockCount : 64;
            EXTexture *p = m_ppBlocks[b];
            for (; n; n--, p++) p->~EXTexture();
            EXFree(m_ppBlocks[b]);
        }
        EXFree(m_ppBlocks);
        m_ppBlocks = 0;
    }
    if (m_pBlockUse) {
        EXFree(m_pBlockUse);
        m_pBlockUse = 0;
    }
    m_NumBlocks = 0;
    m_NumUsed = 0;
}

// NEAR: register allocation differs (VC keeps 0 in esi; retail uses immediates), rest matches.
signed char EXTextureArray::InitBlockUse()
{
    if (m_pBlockUse) return 1;
    if (!m_NumBlocks) return 1;
    m_pBlockUse = (ex::EXBit128 *)_EXAlloc(m_NumBlocks * sizeof(ex::EXBit128), 0);
    if (!m_pBlockUse) return 0;
    for (u32 b = 0; b < m_NumBlocks; b++) {
        if (b < m_NumBlocks - 1) {
            m_pBlockUse[b].SetAll();
        } else {
            m_pBlockUse[b].ClrAll();
            for (int i = 0; i < m_LastBlockCount; i++) {
                ex::EXBit128 *pUse = &m_pBlockUse[b];
                pUse->m_bits[i / 32] |= 1 << (i & 31);
            }
        }
    }
    if (m_LastBlockCount != 64) {
        u32 last = m_NumBlocks - 1;
        EXTexture *pNew = (EXTexture *)_EXAlloc(64 * sizeof(EXTexture), 0);
        if (!pNew) return 0;
        memcpy(pNew, m_ppBlocks[last], m_LastBlockCount * sizeof(EXTexture));
        EXTexture *p = &m_ppBlocks[last][m_LastBlockCount];
        for (int n = 64 - m_LastBlockCount; n; n--, p++) new (p) EXTexture;
        EXFree(m_ppBlocks[last]);
        m_ppBlocks[last] = pNew;
    }
    return 1;
}

// NEAR: retail shares one zero register (ebp) between the construct loop null check, the
// EXBit128 clear and slot = 0; ours keeps separate zeros. Logic and layout match.
int EXTextureArray::NewTexture()
{
    if (!m_pBlockUse && !InitBlockUse()) return -1;
    int block = -1;
    int slot = -1;
    for (u32 b = 0; b < m_NumBlocks; b++) {
        ex::EXBit128 *pUse = &m_pBlockUse[b];
        for (u32 i = 0; i < 64; i++) {
            if (!((pUse->m_bits[(int)i / 32] >> (i & 31)) & 1)) {
                block = b;
                slot = i;
                break;
            }
        }
    }
    if (block < 0) {
        EXTexture **ppBlocks = (EXTexture **)_EXAlloc(m_NumBlocks * 4 + 4, 0);
        if (!ppBlocks) return -1;
        ppBlocks[m_NumBlocks] = (EXTexture *)_EXAlloc(64 * sizeof(EXTexture), 0);
        if (!ppBlocks[m_NumBlocks]) {
            EXFree(ppBlocks);
            return -1;
        }
        ex::EXBit128 *pUse = (ex::EXBit128 *)_EXAlloc((m_NumBlocks + 1) * sizeof(ex::EXBit128), 0);
        if (!pUse) {
            EXFree(ppBlocks);
            return -1;
        }
        if (m_ppBlocks) {
            memcpy(ppBlocks, m_ppBlocks, m_NumBlocks * 4);
            EXFree(m_ppBlocks);
        }
        m_ppBlocks = ppBlocks;
        if (m_pBlockUse) {
            memcpy(pUse, m_pBlockUse, m_NumBlocks * sizeof(ex::EXBit128));
            EXFree(m_pBlockUse);
        }
        m_pBlockUse = pUse;
        EXTexture *p = m_ppBlocks[m_NumBlocks];
        for (int n = 64; n; n--, p++) new (p) EXTexture;
        m_pBlockUse[m_NumBlocks].ClrAll();
slot = 0;
        block = m_NumBlocks++;
        m_LastBlockCount = 64;
    }
    EXTexture *pTex = &m_ppBlocks[block][slot];
    pTex->~EXTexture();
    new (pTex) EXTexture;
    ex::EXBit128 *pUse = &m_pBlockUse[block];
    pUse->m_bits[slot / 32] |= 1 << (slot & 31);
    int index = block * 64 + slot;
    if ((u32)(index + 1) > m_NumUsed) m_NumUsed = index + 1;
    return index;
}

signed char EXTextureArray::DeleteTexture(unsigned int i)
{
    u32 block = i >> 6;
    u32 slot = i & 0x3f;
    EXTexture *pTex = &m_ppBlocks[block][slot];
    pTex->~EXTexture();
    new (pTex) EXTexture;
    ex::EXBit128 *pUse = &m_pBlockUse[block];
    pUse->m_bits[(int)slot / 32] &= ~(1 << (slot & 31));
    return 1;
}

int EXTextureArray::FindTextureIndex(const EXTexture *p) const
{
    for (u32 b = 0; b < m_NumBlocks; b++) {
        if (p >= m_ppBlocks[b] && p < m_ppBlocks[b] + 64) return b * 64 + (p - m_ppBlocks[b]);
    }
    return -1;
}

void EXBaseTexture::SetScrollRate(float u, float v)
{
    int iu = (int)((u != 0.0f ? 1.0f / u : 0.0f) * 500.0f);
    int su = max(-0x8000, min(0x7fff, iu));
    int iv = (int)((v != 0.0f ? 1.0f / v : 0.0f) * 500.0f);
    int sv = max(-0x8000, min(0x7fff, iv));
    m_ScrollRateU = su * kInvScrollScale;
    m_ScrollRateV = sv * kInvScrollScale;
    m_ScrollSet = 1;
}

void EXBaseTexture::SetScrollRateAngular(float angle, float speed)
{
    float a = angle * 0.017453292f;
    m_ScrollRateU = (int)-(sin(a) * speed) * kInvScrollScale;
    m_ScrollRateV = (int)(cos(a) * speed) * kInvScrollScale;
    m_ScrollSet = 1;
}

void EXBaseTexture::RestartAnimation(unsigned char frame)
{
    EXGeoTextureHeader *pHdr = m_pHeader;
    if (pHdr) {
        EXGeoTexture *pTex = pHdr->m_Addr[0].m_pObject;
        if (pTex && pTex->m_FrameCount != 1) {
            u8 last = pTex->m_FrameCount - 1;
            frame = min(frame, last);
            m_Paused = 0;
            m_Playing = 0;
            m_OnFirstFrame = frame == 0;
            m_Frame = frame;
            m_11 = 0;
            m_OnLastFrame = frame == pTex->m_FrameCount - 1;
            if (m_OnFirstFrame && (pHdr->Flags() & 0x1000000)) m_Paused = 1;
            if (m_OnLastFrame && (pHdr->Flags() & 0x2000000)) m_Paused = 1;
        }
    }
}

void EXBaseTexture::UpdatePtrs(EXGeoTextureHeader *pHdr, signed char force)
{
    EXGeoTexture *pTex = pHdr->Object(0);
    force |= m_pGeoTexture != pTex;
    m_pHeader = pHdr;
    m_pGeoTexture = pTex;
    Update(force);
}

signed char EXTextureArray::Init(unsigned int n)
{
    if (!n) {
        Release();
        return 1;
    }
    m_NumUsed = n;
    m_NumBlocks = (n + 63) >> 6;
    m_LastBlockCount = n - ((m_NumBlocks - 1) << 6);
    m_ppBlocks = (EXTexture **)_EXAlloc(m_NumBlocks * 4, 0);
    if (!m_ppBlocks) return 0;
    for (u32 b = 0; b < m_NumBlocks; b++) {
        u32 count = b == m_NumBlocks - 1 ? m_LastBlockCount : 64;
        m_ppBlocks[b] = (EXTexture *)_EXAlloc(count * sizeof(EXTexture), 0);
        EXTexture *p = m_ppBlocks[b];
        if (!p) {
            m_NumBlocks = b - 1;
            m_LastBlockCount = count;
            if (b - 1) Release();
            return 0;
        }
        for (; count; count--, p++) new (p) EXTexture;
    }
    return 1;
}

void EXBaseTexture::UpdatePtrs(const EXGeoHeader *pHdr)
{
    EXGeoTextureHeader *pTexHdr = pHdr->GeoTextureHeader(m_HashCode);
    EXGeoTexture *pTex = pTexHdr->Object(0);
    signed char changed = m_pGeoTexture != pTex;
    m_pGeoTexture = pTex;
    m_pHeader = pTexHdr;
    Update(changed);
}
