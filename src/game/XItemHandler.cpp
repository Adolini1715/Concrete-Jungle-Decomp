#include "XItemHandler.h"
#include "EXMemory.h"
#include "FEText.h"

EX_IMPLEMENT_DYNCREATE_ROOT(XItemHandler)  // record names no base

XItemHandler::XItemHandler() {}
XItemHandler::~XItemHandler() {}
s8 XItemHandler::Initialise() { return 1; }
s8 XItemHandler::DoUpdate() { return 1; }
u32 XItemHandler::DoScriptCmdObject(GEOSCRIPT_BASECMD *, EXItemAnimator *) { return 0; }
