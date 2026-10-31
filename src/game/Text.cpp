#include "FEText.h"
#include "EXMemory.h"
#include "EXNew.h"
#include "EXFile.h"
#include "EXFileSys.h"
#include "EXGeoFile.h"
#include "EXBaseDisplay.h"
#include "FEHud.h"
#include "XAppStandIn.h"
#include "ActorUtil.h"

// TODO stand-ins: other areas.
class FEObjectives {
public:
    static FEObjectives *Instance(signed char create);
    void Add(unsigned int hash, unsigned int text, int n);
};
extern EXFixedArray<unsigned char, 290> FlagChannels;

// Huffman tree node: a leaf has left < 0 and one (left == -1) or two characters in the bytes after it.
struct _HUFF {
    short left;
    union {
        short right;
        char ch[2];
    };
};

// Message slot (sizeof 0x2c).
struct TextInfo {
    short m_X;              // 0x00 size once measured (position for timers)
    short m_Y;              // 0x02
    s8 m_bSized;            // 0x04
    s8 m_bCutscene;         // 0x05 created during a cutscene
    int m_Unique;           // 0x08
    int m_Type;             // 0x0c 6 = free
    unsigned int m_Font;    // 0x10
    ex::RGBA m_Colour;      // 0x14
    EXString m_Text;        // 0x18
    EXW::TextAlign m_Align; // 0x1c
    float m_Timer;          // 0x20
    float m_TimerStart;     // 0x24
    int m_FlagChannel;      // 0x28 message dies when this flag channel is set
};

float TextGlobalAlpha = 1.0f;
TextInfo TextBuffer[32];
FEText gText;

unsigned short FEText::Decompress(_HUFF *pHuff, int *pBits, char *pOut, char &button)
{
    char *pStart = pOut;
    _HUFF *pNode = pHuff;
    button = 0;
    for (;;) {
        unsigned int bits = *pBits++;
        for (int i = 0; i < 32; i++) {
            if (bits & 1) pNode += pNode->right;
            else pNode += pNode->left;
            bits >>= 1;
            if (pNode->left >= 0) continue;
            char c;
            if (pNode->left < -1) {
                c = pNode->ch[1];
                if (c == (char)0xac) c = (char)0xa0;
                if (c == '~' && button == 0) button = c;
                else if (button == '~') button = c;
                *pOut++ = c;
            }
            c = pNode->ch[0];
            if (c == (char)0xac) c = (char)0xa0;
            if (c == '~' && button == 0) button = c;
            else if (button == '~') button = c;
            *pOut++ = c;
            if (c == 0) return (unsigned short)(pOut - pStart);
            pNode = pHuff;
        }
    }
}

// NEAR: register allocation (retail keeps this in edi, pIn in edx).
signed char FEText::Parse(char *pIn, char *pOut, unsigned short &len, char &button)
{
    *pOut = 0;
    int i = 0;
    int j = 0;
    signed char bGotButton = 0;
    if (pIn[0]) {
        do {
            if (pIn[i] == '~') {
                char c = 0;
                for (char t = '#'; t <= 'd'; t++) {
                    if (t == pIn[i + 1] && m_pButtons[t]->Graphic) {
                        c = t;
                        break;
                    }
                }
                if (m_pButtons[c]->Graphic->m_TextHash) {
                    const char *p = Text_Get(m_pButtons[c]->Graphic->m_TextHash);
                    while (*p) pOut[j++] = *p++;
                    i += 2;
                } else {
                    pOut[j] = '~';
                    pOut[j + 1] = pIn[i + 1];
                    if (!bGotButton) {
                        button = pIn[i + 1];
                        bGotButton = 1;
                    }
                    j += 2;
                    i += 2;
                }
            } else {
                pOut[j++] = pIn[i++];
            }
        } while (pIn[i]);
    }
    pOut[j] = 0;
    len = (unsigned short)j;
    return 1;
}

void Text_Initialise()
{
    memset(TextBuffer, 0, sizeof(TextBuffer));
    TextGlobalAlpha = 1.0f;
}

