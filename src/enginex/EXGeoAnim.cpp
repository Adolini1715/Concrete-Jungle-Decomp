#include "EXGeoAnim.h"
#include "EXGeoHeader.h"
#include "EXGeoFile.h"
#include "EXItemAnimator_Anim.h"
#include "EXItemCtrl_SnapShot.h"
#include "EXMemory.h"
#include "EXMemHeap.h"
#include "EXQuaternion.h"

unsigned int EXGeoBaseAnim::m_gOnCreateAnimSection;

EXBoundsBox &EXGeoBaseAnimSkin::GetBoundsBox() { return m_BoundsBox; }

void EXGeoBaseAnim::OnDelete(EXGeoHeader *, EXGeoObjectHeader *)
{
    int n = (m_NumFrames + m_FramesPerSeq - 1) / m_FramesPerSeq;
    for (int i = 0; i < n; i++) {
        if (m_SeqMemory[i]) {
            EXFree(m_SeqMemory[i]);
            m_SeqMemory[i] = 0;
        }
    }
}

EXBoundsBox &EXGeoBaseAnim::GetBoundsBox() { return m_BoundsBox; }

ex::EXBit128 EXGeoBaseAnim::SelectiveSectionFlags() const
{
    const ex::EXBit128 *p = (const ex::EXBit128 *)m_XtraData.GetXtra(3);
    ex::EXBit128 flags;
    if (p)
        flags.Set(*p);
    else
        flags.ClrAll();
    return flags;
}

void EXGeoBaseAnim::MorphGeoRefObject(EXGeoRefObject &ref) const
{
    if (m_SkinFileHash == 0xffffffff)
    {
        u32 hash = m_MorphHash;
        ref.m_pGeoFile = m_pGeoFile;
        ref.m_hashRef = hash;
    }
    else
        ref = EXGeoRefObject(EXGeoFile::GetGeoFile(m_MorphFileHash), m_MorphHash);
}

int EXGeoBaseAnimSkin::Value(int i) const
{
    const s32 *pTable = m_Values;
    u32 mask = m_Values.m_Value & 0xff;
    if (!(mask & (1 << i))) return 0;
    int n = 0;
    for (int b = 0; b < i; b++)
        if (mask & (1 << b)) n++;
    return pTable[n];
}

void EXGeoBaseAnim::OnCreate(EXGeoHeader *pHdr, EXGeoObjectHeader *pObjHdr)
{
    if ((u32)this & 0xf) for (;;) {}
    m_pGeoFile = pHdr->m_pGeoFile;
    m_gOnCreateAnimSection = ((EXGeoAnimHeader *)pObjHdr)->Section();
    m_Motion.m_Offset = (char *)((EXGeoAnimHeader *)pObjHdr)->MotionData() - (char *)&m_Motion;
    m_DataSize = ((EXGeoAnimHeader *)pObjHdr)->DataSize();
}

void EXGeoBaseAnim::GetTotalMotion(EXMotionData &m) const
{
    m.m_pos = EXVector(m_TotalPos[0], m_TotalPos[1], m_TotalPos[2], 0.0f);
    m.m_rot = *(const EXQuaternion *)m_TotalRot;
}

unsigned int EXGeoBaseAnim::FrameFlags(int frame) const
{
    int f = frame >= m_NumFrames - 1 ? m_NumFrames - 1 : (frame > 0 ? frame : 0);
    const u32 *pFlags = (const u32 *)m_XtraData.GetXtra(4);
    if (pFlags) return pFlags[f];
    return 0;
}

EXGeoSkinAnimFrame *EXGeoBaseAnim::ValidSeqMemoryPtr(const EXGeoHeader *, int frame) const
{
    int f = frame >= m_NumFrames - 1 ? m_NumFrames - 1 : (frame > 0 ? frame : 0);
    u8 *pMem = (u8 *)m_SeqMemory[f / m_FramesPerSeq];
    if (!pMem) return 0;
    u32 stride = ((u32)(((m_14 + m_15) * 4 + m_NumMorphs) * 4) + sizeof(EXGeoSkinAnimFrame) + 15) >> 4;
    return (EXGeoSkinAnimFrame *)(pMem + ((stride * (f % m_FramesPerSeq)) << 4));
}

