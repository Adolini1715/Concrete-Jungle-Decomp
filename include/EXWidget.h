#ifndef EXWIDGET_H
#define EXWIDGET_H
#include "types.h"
#include "EXRect.h"
#include "EXArray.h"
#include "EXSprite2D.h"
#include "XRGBA.h"
#include "EXBaseWnd.h"
#include "EXBitFuncs.h"
#include "EXCircularBuffer.h"

// EngineX debug UI widgets (EXWidget.obj). Layouts recovered from the constructors.
class EXWidget;
typedef signed char (__cdecl *EXWidgetCallback)(EXWidget *, unsigned int);

enum EXWIDGETTYPE {
    EXWT_WINDOW = 0,
    EXWT_SCROLLV = 1,
    EXWT_SCROLLH = 2,
    EXWT_LISTBOX = 3,
    EXWT_SPINLIST = 4,
    EXWT_UNK5 = 5,
    EXWT_TEXTCMD = 6,
    EXWT_BUTTON = 7,
    EXWT_CHECKBOX = 8,
    EXWT_IMAGE = 9,
    EXWT_TEXT = 10,
    EXWT_PAGE = 11
};

// Callback / SendMsg notification codes seen in the code.
enum {
    EXWN_CREATE = 0,
    EXWN_DESTROY = 2,
    EXWN_MOVE = 3,
    EXWN_STATE = 8,
    EXWN_CLICK = 0x10
};

// A glyph set: sprite template plus minimum size (size 0x1c).
struct EXWIDGETINFO {
    EXWIDGETINFO(EXSprite2D *pSprites, float minw, float minh, unsigned int nSprites, unsigned int unk10,
                 unsigned int file, unsigned int id)
    {
        m_pSprites = pSprites;
        m_MinW = (int)minw;
        m_MinH = (int)minh;
        m_nSprites = nSprites;
        m_Unk10 = unk10;
        m_File = file;
        m_ID = id;
    }
    ~EXWIDGETINFO() {}
    EXSprite2D *m_pSprites;   // 0x00
    int m_MinW;               // 0x04
    int m_MinH;               // 0x08
    unsigned int m_nSprites;  // 0x0c
    unsigned int m_Unk10;     // 0x10
    unsigned int m_File;      // 0x14 geo file loaded by GetGlyph
    unsigned int m_ID;        // 0x18 lookup key
};

struct WIDGETSORTORDER {
    EXWidget *m_pWidget;  // TODO layout
    int m_Order;
};

class EXWidget {
protected:
    virtual unsigned short GetNumSprites() const { return 0; }
public:
    virtual ~EXWidget();
    virtual EXWIDGETTYPE Type() const = 0;
    virtual void SetRect(const EXRect &r)
    {
        m_Rect = r;
        m_ClientRect = r;
    }
    virtual void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }
    virtual EXRect GetRect() const { return m_Rect; }
    virtual EXRect FitInToRect(const EXRect &) const { return m_Rect; }
    virtual void Move(const EXPoint &p);
    virtual void SetClr(const ex::XRGBA &c);
    virtual void SetTextScale(float) {}
    virtual void SetTextClr(const ex::XRGBA &, const ex::XRGBA &, const ex::XRGBA &, const ex::XRGBA &) {}
    virtual void GetTextClr(ex::XRGBA &, ex::XRGBA &, ex::XRGBA &, ex::XRGBA &) const {}
    virtual void SetImage(unsigned int, unsigned int, float, float, float, float) {}
    virtual void SetFont(unsigned int, unsigned int) {}
    virtual void SetTextAlignment(EXW::TextAlign) {}
    virtual unsigned int GetTextAlignment() const { return 0; }
    virtual void SetCaption(const char *) {}
    virtual char *GetCaption() const { return 0; }
    virtual unsigned int HitTest(const EXPoint &p);
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    virtual void Draw() {}

    EXWidget(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyph, EXSprite2D *pSprites, unsigned short nSprites);
    unsigned int GetWID() const { return m_WID; }
    void SetUserData(int d) { m_UserData = d; }
    int GetUserData() const { return m_UserData; }
    void SetCallback(EXWidgetCallback cb) { m_pCallback = cb; }
    void SetPage(EXWidget *p) { m_pPage = p; }
    void SetFlag(unsigned short f, signed char b)
    {
        EXBitClear<unsigned short>(m_Flags, f);
        if (b) EXBitSet<unsigned short>(m_Flags, f);
    }
    signed char GetFlag(unsigned short f) const { return EXBitTest<unsigned short>(m_Flags, f) != 0; }
    void SetVisible(signed char b) { SetFlag(1, b); }
    signed char IsVisible() const { return m_Flags & 1; }
    EXRect GetClientRect() const { return EXRect(m_ClientRect.x, m_ClientRect.y, m_ClientRect.w, m_ClientRect.h); }
    EXPoint GetMinSize() const { return m_MinSize; }
    void Size(const EXPoint &d)
    {
        EXRect r = m_Rect;
        r.w = r.w + d.x;
        r.h = r.h + d.y;
        SetRect(r);
    }
    static signed char AddGlyphSet(unsigned int id, EXWIDGETINFO *pInfo, signed char bReplace);
    static void FreeGlyphs();
