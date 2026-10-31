#include "EXResourceWnd.h"
#include "EXMenu.h"
#include "EXBaseDisplay.h"
#include "EXStateSaver.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"
#include "EXMemHeap.h"
#include "EXMemBlock.h"
#include "EXMouse.h"
#include "EXHistogram.h"
#include "EXItemEnv.h"
#include "EXFileSys.h"
#include "EXMapOn.h"
#include "EXFile.h"
#include "EXGeoFile.h"
#include "EXGeoHeader.h"
#include "EXHashCodeFile.h"
#include <stdio.h>

namespace ex { double fd(float f); }

s8 EXBaseResourceWnd::InitialUpdate()
{
    EXStateSaver saver("ResourceWnd");
    m_FSDisplay = 0;
    m_FSDisplayType = 0;
    m_SoundDisplay = 0;
    m_View = saver.Restore("XBViewType", 1);
    m_MemSize = saver.Restore("XBMEMSize", 0x40);
    m_MemDrawScale = saver.Restore("XBMEMScale", 1);
    m_MemHistType = saver.Restore("XBMEMHistType", 0);
    StartView();
    return 1;
}

void EXBaseResourceWnd::SaveState()
{
    EXStateSaver saver("ResourceWnd");
    saver.Save("XBViewType", m_View);
    saver.Save("XBMEMSize", m_MemSize);
    saver.Save("XBMEMScale", m_MemDrawScale);
    saver.Save("XBMEMHistType", m_MemHistType);
}

void EXBaseResourceWnd::ClearMenus()
{
    while (m_pMenu->DelItemByIndex(1)) {
    }
    m_pTree->Empty();
    m_Strings.SetSize(0, -1);
}

void EXBaseResourceWnd::ButtonUp(unsigned int b)
{
    EXInfoTreeWnd::ButtonUp(b);
}

void EXBaseResourceWnd::KeyDown(unsigned char k)
{
}

void EXBaseResourceWnd::KeyUp(unsigned char k)
{
}

void EXBaseResourceWnd::Char(char c)
{
}

void EXBaseResourceWnd::StartMemory()
{
    EXMenu *pMenu = m_pMenu->AddSubMenu("Heap", 0);
    if (_SystemHeapList[0]) pMenu->AddItem("Heap 0", 0x30000010);
    if (_SystemHeapList[1]) pMenu->AddItem("Heap 1", 0x30000011);
    if (_SystemHeapList[2]) pMenu->AddItem("Heap 2", 0x30000012);
    if (_SystemHeapList[3]) pMenu->AddItem("Heap 3", 0x30000013);
    pMenu = m_pMenu->AddSubMenu("Size", 0);
    pMenu->AddItem("Size 1", 0x30000000);
    pMenu->AddItem("Size 4", 0x30000001);
    pMenu->AddItem("Size 16", 0x30000002);
    pMenu->AddItem("Size 64", 0x30000003);
    pMenu = m_pMenu->AddSubMenu("DrawScale", 0);
    pMenu->AddItem(" x4", 0x30000008);
    pMenu->AddItem(" x8", 0x30000009);
    pMenu->AddItem(" x12", 0x3000000a);
    pMenu->AddItem(" x16", 0x3000000a);
    m_MemHeap = 0;
    m_MemScroll = 0.0f;
}

void EXBaseResourceWnd::EndMemory() const
{
}

void EXBaseResourceWnd::MenuMemory(unsigned int id)
{
    switch (id) {
    case 0x30000000:
        m_MemScroll = m_MemScroll * (m_MemSize / 1.0f);
        m_MemSize = 1;
        break;
    case 0x30000001:
        m_MemScroll = m_MemScroll * (m_MemSize / 4.0f);
        m_MemSize = 4;
        break;
    case 0x30000002:
        m_MemScroll = m_MemScroll * (m_MemSize / 16.0f);
        m_MemSize = 0x10;
        break;
    case 0x30000003:
        m_MemScroll = m_MemScroll * (m_MemSize / 64.0f);
        m_MemSize = 0x40;
        break;
    case 0x30000008:
        m_MemDrawScale = 1;
        break;
    case 0x30000009:
        m_MemDrawScale = 2;
        break;
    case 0x3000000a:
        m_MemDrawScale = 3;
        break;
    case 0x3000000b:
        m_MemDrawScale = 4;
        break;
    case 0x30000010:
    case 0x30000011:
    case 0x30000012:
    case 0x30000013:
        m_MemHeap = id - 0x30000010;
        m_MemSize = 0x40;
        m_MemScroll = 0.0f;
        break;
    }
}