EXGeoSkinAnimFrame *EXGeoBaseAnim::DecodeMorph(int frame, unsigned int) const
{
    EXGeoSkinAnimFrame *pFrame = AllocSeqMemory(m_pGeoFile->GeoHeader(), frame);
    if (pFrame && !pFrame->m_MorphDecoded) {
        pFrame->m_MorphDecoded = 1;
        if (m_NumMorphs) DecompressMorph(frame, (float *)((u8 *)pFrame + (m_14 + m_15 + 5) * 16));
    }
    return pFrame;
}

EXGeoSkinAnimFrame *EXGeoBaseAnim::AllocSeqMemory(const EXGeoHeader *, int frame) const
{
    u32 stride = ((((m_14 + m_15) * 4 + m_NumMorphs) * 4 + sizeof(EXGeoSkinAnimFrame)) + 15) & ~15;
    int numFrames = m_NumFrames;
    int f = frame >= numFrames - 1 ? numFrames - 1 : (frame > 0 ? frame : 0);
    int count = m_FramesPerSeq;
    int block = f / count;
    int index = f % count;
    EXGeoSkinAnimFrame **ppMem = (EXGeoSkinAnimFrame **)&m_SeqMemory[block];
    u8 *pMem = (u8 *)*ppMem;
    if (!pMem) {
        int left = numFrames - block * count;
        if (left < count) count = left;
        EXMemHeap *pHeap = _SystemHeapList[1];
        pMem = (u8 *)pHeap->_EXCacheDel(count * stride, 1, 0x3c, (void **)ppMem, 0);
        if (!pMem) {
            EXGeoSkinAnimFrame *pFrame = (EXGeoSkinAnimFrame *)pHeap->_EXCacheDel(stride, 1, 1, 0, 0);
            if (pFrame) {
                pFrame->m_Decoded = 0;
                pFrame->m_MorphDecoded = 0;
                pFrame->m_Frame = -1;
                pFrame->m_Bones.ClrAll();
            }
            return pFrame;
        }
        u8 *p = pMem;
        for (int i = 0; i < count; i++, p += stride) ((EXGeoSkinAnimFrame *)p)->Init();
    } else
        EXAllocSetTime(pMem, 1, 0x3c);
    return (EXGeoSkinAnimFrame *)(pMem + index * stride);
}

void EXGeoBaseAnim::SkinGeoRefObject(EXGeoRefObject &ref) const
{
    if (m_SkinFileHash == 0xffffffff) {
        EXGeoAnimHeader *pAnimHdr = m_pGeoFile->GeoHeader()->AnimList().Find(m_AnimIndex);
        u32 hash = pAnimHdr->m_1c | 0x8d000000;
        ref.m_pGeoFile = m_pGeoFile;
        ref.m_hashRef = hash;
    } else
        ref = EXGeoRefObject(EXGeoFile::GetGeoFile(m_SkinFileHash), m_SkinHash);
}

// TODO stand-in (render area; EXBaseRenderEnv.h belongs to the item layer): lod disable flag.
class EXBaseRenderEnv {
public:
    static s8 LodDisabled() { return ms_RenderInfo.FlagBits().m_lodDisabled; }
protected:
    static EXRenderInfo ms_RenderInfo;
};

