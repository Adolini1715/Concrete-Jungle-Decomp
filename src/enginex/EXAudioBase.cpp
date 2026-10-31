#include "EXAudioStream.h"
#include "EXAudio.h"
#include "EXSortList.h"
#include "EXMemory.h"
#include <string.h>
#include <math.h>

// EXAudioStream.obj (TODO stand-ins until that object is decompiled)
void ES_MusicSetVolume();
void ES_StopAllStreams();
void ES_StopMusic();
void ES_MusicPause();
void ES_MusicUnPause();
int ES_FindpsiStreamHandle(int);
int ES_FindStreamShutdownVolume(int);
void ES_StreamKeyOn(int, int);
void ES_StreamKeyOff(int);
void ES_UnlockStream(int);
int ES_IsStreamPlaying(int);
int ES_HasStreamEnded(int);
struct StreamData;
StreamData *ES_GetStreamPointer(int);
void ES_StreamPause(StreamData *);
void ES_StreamUnPause(StreamData *);
int ES_InitialiseStream(int, int, XFileHandle *, int);
void psiStreamSetVolume(int, int);  // XBAudio.obj
extern "C" int printf(const char *, ...);

struct ES_HashName {
    int m_Hash;
    const char *m_pName;
};
static ES_HashName s_HashNames[] = { { -1, "" } };
static int s_RndSeed1 = 0xbc4ff2;
static int s_RndSeed2 = 0xc83e33;
static SFXPoint3 s_ZeroPoint;
static int s_MovieState;
static int s_FadeFrame;
static PjtMemorySlots s_DefaultMemorySlot;
static int s_SFXInitialised;
// Frequency scale per 1/240 octave-pair step, -240..240 (x1000): 1000 * 4^(i / 240), as stored.
static const short s_PitchTable[481] = {
    250, 251, 253, 254, 256, 257, 259, 260, 262, 263, 265, 266, 268, 269, 271, 273,
    274, 276, 277, 279, 281, 282, 284, 286, 287, 289, 291, 292, 294, 296, 297, 299,
    301, 302, 304, 306, 308, 310, 311, 313, 315, 317, 319, 320, 322, 324, 326, 328,
    330, 332, 334, 336, 338, 340, 342, 343, 345, 347, 349, 352, 354, 356, 358, 360,
    362, 364, 366, 368, 370, 372, 375, 377, 379, 381, 383, 386, 388, 390, 392, 395,
    397, 399, 401, 404, 406, 408, 411, 413, 416, 418, 420, 423, 425, 428, 430, 433,
    435, 438, 440, 443, 445, 448, 451, 453, 456, 459, 461, 464, 467, 469, 472, 475,
    477, 480, 483, 486, 489, 491, 494, 497, 500, 503, 506, 509, 512, 515, 518, 521,
    524, 527, 530, 533, 536, 539, 542, 545, 548, 552, 555, 558, 561, 564, 568, 571,
    574, 578, 581, 584, 588, 591, 595, 598, 602, 605, 609, 612, 616, 619, 623, 626,
    630, 634, 637, 641, 645, 648, 652, 656, 660, 664, 667, 671, 675, 679, 683, 687,
    691, 695, 699, 703, 707, 711, 715, 719, 724, 728, 732, 736, 741, 745, 749, 753,
    758, 762, 767, 771, 776, 780, 785, 789, 794, 798, 803, 808, 812, 817, 822, 826,
    831, 836, 841, 846, 851, 856, 861, 866, 871, 876, 881, 886, 891, 896, 901, 906,
    912, 917, 922, 928, 933, 938, 944, 949, 955, 960, 966, 972, 977, 983, 989, 994,
    1000, 1006, 1012, 1017, 1023, 1029, 1035, 1041, 1047, 1053, 1059, 1066, 1072, 1078, 1084, 1091,
    1097, 1103, 1110, 1116, 1122, 1129, 1136, 1142, 1149, 1155, 1162, 1169, 1176, 1182, 1189, 1196,
    1203, 1210, 1217, 1224, 1231, 1238, 1245, 1253, 1260, 1267, 1275, 1282, 1289, 1297, 1304, 1312,
    1320, 1327, 1335, 1343, 1350, 1358, 1366, 1374, 1382, 1390, 1398, 1406, 1414, 1422, 1431, 1439,
    1447, 1456, 1464, 1473, 1481, 1490, 1498, 1507, 1516, 1524, 1533, 1542, 1551, 1560, 1569, 1578,
    1587, 1597, 1606, 1615, 1625, 1634, 1643, 1653, 1662, 1672, 1682, 1692, 1701, 1711, 1721, 1731,
    1741, 1751, 1761, 1772, 1782, 1792, 1803, 1813, 1823, 1834, 1845, 1855, 1866, 1877, 1888, 1899,
    1910, 1921, 1932, 1943, 1954, 1966, 1977, 1988, 2000, 2012, 2023, 2035, 2047, 2059, 2071, 2083,
    2095, 2107, 2119, 2131, 2144, 2156, 2168, 2181, 2194, 2206, 2219, 2232, 2245, 2258, 2271, 2284,
    2297, 2311, 2324, 2338, 2351, 2365, 2378, 2392, 2406, 2420, 2434, 2448, 2462, 2477, 2491, 2505,
    2520, 2534, 2549, 2564, 2579, 2594, 2609, 2624, 2639, 2654, 2670, 2685, 2701, 2716, 2732, 2748,
    2764, 2780, 2796, 2812, 2828, 2845, 2861, 2878, 2895, 2911, 2928, 2945, 2962, 2979, 2997, 3014,
    3031, 3049, 3067, 3084, 3102, 3120, 3138, 3157, 3175, 3193, 3212, 3230, 3249, 3268, 3287, 3306,
    3325, 3344, 3364, 3383, 3403, 3422, 3442, 3462, 3482, 3502, 3523, 3543, 3564, 3584, 3605, 3626,
    3647, 3668, 3689, 3711, 3732, 3754, 3775, 3797, 3819, 3841, 3864, 3886, 3909, 3931, 3954, 3977,
    4000,
};

// Sound bank to memory slot table (m_Unk004).
struct ES_BankSlot {
    int m_Hash;
    int m_Slot;
};
struct ES_MemorySlotTable {
    int m_nSlots;
    PjtMemorySlots *m_pSlots;
    int m_nBanks;
    ES_BankSlot *m_pBanks;
};

void EXAudioBase::Init() {}

int EXAudioBase::ES_FindSoundBankSlot(int hash)
{
    int i;
    for (i = 0; i < 105; i++) {
        if (m_BankSlot[i] == hash) return i;
    }
    for (i = 0; i < 105; i++) {
        if (m_BankSlot[i] == -1) {
            m_BankSlot[i] = hash;
            return i;
        }
    }
    return -1;
}

void EXAudioBase::SFXSetTestMode(int mode)
{
    m_TestMode = mode;
}

int EXAudioBase::ES_GetSFXHashcode(int bank, int sample)
{
    int count = m_Banks[bank].m_NumSFX;
    EXAudio_SoundBank::Entry *pEntry = m_Banks[bank].m_pSFXTable;
    for (int i = 0; i < count; pEntry++, i++) {
        SFXParameters *pParams = (SFXParameters *)(m_Banks[bank].m_pSFXData + pEntry->m_Offset);
        SFXParameters::Sample *pSample = pParams->m_Samples;
        if (pParams->m_Unk1a) continue;
        int n = pParams->m_NumSamples;
        for (int j = 0; j < n; pSample++, j++) {
            if (pSample->m_Sample >= 0 && pSample->m_Sample == sample) return pEntry->m_Hash;
        }
    }
    return 0;
}

void EXAudioBase::ES_CacheBankHashCodesReset() {}

void EXAudioBase::ES_CacheBankHashCodes(int) {}

void EXAudioBase::ES_UnLoadCacheBankHashCodes(int) {}

unsigned int EXAudioBase::ES_Rnd(int range)
{
    if (range <= 0) return 0;
    s_RndSeed1 += s_RndSeed2;
    s_RndSeed2 += s_RndSeed1 + (s_RndSeed1 >> 31);
    return ((unsigned int)s_RndSeed1 >> 16) % (unsigned int)range;
}

int EXAudioBase::ES_Rndf()
{
    s_RndSeed1 += s_RndSeed2;
    s_RndSeed2 += s_RndSeed1 + (s_RndSeed1 >> 31);
    return ((unsigned int)s_RndSeed1 >> 16) % 0x401;
}

int EXAudioBase::ES_Rndsf()
{
    return ES_Rnd(0x801) - 0x400;
}

// NEAR: 82 bytes differ, retail branches on i > 0 (zero kept in edx); VC emits setle/dec/and here
void EXAudioBase::ES_SFXListInitialise()
{
    SFXItem *p = &m_SFX[0];
    m_pSFXList = 0;
    m_pFreeList = p;
    for (int i = 0; i < 60; i++, p++) {
        if (i == 59) {
            p->m_pNext = 0;
            p->m_pPrev = p - 1;
        } else {
            p->m_pNext = p + 1;
            if (i > 0) p->m_pPrev = p - 1;
            else p->m_pPrev = 0;
        }
    }
}

void EXAudioBase::ES_DuckerStart(int volume, int active, SFXItem *pSFX)
{
    m_DuckerFlag = pSFX->m_pParams->m_Duck != 0;
    m_pDuckerSFX = pSFX;
    m_DuckerStart = psiSFXGetTimer();
    m_DuckerActive = active;
    m_DuckerVolume = volume;
}

void EXAudioBase::ES_DuckerStop()
{
    m_DuckerActive = 0;
    m_pDuckerSFX = 0;
}

int EXAudioBase::ES_HasTimerPassed(unsigned int start, int time)
{
    int diff = psiSFXGetTimer() - start;
    if (diff < -0x40000000) return 0;
    if (diff > time) return 1;
    return 0;
}

void EXAudioBase::ES_PrintHashCode(int hash)
{
    int i = 0;
    hash &= 0xfffff;
    for (; s_HashNames[i].m_Hash != -1; i++) {
        if (s_HashNames[i].m_Hash == hash) {
            gpEXAudio->SFXprintf("%s", s_HashNames[i].m_pName);
            return;
        }
    }
    gpEXAudio->SFXprintf("Unknown HashCode 0x%X ", hash);
}

void EXAudioBase::SFXSetDebug(SFX_Debug_Data *pData)
{
    const int *p = (const int *)pData;
    m_Debug[0] = p[2];
    m_Debug[1] = p[3];
    m_Debug[2] = p[4];
    m_Debug[3] = p[5];
    m_Debug[4] = p[6];
    m_Debug[5] = p[7];
    m_Debug[6] = p[8];
    m_Debug[7] = p[9];
    m_Debug[8] = p[10];
    m_Debug[9] = p[11];
    m_Debug[10] = p[12];
    m_Debug[11] = p[13];
    m_Debug[12] = p[14];
    m_Debug[13] = p[15];
    m_Debug[14] = p[16];
    m_Debug[15] = p[17];
}

SFXItem *EXAudioBase::ES_FindSubSFX(void *pObj)
{
    for (SFXItem *p = m_pSFXList; p; p = p->m_pNext) {
        if (p->m_pObj == pObj && p->m_pParent) return p;
    }
    return 0;
}

void EXAudioBase::ES_ResetEnvironment()
{
    m_Reverb = 0;
    m_Unk240 = 0;
    psiSetReverb(0);
    m_UnderWater = 0;
}

void EXAudioBase::SFXSetEnvironment(int reverb, int unused, int underWater)
{
    if (reverb < 0) reverb = 0;
    if (reverb > 100) reverb = 100;
    m_UnderWater = underWater;
    m_Reverb = reverb;
}

int EXAudioBase::ES_ConvertValueToMusicFade(int value)
{
    if (value < 1) value = 1;
    if (value > 100) value = 100;
    return 10000000 / (value * 100);
}

void EXAudioBase::SFXStartCutscene()
{
    m_Cutscene = 1;
}

void EXAudioBase::SFXEndCutscene()
{
    if (m_SFXFader.m_State == -1) m_CutsceneEnd = 1;
    else m_Cutscene = 0;
}

void EXAudioBase::SFXFadeMusicDown(int speed, int mode)
{
    if (m_Debug[3]) gpEXAudio->SFXprintf("Command_SFXFadeMusicDown %d,%d\n", speed, mode);
    m_MusicFader.m_State = -1;
    m_MusicFader.m_Speed = ES_ConvertValueToMusicFade(speed);
    m_MusicFader.m_Stop = mode == 0;
}

void EXAudioBase::SFXSetSFXVolume(int volume)
{
    m_SFXVolumeReq = volume;
}

int EXAudioBase::SFXGetSFXVolume()
{
    return m_SFXVolume;
}

void EXAudioBase::SFXSetPosition(SFXPoint3 *pPos, const void *pObj, int hash)
{
    SFXItem *p = m_pSFXList;
    SFXPoint3 pos = *pPos;
    if (pos.x < -1e9f) pos.x = -1e9f;
    else if (pos.x > 1e9f) pos.x = 1e9f;
    if (pos.y < -1e9f) pos.y = -1e9f;
    else if (pos.y > 1e9f) pos.y = 1e9f;
    if (pos.z < -1e9f) pos.z = -1e9f;
    else if (pos.z > 1e9f) pos.z = 1e9f;
    for (; p; p = p->m_pNext) {
        if (p->m_pObj == pObj || pObj == 0) {
            if (p->m_Hash == hash || hash == -1) {
                p->m_Unk324 = pos;
                p->m_Pos = pos;
            }
        }
    }
}

int EXAudioBase::SFXGetLanguage()
{
    return m_Language;
}

void EXAudioBase::SFXSetLanguage(int language)
{
    m_Language = language;
}

void EXAudioBase::SFXSetMode(int mode)
{
    m_Mode = mode;
    psiSFXSetMode(mode);
}

int EXAudioBase::SFXGetMode()
{
    return m_Mode;
}

void EXAudioBase::SFXRemove(const void *pObj, int hash)
{
    SFXItem *p = m_pSFXList;
    if (hash != -1) hash &= 0xffffff;
    while (p) {
        SFXItem *pNext = p->m_pNext;
        if (p->m_pObj == pObj || pObj == 0) {
            if (p->m_Hash == hash || hash == -1) p->m_Remove = 1;
        }
        p = pNext;
    }
}

void EXAudioBase::SFXSetVolumeOverRide(int volume, const void *pObj, int hash)
{
    for (SFXItem *p = m_pSFXList; p; p = p->m_pNext) {
        if (p->m_pObj == pObj || pObj == 0) {
            if (p->m_Hash == hash || hash == -1) p->m_VolumeOverRide = volume;
        }
    }
}

