#include "EXItemHandler.h"

EX_IMPLEMENT_DYNAMIC_ROOT(EXItemHandler)

EXItemHandler::EXItemHandler() { m_pItem = 0; }
EXItemHandler::~EXItemHandler() {}
s8 EXItemHandler::Initialise() { return 1; }
s8 EXItemHandler::DoUpdate() { return 1; }
s8 EXItemHandler::DoPostUpdate() { return 1; }
u32 EXItemHandler::DoScriptCmdEvent(GEOSCRIPT_EVENT *, EXItemAnimator *) { return 0; }
u32 EXItemHandler::DoScriptCmdRunControl(GEOSCRIPT_RUNCONTROL *, EXItemAnimator *) { return 5; }
u32 EXItemHandler::DoScriptCmdObject(GEOSCRIPT_BASECMD *, EXItemAnimator *) { return 0; }
u32 EXItemHandler::DoAnimModeFunction(EXGeoAnimSetHeader *, EXGeoAnimModeFunction *, EXItemAnimator *) { return 0; }
