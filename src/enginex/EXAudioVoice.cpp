#include "Voice.h"
#include "EXAudio.h"
#include "EXAudioVoiceManager.h"
#include "EXAudioCodec.h"
#include "XBMixer.h"
#include "SFXPoint3.h"
#include "EXMemory.h"


extern "C" void *__cdecl memset(void *, int, unsigned int);
extern "C" void *__cdecl memcpy(void *, const void *, unsigned int);
extern "C" double __cdecl sqrt(double);
#pragma intrinsic(sqrt)
#include <math.h>

static SFXPoint3 s_MicOrigin;

void EXAudioVoiceManager::MicSetPos(SFXPoint3 *pPos, SFXPoint3 *pDir, SFXPoint3 *pUp, SFXPoint3 *)
{
    m_MicPos.x = pPos->x;
    m_MicPos.y = pPos->y;
    m_MicPos.z = pPos->z;
    m_MicDir.x = pDir->x;
    m_MicDir.y = pDir->y;
    m_MicDir.z = pDir->z;
    m_MicUp.x = pUp->x;
    m_MicUp.y = pUp->y;
    m_MicUp.z = pUp->z;
}

Voice::~Voice()
{
    gpEXAudio->psiAsyncDeleteBuffer((char *)m_pMixBuffer);
}

// NEAR: 8 bytes differ, x87 stack slot used for the sqrt result (retail: fstp st(3)).
void Voice::NormalisePoint3(SFXPoint3 *p)
{
    float z = p->z;
    float y = p->y;
    float x = p->x;
    float len = (float)sqrt(x * x + y * y + z * z);
    if (len > 0.0f) {
        float inv = 1.0f / len;
        p->x *= inv;
        p->y *= inv;
        p->z *= inv;
    } else {
        p->x = 0.0f;
        p->y = 0.0f;
        p->z = 0.0f;
    }
}

float Voice::DotP_Point3(SFXPoint3 *a, SFXPoint3 *b)
{
    return a->z * b->z + a->y * b->y + a->x * b->x;
}

int Voice::VoiceGetSound()
{
    return -1;
}

void Voice::VoiceLock()
{
    if (this) m_Locked = true;
}

void Voice::VoiceUnlock()
{
    if (this) m_Locked = false;
}

bool Voice::IsVoiceLocked()
{
    if (!this) return false;
    return m_Locked;
}

void Voice::SetFlags(int flags)
{
    gpMixer->m_Lock.Enter();
    m_Flags = flags;
    gpMixer->m_Lock.Leave();
}

void Voice::AndFlags(int flags)
{
    gpMixer->m_Lock.Enter();
    m_Flags = m_Flags & ~flags;
    gpMixer->m_Lock.Leave();
}

void Voice::OrFlags(int flags)
{
    gpMixer->m_Lock.Enter();
    m_Flags = m_Flags | flags;
    gpMixer->m_Lock.Leave();
}

int Voice::GetFlags()
{
    gpMixer->m_Lock.Enter();
    int flags = m_Flags;
    gpMixer->m_Lock.Leave();
    return flags;
}

void Voice::SetVolume(int v)
{
    if (v < 0) v = 0;
    else if (v > 100) v = 100;
    m_Gain = (float)v * 0.01f;
}

void Voice::SetFrequency_()
{
    u32 a = (m_SampleRate << 10) / gpMixer->GetPrimaryFreq();
    u32 b = (m_BaseRate << 10) / m_SampleRate;
    m_Pitch = (u32)((__int64)((unsigned __int64)a * b) >> 10);
}

void Voice::SetFrequency(int freq)
{
    m_BaseRate = freq;
}

void Voice::SetLowPassFilter(float v)
{
    m_VolumeA = v;
}

void Voice::SetLowPassFilterHRTF(float v)
{
    m_VolumeB = v;
}