const EXGeoAnimSkinHeader *EXGeoAnimSkinHeader::CalcAnimSkinLod(EXGeoComplexRefObject &ref, unsigned char &lod,
                                                                unsigned int minLod, unsigned int maxLod, float dist)
{
    EXGeoHeader *pHdr = ref.GeoFile()->GeoHeader();
    if (!pHdr) return 0;
    lod = 0;
    const EXGeoAnimSkinHeader *pSkinHdr = ref.GeoAnimSkinHeader();
    if (pSkinHdr && pSkinHdr->m_LodDistance != 0.0f && !EXBaseRenderEnv::LodDisabled()) {
        for (; lod < maxLod; lod++) {
            if (lod >= minLod) {
                float d = pSkinHdr->m_LodDistance;
                if (d == 0.0f) break;
                if (d > dist) break;
            }
            u32 next = pSkinHdr->m_MipRef;
            if (!next) break;
            pSkinHdr = &pHdr->AnimSkinList().Data()[next & 0xffffff];
        }
    }
    return pSkinHdr;
}

void EXGeoBaseAnimSkin::GetTextureHashList(const EXGeoHeader *pHdr, EXu32Array &list) const
{
    EXGeoEntityHeader *pEntities = pHdr->EntityList().Find(0);
    if (!pEntities) return;
    EXGeoTextureHeader *pTextures = pHdr->TextureList().Find(0);
    if (!pTextures) return;
    EXGeoEntity_InfoArray info;
    info.SetFlags(1);
    EXGeoSkinEntityRef *pRef = m_EntityGroups[0].Data();
    for (u32 i = 0; i < (u32)(m_EntityGroups[0].Size() + m_EntityGroups[1].Size() + m_EntityGroups[2].Size()); i++, pRef++) {
        EXGeoEntityHeader *pEntHdr = &pEntities[pRef->m_EntityHash & 0xffffff];
        EXGeoEntity *pEntity;
        if (pEntHdr && (pEntity = pEntHdr->m_Addr[0].m_pObject) != 0) {
            info.Empty(0);
            pEntity->BuildEntityInfoArray(info, 0);
            for (int j = 0; j < info.Size(); j++) {
                EXGeoEntity_TextureList *pTexList = info[j].m_pEntity->TextureList();
                for (int k = 0; k < pTexList->m_Count; k++) list.AddUnique(pTextures[pTexList->m_Textures[k]].HashCode());
            }
        }
    }
}

// Rotation order used to strip single rotation axes, by which of the x / y / z bits are set.
static const unsigned char s_StripRotOrder[8] = { 4, 4, 4, 4, 7, 3, 5, 4 };

// Decode the root stream and the bones in mask into pFrame, then mark them decoded.
void EXGeoBaseAnim::DecompressFrame(int frame, EXGeoSkinAnimFrame *pFrame, ex::EXBit128 &bones) const
{
    if (!m_14) {
        pFrame->m_Pos.x = m_BasePos[0];
        pFrame->m_Pos.w = 0.0f;
        pFrame->m_Pos.y = m_BasePos[1];
        pFrame->m_Pos.z = m_BasePos[2];
        pFrame->m_Rot.z = 0.0f;
        pFrame->m_Rot.y = 0.0f;
        pFrame->m_Rot.x = 0.0f;
        pFrame->m_Rot.w = 1.0f;
    } else {
        EXAnimStreamInfo info;
        ex::EXBit128 rootMask;
        info.m_pSections = m_XtraData.GetXtra(5);
        info.m_NumFrames0 = m_NumFrames;
        rootMask.Set(m_RootMask);
        EXMaths::vec_xyzw *pOut = (EXMaths::vec_xyzw *)((u8 *)pFrame + (m_14 + 4) * 16);
        info.m_Frame0 = frame;
        info.m_Frame1 = frame;
        info.m_SeekFrame = frame;
        info.m_StartFrame = 0;
        info.m_NumFrames = info.m_NumFrames0;
        DecompressData(MotionCompressedData(), m_14, frame, bones, rootMask, pOut, &pFrame->m_Root, info, 1);
        pFrame->m_Pos.x = m_BasePos[0] + pOut->x;
        pFrame->m_Pos.y = m_BasePos[1] + pOut->y;
        pFrame->m_Pos.z = m_BasePos[2] + pOut->z;
        pFrame->m_Pos.w = pOut->w;
        pFrame->m_Rot.x = pFrame->m_Root.x;
        pFrame->m_Rot.y = pFrame->m_Root.y;
        pFrame->m_Rot.z = pFrame->m_Root.z;
        pFrame->m_Rot.w = pFrame->m_Root.w;
    }
    pFrame->m_Bones |= bones;
    pFrame->m_Frame = -1;
}

