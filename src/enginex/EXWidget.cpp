#include "EXWidget.h"
#include "EXGeoFile.h"
#include "EXNew.h"
#include <string.h>

// EngineX debug UI widgets. Function order follows the original object.

static EXArray<EXWIDGETINFO *, EXWIDGETINFO *const &> s_Glyphs;

// Builds a colour byte by byte (retail stores each byte into a stack temp).
static inline ex::XRGBA MakeClr(u8 r, u8 g, u8 b, u8 a)
{
    ex::XRGBA c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

static void MakeSureFileIsLoaded(unsigned int file)
{
    EXGeoFile::LoadGeoFile(file, 0, 0);
}

void EXWidget::SelectFont(unsigned int file, unsigned int font) const
{
    EXGeoFile *pFile = EXGeoFile::GetGeoFile(file);
    m_pWnd->SelectFont(pFile, font);
    m_pWnd->m_FontColor = m_Clr.m_c;
}

EXWidget::~EXWidget()
{
    Notify(EXWN_DESTROY);
}

void EXWidget::Move(const EXPoint &p)
{
    m_Rect.x = m_Rect.x + p.x;
    m_Rect.y = m_Rect.y + p.y;
    m_ClientRect.x = m_ClientRect.x + p.x;
    m_ClientRect.y = m_ClientRect.y + p.y;
    for (unsigned short i = 0; i < m_nSprites; i++) {
        m_pSprites[i].x += (short)p.x;
        m_pSprites[i].y += (short)p.y;
    }
    Notify(EXWN_MOVE);
}

unsigned int EXWidget::HitTest(const EXPoint &p)
{
    if (m_Rect.ContainsPoint(p)) return 1;
    return 0;
}

void EXWidget::SetClr(const ex::XRGBA &c)
{
    m_Clr = c;
    for (unsigned short i = 0; i < m_nSprites; i++) m_pSprites[i].m_Color = c.m_c;
}

void EXWidgetText::SetFont(unsigned int file, unsigned int font)
{
    m_Font = font;
    m_FontFile = file;
    MakeSureFileIsLoaded(file);
}

void EXWidgetImage::SetRect(const EXRect &r)
{
    if (m_Rect.x == r.x && m_Rect.y == r.y && m_Rect.w == r.w && m_Rect.h == r.h) return;
    EXWidget::SetRect(r);
    // TODO: sprite placement
}

void EXWidgetImage::SetImage(unsigned int file, unsigned int tex, float u0, float v0, float u1, float v1)
{
    m_Tex = tex;
    if (file) m_File = file;
    MakeSureFileIsLoaded(m_File);
    if (m_pSprites) {
        m_pSprites->u0 = u0;
        m_pSprites->v0 = v0;
        m_pSprites->u1 = u1;
        m_pSprites->v1 = v1;
    }
}

unsigned int EXWidgetWindow::HitTest(const EXPoint &p)
{
    if (!m_Rect.ContainsPoint(p)) return 0;
    if (m_ClientRect.ContainsPoint(p)) return 0x10;
    if (EXRect(m_Rect.x, m_Rect.y, m_Rect.w, m_MinSize.y / 2).ContainsPoint(p)) return 4;
    if (EXRect(m_Rect.x + m_Rect.w - m_MinSize.x / 2, m_Rect.y + m_Rect.h - m_MinSize.y / 2, m_MinSize.x,
               m_MinSize.y)
            .ContainsPoint(p))
        return 8;
    return 1;
}

void EXWidgetWindow::SetImage(unsigned int file, unsigned int tex, float u0, float v0, float u1, float v1)
{
    if (file) m_Unk2d0 = file;
    MakeSureFileIsLoaded(m_Unk2d0);
    m_Unk2cc = tex;
    if (m_pSprites) {
        m_pSprites[9].u0 = u0;
        m_pSprites[9].v0 = v0;
        m_pSprites[9].u1 = u1;
        m_pSprites[9].v1 = v1;
    }
}

unsigned int EXWidgetScroll::HitTest(const EXPoint &p)
{
    if (m_Btn1.HitTest(p) || m_Btn2.HitTest(p) || m_Back.HitTest(p)) return 0x20;
    return m_Thumb.HitTest(p);
}

unsigned int EXWidgetListBox::HitTest(const EXPoint &p)
{
    if (!m_Rect.ContainsPoint(p)) return 0;
    if (m_Back.HitTest(p)) return 0x10;
    return m_Scroll.HitTest(p);
}

unsigned int EXWidgetSpinList::HitTest(const EXPoint &p)
{
    if (m_bRolledDown) {
        if (m_Rect.ContainsPoint(p)) return 0x10;
        return 0;
    }
    EXRect r = m_Back.GetRect();
    EXRect rb = m_BtnDrop.GetRect();
    r.w += rb.w;
    if (r.ContainsPoint(p)) return 0x10;
    return 0;
}

void EXWidgetTextCmd::SetFont(unsigned int file, unsigned int font)
{
    m_Unke2c = font;
    m_Unke28 = file;
    MakeSureFileIsLoaded(file);
    SelectFont(file, font);
    SetRect(m_Rect);
}

// Key code -> character table: unshifted by key code; shifted at +0x62 (letters), +0x79 (digits), +0x5e (punctuation).
static const unsigned char s_KeyChars[0xa4] = {
    0x00, 0x00, 0x00, 0x00, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x0a, 0x00, 0x00, 0x09, 0x20, 0x2d, 0x3d, 0x5b, 0x5d, 0x00, 0x23, 0x3b, 0x27, 0x60, 0x2c, 0x2e, 0x2f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x2a, 0x2d, 0x2b, 0x0a, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x2e, 0x5c, 0x00, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5f, 0x2b, 0x7b, 0x7d, 0x00, 0x7e, 0x3a, 0x40, 0xac, 0x3c, 0x3e, 0x3f, 0x21, 0x22, 0xa3, 0x24, 0x25, 0x5e, 0x26, 0x2a, 0x28, 0x29, 0x00, 0x00, 0x00
};

// Text-entry helper: reads the pressed-key states (keys, HID-style codes) and edits buf at the
// cursor. Returns the new cursor position. flags bit 0 = caps lock; insert shifts the tail right.
int EXKeyProcess(unsigned int &flags, const signed char *keys, const signed char *mods, char *buf, int cursor, signed char insert)
{
    char ch = 0;
    int len = 0;
    while (buf[len] != 0) len++;
    if (cursor == -1) cursor = len;
    if (keys[0x39]) {
        if (flags & 1) flags &= ~1u;
        if (keys[0x39] && !(flags & 1)) flags |= 1;
    }
    int pos = len;
    if (cursor < len) pos = cursor > 0 ? cursor : 0;

    if (keys[0x2a] && len > 0 && pos > 0) {
        pos--;
        if (pos <= len - 1) memmove(buf + pos, buf + pos + 1, len - pos);
        return pos;
    }
    if (keys[0x4c] && len > 0 && pos < len) {
        memmove(buf + pos, buf + pos + 1, len - pos);
        return pos;
    }

    bool shift = (mods[0xe5] || mods[0xe1]) ? true : false;
    shift = ((flags & 1) != 0) != shift;

    for (int k = 4; k < 0x1e; k++) {
        if (keys[k]) {
            ch = s_KeyChars[shift ? k + 0x62 : k];
            cursor = pos + 1;
        }
    }
    for (int k = 0x1e; ch == 0 && k < 0x28; k++) {
        if (keys[k]) {
            ch = s_KeyChars[shift ? k + 0x79 : k];
            cursor = pos + 1;
        }
    }
    for (int k = 0x59; ch == 0 && k < 100; k++) {
        if (keys[k]) {
            ch = s_KeyChars[k];
            cursor = pos + 1;
        }
    }
    for (int k = 0x2d; ch == 0 && k < 0x39; k++) {
        if (keys[k]) {
            ch = s_KeyChars[shift ? k + 0x5e : k];
            cursor = pos + 1;
        }
    }
    if (ch == 0) {
        if (keys[100]) {
            ch = shift ? '|' : '\\';
        } else if (keys[0x2b]) {
            ch = '\t';
        } else if (keys[0x2c]) {
            ch = ' ';
        } else if (keys[0x58]) {
            ch = '\n';
        } else if (keys[0x28]) {
            ch = '\n';
        } else if (keys[0x54]) {
            ch = '/';
        } else if (keys[0x55]) {
            ch = '*';
        } else if (keys[0x56]) {
            ch = '-';
        } else if (keys[0x57]) {
            ch = '+';
        } else {
            if (keys[0x50] && pos > 0) return pos - 1;
            if (keys[0x4f] && pos < len) return pos + 1;
            return cursor;
        }
        cursor = pos + 1;
    }
    if (insert) {
        if (pos <= len) memmove(buf + pos + 1, buf + pos, len - pos + 1);
        buf[pos] = ch;
        return cursor;
    }
    buf[pos] = ch;
    buf[pos + 1] = 0;
    return cursor;
}

static int WidgetSort(const void *a, const void *b)
{
    int oa = ((const WIDGETSORTORDER *)a)->m_Order;
    int ob = ((const WIDGETSORTORDER *)b)->m_Order;
    if (oa < ob) return -1;
    return oa > ob;
}

void EXWidgetPage::SetRect(const EXRect &)
{
    Notify(4);
}

void EXWidgetPage::SetFocus(EXWidget *p)
{
    if (m_pFocus && m_pFocus != p) m_pFocus->SendMsg(4, 0, 0, 0, 0);
    m_pFocus = p;
    if (p) p->SendMsg(5, 0, 0, 0, 0);
}

void EXWidgetPage::SetMouseInput(signed char b1, signed char b2, const EXPoint &p)
{
    if (m_MousePos.x || m_MousePos.y) {
        m_MouseDelta.x = p.x - m_MousePos.x;
        m_MouseDelta.y = p.y - m_MousePos.y;
    }
    m_PrevMousePos.x = m_MousePos.x;
    m_PrevMousePos.y = m_MousePos.y;
    m_MousePos.x = p.x;
    m_MousePos.y = p.y;
    m_Unk344 = b1;
    m_Unk345 = b2;
}

int EXWidget::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    switch (msg) {
    case 0:  // set callback
        m_pCallback = (EXWidgetCallback)a;
        return 1;
    case 1:  // fire callback with a message
        if (m_pCallback) m_pCallback(this, a);
        return 1;
    case 2:  // fire the visible-changed callback
        if ((m_Flags & 1) && m_pCallback) m_pCallback(this, 1);
        return 1;
    case 0x12:  // SetVisible
        SetFlag(1, (signed char)a);
        return 1;
    case 0x13:  // IsVisible
        return m_Flags & 1;
    case 0x27:  // SetUserData
        m_UserData = a;
        return 1;
    case 0x28:  // GetUserData
        return m_UserData;
    case 0x14:  // SetRect
        if (a) SetRect(*(EXRect *)a);
        return 1;
    case 0x15:  // GetRect
        if (a) {
            EXRect *r = (EXRect *)a;
            r->x = m_Rect.x;
            r->y = m_Rect.y;
            r->w = m_Rect.w;
            r->h = m_Rect.h;
        }
        return 1;
    case 0x16:  // GetClientRect
        if (a) {
            EXRect *r = (EXRect *)a;
            r->x = m_ClientRect.x;
            r->y = m_ClientRect.y;
            r->w = m_ClientRect.w;
            r->h = m_ClientRect.h;
        }
        return 1;
    case 0x17:  // GetMinSize
        if (a) {
            EXPoint *pt = (EXPoint *)a;
            pt->x = m_MinSize.x;
            pt->y = m_MinSize.y;
        }
        return 1;
    case 0x18:  // Size
        if (a) Size(*(EXPoint *)a);
        return 1;
    case 0x19:  // Move
        if (a) Move(*(EXPoint *)a);
        return 1;
    case 0x1a:  // SetClr
        if (a) SetClr(*(ex::XRGBA *)a);
        return 1;
    case 0x1b:  // GetClr
        if (a) *(ex::XRGBA *)a = m_Clr;
        return 1;
        case 0x3:
        return 0;
    case 0x4:
        return 0;
    case 0x5:
        return 0;
    case 0x6:
        return 0;
    case 0x7:
        return 0;
    case 0x8:
        return 0;
    case 0x9:
        return 0;
    case 0xa:
        return 0;
    case 0xb:
        return 0;
    case 0xc:
        return 0;
    case 0xd:
        return 0;
    case 0xe:
        return 0;
    case 0xf:
        return 0;
    case 0x10:
        return 0;
    case 0x11:
        return 0;
    case 0x1c:
        return 0;
    case 0x1d:
        return 0;
    case 0x1e:
        return 0;
    case 0x1f:
        return 0;
    case 0x20:
        return 0;
    case 0x21:
        return 0;
    case 0x22:
        return 0;
    case 0x23:
        return 0;
    case 0x24:
        return 0;
    case 0x25:
        return 0;
    case 0x26:
        return 0;
    default:
        break;
    }
    return 0;
}