void Text_Print(short x, short y, EXString s, ex::RGBA col, unsigned int font, EXW::TextAlign align,
                unsigned short time, int type, int unique, int flags)
{
    if (!s.Length()) return;
    FEHud::Instance(1)->TextRect();
    int i;
    if (unique > -1) {
        for (i = 0; i < 32; i++) {
            if (TextBuffer[i].m_Unique == unique) {
                TextBuffer[i].m_X = x;
                TextBuffer[i].m_Y = y;
                TextBuffer[i].m_Type = type;
                TextBuffer[i].m_Text = s;
                TextBuffer[i].m_Colour = col;
                TextBuffer[i].m_Font = font;
                TextBuffer[i].m_Align = align;
                TextBuffer[i].m_TimerStart = TextBuffer[i].m_Timer = (float)time;
                TextBuffer[i].m_bCutscene = theApp.GetCutscene();
                TextBuffer[i].m_FlagChannel = flags;
                TextBuffer[i].m_bSized = 0;
                return;
            }
        }
    }
    for (i = 0; i < 32; i++) {
        if (TextBuffer[i].m_Timer <= 0.0f) {
            TextBuffer[i].m_X = x;
            TextBuffer[i].m_Y = y;
            TextBuffer[i].m_Type = type;
            TextBuffer[i].m_Text = s;
            TextBuffer[i].m_Colour = col;
            TextBuffer[i].m_Font = font;
            TextBuffer[i].m_Align = align;
            TextBuffer[i].m_TimerStart = TextBuffer[i].m_Timer = (float)time;
            TextBuffer[i].m_Unique = unique;
            TextBuffer[i].m_bCutscene = theApp.GetCutscene();
            TextBuffer[i].m_FlagChannel = flags;
            TextBuffer[i].m_bSized = 0;
            return;
        }
    }
}

void Text_PrintTimer(EXString s, ex::RGBA col)
{
    Text_Print(0, 0, s, col, 0x7000004, (EXW::TextAlign)4, 1, 0, 3, 0);
}

void Text_PrintStatus(EXString s, ex::RGBA col, int secs, int unique, signed char b, int flags)
{
    Text_Print(0, 0, s, col, 0x7000004, (EXW::TextAlign)4, (unsigned short)(EXBaseApp::NominalGameFrameRate() * secs), 1, unique, flags);
}

// NEAR: retail builds the grey RGBA in its argument slot (mov [esp], imm); ours pushes it.
void Text_PrintEnvironment(EXString s, ex::RGBA col, int secs, int unique, signed char b, int flags)
{
    int tmp27 = ex::RGBA(0x7f7f7f7f);
Text_Print(0, 0, s, tmp27, 0x7000004, (EXW::TextAlign)4, (unsigned short)(EXBaseApp::NominalGameFrameRate() * secs), 3, unique, flags);
}

// NEAR: as Text_PrintEnvironment.
void Text_PrintMsg(EXString s, ex::RGBA col, int secs, int unique, signed char b, int flags)
{
    int tmp27 = ex::RGBA(0x7f7f7f7f);
Text_Print(0, 0, s, tmp27, 0x7000004, (EXW::TextAlign)4, (unsigned short)(EXBaseApp::NominalGameFrameRate() * secs), 4, unique, flags);
}

void Text_DeleteMessage(TextInfo *p)
{
    if (p) {
        if (!p->m_Text.IsEmpty()) p->m_Text.Empty();
        p->m_Timer = 0.0f;
        p->m_Type = 6;
    }
}

void Text_Shutdown()
{
    for (int i = 0; i < 32; i++) Text_DeleteMessage(&TextBuffer[i]);
}

void Text_PrintObjective(unsigned int hash, ex::RGBA col, int secs, int unique, signed char b, int flags)
{
    if (unique == -1 && !b) {
        FEObjectives *pObjectives = FEObjectives::Instance(1);
        pObjectives->Add(hash, Text_Match(hash), 1);
    }
    EXString s = Text_Get(hash);
    Text_Print(0, 0, "OBJECTIVE:\n" + s, col, 0x7000004, (EXW::TextAlign)4, (unsigned short)(EXBaseApp::NominalGameFrameRate() * secs), 2, unique, flags);
}

