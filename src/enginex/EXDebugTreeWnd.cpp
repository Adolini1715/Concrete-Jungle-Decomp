#include "EXDebugTreeWnd.h"
#include "EXWatcher.h"
#include "EXMenu.h"
#include "EXBaseDisplay.h"
#include "EXStateSaver.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"
#include "EXMemHeap.h"
#include "EXMouse.h"
#include <string.h>
#include <stdio.h>

namespace ex { double fd(float f); }

// Round to a number of decimal places (copy emitted here).
__forceinline float EXRound(float v, unsigned int places)
{
    float scale = 1.0f;
    for (unsigned int i = 0; i < places; i++) scale *= 10.0f;
    if (v > 0.0f) return (float)(int)(v * scale + 0.5f) / scale;
    return (float)(int)(v * scale - 0.5f) / scale;
}

s8 EXWatchWnd::Update()
{
    if (m_RepeatCount >= 0) {
        if (--m_RepeatCount == 0) {
            m_ClickCol = m_HeldCol;
            m_ClickRow = m_HeldRow;
            m_ClickUpper = m_HeldUpper;
            if (m_RepeatDelay > 1) m_RepeatDelay--;
            m_RepeatCount = m_RepeatDelay;
        }
    }
    return 1;
}

void EXWatchWnd::SaveState()
{
    EXStateSaver saver("XBWatchWnd");
    saver.Save("X", m_Rect.x);
    saver.Save("Y", m_Rect.y);
    saver.Save("W", m_Rect.w);
    saver.Save("H", m_Rect.h);
    saver.Save("StartRow", m_StartRow);
}

void EXWatchWnd::LoadState()
{
    EXStateSaver saver("XBWatchWnd");
    m_Rect.x = saver.Restore("X", m_Rect.x);
    m_Rect.y = saver.Restore("Y", m_Rect.y);
    m_Rect.w = saver.Restore("W", m_Rect.w);
    m_Rect.h = saver.Restore("H", m_Rect.h);
    m_StartRow = saver.Restore("StartRow", 0);
}

void EXWatchWnd::Close()
{
    SaveState();
    EXBaseDisplay::GetDisplay()->DelWnd(this);
}

void EXWatchWnd::ButtonUp(unsigned int b)
{
    if (b == 4) SaveState();
    m_RepeatCount = -1;
}

void EXStatsWnd::UpdateGroupMenu()
{
}

s8 EXStatsWnd::Update()
{
    return 1;
}

s8 EXStatsWnd::Draw()
{
    return 1;
}

s8 EXStatsWnd::MenuSelect(unsigned int id)
{
    return 1;
}

void EXStatsWnd::SaveState()
{
    EXStateSaver saver("XBNuStatsWnd");
    saver.Save("X", m_Rect.x);
    saver.Save("Y", m_Rect.y);
    saver.Save("W", m_Rect.w);
    saver.Save("H", m_Rect.h);
    saver.Save("ViewType", m_ViewType);
    saver.Save("TimerViewType", m_TimerViewType);
    saver.Save("GroupNum", m_GroupNum);
    saver.Save("Precision", m_Precision);
}

void EXStatsWnd::LoadState()
{
    EXStateSaver saver("XBNuStatsWnd");
    m_Rect.x = saver.Restore("X", m_Rect.x);
    m_Rect.y = saver.Restore("Y", m_Rect.y);
    m_Rect.w = saver.Restore("W", m_Rect.w);
    m_Rect.h = saver.Restore("H", m_Rect.h);
    m_ViewType = saver.Restore("ViewType", 0);
    m_TimerViewType = saver.Restore("TimerViewType", 0);
    m_GroupNum = saver.Restore("GroupNum", 0);
    m_Precision = saver.Restore("Precision", 5);
}

void EXStatsWnd::Close()
{
    SaveState();
    EXBaseDisplay::GetDisplay()->DelWnd(this);
}

void EXStatsWnd::ButtonUp(unsigned int b)
{
    if (b == 4) SaveState();
    m_Clicked = 0;
}

EXInfoTreeWnd::~EXInfoTreeWnd()
{
    if (m_pTree) {
        m_pTree->Empty();
        delete m_pTree;
    }
}

s8 EXInfoTreeWnd::Update()
{
    return 1;
}

void EXInfoTreeWnd::SaveState()
{
    EXStateSaver saver("XBInfoTreeWnd");
    saver.Save("X", m_Rect.x);
    saver.Save("Y", m_Rect.y);
    saver.Save("W", m_Rect.w);
    saver.Save("H", m_Rect.h);
    saver.Save("StartRow", m_StartRow);
}