void EXAudioBase::SFXSetPitchOverRide(int pitch, const void *pObj, int hash)
{
    for (SFXItem *p = m_pSFXList; p; p = p->m_pNext) {
        if (p->m_pObj == pObj || pObj == 0) {
            if (p->m_Hash == hash || hash == -1) p->m_PitchOverRide = pitch;
        }
    }
}

ES_LoadSoundBankDetails *EXAudioBase::ES_LB_GetFree()
{
    ES_LoadSoundBankDetails *p = &m_LB[m_LBWrite];
    m_LBWrite = (m_LBWrite + 1) % 10;
    return p;
}

ES_LoadSoundBankDetails *EXAudioBase::ES_LB_GetMe()
{
    return &m_LB[m_LBRead];
}

void EXAudioBase::ES_LB_GetNext()
{
    m_LBRead = (m_LBRead + 1) % 10;
    m_LBState = 0;
}

void EXAudioBase::SFXLoadSoundBank(int hash, XFileHandle *pFile)
{
    if (m_Debug[3]) gpEXAudio->SFXprintf("Command_SFXLoadSoundBank 0x%x\n", hash);
    ES_LoadSoundBankDetails *p = ES_LB_GetFree();
    if (!p) {
        gpEXAudio->SFXprintf("SFXLoadSoundBank Failed\n");
        return;
    }
    p->m_Hash = hash;
    p->m_File = *pFile;
    if (m_Debug[0] && m_LBBlocked) {
        gpEXAudio->SFXprintf("SFXLoadSoundBank() 0x%x BLOCKED by pending UnLoad of %d Banks\n", hash, m_LBBlocked);
    }
}

bool EXAudioBase::ES_LoadSoundBankHasJobs()
{
    if (m_LBWrite != m_LBRead) return true;
    return false;
}

char *EXAudioBase::ES_LB_StartFileLoad(XFileHandle *pFile, int *pHandle, char *pDest, int size, int offset)
{
    char *pBuffer = 0;
    *pHandle = psiAsyncOpenFile(pFile, FILEMAN_AUDIO, -1);
    if (*pHandle != -1) {
        pBuffer = psiAsyncCreateBuffer(size, offset);
        if (pBuffer) {
            psiAsyncReadFile(*pHandle, (int)pDest, size, pBuffer, 0x3000000);
        } else {
            gpEXAudio->SFXprintf("ERROR: SFXLoadSoundBank() Failed with psiAsyncCreateBuffer() \n");
            psiAsyncCloseFile(*pHandle);
        }
    }
    return pBuffer;
}

void EXAudioBase::ES_LB_EndFileLoad(int handle)
{
    psiAsyncCloseFile(handle);
}

void EXAudioBase::SFXPreMovieReset()
{
    for (SFXItem *p = m_pSFXList; p; p = p->m_pNext) p->m_Remove = 1;
    ES_StopAllStreams();
    psiSFXPreMovieReset();
    m_Unk1e0 = m_Unk1e4;
}

void EXAudioBase::SFXPostMovieReset()
{
    psiSFXPostMovieReset();
    s_MovieState = 2;
    if (m_Unk1e0 && StartMusicData.m_Hash != -1) {
        StartMusicData.m_Start = 1;
        StartMusicData.m_ForceRestart = 0;
    }
}

// NEAR: 18 bytes differ, retail loads ecx before lea edi for both rep movsd copies
void EXAudioBase::SFXInitialise2(XFileHandle *pProject, XFileHandle *pStream)
{
    if (m_InitState == 0) {
        memcpy(&m_ProjectFile, pProject, sizeof(XFileHandle));
        memcpy(&m_StreamFile, pStream, sizeof(XFileHandle));
        m_InitState = m_Unk008 ? 4 : 1;
    }
}

int EXAudioBase::ES_SetCueTimer(SFXParameters *p)
{
    return p->m_CueMin + ES_Rnd(p->m_CueMax - p->m_CueMin);
}

void EXAudioBase::SFXGameEffectMode(SFXGAMEEFFECT mode)
{
    if (mode == 0) {
        m_GameEffectValues[0] = 100;
        m_GameEffectValues[1] = 0;
        m_GameEffectValues[2] = 0;
        m_GameEffectValues[3] = 0;
        m_GameEffectValues[4] = 0;
    }
    if (m_GameEffect != mode) printf("SFXGameEffectMode(%d)\n", mode);
    m_GameEffect = mode;
}

void EXAudioBase::SFXGameEffectModeValues(int a, int b, int c, int d, int e)
{
    m_GameEffectValues[0] = a;
    m_GameEffectValues[1] = b;
    m_GameEffectValues[2] = c;
    m_GameEffectValues[3] = d;
    m_GameEffectValues[4] = e;
}

void EXAudioBase::SFXSetReverbType(SFXREVERBTYPES) {}

void EXAudioBase::SFXSetNumberOfListeners(int count)
{
    if (count > 1) psiSFXSetupListener(&s_ZeroPoint, &s_ZeroPoint, &m_ListenerB, &m_ListenerC, &m_ListenerA);
    m_NumListeners = count;
}

void EXAudioBase::SFXSetMusicVolume(int volume)
{
    m_MusicVolumeReq = volume;
    ES_MusicSetVolume();
}

int EXAudioBase::SFXGetMusicVolume()
{
    return m_MusicVolume;
}

EXAudio_Fader::EXAudio_Fader()
{
    m_State = 0;
    m_Volume = 0;
    m_Speed = 0;
    m_Stop = 0;
    m_Music = 0;
    m_Bank = 0;
    m_Wait = 0;
}

EXAudio_Fader::~EXAudio_Fader() {}

void EXAudio_Fader::FaderClear()
{
    m_Bank = 0;
    m_Music = 0;
    m_Speed = 0;
    m_State = 0;
    m_Stop = 1;
    m_Volume = 0;
    m_Wait = gpEXAudio->m_SFXFader.m_State == -1;
}

EXAudio_SoundBank::EXAudio_SoundBank()
{
    m_Unk00 = 0;
    m_Unk08 = 0;
    m_pSFXData = 0;
    m_pSFXTable = 0;
    m_Unk24 = 0;
    m_Unk04 = 0;
    m_NumSFX = 0;
    m_Loaded = 0;
    m_Unk1c = 0;
    m_Hash = 0;
    m_Unk28 = 0;
    m_Unk3c = 0;
    m_Status = 0;
}

EXAudio_SoundBank::~EXAudio_SoundBank() {}

SFXItem::~SFXItem() {}

void SFXItem::ClearVariables()
{
    m_pNext = 0;
    m_pPrev = 0;
    m_pParent = 0;
    m_Unk01c = 0;
    m_Hash = 0;
    m_Unk024 = 0;
    m_Unk028 = 0;
    m_Unk16c = 0;
    memset(m_Voices, 0, sizeof(m_Voices));
    memset(m_Unk170, 0, sizeof(m_Unk170));
    memset(m_Unk238, 0, sizeof(m_Unk238));
    m_pParams = 0;
    m_Bank = 0;
    m_Unk308 = 0;
    memset(&m_Pos, 0, sizeof(SFXPoint3));
    memset(&m_Unk318, 0, sizeof(SFXPoint3));
    memset(&m_Unk324, 0, sizeof(SFXPoint3));
    memset(&m_Unk330, 0, sizeof(SFXPoint3));
    m_pObj = 0;
    m_VolumeOverRide = 0;
    m_PitchOverRide = 0;
    m_Unk354 = 0;
    m_Unk358 = 0;
    m_Paused = 0;
    m_Unk360 = 0;
    m_Unk364 = 0;
    m_Unk368 = 0;
    m_Unk36c = 0;
    memset(&m_Unk370, 0, sizeof(SFXPoint3));
    m_Unk37c = 0;
    m_Unk380 = 0;
    m_Unk384 = 0;
    m_Unk388 = 0;
    m_Unk38c = 0;
    m_Pausable = 0;
    m_Remove = 0;
    m_Unk398 = 0;
}

int SFXItem::CountAllVoicesActive()
{
    int count = 0;
    if (this) {
        for (SFXItem *p = gpEXAudio->m_pSFXList; p; p = p->m_pNext) {
            if (this == p) continue;
            if (p->m_Remove) continue;
            if (p->m_pParams->m_Unk1a) continue;
            if (p->m_Unk388) continue;
            for (int i = 0; i < 20; i++) {
                if (p->m_Voices[i].m_pVoice) count++;
            }
        }
    }
    return count;
}

int SFXItem::SFXGetImportance()
{
    return m_pParams->m_Importance;
}

SFXItem *SFXItem::GetRealParent()
{
    SFXItem *p = m_pParent;
    int guard = 10;
    if (!p) return 0;
    while (p->m_pParent) {
        if (guard-- < 0) gpEXAudio->SFXprintf("ES_GetRealParent error\n");
        p = p->m_pParent;
    }
    return p;
}

void SFXItem::Delete()
{
    SFXItem *pNext = m_pNext;
    if (m_pPrev) m_pPrev->m_pNext = pNext;
    else gpEXAudio->m_pSFXList = pNext;
    if (pNext) pNext->m_pPrev = m_pPrev;
    m_pNext = gpEXAudio->m_pFreeList;
    m_pPrev = 0;
    gpEXAudio->m_pFreeList = this;
}

EXAudio_Voice *EXAudio_VoiceManager::ES_FindFreeVoiceHandle()
{
    for (int i = 0; i < 100; i++) {
        if (m_Voices[i].m_Used == 0) {
            EXAudio_Voice *p = &m_Voices[i];
            p->m_Used = 1;
            p->m_Streamed = 0;
            p->m_SubSFX = 0;
            return p;
        }
    }
    return 0;
}

EXAudio_Voice *EXAudio_VoiceManager::ES_RequestVoiceHandle(SampleHeaderData *pSamples, int sample, int stream, int a, int b, int c, int d)
{
    EXAudio_Voice *pVoice = 0;
    int handle;
    if (sample >= 0) {
        handle = gpEXAudio->psiRequestVoiceHandle(&pSamples[sample], c);
        if (handle != -1) {
            DEBUGTRACE("ES-> ES_RequestVoiceHandle() = %d\n", handle);
            pVoice = ES_FindFreeVoiceHandle();
            if (pVoice) pVoice->m_Handle = handle;
        }
    } else {
        handle = ES_InitialiseStream(sample, stream, 0, d);
        if (handle != -1) {
            pVoice = ES_FindFreeVoiceHandle();
            if (pVoice) {
                pVoice->m_Handle = handle;
                pVoice->m_Streamed = 1;
            }
        }
    }
    return pVoice;
}

EXAudio_Voice::EXAudio_Voice()
{
    m_Used = 0;
}

EXAudio_Voice::~EXAudio_Voice() {}

void EXAudio_Voice::ES_AudioKeyOn(int a)
{
    if (this == 0) return;
    if (m_SubSFX) return;
    if (m_Streamed) ES_StreamKeyOn(m_Handle, a);
    else gpEXAudio->psiSampleKeyOn(m_Handle, a);
}

void EXAudio_Voice::ES_AudioKeyOff()
{
    if (this == 0) return;
    if (m_SubSFX) {
        SFXItem *p = gpEXAudio->ES_FindSubSFX((void *)m_Handle);
        if (p) p->m_Remove = 1;
    } else if (m_Streamed) {
        ES_StreamKeyOff(m_Handle);
    } else {
        DEBUGTRACE("ES-> psiSampleKeyOff( %d )\n", m_Handle);
        gpEXAudio->psiSampleKeyOff(m_Handle);
    }
}

void EXAudio_Voice::ES_UnLockVoiceHandle()
{
    if (this == 0) return;
    DEBUGTRACE("ES-> ES_UnLockVoiceHandle() = %d\n", m_Handle);
    if (m_SubSFX) {
        SFXItem *p = gpEXAudio->ES_FindSubSFX((void *)m_Handle);
        if (p) p->m_Remove = 1;
    } else if (m_Streamed) {
        ES_UnlockStream(m_Handle);
    } else {
        gpEXAudio->psiUnLockVoiceHandle(m_Handle);
    }
    m_Used = 0;
}

int EXAudio_Voice::ES_AudioIsPlaying()
{
    if (this == 0) return 0;
    if (m_SubSFX) return gpEXAudio->ES_FindSubSFX((void *)m_Handle) != 0;
    if (m_Streamed) return ES_IsStreamPlaying(m_Handle);
    return gpEXAudio->psiIsSamplePlaying(m_Handle);
}

// NEAR: 12 bytes differ, retail places the shared return -1 block first
int EXAudio_Voice::ES_psiFindVoiceHandle()
{
    if (this == 0) return -1;
    if (m_SubSFX != 0) return -1;
    if (m_Streamed == 0) return m_Handle;
    return ES_FindpsiStreamHandle(m_Handle);
}

int EXAudio_Voice::ES_IsVoiceHandleStreamed()
{
    if (this == 0) return 0;
    return m_Streamed;
}

int EXAudio_Voice::ES_IsVoiceHandleSubSFX()
{
    if (this == 0) return 0;
    return m_SubSFX;
}

int EXAudio_Voice::ES_GetStreamShutdownVolume()
{
    if (this == 0) return 0;
    return ES_FindStreamShutdownVolume(m_Handle);
}

void EXAudio_Voice::ES_AudioPause()
{
    if (this == 0) return;
    if (!ES_AudioIsPlaying()) return;
    if (m_Streamed) ES_StreamPause(ES_GetStreamPointer(m_Handle));
    else gpEXAudio->psiSamplePause(m_Handle);
}

void EXAudio_Voice::ES_AudioUnPause()
{
    if (this == 0) return;
    if (!ES_AudioIsPlaying()) return;
    if (m_Streamed) ES_StreamUnPause(ES_GetStreamPointer(m_Handle));
    else gpEXAudio->psiSampleUnPause(m_Handle);
}

void EXAudio_Voice::ES_AudioSetVolume(int volume)
{
    if (this == 0) return;
    if (!ES_AudioIsPlaying()) return;
    volume = volume * volume / 100;
    if (m_Streamed) psiStreamSetVolume(ES_FindpsiStreamHandle(m_Handle), volume);
    else gpEXAudio->psiSetSampleVolume(m_Handle, volume);
}

int EXAudio_Voice::AudioHasEnded(SFXItem *pSFX)
{
    if (this == 0) return 1;
    if (pSFX->m_Unk37c > 0) return 0;
    if (m_SubSFX) {
        if (gpEXAudio->ES_FindSubSFX((void *)m_Handle)) return 0;
        return 1;
    }
    if (m_Streamed) return ES_HasStreamEnded(m_Handle);
    return gpEXAudio->psiIsSamplePlaying(m_Handle) == 0;
}

