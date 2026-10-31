#ifndef FEOBJECT_H
#define FEOBJECT_H
#include "types.h"
#include "EXList.h"
#include "EXRect.h"
#include "EXVector.h"
#include "EXString.h"
#include "EXStack.h"
#include "RGBA.h"
#include "FEFlags.h"
#include "FEMathmos.h"
#include "FEListener.h"
#include "FEWnd.h"

class FEWnd;
class FEObjectScript;

extern const EXString FE_Blank;

// Base front-end element (FEObject.obj): a rectangle with optional text, sprite, 3D scripts,
// keyframed moves, listeners and four colour animators. Root of the FELayout* tree. sizeof 0x1b4.
// m_Flags: 0x2 rect locked, 0x4 focused, 0x8 focusable, 0x10 origin unset, 0x20 aligned,
// 0x40 visible, 0x80 hidden, 0x200 base rect locked, 0x800 bound, 0x1000/0x2000 focus mathmos,
// 0x10000 accepts input, 0x20000 text clipped, 0x40000 no clip, 0x800000 paused.
class FEObject {
public:
    virtual ~FEObject();
    virtual void Pause(signed char pause);
    virtual FEObject *ListenerAdd(FEListener *pListener);
    virtual FEObject *ListenerChildAdd(FEListener listener);
    virtual void NotifyDisplay(signed char show);
    virtual FEObject *LockTextRect(int x, int y, int w, int h);
    virtual FEObject *LockTextRectToParent();
    virtual FEObject *SetMathmos(FEObjColour::Flags which, FEMathmos m, signed char b);
    virtual FEObject *SetRemoteMathmos(FEObjColour::Flags which, FEMathmos *p, signed char b);
    virtual FEObject *MathmosLink(FEObject *p)
    {
        m_pMathmosLink = p;
        return this;
    }
    virtual ex::RGBA MathmosLinkColour() { return m_Colour; }
    virtual FEObject *MathmosGoto(FEMathmosPos::Flags pos);
    virtual void MathmosUpdate();
    virtual signed char HandleInput(signed char b);
    virtual void EnsureVisible(FEObject *p, signed char b);
    virtual void BindUpdate(unsigned __int64 data);
    virtual void BindCommit();
    virtual signed char BindRecall();
    virtual void Bind(FEDataType::Flags type, void *p);
    virtual void Update();
    virtual void Draw();
    virtual void UpdateKeyframes();
    virtual FEObject *SetFocus(signed char focus, signed char b, signed char c);
    virtual void DebugProperties(char *pBuf, unsigned int n);
    virtual void DebugDump(unsigned int n) { DebugProperties(DebugName(), n); }
    virtual char *DebugName() { return "Object"; }
    virtual FEObject *Resize() { return this; }
    virtual FEObject *SetFont(unsigned int file, unsigned int hash, signed char b)
    {
        m_Font[0] = file;
        m_Font[1] = hash;
        return this;
    }
    virtual FEObject *SetTexture(unsigned int file, unsigned int hash)
    {
        m_Texture[0] = file;
        m_Texture[1] = hash;
        return this;
    }
    virtual FEObject *SetTexCoords(EXVector2 uv0, EXVector2 uv1)
    {
        m_TexCoord0.x = uv0.x;
        m_TexCoord0.y = uv0.y;
        m_TexCoord1.x = uv1.x;
        m_TexCoord1.y = uv1.y;
        return this;
    }
    virtual void SetParent(FEObject *p) { m_pParent = p; }
    virtual FEObject *Parent() { return m_pParent; }
    virtual FEObject *SetEventCallback(FEEventCallback p, signed char b)
    {
        m_pEventCallback = p;
        return this;
    }
    virtual signed char DoEventCallback(FEObjEvent::Flags ev)
    {
        signed char r = 0;
        if (m_pEventCallback) r = m_pEventCallback(this, ev);
        return r;
    }
    virtual unsigned int ChildCount() { return 0; }
    // Overloaded virtuals: reverse declaration order.
    virtual FEObject *ObjectAdd(unsigned int i, FEObject *p) { return this; }
    virtual FEObject *ObjectAdd(unsigned int i, unsigned int j, FEObject *p) { return this; }
    virtual void ObjectDel(unsigned int i) {}
    virtual void ObjectDel(unsigned int i, unsigned int j) {}
    virtual FEObject *Object(unsigned int i) { return this; }
    virtual FEObject *Object(unsigned int i, unsigned int j) { return this; }
    virtual FEObject *SetVisibleCount(unsigned int x, unsigned int y) { return this; }
    virtual EXPoint VisibleCount() { return EXPoint(0, 0); }
    virtual FEObject *SetSmoothScroll(signed char b) { return this; }
    virtual signed char SmoothScroll() { return 0; }
    virtual FEObject *SetScrollAnchor(signed char b) { return this; }
    virtual signed char ScrollAnchor() { return 0; }
    virtual FEObject *SetRange(int lo, int hi) { return this; }
    virtual int Range(int &lo, int &hi) { return 0; }
    virtual FEObject *SetDelta(int d) { return this; }
    virtual int Delta() { return 0; }
    virtual FEObject *SetValue(int v) { return this; }
    virtual int Value() { return 0; }
    virtual FEObject *ChildSel() { return this; }
    virtual void PageSelect(unsigned __int64 id, signed char a, signed char b) {}
    virtual FEObject *Page(unsigned __int64 id, int *pIndex) { return 0; }
    virtual signed char PageBack() { return 0; }
    virtual void PageFlush() {}
    virtual FEObject *FindChildObject(unsigned __int64 data) { return 0; }
    virtual FEListener *FindChildListener(unsigned __int64 id) { return 0; }