void EXInfoTreeWnd::LoadState()
{
    EXStateSaver saver("XBInfoTreeWnd");
    m_Rect.x = saver.Restore("X", m_Rect.x);
    m_Rect.y = saver.Restore("Y", m_Rect.y);
    m_Rect.w = saver.Restore("W", m_Rect.w);
    m_Rect.h = saver.Restore("H", m_Rect.h);
    m_StartRow = saver.Restore("StartRow", 0);
}

void EXInfoTreeWnd::Close()
{
    SaveState();
    EXBaseDisplay::GetDisplay()->DelWnd(this);
}

void EXInfoTreeWnd::ButtonUp(unsigned int b)
{
    if (b == 4) SaveState();
}

s8 EXStatsBarWnd::Draw()
{
    return 1;
}

s8 EXMemBarWnd::Draw()
{
    static ex::RGBA usedCol(0x80, 0, 0x80, 0xff);
    static ex::RGBA cachedCol(0, 0x80, 0, 0xff);
    static ex::RGBA freeCol(0, 0x80, 0x80, 0xff);
    EXMemHeap::UpdateHeapStats();
    u32 width = m_Rect.w - 10;
    int bottom = m_BarHeight;
    int top = 0;
    float total = (float)m_Total;
    float scale;
    if (total != 0.0f) scale = (float)width / total;
    else scale = 0.0f;
    if (m_ShowStats) top = 3;
    bottom += top;
    _SolidRect2D(m_Color, 5, top, width + 5, bottom);
    int x = 6;
    if (m_ShowStats) {
        _Line2D(m_Color, 5, top - 3, 5, bottom);
        for (int i = 0; i < 4; i++) {
            EXMemHeap *pHeap = _SystemHeapList[i];
            if (pHeap) {
                x += (int)(pHeap->RtlSizeHeap() * scale);
                _Line2D(m_Color, x, top - 3, x, bottom);
            }
        }
    } else {
        _Line2D(m_Color, 5, top, 5, bottom + 3);
        for (int i = 0; i < 4; i++) {
            EXMemHeap *pHeap = _SystemHeapList[i];
            if (pHeap) {
                x += (int)(pHeap->RtlSizeHeap() * scale);
                _Line2D(m_Color, x, top, x, bottom + 3);
            }
        }
    }
    int start = 6;
    for (int i = 0; i < 4; i++) {
        EXMemHeap *pHeap = _SystemHeapList[i];
        if (pHeap) {
            int used = (int)(pHeap->Debug_Stats(1) * scale);
            int cached = (int)(pHeap->Debug_Stats(2) * scale);
            u32 free = pHeap->Debug_Stats(3);
            int x1 = start + used;
            _SolidRect2D(usedCol, start, top + 1, x1, bottom - 1);
            int x2 = x1 + cached;
            _SolidRect2D(cachedCol, x1, top + 1, x2, bottom - 1);
            _SolidRect2D(freeCol, x2, top + 1, x2 + (int)(free * scale), bottom - 1);
            start += (int)(pHeap->RtlSizeHeap() * scale);
        }
    }
    return 1;
}

EXWatchWnd::EXWatchWnd()
{
    m_Flags = 0x80004d9a;
    m_BkColor = ex::RGBA(0x10, 0x10, 0x10, 0x60);
    m_StartRow = 0;
    m_Rect.Set(10, 20, 500, 380);
    LoadState();
    SetRect(m_Rect);
    m_ClickCol = -1;
    m_ClickRow = -1;
    m_RepeatCount = -1;
    m_RepeatDelay = -1;
    m_ScrollUp = 0;
    m_ScrollDown = 0;
}

