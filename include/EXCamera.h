#ifndef EXCAMERA_H
#define EXCAMERA_H
#include "types.h"
#include "EXVector.h"
#include "EXRect.h"
#include "EXMatrix.h"
#include "EXString.h"


namespace ex { double fd(float f); }

// Camera parameters shared by game cameras and projectors (EXCamera.obj). The setters log through
// DebugMessage when their bit in m_debugFlags is set: 1 SetPosition, 2 SetTarget, 8 SetZRange,
// 0x20 SetAspectCorrection, 0x40 SetOrigin, 0x80 SetClipSize, 0x200 SetName, 0x8000 UpdateDisplay,
// 0x800 SetVFOV, 0x4000 SetFocalLength.
class EXCommonCamera {
public:
    virtual ~EXCommonCamera()
    {
        if (m_pName) {
            delete m_pName;
            m_pName = 0;
        }
    }
    virtual void UpdateDisplay(unsigned int flags) = 0;
    EXCommonCamera()
    {
        m_debugFlags = 0;
        m_pName = 0;
        m_bPerspective = 1;
    }
    const EXVector &Position() const { return m_Position; }        // EXItemEnv.obj
    const EXVector &Target() const { return m_Target; }            // EXItemEnv.obj
    const EXVector &WorldUp() const { return m_WorldUp; }
    signed char Perspective() const { return m_bPerspective; }     // XBCamera.obj
    void SetPerspective(signed char b) { m_bPerspective = b; }
    __forceinline void SetPosition(const EXVector &v)
    {
        if (m_debugFlags & 1)
            DebugMessage("SetPosition(%f,%f,%f,%f)", ex::fd(v.x), ex::fd(v.y), ex::fd(v.z), ex::fd(v.w));
        m_Position = v;
    }
    __forceinline void SetTarget(const EXVector &v)
    {
        if (m_debugFlags & 2)
            DebugMessage("SetTarget(%f,%f,%f,%f)", ex::fd(v.x), ex::fd(v.y), ex::fd(v.z), ex::fd(v.w));
        m_Target = v;
    }
    __forceinline void SetWorldUp(const EXVector &v)  // inlined in CameraScript.obj ([fi] copy in Camera.obj)
    {
        if (m_debugFlags & 4)
            DebugMessage("SetWorldUp(%f,%f,%f,%f)", ex::fd(v.x), ex::fd(v.y), ex::fd(v.z), ex::fd(v.w));
        m_WorldUp = v;
    }
    __forceinline void SetZRange(float zNear, float zFar)
    {
        if (m_debugFlags & 8) DebugMessage("SetZRange(%f,%f)", ex::fd(zNear), ex::fd(zFar));
        m_ZNear = zNear;
        m_ZFar = zFar;
    }
    float HFOV() const;
    float VFOV() const;
    float HZoom() const;
    float VZoom() const;
    float FocalLength() const;
    const EXRectf &ViewSpaceSize() const;
    void SetName(const char *pName);
    void SetVFOV(const EXRectf &r, float f);
    __forceinline void SetVFOV(const EXRect &r, float f)  // [fi] copy in Camera.obj
    {
        EXRectf rf((float)r.x, (float)r.y, (float)r.w, (float)r.h);
        SetVFOV(rf, f);
    }
    void SetVZoom(const EXRectf &r, float f);
    void SetFocalLength(const EXRectf &r, float f);
    void SetHFOV(const EXRectf &r, float f);
    void SetHZoom(const EXRectf &r, float f);
protected:
    void DebugMessage(const char *pFmt, ...);
public:
    EXRectf m_viewSpaceSize;    // +4
    float m_focalLength;        // +0x14
    float m_vfov;               // +0x18
    EXVector m_Position;        // +0x1c
    EXVector m_Target;          // +0x2c
    EXVector m_WorldUp;         // +0x3c
    float m_ZNear;              // +0x4c
    float m_ZFar;               // +0x50
    float m_54;                 // +0x54 TODO (0; set by EXCamera/EXProjector)
    float m_58;                 // +0x58 TODO (1)
    s8 m_bPerspective;          // +0x5c
    EXString *m_pName;          // +0x60
    u32 m_debugFlags;           // +0x64
};

// Window camera: screen origin and clip size, aspect correction, projection (EXCamera.obj).
class EXBaseCamera : public EXCommonCamera {
public:
    EXBaseCamera();
    virtual ~EXBaseCamera();
    const EXVector &LookAt() const { return m_Target; }
    const EXVector2 &Origin() const { return m_Origin; }
    // Same body as EXCommonCamera::SetTarget (its debug text too); [fi] copy in Camera.obj.
    __forceinline void SetLookAt(const EXVector &v)
    {
        if (m_debugFlags & 2)
            DebugMessage("SetTarget(%f,%f,%f,%f)", ex::fd(v.x), ex::fd(v.y), ex::fd(v.z), ex::fd(v.w));
        m_Target = v;
    }
    inline void SetOrigin(const EXVector2 &v)
    {
        if (m_debugFlags & 0x40) DebugMessage("SetOrigin(%f,%f)", ex::fd(v.x), ex::fd(v.y));
        m_Origin = v;
    }
    // The same body, always expanded: Camera.obj DoUpdate inlines it where MoveLockon calls the copy.
    __forceinline void SetOriginInline(const EXVector2 &v)
    {
        if (m_debugFlags & 0x40) DebugMessage("SetOrigin(%f,%f)", ex::fd(v.x), ex::fd(v.y));
        m_Origin = v;
    }
    __forceinline void SetClipSize(unsigned short w, unsigned short h, signed char b)
    {
        if (m_debugFlags & 0x80) DebugMessage("SetClipSize(%d,%d)", w, h);
        m_ClipW = w;
        m_ClipH = b ? 0x480 : h;
        m_74 = b;
    }
    __forceinline void SetAspectCorrection(signed char b)
    {
        if (m_debugFlags & 0x20) DebugMessage("SetAspectCorrection(%d)", b);
        m_AspectCorrection = b;
    }
    EXVector2 m_Origin;         // +0x68
    u16 m_ClipW;                // +0x70
    u16 m_ClipH;                // +0x72
    s8 m_74;                    // +0x74
    s8 m_AspectCorrection;      // +0x75
};

// Shadow / light projector (EXCamera.obj).
class EXBaseProjector : public EXCommonCamera {
public:
    const EXVector &ProjectAt() const { return m_Target; }     // XBShadow.obj
    EXBaseProjector();
    virtual ~EXBaseProjector();
};

class EXCamera : public EXBaseCamera {
public:
    EXCamera();
    virtual ~EXCamera();
    virtual void UpdateDisplay(unsigned int flags);
private:
    void SetupWBufferProjection();
    void SetupViewport();
public:
    EXMatrix m_Projection;      // +0x78
};

class EXProjector : public EXBaseProjector {
public:
    EXProjector();
    virtual ~EXProjector();
    virtual void UpdateDisplay(unsigned int flags);
    virtual void UpdateDisplayProjection();
};
#endif
