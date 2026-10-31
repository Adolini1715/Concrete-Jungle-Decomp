#ifndef EXGEOINSTANCE_H
#define EXGEOINSTANCE_H
#include "types.h"
#include "EXArray.h"
#include "EXMatrix.h"

class EXGeoHeader;
class EXGDiInstanceData;

// One instance's data in an instance buffer (0x84).
class EXInstanceInfo {
public:
    u8 m_00[0x84];
};

// Instances of one entity collected for a frame (sizeof 0x60).
class EXInstanceEnv {
public:
    EXInstanceEnv();
    ~EXInstanceEnv();
    u32 m_Key;                      // 0x00
    u32 m_Hash;                     // 0x04 entity hash
    u32 m_Count;                    // 0x08 instances used this frame
    u32 m_Life;                     // 0x0c frames until the buffer is freed
    EXMatrix m_Parent;              // 0x10
    u8 m_HasParent;                 // 0x50
    u8 m_51[3];
    EXGeoHeader *m_pHeader;         // 0x54
    EXInstanceInfo *m_pBuffer;      // 0x58 200 entries
    EXGDiInstanceData *m_pGDi;      // 0x5c
};

class EXInstanceManager {
public:
    EXInstanceManager();
    ~EXInstanceManager();
    void ReduceInstanceBuffer(unsigned int n);
    void SetParent(const EXMatrix &m);
    void DeleteManager();
    signed char AddInstanceBuffer(EXInstanceEnv *pEnv);
    void RenderInstances();
    void ResetInstanceManager();
    signed char AllocateManager();
    EXInstanceInfo *Instance(EXGeoHeader *pHdr, unsigned int hash, unsigned int key);
    EXInstanceInfo *AllocInstanceBuffer(EXGeoHeader *pHdr, unsigned int hash, unsigned int key, unsigned int count);

    EXDataArray<EXInstanceEnv, const EXInstanceEnv &> m_Envs;  // 0x00
    EXPtrArray<EXInstanceEnv> m_Buffers;                       // 0x10
    u32 m_NumEnvs;                  // 0x20
    u32 m_NumBuffers;               // 0x24
    u32 m_MaxEnvs;                  // 0x28
    u32 m_MaxBuffers;               // 0x2c
    EXInstanceEnv *m_pCurrent;      // 0x30
};
#endif