void EXBaseResourceWnd::UpdateMemory()
{
}

void EXBaseResourceWnd::DrawMemoryBlock(ex::RGBA col, int x, int y, int w, int h, int a, int b)
{
}

void EXBaseResourceWnd::MouseMoveMemory(int x, int y)
{
    m_MemScroll -= y;
    m_MemScroll = limit(m_MemScroll, 0.0f, 65536.0f);
}

void EXBaseResourceWnd::StartMemoryHistograms() const
{
    EXMenu *pMenu = m_pMenu->AddSubMenu("View", 0);
    pMenu->AddItem("Alloc Size", 0x30000028);
    pMenu->AddItem("Free Size", 0x30000029);
    pMenu->AddItem("Alloc Count", 0x3000002a);
    pMenu->AddItem("Free Count", 0x3000002b);
    pMenu->AddItem("Alloc Scan", 0x3000002c);
    pMenu->AddItem("Free Scan", 0x3000002d);
    m_pMenu->AddItem("Reset", 0x30000020);
}

void EXBaseResourceWnd::EndMemoryHistograms() const
{
}

void EXBaseResourceWnd::MenuMemoryHistograms(unsigned int id)
{
}

void EXBaseResourceWnd::UpdateMemoryHistograms() const
{
}

void EXBaseResourceWnd::DrawMemoryHistograms() const
{
}

void EXBaseResourceWnd::EndFiles() const
{
}

void EXBaseResourceWnd::MenuFiles(unsigned int id) const
{
}

void EXBaseResourceWnd::UpdateFiles() const
{
}

void EXBaseResourceWnd::DrawFiles()
{
    EXInfoTreeWnd::Draw();
}

void EXBaseResourceWnd::StartSounds() const
{
    EXMenu *pMenu = m_pMenu->AddSubMenu("Display", 0);
    pMenu->AddItem("Activity", 0x30000040);
    pMenu->AddItem("Memory", 0x30000041);
}

void EXBaseResourceWnd::EndSounds() const
{
}

void EXBaseResourceWnd::MenuSounds(unsigned int id)
{
    switch (id) {
    case 0x30000040:
        m_SoundDisplay = 0;
        break;
    case 0x30000041:
        m_SoundDisplay = 1;
        break;
    }
}

void EXBaseResourceWnd::UpdateSounds() const
{
}

void EXBaseResourceWnd::DrawSounds() const
{
}

void EXBaseResourceWnd::StartCpuHistograms() const
{
    m_pMenu->AddItem("Reset", 0x30000020);
}

void EXBaseResourceWnd::EndCpuHistograms() const
{
}

void EXBaseResourceWnd::MenuCpuHistograms(unsigned int id) const
{
    if (id == 0x30000020) {
        g_CpuHistogram.Reset();
        g_GpuHistogram.Reset();
    }
}

void EXBaseResourceWnd::UpdateCpuHistograms() const
{
}

