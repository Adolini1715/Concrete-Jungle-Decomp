#include "XBBaseRenderState.h"

// The engine's render state shadow (XBRenderState.obj).

XBBaseRenderState XBRenderState;

// Back to plain alpha blending after the shadow / light map projection pass (alpha test on,
// stage 0 modulate, no specular).
void XBBaseRenderState::EndShadowLightMapProjection()
{
    SetRenderState(xb::D3DRS_ALPHABLENDENABLE, 1);
    SetRenderState(xb::D3DRS_ALPHATESTENABLE, 1);
    SetRenderState(xb::D3DRS_SRCBLEND, 0x302);
    SetRenderState(xb::D3DRS_DESTBLEND, 0x303);
    SetTextureStageState(0, xb::D3DTSS_ALPHAOP, 4);
    SetTextureStageState(0, xb::D3DTSS_COLOROP, 5);
    SetRenderState(xb::D3DRS_SPECULARENABLE, 0);
}