void EXGeoBaseAnim::DecompressMorph(int frame, float *pOut) const
{
    EXAnimStreamInfo info;
    ex::EXBit128 channels;
    channels.SetAll();
    info.m_Frame0 = frame;
    info.m_Frame1 = frame;
    info.m_NumFrames0 = m_NumFrames;
    info.m_pSections = 0;
    info.m_StartFrame = 0;
    info.m_NumFrames = m_NumFrames;
    info.m_SeekFrame = frame;
    DecompressMorphData(MorphCompressedData(), m_NumMorphs, frame, channels, pOut, info, 1);
}

// Express pos / rot relative to the strip motion (apply == 0) or put it back on (apply != 0). Only
// the strip position axes selected by flags 0x100 / 0x200 / 0x400 take part.
void EXGeoBaseAnim::StripMotionData(const EXMotionData &strip, EXVector &pos, EXQuaternion &rot, unsigned int flags,
                                    signed char apply) const
{
    EXMatrix mtx;
    mtx.set_rotate_quat(rot);
    mtx.r[3].x = pos.x;
    mtx.r[3].y = pos.y;
    mtx.r[3].z = pos.z;
    mtx.r[3].w = 1.0f;
    EXMatrix stripMtx;
    stripMtx.set_rotate_quat(strip.m_rot);
    stripMtx.r[3].x = strip.m_pos.x;
    stripMtx.r[3].y = strip.m_pos.y;
    stripMtx.r[3].z = strip.m_pos.z;
    if (!(flags & 0x100)) stripMtx.r[3].x = 0.0f;
    if (!(flags & 0x200)) stripMtx.r[3].y = 0.0f;
    if (!(flags & 0x400)) stripMtx.r[3].z = 0.0f;
    stripMtx.r[3].w = 1.0f;
    if (!apply) {
        EXMatrix inv(stripMtx.fastinverse());
        mtx = EXMatrix(mtx.EXMaths::mat_44::operator*(inv));
    } else
        mtx = EXMatrix(stripMtx.EXMaths::mat_44::operator*(mtx));
    mtx.GetRotation(rot);
    pos.x = mtx.r[3].x;
    pos.y = mtx.r[3].y;
    pos.z = mtx.r[3].z;
    pos.w = mtx.r[3].w;
}

// Keep only the root motion axes selected by flags: 0x100 / 0x200 / 0x400 position x / y / z,
// 0x800 / 0x1000 / 0x2000 rotation x / y / z; 0x4000 then removes the anim's strip motion.
void EXGeoBaseAnim::DecompressMotion_StripMotion(EXMotionData &m, unsigned char, unsigned int flags) const
{
    if ((flags & 0x700) != 0x700) {
        if (!(flags & 0x700)) {
            m.m_pos.x = 0.0f;
            m.m_pos.y = 0.0f;
            m.m_pos.z = 0.0f;
            m.m_pos.w = 1.0f;
        } else {
            if (!(flags & 0x100)) m.m_pos.x = 0.0f;
            if (!(flags & 0x200)) m.m_pos.y = 0.0f;
            if (!(flags & 0x400)) m.m_pos.z = 0.0f;
        }
    }
    if ((flags & 0x3800) != 0x3800) {
        if (!(flags & 0x3800)) {
            m.m_rot.z = 0.0f;
            m.m_rot.y = 0.0f;
            m.m_rot.x = 0.0f;
            m.m_rot.w = 1.0f;
        } else {
            unsigned char order = s_StripRotOrder[(flags >> 11) & 7];
            EXVector angles;
            m.m_rot.GetRotation(angles, order);
            if (!(flags & 0x800)) angles.x = 0.0f;
            if (!(flags & 0x1000)) angles.y = 0.0f;
            if (!(flags & 0x2000)) angles.z = 0.0f;
            m.m_rot.EXMaths::quat_xyzw::SetRotation(angles, order);
        }
    }
    if (flags & 0x4000) {
        const EXMotionData *pStrip = (const EXMotionData *)m_XtraData.GetXtra(1);
        if (pStrip) StripMotionData(*pStrip, m.m_pos, m.m_rot, flags, 0);
    }
}