void EXWatchWnd::DrawItem(EXDebugTreeItem *pItem, unsigned short depth, unsigned char branch, unsigned char width)
{
    short row = m_Row - m_StartRow;
    if (row < 1 || row >= m_DbgHeight) {
        m_Row++;
        return;
    }
    SetDbgPos(0, row);
    DbgPrint("~C222");
    for (unsigned int i = 1; i < depth; i++) DbgPrint("%c", 0x92);
    if (depth > 0) DbgPrint("%c", branch);
    DbgPrint("~C333");
    EXWatchItem *pWatch = EXWatcher::GetWatcher()->WatchItem(pItem->m_Type);
    void *p = (void *)pItem->m_Value;
    switch (pWatch->m_Type) {
    case EXWATCH::DT_1: {
        u8 *pv = (u8 *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s %c%u", width, pItem->m_Name.GetData(), 0x93, *pv);
            if (m_ClickRow == row && m_ClickCol == depth + width + 2) {
                if (m_ClickUpper) {
                    if (*pv < pWatch->m_Max.m_s32 - (u8)pWatch->m_Step.m_u32) *pv += (u8)pWatch->m_Step.m_u32;
                    else *pv = (u8)pWatch->m_Max.m_u32;
                } else {
                    if (*pv > pWatch->m_Min.m_s32 + (u8)pWatch->m_Step.m_u32) *pv -= (u8)pWatch->m_Step.m_u32;
                    else *pv = (u8)pWatch->m_Min.m_u32;
                }
                goto save;
            }
        } else {
            DbgPrint(" %-*s %u", width, pItem->m_Name.GetData(), *pv);
        }
        break;
    }
    case EXWATCH::DT_2: {
        u16 *pv = (u16 *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s %c%u", width, pItem->m_Name.GetData(), 0x93, *pv);
            if (m_ClickRow == row && m_ClickCol == depth + width + 2) {
                if (m_ClickUpper) {
                    if (*pv < pWatch->m_Max.m_s32 - (u16)pWatch->m_Step.m_u32) *pv += (u16)pWatch->m_Step.m_u32;
                    else *pv = (u16)pWatch->m_Max.m_u32;
                } else {
                    if (*pv > pWatch->m_Min.m_s32 + (u16)pWatch->m_Step.m_u32) *pv -= (u16)pWatch->m_Step.m_u32;
                    else *pv = (u16)pWatch->m_Min.m_u32;
                }
                goto save;
            }
        } else {
            DbgPrint(" %-*s %u", width, pItem->m_Name.GetData(), *pv);
        }
        break;
    }
    case EXWATCH::DT_3: {
        u32 *pv = (u32 *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s %c%u", width, pItem->m_Name.GetData(), 0x93, *pv);
            if (m_ClickRow == row && m_ClickCol == depth + width + 2) {
                if (m_ClickUpper) {
                    if (*pv < pWatch->m_Max.m_u32 - pWatch->m_Step.m_u32) *pv += pWatch->m_Step.m_u32;
                    else *pv = pWatch->m_Max.m_u32;
                } else {
                    if (*pv > pWatch->m_Min.m_u32 + pWatch->m_Step.m_u32) *pv -= pWatch->m_Step.m_u32;
                    else *pv = pWatch->m_Min.m_u32;
                }
                goto save;
            }
        } else {
            DbgPrint(" %-*s %u", width, pItem->m_Name.GetData(), *pv);
        }
        break;
    }
    case EXWATCH::DT_4: {
        s8 *pv = (s8 *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s %c%d", width, pItem->m_Name.GetData(), 0x93, *pv);
            if (m_ClickRow == row && m_ClickCol == depth + width + 2) {
                if (m_ClickUpper) {
                    if (*pv < pWatch->m_Max.m_s32 - (s8)pWatch->m_Step.m_s32) *pv += (s8)pWatch->m_Step.m_s32;
                    else *pv = (s8)pWatch->m_Max.m_s32;
                } else {
                    if (*pv > pWatch->m_Min.m_s32 + (s8)pWatch->m_Step.m_s32) *pv -= (s8)pWatch->m_Step.m_s32;
                    else *pv = (s8)pWatch->m_Min.m_s32;
                }
                goto save;
            }
        } else {
            DbgPrint(" %-*s %d", width, pItem->m_Name.GetData(), *pv);
        }
        break;
    }
    case EXWATCH::DT_5: {
        short *pv = (short *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s %c%d", width, pItem->m_Name.GetData(), 0x93, *pv);
            if (m_ClickRow == row && m_ClickCol == depth + width + 2) {
                if (m_ClickUpper) {
                    if (*pv < pWatch->m_Max.m_s32 - (short)pWatch->m_Step.m_s32) *pv += (short)pWatch->m_Step.m_s32;
                    else *pv = (short)pWatch->m_Max.m_s32;
                } else {
                    if (*pv > pWatch->m_Min.m_s32 + (short)pWatch->m_Step.m_s32) *pv -= (short)pWatch->m_Step.m_s32;
                    else *pv = (short)pWatch->m_Min.m_s32;
                }
                goto save;
            }
        } else {
            DbgPrint(" %-*s %d", width, pItem->m_Name.GetData(), *pv);
        }
        break;
    }
    case EXWATCH::DT_6: {
        int *pv = (int *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s %c%d", width, pItem->m_Name.GetData(), 0x93, *pv);
            if (m_ClickRow == row && m_ClickCol == depth + width + 2) {
                if (m_ClickUpper) {
                    if (*pv < pWatch->m_Max.m_s32 - pWatch->m_Step.m_s32) *pv += pWatch->m_Step.m_s32;
                    else *pv = pWatch->m_Max.m_s32;
                } else {
                    if (*pv > pWatch->m_Min.m_s32 + pWatch->m_Step.m_s32) *pv -= pWatch->m_Step.m_s32;
                    else *pv = pWatch->m_Min.m_s32;
                }
                goto save;
            }
        } else {
            DbgPrint(" %-*s %d", width, pItem->m_Name.GetData(), *pv);
        }
        break;
    }
    case EXWATCH::DT_7: {
        float *pv = (float *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s %c%f", width, pItem->m_Name.GetData(), 0x93, ex::fd(*pv));
            if (m_ClickRow == row && m_ClickCol == depth + width + 2) {
                if (m_ClickUpper) {
                    if (*pv < pWatch->m_Max.m_f32) *pv += pWatch->m_Step.m_f32;
                    else *pv = pWatch->m_Max.m_f32;
                } else {
                    if (*pv > pWatch->m_Min.m_f32) *pv -= pWatch->m_Step.m_f32;
                    else *pv = pWatch->m_Min.m_f32;
                }
                goto save;
            }
        } else {
            DbgPrint(" %-*s %f", width, pItem->m_Name.GetData(), ex::fd(*pv));
        }
        break;
    }
    case EXWATCH::DT_8: {
        s8 *pv = (s8 *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s %c%s", width, pItem->m_Name.GetData(), *pv ? 0x8a : 0x8b, *pv ? "TRUE" : "FALSE");
            if (m_ClickRow == row && m_ClickCol == depth + width + 2) {
                *pv = !*pv;
                goto save;
            }
        } else {
            DbgPrint(" %-*s %s", width, pItem->m_Name.GetData(), *pv ? "TRUE" : "FALSE");
        }
        break;
    }
    case EXWATCH::DT_9: {
        ex::RGBA col = *(ex::RGBA *)p;
        if (pItem->m_Edit & 1) {
            DbgPrint(" %-*s ~C311%c%-3d ~C131%c%-3d ~C113%c%-3d ~C333%c%-3d", width, pItem->m_Name.GetData(),
                     0x93, col.m_R, 0x93, col.m_G, 0x93, col.m_B, 0x93, col.m_A);
            if (m_ClickRow == row) {
                if (m_ClickCol == depth + width + 2) {
                    if (m_ClickUpper && col.m_R < 0xff) col.m_R++;
                    else if (!m_ClickUpper && col.m_R > 0) col.m_R--;
                }
                if (m_ClickCol == depth + width + 7) {
                    if (m_ClickUpper && col.m_G < 0xff) col.m_G++;
                    else if (!m_ClickUpper && col.m_G > 0) col.m_G--;
                }
                if (m_ClickCol == depth + width + 12) {
                    if (m_ClickUpper && col.m_B < 0xff) col.m_B++;
                    else if (!m_ClickUpper && col.m_B > 0) col.m_B--;
                }
                if (m_ClickCol == depth + width + 17) {
                    if (m_ClickUpper && col.m_A < 0xff) col.m_A++;
                    else if (!m_ClickUpper && col.m_A > 0) col.m_A--;
                }
                *(ex::RGBA *)p = col;
                goto save;
            }
        } else {
            DbgPrint(" %-*s ~C311%-3d ~C131%-3d ~C113%-3d ~C333%-3d", width, pItem->m_Name.GetData(),
                     col.m_R, col.m_G, col.m_B, col.m_A);
        }
        break;
    }
    default:
        DbgPrint(" %-*s ???", width, pItem->m_Name.GetData());
        break;
    }
    m_Row++;
    return;
save:
    if (pItem->m_Edit & 2) {
        EXStateSaver saver("WatcherItems");
        EXString key(EXWatcher::GetWatcher()->Section(pItem->m_Section)->m_Name);
        key += "-";
        key += pItem->m_Name;
        switch (pWatch->m_Type) {
        case EXWATCH::DT_1:
        case EXWATCH::DT_4:
            saver.Save(key, *(u8 *)p);
            break;
        case EXWATCH::DT_2:
        case EXWATCH::DT_5:
            saver.Save(key, *(u16 *)p);
            break;
        case EXWATCH::DT_3:
        case EXWATCH::DT_6:
        case EXWATCH::DT_7:
        case EXWATCH::DT_9:
            saver.Save(key, *(u32 *)p);
            break;
        case EXWATCH::DT_8:
            saver.Save(key, *(s8 *)p);
            break;
        }
    }
    m_Row++;
}