EXRect EXWidgetText::FitInToRect(const EXRect &r) const
{
    int x = m_Rect.x;
    int y = m_Rect.y;
    int w = m_Rect.w;
    int h = m_Rect.h;
    const char *cap = m_Caption;
    if (cap) {
        SelectFont(m_FontFile, m_Font);
        EXRect tmp = r;
        h = (int)((float)m_pWnd->FontTextSize(cap, tmp, 1.0f).y * m_Scale);
        w = r.w;
    }
    return EXRect(x, y, w, h);
}

template <class T> static inline const T &MinRef(const T &a, const T &b) { return (b < a) ? b : a; }

void EXWidgetText::SetCaption(const char *s)
{
    if (s) {
        unsigned int len = strlen(s);
        unsigned int n = MinRef(255u, len);
        memcpy(m_Caption.m_Data, s, n);
        m_Caption[n] = 0;
    } else {
        memset(m_Caption.m_Data, 0, 256);
    }
}

void EXWidgetText::Draw()
{
    if (m_Flags & 1) {
        if (m_Caption[0] && m_pWnd) {
            SelectFont(m_FontFile, m_Font);
            m_pWnd->FontPrint((const char *)m_Caption.m_Data, m_Rect, m_Scale, (EXW::TextAlign)m_Align, 0, 0);
        }
        if (m_pCallback) m_pCallback(this, 5);
    }
}