// The frame form: the decoded root goes to the root bone slot, then the axes selected by flags are
// removed from it (the opposite sense to DecompressMotion_StripMotion) and the strip motion put back.
void EXGeoBaseAnim::DecompressMotion_StripMotion2(EXGeoSkinAnimFrame *pFrame, unsigned int flags) const
{
    EXVector &pos = *(EXVector *)((u8 *)pFrame + (m_14 + 4) * 16);
    pos.z = pFrame->m_Pos.z;
    pos.x = pFrame->m_Pos.x;
    pos.y = pFrame->m_Pos.y;
    pos.w = pFrame->m_Pos.w;
    EXQuaternion &rot = *(EXQuaternion *)&pFrame->m_Root;
    rot.x = pFrame->m_Rot.x;
    rot.z = pFrame->m_Rot.z;
    rot.y = pFrame->m_Rot.y;
    rot.w = pFrame->m_Rot.w;
    if (flags & 0x700) {
        if ((flags & 0x700) == 0x700) {
            pos.x = 0.0f;
            pos.y = 0.0f;
            pos.z = 0.0f;
            pos.w = 1.0f;
        } else {
            if (flags & 0x100) pos.x = 0.0f;
            if (flags & 0x200) pos.y = 0.0f;
            if (flags & 0x400) pos.z = 0.0f;
        }
    }
    if (flags & 0x3800) {
        if ((flags & 0x3800) == 0x3800) {
            rot.z = 0.0f;
            rot.y = 0.0f;
            rot.x = 0.0f;
            rot.w = 1.0f;
        } else {
            unsigned char order = s_StripRotOrder[(flags >> 11) & 7];
            EXVector angles;
            rot.GetRotation(angles, order);
            if (flags & 0x800) angles.x = 0.0f;
            if (flags & 0x1000) angles.y = 0.0f;
            if (flags & 0x2000) angles.z = 0.0f;
            rot.EXMaths::quat_xyzw::SetRotation(angles, order);
        }
    }
    if (flags & 0x4000) {
        const EXMotionData *pStrip = (const EXMotionData *)m_XtraData.GetXtra(1);
        if (pStrip) StripMotionData(*pStrip, pos, rot, flags, 1);
    }
}

// Decode (or reuse) the frame's sequence memory entry for the bones in mask (bit 0, the root, is
// always included), plus its morph weights, and apply the strip flags if they changed.
EXGeoSkinAnimFrame *EXGeoBaseAnim::DecodeFrame(int frame, unsigned int strip, ex::EXBit128 bones) const
{
    EXGeoSkinAnimFrame *pFrame = AllocSeqMemory(m_pGeoFile ? m_pGeoFile->GeoHeader() : 0, frame);
    if (!pFrame) return 0;
    bones.m_bits[0] |= 1;
    if (pFrame->m_Decoded) {
        bones.m_bits[0] ^= bones.m_bits[0] & pFrame->m_Bones.m_bits[0];
        bones.m_bits[1] ^= bones.m_bits[1] & pFrame->m_Bones.m_bits[1];
        bones.m_bits[3] ^= bones.m_bits[3] & pFrame->m_Bones.m_bits[3];
        bones.m_bits[2] ^= bones.m_bits[2] & pFrame->m_Bones.m_bits[2];
        int any = bones.IsAnySet();
        bones |= pFrame->m_Bones;
        if (!any) goto strip;
    }
    pFrame->m_Decoded = 1;
    DecompressFrame(frame, pFrame, bones);
    if (m_NumMorphs && !pFrame->m_MorphDecoded) {
        DecompressMorph(frame, (float *)((u8 *)pFrame + (m_15 + 5 + m_14) * 16));
        pFrame->m_MorphDecoded = 1;
    }
strip:
    if ((unsigned short)pFrame->m_Frame != strip) {
        DecompressMotion_StripMotion2(pFrame, strip);
        pFrame->m_Frame = (short)strip;
    }
    return pFrame;
}