protected:
    void SelectFont(unsigned int file, unsigned int font) const;
    static EXWIDGETINFO *GetGlyph(unsigned int id, const EXWidget *pWidget);
    void SetUVs(unsigned int glyph, unsigned int &cur);
    void Notify(unsigned int msg)
    {
        if (m_pCallback) m_pCallback(this, msg);
    }
public:
    EXRect m_Rect;                 // 0x04
    EXRect m_ClientRect;           // 0x14
    EXPoint m_MinSize;             // 0x24
    ex::XRGBA m_Clr;               // 0x2c
    EXWidgetCallback m_pCallback;  // 0x30
    EXSprite2D *m_pSprites;        // 0x34
    EXBaseWnd *m_pWnd;             // 0x38
    EXWidget *m_pPage;             // 0x3c
    unsigned int m_WID;            // 0x40
    int m_UserData;                // 0x44
    unsigned short m_nSprites;     // 0x48
    unsigned short m_Flags;        // 0x4a bit0 visible, bit1 focusable(?)
};

class EXWidgetText : public EXWidget {
public:
    virtual EXWIDGETTYPE Type() const { return EXWT_TEXT; }
    virtual EXRect FitInToRect(const EXRect &r) const;
    virtual void SetTextScale(float s)
    {
        if (s >= 10.0f) s = 10.0f;
        else if (s <= 0.05f) s = 0.05f;
        m_Scale = s;
    }
    virtual void SetTextClr(const ex::XRGBA &a, const ex::XRGBA &b, const ex::XRGBA &c, const ex::XRGBA &d)
    {
        m_Clr = a;
        m_TextClr[0] = b;
        m_TextClr[1] = c;
        m_TextClr[2] = d;
    }
    virtual void GetTextClr(ex::XRGBA &a, ex::XRGBA &b, ex::XRGBA &c, ex::XRGBA &d) const
    {
        a = m_Clr;
        b = m_TextClr[0];
        c = m_TextClr[1];
        d = m_TextClr[2];
    }
    virtual void SetFont(unsigned int file, unsigned int font);
    virtual void SetTextAlignment(EXW::TextAlign a) { m_Align = a; }
    virtual unsigned int GetTextAlignment() const { return m_Align; }
    virtual void SetCaption(const char *s);
    virtual char *GetCaption() const { return m_Caption[0] ? (char *)m_Caption.m_Data : 0; }
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    virtual void Draw();
    EXWidgetText(EXBaseWnd *pWnd, unsigned int wid, const char *caption, unsigned int file, unsigned int font,
                 const ex::XRGBA &clr);
    unsigned int m_Font;                // 0x4c
    unsigned int m_FontFile;            // 0x50
    EXFixedArray<char, 256> m_Caption;  // 0x54
    unsigned int m_Align;               // 0x154
    ex::XRGBA m_TextClr[3];             // 0x158
    float m_Scale;                      // 0x164
};

