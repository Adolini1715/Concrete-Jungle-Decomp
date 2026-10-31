#include "EXGamePad.h"
#include "EXMemory.h"

EXBaseGamePad *EXBaseGamePad::m_pGamePad;

EXBaseGamePad::~EXBaseGamePad() {}

s8 EXBaseGamePad::Init(EXBaseGamePad *pPad)
{
    m_pGamePad = pPad;
    return 1;
}

void EXBaseGamePad::Shutdown()
{
    if (m_pGamePad) delete m_pGamePad;
}

EXBaseGamePad::EXBaseGamePad()
{
    for (int i = 0; i < 8; i++) {
        m_Buttons[i] = 0;
        m_PrevButtons[i] = 0;
        m_ButtonsPressed[i] = 0;
        m_ButtonsReleased[i] = 0;
        for (int j = 0; j < 16; j++) m_Analog[i][j] = 0;
        m_DeadZoneMode[i][0] = 0;
        m_DeadZoneMode[i][1] = 0;
        m_Actions[i] = 0;
        m_PrevActions[i] = 0;
        m_ActionsPressed[i] = 0;
        m_ActionsReleased[i] = 0;
        m_NumActionMaps[i] = 0;
        m_RumbleHiTime[i] = 0;
        m_RumbleLoTime[i] = 0;
        m_RumbleHi[i] = 0;
        m_RumbleLo[i] = 0;
        m_Attached[i] = 0;
        m_RumbleEnabled[i] = 0;
    }
}

// NEAR: retail reloads m_PrevActions after the inner loop and keeps the map pointer in ebp;
// ours forwards the copied value in a register (register allocation differs).
void EXBaseGamePad::UpdateActions()
{
    for (int i = 0; i < 8; i++) {
        if (m_Attached[(u8)i]) {
            if (m_NumActionMaps[i] > 0) {
                m_PrevActions[i] = m_Actions[i];
                u32 buttons = m_Buttons[i];
                u32 actions = 0;
                EXPadActionMap *pMap = m_ActionMap[i];
                for (int j = 0; j < m_NumActionMaps[i]; j++) {
                    if (pMap[j].m_Buttons & buttons) actions |= pMap[j].m_Action;
                }
                m_ActionsPressed[i] = (m_PrevActions[i] ^ actions) & actions;
                m_ActionsReleased[i] = (m_PrevActions[i] ^ actions) & m_PrevActions[i];
                m_Actions[i] = actions;
            } else {
                m_Actions[i] = 0;
                m_PrevActions[i] = 0;
                m_ActionsPressed[i] = 0;
                m_ActionsReleased[i] = 0;
            }
        }
    }
}

void EXBaseGamePad::SetActionMap(u8 pad, u8 count, const EXPadActionMap *pMap)
{
    m_NumActionMaps[pad] = count;
    memcpy(m_ActionMap[pad], pMap, count * sizeof(EXPadActionMap));
}

void EXBaseGamePad::StartRumbleAll(u8 pad, u8 strength, u32 time)
{
    m_RumbleHiTime[pad] = max(time, m_RumbleHiTime[pad]);
    m_RumbleLoTime[pad] = max(time, m_RumbleLoTime[pad]);
    if (m_RumbleHiTime[pad]) m_RumbleHi[pad] = strength;
    else m_RumbleHi[pad] = 0;
    m_RumbleLo[pad] = m_RumbleLoTime[pad] ? strength : (u8)0;
    ((EXGamePad *)this)->SetPadRumble(pad);
}

void EXBaseGamePad::StartRumbleHi(u8 pad, u8 strength, u32 time)
{
    m_RumbleHiTime[pad] = max(time, m_RumbleHiTime[pad]);
    m_RumbleHi[pad] = m_RumbleHiTime[pad] ? strength : (u8)0;
    ((EXGamePad *)this)->SetPadRumble(pad);
}

void EXBaseGamePad::StartRumbleLo(u8 pad, u8 strength, u32 time)
{
    m_RumbleLoTime[pad] = max(time, m_RumbleLoTime[pad]);
    m_RumbleLo[pad] = m_RumbleLoTime[pad] ? strength : (u8)0;
    ((EXGamePad *)this)->SetPadRumble(pad);
}

void EXBaseGamePad::StopRumbleAll(u8 pad)
{
    m_RumbleHiTime[pad] = 0;
    m_RumbleLoTime[pad] = 0;
    m_RumbleHi[pad] = 0;
    m_RumbleLo[pad] = 0;
    ((EXGamePad *)this)->SetPadRumble(pad);
}

void EXBaseGamePad::StopRumbleHi(u8 pad)
{
    m_RumbleHiTime[pad] = 0;
    m_RumbleHi[pad] = 0;
    ((EXGamePad *)this)->SetPadRumble(pad);
}

void EXBaseGamePad::StopRumbleLo(u8 pad)
{
    m_RumbleLoTime[pad] = 0;
    m_RumbleLo[pad] = 0;
    ((EXGamePad *)this)->SetPadRumble(pad);
}

// NEAR: retail replicates the byte without the movsx and pushes ebx/edi only on the !enable path.
void EXBaseGamePad::EnableAllRumble(s8 enable)
{
    memset(&m_RumbleEnabled[0], enable, 8);
    if (!enable) {
        for (int i = 0; i < 8; i++) StopRumbleAll(i);
    }
}

// NEAR: same code, retail uses esi/dl where we use edx/bl.
void EXBaseGamePad::RumbleUpdate(u8 pad)
{
    s8 changed = 0;
    if (m_RumbleHiTime[pad] && --m_RumbleHiTime[pad] == 0) {
        m_RumbleHi[pad] = 0;
        changed = 1;
    }
    if (m_RumbleLoTime[pad] && --m_RumbleLoTime[pad] == 0) {
        m_RumbleLo[pad] = 0;
        changed = 1;
    }
    if (changed) ((EXGamePad *)this)->SetPadRumble(pad);
}

void EXBaseGamePad::KillAllRumble()
{
    for (int i = 0; i < 8; i++) {
        if (m_Attached[(u8)i]) StopRumbleAll(i);
    }
}