// Speaker gain along an arc of the pan circle: 1 at angle a falling to 0 at angle b (a <= b), or 0 at b
// rising to 1 at a (a > b). out is left alone outside the arc.
void VolArc(float angle, float a, float b, float &out)
{
    if (a <= b) {
        angle -= a;
        if (angle >= 0.0f && angle <= b - a) out = 1.0f - angle / (b - a);
    } else {
        angle -= b;
        if (angle >= 0.0f && angle <= a - b) out = angle / (a - b);
    }
}

// 5.1 speaker gains: [0] front left, [1] front right, [2] rear left, [3] rear right, [4] centre.
// Positioned voices pan round the circle (0 degrees ahead, clockwise): centre 0, right 60,
// rear right 135, rear left 225, left 300.
void Voice::UpdateVolumes_51_()
{
    switch (m_Mode) {
    case 0: {
        float l = 0.0f - m_PanLR;
        if (l < 0.0f) l = 0.0f;
        float r = m_PanLR;
        if (r < 0.0f) r = 0.0f;
        m_TargetVol[0] = l;
        m_TargetVol[1] = r;
        m_TargetVol[2] = 0.0f;
        m_TargetVol[3] = 0.0f;
        m_TargetVol[4] = 1.0f - (float)fabs(m_PanLR);
        return;
    }
    case 1:
    case 5:
    case 7: {
        float angle = 0.0f;
        if (m_PanLR != 0.0f || m_PanFR != 0.0f) {
            angle = (float)(atan2(m_PanLR, m_PanFR) * 57.3f);
            if (angle < 0.0f) angle += 360.0f;
        }
        float fl = 0.0f, fr = 0.0f, c = 0.0f, rl = 0.0f, rr = 0.0f;
        VolArc(angle, 300.0f, 360.0f, fl);
        VolArc(angle, 300.0f, 225.0f, fl);
        VolArc(angle, 60.0f, 0.0f, fr);
        VolArc(angle, 60.0f, 135.0f, fr);
        VolArc(angle, 360.0f, 300.0f, c);
        VolArc(angle, 0.0f, 60.0f, c);
        VolArc(angle, 225.0f, 300.0f, rl);
        VolArc(angle, 225.0f, 135.0f, rl);
        VolArc(angle, 135.0f, 225.0f, rr);
        VolArc(angle, 135.0f, 60.0f, rr);
        m_TargetVol[0] = fl;
        m_TargetVol[2] = rl;
        m_TargetVol[3] = rr;
        m_TargetVol[1] = fr;
        m_TargetVol[4] = c;
        return;
    }
    case 2: {
        float l = 0.0f - m_PanLR;
        if (l < 0.0f) l = 0.0f;
        float r = m_PanLR;
        if (r < 0.0f) r = 0.0f;
        float rearR = (m_PanLR + 1.0f) * 0.5f;
        float rearL = 1.0f - rearR;
        m_TargetVol[4] = (1.0f - (float)fabs(m_PanLR)) * 0.6f;
        m_TargetVol[0] = l * 0.6f;
        m_TargetVol[1] = r * 0.6f;
        m_TargetVol[2] = rearL * 0.6f;
        m_TargetVol[3] = rearR * 0.6f;
        return;
    }
    case 0x80:
        m_TargetVol[0] = 0.8f;
        m_TargetVol[1] = 0.8f;
        m_TargetVol[2] = 0.2f;
        m_TargetVol[3] = 0.2f;
        m_TargetVol[4] = 0.05f;
        return;
    default:
        m_TargetVol[0] = 0.22f;
        m_TargetVol[1] = 0.22f;
        m_TargetVol[4] = 0.22f;
        m_TargetVol[2] = 0.22f;
        m_TargetVol[3] = 0.22f;
        return;
    }
}

void Voice::UpdateVolumes_71_() {}

void Voice::UpdateVolumes_Mono_()
{
    switch (m_Mode) {
    case 1:
    case 5:
    case 7:
    {
        float g;
        if (0.0f > m_PanFR) g = 1.0f - m_PanFR * -0.5f;
        else g = 1.0f;
        m_TargetVol[4] = g;
        return;
    }
    case 0:
    case 2:
    case 0x80:
        m_TargetVol[4] = 1.0f;
        return;
    default:
        m_TargetVol[4] = 1.0f;
        return;
    }
}