class EXWidgetImage : public EXWidget {
protected:
    virtual unsigned short GetNumSprites() const { return 1; }
public:
    virtual EXWIDGETTYPE Type() const { return EXWT_IMAGE; }
    virtual void SetRect(const EXRect &r);
    virtual void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }
    virtual void SetImage(unsigned int file, unsigned int tex, float u0, float v0, float u1, float v1);
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    virtual void Draw();
    EXWidgetImage(EXBaseWnd *pWnd, unsigned int wid, unsigned int tex, unsigned int file, const EXRect &uv);
    unsigned int m_Tex;       // 0x4c
    unsigned int m_File;      // 0x50
    EXSprite2D m_Sprite;      // 0x54
};

class EXWidgetWindow : public EXWidget {
protected:
    virtual unsigned short GetNumSprites() const { return 10; }
public:
    virtual ~EXWidgetWindow() {}
    virtual EXWIDGETTYPE Type() const { return EXWT_WINDOW; }
    virtual void SetRect(const EXRect &r);
    virtual void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }
    virtual void Move(const EXPoint &p)
    {
        m_Text.Move(p);
        EXWidget::Move(p);
    }
    virtual void SetTextScale(float s) { m_Text.SetTextScale(s); }
    virtual void SetTextClr(const ex::XRGBA &a, const ex::XRGBA &b, const ex::XRGBA &c, const ex::XRGBA &d)
    {
        m_Text.SetTextClr(a, b, c, d);
    }
    virtual void GetTextClr(ex::XRGBA &a, ex::XRGBA &b, ex::XRGBA &c, ex::XRGBA &d) const
    {
        m_Text.GetTextClr(a, b, c, d);
    }
    virtual void SetImage(unsigned int file, unsigned int tex, float u0, float v0, float u1, float v1);
    virtual void SetFont(unsigned int file, unsigned int font) { m_Text.SetFont(file, font); }
    virtual void SetTextAlignment(EXW::TextAlign a) { m_Text.SetTextAlignment(a); }
    virtual unsigned int GetTextAlignment() const { return m_Text.GetTextAlignment(); }
    virtual void SetCaption(const char *s) { m_Text.SetCaption(s); }
    virtual char *GetCaption() const { return m_Text.GetCaption(); }
    virtual unsigned int HitTest(const EXPoint &p);
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    virtual void Draw();
    EXWidgetWindow(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyph);
    EXFixedArray<EXSprite2D, 10> m_Sprites;  // 0x4c
    EXWidgetText m_Text;                     // 0x164
    int m_Unk2cc;                            // 0x2cc
    int m_Unk2d0;                            // 0x2d0
    EXWIDGETINFO *m_pGlyph;                  // 0x2d4
};

class EXWidgetButton : public EXWidgetWindow {
public:
    virtual EXWIDGETTYPE Type() const { return EXWT_BUTTON; }
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    signed char GetState() const { return m_fPressed != 0.0f ? 1 : 0; }
    EXWidgetButton(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyphUp, unsigned int glyphDown);
    signed char SetState(signed char bDown, signed char bNotify);
    unsigned int m_GlyphUp;    // 0x2d8
    unsigned int m_GlyphDown;  // 0x2dc
    unsigned int m_CurGlyph;   // 0x2e0
    float m_fPressed;          // 0x2e4 press timer
};

class EXWidgetCheckBox : public EXWidgetWindow {
public:
    virtual ~EXWidgetCheckBox() {}
    virtual EXWIDGETTYPE Type() const { return EXWT_CHECKBOX; }
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    signed char GetState() const { return m_bState; }
    EXWidgetCheckBox(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyphOff, unsigned int glyphOn);
    signed char SetState(signed char bOn, signed char bNotify);
    unsigned int m_GlyphOff;  // 0x2d8
    unsigned int m_GlyphOn;   // 0x2dc
    unsigned int m_CurGlyph;  // 0x2e0
    signed char m_bState;     // 0x2e4
};

