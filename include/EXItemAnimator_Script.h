#ifndef EXITEMANIMATOR_SCRIPT_H
#define EXITEMANIMATOR_SCRIPT_H
#include "EXItemAnimator.h"
#include "EXFlag.h"
#include "EXList.h"
#include "EXBit128.h"

class EXGeoScript;
struct GEOSCRIPT_CMD;
class EXDatumTempArray;
struct EXDatumTemp;
struct EXGrabPointTemp;
struct EXGrabPointArrayTemp;
struct EXRayCastTemp;
class EXCollisionScoopTemp;
class EXGeoEntity_InfoArray;
class EXGeoCommonObject;
class EXItemAnimator_Script;
class EXScriptThread;
struct GEOSCRIPT_SOUND;
// Hooks a script animator calls into game code. TODO layout: slots 0 and 2 unknown.
// Script sound command.
struct GEOSCRIPT_SOUND {
    u8 m_00[0x10];
    u32 m_flags;        // +0x10 0x100 owned handle, 0x8000 random pick
    u8 m_count;         // +0x14
    u8 m_reverb;        // +0x15
    u8 m_16;
    u8 m_17;
    float m_volume;     // +0x18
    float m_range;      // +0x1c
    u32 m_hashes[1];    // +0x20
};

// Sound hash handed to the sound callback.
struct EXSoundScriptTemp {
    u32 m_Hash;
};
struct GEOSCRIPT_CAMERA;
class EXItemAnimator_Camera;
struct EXAnimatorScriptCallbacks {
    // Typed constructor ([fi] copy in Actor.obj); the members keep the generic call signatures.
    EXAnimatorScriptCallbacks(unsigned int (__cdecl *pCreate)(EXItemAnimator_Script *, EXScriptThread *, EXGeoScript *, struct GEOSCRIPT_REFCMD *),
                              void (__cdecl *pEnd)(EXItemAnimator_Script *, EXScriptThread *),
                              void (__cdecl *pCamera)(EXItemAnimator_Script *, EXGeoScript *, GEOSCRIPT_CAMERA *, EXItemAnimator_Camera *),
                              void (__cdecl *pSound)(const EXItemAnimator *, const struct GEOSCRIPT_SOUND *, EXSoundScriptTemp &))
    {
        m_pCreateThread = (int (__cdecl *)(EXItemAnimator_Script *, EXScriptThread *, EXGeoScript *, const GEOSCRIPT_CMD *))pCreate;
        m_pThreadEnd = pEnd;
        m_pCamera = (void (__cdecl *)(EXItemAnimator_Script *, EXGeoScript *, GEOSCRIPT_CMD *, EXItemAnimator *))pCamera;
        m_pSound = (void (__cdecl *)(const EXItemAnimator_Script *, const GEOSCRIPT_SOUND *, u32 *))pSound;
    }
    int (__cdecl *m_pCreateThread)(EXItemAnimator_Script *pScript, EXScriptThread *pThread, EXGeoScript *pGeo, const GEOSCRIPT_CMD *pCmd);  // +0 nonzero: handled
    void (__cdecl *m_pThreadEnd)(EXItemAnimator_Script *pScript, EXScriptThread *pThread);   // +4
    void (__cdecl *m_pCamera)(EXItemAnimator_Script *pScript, EXGeoScript *pGeo, GEOSCRIPT_CMD *pCmd, EXItemAnimator *pCamera);  // +8
    void (__cdecl *m_pSound)(const EXItemAnimator_Script *pScript, const GEOSCRIPT_SOUND *pCmd, u32 *pHash);  // +0xc
};
struct EXGeoScriptControllerHeader;
struct EXScriptController;
struct GEOSCRIPT_CMD;
struct GEOSCRIPT_REFCMD;
struct GEOSCRIPT_SOUND;
struct GEOSCRIPT_COLLISION;
struct _GEOSCRIPT_GEOOBJECT;
struct _GEOSCRIPT_ANIM;
struct _GEOSCRIPT_LIGHT;

