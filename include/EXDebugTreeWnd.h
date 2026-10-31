#ifndef EXDEBUGTREEWND_H
#define EXDEBUGTREEWND_H
#include "types.h"
#include "EXWnd.h"
#include "EXArray.h"
#include "EXDebugTree.h"

class EXMenu;
class EXCounter;
class EXTimer;

// Debug window listing the EXWatcher tree (EXDebugTreeWnd.obj, sizeof 0x2b4). Editable items step
// up or down when their value column is clicked (upper or lower half of the cell); holding the
// button repeats the click with a shrinking delay.
class EXWatchWnd : public EXWnd {
public:
    virtual s8 Update();
    virtual s8 Draw();
    virtual void Close();
    virtual void ButtonDown(unsigned int b);
    virtual void ButtonUp(unsigned int b);
    EXWatchWnd();
    void SaveState();
    void LoadState();
    void DrawItem(EXDebugTreeItem *pItem, unsigned short depth, unsigned char branch, unsigned char width);
    s8 DrawSection(EXDebugTreeSection *pSection, unsigned short depth, unsigned char branch);

    u16 m_StartRow;            // 0x29c first tree row shown
    u16 m_Row;                 // 0x29e tree row being drawn
    short m_ClickCol;          // 0x2a0 text cell clicked this frame (-1 none)
    short m_ClickRow;          // 0x2a2
    s8 m_ClickUpper;           // 0x2a4 click in the upper half of the cell
    short m_HeldCol;           // 0x2a6 last click, replayed while the button is held
    short m_HeldRow;           // 0x2a8
    s8 m_HeldUpper;            // 0x2aa
    short m_RepeatCount;       // 0x2ac frames to the next repeat (-1 released)
    short m_RepeatDelay;       // 0x2ae
    s8 m_ScrollUp;             // 0x2b0
    s8 m_ScrollDown;           // 0x2b1
};

// Frame statistics window (sizeof 0x2d0). Only the menu and the saved state survive in this build.
class EXStatsWnd : public EXWnd {
public:
    virtual s8 Update();
    virtual s8 Draw();
    virtual void Close();
    virtual void ButtonDown(unsigned int b);
    virtual void ButtonUp(unsigned int b);
    virtual s8 MenuSelect(unsigned int id);
    EXStatsWnd();
    void UpdateGroupMenu();
    void SaveState();
    void LoadState();
protected:
    void FormatValue(float v, char *pBuf, int width);
public:
    EXMenu *m_pGroupMenu;      // 0x29c
    u32 m_GroupNum;            // 0x2a0
    u32 m_Unk2a4;              // 0x2a4
    int m_Unk2a8;              // 0x2a8 -1
    int m_Unk2ac;              // 0x2ac -1
    int m_Unk2b0;              // 0x2b0 -1
    u32 m_Unk2b4;              // 0x2b4
    u32 m_ViewType;            // 0x2b8
    u32 m_TimerViewType;       // 0x2bc
    u16 m_StartRow;            // 0x2c0
    short m_ClickCol;          // 0x2c2
    short m_ClickRow;          // 0x2c4
    short m_HeldCol;           // 0x2c6
    short m_HeldRow;           // 0x2c8
    s8 m_ScrollUp;             // 0x2ca
    s8 m_ScrollDown;           // 0x2cb
    u8 m_Precision;            // 0x2cc decimal places
    s8 m_Clicked;              // 0x2cd
};

// Debug window showing an EXInfoTree it owns (sizeof 0x2ac).
class EXInfoTreeWnd : public EXWnd {
public:
    virtual ~EXInfoTreeWnd();
    virtual s8 Update();
    virtual s8 Draw();
    virtual void Close();
    virtual void ButtonDown(unsigned int b);
    virtual void ButtonUp(unsigned int b);
    EXInfoTreeWnd();
    void SaveState();
    void LoadState();
    void DrawItem(EXDebugTreeItem *pItem, unsigned short depth, unsigned char branch, unsigned char width);
    s8 DrawSection(EXDebugTreeSection *pSection, unsigned short depth, unsigned char branch);

    u16 m_StartRow;            // 0x29c
    u16 m_Row;                 // 0x29e
    short m_ClickCol;          // 0x2a0
    short m_ClickRow;          // 0x2a2
    s8 m_ScrollUp;             // 0x2a4
    s8 m_ScrollDown;           // 0x2a5
    EXInfoTree *m_pTree;       // 0x2a8
};

// One bar segment of EXStatsBarWnd (sizeof 0x14).
struct EXStatsSubBar {
    EXTimer *m_pTimer;         // 0x00
    EXCounter *m_pCounter;     // 0x04
    ex::RGBA m_Color;          // 0x08
    u8 m_Bar : 1;              // 0x0c
    u32 m_Unk10;               // 0x10
};

// Bar of timers and counters (sizeof 0x2c8). Draw is a stub in this build.
class EXStatsBarWnd : public EXWnd {
public:
    virtual s8 Draw();
    EXStatsBarWnd();
    unsigned int AddCounter(EXCounter *pCounter, ex::RGBA col);
    unsigned int AddTimer(EXTimer *pTimer, ex::RGBA col);
    unsigned int AddBarCounter(EXCounter *pCounter, ex::RGBA col);
    unsigned int AddBarTimer(EXTimer *pTimer, ex::RGBA col);
    void DelStat(unsigned int i);

    EXDataArray<EXStatsSubBar, EXStatsSubBar &> m_Bars;  // 0x29c
    u32 m_Unk2ac;              // 0x2ac 100
    u32 m_Unk2b0;              // 0x2b0 7
    ex::RGBA m_Unk2b4;         // 0x2b4
    u8 m_Unk2b8;               // 0x2b8
    u8 m_Unk2b9;               // 0x2b9
    u8 m_Unk2ba;               // 0x2ba
    u8 m_Unk2bb;               // 0x2bb
    u32 m_Unk2bc;              // 0x2bc
    u32 m_Unk2c0;              // 0x2c0
    u8 m_Unk2c4;               // 0x2c4
};

// Bar showing each system heap's size and usage (sizeof 0x2ac).
class EXMemBarWnd : public EXWnd {
public:
    virtual s8 Draw();
    EXMemBarWnd();

    int m_BarHeight;           // 0x29c
    ex::RGBA m_Color;          // 0x2a0
    s8 m_ShowStats;            // 0x2a4
    u32 m_Total;               // 0x2a8 sum of the heap sizes
};
#endif