    FEObject();
    FEObject(FEObjType::Flags type, unsigned int file, unsigned int hash, unsigned __int64 data, EXString text,
             EXString help, EXW::TextAlign textAlign, unsigned int x, unsigned int y, unsigned int w, unsigned int h,
             EXW::TextAlign objAlign);

    // State flags.
    unsigned int State() { return m_Flags; }
    FEObject *StateSet(unsigned int s)
    {
        m_Flags = s;
        return this;
    }
    FEObject *StateAdd(unsigned int s)
    {
        m_Flags |= s;
        return this;
    }
    FEObject *StateSub(unsigned int s)
    {
        m_Flags &= ~s;
        return this;
    }
    FEObject *StateModify(unsigned int add, unsigned int sub)
    {
        m_Flags = (m_Flags & ~sub) | add;
        return this;
    }
    FEObject *StatePush()
    {
        m_StateStack.Push(m_Flags);
        return this;
    }
    FEObject *StatePop()
    {
        m_StateStack.Pop(m_Flags);
        return this;
    }

    // Geometry.
    EXRect Rect() { return EXRect(m_BaseRect.x + m_Rect.x, m_BaseRect.y + m_Rect.y, m_Rect.w, m_Rect.h); }
    EXRect LocalRect() { return m_Rect; }
    EXRect TextRect() { return m_TextRect; }
    EXPoint Origin()
    {
        EXPoint p;
        p.x = m_BaseRect.x + m_Rect.x;
        p.y = m_BaseRect.y + m_Rect.y;
        return p;
    }
    EXPoint OutPoint(EXPoint p) { EXPoint o; o.y = m_BaseRect.y + m_Rect.y; o.x = m_BaseRect.x + m_Rect.x; return EXPoint(p.x + o.x, p.y + o.y); }
    // NEAR: 15 bytes differ, retail computes the y sum before x
    EXRect OutRect(EXRect r)
    {
        EXPoint o(m_BaseRect.x + m_Rect.x, m_BaseRect.y + m_Rect.y);
        return EXRect(r.x + o.x, r.y + o.y, r.w, r.h);
    }
    EXPoint Centre() { return EXPoint(m_Rect.w / 2 + m_BaseRect.x + m_Rect.x, m_Rect.h / 2 + m_BaseRect.y + m_Rect.y); }
    void Inflate(int dx, int dy)
    {
        m_Rect.Inflate(dx, dy);
        m_Flags |= 2;
        Resize();
    }
    void Deflate(int dx, int dy)
    {
        m_Rect.Deflate(dx, dy);
        m_Flags |= 2;
        Resize();
    }
    void SetSize(EXPoint size)
    {
        m_Rect.w = size.x;
        m_Rect.h = size.y;
        DoObjectAlign();
        Resize();
    }
    EXW::TextAlign ObjectAlign() { return m_ObjectAlign; }
    EXVector2 OriginFloat() { return m_OriginFloat; }
    // NEAR: retail converts o.x before building the rect argument (x stored first)
    void SetOriginFloat(EXVector2 o)
    {
        StateSub(0x10);
        SetRect(EXRect((int)o.x, (int)o.y, m_Rect.w, m_Rect.h));
        m_OriginFloat.x = o.x;
        m_OriginFloat.y = o.y;
    }
    void SetOrigin(EXPoint p, EXW::TextAlign align);