void EXBaseResourceWnd::DrawCpuHistograms() const
{
    EXBaseDisplay::ActiveWnd()->DbgPrint("           CPU (%7d) / GPU (%7d)\n", g_CpuHistogram.NumCounts(),
                                         g_GpuHistogram.NumCounts());
    for (int i = 0; i < g_CpuHistogram.NumBins(); i++) {
        int lo, hi, cpuCount, cpuPercent, gpuCount, gpuPercent;
        g_CpuHistogram.BinData(i, lo, hi, cpuCount, cpuPercent);
        g_GpuHistogram.BinData(i, lo, hi, gpuCount, gpuPercent);
        if (i < g_CpuHistogram.NumBins() - 1)
            EXBaseDisplay::ActiveWnd()->DbgPrint("%4d <= N <  %4d : %3d%% / %3d%%\n", lo, hi, cpuPercent, gpuPercent);
        else
            EXBaseDisplay::ActiveWnd()->DbgPrint("%4d <= N <= %4d : %3d%% / %3d%%\n", lo, hi, cpuPercent, gpuPercent);
    }
    EXBaseDisplay::ActiveWnd()->DbgPrint("Averages : CPU=%3.1f GPU=%3.1f\n", ex::fd(g_CpuHistogram.Average()),
                                         ex::fd(g_GpuHistogram.Average()));
}

void EXBaseResourceWnd::StartFileSystem() const
{
    EXMenu *pMenu = m_pMenu->AddSubMenu("Display", 0);
    pMenu->AddItem("Activity", 0x30000030);
    pMenu->AddItem("Open Files", 0x30000031);
    pMenu = m_pMenu->AddSubMenu("Display Type", 0);
    pMenu->AddItem("Bar", 0x30000032);
    pMenu->AddItem("Line", 0x30000033);
}

void EXBaseResourceWnd::EndFileSystem() const
{
}

void EXBaseResourceWnd::MenuFileSystem(unsigned int id)
{
    switch (id) {
    case 0x30000030:
        m_FSDisplay = 0;
        break;
    case 0x30000031:
        m_FSDisplay = 1;
        break;
    case 0x30000032:
        m_FSDisplayType = 0;
        break;
    case 0x30000033:
        m_FSDisplayType = 1;
        break;
    }
}

void EXBaseResourceWnd::UpdateFileSystem() const
{
}

void EXBaseResourceWnd::DrawFileSystem()
{
}

void EXBaseResourceWnd::EndWindows() const
{
}

void EXBaseResourceWnd::MenuWindows(unsigned int id) const
{
}

void EXBaseResourceWnd::StartItemEnvInfo() const
{
}

void EXBaseResourceWnd::EndItemEnvInfo() const
{
}

void EXBaseResourceWnd::MenuItemEnvInfo(unsigned int id) const
{
}

void EXBaseResourceWnd::UpdateItemEnvInfo() const
{
}

void EXBaseResourceWnd::DrawWindows()
{
    EXInfoTreeWnd::Draw();
}

s8 EXBaseResourceWnd::MenuSelect(unsigned int id)
{
    if (id <= 0x2fffffff) {
        if (id != m_View) {
            ClearMenus();
            EndView();
            m_View = id;
            StartView();
        }
        SaveState();
    } else if (id >= 0x30000000 && id < 0xffffffff) {
        MenuView(id);
        SaveState();
    }
    return 1;
}

void EXBaseResourceWnd::EndView()
{
    switch (m_View) {
    case VIEW_MEMORY:
        EndMemory();
        break;
    case VIEW_MEMORYHISTOGRAMS:
        EndMemoryHistograms();
        break;
    case VIEW_FILES:
        EndFiles();
        break;
    case VIEW_SOUNDS:
        EndSounds();
        break;
    case VIEW_CPUHISTOGRAMS:
        EndCpuHistograms();
        break;
    case VIEW_FILESYSTEM:
        EndFileSystem();
        break;
    case VIEW_WINDOWS:
        EndWindows();
        break;
    case VIEW_ITEMENVINFO:
        EndItemEnvInfo();
        break;
    }
}

void EXBaseResourceWnd::MenuView(unsigned int id)
{
    switch (m_View) {
    case VIEW_MEMORY:
        MenuMemory(id);
        break;
    case VIEW_MEMORYHISTOGRAMS:
        MenuMemoryHistograms(id);
        break;
    case VIEW_FILES:
        MenuFiles(id);
        break;
    case VIEW_SOUNDS:
        MenuSounds(id);
        break;
    case VIEW_CPUHISTOGRAMS:
        MenuCpuHistograms(id);
        break;
    case VIEW_FILESYSTEM:
        MenuFileSystem(id);
        break;
    case VIEW_WINDOWS:
        MenuWindows(id);
        break;
    case VIEW_ITEMENVINFO:
        MenuItemEnvInfo(id);
        break;
    }
}

