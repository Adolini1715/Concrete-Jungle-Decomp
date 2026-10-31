#include "types.h"

// IMA ADPCM decoder producing float samples. Tables are file statics.
static const int s_IndexAdjust[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
static const int s_StepTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97,
    107, 118, 130, 143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871,
    5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623,
    27086, 29794, 32767
};
static int s_NextIndex[89][16];
static int s_Delta[89][16];

// NEAR: 4 bytes, eax/ecx swapped for the inner counter and delta.
static int ADPCM_Init()
{
    int step, n;
    for (step = 0; step < 89; step++) {
        for (n = 0; n < 16; n++) {
            int s = s_StepTable[step];
            int d = s >> 3;
            if (n & 4) d += s;
            if (n & 2) d += s >> 1;
            if (n & 1) d += s >> 2;
            if (n & 8) d = -d;
            s_Delta[step][n] = d;
            int idx = s_IndexAdjust[n] + step;
            s_NextIndex[step][n] = idx < 0 ? 0 : (idx > 88 ? 88 : idx);
        }
    }
    return 0;
}

static int s_ADPCMInit = ADPCM_Init();

// Decodes 'bytes' of 4-bit data in 0x20-byte blocks (4-byte header: s16 predictor, u8 index).
// Stereo data alternates blocks between the two channels, written interleaved.
// NEAR: ~300 bytes differ, stack-slot/register assignment of the locals (retail keeps stride in a param home).
void ADPCM_Decrunch(const void *pSrc, void *pDst, int bytes, int channels, int interleaved)
{
    const u8 *src = (const u8 *)pSrc;
    float *dst = (float *)pDst;
    int n = bytes / 4;
    int count = 0;
    int toggle = 0;
    int stride = 1;
    int offA = 0;
    int offB = 0;
    int pred = 0;
    int index = 0;
    if (channels == 2) {
        if (interleaved) {
            stride = 2;
            offA = -1;
            offB = -0x6f;
        } else {
            offA = -(n / 2);
            offB = n / 2 - 0x38;
            stride = 1;
        }
        dst -= offA;
    }
    for (int i = 0; i < n; i += 2) {
        if (--count <= 0) {
            pred = (s8)src[1] << 8 | src[0];
            index = src[2];
            src += 2;
            src += 2;
            count = 0x1c;
            if ((u32)index > 88) index = 0;
            if (channels == 2) {
                dst += toggle ? offB : offA;
                toggle ^= 1;
            }
        }
        int b = *src;
        int lo = b & 0xf;
        int hi = b >> 4;
        pred += s_Delta[index][hi];
        index = s_NextIndex[index][hi];
        *dst = (float)pred;
        pred += s_Delta[index][lo];
        index = s_NextIndex[index][lo];
        dst += stride;
        *dst = (float)pred;
        src++;
        dst += stride;
    }
}