// Root motion of one frame: the first frame of an anim with a start pose (extra data 0) uses it,
// otherwise the decoded frame from sequence memory or a fresh root-only decode.
void EXGeoBaseAnim::DecompressMotion(int frame, EXMotionData &m, unsigned char c, unsigned int flags) const
{
    if (frame == 0 && m_XtraData.HasXtraData(0)) {
        m.m_pos.x = m_BasePos[0];
        m.m_pos.y = m_BasePos[1];
        m.m_pos.z = m_BasePos[2];
        m.m_pos.w = 0.0f;
        const float *pStart = (const float *)m_XtraData.GetXtra(0);
        m.m_rot.x = pStart[0];
        m.m_rot.y = pStart[1];
        m.m_rot.z = pStart[2];
        m.m_rot.w = pStart[3];
    } else if (!m_14) {
        m.m_pos.x = m_BasePos[0];
        m.m_pos.w = 0.0f;
        m.m_pos.y = m_BasePos[1];
        m.m_pos.z = m_BasePos[2];
        m.m_rot.z = 0.0f;
        m.m_rot.y = 0.0f;
        m.m_rot.x = 0.0f;
        m.m_rot.w = 1.0f;
    } else {
        EXGeoSkinAnimFrame *pFrame = ValidSeqMemoryPtr(m_pGeoFile ? m_pGeoFile->GeoHeader() : 0, frame);
        if (!pFrame || !pFrame->m_Decoded) {
            EXAnimStreamInfo info;
            ex::EXBit128 bones;
            ex::EXBit128 root;
            info.m_pSections = m_XtraData.GetXtra(5);
            info.m_NumFrames0 = m_NumFrames;
            root.m_bits[0] |= 1;   // retail sets bit 0 of an uninitialised mask
            bones.SetAll();
            info.m_Frame0 = frame;
            info.m_Frame1 = frame;
            info.m_StartFrame = 0;
            info.m_SeekFrame = frame;
            info.m_NumFrames = info.m_NumFrames0;
            DecompressData(MotionCompressedData(), 1, frame, bones, root, &m.m_pos, &m.m_rot, info, 0);
            m.m_pos.y = m_BasePos[1] + m.m_pos.y;
            m.m_pos.x = m.m_pos.x + m_BasePos[0];
            m.m_pos.z = m.m_pos.z + m_BasePos[2];
        } else {
            m.m_pos.x = pFrame->m_Pos.x;
            m.m_pos.y = pFrame->m_Pos.y;
            m.m_pos.z = pFrame->m_Pos.z;
            m.m_pos.w = pFrame->m_Pos.w;
            m.m_rot.x = pFrame->m_Rot.x;
            m.m_rot.y = pFrame->m_Rot.y;
            m.m_rot.z = pFrame->m_Rot.z;
            m.m_rot.w = pFrame->m_Rot.w;
        }
    }
    DecompressMotion_StripMotion(m, c, flags);
}