void Text_DeleteMessages(int type)
{
    for (int i = 0; i < 32; i++)
        if (TextBuffer[i].m_Type == type) Text_DeleteMessage(&TextBuffer[i]);
}

void Text_DeleteUnique(int unique)
{
    for (int i = 0; i < 32; i++) {
        if (TextBuffer[i].m_Unique == unique) {
            Text_DeleteMessage(&TextBuffer[i]);
            return;
        }
    }
}

// NEAR: one Text_DeleteMessage expansion stays a call (inline budget); retail inlines all four.
void Text_Update()
{
    signed char cutscene = theApp.GetCutscene();
    int count = 0;
    if (theApp.GameStateCurrent()->m_State != 9) {
        TextGlobalAlpha = 0.0f;
        return;
    }
    if (TextGlobalAlpha < 1.0f) {
        TextGlobalAlpha += EXBaseApp::GameFrameRateMul() / EXBaseApp::FrameRate();
        TextGlobalAlpha = min(TextGlobalAlpha, 1.0f);
    }
    EXRect rc = FEHud::Instance(1)->TextRect();
    int y;
    if (cutscene) y = EXBaseDisplay::GetDisplay()->Height() / 6;
    else y = rc.y;
    for (int type = 0; type < 7; type++) {
        for (int i = 0; i < 32; i++) {
            if (TextBuffer[i].m_Type != type) continue;
            if (!(TextBuffer[i].m_Timer > 0.0f)) continue;
            if (!cutscene) {
                if (TextBuffer[i].m_bCutscene) {
                    Text_DeleteMessage(&TextBuffer[i]);
                    continue;
                }
            } else if (!TextBuffer[i].m_bCutscene) continue;
            if (TextBuffer[i].m_FlagChannel && FlagChannels[TextBuffer[i].m_FlagChannel]) {
                Text_DeleteMessage(&TextBuffer[i]);
                continue;
            }
            if (TextBuffer[i].m_TimerStart >= 32.0f && 32.0f > TextBuffer[i].m_Timer) TextBuffer[i].m_Colour.m_A = (u8)TextBuffer[i].m_Timer << 2;
            if (TextBuffer[i].m_Type < 5 && count < 1) {
                if (!TextBuffer[i].m_bSized && rc.w > 0) {
                    EXRect r(0, 0, rc.w, rc.h);
                    FEHud::Instance(1)->SelectFont(0x100003a, TextBuffer[i].m_Font);
                    EXPoint size = FEHud::Instance(1)->FontTextSize(TextBuffer[i].m_Text, r, 1.0f);
                    TextBuffer[i].m_X = (short)size.x;
                    TextBuffer[i].m_Y = (short)size.y;
                    TextBuffer[i].m_bSized = 1;
                }
                EXRect r;
                r.w = rc.w;
                r.h = TextBuffer[i].m_Y;
                r.x = rc.x + 1;
                r.y = y + 1;
                FEHud::Instance(1)->SelectFont(0x100003a, TextBuffer[i].m_Font);
                ex::RGBA shadow;
                shadow.m_R = 0;
                shadow.m_G = 0;
                shadow.m_B = 0;
                shadow.m_A = (u8)(TextBuffer[i].m_Colour.m_A * TextGlobalAlpha);
                FEHud::Instance(1)->SetFontColor(shadow);
                FEHud::Instance(1)->FontPrint(TextBuffer[i].m_Text, r, 1.0f, TextBuffer[i].m_Align, 1);
                r.x--;
                r.y--;
                ex::RGBA c;
                c.m_R = TextBuffer[i].m_Colour.m_R;
                c.m_G = TextBuffer[i].m_Colour.m_G;
                c.m_B = TextBuffer[i].m_Colour.m_B;
                c.m_A = (u8)(TextBuffer[i].m_Colour.m_A * TextGlobalAlpha);
                FEHud::Instance(1)->SetFontColor(c);
                FEHud::Instance(1)->FontPrint(TextBuffer[i].m_Text, r, 1.0f, TextBuffer[i].m_Align, 1);
                y += TextBuffer[i].m_Y;
                if (TextBuffer[i].m_Type > 1) count++;
                TextBuffer[i].m_Timer -= EXBaseApp::GameFrameRateMul();
                if (TextBuffer[i].m_Timer <= 0.0f) Text_DeleteMessage(&TextBuffer[i]);
            } else if (TextBuffer[i].m_Type == 5) {
                FEHud::Instance(1)->SelectFont(0x100003a, TextBuffer[i].m_Font);
                ex::RGBA shadow;
                shadow.m_R = 0;
                shadow.m_G = 0;
                shadow.m_B = 0;
                shadow.m_A = (u8)(TextBuffer[i].m_Colour.m_A * TextGlobalAlpha);
                FEHud::Instance(1)->SetFontColor(shadow);
                FEHud::Instance(1)->FontPrint(TextBuffer[i].m_X + 1, TextBuffer[i].m_Y + 1, TextBuffer[i].m_Text, 1.0f, TextBuffer[i].m_Align, 1);
                ex::RGBA c;
                c.m_R = TextBuffer[i].m_Colour.m_R;
                c.m_G = TextBuffer[i].m_Colour.m_G;
                c.m_B = TextBuffer[i].m_Colour.m_B;
                c.m_A = (u8)(TextBuffer[i].m_Colour.m_A * TextGlobalAlpha);
                FEHud::Instance(1)->SetFontColor(c);
                FEHud::Instance(1)->FontPrint(TextBuffer[i].m_X, TextBuffer[i].m_Y, TextBuffer[i].m_Text, 1.0f, TextBuffer[i].m_Align, 1);
                count++;
                TextBuffer[i].m_Timer -= EXBaseApp::GameFrameRateMul();
                if (TextBuffer[i].m_Timer <= 0.0f) Text_DeleteMessage(&TextBuffer[i]);
            }
        }
    }
}