class EXWidgetScroll : public EXWidget {
public:
    virtual ~EXWidgetScroll() {}
    virtual EXWIDGETTYPE Type() const { return m_bVertical ? EXWT_SCROLLV : EXWT_SCROLLH; }
    virtual void SetRect(const EXRect &r);
    virtual void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }
    virtual void Move(const EXPoint &p)
    {
        m_Btn2.Move(p);
        m_Btn1.Move(p);
        m_Back.Move(p);
        m_Thumb.Move(p);
        EXWidget::Move(p);
    }
    virtual void SetClr(const ex::XRGBA &c)
    {
        EXWidget::SetClr(c);
        m_Btn1.SetClr(c);
        m_Btn2.SetClr(c);
        m_Back.SetClr(c);
        m_Thumb.SetClr(c);
    }
    virtual void SetTextScale(float s) { m_Back.SetTextScale(s); }
    virtual void SetTextClr(const ex::XRGBA &a, const ex::XRGBA &b, const ex::XRGBA &c, const ex::XRGBA &d)
    {
        m_Back.SetTextClr(a, b, c, d);
    }
    virtual void GetTextClr(ex::XRGBA &a, ex::XRGBA &b, ex::XRGBA &c, ex::XRGBA &d) const
    {
        m_Back.GetTextClr(a, b, c, d);
    }
    virtual void SetFont(unsigned int file, unsigned int font) { m_Back.SetFont(file, font); }
    virtual void SetTextAlignment(EXW::TextAlign a) { m_Back.SetTextAlignment(a); }
    virtual unsigned int GetTextAlignment() const { return m_Back.GetTextAlignment(); }
    virtual unsigned int HitTest(const EXPoint &p);
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    virtual void Draw();
    int GetTrack() const { return m_Track; }
    signed char SetTrackSize(int, int, int);
    signed char SetTrack(int, signed char, signed char);
    void ThumbTrack(const EXPoint &p);
    EXWidgetScroll(EXBaseWnd *pWnd, unsigned int wid, signed char bVertical, unsigned int, unsigned int,
                   unsigned int, unsigned int, unsigned int, unsigned int);
protected:
    void SetTracker();
public:
    EXWidgetButton m_Btn1;    // 0x04c
    EXWidgetButton m_Btn2;    // 0x334
    EXWidgetButton m_Back;    // 0x61c
    EXWidgetWindow m_Thumb;   // 0x904
    int m_Track;              // 0xbdc
    int m_Unkbe0;             // 0xbe0
    int m_Unkbe4;             // 0xbe4
    int m_Unkbe8;             // 0xbe8
    int m_Unkbec;             // 0xbec
    unsigned int m_Glyph;     // 0xbf0
    signed char m_bVertical;  // 0xbf4
};

class EXWidgetListBox : public EXWidget {
public:
    struct LBITEM {
        LBITEM()
        {
            m_pWidget = 0;
            m_pData = 0;
        }
        ~LBITEM() {}
        EXRect m_Rect;        // 0x00
        EXWidget *m_pWidget;  // 0x10
        void *m_pData;        // 0x14
    };
    virtual ~EXWidgetListBox();
    virtual EXWIDGETTYPE Type() const { return EXWT_LISTBOX; }
    virtual void SetRect(const EXRect &r);
    virtual void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }
    virtual void Move(const EXPoint &p);
    virtual void SetClr(const ex::XRGBA &c)
    {
        m_Back.SetClr(c);
        m_Scroll.SetClr(c);
    }
    virtual void SetTextScale(float s)
    {
        for (int i = 0; i < m_Items.m_nSize; i++) m_Items[i].m_pWidget->SendMsg(0x24, (int)&s, 0, 0, 0);
    }
    virtual void SetFont(unsigned int file, unsigned int font)
    {
        for (int i = 0; i < m_Items.m_nSize; i++) m_Items[i].m_pWidget->SendMsg(0x20, file, font, 0, 0);
    }
    virtual void SetTextAlignment(EXW::TextAlign a)
    {
        for (int i = 0; i < m_Items.m_nSize; i++) m_Items[i].m_pWidget->SendMsg(0x22, a, 0, 0, 0);
    }
    virtual unsigned int HitTest(const EXPoint &p);
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    virtual void Draw();
    virtual unsigned int SetCurSel(int sel, signed char bNotify);
    int GetCurSel() const { return m_CurSel; }
    int NumElements() const { return m_Items.m_nSize; }
    EXWidget *GetItem(unsigned int i) const;
    void *GetItemData(unsigned int i) const;
    signed char SetItemData(unsigned int i, void *p);
    int AddItem(EXWidget *p, void *data);
    void ResetContent();
    EXWidgetListBox(EXBaseWnd *pWnd, unsigned int wid, signed char, unsigned int, unsigned int, unsigned int,
                    unsigned int, unsigned int, unsigned int, unsigned int);