    // Misc accessors.
    FEWnd *OwnerWnd() { return m_pOwnerWnd; }
    FEObject *SetBorderWidth(unsigned int w)
    {
        m_BorderWidth = w;
        return this;
    }
    FEObject *SetBlendMode(unsigned int m)
    {
        m_BlendMode = m;
        return this;
    }
    void Delete() { m_DeleteNeeded = 1; }
    signed char DeleteNeeded() { return m_DeleteNeeded; }
    EXString &Text() { return m_Text; }
    EXString &HelpText() { return m_HelpText; }
    FEObject *SetHelpText(EXString s)
    {
        m_HelpText.Empty();
        m_HelpText = s;
        return this;
    }
    int SetText(EXString s, int speed, int delay)
    {
        m_Text.Empty();
        m_Text = s;
        m_TypeSpeed = speed;
        m_ScrollPos = 0.0f;
        m_TypeDelay = delay;
        m_TypeChars = 0;
        m_TypeCounter = 0;
        m_ScrollDelay = 2.0f;
        CalculateTextRect();
        return m_Text.Length() * m_TypeSpeed;
    }
    int AddText(EXString s, int speed, int delay) { return SetText(m_Text + s, speed, delay); }
    FEObject *SetData(unsigned __int64 d)
    {
        m_Data = d;
        return this;
    }
    unsigned __int64 Data() { return m_Data; }
    unsigned int Data32() { return (unsigned int)m_Data; }
    FEObject *SetAltData(unsigned __int64 d)
    {
        m_AltData = d;
        return this;
    }
    unsigned __int64 AltData() { return m_AltData; }

    // Colour.
    ex::RGBA MultiplyMathmos(ex::RGBA c, int i)
    {
        ex::RGBA m;
        if (m_pRemoteMathmos[i]) m = m_pRemoteMathmos[i]->Colour();
        else m = m_Mathmos[i].Colour();

        ex::RGBA r;
        r.m_R = (u8)(c.m_R * ((float)m.m_R / 128.0f));
        r.m_G = (u8)(c.m_G * ((float)m.m_G / 128.0f));
        r.m_B = (u8)(c.m_B * ((float)m.m_B / 128.0f));
        r.m_A = (u8)(c.m_A * ((float)m.m_A / 128.0f));
        return r;
    }
    static ex::RGBA MultiplyColour(ex::RGBA a, ex::RGBA b);