// A running thread of a geo script (sizeof 0x20).
class EXScriptThread {
public:
    EXItemAnimator *Placement() const { return m_pPlacement; }
    s8 HasParentThread() const { return m_parentThread != 0xff; }
    void SetPlacement(EXItemAnimator *p) { m_pPlacement = p; }
    u8 ParentThread() const { return m_parentThread; }
    u8 Thread() const { return m_thread; }
    s8 IsRootNode() const { return m_rootNode; }
    void SetRootNode() { m_rootNode = 1; }
    u32 ScriptOffset() const { return m_scriptOffset; }
    u32 CtrlIndex() const { return m_ctrlIndex; }
    float LocalFrameTime() const { return m_localFrameTime; }
    s8 AnimatorType() const { return m_animatorType; }
    void SetAnimatorType(s8 t) { m_animatorType = t; }
    void SetValid(s8 b) { m_valid = b; }
    void Initialise(int a, int b, int thread);
    void UpdateThreadControllers(const EXItemAnimator_Script *pScript, const EXGeoScript *pGeo, float t);
    void ResetThread(const EXItemAnimator_Script *pScript, const EXGeoScript *pGeo, const GEOSCRIPT_CMD *pCmd, s8 b);
    void InitialiseThread(EXItemAnimator *p, int parent)
    {
        m_pPlacement = p;
        m_parentThread = (u8)parent;
    }

    float m_localFrameTime;     // +0
    float m_04;
    EXItemAnimator *m_pPlacement;   // +8
    s8 m_animatorType;          // +0xc
    s8 m_valid;                 // +0xd
    u8 m_thread;                // +0xe
    u8 m_parentThread;          // +0xf 0xff: none
    s8 m_rootNode;              // +0x10
    u8 m_11;
    u16 m_ctrlIndex;            // +0x12
    u16 m_14;
    u16 m_16;
    u32 m_scriptOffset;         // +0x18
    u32 m_1c;
};

// Animator playing a geo script: one thread per script thread (sizeof 0x148).
class EXItemAnimator_Script : public EXItemAnimator {
    EX_DECLARE_DYNAMIC(EXItemAnimator_Script)
public:
    virtual ~EXItemAnimator_Script();
    EX_DECLARE_ITEMTYPE(EXItemAnimator_Script)
    virtual void DoUpdate();
    virtual u32 DoRender(const EXMatrix &mtx, const EXRenderInfo &info, u32 flags);
    virtual s8 Initialise(EXGeoFile *pFile, u32 hash);
    virtual float ObjectTime() const;
    virtual void SetObjectTime(float t);
    virtual void UpdateObjectTime(float dt);
    virtual void EnableParticleSystems(s8 b) const;
    virtual void *GetDatum(u32 type, EXDatumTemp &d, u32 flags, void *&pCtx);
    virtual void GetDatum(u32 type, EXDatumTempArray &arr, u32 flags);
    virtual s8 DoGrabPoint(const EXMatrix &mtx, EXGrabPointTemp &t, u32 flags);
    virtual s8 DoGrabPoint(const EXMatrix &mtx, EXGrabPointArrayTemp &t, u32 flags);
    virtual void DoRayCast(const EXMatrix &mtx, EXRayCastTemp &t, u32 flags);
    virtual void DoEntityScoop(const EXMatrix &mtx, EXCollisionScoopTemp &t, u32 flags);
    virtual void SetRenderState(u32 state, s8 bOn);
    virtual void SetLodMin(u32 lod);
    virtual void SetLodMax(u32 lod);
    virtual void ApplyFunction(u32 (__cdecl *pFunc)(EXItemAnimator *, void *), void *pData);
    virtual void BuildEntityInfoArray(const EXMatrix &mtx, EXGeoEntity_InfoArray &arr, s8 b);
    virtual EXGeoCommonObject *GetGeoObject() const;
protected:
    virtual s8 _GetBoundsBox(EXBoundsBox &box, u32 flags);
public:
    EXItemAnimator_Script();
    static EXItemAnimator_Script *CreateObject();
    static EXItemAnimator_Script *NewItem() { return new EXItemAnimator_Script; }
    s8 HasScriptFinished() const;
    EXScriptThread &Thread(u32 i) const { return m_pThreads[i]; }
    u32 NumThreads() const { return m_numThreads; }
    void RegisterCallbacks(const EXAnimatorScriptCallbacks *p) { m_pScriptCallbacks = p; }
    u32 ScriptStatus() const { return m_scriptStatus; }
    u32 ScriptOffset() const { return m_scriptOffset; }
    EXDList &ThreadNodes() { return m_threadNodes; }
    ex::EXFlag<u32> &ValidCmds() { return m_validCmds; }

