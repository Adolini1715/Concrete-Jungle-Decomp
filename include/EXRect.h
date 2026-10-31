#ifndef EXRECT_H
#define EXRECT_H
#include "types.h"

class EXPoint {
public:
    EXPoint() {}
    EXPoint(int x_, int y_) { x = x_; y = y_; }
    EXPoint(const EXPoint &p) { x = p.x; y = p.y; }
    EXPoint &operator=(const EXPoint &p) { x = p.x; y = p.y; return *this; }
    EXPoint &operator+=(const EXPoint &p) { x += p.x; y += p.y; return *this; }
    EXPoint &operator-=(const EXPoint &p) { x -= p.x; y -= p.y; return *this; }
    int x, y;
};

// Emitted in FELayout.obj / FEMainMenu.obj.
__forceinline s8 operator==(const EXPoint &a, const EXPoint &b) { return a.x == b.x && a.y == b.y; }
inline s8 operator!=(const EXPoint &a, const EXPoint &b) { return a.x != b.x || a.y != b.y; }
inline EXPoint operator+(const EXPoint &a, const EXPoint &b) { return EXPoint(a.x + b.x, a.y + b.y); }
inline EXPoint operator-(const EXPoint &p) { return EXPoint(-p.x, -p.y); }  // emitted in FEMainMenu.obj
inline EXPoint operator-(const EXPoint &a, const EXPoint &b) { return EXPoint(a.x - b.x, a.y - b.y); }

// Integer rectangle: origin plus size.
class EXRect {
public:
    EXRect() {}
    EXRect(int x_, int y_, int w_, int h_) { x = x_; y = y_; w = w_; h = h_; }
    EXRect(const EXRect &r) { x = r.x; y = r.y; w = r.w; h = r.h; }
    ~EXRect() {}
    EXRect &operator=(const EXRect &r) { x = r.x; y = r.y; w = r.w; h = r.h; return *this; }
    void Set(int x_, int y_, int w_, int h_) { x = x_; y = y_; w = w_; h = h_; }
    int Width() const { return w; }
    int Height() const { return h; }
    void Inflate(u32 dx, u32 dy) { x -= dx; y -= dy; w += dx * 2; h += dy * 2; }
    void Deflate(u32 dx, u32 dy) { x += dx; y += dy; w -= dx << 1; h -= dy << 1; }
    void Offset(int dx, int dy) { x += dx; y += dy; }
    void Offset(const EXPoint &p) { Offset(p.x, p.y); }
    EXPoint TopLeft() const { return EXPoint(x, y); }
    // Corners other than TopLeft are inclusive (last pixel).
    EXPoint TopRight() const { return EXPoint(x + w - 1, y); }
    EXPoint BottomLeft() const { return EXPoint(x, y + h - 1); }
    EXPoint BottomRight() const { return EXPoint(x + w - 1, y + h - 1); }
    s8 IsEmpty() const { return w == 0 && h == 0; }
    // NEAR (emitted copy in EXWnd.obj): the retail out-of-line copy places the first early-out return
    // inline; this && form matches every inlined use checked (EXDebugWnd::ButtonDown).
    s8 ContainsPoint(const EXPoint &p) const
    {
        return p.x >= x && p.y >= y && p.x <= x + w - 1 && p.y <= y + h - 1;
    }
    int x, y, w, h;
};

class EXRectf {
public:
    EXRectf() {}
    EXRectf(float x_, float y_, float w_, float h_) { x = x_; y = y_; w = w_; h = h_; }
    ~EXRectf() {}
    float x, y, w, h;
};

EXRect IntersectRect(const EXRect &a, const EXRect &b);
EXRectf IntersectRect(const EXRectf &a, const EXRectf &b);
#endif
