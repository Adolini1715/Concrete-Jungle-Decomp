#ifndef XBRENDERTARGETMANAGER_H
#define XBRENDERTARGETMANAGER_H
#include "types.h"

namespace xb { struct D3DDevice; struct D3DSurface; struct D3DTexture; }

// The display's render target bookkeeping (EXDisplay+0x98c, sizeof 0x14): the back buffer and
// depth surfaces to return to, and the texture currently rendered into (Set / RestoreDefault).
class XBRenderTargetManager {
public:
    XBRenderTargetManager();
    void Initialise(xb::D3DDevice *pDevice);
    void SetDefault();
    void Set(xb::D3DTexture *pTex);
    void RestoreDefault();

    xb::D3DSurface *m_pDefaultDepth;    // 0x00
    xb::D3DSurface *m_pDefaultColour;   // 0x04 recorded by SetDefault (EXWnd::BeginDraw)
    xb::D3DSurface *m_pSurface;         // 0x08 level 0 of m_pTexture
    xb::D3DTexture *m_pTexture;         // 0x0c
    xb::D3DDevice *m_pDevice;           // 0x10
};
#endif