EXSortList::EXSortList(int count)
{
    m_Count = count;
    m_pNodes = (Node *)_EXAlloc((count + 2) * sizeof(Node), 0);
    memset(m_pNodes, 0, (m_Count + 2) * sizeof(Node));
    m_pNodes[0].m_Next = count;
    m_pNodes[0].m_Prev = -1;
    m_pNodes[count + 1].m_Next = count;
    m_pNodes[count + 1].m_Prev = -1;
    m_pNodes++;
}

EXSortList::~EXSortList()
{
    if (m_pNodes) {
        m_pNodes--;
        EXFree(m_pNodes);
    }
}

void EXSortList::Add(void *pData, u32 key)
{
    int i;
    for (i = 0; i < m_Count; i++) {
        if (m_pNodes[i].m_pData == 0) break;
    }
    if (i >= m_Count) return;
    for (int n = m_pNodes[-1].m_Next; n < m_Count + 1; n = m_pNodes[n].m_Next) {
        if (n == m_Count || key < m_pNodes[n].m_Key) {
            int prev = m_pNodes[n].m_Prev;
            m_pNodes[i].m_Prev = prev;
            m_pNodes[i].m_Next = n;
            m_pNodes[i].m_pData = pData;
            m_pNodes[i].m_Key = key;
            m_pNodes[prev].m_Next = i;
            m_pNodes[n].m_Prev = i;
            return;
        }
    }
}

void EXSortList::Remove(int i)
{
    if (i < 0) return;
    int next = m_pNodes[i].m_Next;
    int prev = m_pNodes[i].m_Prev;
    m_pNodes[i].m_pData = 0;
    m_pNodes[prev].m_Next = next;
    m_pNodes[next].m_Prev = prev;
}

void *EXSortList::GetHead(int &i)
{
    i = m_pNodes[-1].m_Next;
    return m_pNodes[i].m_pData;
}

void EXAudioBase::ES_UnLoadSoundBankUpdate()
{
    for (int i = 0; i < 105; i++) {
        if (m_Banks[i].m_Status == 2) ES_UnLoadSoundBankReleaseFinished(i);
    }
}

SFXItem *EXAudioBase::ES_SFXNew()
{
    SFXItem *p = m_pFreeList;
    if (!p) return 0;
    if (m_pSFXList) m_pSFXList->m_pPrev = p;
    m_pFreeList = p->m_pNext;
    p->ClearVariables();
    p->m_pPrev = 0;
    p->m_pNext = m_pSFXList;
    m_pSFXList = p;
    return p;
}

void EXAudioBase::ES_DuckerUpdate()
{
    int passed = ES_HasTimerPassed(m_DuckerStart, m_DuckerActive);
    if (passed) {
        m_DuckerVolume = 100;
        m_pDuckerSFX = 0;
    }
    int diff = (m_DuckerVolume - m_Unk1fc) << 10;
    if (diff == 0) return;
    int sign = diff >= 0 ? 1 : -1;
    int step;
    if (passed) step = diff / 15;
    else step = diff / 6;
    step >>= 10;
    if (step == 0) step = sign;
    m_Unk1fc += step;
    if (m_DuckerFlag) m_Unk200 = m_Unk1fc;
    else m_Unk200 = 100 - (100 - m_Unk1fc) * m_SFXVolume / 100;
    if (step) ES_MusicSetVolume();
}

void EXAudioBase::ES_FadeReset()
{
    m_SFXFader.FaderClear();
    m_MusicFader.FaderClear();
    m_MusicFader.m_Music = 1;
}

SFXItem::SFXItem()
{
    ClearVariables();
}

void SFXItem::KeyOnPreCachedSFX()
{
    m_Unk364 = gpEXAudio->m_Unk1c8;
    m_Unk360 = gpEXAudio->m_Unk1d0;
    for (int i = 0; i < 20; i++) {
        if (m_Voices[i].m_pVoice) m_Voices[i].m_pVoice->ES_AudioKeyOn(m_pParams->m_KeyOnArg);
    }
    if (m_pParams->m_DuckVolume) {
        gpEXAudio->ES_DuckerStart(100 - m_pParams->m_DuckVolume, m_pParams->m_DuckTime * 10, this);
    }
}

void SFXItem::Pause()
{
    if (m_Paused) return;
    for (int i = 0; i < 20; i++) {
        EXAudio_Voice *pVoice = m_Voices[i].m_pVoice;
        if (pVoice && !pVoice->m_SubSFX && !m_Unk37c) {
            pVoice->ES_AudioSetVolume(0);
            m_Voices[i].m_pVoice->ES_AudioPause();
        }
    }
    m_Paused = 1;
}

void SFXItem::UnPause()
{
    if (!m_Paused) return;
    for (int i = 0; i < 20; i++) {
        EXAudio_Voice *pVoice = m_Voices[i].m_pVoice;
        if (pVoice && !m_Unk37c && !pVoice->m_SubSFX) pVoice->ES_AudioUnPause();
    }
    m_Paused = 0;
}

void SFXItem::Remove()
{
    DEBUGTRACE("ES-> ES_SFXRemove()\n");
    for (int i = 0; i < 20; i++) {
        m_Voices[i].m_pVoice->ES_AudioKeyOff();
        if (m_Voices[i].m_pVoice->AudioHasEnded(this)) {
            m_Voices[i].m_pVoice->ES_UnLockVoiceHandle();
            m_Voices[i].m_pVoice = 0;
        }
    }
    if (this == gpEXAudio->m_pDuckerSFX) gpEXAudio->ES_DuckerStop();
    m_Unk398 = 1;
}

EXAudio_VoiceManager::EXAudio_VoiceManager() {}

EXAudio_VoiceManager::~EXAudio_VoiceManager() {}

EXAudioBase::EXAudioBase()
{
    m_Doppler = 1;
    m_Unk1e8 = 0;
    m_Unk010 = 0;
    m_TestMode = 0;
    m_Cutscene = 0;
    m_CutsceneEnd = 0;
    m_Unk1c0 = 0;
    m_Ticks = 0;
    m_Unk1c8 = 0;
    m_Unk1cc = 0;
    m_Unk1d0 = 0;
    m_SFXFadedDown = 0;
    m_Language = 0;
    m_Mode = 1;
    m_Unk1e4 = 0;
    m_Unk1e0 = 0;
    m_DuckerActive = 1;
    m_DuckerVolume = 100;
    m_DuckerStart = 0;
    m_Unk1fc = 100;
    m_Unk200 = 100;
    m_pDuckerSFX = 0;
    m_DuckerFlag = 0;
    m_SFXVolume = 100;
    m_MusicVolume = 60;
    m_SFXVolumeReq = 100;
    m_MusicVolumeReq = 60;
    m_Reverb = 0;
    m_Unk240 = 0;
    m_UnderWater = 0;
    m_Unk248 = 100;
    m_GameEffect = 0;
    m_Unk250 = 0;
    m_Unk254 = 0;
    m_Unk258 = 0;
    m_Unk25c = 100;
    m_Unk260 = 0;
    m_Unk264 = 0;
    m_Unk268 = 0;
    m_Unk26c = 0;
    m_GameEffectValues[0] = 100;
    m_GameEffectValues[1] = 0;
    m_GameEffectValues[2] = 0;
    m_GameEffectValues[3] = 0;
    m_GameEffectValues[4] = 0;
    m_Unk284 = 0;
    m_LBWrite = 0;
    m_LBRead = 0;
    m_LBState = 0;
    ES_SFXListInitialise();
    m_LBBlocked = 0;
    memset(m_BankSlot, 0, sizeof(m_BankSlot));
    memset(m_Unk43c, 0, sizeof(m_Unk43c));
    memset(m_FileStatus, 0, 0x2c8);
    m_NumListeners = 1;
    memset(&m_ListenerA, 0, sizeof(SFXPoint3));
    memset(&m_ListenerB, 0, sizeof(SFXPoint3));
    memset(&m_ListenerC, 0, sizeof(SFXPoint3));
    memset(m_ListenerPos, 0, sizeof(m_ListenerPos));
    memset(&m_Unk864, 0, sizeof(SFXPoint3));
    memset(&m_Unk87c, 0, sizeof(SFXPoint3));
    memset(&m_Unk888, 0, sizeof(SFXPoint3));
    m_ListenerB.y = 1.0f;
    m_ListenerC.z = 1.0f;
    m_ListenerA.x = 1.0f;
    m_Unk84c[0] = -999999.0f;
    m_Unk84c[1] = -888888.0f;
    m_Unk84c[2] = -777777.0f;
    m_PreCache = 0;
    m_Unk8e4 = 1;
    m_Unk8e8 = 0;
    m_Unk8ec = 0;
    m_Unk8f0 = 0;
    m_InitState = 0;
    m_Unk008 = 0;
    m_Unk00c = 0;
    m_Unk1185c = 0;
    m_Unk898 = 0;
    m_Debug[0] = 0;
    m_Debug[1] = 0;
    m_Debug[2] = 0;
    m_Debug[3] = 0;
    m_Debug[4] = 0;
    m_Debug[5] = 0;
    m_Debug[6] = 0;
    m_Debug[7] = 0;
    m_Debug[8] = 0;
    m_Debug[9] = 0;
    m_Debug[10] = 0;
    m_Debug[11] = 0;
    m_Debug[12] = 0;
    m_Debug[13] = 0;
    m_Debug[14] = 0;
    m_Debug[15] = 0;
    m_Unk87c.y = 1000.0f;
    m_Unk888.z = 1000.0f;
    m_Unk8e0 = -1;
}

EXAudioBase::~EXAudioBase() {}

void EXAudioBase::ES_UnLoadSoundBankRelease(int bank)
{
    SFXItem *p = m_pSFXList;
    while (p) {
        SFXItem *pNext = p->m_pNext;
        if (p->m_Bank == bank) {
            if (p->m_Paused) p->UnPause();
            p->m_Remove = 1;
        }
        p = pNext;
    }
    m_Banks[bank].m_Status = 2;
}

void EXAudioBase::SFXFadeSFXDown(int speed, int mode)
{
    if (m_Debug[3]) gpEXAudio->SFXprintf("Command_SFXFadeSFXDown %d,%d\n", speed, mode);
    m_SFXFadedDown = 1;
    m_SFXFader.m_State = -1;
    m_SFXFader.m_Speed = ES_ConvertValueToMusicFade(speed);
    if (mode) {
        m_SFXFader.m_Stop = 0;
        for (SFXItem *p = m_pSFXList; p; p = p->m_pNext) {
            if (!p->m_pParams->m_NoFade) {
                if (p->m_pParams->m_PauseOnFade) p->Pause();
                p->m_Pausable = 1;
            }
        }
    } else {
        m_SFXFader.m_Stop = 1;
    }
}

void EXAudio_Fader::FaderEndUpdate(int bank)
{
    if (m_Bank) {
        gpEXAudio->ES_UnLoadSoundBankRelease(bank);
        return;
    }
    if (m_Stop) {
        if (m_Music) {
            ES_StopMusic();
        } else {
            gpEXAudio->SFXRemove(0, -1);
        }
    } else {
        if (m_Music) {
            ES_MusicPause();
        } else {
            for (SFXItem *p = gpEXAudio->m_pSFXList; p; p = p->m_pNext) {
                if (p->m_Pausable) p->Pause();
            }
        }
    }
}

void EXAudio_Fader::FaderUpdate(int bank)
{
    if (m_State == 0) return;
    if (m_State == 1) {
        m_Volume += gpEXAudio->m_Ticks * m_Speed;
        if (m_Volume > 10000000) {
            m_Volume = 10000000;
            m_State = 0;
        }
        return;
    }
    if (m_Bank && m_Wait && gpEXAudio->m_SFXFader.m_State == -1) return;
    m_Wait = 0;
    m_Volume -= gpEXAudio->m_Ticks * m_Speed;
    if (m_Volume < 0) {
        m_Volume = 0;
        m_State = 0;
        FaderEndUpdate(bank);
    }
}

void EXAudioBase::SFXUnLoadSoundBankForced(int hash)
{
    if (m_Debug[3]) {
        gpEXAudio->SFXprintf(">>> Banks Still Loaded = ");
        for (int j = 0; j < 105; j++) {
            if (m_Banks[j].m_Loaded && !m_Banks[j].m_Unk1c && hash != m_Banks[j].m_Hash) {
                gpEXAudio->SFXprintf("0x%x :", m_Banks[j].m_Hash);
            }
        }
        gpEXAudio->SFXprintf("\n");
    }
    int i = 0;
    while (m_BankSlot[i] != hash) {
        i++;
        if (i >= 105) return;
    }
    if (!m_Banks[i].m_Loaded) return;
    if (m_Banks[i].m_Unk1c) return;
    if (m_Debug[3]) gpEXAudio->SFXprintf("SFXUnLoadSoundBank Pending = 0x%x\n", hash);
    m_Banks[i].m_Unk1c = 1;
    m_Banks[i].m_Status = 1;
    m_Banks[i].m_Fader.m_State = -1;
    m_LBBlocked++;
    if (m_TestMode == 1) ES_UnLoadSoundBankRelease(i);
}

void EXAudioBase::ES_FadeUpdate()
{
    s_FadeFrame++;
    m_SFXFader.FaderUpdate(-1);
    m_MusicFader.FaderUpdate(-1);
    if (m_CutsceneEnd && m_SFXFader.m_State != -1) {
        m_Cutscene = 1;
        m_CutsceneEnd = 0;
    }
    for (int i = 0; i < 105; i++) {
        if (m_Banks[i].m_Loaded) m_Banks[i].m_Fader.FaderUpdate(i);
    }
}

void EXAudioBase::SFXFadeSFXUp(int speed)
{
    if (m_Debug[3]) gpEXAudio->SFXprintf("Command_SFXFadeSFXUp %d\n", speed);
    m_SFXFadedDown = 0;
    if (m_SFXFader.m_State == -1) {
        m_SFXFader.FaderEndUpdate(-1);
        if (m_Debug[0]) gpEXAudio->SFXprintf("WARNING: SFXFadeSFXUp() called before SFXFadeSFXDown() completion!\n");
    }
    m_SFXFader.m_State = 1;
    m_SFXFader.m_Speed = ES_ConvertValueToMusicFade(speed);
    for (SFXItem *p = m_pSFXList; p; p = p->m_pNext) {
        if (p->m_Pausable) {
            p->UnPause();
            p->m_Pausable = 0;
        }
    }
}

void EXAudioBase::SFXFadeMusicUp(int speed)
{
    if (m_Debug[3]) gpEXAudio->SFXprintf("Command_SFXFadeMusicUp %d\n", speed);
    if (m_MusicFader.m_State == -1) {
        m_MusicFader.FaderEndUpdate(-1);
        if (m_Debug[0]) gpEXAudio->SFXprintf("WARNING: SFXFadeMusicUp() called before SFXFadeMusicDown() completion!\n");
    }
    m_MusicFader.m_State = 1;
    m_MusicFader.m_Speed = ES_ConvertValueToMusicFade(speed);
    if (!m_MusicFader.m_Stop) ES_MusicUnPause();
}

