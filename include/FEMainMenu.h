#ifndef FEMAINMENU_H
#define FEMAINMENU_H
#include "FELayout.h"
#include "EXArray.h"
#include "EXItemHandler.h"
#include "EXItem.h"
#include "EXMatrix.h"
#include "EXVector.h"
#include "EXPointf.h"
#include "FEManager.h"

namespace ex {
inline void __cdecl TRACE_RAW(...) {}  // compiled-out debug trace (copy in FEMainMenu.obj)
}

// Front-end marker placed by a level trigger (FEMainMenu.obj); sizeof 0x10. TODO layout.
class FEMarker {
public:
    FEMarker() {}
    void Set(u32 a, float x, float y, float z)
    {
        m_Unk0 = a;
        m_Pos.x = x;
        m_Pos.y = y;
        m_Pos.z = z;
    }
    u32 m_Unk0;       // 0x00 TODO (copied from the trigger's +0x0c)
    EXVector3 m_Pos;  // 0x04
};

// Item handler of a trophy-room exhibit (FEMainMenu.obj); sizeof 0x44. TODO layout past 0x40.
class XItemHandler_FETrophy : public EXItemHandler {
    EX_DECLARE_DYNAMIC(XItemHandler_FETrophy)
public:
    virtual ~XItemHandler_FETrophy();
    virtual signed char Initialise();
    virtual signed char DoUpdate();
    // NEAR: retail schedules the vptr store after the first two zero stores; ours stores it last.
    XItemHandler_FETrophy()
    {
        m_Unk8 = 0;
        m_UnkC = 0;
        m_Unk10 = 0;
        m_Unk14 = 0;
        m_Scale = 1.0f;
        m_Unk18 = 0;
        m_Unk20 = 0;
        m_Unk24 = 0;
        m_Unk40 = 0;
        m_Unk3c = 0;
        m_Unk34 = 0;
        m_Unk38 = 0;
        m_Unk1c = 1;
        m_Unk2c = 0.0f;
        m_Unk30 = 0.0f;
    }
    static void ResetDirFudge() { ms_DirFudgedFrom = 0; }
    // Not in the map: the lookup Object(hash) inlined (FEMainMenu::UpdateStateViewTrophies).
    static XItemHandler_FETrophy *Find(unsigned int hash)
    {
        for (unsigned int i = 0; i < ms_nObjects; i++)
            if (ms_Objects[i]->m_Unk8 == hash) return ms_Objects[i];
        return 0;
    }
    signed char IsVisible()
    {
        if (!(m_pItem->m_itemFlags & 1)) return 0;
        return !m_pItem->m_renderInfo.FlagBits().m_alphaMult;
    }
    const EXPoint &GridPos() const { return *(const EXPoint *)&m_Unk10; }
    XItemHandler_FETrophy *DirFudge(unsigned int from, unsigned int dir, EXPoint &pt);
    void Init();
    static XItemHandler_FETrophy *CreateObject();
    static XItemHandler_FETrophy *Object(unsigned int hash);
    static XItemHandler_FETrophy *Object(EXPoint pt, signed char bVisible);

    unsigned int m_Unk8;   // 0x08 exhibit hash
    u32 m_UnkC;            // 0x0c
    int m_Unk10;           // 0x10 grid position (EXPoint)
    int m_Unk14;           // 0x14
    u32 m_Unk18;           // 0x18
    u32 m_Unk1c;           // 0x1c
    u32 m_Unk20;           // 0x20
    u32 m_Unk24;           // 0x24
    float m_Scale;         // 0x28
    float m_Unk2c;         // 0x2c
    float m_Unk30;         // 0x30
    u32 m_Unk34;           // 0x34
    u32 m_Unk38;           // 0x38
    int m_Unk3c;           // 0x3c index into GameValues::m_Unk38 (>= 8: 0)
    int m_Unk40;           // 0x40 count needed to unlock

    static signed char ms_bWeaponsView;
protected:
    static EXFixedArray<XItemHandler_FETrophy *, 58> ms_Objects;
    static unsigned int ms_nObjects;
    static unsigned int ms_DirFudgedDir;
    static unsigned int ms_DirFudgedFrom;
    friend class FEMainMenu;
};

