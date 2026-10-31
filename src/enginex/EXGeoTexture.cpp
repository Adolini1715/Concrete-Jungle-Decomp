#include "EXGeoTexture.h"
#include "EXGeoHeader.h"

void EXBaseGeoTexture::SetRuntimeValues(unsigned short w, unsigned short h, unsigned char fmt, unsigned short gameFlags, short su, short sv)
{
    m_Width = w;
    m_Height = h;
    m_Depth = 1;
    m_GameFlags = gameFlags;
    m_ScrollU = su;
    m_ScrollV = sv;
    m_FrameCount = 1;
    m_ImageCount = 1;
    m_FrameRate = 1;
    m_0f = 0;
    m_ValueFlags = 0;
    m_RegionsCount = 0;
    m_MipCount = 1;
    m_Format = fmt;
    m_GeoFileHash = 0xffffffff;
    *(u32 *)&m_Colour = 0;
}

int EXGeoTexture::Value(int i) const
{
    int *pValues = m_Values;
    u32 flags = m_ValueFlags;
    if (!(flags & (1 << i))) return 0;
    int n = 0;
    for (int b = 0; b < i; b++)
        if (flags & (1 << b)) n++;
    return pValues[n];
}

EXGeoTexture_Region *EXGeoTexture::Region(unsigned int hash) const
{
    int idx = EXGeoHashIndex(hash);
    if (idx >= 0) {
        if ((u32)idx < m_RegionsCount) return &RegionsData()[idx];
        return 0;
    }
    for (int i = 0; i < m_RegionsCount; i++) {
        EXGeoTexture_Region *pRegion = &RegionsData()[i];
        if (pRegion->m_HashCode == hash) return pRegion;
    }
    return 0;
}

int EXGeoTexture_Region::Value(int i) const
{
    int *pValues = m_Values;
    u32 flags = m_ValueFlags;
    if (!(flags & (1 << i))) return 0;
    int n = 0;
    for (int b = 0; b < i; b++)
        if (flags & (1 << b)) n++;
    return pValues[n];
}

void EXGeoTextureHeader::SetRuntimeValues(EXGeoTexture *pTex, unsigned short a, unsigned short b, unsigned int c, unsigned int d)
{
    m_HashCode = 0;
    m_Flags = 0;
    m_Debug = 0;
    m_Addr[0].m_pObject = pTex;
    m_10 = a;
    m_12 = b;
    m_14 = c;
    m_Flags18 = d;
}

void EXGeoMaterial::Initialise(const EXGeoHeader *pHdr)
{
    m_pTextureHeader = pHdr->GeoTextureHeader(m_TextureHash);
}
