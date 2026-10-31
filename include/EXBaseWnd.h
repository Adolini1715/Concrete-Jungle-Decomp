#ifndef EXBASEWND_H
#define EXBASEWND_H
#include "types.h"
#include "EXList.h"
#include "EXRect.h"
#include "EXVector.h"
#include "EXArray.h"
#include "EXFlag.h"
#include "RGBA.h"

class EXWnd;
class EXMenu;
class EXTexture;
class EXEffectsSystems;
class EXParticleSysMgr;
class EXSwooshSysMgr;
class EXInstanceManager;
class EXItemEnvView;
class EXCamera;
class EXMatrix;
class EXGeoFile;
struct EXPrimVert;
struct EXGeoFont;
struct EXFlexiSprite2D;
struct EXRotSprite2D;
struct EXSprite2D;
struct EXStringInfo;
struct EXLineInfo;
namespace EXW { enum TextAlign { TA_0 }; }

// Platform-independent window: a node in the display's window list (the EXDListItem base at +4 links
// it to its parent's children or the display's list). EXWnd (XBWnd.obj) is the Xbox implementation.
// sizeof at least 0x164. m_Flags: 0x10 debug text, 0x1000 / 0x80000000 stacking, 0x8000 hidden,
// 0x10000 delete pending, 0x40000 casts shadows, 0x200000 shows load activity.
class EXBaseWnd : public EXDListItem {
public:
    virtual ~EXBaseWnd();
    // Overloaded virtuals: VC places them in reverse declaration order.
    virtual EXVector WndToPrim(const EXVector &v)
    {
        return EXVector((float)m_PrimRect.x + v.x, (float)m_PrimRect.y + v.y, v.z, 0.0f);
    }
    virtual EXVector2 WndToPrim(const EXVector2 &v)
    {
        return EXVector2((float)m_PrimRect.x + v.x, (float)m_PrimRect.y + v.y);
    }
    virtual EXPoint WndToPrim(const EXPoint &p)
    {
        return EXPoint(m_PrimRect.x + p.x, m_PrimRect.y + p.y);
    }
    virtual EXVector PrimToWnd(const EXVector &v)
    {
        return EXVector(v.x - (float)m_PrimRect.x, v.y - (float)m_PrimRect.y, v.z, 0.0f);
    }
    virtual EXVector2 PrimToWnd(const EXVector2 &v)
    {
        return EXVector2(v.x - (float)m_PrimRect.x, v.y - (float)m_PrimRect.y);
    }
    virtual EXPoint PrimToWnd(const EXPoint &p)
    {
        return EXPoint(p.x - m_PrimRect.x, p.y - m_PrimRect.y);
    }
    virtual s8 UpdateWindow();
    virtual s8 InitialUpdate() { return 1; }
    virtual s8 Update() { return 1; }
    virtual void DrawWindowShadow();
    virtual void BeginDrawShadow() = 0;
    virtual void DrawShadow() {}
    virtual void EndDrawShadow() = 0;
    virtual s8 DrawWindow();
    virtual void BeginDraw() = 0;
    virtual s8 Draw() { return 1; }
    virtual void EndDraw() = 0;
    virtual void DeleteRenderMem() {}
    virtual void Close() {}
    virtual void ButtonDown(unsigned int) {}
    virtual void ButtonUp(unsigned int) {}
    virtual void MouseMove(int, int) {}
    virtual void KeyDown(unsigned char) {}
    virtual void KeyUp(unsigned char) {}
    virtual void Char(char) {}
    virtual s8 MenuSelect(unsigned int) { return 1; }
    virtual void DrawLine(const EXPrimVert &, const EXPrimVert &) {}
    virtual void DrawLines(unsigned int, EXPrimVert *) {}
    virtual void DrawLines(unsigned int, EXVector *, unsigned int, unsigned int, float) {}
    virtual void SetTriFanBackFaceCulling(s8) {}
    virtual void SetTriFanBlendMode(unsigned int) {}
    virtual void SetTriFanZMode(s8, s8) {}
    virtual void SelectTriFanTexture(EXTexture *) {}
    virtual void DrawTriFan(unsigned int, EXPrimVert *) {}
    virtual void SetTriStripBackFaceCulling(s8) {}
    virtual void SetTriStripBlendMode(unsigned int mode) { SetTriFanBlendMode(mode); }
    virtual void SetTriStripZMode(s8 a, s8 b) { SetTriFanZMode(a, b); }
    virtual void SelectTriStripTexture(EXTexture *pTex) { SelectTriFanTexture(pTex); }
    virtual void DrawTriStrip(unsigned int n, EXPrimVert *p) { DrawTriFan(n, p); }
    virtual void _SetSprite2DBlendMode(unsigned int) = 0;
    virtual void _SelectSprite2DTexture(EXTexture *, s8, s8) = 0;
    virtual void _SetSprite2DClipRect(short, short, unsigned short, unsigned short) = 0;
    // Overloaded virtuals: reverse declaration order.
    virtual void _DrawSprite2D(EXSprite2D &) = 0;
    virtual void _DrawSprite2D(EXRotSprite2D &) = 0;
    virtual void _DrawSprite2D(EXFlexiSprite2D &) = 0;
    virtual void MovieOpen(char *, int, int, int, int, s8);
    virtual void MovieUpdate();
    virtual void MovieRender();
    virtual void MovieClose();
    virtual s8 MovieCheckFreeMem();
protected:
    virtual void RebuildBloom() {}
public:
    EXBaseWnd();

