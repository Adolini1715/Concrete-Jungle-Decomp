#ifndef EXLINESEG_H
#define EXLINESEG_H
#include "EXVector.h"

// TODO layout (EXCollision area): a line segment as origin + direction (Traffic.obj usage).
class EXLineSeg {
public:
    EXVector m_origin;  // 0x00
    EXVector m_dir;     // 0x10
};

#endif
