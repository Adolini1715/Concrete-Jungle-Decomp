#ifndef EXMEMCARD_H
#define EXMEMCARD_H
#include "types.h"
#include "EXRuntimeClass.h"
#include "EXWnd.h"

// Save-file description handed to the memory card (sizeof 0x64).
struct EXSaveInfo {
    u32 m_Version;          // 0x00 1
    u32 m_Size;             // 0x04 bytes in slot 0 (later slots follow)
    u32 m_SlotSize[7];      // 0x08
    char m_Folder[0x20];    // 0x24 save name ("Saved Games"; empty: app name and slot letter)
    char m_Name[0x20];      // 0x44 "Predator"
    u32 SlotSize(int slot) const { return (&m_Size)[slot]; }
};

// What a memory-card dialog shows (sizeof 0x38).
// m_Type: 1 one button per pad face button (A/B/X/Y), 2 a vertical list chosen with A.
struct EXMemCardDlgData {
    u32 m_Type;                 // 0x00
    unsigned int m_Message;     // 0x04 message hash (-1: use m_pText)
    unsigned int m_ButtonHash[4];   // 0x08
    u16 m_NumButtons;           // 0x18
    u16 m_Selected;             // 0x1a
    u32 m_MemRequired;          // 0x1c KB substituted for (mcKB)
    const char *m_pSlotName;    // 0x20 substituted for (mcSLOT)
    char *m_pText;              // 0x24 literal message text
    const char *m_pButton[4];   // 0x28
};

// Debug dialog window (sizeof 0x2b0).
class EXMemCardDlgWnd : public EXWnd {
public:
    EXMemCardDlgWnd() { m_Frame = 0; }
    virtual ~EXMemCardDlgWnd() {}
    virtual s8 Update();
    virtual s8 Draw();
    u32 m_Type;                 // 0x29c
    unsigned int m_NumButtons;  // 0x2a0
    int m_Selected;             // 0x2a4
    unsigned int m_Result;      // 0x2a8 1-based button chosen, 0 while open
    int m_Frame;                // 0x2ac
};

class EXMemCardDlg {
    EX_DECLARE_DYNAMIC(EXMemCardDlg)
public:
    static EXMemCardDlg *CreateObject();
    virtual ~EXMemCardDlg();
    virtual void ShowDlg(const EXMemCardDlgData &d);
    virtual void HideDlg();
    virtual unsigned int DlgResult();
    EXMemCardDlg();
    EXMemCardDlgWnd *m_pWnd;    // 0x04
};

struct MemCrdMsgData_tag {
    int m_Code;
    const char *m_pText;
};

// Platform-independent memory card (EXMemCard.obj, sizeof 0x48).
class EXBaseMemCard {
public:
    EXBaseMemCard();
    virtual ~EXBaseMemCard();
    virtual s8 Init(const EXSaveInfo &info);
    virtual int CardChanged(signed char &b);
    virtual int CheckSavePossible(unsigned int n);
    virtual int CheckLoadPossible(unsigned int n);
    virtual int SavesPresent(unsigned char &mask, unsigned int n);
    virtual int Save(unsigned char slot, void *p, unsigned int n);
    virtual int Load(unsigned char slot, void *p, unsigned int a, unsigned int b);
    virtual signed char CheckHardDiskSpace();
    virtual void SetSaveDirectory(const char *pDir) {}
    virtual void Update() {}
protected:
    virtual void ShowDlg(unsigned int type, unsigned int sel);
    virtual void HideDlg();
public:
    void SetMemCardDlgClass(EXRuntimeClass *p) { m_pDlgClass = p; }
    void StartMultiCardOp();
    void EndMultiCardOp();
    const char *Message(int code) const;
protected:
    s8 DlgVisible() const;
    unsigned int DlgResult();
    void SetDlgHashCodes(unsigned int msg, unsigned int n, unsigned int b0, unsigned int b1, unsigned int b2, unsigned int b3);
    void SetDlgMessages(const char *pMsg, unsigned int n, const char *b0, const char *b1, const char *b2, const char *b3);
    void SetDlgMemRequired(unsigned int kb) { m_DlgData.m_MemRequired = kb; }
public:
    EXRuntimeClass *m_pDlgClass;    // 0x04
    EXMemCardDlgData m_DlgData;     // 0x08
    EXMemCardDlg *m_pDlg;           // 0x40
    s8 m_bDlgLocked;                // 0x44
    s8 m_bMultiCardOp;              // 0x45
};

// Xbox memory card: saves to the hard disk (XBMemCard.obj, sizeof 0x11c).
class EXMemCard : public EXBaseMemCard {
public:
    EXMemCard();
    virtual ~EXMemCard();
    virtual s8 Init(const EXSaveInfo &info);
    virtual int CheckSavePossible(unsigned int n);
    virtual int SavesPresent(unsigned char &mask, unsigned int n);
    virtual int Save(unsigned char slot, void *p, unsigned int n);
    virtual int Load(unsigned char slot, void *p, unsigned int a, unsigned int b);
    virtual signed char CheckHardDiskSpace();
    int XBCalcPad512Size(int n)
    {
        int r = n & 0x1ff;
        if (r) return n - r + 0x200;
        return n;
    }
    int XBGetFileSizeInBlocks(int n);
    int XBCheckHDForSpace(int n);
    int XBGetIOStatus();
    int Poll();
    void XBResetToMemoryManger();
    int ProcessError(int flags);
    int XBIsSaveGamePresent(int slot, char *pName);
    u8 m_48[0x5c - 0x48];
    int m_State;                // 0x5c operation or error being handled (ProcessError)
    int m_Status;               // 0x60 step of the current Load/Save
    u32 m_BytesDone;            // 0x64
    int m_BlocksNeeded;         // 0x68 16K blocks a save takes
    int m_BlocksFree;           // 0x6c blocks left after it
    EXSaveInfo m_SaveInfo;      // 0x70
    void *m_pBuffer;            // 0xd4 padded copy of the save data
    int m_hFile;                // 0xd8
    struct Overlapped {
        u32 m_Internal;
        u32 m_InternalHigh;
        u32 m_Offset;
        u32 m_OffsetHigh;
        int m_hEvent;
    } m_Overlapped;             // 0xdc
    int m_hSignature;           // 0xf0
    struct Signature {
        u8 m_Data[0x14];
    } m_Signature;              // 0xf4
    void *m_pLoadHeader;        // 0x108
    void *m_pSaveHeader;        // 0x10c
    u32 m_Transferred;          // 0x110
    u32 m_114;
    int m_Timer;                // 0x118 frames left on the saving/saved dialog
};
#endif
