#include "EXWnd.h"
#include "EXStdFuncs.h"
#include "EXMemory.h"

// TODO stand-in: geometry file / header (EXGeoFile.obj, EXGeoHeader area).
class EXGeoHeader {
public:
    EXTexture *Texture(unsigned int hash) const;
};
class EXGeoFile {
public:
    u8 m_Unk00[0x20];
    EXGeoHeader *m_pHeader;  // 0x20
    EXGeoHeader *Header() const { return m_pHeader; }
};

// Sprite command block: header then 0x1ff0 bytes of queued sprite data.
struct EXSprite2DBlock {
    u32 m_Used;     // 0x00
    int m_Life;     // 0x04 frames left before the block is freed
    s8 m_Full;      // 0x08
    u8 m_Data[0x1ff7];  // 0x09 (data starts at 0x10)
};

void EXBaseWnd::SelectSprite2DTexture(const EXGeoFile *pFile, unsigned int hash, s8 a, s8 b)
{
    if (pFile && pFile->Header()) {
        EXTexture *pTex = pFile->Header()->Texture(hash);
        _SelectSprite2DTexture(pTex, a, b);
    }
}

void EXBaseWnd::DeleteSprite2DData()
{
    for (u32 i = 0; i < m_Sprite2DBlocks.Size(); i++) {
        EXFree(m_Sprite2DBlocks[i]);
        m_Sprite2DBlocks[i] = 0;
    }
    m_Sprite2DBlocks.RemoveAll();
}

void EXBaseWnd::ClearSprite2DData()
{
    for (u32 i = 0; i < m_Sprite2DBlocks.Size(); i++) {
        EXSprite2DBlock *pBlock = (EXSprite2DBlock *)m_Sprite2DBlocks[i];
        if (pBlock) {
            if (pBlock->m_Life-- <= 0) {
                EXFree(m_Sprite2DBlocks[i]);
                m_Sprite2DBlocks[i] = 0;
            } else {
                pBlock->m_Used = 0;
                pBlock->m_Full = 0;
            }
        }
    }
    int n = m_Sprite2DBlocks.Size();
    if (n > 0 && !m_Sprite2DBlocks[n - 1]) m_Sprite2DBlocks.SetSize(n - 1, -1);
}

void EXBaseWnd::AddSprite2DData(const void *pData, unsigned int size)
{
    static s8 s_Once;
    if (!s_Once) s_Once = 1;
    EXSprite2DBlock *pBlock;
    u32 i;
    for (i = 0; i < m_Sprite2DBlocks.Size(); i++) {
        pBlock = (EXSprite2DBlock *)m_Sprite2DBlocks[i];
        if (pBlock && !pBlock->m_Full) {
            if (0x1ff0 - pBlock->m_Used > size) goto found;
            pBlock->m_Full = 1;
        }
    }
    for (i = 0; i < m_Sprite2DBlocks.Size(); i++) {
        if (!m_Sprite2DBlocks[i]) {
            pBlock = (EXSprite2DBlock *)_EXAlloc(0x2000, 0);
            pBlock->m_Used = 0;
            pBlock->m_Life = 0;
            pBlock->m_Full = 0;
            m_Sprite2DBlocks[i] = (unsigned char *)pBlock;
            goto found;
        }
    }
    pBlock = (EXSprite2DBlock *)_EXAlloc(0x2000, 0);
    pBlock->m_Used = 0;
    pBlock->m_Life = 0;
    pBlock->m_Full = 0;
    m_Sprite2DBlocks.Add((unsigned char *)pBlock);
found:
    memcpy((u8 *)pBlock + 0x10 + pBlock->m_Used, pData, size);
    pBlock->m_Used += size;
    pBlock->m_Life = 0x20;
}

void *EXBaseWnd::AllocSprite2DData(unsigned int size)
{
    EXSprite2DBlock *pBlock;
    u32 i;
    for (i = 0; i < m_Sprite2DBlocks.Size(); i++) {
        pBlock = (EXSprite2DBlock *)m_Sprite2DBlocks[i];
        if (pBlock && !pBlock->m_Full) {
            if (0x1ff0 - pBlock->m_Used > size) goto found;
            pBlock->m_Full = 1;
        }
    }
    for (i = 0; i < m_Sprite2DBlocks.Size(); i++) {
        if (!m_Sprite2DBlocks[i]) {
            pBlock = (EXSprite2DBlock *)_EXAlloc(0x2000, 0);
            pBlock->m_Used = 0;
            pBlock->m_Life = 0;
            pBlock->m_Full = 0;
            m_Sprite2DBlocks[i] = (unsigned char *)pBlock;
            goto found;
        }
    }
    pBlock = (EXSprite2DBlock *)_EXAlloc(0x2000, 0);
    pBlock->m_Used = 0;
    pBlock->m_Life = 0;
    pBlock->m_Full = 0;
    m_Sprite2DBlocks.Add((unsigned char *)pBlock);
found:
    void *p = (u8 *)pBlock + 0x10 + pBlock->m_Used;
    pBlock->m_Used += size;
    pBlock->m_Life = 0x20;
    return p;
}
