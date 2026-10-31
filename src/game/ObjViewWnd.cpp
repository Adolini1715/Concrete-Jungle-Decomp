#include "XObjViewWnd.h"
#include "EXBaseApp.h"
#include "EXMemory.h"
#include "FEText.h"

XObjViewWnd::XObjViewWnd()
{
    m_Unk434 = 45.0f;
}

s8 XObjViewWnd::MenuSelect(unsigned int id)
{
    switch (id) {
    case 0x10001:
        break;
    case 0x10002: {
        EXStringArray &args = EXBaseApp::GetApp()->Args();
        for (int i = 0; i < args.Size(); i++) {
            if (args[i] == EXString("viewer")) {
                args.RemoveAt(i, 1);
                break;
            }
        }
        EXBaseApp::GetApp()->Restart();
        break;
    }
    case 0x10003:
        EXBaseApp::GetApp()->Restart();
        return 1;
    default:
        return EXGeoViewerWnd::MenuSelect(id);
    }
    return 1;
}