// Root motion between two frames: lerp the positions and slerp the rotations of the two decoded
// frames (decoding the second root if needed), or decode the root straight at the fractional frame.
void EXGeoBaseAnim::DecompressMotionInterp(float frame, EXMotionData &m, unsigned char c, unsigned int flags) const
{
    int f0 = (int)frame;
    float t = frame - (float)f0;
    if (t == 0.0f) {
        DecompressMotion(f0, m, c, flags);
        return;
    }
    EXAnimStreamInfo info;
    ex::EXBit128 bones;
    ex::EXBit128 root;
    info.m_pSections = m_XtraData.GetXtra(5);
    int f1 = f0 + 1;
    const unsigned char *pData = MotionCompressedData();
    info.m_NumFrames0 = m_NumFrames;
    info.m_StartFrame = 0;
    const EXGeoHeader *pHdr = m_pGeoFile ? m_pGeoFile->GeoHeader() : 0;
    root.m_bits[0] |= 1;   // retail sets bit 0 of an uninitialised mask
    bones.SetAll();
    info.m_Frame0 = f0;
    info.m_Frame1 = f1;
    info.m_NumFrames = info.m_NumFrames0;
    info.m_SeekFrame = f0;
    EXGeoSkinAnimFrame *pFrame0 = ValidSeqMemoryPtr(pHdr, f0);
    EXGeoSkinAnimFrame *pFrame1;
    if (!pFrame0 || !pFrame0->m_Decoded || !(pFrame1 = AllocSeqMemory(pHdr, f1))) {
        DecompressData(pData, 1, frame, bones, root, &m.m_pos, &m.m_rot, info, 0);
        m.m_pos.y = m_BasePos[1] + m.m_pos.y;
        m.m_pos.x = m.m_pos.x + m_BasePos[0];
        m.m_pos.z = m.m_pos.z + m_BasePos[2];
    } else {
        if (!(pFrame1->m_Bones.m_bits[0] & 1)) {
            pFrame1->m_Bones.Set(0);
            DecompressData(pData, 1, f1, bones, root, &pFrame1->m_Pos, &pFrame1->m_Rot, info, 0);
            pFrame1->m_Pos.x = pFrame1->m_Pos.x + m_BasePos[0];
            pFrame1->m_Pos.y = m_BasePos[1] + pFrame1->m_Pos.y;
            pFrame1->m_Pos.z = m_BasePos[2] + pFrame1->m_Pos.z;
        }
        m.m_pos.x = (pFrame1->m_Pos.x - pFrame0->m_Pos.x) * t + pFrame0->m_Pos.x;
        m.m_pos.y = (pFrame1->m_Pos.y - pFrame0->m_Pos.y) * t + pFrame0->m_Pos.y;
        m.m_pos.z = (pFrame1->m_Pos.z - pFrame0->m_Pos.z) * t + pFrame0->m_Pos.z;
        m.m_pos.w = (pFrame1->m_Pos.w - pFrame0->m_Pos.w) * t + pFrame0->m_Pos.w;
        EXMaths::quat_xyzw q = ((const EXMaths::quat_xyzw &)pFrame0->m_Rot).slerp((const EXMaths::quat_xyzw &)pFrame1->m_Rot, t);
        m.m_rot.x = q.x;
        m.m_rot.y = q.y;
        m.m_rot.z = q.z;
        m.m_rot.w = q.w;
    }
    DecompressMotion_StripMotion(m, c, flags);
}