void EXWatchWnd::ButtonDown(unsigned int b)
{
    if (b == 1) {
        EXPoint p = EXBaseDisplay::DispToPrim(EXMouse::GetMouse()->Pos());
        EXRect r(m_PrimRect.x + m_DbgWidth * 12 - 0x12, m_PrimRect.y, 0x18, 0xc);
        if (r.ContainsPoint(p)) {
            SaveState();
            EXBaseDisplay::GetDisplay()->DelWnd(this);
            return;
        }
        p -= m_PrimRect.TopLeft();
        m_HeldCol = m_ClickCol = p.x / 12;
        m_HeldRow = m_ClickRow = p.y / 12;
        m_HeldUpper = m_ClickUpper = p.y < m_ClickRow * 12 + 6;
        m_RepeatCount = 20;
        m_RepeatDelay = 10;
    } else if (b == 0x20) {
        m_ScrollUp = 1;
    } else if (b == 0x40) {
        m_ScrollDown = 1;
    }
}

EXStatsWnd::EXStatsWnd()
{
    m_Flags = 0x80004d9a;
    m_BkColor = ex::RGBA(0x10, 0x10, 0x10, 0x60);
    m_Unk2a4 = 0;
    m_GroupNum = 0;
    m_StartRow = 0;
    m_ViewType = 0;
    m_TimerViewType = 0;
    m_Precision = 5;
    m_Rect.Set(10, 20, 500, 380);
    LoadState();
    SetRect(m_Rect);
    m_ClickCol = -1;
    m_ClickRow = -1;
    m_ScrollUp = 0;
    m_ScrollDown = 0;
    m_Clicked = 0;
    m_pMenu = new EXMenu;
    m_pMenu->SetTopLevel(1);
    m_pMenu->ShowPushPin(0);
    m_pMenu->ShowCloseButton(1);
    m_pMenu->SetPushPinIn(1);
    m_pMenu->Open(this, 0, 0);
    m_pGroupMenu = m_pMenu->AddSubMenu("Group", 0);
    EXMenu *pMenu = m_pMenu->AddSubMenu("View", 0);
    pMenu->AddItem("Millisecs", 0);
    pMenu->AddItem("%Frame", 1);
    pMenu->AddItem("ScanLines", 2);
    pMenu = m_pMenu->AddSubMenu("Timers", 0);
    pMenu->AddItem("Current", 10);
    pMenu->AddItem("Average", 11);
    pMenu->AddItem("Maximum", 12);
    pMenu->AddItem("Minimun", 13);
    pMenu->AddItem("Reset", 14);
    pMenu = m_pMenu->AddSubMenu("Format", 0);
    pMenu->AddItem("Precision++", 20);
    pMenu->AddItem("Precision--", 21);
    m_Unk2a8 = -1;
    m_Unk2ac = -1;
    m_Unk2b0 = -1;
    m_Unk2b4 = 0;
}