protected:
    void RecalculateContents(int, int);
    void RecalculateTrack(int);
    void BroadcastMsg2Contents(unsigned int, int, int, int, int, signed char);
public:
    EXArray<LBITEM, const LBITEM &> m_Items;  // 0x04c
    EXWidgetWindow m_Back;                    // 0x060
    EXWidgetScroll m_Scroll;                  // 0x338
    EXSprite2D m_SelSprite;                   // 0xf30
    int m_Unkf4c;                             // 0xf4c
    float m_Unkf50;                           // 0xf50
    int m_Unkf54;                             // 0xf54
    int m_CurSel;                             // 0xf58
    signed char m_Unkf5c;                     // 0xf5c
    signed char m_Unkf5d;                     // 0xf5d
};

class EXWidgetSpinList : public EXWidget {
public:
    virtual ~EXWidgetSpinList() {}
    virtual EXWIDGETTYPE Type() const { return EXWT_SPINLIST; }
    virtual void SetRect(const EXRect &r);
    virtual void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }
    virtual EXRect GetRect() const;
    virtual void Move(const EXPoint &p)
    {
        m_List.Move(p);
        m_Back.Move(p);
        m_BtnUp.Move(p);
        m_BtnDown.Move(p);
        m_BtnDrop.Move(p);
        Notify(EXWN_MOVE);
    }
    virtual void SetClr(const ex::XRGBA &c)
    {
        m_List.SetClr(c);
        m_Back.SetClr(c);
        m_BtnUp.SetClr(c);
        m_BtnDown.SetClr(c);
        m_BtnDrop.SetClr(c);
    }
    virtual unsigned int HitTest(const EXPoint &p);
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    virtual void Draw();
    virtual unsigned int SetCurSel(int sel, signed char bNotify);
    int GetCurSel() const { return m_List.m_CurSel; }
    int NumElements() const { return m_List.m_Items.m_nSize; }
    void RollUp(signed char, signed char);
    signed char Next();
    signed char Prev();
    int AddItem(EXWidget *p, void *data);
    EXWidgetSpinList(EXBaseWnd *pWnd, unsigned int wid, unsigned int, unsigned int, unsigned int, unsigned int,
                     unsigned int, unsigned int, unsigned int);
private:
    void ConvertPtToBack(EXPoint &p) const;
public:
    EXWidgetListBox m_List;     // 0x004c
    EXWidgetWindow m_Back;      // 0x0fac
    EXWidgetButton m_BtnUp;     // 0x1284
    EXWidgetButton m_BtnDown;   // 0x156c
    EXWidgetButton m_BtnDrop;   // 0x1854
    EXWidget *m_pSel;           // 0x1b3c
    EXWIDGETINFO *m_pGlyph;     // 0x1b40
    int m_Unk1b44;              // 0x1b44
    unsigned int m_Glyph;       // 0x1b48
    signed char m_bRolledDown;  // 0x1b4c
};

