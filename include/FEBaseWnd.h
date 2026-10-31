#ifndef FEBASEWND_H
#define FEBASEWND_H
#include "XWnd.h"
#include "EXStack.h"
#include "EXString.h"
#include "FEFlags.h"

class FEObject;
class FEObjectScript;

#include "EXCamera.h"

// One cached front-end sprite or text draw (FEWnd.obj); sizeof 0x5c.
struct FEBaseWndSprite {
    FEBaseWndSprite() {}
    FERenderPass::Flags m_Pass;     // 0x00
    unsigned int m_File;            // 0x04
    unsigned int m_Hash;            // 0x08
    ex::RGBA m_Colour;              // 0x0c
    EXRect m_Rect;                  // 0x10
    EXRect m_ClipRect;              // 0x20
    EXVector2 m_UV0;                // 0x30
    EXVector2 m_UV1;                // 0x38
    EXString m_Text;                // 0x40
    EXW::TextAlign m_Align;         // 0x44
    unsigned int m_Blend;           // 0x48
    FEObjectScript *m_pScript;      // 0x4c
    unsigned int m_Datum;           // 0x50
    unsigned int m_Unk54;           // 0x54
    FEObject *m_pObject;            // 0x58
};

// One cached front-end quad (FEWnd.obj); sizeof 0xac.
struct FEBaseWndPoly {
    struct Vert {
        EXMaths::vec_xyzw m_Pos;    // 0x00
        float m_U, m_V;             // 0x10
        u32 m_Unk18;                // 0x18
        ex::RGBA m_Colour;          // 0x1c
    };
    FEBaseWndPoly() {}
    unsigned int m_File;            // 0x00
    unsigned int m_Hash;            // 0x04
    Vert m_Verts[4];                // 0x08
    unsigned int m_A;               // 0x88
    unsigned int m_B;               // 0x8c
    unsigned int m_C;               // 0x90
    unsigned int m_D;               // 0x94
    EXVector2 m_UV;                 // 0x98
    FEObject *m_pObject;            // 0xa0
    unsigned int m_Blend;           // 0xa4
    signed char m_Flag;             // 0xa8
};

// Front-end window: sprite/poly/text caches and the clip-rect stack (FEWnd.obj).
class FEBaseWnd : public XWnd {
public:
    FEBaseWnd();

    void InvalidateExtents() { m_ExtentsValid = 0; }
    void DrawPolyDisable()
    {
        if (m_pPolys) EXFree(m_pPolys);
        m_pPolys = 0;
    }
    void DrawPolyEnable()
    {
        if (!m_pPolys) {
            m_pPolys = new FEBaseWndPoly[32];
            m_nPolys = 0;
        }
    }
    void DrawSpriteEnable()
    {
        if (!m_pSprites) {
            m_pSprites = new FEBaseWndSprite[256];
            m_nSprites = 0;
        }
    }
    void DrawSpriteDisable();
    void FlushClipRect();
    void SetClipRect(EXRect r)
    {
        m_ClipStack.Push(m_ClipRect);
        m_ClipRect = r;
    }
    void AddClipRect(EXRect r);
    void RestoreClipRect()
    {
        if (!m_ClipStack.Pop(m_ClipRect)) m_ClipRect.Set(0, 0, 0, 0);
    }
    void MovieOpen(char *pName);
    void Debug_Circle2D(EXPoint p, float r, unsigned int n, ex::RGBA c, signed char b);
    void RenderPolys();
    void SetupCamera(signed char b);
    EXPoint WorldToWnd(EXVector v);
    void CalculateExtents();
    static EXRect AlignRectToRect(EXRect a, EXRect b, EXW::TextAlign align);
    static EXRect AlignPointToPoint(EXPoint a, EXPoint b, EXW::TextAlign align);
    static EXPoint AlignPointToRect(EXRect r, EXW::TextAlign align);
    EXVector WndToWorld(EXPoint p);
    void DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXRect r, EXVector2 uv0, EXVector2 uv1);
    void DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXPoint p, EXPoint size, EXW::TextAlign align);
    void DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, FEObject *pObj, unsigned int datum, EXPoint size, EXW::TextAlign align);
    void DrawBorderSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXRect r, unsigned int border);
    void DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXRect r, EXString s, EXW::TextAlign align, unsigned int n);
    void DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXPoint p, EXString s, EXW::TextAlign align, unsigned int n);
    void DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, FEObject *pObj, unsigned int datum, EXString s, EXW::TextAlign align, unsigned int n);
    void DrawScript(FERenderPass::Flags pass, ex::RGBA c, FEObjectScript *pScript);
    void RenderSprites(FERenderPass::Flags pass);
protected:
    void CacheSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXRect r,
                     EXVector2 uv0, EXVector2 uv1, EXString text, EXW::TextAlign align, FEObject *pObj, FEObjectScript *pScript,
                     unsigned int a, unsigned int b);
    void CachePoly(unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, FEObject *pObj, unsigned int a, unsigned int b,
                   unsigned int cc, unsigned int d, EXVector v0, EXVector v1, EXVector v2, EXVector v3, EXVector2 uv, signed char flag);
public:
    void DrawPoly(unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, FEObject *pObj, unsigned int a, unsigned int b,
                  unsigned int cc, unsigned int d, EXVector2 uv, signed char flag);
    void DrawPoly(unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend, EXVector v0, EXVector v1, EXVector v2,
                  EXVector v3, EXVector2 uv, signed char flag);

    EXCamera m_Camera;               // 0x2b0
    FEBaseWndSprite *m_pSprites;     // 0x368 sprite cache (DrawSpriteEnable)
    FEBaseWndPoly *m_pPolys;         // 0x36c poly cache (DrawPolyEnable)
    unsigned int m_nSprites;         // 0x370
    unsigned int m_nPolys;           // 0x374
    EXRect m_ClipRect;               // 0x378
    EXStack<EXRect> m_ClipStack;     // 0x388
    float m_Extents[8];              // 0x3a4 CalculateExtents
    signed char m_ExtentsValid;      // 0x3c4
};
#endif
