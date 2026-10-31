#include "EXLine.h"
#include "EXMatrix.h"

#define EX_LINE_TOL 0.0001f

// NEAR: large diff (>600 bytes): the original keeps m_Origin in xmm2-5 and builds the dot products in
// SSE from int-copied temporaries; the expression shapes here inline differently.
EXLineSeg::LINE_POS EXLineSeg::NearestTo(const EXVector &p, EXVector &out) const
{
    EXVector end = m_Origin + m_Direction;
    if (m_Direction.DotXYZ(end - p) <= 0.0f) {
        out = end;
        return LINE_END;
    }
    float t = m_Direction.DotXYZ(p - m_Origin);
    if (t > 0.0f) {
        float len2 = m_Direction.LengthXYZSquared();
        if ((float)fabs(len2) > 0.0f) {
            out = m_Origin + m_Direction * (t / len2);
            return LINE_MIDDLE;
        }
    }
    out = m_Origin;
    return LINE_START;
}

s8 EXLineSeg::PtOnLine(const EXVector &p) const
{
    EXVector n;
    NearestTo(p, n);
    if ((n - p).LengthXYZSquared() < EX_LINE_TOL * EX_LINE_TOL) return 1;
    return 0;
}

void EXLine::ApplyXForm(const EXMatrix &m)
{
    m_Origin.ApplyXForm(m);
    m_Direction *= m;
}

void EXRay::ApplyXForm(const EXMatrix &m)
{
    m_Origin.ApplyXForm(m);
    m_Direction *= m;
}

void EXLineSeg::ApplyXForm(const EXMatrix &m)
{
    m_Origin.ApplyXForm(m);
    m_Direction *= m;
}
