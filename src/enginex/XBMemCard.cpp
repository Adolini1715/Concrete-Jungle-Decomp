#include "EXMemCard.h"
#include "EXMemory.h"
#include "EXBaseApp.h"
#include "EXString.h"
#include <string.h>

// Xbox API (declared here from the map's names).
extern "C" {
int __stdcall CreateFileA(const char *pName, u32 access, u32 share, void *pSec, u32 disposition, u32 flags, void *hTemplate);
int __stdcall CloseHandle(int h);
int __stdcall ReadFile(int h, void *pBuf, u32 size, u32 *pRead, void *pOverlapped);
int __stdcall WriteFile(int h, const void *pBuf, u32 size, u32 *pWritten, void *pOverlapped);
u32 __stdcall GetFileSize(int h, u32 *pHigh);
u32 __stdcall GetLastError();
int __stdcall GetOverlappedResult(int h, void *pOverlapped, u32 *pTransferred, int wait);
int __stdcall GetDiskFreeSpaceExA(const char *pDir, void *pAvail, void *pTotal, void *pFree);
int __stdcall XGetDiskClusterSizeA(const char *pRoot);
int __stdcall XFindFirstSaveGame(const char *pRoot, void *pData);
int __stdcall XFindNextSaveGame(int h, void *pData);
int __stdcall XFindClose(int h);
u32 __stdcall XCreateSaveGame(const char *pRoot, const unsigned short *pName, u32 disposition, u32 flags, char *pPath, u32 size);
int __stdcall XCalculateSignatureBegin(u32 flags);
u32 __stdcall XCalculateSignatureUpdate(int h, const void *pData, u32 size);
u32 __stdcall XCalculateSignatureEnd(int h, void *pSig);
u32 __stdcall XLaunchNewImageA(const char *pImage, void *pData);
}

struct ULargeInteger {
    u32 m_Low;
    u32 m_High;
};

// XGAME_FIND_DATA: WIN32_FIND_DATAA, then the save's directory and name.
struct XGameFindData {
    u8 m_FindData[0x140];
    char m_Directory[0x104];
    unsigned short m_Name[0x80];
};

struct LaunchDashboard {
    u32 m_Reason;
    u32 m_Context;
    u32 m_Param1;
    u32 m_Param2;
    u8 m_Reserved[0xc00 - 0x10];
};

EXMemCard::EXMemCard()
{
    m_State = 0;
    m_Status = 0;
    m_114 = 0;
}

EXMemCard::~EXMemCard()
{
}

void GenerateSaveName(EXString *pName, int slot)
{
    *pName += ' ';
    if (slot == 0) *pName += 'A';
    else if (slot == 1) *pName += 'B';
    else if (slot == 2) *pName += 'C';
    else if (slot == 3) *pName += 'D';
    else if (slot == 4) *pName += 'E';
    else if (slot == 5) *pName += 'F';
    else if (slot == 6) *pName += 'G';
    else if (slot == 7) *pName += 'H';
}

int EXMemCard::XBGetFileSizeInBlocks(int n)
{
    char root[] = "U:\\";
    int cluster = XGetDiskClusterSizeA(root);
    if (cluster < 1) return 0;
    int clusters = (n + 0x200 + cluster - 1) / cluster;
    return ((clusters + 2) * cluster + 0x3fff) / 0x4000;
}

int EXMemCard::XBCheckHDForSpace(int n)
{
    ULargeInteger freeBytes, totalBytes, availBytes;
    GetDiskFreeSpaceExA("U:\\", &availBytes, &totalBytes, &freeBytes);
    return ((freeBytes.m_Low + 0x3fff) >> 14) - n;
}

s8 EXMemCard::Init(const EXSaveInfo &info)
{
    m_SaveInfo = info;
    return 1;
}

int EXMemCard::XBGetIOStatus()
{
    if (!GetOverlappedResult(m_hFile, &m_Overlapped, &m_Transferred, 0)) return GetLastError() == 0x3e4 ? 0 : 2;
    return 1;
}

