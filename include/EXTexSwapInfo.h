#ifndef EXTEXSWAPINFO_H
#define EXTEXSWAPINFO_H
#include "types.h"
#include "EXArray.h"
class EXGeoFile;
class EXTexture;

// One texture swap (sizeof 0x10). m_Src is the hash of the texture to replace (-2: every texture,
// kept first in the list). The replacement is texture m_Tex of geo file m_FileHash (looked up into
// m_pFile and turned into an index on first use), or, when m_FileHash is -1, the EXTexture held in
// m_pFile.
struct EXTexSwapEntry {
    u32 m_Src;          // 0x00
    u32 m_FileHash;     // 0x04
    EXGeoFile *m_pFile; // 0x08
    u32 m_Tex;          // 0x0c (top bit set: already an index)
};

// Texture-swap state (EXRenderEnv.obj).
class EXTexSwapInfo {
public:
    EXTexSwapInfo();
    ~EXTexSwapInfo();
    void SetSwap(unsigned int src, unsigned int file, unsigned int tex);
    void SetSwap(unsigned int src, EXGeoFile *pFile, unsigned int tex);
    void SetSwap(unsigned int src, EXTexture *pTex);
    void SetCatchAllSwap(unsigned int file, unsigned int tex);
    void SetCatchAllSwap(EXGeoFile *pFile, unsigned int tex);
    void SetCatchAllSwap(EXTexture *pTex);
    signed char StopSwap(unsigned int src);
    signed char StopCatchAllSwap();
    void StopAllSwaps();
    EXTexture *SwappedTexture(EXTexture *p);
    unsigned int ChangeNum() const { return m_ChangeNum; }
    unsigned int SwapCount() const { return m_Swaps.Size(); }
protected:
    void ResolveHashCodes(EXTexSwapEntry &e) const;
public:
    u32 m_ChangeNum;                                            // 0x00 bumped on every change
    EXDataArray<EXTexSwapEntry, const EXTexSwapEntry &> m_Swaps;  // 0x04
};
#endif