int EXWidgetText::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    switch (msg) {
    case 0x20:
        SetFont(a, b);
        return 1;
    case 0x24:
        SetTextScale(*(float *)a);
        return 1;
    case 0x21:
        if (a) *(unsigned int *)a = m_FontFile;
        if (b) *(unsigned int *)b = m_Font;
        return 1;
    case 0x22:
        SetTextAlignment((EXW::TextAlign)a);
        return 1;
    case 0x23:
        return GetTextAlignment();
    case 0x25:
        SetCaption((const char *)a);
        return 1;
    case 0x26:
        return (int)GetCaption();
    case 0x1c:
        if (a) m_Clr.m_c = *(unsigned int *)a;
        if (b) m_TextClr[0].m_c = *(unsigned int *)b;
        if (c) m_TextClr[1].m_c = *(unsigned int *)c;
        if (d) m_TextClr[2].m_c = *(unsigned int *)d;
        return 1;
    case 0x1d:
        if (a) *(unsigned int *)a = m_Clr.m_c;
        if (b) *(unsigned int *)b = m_TextClr[0].m_c;
        if (c) *(unsigned int *)c = m_TextClr[1].m_c;
        if (d) *(unsigned int *)d = m_TextClr[2].m_c;
        return 1;
    }
    return EXWidget::SendMsg(msg, a, b, c, d);
}

void EXWidgetImage::Draw()
{
    if (m_Flags & 1) {
        if (m_pWnd && m_pSprites) {
            EXGeoFile *pGeo = EXGeoFile::GetGeoFile(m_File);
            m_pWnd->SelectSprite2DTexture(pGeo, m_Tex, 1, 1);
            m_pWnd->_DrawSprite2D(*m_pSprites);
        }
        if (m_pCallback) m_pCallback(this, 5);
    }
}

int EXWidgetImage::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    switch (msg) {
    case 0x1f: {
        if (a) *(unsigned int *)a = m_File;
        if (b) *(unsigned int *)b = m_Tex;
        EXRect *pr = (EXRect *)c;
        if (pr) {
            EXSprite2D *sp = m_pSprites;
            int y = (int)sp->v0;
            int w = (int)sp->u1;
            int x = (int)sp->u0;
            int h = (int)sp->v1;
            pr->w = w;
            pr->y = y;
            pr->h = h;
            pr->x = x;
        }
        return 1;
    }
    case 0x1e: {
        int x = 0, y = 0, w = 0, h = 0;
        EXRect *pr = (EXRect *)c;
        if (pr) {
            x = pr->x;
            y = pr->y;
            w = pr->w;
            h = pr->h;
        }
        SetImage(a, b, (float)x, (float)y, (float)w, (float)h);
        return 1;
    }
    }
    return EXWidget::SendMsg(msg, a, b, c, d);
}

void EXWidgetWindow::SetRect(const EXRect &r)
{
    // TODO
}

void EXWidgetWindow::Draw()
{
    if (m_Flags & 1) {
        if (m_pWnd && m_pSprites && m_pGlyph) {
            if (m_pGlyph->m_Unk10 == 0) {
                m_pWnd->_SelectSprite2DTexture(0, 0, 0);
            } else {
                EXGeoFile *pGeo = EXGeoFile::GetGeoFile(m_pGlyph->m_File);
                m_pWnd->SelectSprite2DTexture(pGeo, m_pGlyph->m_Unk10, 1, 1);
            }
            for (unsigned short i = 0; i < m_nSprites - 1; i++) m_pWnd->_DrawSprite2D(m_pSprites[i]);
            if (m_Unk2cc) {
                EXGeoFile *pGeo = EXGeoFile::GetGeoFile(m_Unk2d0);
                m_pWnd->SelectSprite2DTexture(pGeo, m_Unk2cc, 1, 1);
                m_pWnd->_DrawSprite2D(m_pSprites[9]);
            }
        }
        if (m_Text.GetCaption()) m_Text.Draw();
        if (m_pCallback) m_pCallback(this, 5);
    }
}

int EXWidgetWindow::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    switch (msg) {
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        return 0;
    case 8:
        if (a && b) {
            switch (HitTest(*(EXPoint *)a)) {
            case 4:
                Move(*(EXPoint *)b);
                break;
            case 8:
                Size(*(EXPoint *)b);
                break;
            }
        }
        return 1;
    case 0x1e: {
        int x = 0, y = 0, w = 0, h = 0;
        EXRect *pr = (EXRect *)c;
        if (pr) {
            x = pr->x;
            y = pr->y;
            w = pr->w;
            h = pr->h;
        }
        SetImage(a, b, (float)x, (float)y, (float)w, (float)h);
        return 1;
    }
    case 0x1f: {
        if (a) *(unsigned int *)a = m_Unk2d0;
        if (b) *(unsigned int *)b = m_Unk2cc;
        EXRect *pr = (EXRect *)c;
        if (pr) {
            EXSprite2D *sp = m_pSprites;
            int x = (int)sp->u0;
            int y = (int)sp->v0;
            int w = (int)sp->u1;
            int h = (int)sp->v1;
            pr->x = x;
            pr->y = y;
            pr->w = w;
            pr->h = h;
        }
        return 1;
    }
    case 0x1c:
    case 0x1d:
    case 0x20:
    case 0x21:
    case 0x22:
    case 0x23:
    case 0x24:
    case 0x25:
    case 0x26:
        return m_Text.SendMsg(msg, a, b, c, d);
    }
    return EXWidget::SendMsg(msg, a, b, c, d);
}

