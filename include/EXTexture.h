#ifndef EXTEXTURE_H
#define EXTEXTURE_H
#include "types.h"
#include "XBGeoDrawInfo.h"
#include "EXGeoHeader.h"
#include "EXGeoTexture.h"
#include "EXBit128.h"

// Runtime texture state over a file texture (sizeof 0x24). The platform texture derives from it.
namespace xb { struct D3DTexture; }

class EXBaseTexture {
public:
    virtual ~EXBaseTexture();
    virtual void Load() = 0;
    virtual void Unload() = 0;
    virtual void Select() = 0;
    virtual void Animate(float) = 0;
    virtual signed char InitRuntimeTexture(unsigned short, unsigned short, unsigned char, unsigned int) = 0;
    virtual void DestroyRuntimeTexture() = 0;
    virtual void Update(signed char) = 0;
    EXBaseTexture();
    void SetHashCode(unsigned int h) { m_HashCode = h; }
    unsigned int HashCode() const { return m_HashCode; }
    EXGeoTexture *GeoTexture() const { return m_pGeoTexture; }
    float UOffset() const { return m_UOffset; }
    float VOffset() const { return m_VOffset; }
    unsigned short Width() const { return m_pHeader ? m_pHeader->m_10 : 0; }
    unsigned short Height() const { return m_pHeader ? m_pHeader->m_12 : 0; }
    unsigned int Flags() const { return m_pHeader ? m_pHeader->Flags() : 0; }
    signed char IsPaused() const { return m_Paused; }
    signed char IsOnFirstFrame() const { return m_OnFirstFrame; }
    signed char IsOnLastFrame() const { return m_OnLastFrame; }
    void SetFrame(unsigned char f) { m_Frame = f; }
    float ScrollRateU() const;
    float ScrollRateV() const;
    void StopScroll();
    void ResetScrollRate();
    void ReverseScroll();
    unsigned char FrameCount() const;
    void PauseAnimation();
    void ResumeAnimation();
    void SetScrollRate(float u, float v);
    void SetScrollRateAngular(float angle, float speed);
    void RestartAnimation(unsigned char frame);
    void UpdatePtrs(EXGeoTextureHeader *pHdr, signed char force);
    void UpdatePtrs(const EXGeoHeader *pHdr);

    EXGeoTextureHeader *m_pHeader;  // 0x04
    EXGeoTexture *m_pGeoTexture;    // 0x08
    u32 m_HashCode;                 // 0x0c
    u8 m_Frame;                     // 0x10
    u8 m_11;
    u8 m_ScrollSet : 1;             // 0x12 bit 0: scroll rate overridden
    u8 m_Paused : 1;
    u8 m_Playing : 1;
    u8 m_OnFirstFrame : 1;
    u8 m_OnLastFrame : 1;
    u8 m_13;
    float m_ScrollRateU;            // 0x14
    float m_ScrollRateV;            // 0x18
    float m_UOffset;                // 0x1c
    float m_VOffset;                // 0x20
};

// Platform texture (XBTexture.obj, sizeof 0x50).
class EXTexture : public EXD3DResource, public EXBaseTexture {
public:
    virtual ~EXTexture();
    virtual void ReleaseD3DResource();
    virtual void Load() {}
    virtual void Unload() { ReleaseD3DResource(); }
    virtual void Select();
    virtual void Animate(float);
    virtual signed char InitRuntimeTexture(unsigned short, unsigned short, unsigned char, unsigned int);
    virtual void DestroyRuntimeTexture();
private:
    virtual void Update(signed char);
public:
    EXTexture();
    void SetTiling(signed char on);
    void SetFiltered(signed char on);
    // m_pD3DTexture is m_Texture for a still texture, or the current entry of m_pFrames when animated.
    u32 m_Texture[5];               // 0x30 xb::D3DTexture header over the file's pixel data
    xb::D3DTexture *m_pFrames;      // 0x44 one header per image of an animated texture
    u32 m_NumFrames;                // 0x48
    xb::D3DTexture *m_pD3DTexture;  // 0x4c
};

// Textures in blocks of 64, with a 128-bit in-use mask per block.
class EXTextureArray {
public:
    EXTextureArray() { m_NumBlocks = 0; m_NumUsed = 0; m_ppBlocks = 0; m_pBlockUse = 0; }
    ~EXTextureArray() { Release(); }
    EXTexture &operator[](unsigned int i) const { return m_ppBlocks[i >> 6][i & 0x3f]; }
    void Release();
    int NewTexture();
    signed char DeleteTexture(unsigned int i);
    int FindTextureIndex(const EXTexture *p) const;
    signed char Init(unsigned int n);
protected:
    signed char InitBlockUse();
public:
    u32 m_NumBlocks;                // 0x00
    u32 m_NumUsed;                  // 0x04 highest index + 1
    EXTexture **m_ppBlocks;         // 0x08
    ex::EXBit128 *m_pBlockUse;      // 0x0c
    u32 m_LastBlockCount;           // 0x10
};
#endif
