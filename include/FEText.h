#ifndef FETEXT_H
#define FETEXT_H
#include "types.h"

#include "EXBaseApp.h"

// Every front-end object (and App.obj, Text.obj) has a 16-byte $E6 storing
// NominalGameFrameRate() * 15 into a file static: a static defined in this header. Five $E numbers
// are used up before it. A global with an empty destructor (three: ctor, atexit stub, $S; the stubs
// fold away under the linker's ICF) and a folded const-float pair (two) reproduce that.
class FETextPad {
public:
    ~FETextPad() {}
};
static FETextPad s_FETextPad;
static const float s_FETextPadInit = 1.0f / 3.0f;
static float s_FETextPadF = s_FETextPadInit;
static u32 s_FETextTimeout = EXBaseApp::NominalGameFrameRate() * 15;

// ---- Localised string table (Text.obj) ----
#include "EXString.h"
#include "EXArray.h"
#include "RGBA.h"
#include "GameData.h"
#include "XGamePad.h"
#include "EXBaseWnd.h"
#include "EXFile.h"

class EXFile;
class EXGamePad;
struct _HUFF;

// One decoded string slot (sizeof 8).
struct TextEntry {
    TextEntry() { m_pString = 0; m_bDecoded = 0; m_Button = 0; }
    ~TextEntry() { delete m_pString; }
    EXString *m_pString;    // 0x00
    s8 m_bDecoded;          // 0x04
    char m_Button;          // 0x05 first button escape seen while parsing
};
// Hash remap pair from the language worksheet (sizeof 8).
struct TextMatch {
    TextMatch() { m_From = 0; m_To = 0; }
    unsigned int m_From;
    unsigned int m_To;
};
// Packed string directory entry in textoutN.bin (sizeof 6).
struct TextDirEntry {
    u16 m_Hash;         // low 16 bits of the text hash
    short m_Offset;     // in dwords from the bit stream start
    u16 m_Length;       // bit 15: contains button escapes
};

class FEText {
public:
    FEText();
    ~FEText() {}
    unsigned short Decompress(_HUFF *pHuff, int *pBits, char *pOut, char &button);
    signed char Parse(char *pIn, char *pOut, unsigned short &len, char &button);
    void DumpData()
    {
        if (m_Strings.m_pData) {
            DestructElements(m_Strings.m_pData, m_Strings.m_nSize);
            EXFree(m_Strings.m_pData);
            m_Strings.m_pData = 0;
        }
        m_Strings.m_nMaxSize = 0;
        m_Strings.m_nSize = 0;
    }
    void Shutdown();
    signed char LoadLanguage();
    // NEAR (App.obj copy): retail reloads m_pData for the final add only (7 bytes).
    void HandleMemCompaction()
    {
        if (m_bLoaded) {
            u32 *p = (u32 *)m_pFile->m_pMem;
            m_pData = p;
            m_NumDir = p[0];
            m_pHuff = (_HUFF *)(p + 2);
            m_pDir = (TextDirEntry *)(p[1] + (u8 *)m_pData);
        }
    }
    unsigned int RemapHash(unsigned int hash, char button);
    EXString &Get(unsigned int hash);
    unsigned int Match(unsigned int hash, signed char bForward);

    EXArray<TextEntry, TextEntry &> m_Strings;  // 0x00
    s8 m_bLoaded;               // 0x14
    EXFile *m_pFile;            // 0x18
    u32 *m_pData;               // 0x1c file image
    _HUFF *m_pHuff;             // 0x20
    TextDirEntry *m_pDir;       // 0x24
    int m_NumDir;               // 0x28
    TextMatch *m_pMatches;      // 0x2c
    int m_NumMatches;           // 0x30
    EXString m_Empty;           // 0x34
    EXString m_NotFound;        // 0x38
    XButton *m_pButtons[128];   // 0x3c indexed by escape character
    u32 m_Unk23c;               // 0x23c TODO
};
extern FEText gText;