// NEAR: 2 bytes differ, order of the two pan-gain multiplies.
void Voice::UpdateVolumes_Stereo_()
{
    switch (m_Mode) {
    case 0:
    case 2: {
        float r = (m_PanLR + 1.0f) * 0.5f;
        m_TargetVol[0] = 1.0f - r;
        m_TargetVol[1] = r;
        return;
    }
    case 1:
    case 5:
    case 7: {
        float g = 1.0f;
        float r = (m_PanLR + g) * 0.5f;
        float l = g - r;
        m_TargetVol[1] = r;
        m_TargetVol[0] = l;
        if (0.0f > m_PanFR) {
            g -= m_PanFR * -0.5f;
            l *= g;
            r *= g;
            m_TargetVol[1] = r;
m_TargetVol[0] = l;
        }
        return;
    }
    case 0x80:
        m_TargetVol[0] = 1.0f;
        m_TargetVol[1] = 1.0f;
        return;
    default:
        m_TargetVol[0] = 0.5f;
        m_TargetVol[1] = 0.5f;
        return;
    }
}

void Voice::SetPanLR(float v)
{
    m_PanLR = v;
}

void Voice::SetPanFR(float v)
{
    m_PanFR = v;
}

void Voice::SetPan(float lr, float fr)
{
    m_PanLR = lr;
    m_PanFR = fr;
}

void Voice::GetWrite(unsigned int pos, unsigned int size, void **pp1, unsigned int *pn1, void **pp2,
                     unsigned int *pn2)
{
    if (pp1) *pp1 = m_pData + pos;
    if (pp2) *pp2 = 0;
    if (pos + size > m_Length) {
        if (pn1) *pn1 = m_Length - pos;
        if (pp2) *pp2 = m_pData;
        if (pn2) *pn2 = pos - m_Length + size;
    } else {
        if (pn1) *pn1 = size;
    }
}

void EXAudio::psiUnLockVoiceHandle(int h)
{
    if (h) ((Voice *)h)->m_Locked = false;
}

void EXAudio::psiLockVoiceHandle(int h)
{
    if (h) ((Voice *)h)->m_Locked = true;
}

void EXAudio::psiInitialiseSound(psiSFX *pSfx)
{
    psiUpdateSound(pSfx);
}

void EXAudio::psiSFXSetupListener(SFXPoint3 *pPos, SFXPoint3 *pVel, SFXPoint3 *pFront, SFXPoint3 *pTop,
                                  SFXPoint3 *pRight)
{
    gpAudioVoice->MicSetPos(pPos, pFront, pRight, pTop);
}

void psiStreamDeletePlaybackBuffer(int) {}

void psiStreamSetToNull(int size, char *p)
{
    memset(p, 0, size);
}

unsigned int EXAudio::psiSFXGetTimer()
{
    return gpAudioVoice->GetTime();
}

EXAudioVoiceManager::~EXAudioVoiceManager() {}

Voice *EXAudioVoiceManager::FindFreeVoice()
{
    int i = m_NextVoice;
    do {
        if (!m_Voices[i].GetFlags() && !m_Voices[i].m_Locked) {
            m_NextVoice = (i + 1) % 32;
            return &m_Voices[i];
        }
        i = (i + 1) % 32;
    } while (i != m_NextVoice);
    return 0;
}

// NEAR: ~28 bytes differ, scheduling of the slot field stores.
int EXAudioVoiceManager::FxCreate(void *pData, int length, int rate, int channels, bool loop, int loopStart,
                                  bool alloc)
{
    if (!m_Active) return -1;
    SOUNDLIST_tag *pSound = &m_Sounds[1];
    int i;
    for (i = 1; i < 0x800; i++, pSound++) {
        if (!pSound->m_Used) break;
    }
    if (i >= 0x800) return -1;
    memset(pSound, 0, sizeof(SOUNDLIST_tag));
    pSound->m_SampleRate = (u16)rate;
    pSound->m_Loop = loop;
    pSound->m_LoopStart = loopStart;
    pSound->m_Length = length;
    pSound->m_Channels = channels;
    pSound->m_Owned = alloc;
    pSound->m_pData = (char *)pData;
    pSound->m_Used = 1;
    if (alloc) {
        char *p = gpEXAudio->psiAsyncCreateBuffer(length * channels, 0);
        pSound->m_pData = p;
        if (p) memset(p, 0, pSound->m_Length * channels);
    }
    return i;
}