template <class T> static inline const T &MaxRef(const T &a, const T &b) { return (a >= b) ? a : b; }

void EXWidgetScroll::SetTracker()
{
    // m_Back is the sliding thumb button, m_Thumb the track bar behind it.
    if (m_Back.m_Flags & 1) {
        float ratio = 1.0f;
        EXRect tmp = m_Thumb.GetRect();
        EXRect tr = tmp;
        EXPoint minSz(m_Back.GetMinSize());
        if (m_Unkbe0 > m_Unkbe4) m_Unkbe0 = m_Unkbe4;
        int c = m_Unkbe8;
        int t = m_Track;
        int hi = m_Unkbe4 - c;
        if (t < hi) {
            if (t <= m_Unkbe0) t = m_Unkbe0;
        } else {
            t = hi;
        }
        int range = m_Unkbe4 - m_Unkbe0;
        m_Track = t;
        if (c > range) m_Unkbe8 = range;
        if (range) ratio = (float)m_Unkbe8 / (float)range;
        int len, room;
        if (m_bVertical) {
            len = (int)((float)tr.h * ratio);
            tr.h = MaxRef(len, minSz.y);
            room = m_ClientRect.h - tr.h;
        } else {
            len = (int)((float)tr.w * ratio);
            tr.w = MaxRef(len, minSz.x);
            room = m_ClientRect.w - tr.w;
        }
        int denom = range - m_Unkbe8;
        if (denom == 0) denom = 1;
        int off = (int)((float)room / (float)denom * (float)t);
        if (m_bVertical) tr.y = m_Btn1.GetRect().h + m_ClientRect.y + off;
        else tr.x = m_Btn1.GetRect().w + m_ClientRect.x + off;
        m_Back.SetRect(tr);
    }
}

void EXWidgetScroll::SetRect(const EXRect &r)
{
    // TODO
}

signed char EXWidgetScroll::SetTrackSize(int a, int b, int c)
{
    m_Unkbe0 = a;
    m_Unkbe4 = b;
    m_Unkbe8 = c;
    SetTracker();
    return m_Unkbe4 > m_Unkbe0;
}

signed char EXWidgetScroll::SetTrack(int pos, signed char bBtn, signed char bNotify)
{
    if (bBtn) {
        if (pos < m_Track) m_Btn1.SendMsg(EXWN_STATE, 0, 0, 0, 0);
        else if (pos > m_Track) m_Btn2.SendMsg(EXWN_STATE, 0, 0, 0, 0);
    }
    int old = m_Track;
    int v = m_Unkbe4;
    if (pos < m_Unkbe4) {
        v = m_Unkbe0;
        if (pos > m_Unkbe0) v = pos;
    }
    m_Track = v;
    SetTracker();
    if (m_Track != old) {
        if (bNotify && m_pCallback) m_pCallback(this, 7);
    }
    return m_Track != old;
}

void EXWidgetScroll::Draw()
{
    if (m_Flags & 1) {
        m_Thumb.Draw();
        m_Btn1.Draw();
        m_Btn2.Draw();
        m_Back.Draw();
        if (m_pCallback) m_pCallback(this, 5);
    }
}

void EXWidgetListBox::RecalculateContents(int bMeasure, int dy)
{
    if (bMeasure) {
        for (int i = 0; i < m_Items.m_nSize; i++) {
            LBITEM &it = m_Items.m_pData[i];
            EXRect probe(m_ClientRect.x, 10000, m_ClientRect.w, m_ClientRect.h);
            it.m_Rect = it.m_pWidget->FitInToRect(probe);
            it.m_Rect.x = m_ClientRect.x;
            it.m_Rect.y = (int)m_Unkf50 + m_ClientRect.y;
            it.m_pWidget->SetRect(it.m_Rect);
            it.m_Rect = it.m_pWidget->GetRect();
        }
    }
    m_Unkf50 = 0.0f;
    for (int i = 0; i < m_Items.m_nSize; i++) m_Unkf50 += (float)m_Items.m_pData[i].m_Rect.h;
    if (m_Unkf50 > (float)m_ClientRect.h) {
        m_Scroll.SetVisible(1);
        m_Scroll.SetTrackSize(0, (int)m_Unkf50, m_ClientRect.h);
        if ((int)m_Unkf50 - m_Unkf54 < m_ClientRect.h && dy > 0) {
            m_Unkf54 -= dy;
            m_Scroll.SetTrack(m_Unkf54, 0, 1);
        }
    } else {
        m_Unkf54 = 0;
        m_Scroll.SetVisible(0);
    }
    int top = m_Unkf54;
    int hi = (int)m_Unkf50;
    if (top < hi) {
        top = (top > 0) ? top : 0;
    } else {
        top = hi;
    }
    int y = m_ClientRect.y - top;
    m_Unkf54 = top;
    int x = m_ClientRect.x;
    for (int i = 0; i < m_Items.m_nSize; i++) {
        LBITEM &it = m_Items.m_pData[i];
        it.m_Rect.x = x;
        it.m_Rect.y = y;
        it.m_pWidget->SetRect(it.m_Rect);
        y += it.m_Rect.h;
    }
    m_SelSprite.x = (short)m_Rect.x;
    m_SelSprite.w = (unsigned short)m_Rect.w;
    m_ClientRect.w = m_Rect.w - 2 * m_Back.m_MinSize.x;
    if (m_Scroll.m_Flags & 1) {
        m_SelSprite.w -= (unsigned short)m_Scroll.GetRect().w;
        m_ClientRect.w -= m_Scroll.GetRect().w;
    }
    if (m_CurSel >= 0 && m_CurSel < m_Items.m_nSize) {
        m_SelSprite.y = (short)m_Items.m_pData[m_CurSel].m_Rect.y;
        m_SelSprite.h = (unsigned short)m_Items.m_pData[m_CurSel].m_Rect.h;
    }
}

void EXWidgetListBox::RecalculateTrack(int t)
{
    m_Unkf54 = t;
    RecalculateContents(0, 0);
}

