#ifndef EXRESOURCEWND_H
#define EXRESOURCEWND_H
#include "types.h"
#include "EXDebugTreeWnd.h"
#include "EXString.h"

class EXDListItem;
class EXMemBlock;

// Resource browser debug window (EXResourceWnd.obj). The View menu picks one of eight pages; each
// page has Start/End/Menu/Update/Draw handlers dispatched on m_View. sizeof 0x2f0.
class EXBaseResourceWnd : public EXInfoTreeWnd {
public:
    enum {
        VIEW_MEMORY = 1,
        VIEW_MEMORYHISTOGRAMS = 2,
        VIEW_FILES = 3,
        VIEW_SOUNDS = 4,
        VIEW_CPUHISTOGRAMS = 5,
        VIEW_FILESYSTEM = 6,
        VIEW_WINDOWS = 7,
        VIEW_ITEMENVINFO = 8,
    };
    virtual ~EXBaseResourceWnd();
    virtual s8 InitialUpdate();
    virtual s8 Update();
    virtual s8 Draw();
    virtual void ButtonDown(unsigned int b);
    virtual void ButtonUp(unsigned int b);
    virtual void MouseMove(int x, int y);
    virtual void KeyDown(unsigned char k);
    virtual void KeyUp(unsigned char k);
    virtual void Char(char c);
    virtual s8 MenuSelect(unsigned int id);
    virtual void StartView();
    virtual void EndView();
    virtual void MenuView(unsigned int id);
    EXBaseResourceWnd();
    void SaveState();
protected:
    void ClearMenus();
    void StartMemory();
    void EndMemory() const;
    void MenuMemory(unsigned int id);
    void UpdateMemory();
    void DrawMemory();
    void DrawMemoryBlock(ex::RGBA col, int x, int y, int w, int h, int a, int b);
    void MouseMoveMemory(int x, int y);
    void StartMemoryHistograms() const;
    void EndMemoryHistograms() const;
    void MenuMemoryHistograms(unsigned int id);
    void UpdateMemoryHistograms() const;
    void DrawMemoryHistograms() const;
    void StartFiles();
    void EndFiles() const;
    void MenuFiles(unsigned int id) const;
    void UpdateFiles() const;
    void DrawFiles();
    void StartSounds() const;
    void EndSounds() const;
    void MenuSounds(unsigned int id);
    void UpdateSounds() const;
    void DrawSounds() const;
    void StartCpuHistograms() const;
    void EndCpuHistograms() const;
    void MenuCpuHistograms(unsigned int id) const;
    void UpdateCpuHistograms() const;
    void DrawCpuHistograms() const;
    void StartFileSystem() const;
    void EndFileSystem() const;
    void MenuFileSystem(unsigned int id);
    void UpdateFileSystem() const;
    void DrawFileSystem();
    void StartWindows();
    void EndWindows() const;
    void MenuWindows(unsigned int id) const;
    void UpdateWindows();
    void DrawWindows();
    void BuildWindowList(EXDListItem *pItem, s8 bClear);
    void UpdateWindowInfo(EXWnd *pWnd, EXInfoTree *pTree) const;
    void StartItemEnvInfo() const;
    void EndItemEnvInfo() const;
    void MenuItemEnvInfo(unsigned int id) const;
    void UpdateItemEnvInfo() const;
    void DrawItemEnvInfo();

    int m_View;                    // 0x2ac XBViewType
    float m_MemScroll;             // 0x2b0 first memory row shown
    int m_MemSize;                 // 0x2b4 XBMEMSize, bytes per cell / 32
    int m_MemHeap;                 // 0x2b8
    int m_MemDrawScale;            // 0x2bc XBMEMScale, cell size / 4
    int m_MemHistType;             // 0x2c0 XBMEMHistType
    int m_FSDisplay;               // 0x2c4 0 activity, 1 open files
    int m_FSDisplayType;           // 0x2c8 0 bar, 1 line
    int m_SoundDisplay;            // 0x2cc 0 activity, 1 memory
    EXStringArray m_Strings;       // 0x2d0 names handed to the info tree
    EXPtrArray<EXWnd> m_Windows;   // 0x2e0
};

// Xbox resource window (XBResourceWnd.obj).
class EXResourceWnd : public EXBaseResourceWnd {
public:
    EXResourceWnd();
    virtual ~EXResourceWnd();
};
#endif
