#ifndef FELAYOUT_H
#define FELAYOUT_H
#include "FEObject.h"

// Container object: owns an array of child objects and forwards most calls to them (FELayout.obj).
// sizeof 0x1c0. The derived layouts below live in the same object.
class FELayout : public FEObject {
public:
    virtual ~FELayout();
    virtual void Pause(signed char pause);
    virtual void NotifyDisplay(signed char show);
    virtual FEObject *SetMathmos(FEObjColour::Flags which, FEMathmos m, signed char b);
    virtual FEObject *SetRemoteMathmos(FEObjColour::Flags which, FEMathmos *p, signed char b);
    // NEAR: retail returns through an address-taken local temporary
    virtual ex::RGBA MathmosLinkColour()
    {
        ex::RGBA c;
        if (ChildSel()) c = ChildSel()->m_Colour;
        else c = m_Colour;
        return c;
    }
    virtual FEObject *MathmosGoto(FEMathmosPos::Flags pos);
    virtual signed char HandleInput(signed char b);
    virtual void BindCommit();
    virtual signed char BindRecall();
    virtual void Update();
    virtual void Draw();
    virtual FEObject *SetFocus(signed char focus, signed char b, signed char c);
    virtual void DebugDump(unsigned int n);
    virtual char *DebugName() { return "LtBase"; }
    virtual FEObject *SetFont(unsigned int file, unsigned int hash, signed char b);
    virtual FEObject *SetEventCallback(FEEventCallback p, signed char b);
    virtual signed char DoEventCallback(FEObjEvent::Flags ev);
    virtual unsigned int ChildCount() { return m_ChildCount; }
    virtual FEObject *ObjectAdd(unsigned int i, FEObject *p);
    virtual void ObjectDel(unsigned int i);
    virtual FEObject *Object(unsigned int i);
    virtual FEObject *ChildSel() { return m_pChildren ? m_pChildren[0] : 0; }
    virtual FEObject *FindChildObject(unsigned __int64 data);
    virtual FEListener *FindChildListener(unsigned __int64 id);

    FELayout();

    FEObject **m_pChildren;      // 0x1b8
    unsigned int m_ChildCount;   // 0x1bc
};

// Children stacked on top of each other, each filling the layout's rect. sizeof 0x1c0.
class FELayoutLayered : public FELayout {
public:
    virtual char *DebugName() { return "LtLayered"; }
    virtual FEObject *Resize();
    FELayoutLayered();
    FELayoutLayered(unsigned int count, unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                    EXW::TextAlign align);
    void InitLayered(unsigned int count, unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                     EXW::TextAlign align);
};

// m_Cols x m_Rows grid of equal cells; child (i, j) at m_pChildren[j * m_Cols + i]. sizeof 0x1c8.
class FELayoutFixedGrid : public FELayout {
public:
    virtual char *DebugName() { return "LtFixedGrid"; }
    virtual FEObject *Resize();
    virtual FEObject *ObjectAdd(unsigned int i, unsigned int j, FEObject *p);
    virtual void ObjectDel(unsigned int i, unsigned int j);
    virtual FEObject *Object(unsigned int i, unsigned int j);
    FELayoutFixedGrid() {}
    FELayoutFixedGrid(unsigned int cols, unsigned int rows, unsigned int x, unsigned int y, unsigned int w,
                      unsigned int h, EXW::TextAlign align);

    unsigned int m_Cols;  // 0x1c0
    unsigned int m_Rows;  // 0x1c4
};

// Grid whose column widths / row heights follow the largest child; the layout sizes itself.
class FELayoutFlexGrid : public FELayoutFixedGrid {
public:
    virtual char *DebugName() { return "LtFlexGrid"; }
    virtual FEObject *Resize();
    FELayoutFlexGrid() {}
    FELayoutFlexGrid(unsigned int cols, unsigned int rows, EXW::TextAlign align);
    void Init(unsigned int cols, unsigned int rows, EXW::TextAlign align);
};