    EXMenu *Menu() { return m_pMenu; }
    EXWnd *Parent() { return m_pParent; }
    void SetParent(EXWnd *p) { m_pParent = p; }
    EXWnd *FirstChild() const { return (EXWnd *)m_Children.Head(); }
    const char *Debug_Name() const { return ""; }
    EXRect Sprite2DClipRect() const { return m_Sprite2DClipRect; }
    const EXRect &Rect() const { return m_Rect; }
    const EXRect &PrimRect() const { return m_PrimRect; }
    const EXRect &PrimClipRect() const { return m_PrimClipRect; }
    ex::EXFlag<unsigned int> &Flags() { return m_Flags; }
    ex::EXFlag<unsigned int> &Effects() { return m_Effects; }
    void ResetSprite2DClipRect()
    {
        m_Sprite2DClipRect = m_Rect;
        _SetSprite2DClipRect(0, 0, m_Rect.w, m_Rect.h);
    }
    void SetSprite2DClipRect(EXRect &r)  // out-of-line copy at FEWnd.obj 0x10ac50
    {
        m_Sprite2DClipRect = r;
        _SetSprite2DClipRect(r.x, r.y, r.w, r.h);
    }
    void SetBkColor(ex::RGBA c) { m_BkColor = c; }
    ex::RGBA BkColor() const { return m_BkColor; }
    void SetFontColor(ex::RGBA c) { m_FontColor = c; }
    s8 HasMovieFinished() const { return m_MovieFinished; }
    EXEffectsSystems *ActiveEffectsSystem() { return m_pActiveEffects; }
    EXItemEnvView *ItemEnvView() const { return m_pItemEnvView; }
    void Debug_DeleteDebugItemData() {}
    EXVector2 &Origin() { return m_Origin; }
    void SetOriginToCentre()
    {
        m_Origin = EXVector2((float)m_Rect.w * 0.5f, (float)m_Rect.h * 0.5f);
    }
    void SetFocusBlurFactor(unsigned int f) { m_FocusBlurFactor = f; }
    unsigned int GetFocusBlurFactor() const { return m_FocusBlurFactor; }
    float GetFocusBlurDistance() const { return m_FocusBlurDistance; }
    const EXRectf &GetMotionBlurSampleRect() const { return m_Unk104; }
    const EXRectf &GetMotionBlurDrawRect() const { return m_MotionBlurDrawRect; }
    void SetFocusBlurDistance(float d) { m_FocusBlurDistance = d; }
    void SetGlowIntensity(float f) { m_GlowIntensity = f; }
    float GetGlowIntensity() const { return m_GlowIntensity; }
    void SetMotionBlurFactor(float f) { m_MotionBlurFactor = f; }
    float GetMotionBlurFactor() const { return m_MotionBlurFactor; }
    void SetNoiseIntensity(float f) { m_NoiseIntensity = f; }
    float GetNoiseIntensity() const { return m_NoiseIntensity; }
    void SetMotionBlurDrawRect(const EXRectf &r)
    {
        m_MotionBlurDrawRect.x = r.x;
        m_MotionBlurDrawRect.y = r.y;
        m_MotionBlurDrawRect.w = r.w;
        m_MotionBlurDrawRect.h = r.h;
    }
    void SetInverseColorFactor(float f) { m_InverseColorFactor = f; }
    void SetInverseColorMask(unsigned int m) { m_InverseColorMask = m; }
    float InverseColorFactor() const { return m_InverseColorFactor; }
    unsigned int InverseColorMask() const { return m_InverseColorMask; }
    float GetNoiseTextureSampleWidth() const { return m_NoiseSampleW; }
    float GetNoiseTextureSampleHeight() const { return m_NoiseSampleH; }
    void SetBloomColor(ex::RGBA c)
    {
        if (m_BloomColor != c) {
            m_BloomColor = c;
            RebuildBloom();
        }
    }
    ex::RGBA BloomColor() const { return m_BloomColor; }
    unsigned short DbgRow() const { return (unsigned short)m_DbgY; }
    unsigned short DbgCol() const { return (unsigned short)m_DbgX; }
    void Debug_Line2D(ex::RGBA, int, int, int, int) {}
    void _Debug_Rect2D(ex::RGBA, int, int, int, int) {}
    void _Debug_SolidRect2D(ex::RGBA, int, int, int, int) {}
    unsigned short DbgWidth() const { return m_DbgWidth; }
    unsigned short DbgHeight() const { return m_DbgHeight; }
    void Debug_SetName(const char *) {}
    void Debug_SolidRect2D(ex::RGBA, const EXRect &) {}
    void Debug_SolidRect2D(ex::RGBA, int, int, int, int) {}
    void SetSprite2DBlendMode(unsigned int mode) { _SetSprite2DBlendMode(mode); }
    void SelectSprite2DTexture(EXTexture *pTex, s8 a, s8 b) { _SelectSprite2DTexture(pTex, a, b); }

