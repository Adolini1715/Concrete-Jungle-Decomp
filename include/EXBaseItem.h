#ifndef EXBASEITEM_H
#define EXBASEITEM_H
#include "types.h"
#include "EXList.h"
#include "EXFlag.h"

// Root of the scene-item hierarchy (no runtime-class record). Items form a tree: each item is
// linked into its owner's node list through m_node.
class EXBaseItem {
public:
    EXBaseItem() : m_pOwner(0)
    {
        m_node.Init();
        m_itemFlags = 0;
    }
    virtual ~EXBaseItem() = 0;  // pure with a body (vtable slot is __purecall), EXItem.obj
    EXDList &Nodes() { return m_nodes; }
    void SetOwner(EXBaseItem *pOwner) { m_pOwner = pOwner; }
    EXBaseItem *GetOwner() const { return m_pOwner; }
    EXBaseItem *GetBaseOwner() const;
    // Next sibling in the owner's node list (not in the map: always inlined).
    EXBaseItem *NextItem() const { return m_node.m_pNext ? (EXBaseItem *)((char *)m_node.m_pNext - 4) : 0; }
    EXBaseItem *FirstItem() const { return m_nodes.m_pHead ? (EXBaseItem *)((char *)m_nodes.m_pHead - 4) : 0; }
public:
    EXDListItem m_node;     // +4 link in the owner's m_nodes
    EXBaseItem *m_pOwner;   // +0xc
    ex::EXFlag<u16> m_itemFlags; // +0x10
    EXDList m_nodes;        // +0x14 child items
};
#endif
