#ifndef EXWND_H
#define EXWND_H
#include "EXBaseWnd.h"

#include "EXArray.h"
#include "EXCachedGeoEntity.h"

class EXMatrix;
namespace xb { struct D3DDevice; }

typedef EXDataArray<EXCachedGeoEntity, EXCachedGeoEntity &> EXCachedGeoEntityArray;

// Xbox window (XBWnd.obj / XBWndPrim.obj / XBWndSprite.obj / XBWndDebug.obj). TODO layout after EXBaseWnd.
class EXWnd : public EXBaseWnd {
public:
    virtual ~EXWnd();
    virtual void BeginDrawShadow();
    virtual void EndDrawShadow();
    virtual void BeginDraw();
    virtual void EndDraw();
    virtual void DrawLine(const EXPrimVert &a, const EXPrimVert &b);
    virtual void DrawLines(unsigned int n, EXPrimVert *p);
    virtual void SetTriFanBackFaceCulling(s8 cull);
    virtual void SetTriFanBlendMode(unsigned int mode);
    virtual void SetTriFanZMode(s8 a, s8 b);
    virtual void SelectTriFanTexture(EXTexture *pTex);
    virtual void DrawTriFan(unsigned int n, EXPrimVert *p);
    virtual void SetTriStripBackFaceCulling(s8 cull);
    virtual void SetTriStripBlendMode(unsigned int mode);
    virtual void SetTriStripZMode(s8 a, s8 b);
    virtual void SelectTriStripTexture(EXTexture *pTex);
    virtual void DrawTriStrip(unsigned int n, EXPrimVert *p);
    virtual void _SetSprite2DBlendMode(unsigned int mode);
    virtual void _SelectSprite2DTexture(EXTexture *pTex, s8 a, s8 b);
    virtual void _SetSprite2DClipRect(short x, short y, unsigned short w, unsigned short h);
    // Overloaded virtuals: reverse declaration order.
    virtual void _DrawSprite2D(EXSprite2D &s);
    virtual void _DrawSprite2D(EXRotSprite2D &s);
    virtual void _DrawSprite2D(EXFlexiSprite2D &s);
    virtual void MovieUpdate();
    virtual void MovieClose();
    virtual s8 MovieCheckFreeMem();
    virtual void MovieOpen(char *pName, int a, int b, int c, int d);
    virtual void Calculate2DScreenRect(EXMatrix *pMat, EXVector &a, EXVector &b, EXVector &c, EXVector &d);
protected:
    virtual void _Line2D(ex::RGBA col, int x0, int y0, int x1, int y1);
    virtual void _Rect2D(ex::RGBA col, int x, int y, int w, int h);
    virtual void _SolidRect2D(ex::RGBA col, int x, int y, int w, int h);
    virtual void _Line3D(ex::RGBA col, const EXVector *p, int n);
    virtual void _Line3D(ex::RGBA col, const EXVector &a, const EXVector &b);
    virtual void _TriLine3D(ex::RGBA col, const EXVector &a, const EXVector &b, const EXVector &c);
    virtual void _QuadLine3D(ex::RGBA col, const EXVector &a, const EXVector &b, const EXVector &c, const EXVector &d);
    virtual void _Text3D(ex::RGBA col, const EXVector &pos, EXW::TextAlign align, const char *pText);
public:
    EXWnd();
    void XBRenderSimpleShadowEntities();
    s8 XBRenderShadowEntities();
    s8 XBRenderLightEntities();
    void XBRenderHeatEntities();
    void XBRenderRefractionEntities();
    void RenderEnitityArray(EXCachedGeoEntityArray &arr);
    void SortAndRenderEnitityArray(EXCachedGeoEntityArray &arr, unsigned char layer);
    s8 RenderSortedShadowArray(EXCachedGeoEntityArray &arr);
    // Queue an entity for EndDraw (copies emitted in XBGeoEntity.obj; EXGeoEntity::Draw picks the list).
    void AddAlphaGeoEntity(EXCachedGeoEntity &e) { m_Alpha.Add(e, 1); }
    void AddNormalGeoEntity(EXCachedGeoEntity &e) { m_178.Add(e, 1); }
    void AddShadowGeoEntity(EXCachedGeoEntity &e) { m_ShadowCasters.Add(e, 1); }
    void AddIdShadowGeoEntity(EXCachedGeoEntity &e) { m_ShadowReceivers.Add(e, 1); }
    void AddLightGeoEntity(EXCachedGeoEntity &e) { m_Lights.Add(e, 1); }
    void AddRefractionGeoEntity(EXCachedGeoEntity &e) { m_Refraction.Add(e, 1); }
    void AddHeatGeoEntity(EXCachedGeoEntity &e) { m_Heat.Add(e, 1); }
    void AddAlphaFogGeoEntity(EXCachedGeoEntity &e) { m_RefractionPre.Add(e, 1); }
    void AddPreRefractionGeoEntity(EXCachedGeoEntity &e) { m_1e8.Add(e, 1); }
    void AddPostRefractionGeoEntity(EXCachedGeoEntity &e) { m_1f8.Add(e, 1); }
    void AddPreRefractionAlphaGeoEntity(EXCachedGeoEntity &e) { m_208.Add(e, 1); }
    void AddPostRefractionAlphaGeoEntity(EXCachedGeoEntity &e) { m_218.Add(e, 1); }
protected:
    void DrawDebugText();
    void DrawSprite2DData();
private:
    void DoExpansionBlur();
    void RenderEffects();
public:
    // Entities queued by the scene draw (EXGeoEntity::AddCachedDraw) and drawn in EndDraw.
    xb::D3DDevice *m_pDevice;                   // 0x164
    EXCachedGeoEntityArray m_Alpha;             // 0x168 sorted back to front with the particles (layer 0)
    EXCachedGeoEntityArray m_178;               // 0x178 opaque entities (AddNormalGeoEntity), drawn after the shadow and light passes
    EXCachedGeoEntityArray m_ShadowCasters;     // 0x188 drawn into the shadow map
    EXCachedGeoEntityArray m_ShadowReceivers;   // 0x198 RenderSortedShadowArray (AddIdShadowGeoEntity: each gets a shadow object id)
    EXCachedGeoEntityArray m_Lights;            // 0x1a8 drawn into the light map
    EXCachedGeoEntityArray m_Refraction;        // 0x1b8 drawn over a copy of the back buffer
    EXCachedGeoEntityArray m_1c8;               // 0x1c8 TODO not used in XBWnd.obj
    EXCachedGeoEntityArray m_Heat;              // 0x1d8 heat vision
    EXCachedGeoEntityArray m_1e8;               // 0x1e8 render order 3 (AddPreRefractionGeoEntity): drawn first, and again after the movie update
    EXCachedGeoEntityArray m_1f8;               // 0x1f8 render order 4 (AddPostRefractionGeoEntity): drawn after the refraction pass
    EXCachedGeoEntityArray m_208;               // 0x208 render order 3 with alpha (AddPreRefractionAlphaGeoEntity), sorted, layer 3
    EXCachedGeoEntityArray m_218;               // 0x218 render order 4 with alpha (AddPostRefractionAlphaGeoEntity), sorted, layer 4
    EXCachedGeoEntityArray m_RefractionPre;     // 0x228 render order 5 (AddAlphaFogGeoEntity, no Z): drawn before the back buffer is copied
    EXCachedGeoEntityArray m_238;               // 0x238
    EXCachedGeoEntityArray m_Layers[5];         // 0x248 TODO
    u32 m_EfxFlags;                             // 0x298 bit 0: bloom this frame, bit 1: TODO
};
#endif
