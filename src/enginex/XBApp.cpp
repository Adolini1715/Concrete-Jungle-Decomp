#include "EXApp.h"
#include "EXMemory.h"
#include "EXStdFuncs.h"
#include "EXDisplay.h"
#include "EXRunFirst.h"
#include "EXStats.h"
#include "XBBaseBenchMark.h"
#include "XBFileManager.h"

// System entry points (our own prototypes).
extern "C" {
double __cdecl ceil(double);
int __cdecl vsprintf(char *, const char *, char *);
int __cdecl sprintf(char *, const char *, ...);
void __cdecl _splitpath(const char *, char *, char *, char *, char *);
struct _iobuf;
_iobuf *__cdecl fopen(const char *, const char *);
unsigned int __cdecl fwrite(const void *, unsigned int, unsigned int, _iobuf *);
int __cdecl fclose(_iobuf *);
void __stdcall DebugBreak();
struct _SYSTEMTIME {
    u16 wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
};
void __stdcall GetLocalTime(_SYSTEMTIME *p);
int __stdcall QueryPerformanceFrequency(s64 *pFreq);
unsigned int __stdcall XGetLaunchInfo(u32 *pType, EXLaunchData *pData);
unsigned int __stdcall XLaunchNewImageA(const char *pPath, EXLaunchData *pData);
struct _XDEVICE_PREALLOC_TYPE {
    const void *DeviceType;
    u32 dwPreallocCount;
};
void __stdcall XInitDevices(u32 n, _XDEVICE_PREALLOC_TYPE *p);
extern const u8 XDEVICE_TYPE_GAMEPAD_TABLE[];
extern const u8 XDEVICE_TYPE_MEMORY_UNIT_TABLE[];
xb::Direct3D *__stdcall Direct3DCreate8(unsigned int version);
}
namespace xb {
enum _D3DCALLBACKTYPE { D3DCALLBACK_READ = 0 };
}
extern "C" void __stdcall D3DDevice_InsertCallback(xb::_D3DCALLBACKTYPE type, void(__cdecl *f)(unsigned long), unsigned long ctx);
static unsigned long __stdcall Direct3D_Release() { return 1; }
namespace xb {
struct Direct3D {
    static unsigned long __stdcall Release() { return Direct3D_Release(); }
};
struct D3DDevice {
    static void __stdcall InsertCallback(_D3DCALLBACKTYPE type, void(__cdecl *f)(unsigned long), unsigned long ctx)
    {
        D3DDevice_InsertCallback(type, f, ctx);
    }
};
}
extern "C" inline float __cdecl ceilf(float x) { return (float)ceil((double)x); }
inline float __cdecl ceil(float x) { return ceilf(x); }

void XbOutputDebugString(char *s);

// TODO stand-ins: font and shadow singletons (XBFont.obj / XBShadow.obj, render area).
class _XBFont {
public:
    void XBInitFont();
    void XBRenderFont();
};
extern _XBFont XBFont;
class XBShadow {
public:
    void Init();
};
extern XBShadow XBShadowController;

static char s_DebugBuffer[0x200];
int ResetDelay;
void *g_ExceptionPadHandle;
static char s_ELBuffer[0x200];

extern "C" void MapFileLabelAnchor()
{
    __asm nop
}

EXApp::EXApp()
{
    g_EXRunFirstVar = 0;
    m_pD3D = 0;
}

EXApp::~EXApp()
{
}

s8 EXApp::InitHardware()
{
    m_pD3D = Direct3DCreate8(0);
    if (!m_pD3D) return 0;
    s64 freq;
    if (!QueryPerformanceFrequency(&freq))
        for (;;) {
        }
    m_TicksPerFrame = (u32)(freq / 60);
    _XDEVICE_PREALLOC_TYPE devs[2] = {{XDEVICE_TYPE_GAMEPAD_TABLE, 4}, {XDEVICE_TYPE_MEMORY_UNIT_TABLE, 8}};
    XInitDevices(2, devs);
    return 1;
}

s8 EXApp::ShutdownHardware()
{
    if (m_pD3D) {
        m_pD3D->Release();
        m_pD3D = 0;
    }
    return 1;
}