int EXMemCard::Poll()
{
    switch (XBGetIOStatus()) {
    case 0:
        m_Status = 2;
        return 0;
    case 1:
        switch (m_State) {
        case 1:
            m_Status = 5;
            return 0;
        case 5:
            m_Status = 0x13;
            return 0;
        case 6:
            m_Status = 7;
            return 0;
        case 7:
            m_Status = 8;
            return 0;
        }
        break;
    case 2:
        m_Status = 0;
        CloseHandle(m_hFile);
        break;
    }
    return -4;
}

static unsigned short *ToWideChar(char *s)
{
    static unsigned short buf[0x100];
    memset(buf, 0, sizeof(buf));
    if (*s) {
        char *p = s;
        unsigned short *d = buf;
        do {
            if ((int)d >= (int)(buf + 0x80)) break;
            *d++ = *p & 0xff;
        } while (*++p);
    }
    return buf;
}

static char *FromWideChar(unsigned short *s)
{
    static char buf[0x100];
    memset(buf, 0, sizeof(buf));
    for (int i = 0; s[i] && i < 0x80; i++) buf[i] = (char)s[i];
    return buf;
}

void EXMemCard::XBResetToMemoryManger()
{
    LaunchDashboard ld;
    LaunchDashboard data;
    ld.m_Reason = 2;
    ld.m_Context = 0;
    ld.m_Param1 = 'U';
    ld.m_Param2 = m_BlocksNeeded;
    data = ld;
    XLaunchNewImageA(0, &data);
}

signed char EXMemCard::CheckHardDiskSpace()
{
    m_BlocksNeeded = XBGetFileSizeInBlocks(XBCalcPad512Size(m_SaveInfo.m_Size));
    m_BlocksFree = XBCheckHDForSpace(m_BlocksNeeded);
    return m_BlocksFree >= 0;
}

int EXMemCard::ProcessError(int flags)
{
    unsigned int r;
    switch (m_State) {
    case 10:
        if (DlgResult() == 1) {
            HideDlg();
            m_Status = 0;
            return -4;
        }
        break;
    case 14:
        r = DlgResult();
        if (r == 1) {
            HideDlg();
            m_Status = 0x10;
            return 0;
        }
        if (r == 2) {
            HideDlg();
            m_Status = 0;
            return -6;
        }
        break;
    case 18:
        r = DlgResult();
        if (r == 1) {
            HideDlg();
            m_Status = 0;
            return -1;
        }
        if (r == 2) {
            HideDlg();
            m_Status = 0;
            XBResetToMemoryManger();
            return -4;
        }
        break;
    case 17:
        SetDlgMessages("Not Enough Space on HD.Press A To Continue without saving, Press B to free more blocks", 0, 0, 0, 0, 0);
        SetDlgMemRequired(m_BlocksNeeded);
        SetDlgHashCodes(0x22020006, 2, 0, 0, 0, 0);
        ShowDlg(1, 0);
        m_State = 0x12;
        return 0;
    case 15:
        SetDlgMessages("Previous save data will be lost. Do You wish to continue with Overwrite?", 2, "Yes", "No", 0, 0);
        SetDlgHashCodes(0x22020003, 2, 0x22020001, 0x22020002, 0, 0);
        ShowDlg(2, 1);
        m_State = 14;
        return 0;
    case 13:
        SetDlgMessages("Unable To Save Game. Press A To Continue", 1, 0, 0, 0, 0);
        SetDlgHashCodes(0x22020004, 2, 0, 0, 0, 0);
        ShowDlg(1, 0);
        m_State = 10;
        return 0;
    case 11:
        if (flags & 2) {
            m_Status = 0;
            return -4;
        }
        SetDlgMessages("Unable To Load Save Game.The Save Game is damaged. Press A To Continue", 1, 0, 0, 0, 0);
        SetDlgHashCodes(0x22020005, 1, 0, 0, 0, 0);
        ShowDlg(1, 0);
        m_State = 10;
        break;
    default:
        return -4;
    }
    return 0;
}