    void FlushMem();
    void SetRect(const EXRect &r);
    void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }  // App.obj copy
    void DelChild(EXWnd *pWnd);
    void HideChild(EXWnd *pWnd);
    s8 IsChild(EXWnd *pWnd) const;
    void CalcPrimRects();
    s8 IsAtFront() const;
    s8 IsAtBack() const;
    s8 HasFocus() const;
    s8 IsVisible();
    EXParticleSysMgr *ParticleSysMgr();
    void DeleteParticleSysMem();
    void DeleteAllParticleSystems();
    void StopParticleUpdateAndRender(s8 stop);
    void PauseParticleSystems(s8 pause);
    EXSwooshSysMgr *SwooshSysMgr();
    void DeleteSwooshSysMem();
    void DeleteAllSwooshSystems();
    void StopSwooshUpdateAndRender(s8 stop);
    void PauseSwooshSystems(s8 pause);
    EXInstanceManager *GetInstanceManager();
    s8 RequestTextureCapture(EXTexture *pTex);
    void RegisterItemEnvView(s8 a);
    void ReleaseItemEnvView();
    s8 UpdateItemEnvView(EXCamera &cam, const EXMatrix &m);
    s8 RenderItemEnvView(EXCamera &cam, const EXMatrix &m);
    void DeleteMem();
    EXWnd *AddChild(EXWnd *pWnd);
    EXWnd *ContainsPrimPoint(const EXPoint &p);
    EXWnd *ContainsDispPoint(const EXPoint &p);
    void DeleteInstanceManager();

    // EXWndText.obj
    EXPoint FontCharSize(char c, float scale) const;
    EXPoint FontCharSize(unsigned short c, float scale) const;
    unsigned short FontCharWidth(char c) const;
    unsigned short FontCharHeight(char c) const;
    void SelectFont(const EXGeoFile *pFile, unsigned int hash);
    void FontPrint(unsigned short x, unsigned short y, unsigned short c, float scale, EXW::TextAlign align, s8 filter);
    void GetStringProperties(const char *pText, const EXRect &r, EXStringInfo *pInfo, EXW::TextAlign align,
                             float scale) const;
    void GetStringProperties(const unsigned short *pText, const EXRect &r, EXStringInfo *pInfo, EXW::TextAlign align,
                             float scale) const;
    EXPoint FontTextSize(const unsigned short *pText, float scale) const;
    EXPoint FontTextSize(const char *pText, const EXRect &r, float scale) const;
    EXPoint FontTextSize(const unsigned short *pText, const EXRect &r, float scale) const;
    EXPoint FontTextSize(const char *pText, float scale) const;
    void FontPrint(const unsigned short *pText, const EXRect &r, float scale, EXW::TextAlign align, s8 filter,
                   const EXStringInfo *pInfo = 0);
    void FontPrint(unsigned short x, unsigned short y, const unsigned short *pText, float scale, EXW::TextAlign align,
                   s8 filter, const EXStringInfo *pInfo = 0);
    void FontPrint(const char *pText, const EXRect &r, float scale, EXW::TextAlign align, s8 filter,
                   const EXStringInfo *pInfo = 0);
    void FontPrint(unsigned short x, unsigned short y, const char *pText, float scale, EXW::TextAlign align,
                   s8 filter, const EXStringInfo *pInfo = 0);
    // EXWndSprite.obj
    void SelectSprite2DTexture(const EXGeoFile *pFile, unsigned int hash, s8 a, s8 b);
    void DrawSprite2D(ex::RGBA c, const EXRect &r, const EXVector2 &uv0, const EXVector2 &uv1);  // inline, FEWnd.obj copy
    void DeleteSprite2DData();
    // EXWndDebug.obj
    void SetDbgPosPixels(unsigned short x, unsigned short y);
    void DeleteDbgMem();
    void DbgClear();
    void SetDbgPos(unsigned short x, unsigned short y);
    void CreateDbgMem();
    void DbgScroll();
    void DbgPrint(const char *pFmt, ...);
