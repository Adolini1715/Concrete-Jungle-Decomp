#ifndef EXPARTICLESYS_H
#define EXPARTICLESYS_H
#include "EXBaseParticleSys.h"

class EXGeoHeader;
namespace xb { struct D3DDevice; }

// Xbox particle system (XBParticle.obj), sizeof 0x27c.
class EXParticleSys : public EXBaseParticleSys {
    EX_DECLARE_DYNAMIC(EXParticleSys)
public:
    virtual ~EXParticleSys();
    virtual void Draw(u32 pass);
    EXParticleSys();
    static EXParticleSys *CreateObject();
    int SortByTexture(const void *a, const void *b);
    void SetParticleRenderStates(xb::D3DDevice *pDev);
    void Debug_Draw(u32 pass);
    void DrawQuadParticles(xb::D3DDevice *pDev, EXGeoHeader *pHdr);
    void DrawEntityParticles(xb::D3DDevice *pDev, EXGeoHeader *pHdr);
};
#endif
