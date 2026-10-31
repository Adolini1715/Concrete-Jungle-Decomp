#include "EXRect.h"

// NEAR: ~42 bytes differ, register choice for the right/bottom max (cmovg into b's register).
EXRect IntersectRect(const EXRect &a, const EXRect &b)
{
    EXPoint tl = a.TopLeft();
    EXPoint rb(a.x + a.w, a.y + a.h);
    EXPoint btl = b.TopLeft();
    EXPoint brb(b.x + b.w, b.y + b.h);
    int l = tl.x > btl.x ? tl.x : btl.x;
    int t = tl.y > btl.y ? tl.y : btl.y;
    l = l < brb.x ? l : brb.x;
    t = t < brb.y ? t : brb.y;
    int r = btl.x > rb.x ? btl.x : rb.x;
    int bt = btl.y > rb.y ? btl.y : rb.y;
    r = r < brb.x ? r : brb.x;
    bt = bt < brb.y ? bt : brb.y;
    return EXRect(l, t, r - l, bt - t);
}

EXRectf IntersectRect(const EXRectf &a, const EXRectf &b)
{
    float ar = a.x + a.w, ab = a.y + a.h;
    float br = b.x + b.w, bb = b.y + b.h;
    float l = a.x > b.x ? a.x : b.x;
    float t = a.y > b.y ? a.y : b.y;
    if (!(br > l)) l = br;
    if (!(bb > t)) t = bb;
    if (!(ar > b.x)) ar = b.x;
    if (!(ab > b.y)) ab = b.y;
    if (!(br > ar)) ar = br;
    if (!(bb > ab)) ab = bb;
    return EXRectf(l, t, ar - l, ab - t);
}