void EXBaseResourceWnd::ButtonDown(unsigned int b)
{
    if (b == 0x20) {
        m_MemScroll -= 16.0f;
        m_MemScroll = limit(m_MemScroll, 0.0f, 65536.0f);
    } else if (b == 0x40) {
        m_MemScroll -= -16.0f;
        m_MemScroll = limit(m_MemScroll, 0.0f, 65536.0f);
    }
    EXInfoTreeWnd::ButtonDown(b);
}

void EXBaseResourceWnd::MouseMove(int x, int y)
{
}

void EXBaseResourceWnd::DrawMemory()
{
    static ex::RGBA s_Colours[10] = {
        ex::RGBA(0x40, 0, 0, 0xff),    ex::RGBA(0x70, 0, 0, 0xff),    ex::RGBA(0x80, 0, 0x80, 0xff),
        ex::RGBA(0xf0, 0, 0xf0, 0xff), ex::RGBA(0, 0x80, 0, 0xff),    ex::RGBA(0, 0xf0, 0, 0xff),
        ex::RGBA(0, 0x80, 0x80, 0xff), ex::RGBA(0, 0xf0, 0xf0, 0xff), ex::RGBA(0, 0x40, 0x40, 0xff),
        ex::RGBA(0, 0x70, 0x70, 0xff),
    };
    EXRect rect = m_Rect;
    int cell = m_MemDrawScale * 4;
    int width = max(8, rect.w - 0x8c);
    int top = 0x18;
    int height = max(0x10, rect.h - 0x70);
    if (m_pMenu && m_pMenu->IsOpen()) {
        height -= 12;
        top = 0x24;
    }
    EXMemHeap *pHeap = _SystemHeapList[m_MemHeap];
    int bytesPerCell = m_MemSize * 32;
    int startRow = (int)m_MemScroll;
    EXMemBlock *pEnd = pHeap->MemEnd();
    int rem = 0;
    int row = 0;
    int col = 0;
    EXMemBlock *pSel = 0;
    EXPoint mouse = EXMouse::GetMouse()->Pos();
    int mx = mouse.x - rect.x;
    int my = mouse.y - rect.y;
    int y = top;
    for (EXMemBlock *pBlock = pHeap->MemStart(); pBlock < pEnd; pBlock = pBlock->NextLink()) {
        int perRow = width / cell;
        int bytes = rem + pBlock->Size() + 0x10;
        int n = bytes / bytesPerCell;
        rem = bytes - n * bytesPerCell;
        ex::RGBA c = s_Colours[(pBlock->IsFree() ? 0 : pBlock->IsMoveable() ? 4 : 2) + (pBlock == pSel)];
        int hc = col;
        int hr = row;
        int hn = n;
        int hy = y;
        while (hc + hn >= perRow) {
            if (hr >= startRow && hr < startRow + height / cell) {
                if (mx >= hc * cell + 0x84 && mx < width + 0x84 && my >= hy && my < hy + cell) {
                    pSel = pBlock;
                    break;
                }
                hy += cell;
            }
            hn += hc - perRow;
            hc = 0;
            hr++;
        }
        if (hn > 0 && hr >= startRow && hr < startRow + height / cell && mx >= hc * cell + 0x84 &&
            mx < (hc + hn) * cell + 0x84 && my >= hy && my < hy + cell)
            pSel = pBlock;
        while (col + n >= perRow) {
            if (row >= startRow && row < startRow + height / cell) {
                DrawMemoryBlock(c, col, y, perRow - col, cell, rect.x, rect.y);
                y += cell;
            }
            n += col - perRow;
            col = 0;
            row++;
        }
        if (n > 0) {
            if (row >= startRow && row < startRow + height / cell) DrawMemoryBlock(c, col, y, n, cell, rect.x, rect.y);
            col += n;
        }
    }
    DbgPrint(" Heap %d :: %s\n", m_MemHeap, pHeap->Name());
    DbgPrint("\n");
    DbgPrint(" Heap Size\n %d\n", pHeap->RtlSizeHeap());
    DbgPrint("\n");
    DbgPrint(" Free  \n %d\n[%d]\n\n", pHeap->Debug_Stats(0), pHeap->FindLargestFree());
    DbgPrint(" Alloc \n %d\n\n", pHeap->Debug_Stats(1));
    DbgPrint(" DAlloc\n %d\n\n", pHeap->Debug_Stats(2));
    DbgPrint(" Cached\n %d\n\n", pHeap->Debug_Stats(3));
    SetDbgPos(0, max(0x12, rect.h / 12 - 0x10));
    if (pSel) {
        u32 *p = (u32 *)(pSel + 1);
        DbgPrint(" Addr \n %8.8x\n", p);
        DbgPrint(" Size \n %8.8x\n", pSel->Size());
        DbgPrint(" Alignment\n %s\n", pSel->AlignText());
        DbgPrint(" Flags\n %8.8x\n", pSel->Flags());
        DbgPrint("\n\n");
        if (pSel->IsFree()) {
            DbgPrint(" Free memory\n");
            return;
        }
        int i;
        for (i = 0; i < 8; i++) {
            DbgPrint(" %8.8x", p[i]);
            if ((u8)(i & 3) == 3) DbgPrint("\n");
        }
        DbgPrint(" ");
        for (i = 0; i < 32; i++) {
            u8 ch = ((u8 *)p)[i] & 0x7f;
            if (ch >= 0x20) DbgPrint("%c", ch);
            else DbgPrint(".");
        }
    }
}

