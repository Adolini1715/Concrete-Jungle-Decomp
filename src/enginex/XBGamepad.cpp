#include <math.h>
#include "EXGamePad.h"
#include "EXMemory.h"

// Xbox input API, declared here (no XDK headers).
extern "C" {
extern xb::_XPP_DEVICE_TYPE XDEVICE_TYPE_GAMEPAD_TABLE;
u32 __stdcall XGetDevices(xb::_XPP_DEVICE_TYPE *pType);
void *__stdcall XInputOpen(xb::_XPP_DEVICE_TYPE *pType, u32 port, u32 slot, void *pParams);
void __stdcall XInputClose(void *h);
u32 __stdcall XInputGetState(void *h, xb::_XINPUT_STATE *pState);
u32 __stdcall XInputSetState(void *h, xb::_XINPUT_FEEDBACK *pFeedback);
}
extern void *g_ExceptionPadHandle;

EXGamePad::~EXGamePad() {}

s8 EXGamePad::OpenPads() { return 1; }

// NEAR: retail clears m_Attached[0..3] with four byte stores; ours merges them into one dword.
EXGamePad::EXGamePad()
{
    memset(m_hPad, 0, sizeof(m_hPad));
    for (int i = 0; i < 4; i++) m_Attached[i] = 0;
    memset(m_Feedback, 0, sizeof(m_Feedback));
    m_PortName[0] = "0";
    m_PortName[1] = "1";
    m_PortName[2] = "2";
    m_PortName[3] = "3";
}

// NEAR: 7 bytes differ, retail pushes the handle after the first motor-speed store.
void EXGamePad::SetPadRumble(u8 pad)
{
    if (pad < 4 && m_RumbleEnabled[pad]) {
        xb::_XINPUT_FEEDBACK *pFeedback = &m_Feedback[pad];
        if (m_Feedback[pad].dwStatus != 0x3e5) {
            u16 hi = m_RumbleHi[pad] << 8;
            u16 lo = m_RumbleLo[pad] << 8;
            void *h = m_hPad[pad];
            if (h) {
                pFeedback->dwStatus = 0;
                m_Feedback[pad].wLeftMotorSpeed = lo;
                m_Feedback[pad].wRightMotorSpeed = hi;
                XInputSetState(h, pFeedback);
            }
        }
    }
}

// NEAR: register allocation differs (retail spills the device mask, keeps the port bit in ebp).
void EXGamePad::Update()
{
    u32 devices = XGetDevices(&XDEVICE_TYPE_GAMEPAD_TABLE);
    for (int i = 0; i < 4; i++) {
        s8 inserted = 0;
        RumbleUpdate(i);
        u32 bit = (1 << i) & devices;
        if (!bit) {
            if (m_hPad[i]) {
                XInputClose(m_hPad[i]);
                m_hPad[i] = 0;
                SetRemoved(i);
                m_Buttons[i] = 0;
                m_PrevButtons[i] = 0;
                m_ButtonsReleased[i] = 0;
                m_ButtonsPressed[i] = 0;
            }
        }
        if (bit) {
            if (!m_hPad[i]) {
                m_hPad[i] = XInputOpen(&XDEVICE_TYPE_GAMEPAD_TABLE, i, 0, 0);
                if (i == 0) g_ExceptionPadHandle = m_hPad[i];
                inserted = 1;
                SetInserted(i);
            }
        }
        if (m_hPad[i]) {
            xb::_XINPUT_STATE state;
            XInputGetState(m_hPad[i], &state);
            u32 packet = state.dwPacketNumber;
            if (!inserted && packet == m_PacketNumber[i]) {
                m_PrevButtons[i] = m_Buttons[i];
                m_ButtonsReleased[i] = 0;
                m_ButtonsPressed[i] = 0;
            } else {
                StorePadState(i, &state.Gamepad);
                m_PacketNumber[i] = packet;
            }
        }
    }
}

// Stick axes come in as -32768..32767, are scaled by sqrt(2) (so a full diagonal reaches the edge on
// both axes) and clamped to +-32768, then shaped by the pad's dead zone mode per stick:
//   0 radial: a 13107-unit circle, the rest of the magnitude rescaled to 0..1 (direction kept);
//   1 per axis: each axis zeroed within +-13107 and the rest rescaled (snaps toward the axes);
//   2 raw: just scaled to -1..1.
// Y comes out up-positive (negated).
static const float kPadDeadZone = 13107.0f;
static const float kPadRescale = 1.0f / (32768.0f - 13107.0f);

static inline float PadScaleAxis(s16 v)
{
    float f = (float)v * 1.4142135f;
    if (f > 32768.0f) f = 32768.0f;
    if (f < -32768.0f) f = -32768.0f;
    return f;
}

static inline float PadAxisDeadZone(float v)
{
    if (v > kPadDeadZone) return (v - kPadDeadZone) * kPadRescale;
    if (v < -kPadDeadZone) return (v + kPadDeadZone) * kPadRescale;
    return 0.0f;
}