// Fixed grid showing a window of the cells, with scroll arrows. sizeof 0x210.
class FELayoutScrollGrid : public FELayoutFixedGrid {
public:
    virtual signed char HandleInput(signed char b);
    virtual void EnsureVisible(FEObject *p, signed char b);
    virtual void Draw();
    virtual char *DebugName() { return "LtScrollGrid"; }
    virtual FEObject *Resize();
    // NEAR: retail loads the vptr after both stores
    virtual FEObject *SetVisibleCount(unsigned int x, unsigned int y)
    {
        m_Visible.x = y;
        m_Visible.y = x;
        Resize();
        return this;
    }
    virtual EXPoint VisibleCount() { return m_Visible; }
    virtual FEObject *SetSmoothScroll(signed char b)
    {
        m_SmoothScroll = b;
        return this;
    }
    virtual signed char SmoothScroll() { return m_SmoothScroll; }
    virtual FEObject *SetScrollAnchor(signed char b)
    {
        m_ScrollAnchor = b;
        return this;
    }
    virtual signed char ScrollAnchor() { return m_ScrollAnchor; }
    virtual FEObject *ChildSel();
    FELayoutScrollGrid(unsigned int cols, unsigned int rows, unsigned int x, unsigned int y, unsigned int w,
                       unsigned int h, EXW::TextAlign align);

    void SetUpDownScrollArrowForceY(int y) { m_UpDownArrowY = y; }
    void SetGridWidthProportions(float *p) { m_pGridWidths = p; }
    EXPoint TopIndex() { return m_Top; }

    EXPoint m_Top;              // 0x1c8 top-left visible cell
    EXPoint m_Visible;          // 0x1d0 visible cells
    signed char m_Scrolling;    // 0x1d8 animate the next Resize
    signed char m_SmoothScroll; // 0x1d9
    signed char m_ScrollAnchor; // 0x1da
    FEMathmos m_ArrowMathmos;   // 0x1dc
    int m_LeftRightArrowY;      // 0x204 < -100: centred
    int m_UpDownArrowY;         // 0x208 < -100: 8
    float *m_pGridWidths;       // 0x20c column widths as fractions of the rect width
};

// Flex grid scrolled by a pixel offset. sizeof 0x1fc.
class FELayoutFlexScrollGrid : public FELayoutFlexGrid {
public:
    virtual signed char HandleInput(signed char b);
    virtual void EnsureVisible(FEObject *p, signed char b);
    virtual void Draw();
    virtual char *DebugName() { return "LtFlexScrollGrid"; }
    virtual FEObject *Resize();
    virtual FEObject *SetSmoothScroll(signed char b)
    {
        m_SmoothScroll = b;
        return this;
    }
    virtual signed char SmoothScroll() { return m_SmoothScroll; }
    virtual FEObject *ChildSel();
    FELayoutFlexScrollGrid(unsigned int cols, unsigned int rows, unsigned int x, unsigned int y, unsigned int w,
                           unsigned int h, EXW::TextAlign align);

    signed char m_Scrolling;    // 0x1c8
    signed char m_SmoothScroll; // 0x1c9
    EXPoint m_Scroll;           // 0x1cc
    FEMathmos m_ArrowMathmos;   // 0x1d4
};

// Children placed along a curve (m_Curve) down the layout. sizeof 0x1c8.
class FELayoutCurve : public FELayout {
public:
    virtual char *DebugName() { return "LtCurve"; }
    virtual FEObject *Resize();
    FELayoutCurve(unsigned int count, EXW::TextAlign curve, EXW::TextAlign childAlign, unsigned int x,
                  unsigned int y, unsigned int w, unsigned int h, EXW::TextAlign align);

    EXW::TextAlign m_Curve;      // 0x1c0
    EXW::TextAlign m_ChildAlign; // 0x1c4
};

// Tick box drawn as a font glyph. sizeof 0x1c4.
class FELayoutCheckbox : public FELayout {
public:
    virtual signed char HandleInput(signed char b);
    virtual void BindUpdate(unsigned __int64 data);
    virtual signed char BindRecall();
    virtual void Draw();
    virtual char *DebugName() { return "LtCheckbox"; }
    virtual int Value() { return m_Checked; }
    virtual FEObject *SetValue(signed char b);
    FELayoutCheckbox(signed char b, unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                     EXW::TextAlign align);