void ResetToGame()
{
    u32 type;
    union {
        s64 m_Align;
        EXLaunchData m_Data;
    } data;
    XGetLaunchInfo(&type, &data.m_Data);
    if (!*(char *)&data)
        strcpy((char *)&data, "viewer");
    else
        memset(&data, 0, sizeof(data));
    EXString path("D:\\");
    path += EXBaseApp::GetApp()->AppName();
    path += ".xbe";
    XLaunchNewImageA(path, &data.m_Data);
}

void EXApp::ReturnToLauncher()
{
    XLaunchNewImageA(m_LaunchData.m_LauncherPath, &m_LaunchData);
}

// NEAR: the `1.0f >= rate` early return gets its own epilogue instead of jumping to the shared one
// (and the rate store/reload around it differs); body otherwise identical.
s8 EXApp::MainUpdate()
{
    static float s_FrameRate = 60.0f;
    EXDisplay *pDisplay = (EXDisplay *)EXBaseDisplay::GetDisplay();
    EXPresentParams params = pDisplay->Params();
    pDisplay->EnableInvalidateWriteCache();
    float rate;
    if (params.m_VSync == 1) {
        if (((EXDisplay *)EXBaseDisplay::GetDisplay())->GetVerticalBlanks() <
            (unsigned int)ceil((float)((EXDisplay *)EXBaseDisplay::GetDisplay())->GetPresentParameters()->FullScreen_RefreshRateInHz / (float)m_MaxFrameRate))
            return 1;
        unsigned int vb = ((EXDisplay *)EXBaseDisplay::GetDisplay())->GetAndResetVerticalBlanks();
        rate = (float)((EXDisplay *)EXBaseDisplay::GetDisplay())->GetPresentParameters()->FullScreen_RefreshRateInHz / (float)vb;
        if (1.0f >= rate) return 1;
    } else {
        rate = (float)(unsigned int)XBBenchMark.m_FrameRate;
    }
    XBBenchMark.m_Unk64 = (int)rate;
    XBBenchMark.StartCPUTimer();
    xb::D3DDevice::InsertCallback(xb::D3DCALLBACK_READ, (void(__cdecl *)(unsigned long))StartGPUTimer, 0);
    Update();
    g_Stats.ResetDisplayLine();
    g_Stats.FrameUpdate(1);
    Render();
    float minRate = (float)m_MinFrameRate;
    float target = minRate > rate ? minRate : rate;
    rate = (target - s_FrameRate) * 0.5f + s_FrameRate;
    s_FrameRate = rate;
    unsigned int frameRate = (unsigned int)ceil(rate);
    UpdateFrameRate(frameRate);
    XBBenchMark.m_Unk44 = frameRate;
    xb::D3DDevice::InsertCallback(xb::D3DCALLBACK_READ, (void(__cdecl *)(unsigned long))StopGPUTimer, 0);
    XBBenchMark.DisplayGPUTimer();
    XBBenchMark.StopCPUTimer();
    XBBenchMark.DisplayCPUTimer();
    XBFont.XBRenderFont();
    EXBaseDisplay::GetDisplay()->Present();
    return 1;
}

s8 EXApp::GetSystemTime(EXAppTime &t)
{
    _SYSTEMTIME st;
    GetLocalTime(&st);
    t.SetSeconds(st.wSecond);
    t.SetMinutes(st.wMinute);
    t.SetHours(st.wHour);
    t.SetDay(st.wDay);
    t.SetMonth(st.wMonth);
    t.SetYear(st.wYear);
    return 1;
}

static int main2()
{
    EXBaseApp *pApp = EXBaseApp::GetApp();
    if (!pApp || !pApp->InitHardware() || !pApp->InitInstance()) return 0;
    if (!pApp->InitGame()) {
        pApp->ExitInstance();
        return 0;
    }
    XBFont.XBInitFont();
    XBShadowController.Init();
    while (pApp->GetAppState() == EXBaseApp::AS_RUNNING && pApp->MainUpdate()) gFileManager.HandleDiskErrors();
    pApp->ExitGame();
    int ret = pApp->ExitInstance();
    pApp->ShutdownHardware();
    if (pApp->GetAppState() == EXBaseApp::AS_RESTART) ResetToGame();
    return ret;
}