void EXStatsWnd::FormatValue(float v, char *pBuf, int width)
{
    v = EXRound(v, m_Precision);
    sprintf(pBuf, "%f", ex::fd(v));
    for (int i = 0; pBuf[i]; i++) {
        if (pBuf[i] == '.') {
            pBuf[i + m_Precision + 1] = 0;
            int pad = max(width - i, 0);
            if (pad > 0) {
                for (int j = strlen(pBuf); j >= 0; j--) pBuf[j + pad] = pBuf[j];
                while (pad--) pBuf[pad] = ' ';
            }
            return;
        }
    }
}

void EXStatsWnd::ButtonDown(unsigned int b)
{
    if (b == 1) {
        EXPoint p = EXBaseDisplay::DispToPrim(EXMouse::GetMouse()->Pos());
        p -= m_PrimRect.TopLeft();
        p.x /= 12;
        p.y /= 12;
        m_HeldCol = m_ClickCol = p.x;
        m_HeldRow = m_ClickRow = p.y;
        m_Clicked = 1;
    } else if (b == 0x20) {
        m_ScrollUp = 1;
    } else if (b == 0x40) {
        m_ScrollDown = 1;
    }
}

EXInfoTreeWnd::EXInfoTreeWnd()
{
    m_BkColor = ex::RGBA(0x10, 0x10, 0x10, 0x60);
    m_Flags = 0x4d9a;
    m_StartRow = 0;
    m_Rect.Set(10, 20, 500, 380);
    SetRect(m_Rect);
    m_ClickCol = -1;
    m_ClickRow = -1;
    m_ScrollUp = 0;
    m_ScrollDown = 0;
    m_pTree = new EXInfoTree(500, 1000);
}