void EXAudioBase::SFXPreCache(int hash, int count)
{
    if (count > 200 || count == 0) count = 200;
    m_PreCache = count + 20;
    SFXStart3D(hash, 0, 0, -1, -1, -1, 0);
}

void SFXItem::UpdateEnding(SFXItem **ppFirst)
{
    if (m_pParams->m_Unk16) {
        m_Unk170[0] -= gpEXAudio->m_Unk8e8;
        if (m_Unk170[0] > 0) return;
        if (m_pParent) gpEXAudio->m_Unk898 = 1;
        if (SFXSetup() && m_Unk308 < 2) *ppFirst = this;
        gpEXAudio->m_Unk898 = 0;
    } else {
        m_Unk398 = 1;
    }
}

void EXAudioBase::SFXUnLoadSoundBank(int bank)
{
    if (m_Debug[3]) gpEXAudio->SFXprintf("Command_SFXUnLoadSoundBank 0x%x\n", bank);
    if (bank == -1) {
        for (int i = 0; i < 105; i++) {
            if (m_Banks[i].m_Loaded && !m_Banks[i].m_Unk1c) {
                int hash = m_Banks[i].m_Hash;
                if (SoundBankGetMemorySlot(hash)->m_Type != 1) SFXUnLoadSoundBankForced(hash);
            }
        }
        return;
    }
    if (SoundBankGetMemorySlot(bank)->m_Type != 1) SFXUnLoadSoundBankForced(bank);
}

#define ES_SLOTS ((ES_MemorySlotTable *)m_Unk004)
PjtMemorySlots *EXAudioBase::SoundBankGetMemorySlot(int hash)
{
    s_DefaultMemorySlot.m_Unk4 = 0;
    s_DefaultMemorySlot.m_Type = 10;
    s_DefaultMemorySlot.m_Unk0 = 0;
    if (!m_Unk010) return &s_DefaultMemorySlot;
    PjtMemorySlots *s = ES_SLOTS->m_pSlots;
    ES_BankSlot *b = ES_SLOTS->m_pBanks;
    int i;
    for (i = 0; i < ES_SLOTS->m_nBanks; b++, i++) {
        if ((hash & 0xfffff) == b->m_Hash) {
            for (int j = 0; j < ES_SLOTS->m_nSlots; s++, j++)
                if (s->m_Unk0 == b->m_Slot) return s;
        }
    }
    printf("-----------------------------------------------------\n");
    printf("SoundBankGetMemorySlot Failed to Find %X\n", hash);
    b = ES_SLOTS->m_pBanks;
    for (i = 0; i < ES_SLOTS->m_nBanks; b++, i++) {
        printf("SoundBank Index %d\n", i);
        printf(">>>>>> SoundBankData->HashCode          %6.6X\n", b->m_Hash);
        printf(">>>>>> SoundBankData->MemorySlotNumber  %d\n", b->m_Slot);
    }
    printf("Switching to old system !!! \n");
    m_Unk010 = 0;
    return &s_DefaultMemorySlot;
}
#undef ES_SLOTS

void EXAudioBase::SFXInitialise()
{
    if (s_SFXInitialised) {
        gpEXAudio->SFXprintf("AUDIO ERROR: SFXInitialise() should only be called once! Please stop calling me!\n");
        return;
    }
    memset(MusicFilePosStoreData, 0, sizeof(MusicFilePosStoreData));
    memset(StreamWaitingIntialise, 0, sizeof(StreamWaitingIntialise));
    memset(Streams, 0, sizeof(Streams));
    memset(StreamBuffers, 0, sizeof(StreamBuffers));
    memset(StreamMemCpyHandles, 0, sizeof(StreamMemCpyHandles));
    s_SFXInitialised = 1;
    psiSFXSetMode(m_Mode);
    psiInitialiseSoundSystem();
    psiInitialiseSoundRam();
    int i;
    for (i = 0; i < 105; i++) m_BankSlot[i] = -1;
    for (i = 0; i < 60; i++) m_Unk43c[i] = -1;
    m_Unk8e4 = 1;
    ES_InitialiseAudioStreamSystem();
    m_DuckerActive = 1;
    m_DuckerVolume = 100;
    m_Unk1fc = 100;
    m_Unk200 = 100;
    m_DuckerStart = 0;
    m_Unk1d0 = 0;
    ES_FadeReset();
    m_Reverb = 0;
    m_Unk240 = 0;
    psiSetReverb(0);
    m_UnderWater = 0;
}

void EXAudioBase::ES_TryPlayMusic()
{
    if (!StartMusicData.m_Start) return;
    StartMusicData.m_Unk1bc++;
    MusicStartSection = StartMusicData.m_Section;
    MusicForceRestart = StartMusicData.m_ForceRestart;
    int found = -1;
    int resumed = 0;
    for (int i = 0; i < 2; i++) {
        if (!Streams[i].m_Fade.m_Paused) continue;
        if (((MusicStartSection << 8) | StartMusicData.m_Hash) == Streams[i].m_Section && !StartMusicData.m_ForceRestart) {
            found = i;
            resumed = 1;
        } else {
            ES_StreamKeyOff(Streams[i].m_Index);
        }
    }
    if (found == -1) {
        found = ES_InitialiseStream(StartMusicData.m_Hash, 0, &StartMusicData.m_File, StartMusicData.m_Hash);
        if (found == -1) return;
    }
    int other = 1 - found;
    Streams[found].m_Fade.m_Speed = StartMusicData.m_FadeOut;
    Streams[found].m_Fade.m_Volume = 0;
    Streams[found].m_Fade.m_Time = StartMusicData.m_FadeIn;
    Streams[found].m_Fade.m_State = 1;
    Streams[found].m_Fade.m_Unk0c = 0;
    Streams[found].m_Fade.m_FadeUpWait = 0;
    Streams[found].m_Fade.m_PauseWait = 0;
    Streams[found].m_Fade.m_PauseAfter = 0;
    if (resumed)
        Streams[found].m_Fade.m_PauseAfter = 1;
    else
        Streams[found].m_Fade.m_Paused = 0;
    if (StartMusicData.m_FadeAndStop) {
        Streams[found].m_Fade.m_FadeUpWait = 1;
    } else if (Streams[other].m_State) {
        Streams[found].m_Fade.m_PauseWait = 1;
        Streams[other].m_Fade.m_Unk0c = 1;
        StartMusicData.m_Start = 0;
        return;
    }
    StartMusicData.m_Start = 0;
    Streams[found].m_KeyOn = 1;
}

void SFXItem::CalculateDopplerShift()
{
    EXAudio *a = gpEXAudio;
    float dx = m_Pos.x - a->m_Unk84c[0];
    float dy = m_Pos.y - a->m_Unk84c[1];
    float dz = m_Pos.z - a->m_Unk84c[2];
    float len = a->psiSquareRoot(dx * dx + dy * dy + dz * dz);
    if (len != 0.0f) {
        float inv = 1.0f / len;
        dx *= inv;
        dy *= inv;
        dz *= inv;
    }
    float v = ((m_Velocity.x - gpEXAudio->m_Unk864.x) * dx + (m_Velocity.y - gpEXAudio->m_Unk864.y) * dy +
               (m_Velocity.z - gpEXAudio->m_Unk864.z) * dz) * (float)m_pParams->m_Doppler * 0.01f;
    if (v == 0.0f || v >= 100.0f)
        m_Doppler = 0.0f;
    else if (v <= -100.0f)
        m_Doppler = 4096.0f;
    else
        m_Doppler = log(1.0f - v * 0.01f) * 4096.0f;
}

int SFXItem::CombineVolumes(int vol)
{
    if (gpEXAudio->m_Debug[5]) return 100;
    SFXParameters *p = m_pParams;
    u8 noScale = p->m_NoVolumeScale;
    int v;
    if (!noScale) {
        if (p->m_Duck)
            v = vol * gpEXAudio->m_MusicVolume / 100 * gpEXAudio->m_Unk1fc / 100;
        else
            v = vol * gpEXAudio->m_SFXVolume / 100;
    } else {
        v = vol;
    }
    v = m_VolumeOverRide * v / 100;
    if (gpEXAudio->m_UnderWater && !p->m_NoUnderWater) v = gpEXAudio->m_Unk248 * v / 100;
    v = m_Unk36c * v / 100;
    v = v * m_pParams->m_Volume / 100;
    v = v * m_Unk38c / 100;
    if (!m_pParams->m_DuckVolume) v = (100 - (100 - gpEXAudio->m_Unk1fc) / 3) * v / 100;
    v = v * v / 100;
    if (!noScale) {
        if (m_pParams->m_Duck)
            v = gpEXAudio->m_MusicFader.m_Volume * v / 10000000;
        else if (gpEXAudio->m_SFXFader.m_Stop || m_Pausable)
            v = gpEXAudio->m_SFXFader.m_Volume * v / 10000000;
    } else if (m_Unk388) {
        gpEXAudio->m_Unk1e8 = 1;
    }
    return gpEXAudio->m_Banks[m_Bank].m_Fader.m_Volume * v / 10000000;
}

void EXAudioBase::ES_UnLoadSoundBankReleaseFinished(int bank)
{
    SFXItem *s;
    for (s = m_pSFXList; s; s = s->m_pNext) {
        if (s->m_Bank == bank) {
            if (s->m_Unk024 % 120 == 0)
                gpEXAudio->SFXprintf("ES-> ES_UnLoadSoundBankReleaseFinished Waiting %X\n", s->m_Hash);
            return;
        }
    }
    DEBUGTRACE("ES-> ES_UnLoadSoundBankReleaseFinished End\n");
    DEBUGTRACE("pih");
    for (s = m_pSFXList; s; s = s->m_pNext) {
        DEBUGTRACE("-");
        DEBUGTRACE("ES-> Sfx sbs %d  Bank sbs %d\n", s->m_Bank, bank);
    }
    m_Banks[bank].m_Status = 3;
    if (m_Banks[bank].m_Unk3c == 7) {
        m_LBBlocked--;
        if (m_Debug[3]) gpEXAudio->SFXprintf("psiBankMemoryDeAlloc Bank = 0x%x\n", m_Banks[bank].m_Hash);
        psiBankMemoryDeAlloc(m_Banks[bank].m_Unk28);
        SampleHeaderData *h = (SampleHeaderData *)m_Banks[bank].m_Unk08;
        for (int i = 0; i < m_Banks[bank].m_Unk04; i++) {
            psiUnLoadSampleInitialise(h);
            h = (SampleHeaderData *)((char *)h + 0x20);
        }
    }
    psiAsyncDeleteBuffer(m_Banks[bank].m_pSFXData);
    psiAsyncDeleteBuffer((char *)m_Banks[bank].m_Unk00);
    if (m_Banks[bank].m_Unk24) psiAsyncDeleteBuffer((char *)m_Banks[bank].m_Unk24);
    m_Banks[bank].m_Loaded = 0;
    m_Banks[bank].m_Unk1c = 0;
    if (m_Debug[3]) gpEXAudio->SFXprintf("ES_UnLoadSoundBankReleaseFinished = 0x%x\n", m_Banks[bank].m_Hash);
}

void EXAudioBase::SFXSetupListener(SFXPoint3 *pPos, SFXPoint3 *pVel, SFXPoint3 *pA, SFXPoint3 *pB, SFXPoint3 *pC, int n)
{
    if ((unsigned int)n >= 4) {
        SFXprintf("Illegal ListenerNumber\n");
        return;
    }
    SFXPoint3 p = *pPos;
    if (p.x < -1e9f)
        p.x = -1e9f;
    else if (p.x > 1e9f)
        p.x = 1e9f;
    if (p.y < -1e9f)
        p.y = -1e9f;
    else if (p.y > 1e9f)
        p.y = 1e9f;
    if (p.z < -1e9f)
        p.z = -1e9f;
    else if (p.z > 1e9f)
        p.z = 1e9f;
    if (m_NumListeners == 1) {
        psiSFXSetupListener(&p, pVel, pA, pB, pC);
        *(SFXPoint3 *)m_Unk84c = p;
        m_Unk870 = *pC;
        m_Unk87c = *pA;
        m_Unk888 = *pB;
        m_ListenerPos[0] = p;
        m_Unk864.x = m_Unk84c[0] - m_PrevListenerPos.x;
        m_Unk864.y = m_Unk84c[1] - m_PrevListenerPos.y;
        m_Unk864.z = m_Unk84c[2] - m_PrevListenerPos.z;
        m_PrevListenerPos = *(SFXPoint3 *)m_Unk84c;
    } else {
        m_Unk870 = m_ListenerA;
        m_Unk87c = m_ListenerB;
        m_Unk888 = m_ListenerC;
        m_ListenerPos[n] = p;
    }
}

int SFXItem::GetDist()
{
    EXAudio *a = gpEXAudio;
    float best = 0.0f;
    SFXPoint3 *p = a->m_ListenerPos;
    for (int i = 0; i < a->m_NumListeners; i++, p++) {
        float dx = (p->x - m_Pos.x) * 0.01f;
        float dy = (p->y - m_Pos.y) * 0.01f;
        float dz = (p->z - m_Pos.z) * 0.01f;
        float d = dz * dz + dy * dy + dx * dx;
        if (!i || d < best) best = d;
    }
    return (int)(a->psiSquareRoot(best) * 0.1f);
}

void SFXItem::GetMyPos(int force)
{
    EXAudio *a = gpEXAudio;
    if (a->m_NumListeners <= 1 && m_pParams->m_Type != 3 && m_pParams->m_Type != 5 && m_pParams->m_Type != 7 && !force)
        return;
    float best = 0.0f;
    for (int i = 0; i < a->m_NumListeners; i++) {
        float dx = (a->m_ListenerPos[i].x - m_Pos.x) * 0.01f;
        float dy = (a->m_ListenerPos[i].y - m_Pos.y) * 0.01f;
        float dz = (a->m_ListenerPos[i].z - m_Pos.z) * 0.01f;
        float d = dz * dz + dy * dy + dx * dx;
        if (!i || d < best) best = d;
    }
    m_Unk324 = *(SFXPoint3 *)a->m_Unk84c;
    if (m_pParams->m_Type == 5 || m_pParams->m_Type == 7) {
        m_Unk324.x += m_Unk330.x;
        m_Unk324.y += m_Unk330.y;
        m_Unk324.z += m_Unk330.z;
    }
    if (m_pParams->m_Type == 7) {
        m_Unk38c = 100;
        return;
    }
    float d = gpEXAudio->psiSquareRoot(best) * 100.0f;
    float lo = (float)m_Unk354;
    float hi = (float)m_Unk358;
    if (d > lo) {
        m_Unk38c = 0;
        return;
    }
    if (!(hi > d)) {
        float range = lo - hi;
        if (range >= 1.0f) {
            float t = (range - (d - hi)) * 100.0f;
            m_Unk38c = (int)(t / range);
            if (m_Unk38c < 0) m_Unk38c = 0;
            if (m_Unk38c > 100) m_Unk38c = 100;
            return;
        }
    }
    m_Unk38c = 100;
}