void __cdecl main()
{
    u32 type;
    union {
        s64 m_Align;
        EXLaunchData m_Data;
    } data;
    memset(&data, 0, sizeof(data));
    XGetLaunchInfo(&type, &data.m_Data);
    main2();
    for (;;) {
    }
}

namespace ex {
double fd(float f) { return f; }
float df(double d) { return (float)d; }
void EXDebugBreak() { DebugBreak(); }

void EXELSendMessage(const char *fmt, ...)
{
    int len = vsprintf(s_ELBuffer, fmt, (char *)(&fmt + 1));
    if (len > 0x200) {
        XbOutputDebugString("String Length Exceeded in EXELSendMessage Call\n");
        return;
    }
    _iobuf *f = fopen("\\\\.\\pipe\\XLink", "wb");
    if (f) {
        fwrite(s_ELBuffer, len, 1, f);
        fclose(f);
    }
}
}

// NEAR: the launch-data-valid store is sunk below the path copy (orig stores it first).
void EXApp::InitMagDemo()
{
    char drive[4];
    char path[64];
    char dir[256];
    char ext[256];
    char fname[256];
    XGetLaunchInfo(&m_LaunchDataType, &m_LaunchData);
    if (m_LaunchDataType == 0) {
        SetLaunchDataValid(1);
        memcpy(path, m_LaunchData.m_LaunchedPath, sizeof(path));
        _splitpath(path, drive, dir, fname, ext);
        EXString root(drive);
        EXString sdir(dir);
        root += sdir;
        char *src = root.m_pchData;
        memcpy(&m_DataRoot[0], src, root.Length());
        m_DemoTimeout = m_LaunchData.m_Timeout;
    } else {
        SetLaunchDataValid(0);
        memset(m_LaunchData.m_LaunchedPath, 0, sizeof(m_LaunchData.m_LaunchedPath));
        strcpy(m_LaunchData.m_LaunchedPath, "D:\\");
    }
}

namespace ex {
void EXDebugPrintFatal(const char *fmt, ...)
{
    if ((unsigned int)vsprintf(s_DebugBuffer, fmt, (char *)(&fmt + 1)) > 0x200) {
        XbOutputDebugString("String Length Exceeded in XDebugMessage Call\n");
        DebugBreak();
    }
    XbOutputDebugString(s_DebugBuffer);
}

void EXDebugPrint(const char *fmt, ...)
{
    if ((unsigned int)vsprintf(s_DebugBuffer, fmt, (char *)(&fmt + 1)) > 0x200) {
        XbOutputDebugString("String Length Exceeded in XDebugMessage Call\n");
        DebugBreak();
    }
    XbOutputDebugString(s_DebugBuffer);
}

void EXDebugPrint(const unsigned short *fmt, ...)
{
    char buf[0x200];
    if (Wcslen(fmt) + 1 > 0x200) {
        XbOutputDebugString("String Length Exceeded in XDebugMessage Call\n");
        DebugBreak();
    }
    int i = 0;
    const char *c = (const char *)fmt;
    const unsigned short *p = fmt;
    do {
        buf[i++] = *c;
        p++;
        c = (const char *)p;
    } while (*p);
    buf[i] = 0;
    if ((unsigned int)vsprintf(s_DebugBuffer, buf, (char *)(&fmt + 1)) > 0x200) {
        XbOutputDebugString("String Length Exceeded in XDebugMessage Call\n");
        DebugBreak();
    }
    XbOutputDebugString(s_DebugBuffer);
}
}

extern EXFixedArray<const char *, 16> g_DebugMessagesStart;
extern EXFixedArray<const char *, 16> g_DebugMessagesEnd;
extern EXFixedArray<signed char, 2> g_bTraceInfo;
extern EXFixedArray<signed char, 2> g_bTraceShort;