void EXInfoTreeWnd::DrawItem(EXDebugTreeItem *pItem, unsigned short depth, unsigned char branch, unsigned char width)
{
    short row = m_Row - m_StartRow;
    if (row < 1 || row >= m_DbgHeight) {
        m_Row++;
        return;
    }
    {
        SetDbgPos(0, row);
        DbgPrint("~C222");
        for (unsigned int i = 1; i < depth; i++) DbgPrint("%c", 0x92);
        if (depth > 0) DbgPrint("%c", branch);
        DbgPrint("~C333");
        switch (pItem->m_Type) {
        case 2:
            DbgPrint(" %s", ((EXString *)pItem->m_Value)->c_str());
            break;
        case 1:
            DbgPrint(" %f", ex::fd(*(float *)&pItem->m_Value));
            break;
        case 0:
            DbgPrint(" %u", pItem->m_Value);
            break;
        case 0x80000000:
            DbgPrint(" %-*s %u", width, pItem->m_Name.GetData(), pItem->m_Value);
            break;
        default:
            DbgPrint(" %-*s *UNKNOWN TYPE*", width, pItem->m_Name.GetData());
            break;
        case 0x80000002:
            DbgPrint(" %-*s %s", width, pItem->m_Name.GetData(), ((EXString *)pItem->m_Value)->c_str());
            break;
        case 0x80000001:
            DbgPrint(" %-*s %f", width, pItem->m_Name.GetData(), ex::fd(*(float *)&pItem->m_Value));
            break;
        }
    }
    m_Row++;
}

void EXInfoTreeWnd::ButtonDown(unsigned int b)
{
    if (b == 1) {
        EXPoint p = EXBaseDisplay::DispToPrim(EXMouse::GetMouse()->Pos());
        EXRect r(m_PrimRect.x + m_DbgWidth * 12 - 0x12, m_PrimRect.y, 0x18, 0xc);
        if (r.ContainsPoint(p)) {
            SaveState();
            EXBaseDisplay::GetDisplay()->DelWnd(this);
            return;
        }
        p -= m_PrimRect.TopLeft();
        m_ClickCol = p.x / 12;
        m_ClickRow = p.y / 12;
    } else if (b == 0x20) {
        m_ScrollUp = 1;
    } else if (b == 0x40) {
        m_ScrollDown = 1;
    }
}

void EXStatsBarWnd::DelStat(unsigned int i)
{
    m_Bars.RemoveAt(i, 1);
}

EXMemBarWnd::EXMemBarWnd()
{
    m_BkColor = ex::RGBA(0, 0, 0, 0);
    m_Color = ex::RGBA(0x80, 0x80, 0x80, 0x80);
    m_Flags = 4;
    m_BarHeight = 7;
    m_ShowStats = 0;
    m_Rect.Set(10, 10, EXBaseDisplay::GetDisplay()->Params().m_Width - 0x14, 0x32);
    m_Total = 0;
    for (int i = 0; i < 4; i++) {
        if (_SystemHeapList[i]) m_Total += _SystemHeapList[i]->RtlSizeHeap();
    }
}

s8 EXWatchWnd::DrawSection(EXDebugTreeSection *pSection, unsigned short depth, unsigned char branch)
{
    short row = m_Row - m_StartRow;
    if (m_ClickRow == row && m_ClickCol == depth) {
        pSection->m_Open = !pSection->m_Open;
        EXStateSaver saver("WatcherSections");
        EXString name(pSection->m_Name);
        saver.Save(name + "-Open", pSection->m_Open);
    }
    if (row > 0 && row < m_DbgHeight) {
        SetDbgPos(0, row);
        DbgPrint("~C222");
        for (unsigned int i = 1; i < depth; i++) DbgPrint("%c", 0x92);
        if (depth > 0) DbgPrint("%c", branch);
        DbgPrint("%c ~C333", 0x8e | pSection->m_Open);
        DbgPrint("%s", pSection->m_Name.GetData());
    }
    m_Row++;
    if (!pSection->m_Open) return row >= m_DbgHeight - 1;
    EXDebugTreeItem *pItem = (EXDebugTreeItem *)pSection->m_Items.Head();
    EXDebugTreeItem *pLastItem = (EXDebugTreeItem *)pSection->m_Items.Tail();
    EXDebugTreeSection *pChild = (EXDebugTreeSection *)pSection->m_Children.Head();
    EXDebugTreeSection *pLast = (EXDebugTreeSection *)pSection->m_Children.Tail();
    if (pItem) pLast = 0;
    for (; pChild; pChild = (EXDebugTreeSection *)pChild->Next())
        DrawSection(pChild, depth + 1, pChild == pLast ? 0x91 : 0x90);
    u8 width = 0;
    for (; pItem; pItem = (EXDebugTreeItem *)pItem->Next()) {
        u8 len = strlen(pItem->m_Name);
        if (len > width) width = len;
    }
    for (pItem = (EXDebugTreeItem *)pSection->m_Items.Head(); pItem; pItem = (EXDebugTreeItem *)pItem->Next())
        DrawItem(pItem, depth + 1, pItem == pLastItem ? 0x91 : 0x90, width);
    return (short)(m_Row - m_StartRow) >= m_DbgHeight - 1;
}

