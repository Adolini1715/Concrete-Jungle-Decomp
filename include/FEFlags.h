#ifndef FEFLAGS_H
#define FEFLAGS_H
#include "types.h"

// Front-end enum scopes. The map mangles them as `W4Flags@FEObjType@@` etc.; value names are ours,
// values come from the code that uses them.
namespace FEObjType {
enum Flags {
    OT_NONE = 0,
    OT_SPRITE = 1,   // Init: data is a texture (file, id) pair
    OT_TEXT = 2,     // Init: data is a font (file, id) pair
    OT_SCRIPT = 3,   // Init: data is a script, attached through ScriptAdd
    OT_SCRIPT2 = 4
};
}
namespace FEObjColour {
enum Flags { OC_FOCUS = 0, OC_NORMAL = 1, OC_TINT = 2, OC_ALPHA = 3, OC_COUNT = 4 };
}
namespace FEMathmosPos {
enum Flags { MP_NONE = 0, MP_START = 1, MP_END = 2 };
}
namespace FEObjEvent {
enum Flags {
    OE_NONE = 0,
    OE_FOCUSON = 9,
    OE_FOCUSOFF = 10,
    OE_SHOW = 0x10,
    OE_HIDE = 0x11,
    OE_KEYFRAMESDONE = 0x12,
    OE_UPDATE = 0x18
};
}
namespace FEDataType {
enum Flags { DT_NONE = 0, DT_S8 = 1, DT_U8 = 2, DT_INT = 3, DT_U32 = 4, DT_U64 = 5 };
}
namespace FERenderPass {
enum Flags { RP_0 = 0, RP_1 = 1, RP_2 = 2 };
}
namespace FEWaveform {
enum Flags { WF_NONE = 0, WF_LINEAR = 1, WF_6 = 6 };
}
namespace FEPad {
enum Flags { PAD_0 = 0 };
}
namespace FERemap {
enum Flags { RM_0 = 0 };
}
namespace FEDirection {
enum Flags { DIR_NONE = 0 };
}

// Unset/"use default" marker used across the front end (0xfeffffff).
#define FE_DEFAULT 0xfeffffff
#endif