    EXItemAnimator *CreateThreadObject(const EXGeoScript *pScript, const EXScriptThread *pThread, const _GEOSCRIPT_GEOOBJECT *pCmd) const;
    EXItemAnimator *CreateThreadScript(const EXGeoScript *pScript, const EXScriptThread *pThread, const _GEOSCRIPT_GEOOBJECT *pCmd);
    EXItemAnimator *CreateThreadSound(const EXGeoScript *pScript, const EXScriptThread *pThread, const GEOSCRIPT_SOUND *pCmd) const;
    EXItemAnimator *CreateThreadTransform(const EXGeoScript *pScript, const EXScriptThread *pThread) const;
    EXItemAnimator *CreateThreadCollision(const EXGeoScript *pScript, const EXScriptThread *pThread, const GEOSCRIPT_COLLISION *pCmd) const;
    EXScriptThread *GetThreadFromItem(const EXBaseItem *p) const;
    void ProcessLevelOfDetailControllers(const EXGeoScript *pScript, const EXScriptThread *pThread, const EXGeoScriptControllerHeader *pHdr, const EXScriptController *pCtrl) const;
    void CleanItemThread(EXScriptThread *pThread);
    void AddItemThread(EXItemAnimator *p, const GEOSCRIPT_CMD *pCmd);
    EXItemAnimator *CreateThreadAnim(const EXGeoScript *pScript, const EXScriptThread *pThread, const _GEOSCRIPT_ANIM *pCmd);
    EXItemAnimator *CreateThreadLight(const EXGeoScript *pScript, const EXScriptThread *pThread, const _GEOSCRIPT_LIGHT *pCmd) const;
    void GetGeoBoundsBox(EXBoundsBox &box);
    EXItemAnimator *GetThreadAnimator(u32 i) const;
    float ObjectFrame() const;
    EXItemAnimator *CreateThreadAnimator(const EXGeoScript *pScript, EXScriptThread *pThread, const GEOSCRIPT_REFCMD *pCmd);
    int DoUpdate_ScriptCamera(EXGeoScript *pScript, GEOSCRIPT_CMD *pCmd, EXScriptThread *pThread);
    int DoUpdate_InitialiseThread(EXGeoScript *pScript, const GEOSCRIPT_CMD *pCmd, EXScriptThread *pThread);
    void SetScriptOffset(u32 off);
    void SetScriptStatus(u32 s);
    void SetObjectFrame(float f);
    EXItemAnimator *ProcessThreadAnimator(EXGeoScript *pScript, EXScriptThread *pThread);
protected:
    static u8 ms_ScriptNodeTypeTable[20];
    int DoUpdate_ScriptCmdObject(GEOSCRIPT_CMD *pCmd);
    void UpdateThreads(const EXGeoScript *pScript);
    EXGeoScript *GetGeoScript() const;
    void UpdateThreadNodes();
    void UpdateThreads_EndThread(const EXGeoScript *pScript, const GEOSCRIPT_CMD *pCmd, EXScriptThread *pThread);
    void CleanInitialise();
    int DoUpdate_ScriptRootNode(const EXGeoScript *pScript, const GEOSCRIPT_CMD *pCmd);
    void SetScriptFrame(float f);
    int DoUpdate_ScriptCmdEventCall(GEOSCRIPT_CMD *pCmd, int n);
    int DoUpdate_ScriptCmdTimeJump(GEOSCRIPT_CMD *pCmd, const EXGeoScript *pScript);
    void DoUpdate_ScriptCmds(float t);
    void DoUpdate_ScriptCmdsReverse(float t);
public:
    EXScriptThread *m_pThreads;                         // +0x104
    int *m_pJumpCounts;                                 // +0x108 one per time jump
    u32 m_scriptOffset;                                 // +0x10c
    u32 m_scriptStatus;                                 // +0x110
    ex::EXFlag<u32> m_validCmds;                        // +0x114
    u8 m_numThreads;                                    // +0x118
    s8 m_bRootControllers;                              // +0x119
    u8 m_11a;
    const EXAnimatorScriptCallbacks *m_pScriptCallbacks; // +0x11c
    EXDList m_threadNodes;                              // +0x120
    ex::EXBit128 m_threadBits;                          // +0x128 threads with a placement
    ex::EXBit128 m_activeBits;                          // +0x138 threads with controllers to update
};
#endif
