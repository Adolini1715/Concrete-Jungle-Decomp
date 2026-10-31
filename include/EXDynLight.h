#ifndef EXDYNLIGHT_H
#define EXDYNLIGHT_H
#include "EXVector.h"
#include "XRGBA.h"

struct EXDynLightData;

// Dynamic light (EXLightManager.obj; the inlines below are emitted in EXItemAnimator_DynLight.obj).
// sizeof 0x44 (allocated in blocks of 16 by EXLightManager).
class EXDynLight {
public:
    EXVector Pos() const { return EXVector(m_pos); }
    EXVector Dir() const { return EXVector(m_dir); }
    // NEAR (8 bytes): scheduling of the local copy of v.
    void SetPos(const EXVector &v)
    {
        EXMaths::vec_xyzw p = v;
        if (EXIsEqual(p.x, m_pos.x) && EXIsEqual(p.y, m_pos.y) && EXIsEqual(p.z, m_pos.z) && EXIsEqual(p.w, m_pos.w)) return;
        m_pos = v;
        m_changed = 1;
    }
    void SetDir(const EXVector &v) { m_dir = (const EXMaths::vec_xyzw)v; }
    s8 Directional() const { return m_directional; }
    void SetColor(ex::XRGBA c) { m_color = c; UpdateCol(); }
    void SetActive(s8 b) { m_active = b; m_changed = 1; }
    s8 IsActive() const { return m_active; }
    void SetChangedFlag(s8 b) { m_changed = b; }
    s8 HasChanged() const { return m_changed; }
    float OuterRadius() const { return m_outerRadius; }
    // Emitted in EXItemAnimator_ScriptControllers.obj.
    void SetOuterRadius(float r)
    {
        if (r != m_outerRadius) {
            m_outerRadius = r;
            m_changed = 1;
        }
    }
    void SetAffectsOneZone(s8 b) { m_affectsOneZone = b; m_changed = 1; }
    s8 AffectsOneZone() const { return m_affectsOneZone; }
    EXDynLightData *Data() { return (EXDynLightData *)this; }
    void UpdateCol();
    void SetPointLight(ex::XRGBA c, const EXVector &pos, float inner, float outer);
    void SetPointLight(ex::XRGBA c, float inner, float outer);
    void SetDirPointLight(ex::XRGBA c, const EXVector &pos, const EXVector &dir, float inner, float outer);
    void SetDirPointLight(ex::XRGBA c, const EXVector &dir, float inner, float outer);

    EXMaths::vec_xyzw m_pos;    // +0
    EXMaths::vec_xyzw m_dir;    // +0x10
    EXMaths::vec_xyzw m_colorF; // +0x20 m_color / 255
    ex::XRGBA m_color;          // +0x30
    float m_innerRadius;        // +0x34
    float m_outerRadius;        // +0x38
    s8 m_directional;           // +0x3c
    u8 m_3d[3];
    s8 m_changed;               // +0x40
    s8 m_active;                // +0x41
    s8 m_affectsOneZone;        // +0x42
};
#endif