void EXWidgetListBox::BroadcastMsg2Contents(unsigned int msg, int a, int b, int c, int d, signed char bVisibleOnly)
{
    EXPoint *pt = (EXPoint *)a;
    for (int i = 0; i < m_Items.m_nSize; i++) {
        LBITEM &it = m_Items.m_pData[i];
        if (bVisibleOnly) {
            if (it.m_Rect.y > m_ClientRect.y + m_ClientRect.h || it.m_Rect.y + it.m_Rect.h < m_ClientRect.y) continue;
        }
        switch (msg) {
        default:
            break;
        case 6:
        case 8: {
            if (!pt) continue;
            if (it.m_Rect.y < m_ClientRect.y + m_ClientRect.h && it.m_Rect.y + it.m_Rect.h > m_ClientRect.y) {
                if (i != m_CurSel && msg == 6) {
                    int cx = m_ClientRect.x;
                    if (pt->x >= cx && pt->y >= it.m_Rect.y && pt->x <= m_ClientRect.w + cx - 1 &&
                        pt->y <= it.m_Rect.y + it.m_Rect.h - 1)
                        SetCurSel(i, 1);
                }
            }
            if (!it.m_pWidget->HitTest(*pt)) continue;
            break;
        }
        }
        it.m_pWidget->SendMsg(msg, (int)pt, b, c, d);
    }
}

void EXWidgetListBox::Move(const EXPoint &p)
{
    EXRect r = m_Rect;
    r.x += p.x;
    r.y += p.y;
    SetRect(r);
    if (m_pCallback) m_pCallback(this, EXWN_MOVE);
}

void EXWidgetListBox::SetRect(const EXRect &r)
{
    if (m_Rect.x == r.x && m_Rect.y == r.y && m_Rect.w == r.w && m_Rect.h == r.h) return;
    int oldW = m_Rect.w;
    int oldH = m_Rect.h;
    m_Rect = r;
    int sbx = m_Scroll.m_MinSize.x;
    int sby = m_Scroll.m_MinSize.y;
    int bx = m_Back.m_MinSize.x;
    int by = m_Back.m_MinSize.y;
    int minW = bx + sbx;
    int w = m_Rect.w;
    if (w >= 800) w = 800;
    else if (w <= minW) w = minW;
    m_Rect.w = w;
    int minH = sby + by;
    int h = m_Rect.h;
    if (h >= 800) h = 800;
    else if (h <= minH) h = minH;
    m_Rect.h = h;
    m_Back.SetRect(m_Rect.x, m_Rect.y, w, h);
    m_Scroll.SetRect(m_Rect.w - sbx + m_Rect.x, m_Rect.y, 0, m_Rect.h);
    m_ClientRect.w = m_Rect.w - bx;
    m_ClientRect.x = m_Rect.x + bx;
    m_ClientRect.y = m_Rect.y + by;
    m_ClientRect.h = m_Rect.h - 2 * by;
    m_SelSprite.x = (short)m_Rect.x;
    m_SelSprite.w = (unsigned short)m_Rect.w;
    if (m_Scroll.m_Flags & 1) {
        m_SelSprite.w -= (unsigned short)m_Scroll.GetRect().w;
        m_ClientRect.w -= m_Scroll.GetRect().w;
    }
    RecalculateContents(m_Rect.w - oldW, m_Rect.h - oldH);
    if (m_pCallback) m_pCallback(this, 4);
}

void EXWidgetListBox::Draw()
{
    if (m_Flags & 1) {
        m_Back.Draw();
        m_Scroll.Draw();
        if (m_Unkf5c) {
            m_pWnd->SetSprite2DClipRect(m_Rect);
            m_pWnd->_SelectSprite2DTexture(0, 0, 0);
            m_pWnd->_DrawSprite2D(m_SelSprite);
        }
        for (int i = 0; i < m_Items.m_nSize; i++) {
            LBITEM &it = m_Items.m_pData[i];
            if (it.m_Rect.y <= m_ClientRect.y + m_ClientRect.h && it.m_Rect.y + it.m_Rect.h >= m_ClientRect.y) {
                m_pWnd->SetSprite2DClipRect(m_ClientRect);
                it.m_pWidget->Draw();
            }
        }
        m_pWnd->ResetSprite2DClipRect();
        if (m_pCallback) m_pCallback(this, 5);
    }
}

EXWidget *EXWidgetListBox::GetItem(unsigned int i) const
{
    if (i >= (unsigned int)m_Items.m_nSize) return 0;
    return m_Items.m_pData[i].m_pWidget;
}

void *EXWidgetListBox::GetItemData(unsigned int i) const
{
    if (i >= (unsigned int)m_Items.m_nSize) return 0;
    return m_Items.m_pData[i].m_pData;
}

signed char EXWidgetListBox::SetItemData(unsigned int i, void *p)
{
    if (i >= (unsigned int)m_Items.m_nSize) return 0;
    m_Items.m_pData[i].m_pData = p;
    return 1;
}

unsigned int EXWidgetListBox::SetCurSel(int sel, signed char bNotify)
{
    if (sel >= 0 && sel < m_Items.m_nSize) {
        if (sel != m_CurSel) {
            m_CurSel = sel;
            LBITEM &it = m_Items.m_pData[sel];
            m_Unkf4c = (int)it.m_pWidget;
            int cy = m_ClientRect.y;
            signed char bScroll = 0;
            if (it.m_Rect.y < cy) {
                m_Unkf54 += it.m_Rect.y - cy;
                bScroll = 1;
            } else if (it.m_Rect.y + it.m_Rect.h > m_ClientRect.h + cy) {
                m_Unkf54 += it.m_Rect.h - m_ClientRect.h - cy + it.m_Rect.y;
                bScroll = 1;
            }
            m_SelSprite.y = (short)it.m_Rect.y;
            m_SelSprite.h = (unsigned short)it.m_Rect.h;
            if (bScroll) {
                m_Scroll.SetTrack(m_Unkf54, 0, 1);
                RecalculateContents(0, 0);
            }
            if (bNotify && m_pCallback) m_pCallback(this, 7);
        }
    }
    return m_CurSel;
}

void EXWidgetSpinList::SetRect(const EXRect &r)
{
    // TODO
}

EXRect EXWidgetSpinList::GetRect() const
{
    return m_Rect;  // TODO
}