    signed char m_Checked;  // 0x1c0
    signed char m_Glyph;    // 0x1c1
};

// Number spinner with left/right arrows. sizeof 0x21c.
class FELayoutSpin : public FELayout {
public:
    virtual signed char HandleInput(signed char b);
    virtual void BindUpdate(unsigned __int64 data);
    virtual signed char BindRecall();
    virtual char *DebugName() { return "LtSpin"; }
    virtual FEObject *Resize();
    virtual FEObject *SetRange(int lo, int hi);
    virtual int Range(int &lo, int &hi);
    virtual FEObject *SetDelta(int d)
    {
        m_Delta = d;
        return this;
    }
    virtual int Delta() { return m_Delta; }
    virtual FEObject *SetValue(int v);
    virtual int Value() { return m_Value; }
    FELayoutSpin(unsigned int lo, unsigned int hi, unsigned int x, unsigned int y, unsigned int w, unsigned int h,
                 EXW::TextAlign align);

    int m_Min;                   // 0x1c0
    int m_Max;                   // 0x1c4
    int m_Value;                 // 0x1c8
    u32 m_Unk1cc;                // 0x1cc
    int m_Delta;                 // 0x1d0
    FEKeyRepeater m_KeyRepeater; // 0x1d4
    FEMathmos m_ArrowMathmos;    // 0x1f4
};

// Horizontal slider bar. sizeof 0x22c.
class FELayoutSlider : public FELayout {
public:
    virtual signed char HandleInput(signed char b);
    virtual void BindUpdate(unsigned __int64 data);
    virtual signed char BindRecall();
    virtual void Draw();
    virtual char *DebugName() { return "LtSlider"; }
    virtual FEObject *Resize();
    virtual FEObject *SetRange(int lo, int hi);
    virtual int Range(int &lo, int &hi);
    virtual FEObject *SetDelta(int d)
    {
        m_Delta = d;
        return this;
    }
    virtual int Delta() { return m_Delta; }
    virtual FEObject *SetValue(int v);
    virtual int Value() { return m_Value; }
    FELayoutSlider(unsigned int lo, unsigned int hi, unsigned int x, unsigned int y, unsigned int w,
                   unsigned int h, EXW::TextAlign align);

    int m_Min;                   // 0x1c0
    int m_Max;                   // 0x1c4
    int m_Value;                 // 0x1c8
    u32 m_Unk1cc;                // 0x1cc
    int m_Delta;                 // 0x1d0
    FEKeyRepeater m_KeyRepeater; // 0x1d4
    EXRect m_BarRect;            // 0x1f4
    FEMathmos m_ArrowMathmos;    // 0x204
};

// One child page visible at a time, chosen by the focus of m_pListener; keeps a back history.
// sizeof 0x1f0.
class FELayoutPaged : public FELayout {
public:
    virtual ~FELayoutPaged();
    virtual signed char HandleInput(signed char b);
    virtual void Update();
    virtual void Draw();
    virtual char *DebugName() { return "LtPaged"; }
    virtual FEObject *Resize();
    virtual FEObject *ChildSel() { return m_pPage; }
    virtual void PageSelect(unsigned __int64 id, signed char push, signed char notify);
    virtual FEObject *Page(unsigned __int64 id, int *pIndex);
    virtual signed char PageBack();
    virtual void PageFlush();
    FELayoutPaged(unsigned int count, FEListener *pListener, unsigned int x, unsigned int y, unsigned int w,
                  unsigned int h, EXW::TextAlign align);

    FEListener *m_pListener;        // 0x1c0
    void **m_pPageData;             // 0x1c4 one slot per page
    u32 m_Unk1c8;                   // 0x1c8
    u32 m_Unk1cc;                   // 0x1cc
    FEObject *m_pPage;              // 0x1d0
    EXStack<FEObject *> m_History;  // 0x1d4
};
#endif
