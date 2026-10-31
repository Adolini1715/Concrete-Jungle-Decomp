#include "EXGeoTexture.h"
#include "EXGeoFile.h"

EXGeoTexture *EXGeoTexture::_BaseTexture()
{
    if (!m_pGeoFile) {
        m_pGeoFile = EXGeoFile::GetGeoFile(m_GeoFileHash);
        if (!m_pGeoFile) return m_pBaseTexture;
    }
    u32 code = m_pGeoFile->CheckCode();
    if (m_CheckCode == code) return m_pBaseTexture;
    EXGeoHeader *pHdr = m_pGeoFile->GeoHeader();
    if (pHdr) {
        if (m_TextureRef & 0x80000000) m_TextureRef = pHdr->GeoTextureIndex(m_TextureRef);
        m_pBaseTexture = pHdr->GeoTexture(m_TextureRef);
        if (m_pBaseTexture) {
            if (m_CheckCode == 0xffffffff) {
                EXGeoTexture *pBase = m_pBaseTexture;
                m_Width = pBase->m_Width;
                m_Height = pBase->m_Height;
                m_Depth = pBase->m_Depth;
                m_FrameCount = pBase->m_FrameCount;
                m_ImageCount = pBase->m_ImageCount;
                m_MipCount = pBase->m_MipCount;
                m_MipBias = pBase->m_MipBias;
                m_Format = pBase->m_Format;
                m_Size = pBase->m_Size;
            }
            m_CheckCode = code;
        }
        return m_pBaseTexture;
    }
    return 0;
}