void EXWidgetSpinList::Draw()
{
    if (m_Flags & 1) {
        if (m_bRolledDown) m_List.Draw();
        m_Back.Draw();
        m_BtnUp.Draw();
        m_BtnDown.Draw();
        m_BtnDrop.Draw();
        if (m_pSel) {
            m_pWnd->SetSprite2DClipRect(m_ClientRect);
            EXRect old = m_pSel->GetRect();
            EXRect r = old;
            r.x = m_ClientRect.x;
            r.y = m_ClientRect.y;
            m_pSel->SetRect(r);
            m_pSel->Draw();
            m_pSel->SetRect(old);
            m_pWnd->ResetSprite2DClipRect();
        }
        if (m_pCallback) m_pCallback(this, 5);
    }
}

void EXWidgetSpinList::RollUp(signed char bUp, signed char bKeep)
{
    if (bUp) {
        if (!m_bRolledDown) return;
        if (m_pGlyph) {
            EXSprite2D *sp = m_pGlyph->m_pSprites;
            m_BtnDrop.SetImage(m_pGlyph->m_File, m_pGlyph->m_Unk10, sp[1].u0, sp[1].v0, sp[1].u1, sp[1].v1);
        }
        m_bRolledDown = 0;
        if (!bKeep) {
            SetCurSel(m_Unk1b44, 0);
        } else if ((unsigned int)m_List.m_CurSel >= (unsigned int)m_List.m_Items.m_nSize) {
            m_pSel = 0;
        } else {
            m_pSel = m_List.m_Items.m_pData[m_List.m_CurSel].m_pWidget;
        }
    } else {
        if (m_bRolledDown) return;
        if (m_pGlyph) {
            EXSprite2D *sp = m_pGlyph->m_pSprites;
            m_BtnDrop.SetImage(m_pGlyph->m_File, m_pGlyph->m_Unk10, sp[0].u0, sp[0].v0, sp[0].u1, sp[0].v1);
        }
        m_bRolledDown = 1;
        m_Unk1b44 = m_List.m_CurSel;
    }
    if (m_pCallback) m_pCallback(this, EXWN_STATE);
}

signed char EXWidgetSpinList::Next()
{
    if (m_List.m_CurSel == m_List.m_Items.m_nSize - 1) SetCurSel(0, 1);
    else SetCurSel(m_List.m_CurSel + 1, 1);
    m_BtnDown.SendMsg(EXWN_STATE, 0, 0, 0, 0);
    return 1;
}

signed char EXWidgetSpinList::Prev()
{
    if (m_List.m_CurSel == 0) SetCurSel(m_List.m_Items.m_nSize - 1, 1);
    else SetCurSel(m_List.m_CurSel - 1, 1);
    m_BtnUp.SendMsg(EXWN_STATE, 0, 0, 0, 0);
    return 1;
}

unsigned int EXWidgetSpinList::SetCurSel(int sel, signed char bNotify)
{
    EXWidgetListBox *pl = &m_List;
    unsigned int r = pl->SetCurSel(sel, bNotify);
    if (r >= (unsigned int)pl->m_Items.m_nSize) {
        m_pSel = 0;
        return m_List.m_CurSel;
    }
    m_pSel = pl->m_Items.m_pData[r].m_pWidget;
    return m_List.m_CurSel;
}

void EXWidgetSpinList::ConvertPtToBack(EXPoint &p) const
{
    if (m_pSel) {
        int dy = m_pSel->m_ClientRect.y - m_ClientRect.y;
        int dx = m_pSel->m_ClientRect.x - m_ClientRect.x;
        p.x += dx;
        p.y += dy;
    }
}

int EXWidgetSpinList::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    return 0;  // TODO
}

void EXWidgetTextCmd::PushCmd(const char *s)
{
    // TODO
}

void EXWidgetTextCmd::SetRect(const EXRect &r)
{
    // TODO
}

int EXWidgetTextCmd::GetCommandInd(int i)
{
    return 0;  // TODO
}

char *EXWidgetTextCmd::GetCommand(int i)
{
    return 0;  // TODO
}

EXWidget *EXWidgetPage::Find(unsigned int wid, signed char bFocusable)
{
    if (wid == (unsigned int)-1) return m_pFocus;
    for (int i = 0; i < m_Widgets.m_nSize; i++) {
        EXWidget *p = m_Widgets[i];
        if ((!bFocusable || ((unsigned char)p->m_Flags >> 1) & 1) && wid == p->m_WID) return p;
    }
    return 0;
}

void EXWidgetPage::Draw()
{
    if (m_Flags & 1) {
        for (int i = 0; i < m_Widgets.m_nSize; i++) {
            EXWidget *w = m_Widgets[i];
            if (!m_Unk340 || m_pFocus != w) w->Draw();
        }
        if (m_Unk340 && m_pFocus) {
            m_Back.Draw();
            m_pFocus->Draw();
        }
        if (m_pCallback) m_pCallback(this, 5);
    }
}

void EXWidgetPage::Move(const EXPoint &p)
{
    // TODO
}

void EXWidgetPage::SetFocus(unsigned int wid)
{
    // TODO
}

void EXWidgetPage::SetClr(const ex::XRGBA &c)
{
    for (int i = 0; i < m_Widgets.m_nSize; i++) m_Widgets[i]->SetClr(c);
}

void EXWidgetPage::SetTextClr(const ex::XRGBA &a, const ex::XRGBA &b, const ex::XRGBA &c, const ex::XRGBA &d)
{
    for (int i = 0; i < m_Widgets.m_nSize; i++) m_Widgets[i]->SetTextClr(a, b, c, d);
}

void EXWidgetPage::Update(float dt, signed char)
{
    // TODO
}

void EXWidgetScroll::ThumbTrack(const EXPoint &p)
{
    // TODO
}

void EXWidgetTextCmd::Draw()
{
    // TODO
}

int EXWidgetTextCmd::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    return 0;  // TODO
}

EXWidgetPage::~EXWidgetPage()
{
    // TODO
}

EXWidget *EXWidgetPage::AddWidget(EXWidget *p, EXWidgetCallback cb)
{
    if (p) {
        m_Widgets.Add(p);
        p->m_pPage = this;
        p->m_pCallback = cb;
    }
    return p;
}

signed char EXWidget::AddGlyphSet(unsigned int id, EXWIDGETINFO *pInfo, signed char bReplace)
{
    int i;
    for (i = 0; i < s_Glyphs.m_nSize; i++) {
        if (s_Glyphs[i]->m_ID == id) goto found;
    }
    s_Glyphs.Add(pInfo);
    return 1;
found:
    if (!bReplace) return 0;
    s_Glyphs[i] = pInfo;
    return 1;
}

