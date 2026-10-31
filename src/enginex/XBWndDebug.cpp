#include "EXWnd.h"
#include "LineController.h"
#include "RGBA.h"
#include "EXMemory.h"

// TODO: AddLines, RenderLines, the EXWnd debug primitives (_Line2D ... _Text3D, DrawDebugText) and the
// static D3DDevice_* helpers are not done yet.

LineController LineControl;

void LineController::AddLine(D3DV_L *a, D3DV_L *b)
{
    if (m_NumLines >= 0x1000) return;
    m_Verts[m_NumVerts] = *a;
    m_Verts[m_NumVerts + 1] = *b;
    ex::RGBA c = m_Verts[m_NumVerts].m_Color;
    m_Verts[m_NumVerts].m_Color = c.argb();
    c = m_Verts[m_NumVerts + 1].m_Color;
    m_Verts[m_NumVerts + 1].m_Color = c.argb();
    m_NumLines++;
    m_NumVerts += 2;
}

LineController::LineController()
{
    m_NumLines = 0;
    m_NumVerts = 0;
}
