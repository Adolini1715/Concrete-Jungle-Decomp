#include "EXSfxCmd_Parser.h"
#include "EXAudio.h"

extern "C" void *__cdecl memcpy(void *, const void *, unsigned int);

static char s_BatchBuffer1[0x4000];
static char s_BatchBuffer0[0x4000];

void XB_EXSfxCmd_Parser::Init()
{
    m_pBatch[0] = 0;
    m_pBatch[1] = 0;
    m_pBuffer[0] = s_BatchBuffer0;
    m_pBuffer[1] = s_BatchBuffer1;
}

void XB_EXSfxCmd_Parser::SendBatchCommands(char *pCmds, int size)
{
    for (;;) {
        for (int i = 0; i < 2; i++) {
            if (!m_pBatch[i]) {
                char *pBuf = m_pBuffer[i];
                if (pBuf) {
                    memcpy(pBuf, pCmds, size);
                    m_pBatch[i] = pBuf;
                    return;
                }
                break;
            }
        }
        DEBUGTRACE("***** ERROR: Both Batch Buffers FULL (Sending too many commands!!) *****\n");
    }
}