void EXDebugPrintNew(ex::EXErrorType type, ex::EXErrorLocation loc, const char *fmt, char *&args)
{
    if (loc < ex::EL_ENGINE || loc > ex::EL_GAME) {
        XbOutputDebugString("Invalid Debug Message Location\n");
        DebugBreak();
        return;
    }
    switch (type) {
    case ex::ET_INFO:
        if (!g_bTraceInfo[loc]) return;
        break;
    case ex::ET_WARNING:
    case ex::ET_ERROR:
    case ex::ET_FATAL:
        break;
    default:
        XbOutputDebugString("Invalid Debug Message Type\n");
        DebugBreak();
        return;
    }
    unsigned int idx = type + loc * 4;
    if (g_bTraceShort[loc]) idx += 8;
    char *p = s_DebugBuffer + sprintf(s_DebugBuffer, "%s ", g_DebugMessagesStart[idx]);
    p += vsprintf(p, fmt, args);
    int nl = 0;
    for (;;) {
        char c = *--p;
        if (c == '\n')
            nl++;
        else if (c != '\r')
            break;
    }
    p++;
    if ((unsigned int)(nl + (p - s_DebugBuffer) + 5) > 0x200) {
        XbOutputDebugString("String Length Exceeded in XDebugMessage Call\n");
        DebugBreak();
        return;
    }
    if (nl == 0) {
        sprintf(p, "%s ", g_DebugMessagesEnd[idx]);
    } else {
        p += sprintf(p, "%s", g_DebugMessagesEnd[idx]);
        while (nl--) *p++ = '\n';
        *p = 0;
    }
    XbOutputDebugString(s_DebugBuffer);
}

void EXDebugPrintNew(ex::EXErrorType type, ex::EXErrorLocation loc, const unsigned short *wfmt, char *&args)
{
    char fmt[0x200];
    if (Wcslen(wfmt) + 1 > 0x200) {
        XbOutputDebugString("String Length Exceeded in XDebugMessage Call\n");
        DebugBreak();
        return;
    }
    int i = 0;
    while ((fmt[i] = (char)wfmt[i]) != 0) i++;
    if (loc < ex::EL_ENGINE || loc > ex::EL_GAME) {
        XbOutputDebugString("Invalid Debug Message Location\n");
        DebugBreak();
        return;
    }
    switch (type) {
    case ex::ET_INFO:
        if (!g_bTraceInfo[loc]) return;
        break;
    case ex::ET_WARNING:
    case ex::ET_ERROR:
    case ex::ET_FATAL:
        break;
    default:
        XbOutputDebugString("Invalid Debug Message Type\n");
        DebugBreak();
        return;
    }
    unsigned int idx = type + loc * 4;
    if (g_bTraceShort[loc]) idx += 8;
    char *p = s_DebugBuffer + sprintf(s_DebugBuffer, "%s ", g_DebugMessagesStart[idx]);
    p += vsprintf(p, fmt, args);
    int nl = 0;
    for (;;) {
        char c = *--p;
        if (c == '\n')
            nl++;
        else if (c != '\r')
            break;
    }
    p++;
    if ((unsigned int)(nl + (p - s_DebugBuffer) + 5) > 0x200) {
        XbOutputDebugString("String Length Exceeded in XDebugMessage Call\n");
        DebugBreak();
        return;
    }
    if (nl == 0) {
        sprintf(p, "%s ", g_DebugMessagesEnd[idx]);
    } else {
        p += sprintf(p, "%s", g_DebugMessagesEnd[idx]);
        while (nl--) *p++ = '\n';
        *p = 0;
    }
    XbOutputDebugString(s_DebugBuffer);
}

namespace ex {
void EXDebugPrintEngine(EXErrorType type, const char *fmt, ...)
{
    char *args = (char *)(&fmt + 1);
    EXDebugPrintNew(type, EL_ENGINE, fmt, args);
}
void EXDebugPrintEngine(EXErrorType type, const unsigned short *fmt, ...)
{
    char *args = (char *)(&fmt + 1);
    EXDebugPrintNew(type, EL_ENGINE, fmt, args);
}
void EXDebugPrintGame(EXErrorType type, const char *fmt, ...)
{
    char *args = (char *)(&fmt + 1);
    EXDebugPrintNew(type, EL_GAME, fmt, args);
}
void EXDebugPrintGame(EXErrorType type, const unsigned short *fmt, ...)
{
    char *args = (char *)(&fmt + 1);
    EXDebugPrintNew(type, EL_GAME, fmt, args);
}
}