void EXBaseResourceWnd::UpdateWindowInfo(EXWnd *pWnd, EXInfoTree *pTree) const
{
    char buf[0x200];
    sprintf(buf, "Wnd 0x%8.8x", pWnd);
    unsigned int section = pTree->AddSection(buf, 0, EXDebugTree::ADD_DUPLICATE_UPDATE);
    sprintf(buf, "%s", pWnd->Debug_Name());
    pTree->AddInfoItem("m_Name", section, buf, 1, EXDebugTree::ADD_DUPLICATE_UPDATE);
    sprintf(buf, "0x%8.8x", pWnd->m_pParent);
    pTree->AddInfoItem("m_Parent", section, buf, 1, EXDebugTree::ADD_DUPLICATE_UPDATE);
    sprintf(buf, "0x%8.8x", (unsigned int)pWnd->m_Flags);
    pTree->AddInfoItem("m_Flags", section, buf, 1, EXDebugTree::ADD_DUPLICATE_UPDATE);
    sprintf(buf, "0x%8.8x", (unsigned int)pWnd->m_Effects);
    pTree->AddInfoItem("m_Effects", section, buf, 1, EXDebugTree::ADD_DUPLICATE_UPDATE);
    sprintf(buf, "0x%8.8x", pWnd->m_pMenu);
    pTree->AddInfoItem("m_pMenu", section, buf, 1, EXDebugTree::ADD_DUPLICATE_UPDATE);
    sprintf(buf, "% 2.2f,% 2.2f", ex::fd(pWnd->m_Origin.x), ex::fd(pWnd->m_Origin.y));
    pTree->AddInfoItem("m_Origin", section, buf, 1, EXDebugTree::ADD_DUPLICATE_UPDATE);
    sprintf(buf, "%d,%d,%d,%d", pWnd->m_Rect.x, pWnd->m_Rect.y, pWnd->m_Rect.w, pWnd->m_Rect.h);
    pTree->AddInfoItem("m_Rect", section, buf, 1, EXDebugTree::ADD_DUPLICATE_UPDATE);
    sprintf(buf, "%d,%d,%d,%d", pWnd->Sprite2DClipRect().x, pWnd->Sprite2DClipRect().y, pWnd->Sprite2DClipRect().w,
            pWnd->Sprite2DClipRect().h);
    pTree->AddInfoItem("m_SprClipRect", section, buf, 1, EXDebugTree::ADD_DUPLICATE_UPDATE);
}

