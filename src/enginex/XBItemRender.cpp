#include "EXItemRender_Anim.h"

EXMatrix *EXItemRender_SkinAnim::TempMtxArray(int i) const
{
    if (!(m_tempMtxBits.m_bits[i / 32] >> (i & 31) & 1)) return 0;
    return &m_pMtx[i];
}

void EXItemRender_SkinAnim::CalcMatrix()
{
    m_tempMtxBits.ClrAll();
    CalcSectionMatrix((EXItemRender_SkinAnimSection *)m_sections.m_pHead, m_pMtx);
    if (m_08) ((void (__cdecl *)(int, EXItemRender *))m_08)(1, this);
    m_10 = 1;
}