void EXWidget::FreeGlyphs()
{
    s_Glyphs.RemoveAll();
}

int EXWidgetListBox::AddItem(EXWidget *p, void *data)
{
    if (!p) return -1;
    LBITEM it;
    it.m_pData = data;
    it.m_pWidget = p;
    EXRect probe(m_ClientRect.x, 10000, m_ClientRect.w, m_ClientRect.h);
    it.m_Rect = p->FitInToRect(probe);
    it.m_Rect.x = m_ClientRect.x;
    it.m_Rect.y = (int)m_Unkf50 + m_ClientRect.y;
    p->SetRect(it.m_Rect);
    it.m_Rect = p->GetRect();
    p->m_Flags &= 0xfff3;
    int idx = m_Items.Add(it);
    RecalculateContents(0, 0);
    return idx;
}

void EXWidgetListBox::ResetContent()
{
    if (m_pCallback) m_pCallback(this, 6);
    for (int i = 0; i < m_Items.m_nSize; i++) {
        if (m_Items.m_pData[i].m_pWidget) {
            delete m_Items.m_pData[i].m_pWidget;
            m_Items.m_pData[i].m_pWidget = 0;
        }
    }
    m_Items.RemoveAll();
    m_CurSel = -1;
}

int EXWidgetListBox::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    return 0;  // TODO
}

int EXWidgetSpinList::AddItem(EXWidget *p, void *data)
{
    int oldSel = m_List.m_CurSel;
    int idx = m_List.AddItem(p, data);
    if (oldSel == -1) SetCurSel(0, 1);
    return idx;
}

EXWidget *EXWidgetPage::FocusChange(unsigned int)
{
    return 0;  // TODO
}

int EXWidgetPage::SendMsg(unsigned int, unsigned int, int, int, int, int)
{
    return 0;  // TODO
}

static EXWIDGETINFO s_DefaultGlyphs[7] = {
    EXWIDGETINFO(0, 0.0f, 0.0f, 0, 0, 0, 0), EXWIDGETINFO(0, 0.0f, 0.0f, 0, 0, 0, 1),
    EXWIDGETINFO(0, 0.0f, 0.0f, 0, 0, 0, 2), EXWIDGETINFO(0, 0.0f, 0.0f, 0, 0, 0, 3),
    EXWIDGETINFO(0, 0.0f, 0.0f, 0, 0, 0, 4), EXWIDGETINFO(0, 0.0f, 0.0f, 0, 0, 0, 5),
    EXWIDGETINFO(0, 0.0f, 0.0f, 0, 0, 0, 6),
};  // TODO: real glyph tables (the $E initialisers)

static void LoadDefaultGlyphs()
{
    for (EXWIDGETINFO *p = s_DefaultGlyphs; p < s_DefaultGlyphs + 7; p++) EXWidget::AddGlyphSet(p->m_ID, p, 0);
}

EXWIDGETINFO *EXWidget::GetGlyph(unsigned int id, const EXWidget *)
{
    static int s_bLoaded;
    if (!s_bLoaded) {
        s_bLoaded = 1;
        LoadDefaultGlyphs();
    }
    for (int i = 0; i < s_Glyphs.m_nSize; i++) {
        EXWIDGETINFO *p = s_Glyphs[i];
        if (p->m_ID == id) {
            MakeSureFileIsLoaded(p->m_File);
            return p;
        }
    }
    return 0;
}

void EXWidget::SetUVs(unsigned int glyph, unsigned int &cur)
{
    if (glyph == cur || !m_pSprites) return;
    EXWIDGETINFO *p = GetGlyph(glyph, this);
    cur = glyph;
    if (!p) return;
    for (unsigned short i = 0; i < min(m_nSprites, (unsigned short)p->m_nSprites); i++) {
        m_pSprites[i].u0 = p->m_pSprites[i].u0;
        m_pSprites[i].v0 = p->m_pSprites[i].v0;
        m_pSprites[i].u1 = p->m_pSprites[i].u1;
        m_pSprites[i].v1 = p->m_pSprites[i].v1;
        m_pSprites[i].m_Color = p->m_pSprites[i].m_Color;
    }
}

EXWidget::EXWidget(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyph, EXSprite2D *pSprites,
                   unsigned short nSprites)
{
    m_pSprites = pSprites;
    m_WID = wid;
    m_nSprites = nSprites;
    m_pWnd = pWnd;
    m_pCallback = 0;
    m_Flags = 0;
    m_Flags = 0xffff;
    m_Clr = MakeClr(0x7f, 0x7f, 0x7f, 0xff);
    m_Rect.Set(0, 0, 0, 0);
    m_ClientRect.Set(0, 0, 0, 0);
    m_MinSize.x = 0;
    m_MinSize.y = 0;
    if (m_pSprites) {
        memset(m_pSprites, 0, m_nSprites * sizeof(EXSprite2D));
        EXWIDGETINFO *p = GetGlyph(glyph, this);
        if (p) {
            m_MinSize.x = p->m_MinW;
            m_MinSize.y = p->m_MinH;
            memcpy(m_pSprites, p->m_pSprites, min((unsigned int)m_nSprites, p->m_nSprites) * sizeof(EXSprite2D));
        }
    }
}

EXWidgetText::EXWidgetText(EXBaseWnd *pWnd, unsigned int wid, const char *caption, unsigned int file,
                           unsigned int font, const ex::XRGBA &clr)
    : EXWidget(pWnd, wid, (unsigned int)-1, 0, 0)
{
    memset(m_Caption.m_Data, 0, sizeof(m_Caption));
    if (caption) strcpy(m_Caption.m_Data, caption);
    m_Clr = clr;
    m_TextClr[0] = MakeClr(0xff, 0xff, 0xff, 0xff);
    m_TextClr[1] = MakeClr(0xff, 0xff, 0xff, 0xff);
    m_TextClr[2] = MakeClr(0xff, 0xff, 0xff, 0xff);
    SetFlag(2, 0);
    m_Align = 0;
    m_Font = font;
    m_FontFile = file;
    m_Scale = 1.0f;
    MakeSureFileIsLoaded(file);
    Notify(EXWN_CREATE);
}

