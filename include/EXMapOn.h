#ifndef EXMAPON_H
#define EXMAPON_H
#include "types.h"

// Which map (high byte) and zone (low byte) an item is on; 0xffff = none.
// TODO: the (int) constructor is an unverified shape.
class EXMapOn {
public:
    EXMapOn() {}
    EXMapOn(int zone) { m_v = (u16)(zone | 0xffffff00); }
    // Shape verified against the EXItemEnv.obj copy (item agent): map is not masked before the shift.
    EXMapOn(int map, int zone)
    {
        m_v = (u16)((map << 8) | (zone & 0xff));
    }
    EXMapOn operator=(EXMapOn o) { m_v = o.m_v; return o; }
    void Set(int map, int zone) { m_v = (u16)(((u8)map << 8) | (u8)zone); }  // NEAR out of line (retail movzx ax, mov ah)
    s8 IsValid() const { return m_v != -1; }
    u8 Zone() const { return *(const u8 *)&m_v; }
    s8 Map() const { return ((const s8 *)&m_v)[1]; }
    s16 m_v;    // signed: the EXItemEnv.obj / EXSoundManager.obj copies compare with -1 and shift with sar
};
inline s8 operator!=(EXMapOn a, EXMapOn b) { return a.m_v != b.m_v; }  // EXItemEnv.obj
#endif