void EXAudioVoiceManager::FxDelete(int i)
{
    if ((unsigned int)i < 0x800) {
        SOUNDLIST_tag *pSound = &m_Sounds[i];
        if (pSound->m_Owned) gpEXAudio->psiAsyncDeleteBuffer(pSound->m_pData);
        memset(pSound, 0, sizeof(SOUNDLIST_tag));
    }
}

void EXAudioVoiceManager::CheckForStoppedVoices()
{
    if (!m_Active) return;
    m_NumPlaying = 0;
    Voice *pVoice = m_Voices;
    for (int i = 32; i; i--, pVoice++) {
        if (pVoice->GetFlags()) {
            if (pVoice->GetFlags() & 0x40) {
                gpMixer->m_Lock.Enter();
                pVoice->m_Flags = 0;
                gpMixer->m_Lock.Leave();
            } else if (!(pVoice->GetFlags() & 1)) {
                if (!pVoice->m_pSound->m_Owned && !pVoice->m_Locked) pVoice->SetFlags(0);
            } else {
                m_NumPlaying++;
            }
        }
    }
}

void EXAudioVoiceManager::DEBUG_GetVoiceFlags(int *pFlags)
{
    for (int i = 0; i < 32; i++) {
        int flags = m_Voices[i].GetFlags();
        if (m_Voices[i].m_Locked) flags |= 0x80000000;
        *pFlags++ = flags;
    }
}

void Voice::VoiceMemCpy(int pos, void *pSrc, int size)
{
    if (this && GetFlags() && pSrc) {
        unsigned int n1 = 0;
        void *p1 = 0;
        unsigned int n2 = 0;
        void *p2 = 0;
        GetWrite(pos, size, &p1, &n1, &p2, &n2);
        if (p1) memcpy(p1, pSrc, n1);
        if (p2) memcpy(p2, (char *)pSrc + n1, n2);
    }
}

int Voice::VoiceGetBytePos()
{
    if (!this) return 0;
    if (!GetFlags()) return 0;
    return m_Pos;
}

void Voice::StreamPause()
{
    if (this && GetFlags()) AndFlags(4);
}

void Voice::StreamUnPause()
{
    if (this && GetFlags()) OrFlags(4);
}

void Voice::StreamStop()
{
    if (this && GetFlags()) SetFlags(0x19);
}

void Voice::VoiceSetVolume(float v)
{
    if (this && GetFlags()) {
        if (v < 0.0f) v = 0.0f;
        else if (v > 100.0f) v = 100.0f;
        m_Volume = v;
    }
}

void Voice::VoiceSet2dPan(float pan)
{
    if (this && GetFlags()) m_PanLR = pan;
}

void Voice::VoiceSetFreq(int freq)
{
    if (this && GetFlags()) {
        SetFrequency(freq);
        SetFrequency_();
    }
}

void Voice::Reset_(s8 full)
{
    if (full) {
        m_Flags = 9;
    } else {
        m_Flags = 0;
        if (m_pCodec) delete m_pCodec;
        m_Volume = 0.0f;
        m_MinRadius = 0.0f;
        m_MaxRadius = 0.0f;
        m_Gain = 0.0f;
        m_PanLR = 0.0f;
        m_PanFR = 0.0f;
        m_Reverb = 0.0f;
        m_pCodec = 0;
        m_SampleRate = 0;
        m_Length = 0;
        m_pSound = 0;
        m_Is3D = 0;
        m_Unk24 = 0;
        m_Locked = 0;
        m_Channels = 0;
        m_pMixRead = 0;
        m_LoopStart = 0;
        m_pData = 0;
        m_Pitch = 0;
        m_Frac = 0;
        m_BaseRate = 0;
        m_Mode = 0;
        m_VolumeA = 1024.0f;
        m_VolumeB = 1024.0f;
    }
    m_Frac = 0;
    m_Pos = 0;
    if (m_pCodec) m_pCodec->Reset_();
}

