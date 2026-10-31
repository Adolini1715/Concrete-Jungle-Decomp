#ifndef EXDATUM_H
#define EXDATUM_H
#include "EXVector.h"
#include "EXQuaternion.h"
#include "EXArray.h"

class EXMatrix;
class EXSphere;
class EXCapsule;
class EXLozenge;
class EXBox;
class EXCylinder;
struct EXGeoBaseDatum;

// A datum (collision/attach shape) resolved to world or local space (sizeof 0x30).
// Copy ctor and assignment are the implicit ones (EXVector/EXQuaternion set(), dword copies).
struct EXDatumTemp {
    EXDatumTemp() {}
    EXVector Position() const { return m_position; }
    float &Radius() { return m_size.x; }
    void Set(const EXGeoBaseDatum *pDatum, const EXVector &scale);
    void Set(const EXGeoBaseDatum *pDatum, const EXVector &scale, const EXMatrix &mtx);
    void Get(EXSphere &s) const;
    void Get(EXCapsule &c) const;
    void Get(EXLozenge &l) const;
    void Get(EXBox &b) const;
    void Get(EXCylinder &c) const;
    void Get(EXVector &v) const;
    void ApplyXForm(const EXMatrix &mtx);

    EXVector m_position;        // +0
    EXQuaternion m_rotation;    // +0x10
    EXVector3 m_size;           // +0x20 (radius in x)
    u16 m_2c;                   // +0x2c TODO
    u8 m_2e;                    // +0x2e TODO
    u8 m_type;                  // +0x2f TODO name (shape type)
};

class EXDatumTempArray : public EXDataArray<EXDatumTemp, EXDatumTemp &> {
};
#endif