EXBaseResourceWnd::~EXBaseResourceWnd()
{
    if (m_pMenu) {
        delete m_pMenu;
        m_pMenu = 0;
    }
    SaveState();
}

// Map file name without its path.
static inline const char *ResourceFileName(const char *pPath)
{
    int i = strlen(pPath);
    while (i >= 0) {
        if (pPath[i] == '/' || pPath[i] == '\\') {
            i++;
            break;
        }
        i--;
    }
    return pPath + i;
}

// Partial layouts read by the item env page.
struct ResMapFile {
    u8 m_Unk00[0x10];
    const char *m_pName;      // 0x10
};
struct ResMapItem {
    u8 m_Unk00[0x1c];
    ResMapFile *m_pFile;      // 0x1c
    u32 m_Unk20;              // 0x20
};
struct ResViewItem {
    u8 m_Unk00[0x68];
    EXMapOn m_MapOn;          // 0x68
};
struct ResItemEnvView {
    u8 m_Unk00[0x28];
    int m_NumItems;           // 0x28
    ResViewItem **m_pItems;   // 0x2c
};

void EXBaseResourceWnd::DrawItemEnvInfo()
{
    EXItemEnv *pEnv = EXItemEnv::GetItemEnv();
    if (!pEnv) return;
    DbgPrint("Num Loaded Maps %d\n", pEnv->ActiveMapArray().Size());
    unsigned int i;
    for (i = 0; i < (unsigned int)pEnv->MapRefArray().Size(); i++) {
        const EXItemMapRef *pRef = &pEnv->MapRefArray()[i];
        if (pRef && pRef->ItemMap()) {
            ResMapItem *pMap = (ResMapItem *)pRef->ItemMap();
            DbgPrint(" Map %d, %s 0x%8.8x\n", pRef->MapId(), ResourceFileName(pMap->m_pFile->m_pName), pMap->m_Unk20);
        }
    }
    DbgPrint("\nNum Active Views %d\n", pEnv->ViewArraySize());
    for (i = 0; i < pEnv->ViewArraySize(); i++) {
        ResItemEnvView *pView = (ResItemEnvView *)pEnv->ViewArray()[i];
        DbgPrint("[ItemEnvView %d]\n", i);
        for (int j = 0; j < pView->m_NumItems; j++) {
            EXMapOn mapOn = pView->m_pItems[j]->m_MapOn;
            s8 map = mapOn.Map();
            if (ResMapItem *pMap = (ResMapItem *)pEnv->GetMapItem(map))
                DbgPrint(" MapOn %3d %3d '%.12s'\n", map, mapOn.Zone(), ResourceFileName(pMap->m_pFile->m_pName));
        }
    }
}

s8 EXBaseResourceWnd::Draw()
{
    switch (m_View) {
    case VIEW_MEMORY:
        DrawMemory();
        break;
    case VIEW_MEMORYHISTOGRAMS:
        DrawMemoryHistograms();
        break;
    case VIEW_FILES:
        DrawFiles();
        break;
    case VIEW_SOUNDS:
        DrawSounds();
        break;
    case VIEW_CPUHISTOGRAMS:
        DrawCpuHistograms();
        break;
    case VIEW_FILESYSTEM:
        DrawFileSystem();
        break;
    case VIEW_WINDOWS:
        DrawWindows();
        break;
    case VIEW_ITEMENVINFO:
        DrawItemEnvInfo();
        break;
    }
    return 1;
}

void EXBaseResourceWnd::BuildWindowList(EXDListItem *pItem, s8 bClear)
{
    if (bClear) m_Windows.SetSize(0, 0x20);
    for (; pItem; pItem = pItem->Next()) {
        EXWnd *pWnd = (EXWnd *)pItem;
        int i;
        for (i = 0; i < m_Windows.Size(); i++)
            if (m_Windows[i] == pWnd) break;
        if (i == m_Windows.Size()) {
            m_Windows.Add(pWnd);
            if (EXDListItem *pChild = pWnd->FirstChild()) BuildWindowList(pChild, 0);
        }
    }
}