static inline void PadStick(u8 mode, float x, float y, float &outX, float &outY)
{
    if (mode == 1) {
        outX = PadAxisDeadZone(x);
        outY = -PadAxisDeadZone(y);
    } else if (mode == 2) {
        outX = x * (1.0f / 32768.0f);
        outY = y * -(1.0f / 32768.0f);
    } else {
        float magSq = y * y + x * x;
        if (magSq <= kPadDeadZone * kPadDeadZone) {
            outX = 0.0f;
            outY = 0.0f;
            return;
        }
        float angle = atan2f(y, x);
        float mag = magSq < 0.0f ? 0.0f : sqrtf(magSq);
        float m = (mag - kPadDeadZone) * kPadRescale;
        if (m > 1.0f) m = 1.0f;
        outX = cosf(angle) * m;
        outY = -(sinf(angle) * m);
    }
}

// Converts one pad's raw XInput state into button bits, edge bits and analog channels.
// Buttons: d-pad 1/2/4/8; left stick pushed past 0x3333 down 0x20, up 0x10, left 0x40, right 0x80;
// right stick down 0x200, up 0x100, left 0x400, right 0x800; face buttons past 30/255 Y 0x1000,
// A 0x2000, X 0x4000, B 0x8000, white 0x200000, black 0x800000; triggers past half LT 0x100000,
// RT 0x400000; thumb clicks 0x10000/0x20000; start 0x40000, back 0x80000.
// Analog channels: 0/1 left stick, 2/3 right stick, 8..15 the pressure of Y, A, X, B, LT, white,
// RT, black (0..1; face buttons only once past 30, triggers always).
void EXGamePad::StorePadState(u8 pad, xb::_XINPUT_GAMEPAD *pState)
{
    if (pad > 3) return;
    m_PrevButtons[pad] = m_Buttons[pad];
    float *pAnalog = m_Analog[pad].m_Data;
    for (int i = 0; i < 16; i++) pAnalog[i] = 0.0f;
    u32 b = 0;
    if (pState->sThumbLY < -0x3333) b = 0x20;
    if (pState->sThumbLY > 0x3333) b |= 0x10;
    if (pState->sThumbLX < -0x3333) b |= 0x40;
    if (pState->sThumbLX > 0x3333) b |= 0x80;
    if (pState->sThumbRY < -0x3333) b |= 0x200;
    if (pState->sThumbRY > 0x3333) b |= 0x100;
    if (pState->sThumbRX < -0x3333) b |= 0x400;
    if (pState->sThumbRX > 0x3333) b |= 0x800;
    u16 w = pState->wButtons;
    if (w & 1) b |= 1;
    if (w & 2) b |= 2;
    if (w & 4) b |= 4;
    if (w & 8) b |= 8;
    if (w & 0x40) b |= 0x10000;
    if (w & 0x80) b |= 0x20000;
    if (w & 0x10) b |= 0x40000;
    if (w & 0x20) b |= 0x80000;
    const u8 *pA = pState->bAnalogButtons;  // A, B, X, Y, black, white, LT, RT
    const float k = 1.0f / 255.0f;
    if (pA[3] > 30) {
        b |= 0x1000;
        pAnalog[8] = pA[3] * k;
    }
    if (pA[0] > 30) {
        b |= 0x2000;
        pAnalog[9] = pA[0] * k;
    }
    if (pA[2] > 30) {
        b |= 0x4000;
        pAnalog[10] = pA[2] * k;
    }
    if (pA[1] > 30) {
        b |= 0x8000;
        pAnalog[11] = pA[1] * k;
    }
    if ((float)pA[6] > 127.0f) b |= 0x100000;
    pAnalog[12] = pA[6] * k;
    if ((float)pA[7] > 127.0f) b |= 0x400000;
    pAnalog[14] = pA[7] * k;
    if (pA[5] > 30) {
        b |= 0x200000;
        pAnalog[13] = pA[5] * k;
    }
    if (pA[4] > 30) {
        b |= 0x800000;
        pAnalog[15] = pA[4] * k;
    }
    m_Buttons[pad] = b;
    u32 changed = m_PrevButtons[pad] ^ b;
    m_ButtonsPressed[pad] = changed & b;
    m_ButtonsReleased[pad] = changed & m_PrevButtons[pad];
    float lx = PadScaleAxis(pState->sThumbLX);
    float ly = PadScaleAxis(pState->sThumbLY);
    float rx = PadScaleAxis(pState->sThumbRX);
    float ry = PadScaleAxis(pState->sThumbRY);
    PadStick(m_DeadZoneMode[pad][0], lx, ly, pAnalog[0], pAnalog[1]);
    PadStick(m_DeadZoneMode[pad][1], rx, ry, pAnalog[2], pAnalog[3]);
}
