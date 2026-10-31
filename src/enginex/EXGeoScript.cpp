#include "EXGeoScript.h"

EXBoundsBox &EXGeoScript::GetBoundsBox() { return m_BoundsBox; }

signed char EXGeoScript::GetFirstCmdObject(unsigned int type, unsigned int &a, unsigned int &b) const
{
    if (type != 6 && type != 3 && type != 4 && type != 2) return 0;
    int i = 0;
    const GEOSCRIPT_CMD *pCmd;
    do {
        pCmd = (const GEOSCRIPT_CMD *)&m_Data[i];
        if (pCmd->m_Type == type) {
            if (type == 2) {
                a = pCmd->m_1c;
                b = pCmd->m_20;
            } else {
                a = pCmd->m_10;
                b = pCmd->m_14;
            }
            return 1;
        }
        i += pCmd->m_Size;
    } while (pCmd->m_Type != 0x12);
    return 0;
}