void EXBaseResourceWnd::UpdateWindows()
{
    BuildWindowList(EXBaseDisplay::GetDisplay()->BottomWnd(), 1);
    EXInfoTree *pTree = m_pTree;
    int i;
    for (i = 0; i < m_Windows.Size(); i++) UpdateWindowInfo(m_Windows[i], pTree);
    for (i = 0; i < pTree->NumSections(); i++) {
        EXDebugTreeSection *pSection = pTree->Section(i);
        if (memcmp(pSection->m_Name, "Wnd 0x", 6) == 0) {
            for (int j = 0; j < m_Windows.Size(); j++) {
                char buf[0x200];
                sprintf(buf, "Wnd 0x%8.8x", m_Windows[j]);
                if (strcmp(pSection->m_Name, buf) == 0) goto next;
            }
            pTree->DelSection(i);
            i = 0;
        }
    next:;
    }
}

s8 EXBaseResourceWnd::Update()
{
    switch (m_View) {
    case VIEW_MEMORY:
        UpdateMemory();
        break;
    case VIEW_MEMORYHISTOGRAMS:
        UpdateMemoryHistograms();
        break;
    case VIEW_FILES:
        UpdateFiles();
        break;
    case VIEW_SOUNDS:
        UpdateSounds();
        break;
    case VIEW_CPUHISTOGRAMS:
        UpdateCpuHistograms();
        break;
    case VIEW_FILESYSTEM:
        UpdateFileSystem();
        break;
    case VIEW_WINDOWS:
        UpdateWindows();
        break;
    case VIEW_ITEMENVINFO:
        UpdateItemEnvInfo();
        break;
    }
    return 1;
}

void EXBaseResourceWnd::StartWindows()
{
    BuildWindowList(EXBaseDisplay::GetDisplay()->BottomWnd(), 1);
    EXInfoTree *pTree = m_pTree;
    for (int i = 0; i < m_Windows.Size(); i++) UpdateWindowInfo(m_Windows[i], pTree);
}

EXBaseResourceWnd::EXBaseResourceWnd() : m_Strings(0, -1)
{
    int w = EXBaseDisplay::GetDisplay()->Params().m_Width;
    int h = EXBaseDisplay::GetDisplay()->Params().m_Height;
    m_Flags = 0x80004d9a;
    SetRect(EXRect(10, 20, max(0x200, w) - 20, max(0x180, h) - 48));
    m_BkColor = ex::RGBA(0x20, 0x20, 0x20, 0x7f);
    m_pMenu = new EXMenu;
    m_pMenu->ShowCloseButton(1);
    EXMenu *pMenu = m_pMenu->AddSubMenu("View", 0);
    pMenu->AddItem("Files", VIEW_FILES);
    pMenu->AddItem("FileSystem", VIEW_FILESYSTEM);
    pMenu->AddItem("ItemEnvInfo", VIEW_ITEMENVINFO);
    pMenu->AddItem("Sound", VIEW_SOUNDS);
    pMenu->AddItem("CpuHistograms", VIEW_CPUHISTOGRAMS);
    pMenu->AddItem("Windows", VIEW_WINDOWS);
}

// File owning a file-system list link.
static inline EXFile *ResListFile(EXDListItem *p)
{
    return p ? (EXFile *)((char *)p - 4) : 0;
}