// Frequency scale (x1000) for a pitch shift in 1/240ths of two octaves, clamped to +-240.
static inline int ES_PitchScale(int pitch)
{
    if (pitch < -240) pitch = -240;
    else if (pitch > 240) pitch = 240;
    return s_PitchTable[pitch + 240];
}

// Fill the platform parameters of one voice of an SFX: handle, mode, radii, combined volume,
// frequency (pitch override, doppler and game effect applied), pan and the position / velocity.
void EXAudioBase::ES_FillpsiSFXData(psiSFX *pSfx, SFXItem *pItem, int voice)
{
    SFXItem::VoiceSlot &slot = pItem->m_Voices[voice];
    if (!slot.m_pVoice) {
        gpEXAudio->SFXprintf("ES_FillpsiSFXData Called with non valid handle!\n");
        return;
    }
    pSfx->m_Mode = (signed char)pItem->m_pParams->m_Type;
    pSfx->m_Voice = slot.m_pVoice->ES_psiFindVoiceHandle();
    pSfx->m_Volume = pItem->CombineVolumes(slot.m_Volume);
    pSfx->m_pVel = &pItem->m_Unk370;
    pSfx->m_Freq = slot.m_Freq;
    pSfx->m_Pan = slot.m_Pan;
    u8 type = pItem->m_pParams->m_Type;
    if (type == 3 || type == 5 || type == 7) {
        pSfx->m_MinRadius = 50;
        pSfx->m_MaxRadius = 50;
    } else {
        pSfx->m_MinRadius = pItem->m_Unk358;
        pSfx->m_MaxRadius = pItem->m_Unk354;
    }
    int pitch = 0;
    if (pItem->m_PitchOverRide) pitch = pItem->m_PitchOverRide / 100;
    if (m_Doppler) {
        u8 t = pItem->m_pParams->m_Type;
        if (!(t & 8) && pItem->m_pParams->m_Doppler && (t & 1)) {
            pItem->CalculateDopplerShift();
            pitch += (int)pItem->m_Doppler;
        }
    }
    pSfx->m_Freq = ES_PitchScale(pitch) * pSfx->m_Freq / 1000;
    pSfx->m_Filter[0] = 0;
    pSfx->m_Filter[1] = 0;
    pSfx->m_Filter[2] = 0;
    if (pItem->m_pParams->m_GameEffect) {
        // Game effect (SFXGameEffectMode): 1 detune, 2 random detune, 3 pitch / volume / filter.
        switch (m_Unk254) {
        case 1:
            pSfx->m_Freq += m_Unk258 * -10;
            break;
        case 2:
            pSfx->m_Freq += ES_Rnd(m_Unk258 * 10) - 5000;
            break;
        case 3:
            pSfx->m_Freq = ES_PitchScale(m_Unk260 * m_Unk258 / 100000) * pSfx->m_Freq / 1000;
            pSfx->m_Volume = (m_Unk25c * pSfx->m_Volume * m_Unk258 / 100 + (1000 - m_Unk258) * pSfx->m_Volume) / 1000;
            pSfx->m_Filter[0] = m_Unk264;
            pSfx->m_Filter[1] = m_Unk268;
            pSfx->m_Filter[2] = m_Unk26c;
            break;
        }
    }
    pSfx->m_Streamed = slot.m_pVoice->ES_IsVoiceHandleStreamed();
    if (pSfx->m_Streamed) pSfx->m_Volume = pSfx->m_Volume * slot.m_pVoice->ES_GetStreamShutdownVolume() / 100;
    pSfx->m_pPos = &pItem->m_Unk324;
}

static int s_EnvEffectState;    // game effect cross-fade: 0 steady, 1 fading out, 2 fading in
static int s_EnvDebugCount;
static const char s_ModeNames[7][12] = { "Mono", "Stereo", "Surround", "ProLogic2", "Dolby51", "Dolby71", "" };

static inline int ES_Sign(int v) { return v < 0 ? -1 : (v != 0); }

// Per-update smoothing of the environment: cross-fade game effect mode changes, ease the effect
// values, reverb, under-water volume and the music / sfx volumes towards their requests.
void EXAudioBase::ES_UpdateEnvironment()
{
    if (!m_Unk254) {
        m_Unk250 = m_GameEffect;
        m_Unk254 = m_GameEffect;
    }
    if (s_EnvEffectState == 0) {
        if (m_GameEffect != m_Unk250) {
            m_Unk250 = m_GameEffect;
            s_EnvEffectState = 1;
            m_Unk258 = 1000;
        }
    } else if (s_EnvEffectState == 1) {
        m_Unk258 -= 10;
        if (m_Unk258 < 1) {
            m_Unk254 = m_Unk250;
            m_Unk258 = 0;
            s_EnvEffectState = 2;
        }
    } else if (s_EnvEffectState == 2) {
        m_Unk258 += 10;
        if (m_Unk258 >= 1000) {
            s_EnvEffectState = 0;
            m_Unk258 = 1000;
        }
    }
    int d = m_GameEffectValues[0] - m_Unk25c;
    m_Unk25c += d / 20 + ES_Sign(d);
    d = m_GameEffectValues[1] - m_Unk260;
    m_Unk260 += d / 20 + ES_Sign(d);
    d = m_GameEffectValues[2] - m_Unk264;
    m_Unk264 += d / 20 + ES_Sign(d);
    d = m_GameEffectValues[3] - m_Unk268;
    m_Unk268 += d / 20 + ES_Sign(d);
    d = m_GameEffectValues[4] - m_Unk26c;
    m_Unk26c += d / 20 + ES_Sign(d);

    d = m_Reverb - m_Unk240;
    if (d > 16) d = 16;
    else if (d < -4) d = -4;
    m_Unk240 += d;
    psiSetReverb(m_SFXFader.m_Volume * m_Unk240 / 10000000);

    int target = m_UnderWater ? 50 : 100;
    m_Unk248 += ES_Sign(target - m_Unk248) * 2;
    if (m_Unk248 < 50) m_Unk248 = 50;
    if (m_Unk248 > 100) m_Unk248 = 100;
    m_MusicVolume += ES_Sign(m_MusicVolumeReq - m_MusicVolume) * 2;
    if (m_MusicVolume < 0) m_MusicVolume = 0;
    if (m_MusicVolume > 100) m_MusicVolume = 100;
    m_SFXVolume += ES_Sign(m_SFXVolumeReq - m_SFXVolume) * 2;
    if (m_SFXVolume < 0) m_SFXVolume = 0;
    if (m_SFXVolume > 100) m_SFXVolume = 100;

    if (m_Debug[4] && s_EnvDebugCount-- < 0) {
        s_EnvDebugCount = 100;
        gpEXAudio->SFXprintf("---> Debug Print Environment <---\n");
        gpEXAudio->SFXprintf("Reverb = %3.3d , UnderWater = ", m_Reverb);
        if (m_UnderWater) gpEXAudio->SFXprintf("TRUE\n");
        else gpEXAudio->SFXprintf("FALSE\n");
        gpEXAudio->SFXprintf("MFX Vol = %3.3d , SFX Vol %3.3d , Mode = ", m_MusicVolume, m_SFXVolume);
        gpEXAudio->SFXprintf("m_SFXUnderWaterVolume %d\n", m_Unk248);
        int mode = m_Mode;
        if (mode >= 0 && mode < 7)   // retail allows 6, one past the names
            gpEXAudio->SFXprintf("%s (%d)\n", s_ModeNames[mode], mode);
        else
            gpEXAudio->SFXprintf("Unknown (%d)\n", mode);
    }
}

// Cull this SFX before it starts: distance cull quiet 3D sounds, keep the active voice count down
// by stealing the least important SFX, and apply the per-hash and per-group instance limits
// (stealing the oldest / quietest instance or refusing). Returns 0 if this SFX must not start.
int SFXItem::ProcessCulling()
{
    int active = CountAllVoicesActive();
    SFXParameters *pParams = m_pParams;
    if (pParams->m_Type == 1 && pParams->m_Importance < 100 && !pParams->m_Unk16 && !gpEXAudio->m_Unk898 &&
        GetDist() > m_Unk354) {
        if (gpEXAudio->m_Debug[13])
            gpEXAudio->SFXprintf("3d SFX Distance Culled HC %X Pos %d,%d,%d\n", m_Hash, (double)m_Pos.x, (double)m_Pos.y,
                                 (double)m_Pos.z);
        m_Remove = 1;
        return 0;
    }
    if (active > 22) {
        for (int n = active - 22; n > 0; n--) {
            SFXItem *pBest = 0;
            int best = 10000;
            SFXItem *p = gpEXAudio->m_pSFXList;
            if (!p) continue;
            for (; p; p = p->m_pNext) {
                if (!p->m_Remove && this != p && !p->m_Unk388 && !p->m_pParent && p->m_pParams->m_Importance < best) {
                    best = p->m_pParams->m_Importance;
                    pBest = p;
                }
            }
            if (pBest) pBest->m_Remove = 1;
        }
        if (m_pParams->m_RejectWhenFull) {
            m_Remove = 1;
            return 0;
        }
    }
    pParams = m_pParams;
    if ((!pParams->m_MaxInstances && !pParams->m_GroupMax) || gpEXAudio->m_Unk898) return 1;
    int instances = 0;
    int groupCount = 0;
    if (!pParams->m_MaxInstances) instances = -1;
    if (!pParams->m_GroupMax) groupCount = -1;
    for (SFXItem *p = gpEXAudio->m_pSFXList; p; p = p->m_pNext) {
        if (p->m_Remove || this == p) continue;
        pParams = m_pParams;
        if (pParams->m_MaxInstances && m_Hash == p->m_Hash) instances++;
        if (pParams->m_GroupMax && pParams->m_Group == p->m_pParams->m_Group && !p->m_pParent && !pParams->m_StealQuietest) {
            if (p->m_pParams->m_GroupPriority) {
                m_Remove = 1;
                return 0;
            }
            if (!pParams->m_GroupPriority) groupCount++;
            else p->m_Remove = 1;
        }
    }
    pParams = m_pParams;
    if (instances < pParams->m_MaxInstances && groupCount < (int)pParams->m_GroupMax) return 1;
    bool groupFull = false;
    if (groupCount >= (int)pParams->m_GroupMax && instances < pParams->m_MaxInstances) groupFull = true;
    if ((pParams->m_RejectWhenFull && !groupFull) || (pParams->m_GroupReject && groupFull)) {
        m_Remove = 1;
        return 0;
    }
    u8 steal = pParams->m_StealQuietest;
    SFXItem *pVictim = 0;
    int best = -1;
    if (groupFull) {
        // The oldest member of the group makes way.
        SFXItem *p = gpEXAudio->m_pSFXList;
        if (!p) return 1;
        for (; p; p = p->m_pNext) {
            if (pParams->m_Group == p->m_pParams->m_Group && this != p && !p->m_pParent && !p->m_Remove && best < p->m_Unk024) {
                pVictim = p;
                best = p->m_Unk024;
            }
        }
        if (!pVictim) return 1;
    } else {
        if (steal) {
            GetMyPos(1);
            best = CombineVolumes(100);
        }
        SFXItem *p = gpEXAudio->m_pSFXList;
        if (!p) {
            m_Remove = 1;
            return 0;
        }
        for (; p; p = p->m_pNext) {
            if (m_Hash != p->m_Hash || this == p || p->m_Remove) continue;
            int v;
            if (!steal) {
                v = p->m_Unk024;
                if (best >= v) continue;
            } else {
                p->GetMyPos(1);
                v = p->CombineVolumes(100);
                if (v >= best) continue;
            }
            pVictim = p;
            best = v;
        }
        if (!pVictim) {
            m_Remove = 1;
            return 0;
        }
        if (steal) {
            // Quieter than this one: the playing instance takes over this request instead.
            pVictim->m_Unk324.x = m_Unk324.x;
            pVictim->m_Unk324.y = m_Unk324.y;
            pVictim->m_Unk324.z = m_Unk324.z;
            pVictim->m_Pos.x = m_Pos.x;
            pVictim->m_Pos.y = m_Pos.y;
            pVictim->m_Pos.z = m_Pos.z;
            pVictim->m_VolumeOverRide = m_VolumeOverRide;
            pVictim->m_Unk358 = m_Unk358;
            pVictim->m_Unk354 = m_Unk354;
            pVictim->m_pObj = m_pObj;
            m_Remove = 1;
            return 1;
        }
    }
    pVictim->m_Remove = 1;
    return 1;
}

// Pick and start the first sample of a new SFX (one at random, all of them, or the first of a
// possibly shuffled sequence), set the cue timers and start the ducker. Returns 0 if culled.
int SFXItem::SFXSetup()
{
    SFXParameters *pParams = m_pParams;
    SFXParameters::Sample *pSamples = pParams->m_Samples;
    for (int i = 0; i < 20; i++) m_Voices[i].m_pVoice = 0;
    m_Remove = 0;
    m_Unk388 = 0;
    if (!m_Unk37c) {
        if (!ProcessCulling()) return 0;
    } else if (gpEXAudio->m_Debug[2])
        gpEXAudio->SFXprintf("Precache no cull\n");
    pParams = m_pParams;
    m_Unk16c = 0;
    if (!pParams->m_MultiSample) {
        int n = gpEXAudio->ES_Rnd(pParams->m_NumSamples);
        if (!gpEXAudio->ES_AudioStart((SFXSamplePoolFiles *)&pSamples[n], m_Bank, this, 0)) {
            m_Remove = 1;
            return 0;
        }
        m_Unk170[0] = gpEXAudio->ES_SetCueTimer(m_pParams);
    } else if (pParams->m_PlayAll) {
        for (int i = 0; i < m_pParams->m_NumSamples; i++) {
            m_Unk170[i] = gpEXAudio->ES_SetCueTimer(m_pParams);
            m_Unk238[i] = (int)pSamples++;
        }
    } else {
        m_Unk16c = pParams->m_NumSamples;
        m_Unk170[0] = 0;
        if (!pParams->m_Shuffle) {
            SFXParameters::Sample *p = &pSamples[m_Unk16c - 1];
            for (int i = 0; i < m_pParams->m_NumSamples; i++) m_Unk238[i] = (int)p--;
        } else {
            int used[50];
            for (int k = 0; k < 50; k++) used[k] = 0;
            int left = pParams->m_NumSamples;
            for (int i = 0; i < m_pParams->m_NumSamples; i++, left--) {
                int r = gpEXAudio->ES_Rnd(left);
                int idx = 0;
                do {
                    if (r > 0) idx++;
                    while (used[idx]) idx++;
                } while (r-- > 0);
                used[idx] = 1;
                m_Unk238[i] = (int)&pSamples[idx];
            }
        }
        gpEXAudio->ES_AudioStart((SFXSamplePoolFiles *)m_Unk238[--m_Unk16c], m_Bank, this, 0);
        m_Unk170[0] = gpEXAudio->ES_SetCueTimer(m_pParams);
    }
    pParams = m_pParams;
    if (pParams->m_DuckVolume && !m_Unk37c) gpEXAudio->ES_DuckerStart(100 - pParams->m_DuckVolume, pParams->m_DuckTime * 10, this);
    return 1;
}