protected:
    // EXWndText.obj
    void CalculateAlignment(unsigned int n, const EXRect &r, EXStringInfo *pInfo, EXW::TextAlign align,
                            float scale) const;
    void RenderFont(const char *pText, const EXLineInfo &line, unsigned short height, float scale);
    void RenderFont(const unsigned short *pText, const EXLineInfo &line, unsigned short height, float scale);
    void Debug_ClearDebugItemData() {}
    void DrawDebugItems() {}            // empty in retail (XBWnd.obj copy)
    static s8 CalcChildPrimRects(EXDListItem *pItem);
    static s8 UpdateChild(EXDListItem *pItem);
    static s8 DrawChild(EXDListItem *pItem);
    void ClearSprite2DData();
    void AddSprite2DData(const void *pData, unsigned int size);
    void *AllocSprite2DData(unsigned int size);

public:
    EXWnd *m_pParent;                      // 0x0c
    EXDList m_Children;                    // 0x10
    EXRect m_Rect;                         // 0x18
    int m_Unk28[2];                        // 0x28 TODO (0, 0)
    int m_Unk30[2];                        // 0x30 TODO (12, 12) debug text cell size?
    EXRect m_PrimRect;                     // 0x38
    EXRect m_PrimClipRect;                 // 0x48
    EXRect m_Sprite2DClipRect;             // 0x58 window rect on the first SetRect
    EXVector2 m_Origin;                    // 0x68
    ex::EXFlag<unsigned int> m_Flags;      // 0x70
    ex::EXFlag<unsigned int> m_Effects;    // 0x74
    ex::RGBA m_BkColor;                    // 0x78
    ex::RGBA m_Unk7c;                      // 0x7c TODO (0x50, 0x50, 0x50, 0xff)
    ex::RGBA m_Unk80;                      // 0x80 TODO (0x80, 0x80, 0x80, 0xff)
    ex::RGBA m_FontColor;                  // 0x84 reset to 0x80808080 by DrawWindow
    u32 m_Unk88;                           // 0x88
    s8 m_Unk8c;                            // 0x8c
    s8 m_Initialised;                      // 0x8d InitialUpdate done
    s8 m_MovieFinished;                    // 0x8e
    s8 m_CaptureRequested;                 // 0x8f
    EXTexture *m_pCaptureTexture;          // 0x90
    u16 *m_pDbgText;                       // 0x94 m_DbgWidth x m_DbgHeight cells (attr | char)
    u16 m_DbgAttr;                         // 0x98 colour bits ORed into each cell
    u16 m_DbgWidth;                        // 0x9a
    u16 m_DbgHeight;                       // 0x9c
    u16 m_Unk9e;                           // 0x9e
    int m_DbgX;                            // 0xa0 cursor column
    int m_DbgY;                            // 0xa4 cursor row
    EXGeoFont *m_pFont;                    // 0xa8 SelectFont
    EXTexture *m_pFontTexture;             // 0xac
    EXTexture *m_pSprite2DTexture;         // 0xb0 _SelectSprite2DTexture
    EXPtrArray<unsigned char> m_Sprite2DData;  // 0xb4
    EXPtrArray<unsigned char> m_Sprite2DBlocks;  // 0xc4 EXSprite2DBlock pointers (0x2000 bytes each)
    EXEffectsSystems *m_pEffectsSystems;   // 0xd4 owned
    EXEffectsSystems *m_pActiveEffects;    // 0xd8
    EXMenu *m_pMenu;                       // 0xdc
    EXInstanceManager *m_pInstanceManager; // 0xe0
    // Post effects (XBWnd).
    u32 m_FocusBlurFactor;                 // 0xe4
    u32 m_Unke8;                           // 0xe8
    u32 m_InverseColorMask;                // 0xec
    float m_GlowIntensity;                 // 0xf0
    float m_FocusBlurDistance;             // 0xf4
    float m_InverseColorFactor;            // 0xf8
    float m_MotionBlurFactor;              // 0xfc
    u32 m_Unk100;                          // 0x100
    EXRectf m_Unk104;                      // 0x104 (0, 0, 1, 1)
    EXRectf m_MotionBlurDrawRect;          // 0x114
    // Expansion blur (EXWnd::DoExpansionBlur, effect 0x100): the back buffer zoomed in m_ExpansionPasses
    // steps (inset m_ExpansionStart, growing by m_ExpansionStep, in 256x224 units), blended in with
    // m_ExpansionAlpha where m_pExpansionMask's sprites set the alpha (everywhere without a mask).
    u32 m_ExpansionPasses;                 // 0x124 3
    float m_ExpansionStart;                // 0x128 2.75
    float m_ExpansionStep;                 // 0x12c 1.8
    float m_ExpansionAlpha;                // 0x130 1.0
    struct EXExpansionMask *m_pExpansionMask;  // 0x134
    // Refraction blur (EXWnd::XBRenderRefractionEntities, m_EfxFlags bit 1): offsets per pass.
    float m_RefractionScale[4];            // 0x138 0.333
    u32 m_RefractionPasses;                // 0x148 14
    u32 m_Unk14c;                          // 0x14c
    float m_NoiseIntensity;                // 0x150
    float m_NoiseSampleW;                  // 0x154 1.0 noise texture repeats across the window
    float m_NoiseSampleH;                  // 0x158 1.0
    ex::RGBA m_BloomColor;                 // 0x15c
    EXItemEnvView *m_pItemEnvView;         // 0x160
};
#endif