void EXBaseResourceWnd::StartFiles()
{
    EXFileSys *pFS = EXFileSys::GetFileSys();
    EXInfoTree *pTree = m_pTree;
    EXString name;
    EXString value;
    int loaded = -1;
    int nSub = 0;
    EXPtrArray<const char> names(0, 0x100);
    EXFile *pFile;
    for (pFile = ResListFile(pFS->FileLoadedList().Head()); pFile; pFile = ResListFile(pFile->m_Link.Next())) {
        if (loaded < 0) loaded = pTree->AddSection("Loaded", 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
        int i;
        for (i = 0; i < names.Size(); i++)
            if (names[i] == pFile->m_pFilename) break;
        if (i != names.Size()) continue;
        names.Add(pFile->m_pFilename);
        EXStringArray parts(0, -1);
        EXString path(pFile->m_pFilename);
        int n = path.Tokenise(parts, "\\", 0);
        EXString last = parts[n - 1];
        unsigned int section = pTree->AddSection(last, loaded, EXDebugTree::ADD_DUPLICATE_IGNORE);
        pTree->AddInfoItem(last, section, pFile->m_pFilename, 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
        if (pFile->IsKindOf(EXRUNTIME_CLASS(EXGeoFile))) {
            EXGeoHeader *pHdr = ((EXGeoFile *)pFile)->m_pGeoHeader;
            for (int k = 0; k < pHdr->SectionList().Size(); k++) {
                EXGeoSectionHeader *pSec = pHdr->SectionHeader(k);
                if (pSec && pSec->m_pFile && pSec->m_pFile->m_Status == 3) {
                    u32 hash = pSec->HashCode();
                    if ((hash & 0x7f000000) == 0x23000000) {
                        value.Format(" SubZone 0x%8.8x", hash);
                        m_Strings.Add(value);
                        name.Format("sf%d", nSub++);
                        pTree->AddInfoItem(name, section, value, 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
                    } else if ((hash & 0xffff0000) == 0x8020000) {
                        value.Format(" Group %s", EXHashCodeFile::Debug_HashCodeName(hash) + 3);
                        m_Strings.Add(value);
                        name.Format("sf%d", nSub++);
                        pTree->AddInfoItem(name, section, value, 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
                    } else {
                        value.Format(" Section %s", EXHashCodeFile::Debug_HashCodeName(hash) + 3);
                        m_Strings.Add(value);
                        name.Format("sf%d", nSub++);
                        pTree->AddInfoItem(name, section, value, 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
                    }
                }
            }
        }
    }
    int queued = -1;
    int nQueued = 0;
    names.RemoveAll();
    for (pFile = ResListFile(pFS->FileQueuedList().Head()); pFile; pFile = ResListFile(pFile->m_Link.Next())) {
        if (queued < 0) queued = pTree->AddSection("Queued", 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
        int i;
        for (i = 0; i < names.Size(); i++)
            if (names[i] == pFile->m_pFilename) break;
        if (i != names.Size()) continue;
        names.Add(pFile->m_pFilename);
        name.Format("%2d:", nQueued++);
        pTree->AddInfoItem(name, queued, pFile->m_pFilename, 1, EXDebugTree::ADD_DUPLICATE_IGNORE);
    }
    for (unsigned int l = 0; l < pFS->FileListCount(); l++) {
        EXFileList *pList = pFS->FileList(l);
        name.Format("FileList %d", l);
        m_Strings.Add(name);
        unsigned int section = pTree->AddSection(name, 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
        name.Format("FList%d", l);
        value.Format("~C330%s~C333", (const char *)pList->m_DiscNames[0]);
        m_Strings.Add(value);
        pTree->AddInfoItem(name, section, value, 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
        EXFileInfoArray files;
        files.Copy(pList->m_Files);
        files.SortByName();
        for (int j = 0; j < files.Size(); j++) {
            name.Format("~C033File%d~C333", j);
            pTree->AddInfoItem(name, section, files[j].m_pName, 0, EXDebugTree::ADD_DUPLICATE_IGNORE);
        }
    }
}

void EXBaseResourceWnd::StartView()
{
    switch (m_View) {
    case VIEW_MEMORY:
        StartMemory();
        break;
    case VIEW_MEMORYHISTOGRAMS:
        StartMemoryHistograms();
        break;
    case VIEW_FILES:
        StartFiles();
        break;
    case VIEW_SOUNDS:
        StartSounds();
        break;
    case VIEW_CPUHISTOGRAMS:
        StartCpuHistograms();
        break;
    case VIEW_FILESYSTEM:
        StartFileSystem();
        break;
    case VIEW_WINDOWS:
        StartWindows();
        break;
    case VIEW_ITEMENVINFO:
        StartItemEnvInfo();
        break;
    }
}