static SFXPoint3 s_NoPosition = { -99999.0f, -99999.0f, -99999.0f };

static inline float ES_ClampCoord(float v)
{
    if (v < -1e9f) return -1e9f;
    if (v > 1e9f) return 1e9f;
    return v;
}

// Start (or with m_PreCache set, pre-cache) SFX hash at pPos. Returns 0 when started (or handed to
// a pre-cached stream), 1 if the SFX is not loaded or was culled, 2 if refused (faded down, once
// per frame, or the SFX list is full).
unsigned int EXAudioBase::SFXStart3D(int hash, SFXPoint3 *pPos, void *pObj, int volume, int innerRadius, int outerRadius,
                                     int pitch)
{
    int preCache = m_PreCache;
    if (m_Debug[14]) {
        pPos->x = m_ListenerPos[0].x;
        pPos->y = m_ListenerPos[0].y;
        pPos->z = m_ListenerPos[0].z;
    }
    hash &= 0xffffff;
    m_PreCache = 0;
    for (int bank = 0;; bank++) {
        if (bank > 104) {
            if (m_Debug[0]) gpEXAudio->SFXprintf("WARNING: Sound not Loaded. HC = 0x%6.6X\n", hash);
            return 1;
        }
        EXAudio_SoundBank &b = m_Banks[bank];
        if (!b.m_Loaded || b.m_Unk1c) continue;
        EXAudio_SoundBank::Entry *pEntry = b.m_pSFXTable;
        for (int i = 0; i < b.m_NumSFX; i++, pEntry++) {
            if (pEntry->m_Hash != hash) continue;
            SFXParameters *pParams = (SFXParameters *)(m_Banks[bank].m_pSFXData + pEntry->m_Offset);
            if (pParams->m_OncePerFrame) {
                for (int k = 0; k < m_NumFrameHashes; k++)
                    if (m_FrameHashes[k] == (u32)hash) return 2;
            }
            m_FrameHashes[m_NumFrameHashes] = hash;
            if (++m_NumFrameHashes > 59) m_NumFrameHashes = 59;
            if (!pParams->m_NoFade && gpEXAudio->m_SFXFadedDown) return 2;
            if (m_Debug[0] && pPos->x == -99999.0f && pPos->y == -99999.0f && pPos->z == -99999.0f &&
                (pParams->m_Type == 3 || pParams->m_Type == 1 || pParams->m_Type == 5))
                gpEXAudio->SFXprintf("WARNING: 3d sound started without position set. HC = 0x%6.6X\n", hash);
            if (!preCache) {
                if (pParams->m_Samples[0].m_Sample < 0) {
                    // A stream: use the pre-cached instance if there is one.
                    for (SFXItem *p = m_pSFXList; p; p = p->m_pNext) {
                        if (p->m_Hash != hash || !p->m_Unk37c) continue;
                        if (!p->m_Voices[0].m_pVoice) {
                            p->m_Unk37c = 0;
                            p->m_Remove = 1;
                            continue;
                        }
                        p->m_PitchOverRide = pitch;
                        p->m_Unk358 = innerRadius;
                        p->m_Unk354 = outerRadius;
                        if (volume == -1) volume = 100;
                        else if (volume < 0) volume = 0;
                        else if (volume > 100) volume = 100;
                        p->m_VolumeOverRide = volume;
                        float x = ES_ClampCoord(pPos->x);
                        float y = ES_ClampCoord(pPos->y);
                        float z = ES_ClampCoord(pPos->z);
                        p->m_Unk324.x = x;
                        p->m_Unk324.y = y;
                        p->m_Unk324.z = z;
                        p->m_Pos.x = x;
                        p->m_Pos.y = y;
                        p->m_Pos.z = z;
                        if (p->ProcessCulling()) {
                            if (m_Debug[2])
                                gpEXAudio->SFXprintf("<GOOD> Started Pre-Cache Stream %8.8X Counter = %d/%d\n",
                                                     hash | 0x1af00000, p->m_Unk37c, p->m_Unk380);
                            p->m_Unk37c = 0;
                            p->KeyOnPreCachedSFX();
                            p->m_pObj = pObj;
                            return 0;
                        }
                        if (m_Debug[2]) gpEXAudio->SFXprintf("<BAD> Pre-Cached Stream Culled. HC %8.8X", hash | 0x1af00000);
                        p->m_Remove = 1;
                        return 0;
                    }
                    if (m_Debug[2]) gpEXAudio->SFXprintf("<BAD> Started NON - Pre-Cache Stream %8.8X\n", hash | 0x1af00000);
                }
            } else {
                // Only streams are pre-cached; refresh an existing pre-cache.
                if (pParams->m_Samples[0].m_Sample >= 0) return 0;
                for (SFXItem *p = m_pSFXList; p; p = p->m_pNext) {
                    if (p->m_Hash == hash && p->m_Unk37c) {
                        p->m_Unk37c = preCache;
                        p->m_Unk380 = preCache;
                        return 0;
                    }
                }
            }
            SFXItem *pItem = m_pFreeList;
            if (!pItem) {
                gpEXAudio->SFXprintf("SFX list full\n");
                return 2;
            }
            if (m_pSFXList) m_pSFXList->m_pPrev = pItem;
            m_pFreeList = pItem->m_pNext;
            pItem->ClearVariables();
            pItem->m_pPrev = 0;
            pItem->m_pNext = m_pSFXList;
            m_pSFXList = pItem;
            pItem->m_Unk38c = 100;
            pItem->m_Unk364 = m_Unk1c8;
            pItem->m_Unk360 = m_Unk1d0;
            pItem->m_Unk36c = 100;
            pItem->m_Unk368 = 0;
            pItem->m_pObj = pObj;
            if (!pPos) {
                pItem->m_Unk324.x = s_NoPosition.x;
                pItem->m_Unk324.y = s_NoPosition.y;
                pItem->m_Unk324.z = s_NoPosition.z;
            } else {
                pItem->m_Unk324.x = ES_ClampCoord(pPos->x);
                pItem->m_Unk324.y = ES_ClampCoord(pPos->y);
                pItem->m_Unk324.z = ES_ClampCoord(pPos->z);
            }
            pItem->m_Pos.x = pItem->m_Unk324.x;
            pItem->m_Pos.y = pItem->m_Unk324.y;
            pItem->m_Pos.z = pItem->m_Unk324.z;
            if (volume == -1) pItem->m_VolumeOverRide = 100;
            else {
                if (volume < 0) volume = 0;
                else if (volume > 100) volume = 100;
                pItem->m_VolumeOverRide = volume;
            }
            pItem->m_PitchOverRide = pitch;
            pItem->m_Unk358 = innerRadius;
            pItem->m_Unk354 = outerRadius;
            if (m_Debug[0] && outerRadius < 1 && (pParams->m_Type == 3 || pParams->m_Type == 1 || pParams->m_Type == 5))
                gpEXAudio->SFXprintf("WARNING: 3d sound started with OuterRadius<=0. HC = 0x%6.6X\n", hash);
            pItem->m_Hash = hash;
            pItem->m_pParams = pParams;
            pItem->m_Bank = bank;
            pItem->m_Unk024 = 0;
            pItem->m_Paused = 0;
            pItem->m_Unk37c = preCache;
            pItem->m_Unk380 = preCache;
            pItem->m_pParent = 0;
            if (gpEXAudio->m_Debug[6]) pParams->m_Type = 0;
            if (!pItem->SFXSetup()) return 1;
            m_Unk894 = (int)pItem;
            return 0;
        }
    }
}

static unsigned int s_WarnTime;
static unsigned int s_LastWarnTime;
static int s_ActiveDumpCount = 100;

// Per-frame update of the EuroSound core: debug overrides, the timer, environment / ducker / fade /
// music, stream and sound bank loading, every SFX, then the removals and the debug snapshots.
void EXAudioBase::SFXUpdate()
{
    m_NumFrameHashes = 0;
    if (m_Debug[8]) {
        m_MusicVolumeReq = 0;
        ES_MusicSetVolume();
    }
    if (m_Debug[9]) {
        m_MusicVolumeReq = 100;
        ES_MusicSetVolume();
    }
    if (m_Debug[10]) m_SFXVolumeReq = 0;
    if (m_Debug[11]) m_SFXVolumeReq = 100;
    if (m_Debug[12]) {
        m_Mode = 2;
        psiSFXSetMode(2);
    }
    if (m_Unk1c0) m_Ticks = psiSFXGetTimer() - m_Unk1c0;
    m_Unk1c0 = psiSFXGetTimer();
    if (m_Ticks < 0) m_Ticks = 16;
    if (m_Ticks > 1000) m_Ticks = 1000;
    s_WarnTime = psiSFXGetTimer() / 2000;
    if (s_WarnTime != s_LastWarnTime && m_Debug[0]) {
        if (m_SFXFader.m_Volume == 0) gpEXAudio->SFXprintf("WARNING: SFXFader.Volume==0 Paused = %d\n", 1 - m_SFXFader.m_Stop);
        if (m_MusicFader.m_Volume == 0)
            gpEXAudio->SFXprintf("WARNING: MusicFader.Volume==0 Paused = %d\n", 1 - m_MusicFader.m_Stop);
        if (m_Unk84c[0] == -999999.0f && m_Unk84c[1] == -888888.0f && m_Unk84c[2] == -777777.0f)
            gpEXAudio->SFXprintf("ERROR: Listener Pos NOT set.\n");
    }
    s_LastWarnTime = s_WarnTime;
    if (m_Unk8e4) {
        m_Unk8e0 = psiSFXGetTimer();
        m_Unk8e4 = 0;
        m_Unk8ec = 0;
        m_Unk8f0 = 0;
    }
    m_Unk8e8 = psiSFXGetTimer() - m_Unk8e0;
    int now = psiSFXGetTimer();
    m_Unk8ec += m_Unk8e8;
    m_Unk8f0++;
    m_Unk8e0 = now;
    psiPreSFXUpdate();
    SFXInitialise2Update();
    SFXItem *p = m_pSFXList;
    ES_UpdateEnvironment();
    ES_DuckerUpdate();
    ES_FadeUpdate();
    ES_TryPlayMusic();
    for (; p; p = p->m_pNext) {
        if (m_Unk1cc && p->m_Unk364) p->m_Unk368 = 1;
        p->m_Unk308 = 0;
    }
    ES_StreamUpdate();
    ES_LoadSoundBankUpdate();
    ES_SfxUpdateAll();
    int dump = 0;
    if (m_Debug[15] && s_ActiveDumpCount-- % 100 == 0) {
        dump = 1;
        printf("---> Active SFXs Code , Counter\n");
    }
    for (p = m_pSFXList; p;) {
        SFXItem *pNext = p->m_pNext;
        if (p->m_Remove) p->Remove();
        if (dump) printf("HashCode %6.6X  Count %d\n", p->m_Hash, p->m_Unk024);
        p = pNext;
    }
    m_NumActive = 0;
    for (p = m_pSFXList; p; p = p->m_pNext) {
        m_Active[m_NumActive].m_pObj = p->m_pObj;
        m_Active[m_NumActive].m_Hash = p->m_Hash;
        m_NumActive++;
    }
    int *pOut = m_LoadedBanks;
    for (int i = 0; i < 10; i++) m_LoadedBanks[i] = 0;
    for (int b = 0; b < 105; b++) {
        if (m_Banks[b].m_Loaded) *pOut++ = m_Banks[b].m_Hash;   // retail does not bound this
    }
    if (!m_SFXFader.m_State) m_SFXFaderState = m_SFXFader.m_Volume ? 2 : 0;
    else m_SFXFaderState = 1;
    if (!m_MusicFader.m_State) m_MusicFaderState = m_MusicFader.m_Volume ? 2 : 0;
    else m_MusicFaderState = 1;
    for (int i = 0; i < 105; i++) {
        if (m_Banks[i].m_Status == 2) ES_UnLoadSoundBankReleaseFinished(i);
    }
    if (!m_Unk1e0 && !m_Unk1e4) m_Busy = 0;
    else m_Busy = 1;
    psiPostSFXUpdate();
}

// Header of the project / stream initialisation files (0x38 bytes); the version words are read a byte
// at a time.
struct ES_InitFileHeader {
    u8 m_00[8];
    u8 m_Version[4];        // 0x08 EuroSound data version, -1 debug
    u8 m_0c[0xc];
    u8 m_CodecVersion[4];   // 0x18
    u8 m_1c[4];
    int m_DataOffset;       // 0x20
    int m_DataSize;         // 0x24 4: old project file without the memory slot table
    int m_AudioOffset;      // 0x28 stream file: start of the audio data
    u8 m_2c[0xc];
};

static inline int ES_ReadInt(const u8 *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24); }

