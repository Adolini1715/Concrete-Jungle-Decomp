#include "EXItemAnimator_Sound.h"
#include "EXBaseRenderEnv.h"
#include "EXMemory.h"
#include "EXBaseSoundManager.h"

// TODO stand-in: EXSoundHandle (audio area), only the fields this unit touches.
class EXSoundHandle {
public:
    u8 pad[0x18];
    u32 m_user;         // +0x18
    u8 pad1c[0xc];
    u16 m_timeout;      // +0x28
    u16 &SoundTimeout() { return m_timeout; }
};
EX_IMPLEMENT_DYNCREATE(EXItemAnimator_Sound, EXItemAnimator)

EXItemAnimator_Sound::EXItemAnimator_Sound()
{
    m_soundFlags = 0;
    m_bStarted = 0;
    m_user = 0;
    m_volume = 0.0f;
    m_range = 0.0f;
    m_reverb = -1;
}

s8 EXItemAnimator_Sound::Initialise(EXGeoFile *pFile, u32 hash)
{
    m_geoRef.Set(pFile, hash);
    m_soundFlags = 0;
    m_bStarted = 0;
    m_volume = 0.0f;
    m_range = 0.0f;
    m_reverb = -1;
    return 1;
}

void EXItemAnimator_Sound::SetSoundDetails(float vol, float range, int reverb, u32 flags, u32 user)
{
    m_soundFlags = flags;
    m_volume = vol;
    m_range = range;
    m_reverb = (s8)reverb;
    m_user = user;
}

void EXItemAnimator_Sound::SetSoundTimeout(u16 t) const
{
    if (m_tag.m_pHead) m_tag.m_pHead->SoundTimeout() = t;
}

void EXItemAnimator_Sound::UpdateSound(s8 bRender)
{
    EXBaseSoundManager *pMgr = EXBaseSoundManager::GetSoundManager();
    if (!pMgr) return;
    u32 hash = RefObject();
    EXVector pos(m_mtx.r[3]);
    switch ((hash >> 24) & 0x7f) {
    case 0x1a:
        if (m_soundFlags & 0x20) {
            if (!m_bStarted) {
                EXSoundHandle *h = m_tag.AddTrackingSound(hash, pos, m_volume, m_range);
                if (h) {
                    h->m_user = m_user;
                    m_bStarted = 1;
                    return;
                }
            }
        } else if (!m_bStarted) {
            const EXSoundDetails &det = pMgr->SoundDetails(hash);
            // EXSoundDetails: s16 range at +2, u8 check-range flag at +0xb.
            float range = m_range != 0.0f ? m_range : (float)((const s16 *)&det)[1];
            if (!((const u8 *)&det)[0xb] || pMgr->SfxIsValidRange(pos, range)) {
                EXSoundHandle *h = m_tag.StartSound(RefObject(), pos, m_volume, m_range, m_reverb, 0);
                if (h) {
                    h->m_user = m_user;
                    m_bStarted = 1;
                    return;
                }
            }
            m_itemFlags.m_flags |= 0x10;
            return;
        } else if (!m_tag.m_pHead) {
            m_itemFlags.m_flags |= 0x10;
            return;
        }
        if (m_bStarted && bRender) m_tag.UpdatePosition(pos);
        break;
    case 0x1c:
        if (bRender && pMgr->SfxIsValidRange(pos, m_range)) pMgr->SfxSetReverbDepth(m_reverb);
        break;
    }
}

void EXItemAnimator_Sound::DoUpdate()
{
    CalcMatrix_InPlaceParent();
    UpdateSound(0);
    UpdateNodes();
}

void EXItemAnimator_Sound::SetObjectTime(float)
{
    CalcMatrix_InPlaceParent();
    UpdateSound(0);
}

u32 EXItemAnimator_Sound::DoRender(const EXMatrix &parent, const EXRenderInfo &info, u32 flags)
{
    u32 result = 0;
    if (EXBaseRenderEnv::RenderState() & 1) CalcMatrix_InPlace(parent, flags);
    if (!EXBaseRenderEnv::ShadowRender()) {
        UpdateSound(1);
        if (EXBaseRenderEnv::RenderState() & 2) result = 1;
    }
    const EXMatrix &m = mtx();
    return RenderNodes(m, info, flags) | result;
}