class FEManager;
class FELayoutScrollGrid;
class EXTrigger;
class EXItemAnimator_Camera;
class FEListener;

// Camera position of one menu state, copied from a placed camera animator (FEMainMenu.obj); sizeof 0x48.
class FECamera {
public:
    FECamera() {}
    EXMatrix m_Mtx;     // 0x00
    u32 m_Unk40;        // 0x40 TODO
    u32 m_Unk44;        // 0x44 TODO
};

// Front-end main menu layout (FEMainMenu.obj); sizeof 0x858. Field names partly guessed.
class FEMainMenu : public FELayoutLayered {
public:
    virtual ~FEMainMenu();
    FEMainMenu();

    static FEMainMenu *Instance() { return ms_pInstance; }
    unsigned int State() const { return m_pManager->m_PageState; }
    unsigned int Substate() const { return m_pManager->m_PageSubstate; }
    signed char IsNewSubstate()
    {
        signed char b = m_bNewSubstate;
        m_bNewSubstate = 0;
        return b;
    }
    void Trace_Substate(char *pName, signed char clear)
    {
        ex::TRACE_RAW(pName);
        if (clear) m_bNewSubstate = 0;
    }
    unsigned int ReturnSubstate() const { return m_ReturnSubstate; }
    float SubstateTime() const { return m_pManager->m_PageSubstateTimer; }
    FEObject *LayoutObject(FELayout *p, unsigned int i) { return p->Object(i); }
    FELayout *Layout(unsigned int i)
    {
        FELayout *p = m_pLayout;
        if (!p) return 0;
        return (FELayout *)p->Object(i);
    }
    void SetDoneStateAction(unsigned int state, unsigned int action)
    {
        m_DoneState = state;
        m_DoneAction = action;
    }
    FEObject *UpdateObject() const { return m_pUpdateObject; }
    void SetUpdateObject(FEObject *p) { m_pUpdateObject = p; }
    signed char IgnoreNextPageSelect() const { return m_bIgnoreNextPageSelect; }
    static void SetPredSkinLoaded(signed char b) { ms_bPredSkinLoaded = b; }
    static void SetCameraLookDispPos(EXVector2 v) { ms_CameraLookDispPos = v; }  // [fi] copy in Camera.obj
    void SetPreDlgFocus(unsigned __int64 id) { m_PreDlgFocus = id; }
    signed char fDialogOn() { return m_bDialogOn == 1; }
    unsigned int InfoHelpText() { return m_InfoHelpText; }
    signed char IsPadEvent(FEObjEvent::Flags ev) const
    {
        switch (ev) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 22:
        case 23:
            return 1;
        }
        return 0;
    }
    void SetSubstate(unsigned int s)
    {
        m_bNewSubstate = 1;
        m_ReturnSubstate = m_pManager->m_PageSubstate;
        m_pManager->SetPageSubstate(s);
    }
    void SetSubstate(unsigned int s, unsigned int a, unsigned int b, unsigned int c)
    {
        m_bNewSubstate = 1;
        m_ReturnSubstate = m_pManager->m_PageSubstate;
        m_pManager->SetPageSubstate(s);
        m_SubstateArg[0] = a;
        m_SubstateArg[1] = b;
        m_SubstateArg[2] = c;
    }
    FEListener *UpdateListener() { return m_UpdateListeners[m_ListenerIdx]; }
    void SetUpdateListener(FEListener *p) { m_UpdateListeners[m_ListenerIdx] = p; }

    static void SET_INITIAL_LOADED_FOCUS();
    static void ResetUpgradesFromQuit();
    static void DisplayStats(unsigned int level, signed char quit, unsigned int page, signed char noFlush);
    static signed char IngameIsRitualTimeBeaten();
    static signed char IsDisplayStats();
    void DeleteContent();
    void AddContent();
    void Exit();
    signed char UpdateStateNone(FEObjEvent::Flags ev);
    static void SetInitialChapterSelectFocus();
    static void SetInitialRitualSelectFocus(signed char);
    void SetPredSkinUpdateState(short);
    FEObject *AddScrollGridText(FELayoutScrollGrid *, unsigned int, unsigned int, unsigned __int64, EXString,
                                EXW::TextAlign, EXString, signed char);
    void SetTitleAndBackgr(char *, EXRect);
    signed char UpdateStateNewGame(FEObjEvent::Flags ev);
    signed char UpdateStateLoadGame(FEObjEvent::Flags ev);
    signed char UpdateStateStoryLevelSelect(FEObjEvent::Flags ev);
    signed char UpdateStateAutosave(FEObjEvent::Flags ev);
    signed char UpdateStateSaveGame(FEObjEvent::Flags ev);
    void RefreshPostLoadContent();
    char *GetStatsTimeStr(unsigned int, char *, signed char);
    unsigned int CameraIndexFromState(unsigned int);
    char *Debug_StateName(unsigned int state);
    void SetInfoObjText(char *, unsigned int);
    static void AddFEMarker(EXTrigger *);
    static FEMarker *Marker(unsigned int i);
    signed char UpdateFade(signed char);
    void SetState(unsigned int);
    signed char GoStateUpdate();
    void SetPlayerSkin(short);
    void UpdateCameraTransition();
    void HideSlotsLayoutUpdate();
    void InitRitualTimesLayout(FELayoutScrollGrid *pGrid, unsigned int level);
    void UpdatePredSkin();
    void NewFocus(FEObject *pFocus, unsigned int state);
    void UpdateCameraScreenPos();
    signed char HandleFrontendEvent(FEObjEvent::Flags ev);
    signed char UpdateStateDone(FEObjEvent::Flags ev);
    void InitCameraTransition(EXVector to, float duration);
    void CameraVectors(unsigned int cam, EXVector &pos, EXVector &target);
    void SetCamera(unsigned int cam, signed char finish);
    void AddCamera(unsigned int, EXItemAnimator_Camera *);
    void SetFadeRange(EXPointf &, signed char);
    signed char GoState(unsigned int);
    signed char UpdateStateViewTrophies(FEObjEvent::Flags ev);
    signed char UpdateStateViewRecords(FEObjEvent::Flags ev);
    signed char UpdateStateTraining(FEObjEvent::Flags ev);
    signed char UpdateStateOptions(FEObjEvent::Flags ev);
    signed char UpdateStateOptionsAudio(FEObjEvent::Flags ev);
    signed char UpdateStateOptionsGame(FEObjEvent::Flags ev);
    signed char UpdateStateOptionsCamera(FEObjEvent::Flags ev);
    signed char UpdateStateOptionsController(FEObjEvent::Flags ev);
    signed char UpdateStateMain(FEObjEvent::Flags ev);
    signed char UpdateStateLoaded(FEObjEvent::Flags ev);
    signed char UpdateStateStoryLevelStats(FEObjEvent::Flags ev);
    signed char UpdateStateRitualChallengeSelect(FEObjEvent::Flags ev);
    signed char UpdateStateCostumeSelect(FEObjEvent::Flags ev);
    signed char UpdateStateEditName(FEObjEvent::Flags ev);
    signed char UpdateStateCredits(FEObjEvent::Flags ev);
    signed char UpdateStateRitualChallengeTimes(FEObjEvent::Flags ev);
    void InitStatsLayout(unsigned int level, FELayoutScrollGrid *pGrid, GameStats *pStats, char *pName, signed char b, EXString s);
    signed char UpdateStateHuntingGear(FEObjEvent::Flags ev);
    void Init();
    signed char UpdateState(unsigned int, FEObjEvent::Flags);

    FEManager *m_pManager;              // 0x1c0
    FELayout *m_pLayout;                // 0x1c4
    FEObject *m_pArrowL;                // 0x1c8 focus arrows
    FEObject *m_pArrowR;                // 0x1cc
    FEObject *m_pFadeExtra;             // 0x1d0 faded with the page on the options pages
    FEObject *m_pSlotsBackground;       // 0x1d4 shown with the save/load slots
    FEObject *m_pTitle;                 // 0x1d8
    FEObject *m_pBackground;            // 0x1dc
    FEObject *m_pHelpText;              // 0x1e0 help line for the focused object
    FEObject *m_pCreditsObj;            // 0x1e4 shown while the credits roll
    FEObject *m_pInfoObj;               // 0x1e8
    u32 m_bUpdateOnChange : 1;          // 0x1ec bit 0: SetState sends state-change events
    u32 m_b1 : 1;
    u32 m_bGoing : 1;                   // bit 2: GoState in progress
    u32 m_bFadeOut : 1;                 // bit 3
    u32 m_b4 : 1;
    u32 m_bDialogOn : 1;                // bit 5
    u32 m_b6 : 1;
    u32 m_b7 : 1;
    u32 m_bIgnoreNextPageSelect : 1;    // bit 8
    u32 m_b9 : 1;
    u32 m_b10 : 1;
    unsigned int m_GoState;             // 0x1f0
    signed char m_bNewSubstate;         // 0x1f4
    unsigned int m_ReturnSubstate;      // 0x1f8
    u32 m_Unk1fc;                       // 0x1fc
    unsigned int m_ListenerIdx;         // 0x200 current page (layout child) index
    FELayout *m_pCurLayout;             // 0x204
    unsigned int m_DoneState;           // 0x208
    unsigned int m_DoneAction;          // 0x20c
    FEObject *m_pUpdateObject;          // 0x210
    EXFixedArray<FEListener *, 24> m_UpdateListeners;  // 0x214
    u32 m_Unk274;                       // 0x274
    EXPointf m_FadeOut;                 // 0x278 fade-out time range (x < 0: off)
    EXPointf m_FadeIn;                  // 0x280 fade-in time range
    float m_FadeTime;                   // 0x288
    int m_Unk28c;                       // 0x28c
    int m_Unk290;                       // 0x290
    EXVector m_CamTransFrom;            // 0x294 camera look-at when the transition started
    EXVector m_CamTransTo;              // 0x2a4
    EXVector2 m_CamTransDisp;           // 0x2b4 screen position of the start look-at
    float m_CamTransDuration;           // 0x2bc
    float m_CamTransTime;               // 0x2c0
    EXFixedArray<EXVector2, 24> m_LayoutPos;  // 0x2c4 page screen positions before the camera offset
    EXFixedArray<FECamera, 16> m_Cameras;  // 0x384
    EXVector2 m_CameraDispOffset;       // 0x804 screen offset of the menu camera's look-at from the centre
    signed char m_bAutoSave;            // 0x80c
    unsigned __int64 m_PreDlgFocus;     // 0x810
    int m_Unk818;                       // 0x818
    u32 m_Unk81c;                       // 0x81c
    short m_PlayerSkin;                 // 0x820
    short m_PredSkinUpdateState;        // 0x822
    float m_PredSkinUpdateTime;         // 0x824
    unsigned int m_PredSkinHash;        // 0x828 skin hash before the change
    short m_Unk82c;                     // 0x82c
    unsigned int m_SubstateArg[3];      // 0x830
    FEObject *m_pDimmedLayout;          // 0x83c page dimmed while a dialog is up
    FEObject *m_pSlotsInfo;             // 0x840 save/load slot help text
    signed char m_Unk844;               // 0x844
    unsigned int m_PrevState;           // 0x848
    int m_Unk84c;                       // 0x84c TODO (>= 0 hides the save slots)
    unsigned int m_InfoHelpText;        // 0x850
    u32 m_Unk854;                       // 0x854

protected:
    static unsigned int ms_StatsState;
    static int ms_RitualTimeBeaten;
    static EXFixedArray<char, 16> ms_NameEdit;
    static unsigned int ms_InitialLoadedFocus;
    static FEMainMenu *ms_pInstance;
    static signed char ms_bDisplayStats;
    static signed char ms_bLevelWasQuit;
    static signed char ms_bPredSkinLoaded;
    static signed char ms_bDisableSaveGame;
    static EXVector2 ms_CameraLookDispPos;  // set by the camera's scripted mode in the front end map
    static EXFixedArray<FEMarker, 16> ms_FEMarkers;
};
#endif