bool Voice::VoiceIsPlaying()
{
    if (!this) return false;
    if (!GetFlags()) return false;
    return (GetFlags() & 1) != 0;
}

void Voice::VoiceSetMode(int mode)
{
    if (this && GetFlags()) {
        m_Is3D = mode & 1;
        m_Mode = mode;
    }
}

void Voice::VoiceSetPos(SFXPoint3 *p)
{
    if (this && GetFlags()) {
        m_Pos3D[0] = p->x;
        m_Pos3D[1] = p->y;
        m_Pos3D[2] = p->z;
    }
}

void Voice::VoiceSetRadius(float minR, float maxR)
{
    if (this && GetFlags()) {
        m_MinRadius = minR;
        m_MaxRadius = maxR;
    }
}

void Voice::Play(int flags)
{
    DEBUGTRACE("Voice::Play\n");
    gpMixer->m_Lock.Enter();
    m_Flags = m_Flags | flags | 6;
    gpMixer->m_Lock.Leave();
}

void Voice::Pause()
{
    gpMixer->m_Lock.Enter();
    m_Flags = m_Flags & ~4;
    gpMixer->m_Lock.Leave();
}

void Voice::Stop()
{
    DEBUGTRACE("Voice::Stop\n");
    gpMixer->m_Lock.Enter();
    m_Flags = m_Flags | 0x80;
    gpMixer->m_Lock.Leave();
}

void EXAudio::psiLoadSampleInitialise(SampleHeaderData *p)
{
    p->m_Handle = gpAudioVoice->FxCreate((void *)p->m_Handle, p->m_Length, p->m_SampleRate, 1, p->m_Flags & 1,
                                         p->m_LoopStart, false);
}

void EXAudio::psiUnLoadSampleInitialise(SampleHeaderData *p)
{
    gpAudioVoice->FxDelete(p->m_Handle);
}

int EXAudio::psiIsSamplePlaying(int h)
{
    return ((Voice *)h)->VoiceIsPlaying() ? 1 : 0;
}

void EXAudio::psiSetSampleFrequency(int h, int freq)
{
    ((Voice *)h)->VoiceSetFreq(freq);
}

void EXAudio::psiSetSampleVolume(int h, int vol)
{
    ((Voice *)h)->VoiceSetVolume((float)vol);
}

void psiStreamStop(int h)
{
    ((Voice *)h)->StreamStop();
}

void psiStreamPause(int h)
{
    ((Voice *)h)->StreamPause();
}

void psiStreamUnPause(int h)
{
    ((Voice *)h)->StreamUnPause();
}

void psiSetStreamFrequency(int h, int freq)
{
    ((Voice *)h)->VoiceSetFreq(freq);
}

void psiStreamSetVolume(int h, int vol)
{
    ((Voice *)h)->VoiceSetVolume((float)vol);
}

int psiStreamGetPlayPos(int h)
{
    return ((Voice *)h)->VoiceGetBytePos();
}

void psiStreamMemCpy(int h, int pos, char *pSrc, int size, int)
{
    ((Voice *)h)->VoiceMemCpy(pos, pSrc, size);
}

void EXAudioVoiceManager::ResetResources()
{
    if (m_Active) {
        MicSetPos(&s_MicOrigin, &s_MicOrigin, &s_MicOrigin, &s_MicOrigin);
        for (int i = 0; i < 0x800; i++) FxDelete(i);
        CheckForStoppedVoices();
    }
}

Voice::Voice()
{
    m_pCodec = 0;
    Reset_(0);
    m_pMixBuffer = (float *)gpEXAudio->psiAsyncCreateBuffer(0x8000, 0);
    for (int i = 0; i < 6; i++) m_TargetVol[i] = 0.0f;
    memset(m_pMixBuffer, 0, 0x8000);
}

