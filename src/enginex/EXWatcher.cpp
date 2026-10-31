#include "EXWatcher.h"
#include "EXStateSaver.h"
#include "EXString.h"
#include "EXMemory.h"

EXWatcher *EXWatcher::m_pWatcher;

void EXWatcher::Shutdown()
{
    if (m_pWatcher) delete m_pWatcher;
}

EXWatcher::~EXWatcher() {}

unsigned int EXWatcher::AddSectionFunc(const char *pName, unsigned int parent)
{
    unsigned int h = EXDebugTree::AddSection(pName, parent, ADD_DUPLICATE_ALWAYS);
    if (!h) return 0;
    EXDebugTreeSection *pSection = &m_Sections.ElementAt(h);
    EXStateSaver saver("WatcherSections");
    EXString name(pSection->m_Name);
    pSection->m_Open = saver.Restore(name + "-Open", 0);
    return h;
}

void EXWatcher::SetItemAddr(unsigned int h, void *pAddr)
{
    Item(h)->m_Value = (u32)pAddr;
}

void EXWatcher::DelItem(unsigned int h)
{
    m_WatchItems.ElementAt(h).m_Type = EXWATCH::DT_NONE;
    Item(h)->m_Value = 0;
    EXDebugTree::DelItem(h);
}

void EXWatcher::DisableItemEdit(unsigned int h)
{
    EXDebugTreeItem *pItem = Item(h);
    pItem->m_Flags &= ~0x10;
}

void EXWatcher::RestoreItemValue(unsigned int h)
{
    EXDebugTreeItem *pItem = Item(h);
    EXWatchItem *pWatch = &m_WatchItems.ElementAt(h);
    EXStateSaver saver("WatcherItems");
    EXString key(m_Sections.ElementAt(pItem->m_Section).m_Name);
    key += "-";
    key += pItem->m_Name;
    switch (pWatch->m_Type) {
    case EXWATCH::DT_1:
    case EXWATCH::DT_4:
        *(u8 *)pItem->m_Value = saver.Restore(key, *(u8 *)pItem->m_Value);
        break;
    case EXWATCH::DT_2:
    case EXWATCH::DT_5:
        *(u16 *)pItem->m_Value = saver.Restore(key, *(u16 *)pItem->m_Value);
        break;
    case EXWATCH::DT_3:
    case EXWATCH::DT_6:
    case EXWATCH::DT_7:
    case EXWATCH::DT_9:
        *(u32 *)pItem->m_Value = saver.Restore(key, *(u32 *)pItem->m_Value);
        break;
    case EXWATCH::DT_8:
        *(s8 *)pItem->m_Value = saver.Restore(key, *(s8 *)pItem->m_Value);
        break;
    }
}

s8 EXWatcher::Init(EXWatcher *pWatcher)
{
    m_pWatcher = pWatcher;
    pWatcher->AddSectionFunc("-Engine-", 0);
    pWatcher->AddSectionFunc("-Game-", 0);
    return 1;
}

unsigned int EXWatcher::AddItem(const char *pName, unsigned int section, EXWATCH::DataType type, void *pAddr)
{
    unsigned int h = EXDebugTree::AddItem(pName, section, 0, ADD_DUPLICATE_IGNORE);
    if (!h) return 0;
    if (h >= (unsigned int)m_WatchItems.Size()) m_WatchItems.SetSize(h + 1, -1);
    EXWatchItem &w = m_WatchItems.ElementAt(h);
    m_Items[h].m_Type = h;
    m_Items[h].m_Value = (u32)pAddr;
    w.m_Type = type;
    return h;
}

void EXWatcher::EnableItemEdit(unsigned int h, s8 save)
{
    if (h) {
        EXDebugTreeItem *pItem = Item(h);
        pItem->m_Edit |= 1;
        EXWatchItem &w = m_WatchItems.ElementAt(h);
        w.m_Min.m_u32 = 0;
        w.m_Max.m_u32 = 0;
        w.m_Step.m_u32 = 0;
        pItem->m_Edit |= save ? 2 : 0;
        if (save) RestoreItemValue(h);
    }
}

void EXWatcher::EnableItemEdit(unsigned int h, const ex::EXVar32 &min, const ex::EXVar32 &max, const ex::EXVar32 &step, s8 save)
{
    if (h) {
        EXDebugTreeItem *pItem = Item(h);
        pItem->m_Edit |= 1;
        EXWatchItem &w = m_WatchItems.ElementAt(h);
        w.m_Min = min;
        w.m_Max = max;
        w.m_Step = step;
        pItem->m_Edit |= save ? 2 : 0;
        if (save) RestoreItemValue(h);
    }
}

void EXWatcher::EnableItemEdit(unsigned int h, unsigned int min, unsigned int max, unsigned int step, s8 save)
{
    ex::EXVar32 vMin, vMax, vStep;
    vMin.m_u32 = min;
    vMax.m_u32 = max;
    vStep.m_u32 = step;
    EnableItemEdit(h, vMin, vMax, vStep, save);
}

void EXWatcher::EnableItemEdit(unsigned int h, int min, int max, int step, s8 save)
{
    ex::EXVar32 vMin, vMax, vStep;
    vMin.m_s32 = min;
    vMax.m_s32 = max;
    vStep.m_s32 = step;
    EnableItemEdit(h, vMin, vMax, vStep, save);
}

void EXWatcher::EnableItemEdit(unsigned int h, float min, float max, float step, s8 save)
{
    ex::EXVar32 vMin, vMax, vStep;
    vMin.m_f32 = min;
    vMax.m_f32 = max;
    vStep.m_f32 = step;
    EnableItemEdit(h, vMin, vMax, vStep, save);
}

EXWatcher::EXWatcher() : EXDebugTree(200, 200)
{
    m_WatchItems.AllocMem(200);
    m_WatchItems.SetGrowBy(200);
}