s8 EXInfoTreeWnd::DrawSection(EXDebugTreeSection *pSection, unsigned short depth, unsigned char branch)
{
    short row = m_Row - m_StartRow;
    if (m_ClickRow == row && m_ClickCol == depth) {
        pSection->m_Open = !pSection->m_Open;
        EXStateSaver saver("StatsSections");
        EXString name(pSection->m_Name);
        saver.Save(name + "-Open", pSection->m_Open);
    }
    if (row > 0 && row < m_DbgHeight) {
        SetDbgPos(0, row);
        DbgPrint("~C222");
        for (unsigned int i = 1; i < depth; i++) DbgPrint("%c", 0x92);
        if (depth > 0) DbgPrint("%c", branch);
        DbgPrint("%c ~C333", 0x8e | pSection->m_Open);
        DbgPrint("%s", pSection->m_Name.GetData());
    }
    m_Row++;
    if (!pSection->m_Open) return row >= m_DbgHeight - 1;
    EXDebugTreeItem *pItem = (EXDebugTreeItem *)pSection->m_Items.Head();
    EXDebugTreeItem *pLastItem = (EXDebugTreeItem *)pSection->m_Items.Tail();
    EXDebugTreeSection *pChild = (EXDebugTreeSection *)pSection->m_Children.Head();
    EXDebugTreeSection *pLast = (EXDebugTreeSection *)pSection->m_Children.Tail();
    if (pItem) pLast = 0;
    for (; pChild; pChild = (EXDebugTreeSection *)pChild->Next())
        DrawSection(pChild, depth + 1, pChild == pLast ? 0x91 : 0x90);
    u8 width = 0;
    for (; pItem; pItem = (EXDebugTreeItem *)pItem->Next()) {
        u8 len = strlen(pItem->m_Name);
        if (len > width) width = len;
    }
    for (pItem = (EXDebugTreeItem *)pSection->m_Items.Head(); pItem; pItem = (EXDebugTreeItem *)pItem->Next())
        DrawItem(pItem, depth + 1, pItem == pLastItem ? 0x91 : 0x90, width);
    return (short)(m_Row - m_StartRow) >= m_DbgHeight - 1;
}

unsigned int EXStatsBarWnd::AddCounter(EXCounter *pCounter, ex::RGBA col)
{
    EXStatsSubBar &b = m_Bars.AddSpace(1);
    b.m_pTimer = 0;
    b.m_pCounter = pCounter;
    b.m_Color = col;
    b.m_Bar = 0;
    return m_Bars.GetUpperBound();
}

unsigned int EXStatsBarWnd::AddTimer(EXTimer *pTimer, ex::RGBA col)
{
    EXStatsSubBar &b = m_Bars.AddSpace(1);
    b.m_pTimer = pTimer;
    b.m_pCounter = 0;
    b.m_Color = col;
    b.m_Bar = 0;
    return m_Bars.GetUpperBound();
}

unsigned int EXStatsBarWnd::AddBarCounter(EXCounter *pCounter, ex::RGBA col)
{
    EXStatsSubBar &b = m_Bars.AddSpace(1);
    b.m_pTimer = 0;
    b.m_pCounter = pCounter;
    b.m_Color = col;
    b.m_Bar = 1;
    return m_Bars.GetUpperBound();
}

unsigned int EXStatsBarWnd::AddBarTimer(EXTimer *pTimer, ex::RGBA col)
{
    EXStatsSubBar &b = m_Bars.AddSpace(1);
    b.m_pTimer = pTimer;
    b.m_pCounter = 0;
    b.m_Color = col;
    b.m_Bar = 1;
    return m_Bars.GetUpperBound();
}

