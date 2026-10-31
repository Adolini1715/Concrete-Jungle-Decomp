#ifndef EXAPP_H
#define EXAPP_H
#include "EXBaseApp.h"

namespace xb { struct Direct3D; }

// Local calendar time (filled by EXApp::GetSystemTime).
class EXAppTime {
public:
    void SetSeconds(int n) { m_Seconds = (u8)n; }
    void SetMinutes(int n) { m_Minutes = (u8)n; }
    void SetHours(int n) { m_Hours = (u8)n; }
    void SetDay(int n) { m_Day = (u8)n; }
    void SetMonth(int n) { m_Month = (u8)n; }
    void SetYear(int n) { m_Year = (u16)n; }
    u16 m_Year;    // 0x00
    u8 m_Month;    // 0x02
    u8 m_Day;      // 0x03
    u8 m_Hours;    // 0x04
    u8 m_Minutes;  // 0x05
    u8 m_Seconds;  // 0x06
};

// Title launch data as returned by the launch-info query (0xc00 bytes). The demo-launcher
// layout is used by InitMagDemo / ReturnToLauncher. TODO names.
struct EXLaunchData {
    u32 m_Unk00;                // 0x00
    u32 m_Unk04;                // 0x04
    u32 m_Timeout;              // 0x08 copied to EXApp::m_DemoTimeout
    char m_LauncherPath[64];    // 0x0c image to return to
    char m_LaunchedPath[64];    // 0x4c path of this image
    u8 m_Unk8c[0xc00 - 0x8c];
};

// Xbox application (XBApp.obj): hardware init, the main frame loop and demo-launch support.
class EXApp : public EXBaseApp {
public:
    EXApp();
    virtual ~EXApp();
    xb::Direct3D *Direct3D() { return m_pD3D; }
    virtual s8 InitHardware();
    virtual s8 ShutdownHardware();
    virtual s8 MainUpdate();
    void ReturnToLauncher();
    s8 GetSystemTime(EXAppTime &t);
    void InitMagDemo();
    void SetLaunchDataValid(s8 b) { m_bLaunchDataValid = b; }
protected:
    xb::Direct3D *m_pD3D;           // 0x68
    u32 m_Unk6c;                    // 0x6c TODO
    u32 m_TicksPerFrame;            // 0x70 performance-counter ticks per 1/60 s
    u32 m_Unk74[3];                 // 0x74 TODO
    EXLaunchData m_LaunchData;      // 0x80
    s8 m_bLaunchDataValid;          // 0xc80
    u32 m_LaunchDataType;           // 0xc84
    u32 m_DemoTimeout;              // 0xc88 TODO name
};

namespace ex {
enum EXErrorType { ET_INFO = 0, ET_WARNING = 1, ET_ERROR = 2, ET_FATAL = 3 };
enum EXErrorLocation { EL_ENGINE = 0, EL_GAME = 1 };
void EXDebugPrint(const char *fmt, ...);
void EXDebugPrint(const unsigned short *fmt, ...);
void EXDebugPrintFatal(const char *fmt, ...);
void EXDebugPrintEngine(EXErrorType type, const char *fmt, ...);
void EXDebugPrintEngine(EXErrorType type, const unsigned short *fmt, ...);
void EXDebugPrintGame(EXErrorType type, const char *fmt, ...);
void EXDebugPrintGame(EXErrorType type, const unsigned short *fmt, ...);
void EXELSendMessage(const char *fmt, ...);
void EXDebugBreak();
double fd(float f);
float df(double d);
}
#endif
