#ifndef EXITEMANIMATOR_CAMERA_H
#define EXITEMANIMATOR_CAMERA_H
#include "EXItemAnimator.h"

struct GEOSCRIPT_CAMERA;

// Camera animator (sizeof 0x124): lens parameters and viewport.
class EXItemAnimator_Camera : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_Camera)
public:
    EX_DECLARE_ITEMTYPE(EXItemAnimator_Camera)
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *pFile, u32 hash);
    virtual void SetObjectTime(float t);
    EXItemAnimator_Camera();
    static EXItemAnimator_Camera *CreateObject();
    float FocalLength() const { return m_focalLength; }
    float ApertureHeight() const { return m_apertureHeight; }
    void SetFocalLength(float f) { m_focalLength = f; }
    void SetAperture(float w, float h) { m_apertureWidth = w; m_apertureHeight = h; }
    void SetZoom(float z) { m_zoom = z; }
    void SetViewport(int x, int y, int w, int h) { m_vpX = x; m_vpY = y; m_vpW = w; m_vpH = h; }
    void SetDefaults();
    void SetFromScriptCmd(const GEOSCRIPT_CAMERA *pCmd);
    void UpdateCamera() const;

    float m_focalLength;    // +0x104
    float m_apertureWidth;  // +0x108
    float m_apertureHeight; // +0x10c
    float m_zoom;           // +0x110
    int m_vpX;              // +0x114
    int m_vpY;              // +0x118
    int m_vpW;              // +0x11c
    int m_vpH;              // +0x120
};
#endif