int EXMemCard::CheckSavePossible(unsigned int flags)
{
    switch (m_Status) {
    case 0:
        if (CheckHardDiskSpace()) {
            m_Status = 0;
            return 1;
        }
        if (flags & 0x10000) {
            m_Status = 0;
            return -3;
        }
        m_State = 0x11;
        m_Status = 9;
        return 0;
    case 9:
        return ProcessError(flags);
    }
    return -4;
}

int EXMemCard::XBIsSaveGamePresent(int slot, char *pOut)
{
    XGameFindData fd;
    int h = XFindFirstSaveGame("U:\\", &fd);
    EXString name(EXBaseApp::GetApp()->AppName());
    char *pName;
    if (strlen(m_SaveInfo.m_Folder)) {
        pName = m_SaveInfo.m_Folder;
    } else {
        GenerateSaveName(&name, slot);
        pName = name.GetBuffer(0);
        name.ReleaseBuffer();
    }
    if (h != -1) {
        do {
            char *pFound = FromWideChar(fd.m_Name);
            if (!strcmp(pFound, pName)) {
                if (pOut) strcpy(pOut, strcat(fd.m_Directory, pFound));
                XFindClose(h);
                return 1;
            }
        } while (XFindNextSaveGame(h, &fd));
        XFindClose(h);
    }
    return 0;
}

int EXMemCard::SavesPresent(unsigned char &mask, unsigned int n)
{
    for (int i = 0; i < 8; i++) {
        if (XBIsSaveGamePresent(i, 0)) mask |= 1 << i;
    }
    return 1;
}

