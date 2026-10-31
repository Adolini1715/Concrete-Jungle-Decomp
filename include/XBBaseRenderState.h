#ifndef XBBASERENDERSTATE_H
#define XBBASERENDERSTATE_H
#include "xbd3d8.h"

// The engine's shadow of the D3D state (XBRenderState.obj; the global
// XBRenderState at 0x4d0390, sizeof 0x4b0). Setters skip calls that would not
// change anything; SetDevice reloads the shadow from D3D's own copies.
class XBBaseRenderState {
public:
    XBBaseRenderState()
    {
        m_pDevice = 0;
        m_Textures[0] = 0;
        m_Textures[1] = 0;
        m_Textures[2] = 0;
        m_Textures[3] = 0;
        memset32(m_RenderState, 0, 0xa6);
        memset32(m_TextureState[0], 0, 0x80);
    }
    __forceinline void SetRenderState(xb::D3DRENDERSTATETYPE State, xb::DWORD Value)
    {
        if (m_RenderState[State] != Value) {
            m_RenderState[State] = Value;
            xb::D3DDevice::SetRenderState(State, Value);
        }
    }
    __forceinline void SetTextureStageState(xb::DWORD Stage, xb::D3DTEXTURESTAGESTATETYPE Type, xb::DWORD Value)
    {
        if (m_TextureState[Stage][Type] != Value) {
            m_TextureState[Stage][Type] = Value;
            xb::D3DDevice::SetTextureStageState(Stage, Type, Value);
        }
    }
    __forceinline void SetTexture(unsigned int Stage, xb::D3DTexture *pTexture)
    {
        if (m_Textures[Stage] != pTexture) {
            m_Textures[Stage] = pTexture;
            xb::D3DDevice::SetTexture(Stage, (xb::D3DBaseTexture *)pTexture);
        }
    }
    xb::DWORD GetRenderState(xb::D3DRENDERSTATETYPE State) { return m_RenderState[State]; }
    void SetDevice(xb::D3DDevice *pDevice);
    void EndShadowLightMapProjection();

    xb::D3DDevice *m_pDevice;               // 0x000
    xb::D3DTexture *m_Textures[4];          // 0x004
    xb::DWORD m_RenderState[0xa6];          // 0x014 indexed by D3DRENDERSTATETYPE
    xb::DWORD m_TextureState[4][32];        // 0x2ac [stage][D3DTEXTURESTAGESTATETYPE]
    u32 m_4ac;                              // 0x4ac TODO
private:
    static void memset32(xb::DWORD *p, xb::DWORD v, int n) { while (n--) *p++ = v; }
};
// Take the device and copy D3D's current render and texture stage states into the shadow
// (texture stage types 0..30 of each of the 4 stages).
inline void XBBaseRenderState::SetDevice(xb::D3DDevice *pDevice)
{
    m_pDevice = pDevice;
    for (xb::DWORD i = 0; i < 0xa6; i++) xb::D3DDevice::GetRenderState((xb::D3DRENDERSTATETYPE)i, &m_RenderState[i]);
    for (xb::DWORD s = 0; s < 4; s++)
        for (xb::DWORD t = 0; t < 31; t++)
            xb::D3DDevice::GetTextureStageState(s, (xb::D3DTEXTURESTAGESTATETYPE)t, &m_TextureState[s][t]);
}
extern XBBaseRenderState XBRenderState;
#endif