// SFXInitialise2 state machine: read the project file header and its memory slot table, size the
// audio memory, then read the stream initialisation file and its lookup table.
void EXAudioBase::SFXInitialise2Update()
{
    switch (m_InitState) {
    case 0:
        return;
    case 1:
        m_Unk00c = psiAsyncOpenFile(&m_ProjectFile, FILEMAN_AUDIO, 0xffffffff);
        if (m_Unk00c == -1) return;
        m_Unk11850 = (int)psiAsyncCreateBuffer(sizeof(ES_InitFileHeader), 1);
        psiAsyncReadFile(m_Unk00c, 0, sizeof(ES_InitFileHeader), (char *)m_Unk11850, 0x2000000);
        m_InitState++;
        return;
    case 2: {
        if (!psiAsyncHasReadFinished(m_Unk00c)) return;
        ES_InitFileHeader *pHdr = (ES_InitFileHeader *)m_Unk11850;
        int version = ES_ReadInt(pHdr->m_Version);
        int codec = ES_ReadInt(pHdr->m_CodecVersion);
        if (version == 5 || version == -1)
            m_VersionOK = 1;
        else {
            if (version > 5) gpEXAudio->SFXprintf("EE: Audio Disabled. EuroSound Code out of date.\n");
            else gpEXAudio->SFXprintf("EE: Audio Disabled. EuroSound Data out of date.\n");
            gpEXAudio->SFXprintf("EE: Code Version %d Data Version %d\n", 5, version);
        }
        if (m_VersionOK) {
            if (codec != 1 && version != -1) {
                if (codec > 1) gpEXAudio->SFXprintf("EE: Audio Disabled. EuroSound Code Platform CODEC out of date.\n");
                else gpEXAudio->SFXprintf("EE: Audio Disabled. EuroSound Data Platform CODEC out of date.\n");
                gpEXAudio->SFXprintf("EE: Code CODEC Version %d Data CODEC Version %d\n", 1, codec);
                m_VersionOK = 0;
            }
            if (version == -1) gpEXAudio->SFXprintf("EW: Data Version is Debug (-1)\n");
            if (m_VersionOK) gpEXAudio->SFXprintf("EI: EuroSound Version %d CODEC %d.\n", 5, 1);
        }
        if (pHdr->m_DataSize == 4) {
            gpEXAudio->SFXprintf("Old Project file. Slot system disabled.\n");
            m_Unk004 = 0;
            m_Unk010 = 0;
        } else {
            m_Unk004 = (u32)psiAsyncCreateBuffer(pHdr->m_DataSize, 0);
            psiAsyncReadFile(m_Unk00c, ((ES_InitFileHeader *)m_Unk11850)->m_DataOffset,
                             ((ES_InitFileHeader *)m_Unk11850)->m_DataSize, (char *)m_Unk004, 0x2000000);
        }
        psiAsyncDeleteBuffer((char *)m_Unk11850);
        m_InitState++;
        return;
    }
    case 3: {
        if (!m_VersionOK) {
            m_Unk008 = 1;
            psiAsyncCloseFile(m_Unk00c);
            m_InitState = 0;
            return;
        }
        if (!psiAsyncHasReadFinished(m_Unk00c)) return;
        ES_MemorySlotTable *pTable = (ES_MemorySlotTable *)m_Unk004;
        if (pTable) {
            // Relocate the table's file offsets.
            *(int *)&pTable->m_pSlots += (int)pTable;
            *(int *)&((ES_MemorySlotTable *)m_Unk004)->m_pBanks += m_Unk004;
            pTable = (ES_MemorySlotTable *)m_Unk004;
            m_Unk010 = pTable->m_nSlots != 0;
        }
        if (m_Unk010) {
            int total = 0;
            PjtMemorySlots *pSlot = pTable->m_pSlots;
            for (int i = 0; i < ((ES_MemorySlotTable *)m_Unk004)->m_nSlots; i++, pSlot++) total += pSlot->m_Type * pSlot->m_Unk4;
            psiInitialiseAudioMemory(total << 10);
        } else
            psiInitialiseAudioMemory(0);
        m_Unk008 = 1;
        psiAsyncCloseFile(m_Unk00c);
        m_InitState++;
        return;
    }
    case 4:
        if (!m_StreamFile.m_Size) {
            gpEXAudio->SFXprintf("============================================\n");
            gpEXAudio->SFXprintf("No Stream Initialization file found.\nAny music / sfx related calls likely to cause problems !!!\n");
            gpEXAudio->SFXprintf("============================================\n");
            m_InitState = 0;
            return;
        }
        for (int i = 0; i < 105; i++) m_Banks[i].m_Unk44 = 1;
        StreamsInitialised = 0;
        m_Unk1185c = 0;
        m_InitState++;
        return;
    case 5:
        m_Unk00c = psiAsyncOpenFile(&m_StreamFile, FILEMAN_AUDIO, 0xffffffff);
        if (m_Unk00c == -1) return;
        m_Unk1185c = (int)psiAsyncCreateBuffer(sizeof(ES_InitFileHeader), 1);
        psiAsyncReadFile(m_Unk00c, 0, sizeof(ES_InitFileHeader), (char *)m_Unk1185c, 0x2000000);
        m_InitState++;
        return;
    case 6: {
        if (!psiAsyncHasReadFinished(m_Unk00c)) return;
        psiAsyncCloseFile(m_Unk00c);
        if (!m_Unk1185c) {
            m_InitState = 0;
            return;
        }
        ES_InitFileHeader *pHdr = (ES_InitFileHeader *)m_Unk1185c;
        StreamOffsetToAudioData = pHdr->m_AudioOffset;
        unsigned int count = (unsigned int)pHdr->m_DataSize >> 2;
        if (m_Debug[3]) gpEXAudio->SFXprintf("Mono streams in file = %d\n", count);
        StreamLookupFileData = (StreamLookupFileDetails *)psiAsyncCreateBuffer(count * 4, 0);
        m_Unk00c = psiAsyncOpenFile(&m_StreamFile, FILEMAN_AUDIO, 0xffffffff);
        psiAsyncReadFile(m_Unk00c, ((ES_InitFileHeader *)m_Unk1185c)->m_DataOffset, count * 4, (char *)StreamLookupFileData,
                         0x2000000);
        m_InitState++;
        return;
    }
    case 7:
        if (!psiAsyncHasReadFinished(m_Unk00c)) return;
        psiAsyncCloseFile(m_Unk00c);
        psiAsyncDeleteBuffer((char *)m_Unk1185c);
        m_InitState = 0;
        StreamsInitialised = 1;
        return;
    }
}

// Sound bank file header (first 0x40 bytes of the bank file).
struct ES_SoundBankHeader {
    u8 m_00[0x20];
    int m_SFXOffset;        // 0x20 SFX parameter table
    int m_SFXSize;          // 0x24
    int m_SampleOffset;     // 0x28 sample headers
    int m_SampleSize;       // 0x2c
    u8 m_30[8];
    int m_DataOffset;       // 0x38 sample data, streamed into audio memory
    int m_DataSize;         // 0x3c
};

static int s_LBFailedHash = -1;     // last bank that failed to load
static int s_LBRetry;
static ES_SoundBankHeader *s_pLBHeader;
static int s_LBFile;
static XFileHandle s_LBDataFile;
static char *s_pLBSampleMem;
static char *s_pLBSampleBase;

// Sound bank load job state machine: 0 claim a slot, 1 read the header, 2 read the SFX table,
// 3 read the sample headers, 4 start the sample data load, 6 fix up the samples, 7 finish.
void EXAudioBase::ES_LoadSoundBankUpdate()
{
    if (s_LBFailedHash != -1 && --s_LBRetry < 0) s_LBRetry = 100;
    if (m_LBWrite == m_LBRead) return;
    ES_LoadSoundBankDetails *pJob = &m_LB[m_LBRead];
    int bank = ES_FindSoundBankSlot(pJob->m_Hash);
    bool failed = false;
    if (m_LBState == 0) {
        if (!m_Unk008) return;
        if (!m_VersionOK) {
            gpEXAudio->SFXprintf("EE: LoadSoundBank cancelled: Audio data / Code Version mis-match.\n");
            goto next;
        }
        if (bank == -1) {
            gpEXAudio->SFXprintf("SFX Failed in FILE %s, LINE %d\n", "\\game\\EngineX\\EXAudioBase.cpp", 0x249);
            ES_LB_GetNext();
            return;
        }
        if (m_LBBlocked) return;
        if (!m_Debug[7] && m_Banks[bank].m_Loaded && !m_Banks[bank].m_Unk44) {
            ES_LB_GetNext();
            return;
        }
        m_Banks[bank].m_Unk44 = 0;
        PjtMemorySlots *pSlot = SoundBankGetMemorySlot(pJob->m_Hash);
        int slot = pSlot->m_Unk0;
        if (pSlot->m_Type == 1) {
            // A resident slot: force out whatever bank holds it now.
            for (int i = 0; i < 105; i++) {
                EXAudio_SoundBank &b = m_Banks[i];
                if (b.m_Loaded && !b.m_Unk1c) {
                    int hash = b.m_Hash;
                    if (SoundBankGetMemorySlot(hash)->m_Unk0 == slot) SFXUnLoadSoundBankForced(hash);
                }
            }
        }
        m_LBState = 1;
    }
    if (m_LBState == 1) {
        if (m_LBBlocked) return;
        psiAsyncLockFilePriority(0x3000000);
        s_pLBHeader = (ES_SoundBankHeader *)ES_LB_StartFileLoad(&pJob->m_File, &s_LBFile, 0, 0x40, 1);
        if (!s_pLBHeader) {
            ES_LB_GetNext();
            psiAsyncUnLockFilePriority();
            return;
        }
        m_Banks[bank].m_Unk3c = 0;
        m_Banks[bank].m_Status = 0;
        m_LBState = 2;
    }
    if (m_LBState == 2 && psiAsyncHasReadFinished(s_LBFile)) {
        psiAsyncCloseFile(s_LBFile);
        ES_SoundBankHeader *pHdr = s_pLBHeader;
        s_LBDataFile = pJob->m_File;
        if (pHdr && s_LBFile != -1) {
            s_LBDataFile.m_ReadOffset = pHdr->m_DataOffset;
            s_LBDataFile.m_Size = pHdr->m_DataSize;
            m_Banks[bank].m_pSFXData = ES_LB_StartFileLoad(&pJob->m_File, &s_LBFile, (char *)pHdr->m_SFXOffset, pHdr->m_SFXSize, 0);
            if (!m_Banks[bank].m_pSFXData) {
                psiAsyncDeleteBuffer((char *)s_pLBHeader);
                ES_LB_GetNext();
                psiAsyncUnLockFilePriority();
                return;
            }
            m_LBState = 3;
        }
    }
    if (m_LBState == 3 && psiAsyncHasReadFinished(s_LBFile)) {
        psiAsyncCloseFile(s_LBFile);
        int *pTable = (int *)m_Banks[bank].m_pSFXData;
        m_Banks[bank].m_pSFXTable = (EXAudio_SoundBank::Entry *)(pTable + 1);
        m_Banks[bank].m_NumSFX = pTable[0];
        m_Banks[bank].m_Unk00 = (int)ES_LB_StartFileLoad(&pJob->m_File, &s_LBFile, (char *)s_pLBHeader->m_SampleOffset,
                                                         s_pLBHeader->m_SampleSize, 0);
        if (!m_Banks[bank].m_Unk00) {
            psiAsyncDeleteBuffer((char *)s_pLBHeader);
            psiAsyncDeleteBuffer(m_Banks[bank].m_pSFXData);
            ES_LB_GetNext();
            psiAsyncUnLockFilePriority();
            return;
        }
        m_LBState = 4;
    }
    if (m_LBState == 4 && psiAsyncHasReadFinished(s_LBFile)) {
        psiAsyncCloseFile(s_LBFile);
        int *pSamples = (int *)m_Banks[bank].m_Unk00;
        m_Banks[bank].m_Unk04 = pSamples[0];
        m_Banks[bank].m_Unk08 = (int)(pSamples + 1);
        if (!m_Banks[bank].m_Unk04) {
            gpEXAudio->SFXprintf("SampleCount==0\n");
            m_LBState = 7;
        } else {
            m_Banks[bank].m_Unk24 = 0;
            s_pLBSampleMem = (char *)psiLoadSoundBank(&s_LBDataFile, SoundBankGetMemorySlot(pJob->m_Hash)->m_Unk4 << 10);
            m_LBState = 6;
        }
    }
    if (m_LBState == 6) {
        failed = s_pLBSampleMem == 0;
        if (failed) {
            ES_UnLoadSoundBankRelease(bank);
            m_LBState = 7;
        }
        if (m_LBState == 6 && psiHasLoadSoundBankFinished()) {
            psiAsyncUnLockFilePriority();
            SampleHeaderData *pSample = (SampleHeaderData *)m_Banks[bank].m_Unk08;
            m_Banks[bank].m_Unk28 = (int)s_pLBSampleMem;
            s_pLBSampleBase = s_pLBSampleMem;
            for (int i = 0; i < m_Banks[bank].m_Unk04; i++, pSample++) {
                pSample->m_Handle = (int)s_pLBSampleMem;
                if ((u32)pSample->m_Length <= (u32)pSample->m_LoopStart) {
                    gpEXAudio->SFXprintf("Illegal loop point for sound 0x%x.\n", ES_GetSFXHashcode(bank, i));
                    pSample->m_LoopStart = 0;
                }
                psiLoadSampleInitialise(pSample);
                pSample->m_Unk14 += m_Banks[bank].m_Unk24;
                s_pLBSampleMem += pSample->m_Unk08;
            }
            if (m_Debug[3])
                gpEXAudio->SFXprintf("SoundBank 0x%x Loaded %d samples into Bank Slot %d\n", pJob->m_Hash, m_Banks[bank].m_Unk04, bank);
            m_LBState = 7;
        }
    }
    if (m_LBState != 7) return;
    psiAsyncUnLockFilePriority();
    m_Banks[bank].m_Unk3c = 7;
    m_Banks[bank].m_Hash = pJob->m_Hash;
    if (failed) {
        gpEXAudio->SFXprintf("EE: SoundBank 0x%X Failed to Load\n", pJob->m_Hash);
        s_LBFailedHash = pJob->m_Hash;
    } else {
        m_Banks[bank].m_Loaded = 1;
        m_Banks[bank].m_Fader.FaderClear();
        m_Banks[bank].m_Fader.m_Bank = 1;
        m_Banks[bank].m_Fader.m_State = 1;
        m_Banks[bank].m_Fader.m_Speed = 10000;
        if (m_TestMode == 1) {
            m_Banks[bank].m_Fader.m_State = 0;
            m_Banks[bank].m_Fader.m_Volume = 10000000;
        }
        if (s_LBFailedHash == pJob->m_Hash) s_LBFailedHash = -1;
    }
    psiAsyncDeleteBuffer((char *)s_pLBHeader);
next:
    m_LBState = 0;
    m_LBRead = (m_LBRead + 1) % 10;
}