void Voice::VoicePlay(float reverb)
{
    if (this && GetFlags()) {
        Play(0);
        if (reverb != -1.0f) SetReverb(reverb);
    }
}

void Voice::VoiceStop(bool pause)
{
    if (this && GetFlags()) {
        if (!pause) Stop();
        else Pause();
    }
}

void Voice::StreamPlay(float reverb)
{
    if (this && GetFlags()) {
        m_Gain = 0.0f;
        m_Frac = 0;
        m_Pos = 0;
        m_pCodec->Reset_();
        if (reverb != -1.0f) SetReverb(reverb);
        Play(8);
    }
}

static inline float ClampUnit(float v)
{
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

// Work out the voice gain (3D voices from the listener distance unless mode bit 8 holds it) and the
// per-speaker targets for the current output mode; [5] is the reverb send.
void Voice::UpdateVolumes()
{
    if (!m_Is3D) {
        int vol = (int)m_Volume;
        if (vol < 0) vol = 0;
        else if (vol > 100) vol = 100;
        m_Gain = vol * 0.01f;
    } else if (!(m_Mode & 8))
        Calculate3dVolume();
    m_TargetVol[0] = 0.0f;
    m_TargetVol[1] = 0.0f;
    m_TargetVol[2] = 0.0f;
    m_TargetVol[3] = 0.0f;
    m_TargetVol[4] = 0.0f;
    m_TargetVol[5] = 0.0f;
    if (m_Gain == 0.0f) return;
    switch (gpEXAudio->SFXGetMode()) {
    case 0:
        UpdateVolumes_Mono_();
        break;
    case 1:
        UpdateVolumes_Stereo_();
        break;
    case 2:
    case 3:
    case 4:
        UpdateVolumes_51_();
        break;
    }
    m_TargetVol[5] = m_Reverb;
    m_TargetVol[0] = ClampUnit(m_TargetVol[0] * m_Gain);
    m_TargetVol[1] = ClampUnit(m_Gain * m_TargetVol[1]);
    m_TargetVol[2] = ClampUnit(m_TargetVol[2] * m_Gain);
    m_TargetVol[3] = ClampUnit(m_Gain * m_TargetVol[3]);
    m_TargetVol[4] = ClampUnit(m_TargetVol[4] * m_Gain);
    m_TargetVol[5] = ClampUnit(m_TargetVol[5] * m_Gain);
}

void EXAudio::psiUpdateSound(psiSFX *pSfx)
{
    Voice *pVoice = (Voice *)pSfx->m_Voice;
    pVoice->VoiceSetMode(pSfx->m_Mode);
    pVoice->VoiceSetPos(pSfx->m_pPos);
    pVoice->VoiceSetRadius((float)pSfx->m_MinRadius, (float)pSfx->m_MaxRadius);
    pVoice->VoiceSetFreq(pSfx->m_Freq);
    pVoice->VoiceSetVolume((float)pSfx->m_Volume);
    pVoice->VoiceSet2dPan(pSfx->m_Pan * 0.01f);
}

void Voice::Update()
{
    if (!(GetFlags() & 0xb0)) {
        UpdateVolumes();
        SetFrequency_();
    }
}

int EXAudio::psiSampleKeyOn(int h, int reverb)
{
    ((Voice *)h)->VoicePlay((float)reverb);
    return 0;
}

int EXAudio::psiSampleKeyOff(int h)
{
    ((Voice *)h)->VoiceStop(false);
    return 0;
}

void EXAudio::psiSamplePause(int h)
{
    ((Voice *)h)->VoiceStop(true);
}

void EXAudio::psiSampleUnPause(int h)
{
    Voice *pVoice = (Voice *)h;
    if (pVoice && pVoice->GetFlags()) pVoice->Play(0);
}

void psiStreamPlay(int h, int reverb)
{
    ((Voice *)h)->StreamPlay((float)reverb);
}

EXAudioVoiceManager::EXAudioVoiceManager()
{
    m_Active = 0;
    m_NumPlaying = 0;
    m_NextVoice = 0;
    memset(m_Sounds, 0, sizeof(m_Sounds));
    for (int i = 0; i < 32; i++) m_Voices[i].m_Flags = 0;
}

// NEAR: ~400 bytes differ, same x87 sqrt slot issue as NormalisePoint3 (retail: fstp st(3)) and the
// SSE register allocation that follows it; logic complete.
void Voice::Calculate3dVolume()
{
    float minR = m_MinRadius;
    float maxR = m_MaxRadius;
    float x = gpAudioVoice->m_MicPos.x - m_Pos3D[0];
    float y = gpAudioVoice->m_MicPos.y - m_Pos3D[1];
    float z = gpAudioVoice->m_MicPos.z - m_Pos3D[2];
    x = (float)sqrt(x * x + y * y + z * z);
    float dist = x;
    float f;
    if (minR >= dist) {
        f = 1.0f;
    } else if (dist >= maxR) {
        f = 0.0f;
    } else {
        float t = maxR - minR;
        if (t != 0.0f) t = (maxR - dist) / t;
        f = t * t;
    }
    int vol = (int)(m_Volume * f);
    if (vol < 0) vol = 0;
    else if (vol > 100) vol = 100;
    m_Gain = vol * 0.01f;
    SFXPoint3 rel;
    rel.x = m_Pos3D[0] - gpAudioVoice->m_MicPos.x;
    rel.y = m_Pos3D[1] - gpAudioVoice->m_MicPos.y;
    rel.z = m_Pos3D[2] - gpAudioVoice->m_MicPos.z;
    NormalisePoint3(&rel);
    SFXPoint3 v = gpAudioVoice->m_MicUp;
    NormalisePoint3(&v);
    float lr = DotP_Point3(&v, &rel);
    if (lr < -1.0f) lr = -1.0f;
    else if (lr > 1.0f) lr = 1.0f;
    v = gpAudioVoice->m_MicDir;
    NormalisePoint3(&v);
    float fr = DotP_Point3(&v, &rel);
    if (fr < -1.0f) fr = -1.0f;
    else if (fr > 1.0f) fr = 1.0f;
    m_PanLR = lr;
    m_PanFR = fr;
}

// Needs Calculate3dVolume compiled earlier in this file (retail skips reloading ecx after it).
void Voice::InitVoice(SOUNDLIST_tag *pSound, int mode)
{
    Reset_(0);
    m_pSound = pSound;
    m_SampleRate = pSound->m_SampleRate;
    m_LoopStart = pSound->m_LoopStart;
    m_pData = pSound->m_pData;
    m_Length = pSound->m_Length;
    m_Channels = pSound->m_Channels;
    m_Mode = mode;
    m_Is3D = mode & 1;
    SetFrequency(pSound->m_SampleRate);
    SetFrequency_();
    Calculate3dVolume();
    Update();
    m_Snap = 1;
    if (pSound->m_Loop) OrFlags(8);
    m_pCodec = new EXAudioCodec_ADPCM;
    m_pCodec->Reset_();
    OrFlags(1);
}

Voice *EXAudioVoiceManager::VoiceAlloc(int sound, int mode)
{
    if (!m_Active) return 0;
    if ((unsigned int)sound >= 0x800) return 0;
    Voice *pVoice = FindFreeVoice();
    if (!pVoice) return 0;
    pVoice->InitVoice(&m_Sounds[sound], mode);
    return pVoice;
}

int EXAudio::psiRequestVoiceHandle(SampleHeaderData *p, int mode)
{
    Voice *pVoice = gpAudioVoice->VoiceAlloc(p->m_Handle, mode);
    if (pVoice) {
        pVoice->VoiceLock();
        return (int)pVoice;
    }
    return -1;
}

int psiStreamCreatePlaybackBuffer(int rate, int channels, int length, int mode, int)
{
    int sound = gpAudioVoice->FxCreate(0, length, rate, channels, true, 0, true);
    Voice *pVoice = gpAudioVoice->VoiceAlloc(sound, mode);
    if (pVoice) {
        pVoice->VoiceLock();
        pVoice->VoiceSetVolume(100.0f);
    }
    return (int)pVoice;
}