int EXMemCard::Load(unsigned char slot, void *p, unsigned int size, unsigned int flags)
{
    EXString name(EXBaseApp::GetApp()->AppName());
    char path[0x104 + 0x80];
    char *pName;
    if (strlen(m_SaveInfo.m_Folder)) {
        pName = m_SaveInfo.m_Folder;
    } else {
        GenerateSaveName(&name, slot);
        pName = name.GetBuffer(0);
        name.ReleaseBuffer();
    }
    switch (m_Status) {
    case 0:
        if (!XBIsSaveGamePresent(slot, path)) {
            m_State = 0xb;
            m_Status = 9;
            return 0;
        }
        m_hFile = CreateFileA(path, 0x80000000, 0, 0, 3, 0x60000000, 0);
        if (m_hFile == -1) {
            m_State = 0xb;
            m_Status = 9;
            return 0;
        }
        m_Overlapped.m_Offset = 0;
        m_Overlapped.m_OffsetHigh = 0;
        m_Overlapped.m_hEvent = 0;
        m_Status = 6;
        return 0;
    case 6: {
        int n = XBCalcPad512Size(m_SaveInfo.SlotSize(slot));
        if (n == 0) {
            CloseHandle(m_hFile);
            m_State = 0xb;
            m_Status = 9;
            return 0;
        }
        m_pBuffer = _EXAlloc(n, 0);
        if (!m_pBuffer) {
            CloseHandle(m_hFile);
            m_State = 0xb;
            m_Status = 9;
            return 0;
        }
        if (ReadFile(m_hFile, m_pBuffer, n, &m_BytesDone, &m_Overlapped)) {
            m_Status = 7;
            return 0;
        }
        if (GetLastError() != 0x3e5) {
            CloseHandle(m_hFile);
            EXFree(m_pBuffer);
            m_State = 0xb;
            m_Status = 9;
            return 0;
        }
        m_Status = 2;
        m_State = 6;
        return 0;
    }
    case 7: {
        m_hSignature = XCalculateSignatureBegin(0);
        if (m_hSignature == -1) {
            CloseHandle(m_hFile);
            m_State = 0xb;
            m_Status = 9;
            return 0;
        }
        int n = XBCalcPad512Size(m_SaveInfo.SlotSize(slot));
        XCalculateSignatureUpdate(m_hSignature, m_pBuffer, n);
        XCalculateSignatureEnd(m_hSignature, &m_Signature);
        m_Overlapped.m_Offset = n;
        m_pLoadHeader = _EXAlloc(0x200, 0);
        memset(m_pLoadHeader, 0, 0x200);
        if (ReadFile(m_hFile, m_pLoadHeader, 0x200, &m_BytesDone, &m_Overlapped)) {
            m_Status = 8;
            return 0;
        }
        if (GetLastError() != 0x3e5) {
            EXFree(m_pBuffer);
            EXFree(m_pLoadHeader);
            CloseHandle(m_hFile);
            m_State = 0xb;
            m_Status = 9;
            return 0;
        }
        m_Status = 2;
        m_State = 7;
        return 0;
    }
    case 8:
        if (!memcmp(m_pLoadHeader, &m_Signature, sizeof(m_Signature))) {
            char *q = (char *)m_pLoadHeader + sizeof(m_Signature);
            for (int i = 0; i < 0x1ec; i++, q++) {
                if (*q) goto bad;
            }
            EXFree(m_pLoadHeader);
            m_State = 8;
            m_Status = 0xc;
            return 0;
        }
    bad:
        EXFree(m_pBuffer);
        EXFree(m_pLoadHeader);
        CloseHandle(m_hFile);
        m_State = 0xb;
        m_Status = 9;
        return 0;
    case 12: {
        u32 fileSize = GetFileSize(m_hFile, 0);
        if (fileSize != (u32)(XBCalcPad512Size(m_SaveInfo.SlotSize(slot)) + 0x200)) {
            m_State = 0xb;
            m_Status = 9;
            CloseHandle(m_hFile);
            EXFree(m_pBuffer);
            return 0;
        }
        memcpy(p, m_pBuffer, m_SaveInfo.SlotSize(slot));
        EXFree(m_pBuffer);
        m_Status = 4;
        return 0;
    }
    case 4:
        m_Status = 0;
        CloseHandle(m_hFile);
        return 1;
    case 2:
        return Poll();
    case 9:
        return ProcessError(flags);
    }
    return -4;
}