EXWidgetImage::EXWidgetImage(EXBaseWnd *pWnd, unsigned int wid, unsigned int tex, unsigned int file,
                             const EXRect &uv)
    : EXWidget(pWnd, wid, (unsigned int)-1, &m_Sprite, 1)
{
    float u0 = (float)uv.x;
    float v0 = (float)uv.y;
    float u1 = (float)uv.w;
    float v1 = (float)uv.h;
    m_Tex = tex;
    if (file) m_File = file;
    MakeSureFileIsLoaded(m_File);
    if (m_pSprites) {
        m_pSprites->u0 = u0;
        m_pSprites->v0 = v0;
        m_pSprites->u1 = u1;
        m_pSprites->v1 = v1;
    }
    EXWidget::SetClr(m_Clr);
    SetFlag(2, 0);
    Notify(EXWN_CREATE);
}

EXWidgetWindow::EXWidgetWindow(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyph)
    : EXWidget(pWnd, wid, glyph, m_Sprites, 10), m_Text(pWnd, wid, 0, 0, 0, MakeClr(0, 0, 0, 0xff))
{
    if (m_pSprites) m_pSprites[9] = m_pSprites[4];
    m_Unk2cc = 0;
    m_Unk2d0 = 0;
    m_pGlyph = GetGlyph(glyph, this);
    m_Text.SetTextAlignment((EXW::TextAlign)4);
    Notify(EXWN_CREATE);
}

EXWidgetButton::EXWidgetButton(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyphUp, unsigned int glyphDown)
    : EXWidgetWindow(pWnd, wid, glyphUp)
{
    m_CurGlyph = glyphUp;
    m_GlyphUp = glyphUp;
    m_GlyphDown = glyphDown;
    m_fPressed = 0.0f;
}

signed char EXWidgetButton::SetState(signed char bDown, signed char bNotify)
{
    signed char bOld = GetState();
    SetUVs(bDown ? m_GlyphDown : m_GlyphUp, m_CurGlyph);
    m_fPressed = bDown ? 0.1f : 0.0f;
    if (bOld != bDown) {
        ex::XRGBA a, b, c, d;
        m_Text.GetTextClr(a, b, c, d);
        m_Text.SetTextClr(b, a, c, d);
        if (bNotify) Notify(EXWN_STATE);
    }
    return bDown;
}

int EXWidgetButton::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    return EXWidgetWindow::SendMsg(msg, a, b, c, d);  // TODO
}

EXWidgetCheckBox::EXWidgetCheckBox(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyphOff,
                                   unsigned int glyphOn)
    : EXWidgetWindow(pWnd, wid, glyphOff)
{
    m_CurGlyph = glyphOff;
    m_GlyphOff = glyphOff;
    m_GlyphOn = glyphOn;
    m_bState = 0;
}

signed char EXWidgetCheckBox::SetState(signed char bOn, signed char bNotify)
{
    signed char bOld = m_bState;
    SetUVs(bOn ? m_GlyphOn : m_GlyphOff, m_CurGlyph);
    m_bState = bOn;
    if (bOn != bOld) {
        ex::XRGBA a, b, c, d;
        m_Text.GetTextClr(a, b, c, d);
        m_Text.SetTextClr(b, a, c, d);
        if (bNotify) Notify(EXWN_STATE);
    }
    return bOn;
}

int EXWidgetCheckBox::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    return EXWidgetWindow::SendMsg(msg, a, b, c, d);  // TODO
}

EXWidgetScroll::EXWidgetScroll(EXBaseWnd *pWnd, unsigned int wid, signed char bVertical, unsigned int g1,
                               unsigned int g2, unsigned int g3, unsigned int g4, unsigned int g5,
                               unsigned int glyph)
    : EXWidget(pWnd, wid, 0, 0, 0), m_Btn1(pWnd, wid, g1, g2), m_Btn2(pWnd, wid, g1, g2),
      m_Back(pWnd, wid, g3, g4), m_Thumb(pWnd, wid, g5)
{
    m_Unkbec = 0;
    m_Unkbe0 = 0;
    m_Unkbe4 = 0;
    m_Track = 0;
    m_Unkbe8 = 0;
    m_bVertical = bVertical;
    m_Glyph = glyph;
    // TODO: glyph images, min size
    Notify(EXWN_CREATE);
}

int EXWidgetScroll::SendMsg(unsigned int msg, int a, int b, int c, int d)
{
    return 0;  // TODO
}

EXWidgetListBox::EXWidgetListBox(EXBaseWnd *pWnd, unsigned int wid, signed char b, unsigned int g1,
                                 unsigned int g2, unsigned int g3, unsigned int g4, unsigned int g5,
                                 unsigned int g6, unsigned int g7)
    : EXWidget(pWnd, wid, g1, 0, 0), m_Back(pWnd, wid, g1),
      m_Scroll(pWnd, wid, 1, g2, g3, g4, g5, g6, g7)
{
    m_Unkf5c = b;
    m_CurSel = -1;
    m_Unkf54 = 0;
    m_Unkf5d = 0;
    m_Unkf50 = 0.0f;
    m_Unkf4c = 0;
    // TODO
    Notify(EXWN_CREATE);
}

EXWidgetListBox::~EXWidgetListBox()
{
    ResetContent();
}

EXWidgetSpinList::EXWidgetSpinList(EXBaseWnd *pWnd, unsigned int wid, unsigned int g1, unsigned int g2,
                                   unsigned int g3, unsigned int g4, unsigned int g5, unsigned int g6,
                                   unsigned int glyph)
    : EXWidget(pWnd, wid, g1, 0, 0), m_List(pWnd, wid, 1, g1, g2, g3, g4, g5, g6, glyph),
      m_Back(pWnd, wid, g1), m_BtnUp(pWnd, wid, g2, g3), m_BtnDown(pWnd, wid, g2, g3),
      m_BtnDrop(pWnd, wid, g2, g3)
{
    m_Glyph = glyph;
    m_Unk1b44 = -1;
    m_pGlyph = GetGlyph(glyph, this);
    // TODO
    m_bRolledDown = 0;
    m_pSel = 0;
    Notify(EXWN_CREATE);
}

EXWidgetTextCmd::EXWidgetTextCmd(EXBaseWnd *pWnd, unsigned int wid, unsigned int glyph)
    : EXWidget(pWnd, wid, 0, 0, 0), m_Back(pWnd, wid, glyph), m_Edit(pWnd, wid, glyph), m_History(1)
{
    // TODO
    Notify(EXWN_CREATE);
}

EXWidgetPage::EXWidgetPage(EXBaseWnd *pWnd, unsigned int wid, unsigned short n)
    : EXWidget(pWnd, wid, (unsigned int)-1, 0, 0), m_Back(pWnd, wid, 6)
{
    // TODO
    Notify(EXWN_CREATE);
}
