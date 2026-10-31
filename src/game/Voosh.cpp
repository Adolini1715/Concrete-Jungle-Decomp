#include "XItemHandler_Voosh.h"
#include "XItem.h"
#include "EXDatum.h"
#include "EXMatrix.h"
#include "Actor.h"
#include "ActorUtil.h"
#include "XItemHandler_Player.h"
#include "EXMemory.h"
#include "FEText.h"

static float s_VooshDrag = 0.9f;
static float s_VooshGravity = 0.009f;
static float s_VooshSpeed = 0.2f;
static float s_VooshLife = 325.0f;
static float s_VooshFade = 13.7f;

XItemList gVooshList;

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler_Voosh)

XItemHandler_Voosh::~XItemHandler_Voosh() { gVooshList.Remove((XItem *)m_pItem); }

s8 XItemHandler_Voosh::Initialise()
{
    if (!EXItemHandler::Initialise()) return 0;
    gVooshList.Add((XItem *)m_pItem);
    return 1;
}

XItem *XItemHandler_Voosh::Voosh_Create(int actorId, unsigned int datum, int life)
{
    XItem *p = XItem::CreateItem(0, 0, EXRUNTIME_CLASS(XItemHandler_Voosh), EXVector(0.0f, 0.0f, 0.0f, 0.0f));
    theItemEnv.AddItem(p, 0);
    ((XItemHandler_Voosh *)p->ItemHandler())->m_Life = life;
    ((XItemHandler_Voosh *)p->ItemHandler())->m_ActorId = actorId;
    ((XItemHandler_Voosh *)p->ItemHandler())->m_Datum = datum;
    ((XItemHandler_Voosh *)p->ItemHandler())->m_Head = 0;
    ((XItemHandler_Voosh *)p->ItemHandler())->m_Count = 0;
    return p;
}

void XItemHandler_Voosh::Add(EXDatumTemp &d)
{
    int head = m_Head;
    int prev = (head - 1) & 31;
    EXMaths::vec_xyzw v;
    v.set(0.0f, s_VooshSpeed, 0.0f, 0.0f);
    EXMatrix m;
    d.m_rotation.GetRotation(m);
    v = (const EXMaths::mat_44 &)m * v;
    if (m_Count > 0) {
        EXVector diff = m_Points[prev].m_Pos - d.m_position;
        m_Points[head].m_Unk20 = m_Points[prev].m_Unk20 - diff.LengthXYZ() * 0.5f;
        if (-8.0f > m_Points[head].m_Unk20) m_Points[head].m_Unk20 += 16.0f;
    } else {
        m_Points[head].m_Unk20 = 0.0f;
    }
    m_Points[head].m_Pos = d.m_position;
    m_Points[head].m_Vel.set(v);
    m_Points[head].m_Life = s_VooshLife;
    m_Head++;
    if (++m_Count >= 32) m_Count = 31;
    m_Head &= 31;
}

// NEAR: 38 bytes; retail addresses the ring element as this + i*40 with member displacements, ours
// folds the +0x1c array offset into the lea.
void XItemHandler_Voosh::Update()
{
    int idx = (m_Head - 1) & 31;
    if (0.0f > m_Points[idx].m_Life) m_pItem->ItemFlags() = 0x10;
    for (int i = 0; i < m_Count; i++) {
        _VOOSH &v = m_Points[idx &= 31];
        v.m_Pos += v.m_Vel;
        v.m_Vel *= EXVector(s_VooshDrag, s_VooshDrag, s_VooshDrag, s_VooshDrag);
        v.m_Vel -= EXVector(0.0f, s_VooshGravity, 0.0f, 0.0f);
        v.m_Life -= s_VooshFade;
        idx--;
    }
}

s8 XItemHandler_Voosh::DoUpdate()
{
    if (m_ActorId) {
        EXItem *pItem;
        if (m_ActorId > 0) {
            Actor *pActor = Actor::GetPtrFromID(m_ActorId);
            if (!pActor || pActor->SearchBrainStateStack(STATE_FadeOutDead)) goto update;
            pItem = pActor->GetItem();
        } else {
            pItem = XItemHandler_Player::Instance()->GetItem();
        }
        if (pItem) {
            EXDatumTemp d;
            pItem->GetAnimatorDatum(m_Datum, d, 0, 0);
            m_pItem->SetPosition(d.m_position);
            m_pItem->SetMapOn(theItemEnv.GetMapOn(m_pItem->Position()));
            Add(d);
        }
    update:
        Update();
    } else {
        m_pItem->ItemFlags() = 0x10;
    }
    if (m_Life < 0) m_pItem->ItemFlags() = 0x10;
    return 1;
}

void Draw_Voosh()
{
    for (XItem *p = gVooshList.GetFirst(); p; p = gVooshList.GetNext(p)) {
        EXItemHandler *pHandler = p->ItemHandler();
        if (pHandler->IsKindOf(EXRUNTIME_CLASS(XItemHandler_Voosh))) ((XItemHandler_Voosh *)pHandler)->Render();
    }
}

// TODO: XItemHandler_Voosh::Render (1968 bytes, builds and draws the trail strip) not written yet.