// Start one sample of an SFX in voice slot `voice`: a sub-SFX (its own SFX started and linked as
// the voice), or a sample / stream voice with randomised pitch, volume and pan, the multi-listener
// distance volume, then initialise the platform voice and key it on unless pre-caching.
int EXAudioBase::ES_AudioStart(SFXSamplePoolFiles *pSample, int bank, SFXItem *pItem, int voice)
{
    SampleHeaderData *pSamples = (SampleHeaderData *)m_Banks[bank].m_Unk08;
    if (pItem->m_pParams->m_Unk1a) {
        m_Unk898 = 1;
        int r = SFXStart3D(pSample->m_Sample | 0xf00000, &pItem->m_Unk324, 0, pItem->m_VolumeOverRide, pItem->m_Unk358,
                           pItem->m_Unk354, pItem->m_PitchOverRide);
        m_Unk898 = 0;
        if (r == 0) {
            SFXItem *pSub = (SFXItem *)m_Unk894;
            pSub->m_pParent = pItem;
            int key = -m_Unk894;
            pSub->m_pObj = (const void *)key;
            EXAudio_Voice *pVoice = m_VoiceManager.ES_FindFreeVoiceHandle();
            if (pVoice) {
                pVoice->m_SubSFX = 1;
                pVoice->m_Handle = key;
                pItem->m_Voices[voice].m_pVoice = pVoice;
                pItem->m_Unk01c = ((SFXItem *)m_Unk894)->m_Unk01c;
                ((SFXItem *)m_Unk894)->m_Pausable = pItem->m_Pausable;
                return 1;
            }
            ((SFXItem *)m_Unk894)->m_Remove = 1;
        }
        pItem->m_Voices[voice].m_pVoice = 0;
        return 0;
    }
    u8 type = pItem->m_pParams->m_Type;
    int stereo = (type == 2 || type == 3) ? 1 : 0;
    if (pSample->m_Sample < 0 && m_Debug[2]) {
        if (ES_MonoStreamActiveCount() == 4)
            gpEXAudio->SFXprintf(pItem->m_Unk37c ? ">>> Warning: 5th Stream Allocated via PreCache %X\n"
                                                 : ">>> Warning: 5th Stream Allocated for Play %X\n",
                                 pItem->m_Hash);
        if (ES_MonoStreamActiveCount() == 5)
            gpEXAudio->SFXprintf(pItem->m_Unk37c ? ">>> Warning: Stream FAILED to Allocate via PreCache . No Streams Free %X\n"
                                                 : ">>> Warning: Stream FAILED to Allocate for Play . No Streams Free %X\n",
                                 pItem->m_Hash);
    }
    SFXItem::VoiceSlot &slot = pItem->m_Voices[voice];
    slot.m_pVoice = m_VoiceManager.ES_RequestVoiceHandle(pSamples, pSample->m_Sample, type & 1, 0, stereo,
                                                         (signed char)pItem->m_pParams->m_Type, pItem->m_Hash);
    if (!slot.m_pVoice) {
        if (m_Debug[2] && pSample->m_Sample < 0) {
            if (!pItem->m_Unk37c) {
                gpEXAudio->SFXprintf("<VBAD> Failed to start NON PreCache stream %8.8X\n", pItem->m_Hash | 0x1af00000);
                return 0;
            }
            gpEXAudio->SFXprintf("<BAD> Failed to PreCache stream %8.8X\n", pItem->m_Hash | 0x1af00000);
        }
        return 0;
    }
    int rate;
    int length = 1000;
    if (pSample->m_Sample < 0) {
        rate = 22050;
        pItem->m_Unk388 = 1;
    } else {
        rate = pSamples[pSample->m_Sample].m_SampleRate;
        pItem->m_Unk388 = 0;
        length = pSamples[pSample->m_Sample].m_Unk1c;
    }
    int pitch = pSample->m_Pitch * 2 + ((pSample->m_PitchRand * ES_Rndsf()) >> 10);
    int freq = ES_PitchScale(pitch) * rate / 1000;
    pItem->m_Unk01c = rate * length / freq;
    int volume = ((pSample->m_VolumeRand * ES_Rndsf()) >> 10) + pSample->m_Volume;
    if (volume < 0) volume = 0;
    else if (volume > 100) volume = 100;
    int pan = ((pSample->m_PanRand * ES_Rndsf()) >> 10) + pSample->m_Pan;
    if (pan < -100) pan = -100;
    else if (pan > 100) pan = 100;
    slot.m_Volume = volume;
    slot.m_Freq = freq;
    slot.m_Pan = pan;
    SFXParameters *pParams = pItem->m_pParams;
    if (pParams->m_Type == 5 || pParams->m_Type == 7) {
        pItem->m_Unk330.x = (float)(int)(ES_Rnd(20) - 10);
        pItem->m_Unk330.y = (float)(int)(ES_Rnd(20) - 10);
        pItem->m_Unk330.z = (float)(int)(ES_Rnd(20) - 10);
    }
    pItem->m_Unk324.x = pItem->m_Pos.x;
    pItem->m_Unk324.y = pItem->m_Pos.y;
    pItem->m_Unk324.z = pItem->m_Pos.z;
    int listeners = gpEXAudio->m_NumListeners;
    if (listeners >= 2 || pParams->m_Type == 3 || pParams->m_Type == 5 || pParams->m_Type == 7) {
        // Listener relative: the volume comes from the distance to the nearest listener.
        float best = 0.0f;
        for (int i = 0; i < listeners; i++) {
            SFXPoint3 &l = gpEXAudio->m_ListenerPos[i];
            float dx = (l.x - pItem->m_Pos.x) * 0.01f;
            float dy = (l.y - pItem->m_Pos.y) * 0.01f;
            float dz = (l.z - pItem->m_Pos.z) * 0.01f;
            float d = dz * dz + dy * dy + dx * dx;
            if (i == 0 || d < best) best = d;
        }
        pItem->m_Unk324.x = gpEXAudio->m_Unk84c[0];
        pItem->m_Unk324.y = gpEXAudio->m_Unk84c[1];
        pItem->m_Unk324.z = gpEXAudio->m_Unk84c[2];
        if (pParams->m_Type == 5 || pParams->m_Type == 7) {
            pItem->m_Unk324.x = pItem->m_Unk330.x + pItem->m_Unk324.x;
            pItem->m_Unk324.y = pItem->m_Unk330.y + pItem->m_Unk324.y;
            pItem->m_Unk324.z = pItem->m_Unk330.z + pItem->m_Unk324.z;
        }
        pItem->m_Unk38c = 100;
        if (pParams->m_Type != 7) {
            float dist = gpEXAudio->psiSquareRoot(best) * 100.0f;
            float inner = (float)pItem->m_Unk358;
            if ((float)pItem->m_Unk354 < dist)
                pItem->m_Unk38c = 0;
            else if (inner <= dist) {
                float span = (float)pItem->m_Unk354 - inner;
                if (span >= 1.0f) {
                    pItem->m_Unk38c = (int)(((span - (dist - inner)) * 100.0f) / span);
                    if (pItem->m_Unk38c < 0) pItem->m_Unk38c = 0;
                    if (pItem->m_Unk38c > 100) pItem->m_Unk38c = 100;
                }
            }
        }
    }
    psiSFX sfx;
    ES_FillpsiSFXData(&sfx, pItem, voice);
    psiInitialiseSound(&sfx);
    if (!pItem->m_Unk37c && slot.m_pVoice) slot.m_pVoice->ES_AudioKeyOn(pItem->m_pParams->m_KeyOnArg);
    return 1;
}

// Listener relative SFX (several listeners, or types 3 / 5 / 7): position at the listener origin (plus
// the random offset for types 5 / 7) and set the distance volume from the nearest listener.
static __forceinline void ES_UpdateListenerVolume(SFXItem *p)
{
    int listeners = gpEXAudio->m_NumListeners;
    if (listeners < 2 && p->m_pParams->m_Type != 3 && p->m_pParams->m_Type != 5 && p->m_pParams->m_Type != 7) return;
    float best = 0.0f;
    for (int i = 0; i < listeners; i++) {
        SFXPoint3 &l = gpEXAudio->m_ListenerPos[i];
        float dy = (l.y - p->m_Pos.y) * 0.01f;
        float dz = (l.z - p->m_Pos.z) * 0.01f;
        float dx = (l.x - p->m_Pos.x) * 0.01f;
        float d = dz * dz + dy * dy + dx * dx;
        if (i == 0 || d < best) best = d;
    }
    p->m_Unk324.x = gpEXAudio->m_Unk84c[0];
    p->m_Unk324.y = gpEXAudio->m_Unk84c[1];
    p->m_Unk324.z = gpEXAudio->m_Unk84c[2];
    SFXParameters *pParams = p->m_pParams;
    if (pParams->m_Type == 5 || pParams->m_Type == 7) {
        p->m_Unk324.x = p->m_Unk330.x + p->m_Unk324.x;
        p->m_Unk324.y = p->m_Unk330.y + p->m_Unk324.y;
        p->m_Unk324.z = p->m_Unk330.z + p->m_Unk324.z;
    }
    if (pParams->m_Type != 7) {
        float dist = gpEXAudio->psiSquareRoot(best) * 100.0f;
        float inner = (float)p->m_Unk358;
        if (dist > (float)p->m_Unk354) {
            p->m_Unk38c = 0;
            return;
        }
        float span = (float)p->m_Unk354 - inner;
        if (dist >= inner && span >= 1.0f) {
            p->m_Unk38c = (int)(((span - (dist - inner)) * 100.0f) / span);
            if (p->m_Unk38c < 0) p->m_Unk38c = 0;
            if (p->m_Unk38c > 100) p->m_Unk38c = 100;
            return;
        }
    }
    p->m_Unk38c = 100;
}

// Update every SFX: follow the parent of sub-SFX, run the timers, pre-cache expiry, the sample
// sequences and the ending, free SFX whose voices have all keyed off, then the position, velocity
// and platform parameters of each voice.
void EXAudioBase::ES_SfxUpdateAll()
{
    SFXItem *pNext = m_pSFXList;
    while (pNext) {
        SFXItem *p = pNext;
        pNext = p->m_pNext;
        SFXItem *pParent = p->GetRealParent();
        if (pParent) {
            p->m_Unk324.x = pParent->m_Unk324.x;
            p->m_Unk324.y = pParent->m_Unk324.y;
            p->m_Unk324.z = pParent->m_Unk324.z;
            p->m_Unk368 = pParent->m_Unk368;
            p->m_Pos.x = pParent->m_Pos.x;
            p->m_Pos.y = pParent->m_Pos.y;
            p->m_Pos.z = pParent->m_Pos.z;
            p->m_Unk358 = pParent->m_Unk358;
            p->m_Unk354 = pParent->m_Unk354;
            p->m_VolumeOverRide = pParent->m_VolumeOverRide;
            p->m_PitchOverRide = pParent->m_PitchOverRide;
        }
        if (!p->m_Paused) {
            p->m_Unk01c -= m_Unk8e8;
            if (p->m_Unk368) {
                p->m_Unk36c -= 5;
                if (p->m_Unk36c < 0) {
                    p->m_Unk36c = 0;
                    p->m_Remove = 1;
                }
            }
            p->m_Unk308++;
            p->m_Unk024++;
            if (p->m_Unk37c && !m_Unk1d0 && --p->m_Unk37c == 0) {
                if (m_Debug[2])
                    gpEXAudio->SFXprintf("<Hmmm> DELETED - Pre-Cache Stream %8.8X InitCount %d\n", p->m_Hash | 0x1af00000, p->m_Unk380);
                p->m_Remove = 1;
            }
            if (!p->m_Unk398 && !p->m_Remove) {
                SFXParameters *pParams = p->m_pParams;
                bool ending = false;
                if (!pParams->m_MultiSample) {
                    if (p->m_Voices[0].m_pVoice->AudioHasEnded(p)) {
                        p->m_Voices[0].m_pVoice->ES_UnLockVoiceHandle();
                        p->m_Voices[0].m_pVoice = 0;
                        ending = true;
                    }
                } else if (!pParams->m_PlayAll) {
                    // Sequence: the next sample starts when this one ends or its cue time passes.
                    if (p->m_Voices[0].m_pVoice->AudioHasEnded(p) ||
                        (p->m_Unk01c + p->m_Unk170[0] < 0 && p->m_Unk170[0] < 0 && !p->m_pParams->m_Unk1a)) {
                        p->m_Voices[0].m_pVoice->ES_UnLockVoiceHandle();
                        int left = p->m_Unk16c;
                        p->m_Voices[0].m_pVoice = 0;
                        if (!left)
                            ending = true;
                        else if ((p->m_Unk170[0] -= m_Unk8e8) < 1) {
                            p->m_Unk16c = left - 1;
                            ES_AudioStart((SFXSamplePoolFiles *)p->m_Unk238[left - 1], p->m_Bank, p, 0);
                            p->m_Unk170[0] = ES_SetCueTimer(p->m_pParams);
                        }
                    }
                } else {
                    // All samples, each on its own cue time.
                    bool done = true;
                    for (int i = 0; i < p->m_pParams->m_NumSamples; i++) {
                        if ((p->m_Unk170[i] -= m_Unk8e8) >= 1) {
                            done = false;
                            continue;
                        }
                        if (!p->m_Unk238[i]) {
                            if (!p->m_Voices[i].m_pVoice->AudioHasEnded(p)) {
                                done = false;
                                continue;
                            }
                            p->m_Voices[i].m_pVoice->ES_UnLockVoiceHandle();
                            p->m_Voices[i].m_pVoice = 0;
                        } else {
                            ES_AudioStart((SFXSamplePoolFiles *)p->m_Unk238[i], p->m_Bank, p, i);
                            done = false;
                            p->m_Unk238[i] = 0;
                        }
                    }
                    ending = done;
                }
                if (ending) p->UpdateEnding(&pNext);
            } else {
                // Keying off: free the SFX once none of its voices is still sounding.
                bool done = true;
                for (int i = 0; i < 20; i++) {
                    EXAudio_Voice *pVoice = p->m_Voices[i].m_pVoice;
                    if (pVoice->AudioHasEnded(p)) {
                        pVoice->ES_UnLockVoiceHandle();
                        p->m_Voices[i].m_pVoice = 0;
                    } else
                        done = false;
                }
                if (done) {
                    DEBUGTRACE("ES-> Sfx->KeyOffWait OK.\n");
                    SFXItem *pAfter = p->m_pNext;
                    if (!p->m_pPrev) gpEXAudio->m_pSFXList = pAfter;
                    else p->m_pPrev->m_pNext = pAfter;
                    if (pAfter) pAfter->m_pPrev = p->m_pPrev;
                    p->m_pNext = gpEXAudio->m_pFreeList;
                    p->m_pPrev = 0;
                    gpEXAudio->m_pFreeList = p;
                    continue;
                }
            }
        }
        p->m_Unk324.x = p->m_Pos.x;
        p->m_Unk324.y = p->m_Pos.y;
        p->m_Unk324.z = p->m_Pos.z;
        ES_UpdateListenerVolume(p);
        float x = p->m_Pos.x;
        float y = p->m_Pos.y;
        float z = p->m_Pos.z;
        p->m_Velocity.x = p->m_Pos.x - p->m_Unk318.x;
        p->m_Velocity.y = p->m_Pos.y - p->m_Unk318.y;
        p->m_Velocity.z = p->m_Pos.z - p->m_Unk318.z;
        p->m_Unk318.x = x;
        p->m_Unk318.y = y;
        p->m_Unk318.z = z;
        for (int v = 0; v < 20; v++) {
            if (!p->m_Voices[v].m_pVoice) continue;
            psiSFX sfx;
            ES_FillpsiSFXData(&sfx, p, v);
            if (sfx.m_Voice != -1) psiUpdateSound(&sfx);
        }
    }
}
