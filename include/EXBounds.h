#ifndef EXBOUNDS_H
#define EXBOUNDS_H
#include "EXVector.h"

class EXMatrix;

// Axis-aligned box, min corner then max corner (XBBounds.obj).
struct MXBoundsBox {
    const EXMaths::vec_xyzw &vctMin() const { return m_vctMin; }
    const EXMaths::vec_xyzw &vctMax() const { return m_vctMax; }
    s8 _Intersect(const EXMaths::vec_xyzw &p, float r) const;
    s8 _Intersect(const MXBoundsBox &b) const;
    s8 _Intersect(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b) const;
    void _ApplyXForm(const EXMaths::mat_44 &m);

    EXMaths::vec_xyzw m_vctMin;
    EXMaths::vec_xyzw m_vctMax;
};

// The engine bounds box: MXBoundsBox with the EX API (EXBounds.obj). Inline shapes not all verified.
struct EXBoundsBox : public MXBoundsBox {
    void SetMin(const EXVector &v) { m_vctMin = v; }
    void SetMax(const EXVector &v) { m_vctMax = (const EXMaths::vec_xyzw)v; }
    void Set(const EXVector &a, const EXVector &b) { SetMin(a); SetMax(b); }
    void Set(const EXMaths::vec_xyzw &a, const EXMaths::vec_xyzw &b) { m_vctMin = a; m_vctMax = b; }
    void Set(const EXBoundsBox &b) { m_vctMin = b.m_vctMin; m_vctMax = b.m_vctMax; }
    void SetZero() { m_vctMin.set_zero(); m_vctMax.set_zero(); }
    // Empty box for accumulation (emitted in EXItemAnimator_Anim.obj; verified inlined in
    // EXItemAnimator_Anim::_GetBoundsBox).
    void SetMaxRange()
    {
        m_vctMin.set(1000000.0f, 1000000.0f, 1000000.0f, 0.0f);
        m_vctMax = EXVector(-1000000.0f, -1000000.0f, -1000000.0f, 0.0f);
    }
    operator const float *() const { return &m_vctMin.x; }
    s8 Intersect(const EXVector &p, float r) const { return _Intersect(p, r); }
    s8 Intersect(const EXVector &a, const EXVector &b) const { return _Intersect(a, b); }
    s8 Intersect(const EXBoundsBox &b) const { return _Intersect(b); }
    void ApplyXForm(const EXMatrix &m);

    void Set(const EXVector &c, float r);
    float CentreAndRadius(EXVector &c) const;
    float CentreAndRadiusSquared(EXVector &c) const;
    float Volume() const;
    void Accumulate(const EXBoundsBox &b);
};

// Compact 3-float bounds box (EXBounds.obj). A struct: EXItemEnv.obj mangles it 'U'.
struct EXBoundsBox3 {
    EXBoundsBox3() {}
    const EXVector3 &vctMin() const { return m_vctMin; }
    const EXVector3 &vctMax() const { return m_vctMax; }
    s8 Intersect(const EXVector &p, float r) const;
    void Set(const EXBoundsBox &b);
    s8 Intersect(const EXBoundsBox &b) const;
    s8 Intersect(const EXVector &a, const EXVector &b) const;

    EXVector3 m_vctMin;
    EXVector3 m_vctMax;
};
#endif