class EXWidgetTextCmd : public EXWidget {
public:
    virtual ~EXWidgetTextCmd() {}
    virtual EXWIDGETTYPE Type() const { return EXWT_TEXTCMD; }
    virtual void SetRect(const EXRect &r);
    virtual void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }
    virtual void Move(const EXPoint &p)
    {
        m_Edit.Move(p);
        m_Back.Move(p);
        Notify(EXWN_MOVE);
    }
    virtual void SetClr(const ex::XRGBA &c)
    {
        m_Back.SetClr(c);
        m_Edit.SetClr(c);
    }
    virtual void SetFont(unsigned int file, unsigned int font);
    virtual int SendMsg(unsigned int msg, int a, int b, int c, int d);
    virtual void Draw();
    char *GetCommand(int i);
    EXWidgetTextCmd(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyph);
private:
    void PushCmd(const char *s);
    int GetCommandInd(int i);
public:
    EXWidgetWindow m_Back;                     // 0x04c
    EXWidgetWindow m_Edit;                     // 0x324
    EXCircularBuffer<char, 2048> m_History;    // 0x5fc
    EXSprite2D m_Cursor;                       // 0xe0c
    int m_Unke28;                              // 0xe28
    int m_Unke2c;                              // 0xe2c
    int m_Unke30;                              // 0xe30
    int m_Unke34;                              // 0xe34
    int m_Unke38;                              // 0xe38
    int m_Unke3c;                              // 0xe3c
    int m_Unke40;                              // 0xe40
};

class EXWidgetPage : public EXWidget {
public:
    virtual ~EXWidgetPage();
    virtual EXWIDGETTYPE Type() const { return EXWT_PAGE; }
    virtual void SetRect(const EXRect &r);
    virtual void SetRect(int x, int y, int w, int h) { SetRect(EXRect(x, y, w, h)); }
    virtual void Move(const EXPoint &p);
    virtual void SetClr(const ex::XRGBA &c);
    virtual void SetTextClr(const ex::XRGBA &a, const ex::XRGBA &b, const ex::XRGBA &c, const ex::XRGBA &d);
    virtual void Draw();
    EXWidget *GetFocus() const { return m_pFocus; }
    void SetKeysInput(signed char *pKeys, signed char *pPrev)
    {
        m_pKeys = pKeys;
        m_pPrevKeys = pPrev;
    }
    void EnableMouseInput(signed char b)
    {
        m_bMouse = b;
        m_Unk344 = 0;
        m_Unk345 = 0;
        m_Unk346 = 0;
    }
    void EnableKeyInput(signed char b) { m_bKeys = b; }
    signed char IsUsingMouseInput() const { return m_bMouse; }
    signed char IsUsingKeysInput() const { return m_bKeys; }
    void SetFocus(EXWidget *p);
    void SetMouseInput(signed char, signed char, const EXPoint &);
    EXWidget *Find(unsigned int wid, signed char bRecurse);
    void SetFocus(unsigned int wid);
    void Update(float dt, signed char);
    EXWidget *AddWidget(EXWidget *p, EXWidgetCallback cb);
    int SendMsg(unsigned int, unsigned int, int, int, int, int);
    EXWidgetPage(EXBaseWnd *pWnd, unsigned int wid, unsigned short);
private:
    EXWidget *FocusChange(unsigned int);
public:
    EXPtrArray<EXWidget> m_Widgets;  // 0x04c
    EXWidget *m_pFocus;              // 0x05c
    EXWidgetWindow m_Back;           // 0x060
    int m_Unk338;                    // 0x338
    unsigned int m_Unk33c;           // 0x33c
    signed char m_Unk340;            // 0x340
    signed char m_Unk341;            // 0x341
    signed char m_bMouse;            // 0x342
    signed char m_bKeys;             // 0x343
    signed char m_Unk344;            // 0x344
    signed char m_Unk345;            // 0x345
    signed char m_Unk346;            // 0x346
    EXPoint m_MousePos;              // 0x348
    EXPoint m_PrevMousePos;          // 0x350
    EXPoint m_MouseDelta;            // 0x358
    signed char *m_pKeys;            // 0x360
    signed char *m_pPrevKeys;        // 0x364
};
#endif