s8 EXWatchWnd::Draw()
{
    SetDbgPos(0, 1);
    m_Row = 1;
    s8 more = 0;
    for (EXDebugTreeSection *p = (EXDebugTreeSection *)EXWatcher::GetWatcher()->Section(0)->m_Children.Head(); p;
         p = (EXDebugTreeSection *)p->Next()) {
        more = DrawSection(p, 0, 0x90);
        if (more) break;
    }
    if (m_StartRow > 0 || more) {
        unsigned short x = m_DbgWidth - 1;
        SetDbgPos(x, 1);
        DbgPrint("%c", 0x87);
        for (int i = 2; i < m_DbgHeight - 1; i++) {
            SetDbgPos(x, i);
            DbgPrint("%c", 0x7f);
        }
        SetDbgPos(x, m_DbgHeight - 1);
        DbgPrint("%c", 0x86);
        unsigned short h = m_DbgHeight;
        unsigned short pos;
        if (m_Row <= h) pos = h - 2;
        else pos = (h - 4) * m_StartRow / (m_Row - h) + 2;
        pos = min(max(pos, (unsigned short)2), (unsigned short)(h - 2));
        SetDbgPos(x, pos);
        DbgPrint("%c", 0x88);
        if (m_ClickCol == x) {
            if (m_ClickRow == 1 && m_StartRow > 0) {
                m_StartRow--;
            } else if (m_ClickRow == m_DbgHeight - 1 && more) {
                m_StartRow++;
            } else if (m_ClickRow < pos) {
                m_StartRow = max(0, m_StartRow - m_DbgHeight + 2);
            } else if (m_ClickRow > pos) {
                m_StartRow = min(m_Row - m_DbgHeight + 1, m_StartRow + m_DbgHeight - 2);
            }
        }
        if (m_ScrollUp && m_StartRow > 0) m_StartRow -= min(m_StartRow, (u16)3);
        else if (m_ScrollDown && more) m_StartRow += 3;
    }
    m_ClickCol = -1;
    m_ClickRow = -1;
    m_ScrollUp = 0;
    m_ScrollDown = 0;
    unsigned short width = m_DbgWidth;
    memset(m_pDbgText, 0, width * 2);
    SetDbgPos(0, 0);
    DbgPrint("Watcher");
    SetDbgPos(width - 2, 0);
    DbgPrint("%cX", 0x7f);
    return 1;
}

s8 EXInfoTreeWnd::Draw()
{
    SetDbgPos(0, 1);
    m_Row = 1;
    s8 more = 0;
    for (EXDebugTreeSection *p = (EXDebugTreeSection *)m_pTree->Section(0)->m_Children.Head(); p;
         p = (EXDebugTreeSection *)p->Next()) {
        more = DrawSection(p, 0, 0x90);
        if (more) break;
    }
    if (m_StartRow > 0 || more) {
        unsigned short x = m_DbgWidth - 1;
        SetDbgPos(x, 1);
        DbgPrint("%c", 0x87);
        for (int i = 2; i < m_DbgHeight - 1; i++) {
            SetDbgPos(x, i);
            DbgPrint("%c", 0x7f);
        }
        SetDbgPos(x, m_DbgHeight - 1);
        DbgPrint("%c", 0x86);
        unsigned short h = m_DbgHeight;
        unsigned short pos;
        if (m_Row <= h) pos = h - 2;
        else pos = (h - 4) * m_StartRow / (m_Row - h) + 2;
        pos = min(max(pos, (unsigned short)2), (unsigned short)(h - 2));
        SetDbgPos(x, pos);
        DbgPrint("%c", 0x88);
        if (m_ClickCol == x) {
            if (m_ClickRow == 1 && m_StartRow > 0) {
                m_StartRow--;
            } else if (m_ClickRow == m_DbgHeight - 1 && more) {
                m_StartRow++;
            } else if (m_ClickRow < pos) {
                m_StartRow = max(0, m_StartRow - m_DbgHeight + 2);
            } else if (m_ClickRow > pos) {
                m_StartRow = min(m_Row - m_DbgHeight + 1, m_StartRow + m_DbgHeight - 2);
            }
        }
        if (m_ScrollUp && m_StartRow > 0) m_StartRow--;
        else if (m_ScrollDown && more) m_StartRow++;
    }
    m_ClickCol = -1;
    m_ClickRow = -1;
    m_ScrollUp = 0;
    m_ScrollDown = 0;
    unsigned short width = m_DbgWidth;
    memset(m_pDbgText, 0, width * 2);
    SetDbgPos(0, 0);
    DbgPrint("InfoTree");
    SetDbgPos(width - 2, 0);
    DbgPrint("%cX", 0x7f);
    return 1;
}

EXStatsBarWnd::EXStatsBarWnd()
{
    m_Bars.AllocMem(10);
    m_Bars.SetGrowBy(10);
    m_Unk2b9 = 0;
    m_Unk2b8 = 0;
    m_Unk2bb = 0;
    m_Unk2bc = 0;
    m_Unk2c4 = 0;
    m_Unk2c0 = 0;
    m_Flags = 4;
    m_BkColor = ex::RGBA(0, 0, 0, 0);
    m_Unk2ac = 100;
    m_Unk2b0 = 7;
    m_Unk2b4 = ex::RGBA(0x80, 0x80, 0x80, 0x80);
    m_Rect.Set(10, 10, EXBaseDisplay::GetDisplay()->Params().m_Width - 0x14, 0x32);
}
