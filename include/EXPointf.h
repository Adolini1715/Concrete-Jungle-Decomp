#ifndef EXPOINTF_H
#define EXPOINTF_H

// 2D float point. Inline members emitted in FEMainMenu.obj.
class EXPointf {
public:
    EXPointf() {}
    EXPointf(float x_, float y_) { x = x_; y = y_; }
    EXPointf(const EXPointf &p) { x = p.x; y = p.y; }
    EXPointf &operator=(const EXPointf &p) { x = p.x; y = p.y; return *this; }
    float x, y;
};
#endif
