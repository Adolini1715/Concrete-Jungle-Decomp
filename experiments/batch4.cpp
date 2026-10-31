typedef signed char s8; typedef unsigned short u16; typedef unsigned int u32; typedef unsigned char u8;
class XNavNode { public: u8 pad[0x10]; u16 m_Id; u8 pad2[0x16 - 0x12]; u16 m_nLinks; u8 pad3[0x20 - 0x18]; u16 *m_pLinks; };
class XNavLink { public: u16 m_0, m_2, m_NodeA, m_NodeB; };
class XNavSection {
public:
    XNavLink *Link(u16 i);
    XNavNode *Node(u16 i);
};
class XNavNetwork {
public:
    u8 pad[8];
    XNavSection m_Section;
    XNavNode *ConnectedNodePtr(XNavNode *pNode, u16 i, XNavLink **ppLink);
};
XNavNode *XNavNetwork::ConnectedNodePtr(XNavNode *pNode, u16 i, XNavLink **ppLink)
{
    if (i >= pNode->m_nLinks || !pNode->m_pLinks) return 0;
    u32 li = pNode->m_pLinks[i * 2];
    if ((u16)li == 0xffff) return 0;
    XNavLink *l = m_Section.Link(li);
    *ppLink = l;
    if (l->m_NodeA == 0xffff || l->m_NodeB == 0xffff) return 0;
    if (l->m_NodeA == pNode->m_Id) return m_Section.Node(l->m_NodeB);
    return m_Section.Node(l->m_NodeA);
}

class EXMatrix { public: float m[16]; };
class EXGeoEntity_InfoArray;
class EXItemRender_SkinAnim { public: void BuildEntityInfoArray(const EXMatrix &m, EXGeoEntity_InfoArray &a) const; };
class EXPlacementItem {
public:
    virtual ~EXPlacementItem();
    const EXMatrix &CalcMatrix_InPlace(const EXMatrix &parent, u32 &flags);
    u8 pad[0x88 - 4];
    EXMatrix m_Matrix;
};
class EXItemAnimator_Anim : public EXPlacementItem {
public:
    virtual void BuildEntityInfoArray(const EXMatrix &m, EXGeoEntity_InfoArray &a, s8 inPlace);
    u8 pad2[0x104 - 0x88 - sizeof(EXMatrix)];
    EXItemRender_SkinAnim *m_pRender;
};
void EXItemAnimator_Anim::BuildEntityInfoArray(const EXMatrix &m, EXGeoEntity_InfoArray &a, s8 inPlace)
{
    if (!m_pRender) return;
    u32 flags = 0;
    const EXMatrix &mat = inPlace ? CalcMatrix_InPlace(m, flags) : m_Matrix;
    m_pRender->BuildEntityInfoArray(mat, a);
}