int EXMemCard::Save(unsigned char slot, void *p, unsigned int flags)
{
    char root[] = "U:\\";
    EXString name(EXBaseApp::GetApp()->AppName());
    char path[0x100];
    char saveName[0x104 + 0x80];
    if (strlen(m_SaveInfo.m_Folder)) {
        strcpy(saveName, m_SaveInfo.m_Folder);
    } else {
        GenerateSaveName(&name, slot);
        strcpy(saveName, name.RawData());
    }
    switch (m_Status) {
    case 0:
        if (XBIsSaveGamePresent(slot, saveName)) {
            if (flags & 0x20) {
                m_Status = 0x10;
                return 0;
            }
            m_State = 0xf;
            m_Status = 9;
            return 0;
        }
        if (CheckHardDiskSpace()) {
            m_Status = 0x10;
            return 0;
        }
        SetDlgMemRequired(-m_BlocksFree);
        m_State = 0x11;
        m_Status = 9;
        return 0;
    case 0x10: {
        if (XCreateSaveGame(root, ToWideChar(saveName), 4, 0, path, 0x104)) break;
        strcpy(path + strlen(path), saveName);
        m_hFile = CreateFileA(path, 0x40000000, 0, 0, 2, 0x60000000, 0);
        if (m_hFile == -1) break;
        m_Overlapped.m_Offset = 0;
        m_Overlapped.m_OffsetHigh = 0;
        m_Overlapped.m_hEvent = 0;
        int n = XBCalcPad512Size(m_SaveInfo.SlotSize(slot));
        m_pBuffer = _EXAlloc(n, 0);
        if (!m_pBuffer) {
            CloseHandle(m_hFile);
            m_State = 0xd;
            m_Status = 9;
            return 0;
        }
        memcpy(m_pBuffer, p, n);
        m_Status = 0x15;
        m_Timer = 0xb4;
        if (!(flags & 0x80000)) {
            SetDlgMessages("SAVING GAME.PLEASE DON'T TURN OFF YOUR XBOX CONSOLE", 0, 0, 0, 0, 0);
            SetDlgHashCodes(0x22020007, 0, 0, 0, 0, 0);
            ShowDlg(0, 0);
        }
        return 0;
    }
    case 0x15:
        if (m_Timer-- < 0) {
            HideDlg();
            m_Status = 1;
        }
        return 0;
    case 1: {
        int n = XBCalcPad512Size(m_SaveInfo.SlotSize(slot));
        if (n == 0) {
            CloseHandle(m_hFile);
            EXFree(m_pBuffer);
            m_State = 0xd;
            m_Status = 9;
            return 0;
        }
        if (WriteFile(m_hFile, m_pBuffer, n, &m_BytesDone, &m_Overlapped)) {
            m_Status = 5;
            return 0;
        }
        if (GetLastError() == 0x3e5) {
            m_Status = 2;
            m_State = 1;
            return 0;
        }
        CloseHandle(m_hFile);
        EXFree(m_pBuffer);
        m_State = 0xd;
        m_Status = 9;
        return 0;
    }
    case 5:
        m_hSignature = XCalculateSignatureBegin(0);
        if (m_hSignature != -1) {
            int n = XBCalcPad512Size(m_SaveInfo.SlotSize(slot));
            m_Overlapped.m_Offset = n;
            memset(&m_Signature, 0, sizeof(m_Signature));
            XCalculateSignatureUpdate(m_hSignature, m_pBuffer, n);
            XCalculateSignatureEnd(m_hSignature, &m_Signature);
            m_pSaveHeader = _EXAlloc(0x200, 0);
            if (m_pSaveHeader) {
                memset(m_pSaveHeader, 0, 0x200);
                *(Signature *)m_pSaveHeader = m_Signature;
                if (WriteFile(m_hFile, m_pSaveHeader, 0x200, &m_BytesDone, &m_Overlapped)) {
                    m_Status = 0x13;
                    return 0;
                }
                if (GetLastError() != 0x3e5) {
                    EXFree(m_pSaveHeader);
                    EXFree(m_pBuffer);
                    CloseHandle(m_hFile);
                    m_State = 0xd;
                    m_Status = 9;
                    return 0;
                }
                m_Status = 2;
                m_State = 5;
                return 0;
            }
            CloseHandle(m_hFile);
            m_State = 0xd;
            m_Status = 9;
            return 0;
        }
        CloseHandle(m_hFile);
        EXFree(m_pBuffer);
        m_State = 0xd;
        m_Status = 9;
        return 0;
    case 0x13:
        if (flags & 0x20000) {
            HideDlg();
            m_Status = 4;
            return 0;
        }
        m_Status = 0x14;
        m_Timer = 0x78;
        if (!(flags & 0x80000)) {
            SetDlgMessages("GAME SAVED", 0, 0, 0, 0, 0);
            SetDlgHashCodes(0x22020008, 0, 0, 0, 0, 0);
            ShowDlg(0, 0);
        }
        return 0;
    case 0x14:
        if (m_Timer-- < 0) {
            HideDlg();
            m_Status = 4;
        }
        return 0;
    case 4:
        m_Status = 0;
        CloseHandle(m_hFile);
        EXFree(m_pBuffer);
        EXFree(m_pSaveHeader);
        return 1;
    case 2:
        return Poll();
    case 9:
        return ProcessError(flags);
    default:
        return -4;
    }
    m_State = 0xd;
    m_Status = 9;
    return 0;
}