inline unsigned int FEText::RemapHash(unsigned int hash, char button)
{
    switch (hash) {
    case 0x51000596:
        switch (GameData::Instance(1)->Values()->m_ButtonConfig) {
        case 0: hash = 0x51000886; break;
        case 1: hash = 0x51000841; break;
        case 2: hash = 0x51000842; break;
        }
        break;
    case 0x5100057c:
        if (((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(button)->Graphic->m_bAnalogue) hash = 0x51000881;
        break;
    case 0x51000588:
        if (((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(button)->Graphic->m_bAnalogue) hash = 0x51000883;
        break;
    case 0x51000584:
        if (((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(button)->Graphic->m_bAnalogue) hash = 0x51000882;
        break;
    case 0x510005a2:
        if (((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(button)->Graphic->m_bAnalogue) hash = 0x51000887;
        break;
    case 0x510005a6:
        if (((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(button)->Graphic->m_bAnalogue) hash = 0x51000888;
        break;
    case 0x510005a8:
        if (((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(button)->Graphic->m_bAnalogue) hash = 0x51000889;
        break;
    }
    return hash;
}

// NEAR (App.obj copy): retail does not keep the entry offset in ebx (one register fewer).
inline EXString &FEText::Get(unsigned int hash)
{
    int len;
    char buf[0x1000];
    while (m_bLoaded) {
        if (hash == 0x5100005a) break;
        if (hash <= 0x51000000) break;
        if (hash >= 0x510009b9) break;
        if (!m_Strings[hash - 0x51000000].m_bDecoded) {
            TextDirEntry *p = m_pDir;
            for (int i = 0; i < m_NumDir; p++, i++) {
                if (p->m_Hash == (u16)hash) {
                    len = p->m_Length & 0x7fff;
                    m_Strings[hash - 0x51000000].m_pString = new EXString;
                    m_Strings[hash - 0x51000000].m_Button = 0;
                    if (p->m_Length & 0x8000) {
                        Decompress(m_pHuff, (int *)&m_pData[p->m_Offset], buf, m_Strings[hash - 0x51000000].m_Button);
                        TextEntry &e = m_Strings[hash - 0x51000000];
                        Parse(buf, e.m_pString->GetBuffer((u16)(len + 0x80)), (unsigned short &)len, e.m_Button);
                        m_Strings[hash - 0x51000000].m_pString->GetBuffer((u16)(len + 1));
                    } else {
                        Decompress(m_pHuff, (int *)&m_pData[p->m_Offset],
                                   m_Strings[hash - 0x51000000].m_pString->GetBuffer((u16)len),
                                   m_Strings[hash - 0x51000000].m_Button);
                    }
                    m_Strings[hash - 0x51000000].m_pString->ReleaseBuffer();
                    break;
                }
            }
            if (!m_Strings[hash - 0x51000000].m_pString)
                m_Strings[hash - 0x51000000].m_pString = new EXString("");
            m_Strings[hash - 0x51000000].m_bDecoded = 1;
        }
        unsigned int remap = RemapHash(hash, m_Strings[hash - 0x51000000].m_Button);
        if (remap == hash) return *m_Strings[hash - 0x51000000].m_pString;
        hash = remap;
    }
    return m_Empty;
}

inline unsigned int FEText::Match(unsigned int hash, signed char bForward)
{
    for (int i = 0; i < m_NumMatches; i++) {
        if (bForward) {
            if (m_pMatches[i].m_From == hash) return m_pMatches[i].m_To;
        } else {
            if (m_pMatches[i].m_To == hash) return m_pMatches[i].m_From;
        }
    }
    return hash;
}

inline void Text_LoadLanguage() { gText.LoadLanguage(); }
inline EXString Text_Get(unsigned int hash) { return gText.Get(hash); }
inline unsigned int Text_Match(unsigned int hash)
{
    for (int i = 0; i < gText.m_NumMatches; i++)
        if (gText.m_pMatches[i].m_From == hash) return gText.m_pMatches[i].m_To;
    return hash;
}
inline unsigned int Text_MatchRev(unsigned int hash)
{
    for (int i = 0; i < gText.m_NumMatches; i++)
        if (gText.m_pMatches[i].m_To == hash) return gText.m_pMatches[i].m_From;
    return hash;
}

struct TextInfo;
void Text_Initialise();
void Text_Print(short x, short y, EXString s, ex::RGBA col, unsigned int font, EXW::TextAlign align,
                unsigned short time, int type, int unique, int flags);
void Text_PrintTimer(EXString s, ex::RGBA col);
void Text_PrintStatus(EXString s, ex::RGBA col, int secs, int unique, signed char b, int flags);
void Text_PrintEnvironment(EXString s, ex::RGBA col, int secs, int unique, signed char b, int flags);
void Text_PrintMsg(EXString s, ex::RGBA col, int secs, int unique, signed char b, int flags);
void Text_PrintObjective(unsigned int hash, ex::RGBA col, int secs, int unique, signed char b, int flags);
void Text_DeleteMessage(TextInfo *p);
void Text_DeleteMessages(int type);
void Text_DeleteUnique(int unique);
void Text_Shutdown();
void Text_Update();
#endif
