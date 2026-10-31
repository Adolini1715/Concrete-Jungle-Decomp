#include "XItemHandler_ClayPigeon.h"
#include "XItemHandler_Objective.h"
#include "XItem.h"
#include "XApp.h"
#include "EXRand.h"
#include "EXGeoFile.h"
#include "EXItemAnimator.h"
#include "EXQuaternion.h"
#include "EXMemory.h"
#include "FEText.h"
#include <math.h>

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_ClayPigeonGenerator)

XItemHandler_ClayPigeonGenerator::~XItemHandler_ClayPigeonGenerator() {}

s8 XItemHandler_ClayPigeonGenerator::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    m_Remaining = 10;
    m_bPigeonActive = 0;
    m_bFirstUpdate = 1;
    return 1;
}

void XItemHandler_ClayPigeonGenerator::ProcessPigeonDestroyed()
{
    m_bPigeonActive = 0;
    m_Remaining--;
}

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_ClayPigeon)

XItemHandler_ClayPigeon::~XItemHandler_ClayPigeon() {}

s8 XItemHandler_ClayPigeon::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    m_pGenerator = 0;
    m_Time = g_EXRandClass.Randf() * 6.0f;
    m_Radius = (g_EXRandClass.Randf() + 1.0f) * 3.0f;
    m_Angle = g_EXRandClass.Randf() * 6.2f;
    return 1;
}

s8 XItemHandler_ClayPigeon::DoUpdate()
{
    m_Time += XApp::FrameLength();
    EXVector p = m_pGenerator->Position();
    float s = sin(m_Angle);
    EXVector out;
    out.x = p.x + sin(m_Time) * m_Radius * s;
    out.z = p.z + cos(m_Time) * m_Radius * s;
    out.y = p.y + cos(m_Angle) * m_Radius;
    out.w = p.w;
    m_Angle += XApp::FrameLength() * 1.5f;
    m_pItem->SetPosition(out);
    return 1;
}

s8 XItemHandler_ClayPigeonGenerator::DoUpdate()
{
    if (m_bFirstUpdate) {
        m_bFirstUpdate = 0;
        for (int i = 0; i < 8; i++) m_pObjectives[i] = Mission_GetMissionItem(m_ObjectiveIds[i]);
    }
    if (m_Remaining <= 0) {
        XItem **pp = m_pObjectives;
        for (int i = 8; i; i--, pp++) {
            if (*pp) {
                XItemHandler_Objective *pObj = (XItemHandler_Objective *)(*pp)->ItemHandler();
                if (pObj->m_State == 1) pObj->SetFlags(0x80);
            }
        }
        m_pItem->ItemFlags() = 0x10;
        return 1;
    }
    if (!m_bPigeonActive) {
        m_bPigeonActive = 1;
        EXGeoFile *pFile = EXGeoFile::LoadGeoFile(0x10000cc, 0, 0);
        XItem *p = XItem::CreateItem(pFile, 0x10000cc, EXRUNTIME_CLASS(XItemHandler_ClayPigeon),
                                     EXVector(0.0f, 0.0f, 0.0f, 0.0f));
        p->SetClipType(2);
        p->AddAnimator(EXItemAnimator::CreateFrom(pFile, 0x2000105));
        p->SetPosition(m_pItem->Position());
        p->m_rotation = EXVector(0.0f, 0.0f, 0.0f, 0.0f);
        p->Invalidate();
        p->SetMapOn(theItemEnv.GetMapOn(p->Position()));
        p->SetScale(EXVector(0.5f, 0.5f, 0.5f, 0.0f));
        theItemEnv.AddItem(p, 2);
        ((XItemHandler_ClayPigeon *)p->ItemHandler())->m_pGenerator = (XItem *)m_pItem;
    }
    return 1;
}

void XItemHandler_ClayPigeon::ApplyDamage(float damage)
{
    if (m_pGenerator) {
        ((XItemHandler_ClayPigeonGenerator *)m_pGenerator->ItemHandler())->ProcessPigeonDestroyed();
    }
    m_pItem->ItemFlags() = 0x10;
}