FEText::FEText()
{
    m_Empty = "";
    m_NotFound = "HashCode not found";
    m_pMatches = 0;
    m_NumMatches = 0;
    m_pHuff = 0;
    m_pDir = 0;
    m_bLoaded = 0;
    m_pFile = 0;
    DumpData();
}

void FEText::Shutdown()
{
    DumpData();
    if (m_pMatches) {
        EXFree(m_pMatches);
        m_pMatches = 0;
    }
    if (m_pFile) EXFileSys::GetFileSys()->RemoveFile(m_pFile);
    m_bLoaded = 0;
}

// NEAR: follows HandleMemCompaction's NEAR (m_pData reload) and the LookupButton argument order.
signed char FEText::LoadLanguage()
{
    if (!m_bLoaded) {
        m_pFile = new EXFile("d:\\Predator\\Binary\\textoutN.bin", 0, 0);
        if (!EXFileSys::LoadFile(m_pFile)) return 0;
        m_bLoaded = 1;
        HandleMemCompaction();
        for (char c = '#'; c <= 'd'; c++)
            m_pButtons[c] = ((XGamePad *)EXBaseGamePad::GetGamePad())->LookupButton(c);
    }
    DumpData();
    m_Strings.SetSize(0x9b9, -1);
    if (m_pMatches) {
        EXFree(m_pMatches);
        m_pMatches = 0;
    }
    EXGeoFile::LoadGeoFile(0x1000076, 0, 0);
    void *pSheet = AU_GetWorkSheet(0x1000076, 0x14000018, 0, m_NumMatches);
    m_pMatches = new TextMatch[m_NumMatches];
    memcpy(m_pMatches, pSheet, m_NumMatches * sizeof(TextMatch));
    EXGeoFile::DeLoadGeoFile(0x1000076, 1);
    return 1;
}