    // Drawing through the owner window.
    // NEAR (emitted copies in FELayout.obj): retail builds OutRect(r) in the argument slot first and calls
    // MultiplyMathmos out of line.
    void DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                    EXRect r, EXVector2 uv0, EXVector2 uv1)
    {
        if (m_pOwnerWnd)
            m_pOwnerWnd->DrawSprite(pass, file, hash, MultiplyMathmos(MultiplyMathmos(c, 2), 3), blend, OutRect(r), uv0, uv1);
    }
    void DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                  EXRect r, EXString s, EXW::TextAlign align, unsigned int n)
    {
        if (m_pOwnerWnd)
            m_pOwnerWnd->DrawText(pass, file, hash, MultiplyMathmos(MultiplyMathmos(c, 2), 3), blend, OutRect(r), s, align, n);
    }
    void DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                    unsigned int datum, EXPoint size, EXW::TextAlign align)
    {
        if (m_pOwnerWnd)
            m_pOwnerWnd->DrawSprite(pass, file, hash, MultiplyMathmos(MultiplyMathmos(c, 2), 3), blend, this, datum, size, align);
    }
    void DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                  unsigned int datum, EXString s, EXW::TextAlign align, unsigned int n)
    {
        if (m_pOwnerWnd)
            m_pOwnerWnd->DrawText(pass, file, hash, MultiplyMathmos(MultiplyMathmos(c, 2), 3), blend, this, datum, s, align, n);
    }
    void DrawSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                    EXPoint p, EXPoint size, EXW::TextAlign align)
    {
        if (m_pOwnerWnd)
            m_pOwnerWnd->DrawSprite(pass, file, hash, MultiplyMathmos(MultiplyMathmos(c, 2), 3), blend, OutPoint(p), size, align);
    }
    void DrawText(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                  EXPoint p, EXString s, EXW::TextAlign align, unsigned int n)
    {
        if (m_pOwnerWnd)
            m_pOwnerWnd->DrawText(pass, file, hash, MultiplyMathmos(MultiplyMathmos(c, 2), 3), blend, OutPoint(p), s, align, n);
    }
    void DrawBorderSprite(FERenderPass::Flags pass, unsigned int file, unsigned int hash, ex::RGBA c, unsigned int blend,
                          EXRect r, unsigned int border)
    {
        if (m_pOwnerWnd)
            m_pOwnerWnd->DrawBorderSprite(pass, file, hash, MultiplyMathmos(MultiplyMathmos(c, 2), 3), blend, OutRect(r), border);
    }

    void InitRect(int x, int y, int w, int h);
    FEObjectScript *Script(unsigned __int64 id);
    FEListener *Listener(unsigned __int64 id);
    FEObject *ListenerClear();
    void PlayEvent(FEObjEvent::Flags ev);
    FEMathmos *Mathmos(FEObjColour::Flags which);
    FEMathmos *RemoteMathmos(FEObjColour::Flags which);
    signed char RectOutside(EXRect r);
    void CalculateTextRect();
    FEObject *SetOwnerWnd(FEWnd *p);
    FEObjectScript *ScriptAdd(unsigned __int64 id, FEObjType::Flags type, unsigned int file, unsigned int hash,
                              EXW::TextAlign align);
    signed char ScriptDel(unsigned __int64 id);
    FEObject *KeyframeAdd(EXPoint target, float time);
    void DoObjectAlign();
    FEObject *SetRect(EXRect r);
    signed char RectInside(EXRect r);
    FEObject *MakeSafeRect();
    EXVector FindDatum(unsigned int hash);
    EXPoint FindDatum2D(unsigned int hash);
    void Init(FEObjType::Flags type, unsigned int file, unsigned int hash, unsigned __int64 data, EXString text,
              EXString help, EXW::TextAlign textAlign, unsigned int x, unsigned int y, unsigned int w, unsigned int h,
              EXW::TextAlign objAlign);
    FEObject *KeyframeClear();
    void Cleanup();

    FEObject *m_pParent;                 // 0x08
    FEObject *m_pMathmosLink;            // 0x0c colour source
    FEWnd *m_pOwnerWnd;                  // 0x10
    EXRect m_Rect;                       // 0x14 local rect (offset from m_BaseRect's origin)
    EXRect m_BaseRect;                   // 0x24
    EXRect m_TextRect;                   // 0x34
    EXString m_Text;                     // 0x44
    EXString m_HelpText;                 // 0x48
    unsigned int m_Texture[2];           // 0x4c file, hash
    unsigned int m_Font[2];              // 0x54 file, hash
    u32 m_Unk5c;                         // 0x5c
    unsigned __int64 m_Data;             // 0x60
    unsigned __int64 m_AltData;          // 0x68
    unsigned int m_Flags;                // 0x70
    EXStack<unsigned int> m_StateStack;  // 0x74
    FEMathmos *m_pRemoteMathmos[4];      // 0x90
    FEMathmos m_Mathmos[4];              // 0xa0 indexed by FEObjColour
    EXW::TextAlign m_ObjectAlign;        // 0x140
    EXW::TextAlign m_TextAlign;          // 0x144
    unsigned int m_BlendMode;            // 0x148
    EXVector2 m_TexCoord0;               // 0x14c
    EXVector2 m_TexCoord1;               // 0x154
    EXVector2 m_OriginFloat;             // 0x15c
    EXDList m_Scripts;                   // 0x164 FEObjectScript (item at +8)
    EXDList m_Keyframes;                 // 0x16c FEKeyframe
    EXDList m_Listeners;                 // 0x174 FEListener (item at +8)
    unsigned int m_BorderWidth;          // 0x17c
    ex::RGBA m_Colour;                   // 0x180
    signed char m_DeleteNeeded;          // 0x184
    signed char m_CleanedUp;             // 0x185
    int m_TypeDelay;                     // 0x188 frames before typing starts
    int m_TypeSpeed;                     // 0x18c frames per character (0: all at once)
    int m_TypeCounter;                   // 0x190
    int m_TypeChars;                     // 0x194 characters shown
    void *m_pBind;                       // 0x198
    FEDataType::Flags m_BindType;        // 0x19c
    unsigned __int64 m_BindValue;        // 0x1a0
    float m_ScrollPos;                   // 0x1a8
    float m_ScrollDelay;                 // 0x1ac
    FEEventCallback m_pEventCallback;    // 0x1b0
};
#endif
