#ifndef XBBASEPIXELSHADER_H
#define XBBASEPIXELSHADER_H
#include "types.h"
#include "xbd3d8.h"

// The engine's pixel shader table (XBPixelShader.obj; the global XBPixelShader at 0x472290).
// sizeof 0x40: the shader definitions and the handles CreatePixelShader returned for them.
class XBBasePixelShader {
public:
    XBBasePixelShader();
    void DeclareAllPixelShaders();
    // Emitted in XBTexture.obj.
    void SetPixelShader(int n) { xb::D3DDevice::SetPixelShader(m_Handles[n]); }
    unsigned long *m_pDefs[8];          // 0x00 index 0 unused
    unsigned long m_Handles[8];         // 0x20
};
extern XBBasePixelShader XBPixelShader;
#endif