// One datum (attachment point) of the anim at a frame; returns 0 (identity) if the anim has no such
// datum. Flags 0x3f00 make it relative to the root motion, 0x4000 put the strip motion back on.
signed char EXGeoBaseAnim::DecompressDatum(int frame, int datum, EXMotionData &m, unsigned int flags) const
{
    signed char ret;
    EXMaths::vec_xyzw pos[32];
    EXMaths::vec_xyzw rot[32];
    unsigned int count = m_DatumCount < 32 ? m_DatumCount : 32;
    if (!count || datum >= (int)count) {
        m.m_rot.z = 0.0f;
        m.m_rot.y = 0.0f;
        m.m_rot.x = 0.0f;
        m.m_rot.w = 1.0f;
        ret = 0;
        m.m_pos.w = 0.0f;
        m.m_pos.z = 0.0f;
        m.m_pos.y = 0.0f;
        m.m_pos.x = 0.0f;
    } else {
        ret = 1;
        const unsigned char *pData = *(const ex::EXRelPtr<unsigned char> *)AnimDatumInfo();
        ex::EXBit128 want;
        ex::EXBit128 all;
        EXAnimStreamInfo info;
        all.SetAll();
        info.m_NumFrames0 = m_NumFrames;
        want.ClrAll();
        info.m_StartFrame = 0;
        info.m_pSections = 0;
        info.m_Frame0 = frame;
        want.Set(datum);
        info.m_Frame1 = frame;
        info.m_SeekFrame = frame;
        info.m_NumFrames = info.m_NumFrames0;
        DecompressData(pData, count, frame, want, all, pos, rot, info, 1);
        m.m_pos.x = pos[datum].x;
        m.m_pos.y = pos[datum].y;
        m.m_pos.z = pos[datum].z;
        m.m_pos.w = pos[datum].w;
        m.m_rot.x = rot[datum].x;
        m.m_rot.y = rot[datum].y;
        m.m_rot.z = rot[datum].z;
        m.m_rot.w = rot[datum].w;
    }
    if (flags & 0x3f00) {
        EXMotionData root;
        DecompressMotion(frame, root, 8, flags & 0x3f00);
        m.m_pos.x = m.m_pos.x - root.m_pos.x;
        m.m_pos.y = m.m_pos.y - root.m_pos.y;
        m.m_pos.z = m.m_pos.z - root.m_pos.z;
        m.m_pos.w = m.m_pos.w - root.m_pos.w;
    }
    if (flags & 0x4000) {
        const EXMotionData *pStrip = (const EXMotionData *)m_XtraData.GetXtra(1);
        if (pStrip) StripMotionData(*pStrip, m.m_pos, m.m_rot, flags, 1);
    }
    return ret;
}

signed char EXGeoBaseAnim::DecompressDatumInterp(float frame, int datum, EXMotionData &m, unsigned int flags) const
{
    signed char ret;
    EXMaths::vec_xyzw pos[32];
    EXMaths::vec_xyzw rot[32];
    unsigned int count = m_DatumCount < 32 ? m_DatumCount : 32;
    ret = count && datum < (int)count;
    if (!count || datum >= (int)count) {
        m.m_rot.z = 0.0f;
        m.m_rot.y = 0.0f;
        m.m_rot.x = 0.0f;
        m.m_rot.w = 1.0f;
        m.m_pos.w = 0.0f;
        m.m_pos.z = 0.0f;
        m.m_pos.y = 0.0f;
        m.m_pos.x = 0.0f;
    } else {
        const unsigned char *pData = *(const ex::EXRelPtr<unsigned char> *)AnimDatumInfo();
        ex::EXBit128 want;
        ex::EXBit128 all;
        EXAnimStreamInfo info;
        all.SetAll();
        want.ClrAll();
        want.Set(datum);
        info.m_Frame0 = (int)frame;
        info.m_Frame1 = info.m_Frame0 + 1;
        info.m_NumFrames0 = m_NumFrames;
        info.m_pSections = 0;
        info.m_StartFrame = 0;
        info.m_NumFrames = info.m_NumFrames0;
        info.m_SeekFrame = info.m_Frame0;
        DecompressData(pData, count, frame, want, all, pos, rot, info, 1);
        m.m_pos.x = pos[datum].x;
        m.m_pos.y = pos[datum].y;
        m.m_pos.z = pos[datum].z;
        m.m_pos.w = pos[datum].w;
        m.m_rot.x = rot[datum].x;
        m.m_rot.y = rot[datum].y;
        m.m_rot.z = rot[datum].z;
        m.m_rot.w = rot[datum].w;
    }
    if (flags & 0x3f00) {
        EXMotionData root;
        DecompressMotionInterp(frame, root, 8, flags & 0x3f00);
        m.m_pos.x = m.m_pos.x - root.m_pos.x;
        m.m_pos.y = m.m_pos.y - root.m_pos.y;
        m.m_pos.z = m.m_pos.z - root.m_pos.z;
        m.m_pos.w = m.m_pos.w - root.m_pos.w;
    }
    if (flags & 0x4000) {
        const EXMotionData *pStrip = (const EXMotionData *)m_XtraData.GetXtra(1);
        if (pStrip) StripMotionData(*pStrip, m.m_pos, m.m_rot, flags, 1);
    }
    return ret;
}
