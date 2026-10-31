#ifndef XBBASEEFX_H
#define XBBASEEFX_H
#include "types.h"

#include "xbd3d8.h"


// Xbox full-screen post effects (bloom, focus blur, motion blur) (XBEfx.obj), sizeof 0xc8.
// TODO layout: only the constructor's stores are recovered.
class XBBaseEfx {
public:
    XBBaseEfx();
    xb::D3DTexture *GetBackBufferAsTexture();
    void XBBlitWindowToTexture(xb::D3DTexture *pTex, int x, int y, int w, int h);
    void XBCreateEFXTexture(int size, xb::D3DTexture **ppTex, int fmt);
    void XBCreateEFXTexture(int w, int h, xb::D3DTexture **ppTex, int fmt);
    void XBCreateBackBufferTexture(xb::D3DTexture **ppTex);
    void BiLinearSmoothTexture(xb::D3DTexture *pDst, xb::D3DTexture *pSrc, float size, int passes, int mode);
    void XBCreateFocusBlurTextures();
    void CalculateDepth(float *pOut, const xb::D3DXVECTOR3 &v);
    float FQuantizedDepth(float d, float *pHi, float *pLo);
    void CalculateDepthMapping(float d, float range, float *pHi, float *pHi2, float *pLo, float *pLo2);
    void XBCreateBackBufferZTexture();
    void XBBlitZBufferToTexture();
    void StartRenderToTexture(xb::D3DTexture *pColour, xb::D3DTexture *pDepth);
    void StopRenderToTexture();
    void XBCreateMotionBlurTexture();
    void DoBloom(xb::D3DTexture *pSrc, xb::D3DTexture *pDst, u32 srcShift, u32 dstShift);
    void XBBlitBackBufferToTexture(xb::D3DTexture *pTex, int x, int y, int w, int h, u32 flags, int mode);
    void XBCreateNoiseTexture();
    void XBBlurFocusBlurTextures();
    void CalculateFocusDepths(xb::D3DXVECTOR3 &a, xb::D3DXVECTOR3 &b);
    void Bloom(u32 pass);

    xb::D3DXVECTOR3 m_Eye;              // 0x00 camera position (focus effects)
    xb::D3DXVECTOR3 m_At;               // 0x0c camera look-at point
    xb::D3DSurface *m_pBackBuffer;      // 0x18 the back buffer when last asked for it
    xb::D3DTexture *m_pBloomHalf;       // 0x1c bloom: half-size target the passes add into
    xb::D3DTexture *m_pExpansion;       // 0x20 expansion blur: second screen-sized texture (ping-pong partner of 0x1c)
    xb::D3DTexture *m_pBlitSrc;         // 0x24 256x256 copy of the back buffer (glow, refraction)
    xb::D3DTexture *m_pRefractionBlur;  // 0x28
    xb::D3DTexture *m_pBlit128;         // 0x2c
    xb::D3DTexture *m_pNoise;           // 0x30 256x256 random bytes
    u32 m_34;
    u32 m_38;
    xb::D3DSurface *m_pSavedColour;     // 0x3c StartRenderToTexture saves the targets here
    xb::D3DSurface *m_pSavedDepth;      // 0x40
    xb::D3DSurface *m_pRTSurface;       // 0x44
    u32 m_48;
    xb::D3DTexture m_ZTexture;          // 0x4c header over the depth buffer's memory (linear A8B8G8R8)
    xb::D3DTexture *m_pZTexture;        // 0x60 &m_ZTexture once set up
    float m_64;                         // 0x64 focus depth (CalculateFocusDepths)
    float m_68;
    float m_6c;
    xb::D3DTexture *m_pFocusBlur256;    // 0x70 focus blur: the back buffer at 256x256, smoothed
    xb::D3DTexture *m_pFocusBlur128;    // 0x74 its half-size ping-pong partner
    xb::D3DTexture *m_pFocusBlur64;     // 0x78
    u8 m_7c[0x98 - 0x7c];
    xb::D3DTexture *m_pMotionBlur;      // 0x98 256x256, last frame for the motion blur
    xb::D3DTexture *m_pBloom[10];       // 0x9c bloom chain, quarter size and halving
    xb::D3DTexture *m_pBloomQuarter;    // 0xc4 the back buffer at quarter size
};
extern XBBaseEfx XBEfx;
#endif
