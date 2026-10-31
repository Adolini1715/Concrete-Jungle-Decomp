#ifndef EXITEMRENDER_H
#define EXITEMRENDER_H
#include "types.h"
#include "EXRuntimeClass.h"

class EXBaseItem;

// Render component of an item (sizeof 0xc).
class EXItemRender {
    EX_DECLARE_DYNAMIC(EXItemRender)
public:
    virtual ~EXItemRender() {}
    EXItemRender();
    static EXItemRender *CreateObject();
    void SetOwner(EXBaseItem *pOwner) { m_pOwner = pOwner; }
    EXBaseItem *GetBaseOwner() const;
protected:
    EXBaseItem *m_pOwner;   // +4 (not set by the constructor)
    u32 m_08;               // +8 TODO name
};
#endif
