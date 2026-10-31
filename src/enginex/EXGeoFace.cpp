#include "EXGeoFace.h"
#include "EXGeoHeader.h"
#include "EXBaseDisplay.h"
#include "EXWnd.h"

void EXGeoFace::Draw(EXGDiData *, unsigned int)
{
    EXWnd *pWnd = EXBaseDisplay::ActiveWnd();
    if (pWnd) {
        EXTexture *pTex = EXBaseDisplay::GeoHeader()->Texture(m_TextureRef);
        pWnd->SetTriFanBlendMode(0);
        pWnd->SelectTriFanTexture(pTex);
        pWnd->DrawTriFan(m_VertexCount, (EXPrimVert *)m_Vtx);
    }
}
