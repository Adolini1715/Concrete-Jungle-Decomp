#include "XGamePad.h"
#include "EXMemory.h"
#include "XItem.h"
#include "FEText.h"
#include "GameData.h"
#include "XAppStandIn.h"

XButtonGraphic GraphicArray[56] = {
    {0x60001ae, 0x0, 1.0f, 0, 1, 0},
    {0x60000a2, 0x0, 1.0f, 0, 1, 0},
    {0x600009f, 0x0, 1.0f, 0, 1, 0},
    {0x60000a1, 0x0, 1.0f, 0, 1, 0},
    {0x60000a0, 0x0, 1.0f, 0, 1, 0},
    {0x0, 0x510002df, 1.0f, 0, 1, 0},
    {0x0, 0x510002e1, 1.0f, 0, 1, 0},
    {0x60000a4, 0x0, 1.0f, 1, 1, 0},
    {0x60001af, 0x0, 1.0f, 0, 1, 0},
    {0x60000a3, 0x0, 1.0f, 1, 1, 0},
    {0x60001b0, 0x0, 1.0f, 0, 1, 0},
    {0x0, 0x510002db, 1.0f, 0, 1, 0},
    {0x0, 0x510002dd, 1.0f, 0, 1, 0},
    {0x60000c2, 0x0, 1.0f, 0, 1, 0},
    {0x60000c4, 0x0, 1.0f, 0, 1, 0},
    {0x60000c3, 0x0, 1.0f, 0, 1, 0},
    {0x60001f4, 0x0, 1.0f, 0, 1, 0},
    {0x60001f1, 0x0, 1.0f, 0, 1, 0},
    {0x60001f2, 0x0, 1.0f, 0, 1, 0},
    {0x60001f3, 0x0, 1.0f, 0, 1, 0},
    {0x600018c, 0x0, 1.0f, 0, 1, 0},
    {0x600018d, 0x0, 1.0f, 0, 1, 0},
    {0x600009f, 0x0, 1.0f, 0, 1, 0},
    {0x60000a0, 0x0, 1.0f, 0, 1, 0},
    {0x0, 0x510002df, 1.0f, 0, 1, 0},
    {0x0, 0x510002e1, 1.0f, 0, 1, 0},
    {0x0, 0x51000344, 1.0f, 0, 1, 0},
    {0x60002d5, 0x0, 1.0f, 0, 1, 0},
    {0x0, 0x510002df, 1.0f, 0, 1, 0},
    {0x0, 0x510002e1, 1.0f, 0, 1, 0},
    {0x0, 0x510007c5, 1.0f, 0, 1, 0},
    {0x60002f4, 0x0, 1.0f, 0, 1, 0},
    {0x60002f6, 0x0, 1.0f, 0, 1, 0},
    {0x60002f5, 0x0, 1.0f, 0, 1, 0},
    {0x60002f7, 0x0, 1.0f, 0, 1, 0},
    {0x60002f2, 0x0, 1.0f, 0, 1, 0},
    {0x60002f3, 0x0, 1.0f, 0, 1, 0},
    {0x60002f1, 0x0, 1.0f, 0, 1, 0},
    {0x60002f0, 0x0, 1.0f, 0, 1, 0},
    {0x60002f8, 0x0, 1.0f, 0, 0, 0},
    {0x0, 0x510007f7, 1.0f, 0, 1, 0},
    {0x0, 0x510007f8, 1.0f, 0, 1, 0},
    {0x6000302, 0x0, 1.0f, 0, 1, 0},
    {0x6000303, 0x0, 1.0f, 0, 1, 0},
    {0x0, 0x51000816, 1.0f, 0, 1, 0},
    {0x0, 0x51000818, 1.0f, 0, 1, 0},
    {0x0, 0x51000819, 1.0f, 0, 1, 0},
    {0x600030b, 0x0, 1.0f, 0, 1, 0},
    {0x6000055, 0x0, 1.0f, 0, 1, 0},
    {0x6000309, 0x0, 1.0f, 0, 1, 0},
    {0x60002b2, 0x0, 1.0f, 0, 1, 0},
    {0x60002b5, 0x0, 1.0f, 0, 1, 0},
    {0x60002b3, 0x0, 1.0f, 0, 1, 0},
    {0x60002b6, 0x0, 1.0f, 0, 1, 0},
    {0x6000055, 0x0, 1.0f, 0, 1, 0},
    {0x6000311, 0x0, 1.0f, 0, 1, 0},
};
XButton ButtonArray[66] = {
    {35, &GraphicArray[47], 0x0},
    {36, &GraphicArray[48], 0x0},
    {37, &GraphicArray[49], 0x0},
    {38, &GraphicArray[43], 0x0},
    {39, &GraphicArray[42], 0x0},
    {40, &GraphicArray[38], 0x0},
    {41, &GraphicArray[37], 0x0},
    {42, &GraphicArray[34], 0x0},
    {43, &GraphicArray[33], 0x0},
    {44, &GraphicArray[36], 0x0},
    {45, &GraphicArray[35], 0x0},
    {46, &GraphicArray[39], 0x0},
    {47, &GraphicArray[32], 0x0},
    {48, 0, 0x400},
    {49, &GraphicArray[30], 0x0},
    {50, &GraphicArray[28], 0x0},
    {51, &GraphicArray[29], 0x0},
    {52, &GraphicArray[27], 0x0},
    {53, &GraphicArray[28], 0x0},
    {54, &GraphicArray[29], 0x0},
    {55, 0, 0x10},
    {56, 0, 0x1},
    {57, 0, 0x2},
    {58, 0, 0x4},
    {59, &GraphicArray[26], 0x0},
    {60, 0, 0x2000},
    {61, &GraphicArray[16], 0x0},
    {62, &GraphicArray[17], 0x0},
    {63, &GraphicArray[18], 0x0},
    {64, &GraphicArray[19], 0x0},
    {65, &GraphicArray[2], 0x0},
    {66, &GraphicArray[4], 0x0},
    {67, 0, 0x1000},
    {68, &GraphicArray[13], 0x0},
    {69, &GraphicArray[22], 0x0},
    {70, &GraphicArray[23], 0x0},
    {71, &GraphicArray[21], 0x0},
    {72, &GraphicArray[15], 0x0},
    {73, &GraphicArray[20], 0x0},
    {74, &GraphicArray[11], 0x0},
    {75, &GraphicArray[12], 0x0},
    {76, &GraphicArray[7], 0x0},
    {77, &GraphicArray[24], 0x0},
    {78, &GraphicArray[40], 0x0},
    {79, &GraphicArray[25], 0x0},
    {80, &GraphicArray[41], 0x0},
    {81, 0, 0x80},
    {82, &GraphicArray[9], 0x0},
    {83, &GraphicArray[8], 0x0},
    {84, &GraphicArray[10], 0x0},
    {85, 0, 0x40},
    {86, &GraphicArray[14], 0x0},
    {87, 0, 0x20},
    {88, &GraphicArray[3], 0x0},
    {89, &GraphicArray[1], 0x0},
    {90, 0, 0x8},
    {91, &GraphicArray[44], 0x0},
    {92, &GraphicArray[45], 0x0},
    {93, &GraphicArray[46], 0x0},
    {94, &GraphicArray[50], 0x0},
    {95, &GraphicArray[51], 0x0},
    {96, &GraphicArray[0], 0x0},
    {97, &GraphicArray[52], 0x0},
    {98, &GraphicArray[53], 0x0},
    {99, &GraphicArray[54], 0x0},
    {100, &GraphicArray[55], 0x0},
};
EXPadActionMap PadRemap4[15] = {
    {0x1000, 0x1}, {0x4000, 0x2}, {0x8000, 0x4}, {0x2000, 0x8}, {0x200000, 0x10}, {0x400000, 0x20}, {0x800000, 0x400}, {0x100000, 0x80}, {0x10000, 0x100}, {0x20000, 0x4000}, {0x80000, 0x200}, {0x1, 0x800}, {0x2, 0x40}, {0x4, 0x1000}, {0x8, 0x2000}
};
EXPadActionMap PadRemap3[15] = {
    {0x1000, 0x2}, {0x4000, 0x8}, {0x8000, 0x4}, {0x2000, 0x20}, {0x200000, 0x10}, {0x400000, 0x1}, {0x800000, 0x400}, {0x100000, 0x80}, {0x10000, 0x100}, {0x20000, 0x4000}, {0x80000, 0x200}, {0x1, 0x800}, {0x2, 0x40}, {0x4, 0x1000}, {0x8, 0x2000}
};
EXPadActionMap PadRemap2[15] = {
    {0x1000, 0x2}, {0x4000, 0x8}, {0x8000, 0x4}, {0x2000, 0x20}, {0x200000, 0x10}, {0x400000, 0x1}, {0x800000, 0x40}, {0x100000, 0x80}, {0x10000, 0x100}, {0x20000, 0x4000}, {0x80000, 0x200}, {0x1, 0x800}, {0x2, 0x400}, {0x4, 0x1000}, {0x8, 0x2000}
};
EXPadActionMap PadRemap1[15] = {
    {0x1000, 0x1}, {0x4000, 0x2}, {0x8000, 0x4}, {0x2000, 0x8}, {0x200000, 0x10}, {0x400000, 0x20}, {0x800000, 0x40}, {0x100000, 0x80}, {0x10000, 0x100}, {0x20000, 0x4000}, {0x80000, 0x200}, {0x1, 0x800}, {0x2, 0x400}, {0x4, 0x1000}, {0x8, 0x2000}
};
EXPadActionMap PadRemap0[15] = {
    {0x1000, 0x2}, {0x8000, 0x8}, {0x4000, 0x4}, {0x2000, 0x20}, {0x200000, 0x10}, {0x400000, 0x1}, {0x800000, 0x40}, {0x100000, 0x80}, {0x10000, 0x100}, {0x20000, 0x4000}, {0x80000, 0x200}, {0x1, 0x800}, {0x2, 0x400}, {0x4, 0x1000}, {0x8, 0x2000}
};
EXPadActionMap *PadRemap[6] = {PadRemap0, PadRemap1, PadRemap2, PadRemap3, PadRemap4, 0};
EXFixedArray<unsigned char, 70> Rumble0 = {{
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 253, 255, 252, 252, 250, 250, 248, 247, 246, 244, 245,
    241, 243, 238, 241, 232, 239, 226, 237, 220, 230, 214, 223, 207, 216, 201, 209, 195, 202, 189, 195, 182, 188, 170, 181,
    158, 174, 146, 166, 133, 148, 121, 130, 109, 111, 96, 93, 84, 75, 72, 56, 59, 38, 47, 0, 35, 0,
}};
EXFixedArray<unsigned char, 64> Rumble1 = {{
    255, 248, 252, 234, 244, 221, 235, 207, 227, 194, 219, 180, 210, 167, 202, 153, 193, 140, 185, 126, 177, 113, 168, 99,
    160, 86, 152, 72, 143, 59, 135, 45, 126, 32, 118, 18, 110, 5, 101, 0, 93, 0, 85, 0, 76, 0, 68, 0,
    59, 0, 51, 0, 43, 0, 34, 0, 26, 0, 18, 0, 9, 0, 1, 0,
}};
EXFixedArray<unsigned char, 178> Rumble2 = {{
    255, 255, 255, 255, 255, 254, 255, 251, 255, 248, 255, 237, 255, 232, 255, 227, 254, 224, 251, 215, 242, 209, 238, 206,
    235, 201, 230, 195, 224, 183, 221, 178, 215, 172, 206, 161, 195, 155, 187, 148, 165, 140, 154, 138, 150, 137, 140, 133,
    138, 131, 133, 125, 131, 123, 128, 122, 123, 115, 120, 114, 116, 102, 112, 98, 110, 96, 106, 90, 103, 89, 101, 86,
    96, 82, 88, 73, 86, 72, 82, 69, 80, 66, 74, 62, 72, 58, 68, 56, 65, 52, 61, 49, 57, 44, 55, 43,
    51, 40, 48, 38, 44, 37, 42, 35, 41, 33, 37, 32, 34, 30, 32, 29, 30, 28, 28, 26, 26, 25, 24, 24,
    22, 23, 20, 22, 19, 21, 19, 19, 18, 19, 17, 18, 16, 17, 15, 16, 15, 15, 14, 14, 14, 13, 13, 13,
    13, 12, 12, 11, 11, 11, 11, 10, 10, 10, 9, 9, 8, 7, 7, 7, 6, 6, 5, 5, 4, 4, 4, 3,
    3, 3, 3, 2, 2, 1, 2, 1, 1, 1,
}};
EXFixedArray<unsigned char, 236> Rumble3 = {{
    0, 21, 4, 30, 9, 39, 14, 48, 19, 57, 24, 66, 29, 75, 34, 84, 40, 93, 45, 102, 50, 111, 55, 120,
    60, 129, 65, 138, 70, 147, 75, 156, 80, 165, 86, 174, 91, 183, 96, 192, 101, 201, 106, 210, 111, 219, 116, 228,
    121, 237, 126, 246, 132, 250, 137, 239, 142, 228, 147, 217, 152, 207, 157, 196, 162, 185, 167, 174, 172, 164, 178, 153,
    183, 142, 188, 131, 193, 121, 198, 110, 203, 99, 208, 88, 213, 78, 218, 67, 224, 56, 229, 45, 234, 35, 239, 24,
    244, 13, 249, 2, 254, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0,
    255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0,
    255, 0, 255, 0, 255, 0, 255, 0, 252, 0, 246, 0, 240, 0, 234, 0, 228, 0, 222, 0, 216, 0, 210, 0,
    204, 0, 198, 0, 192, 0, 186, 0, 180, 0, 174, 0, 168, 0, 162, 0, 156, 0, 150, 0, 144, 0, 138, 0,
    132, 0, 126, 0, 120, 0, 114, 0, 108, 0, 102, 0, 96, 0, 90, 0, 84, 0, 78, 0, 72, 0, 66, 0,
    60, 0, 54, 0, 48, 0, 42, 0, 36, 0, 30, 0, 24, 0, 18, 0, 12, 0, 6, 0,
}};
EXFixedArray<unsigned char, 860> Rumble4 = {{
    241, 255, 235, 249, 227, 242, 222, 238, 220, 232, 217, 228, 220, 224, 223, 222, 228, 211, 231, 209, 235, 206, 237, 205,
    240, 204, 242, 204, 243, 206, 246, 207, 248, 208, 247, 209, 235, 210, 230, 214, 223, 215, 213, 216, 206, 218, 203, 219,
    201, 220, 199, 221, 197, 222, 193, 224, 192, 223, 193, 222, 197, 221, 198, 219, 200, 216, 207, 211, 209, 208, 220, 205,
    224, 202, 230, 200, 240, 195, 243, 193, 246, 190, 247, 188, 249, 187, 251, 185, 247, 181, 242, 179, 238, 175, 234, 173,
    231, 171, 228, 175, 225, 177, 220, 179, 218, 182, 216, 184, 209, 185, 207, 188, 203, 190, 201, 193, 198, 195, 186, 197,
    178, 199, 172, 200, 163, 204, 157, 206, 157, 208, 161, 210, 164, 211, 169, 213, 171, 215, 173, 217, 176, 218, 187, 218,
    194, 220, 206, 222, 210, 222, 216, 224, 224, 216, 232, 214, 234, 212, 238, 209, 240, 207, 243, 204, 245, 202, 249, 199,
    250, 195, 244, 192, 236, 189, 228, 186, 218, 179, 209, 172, 205, 171, 200, 172, 192, 175, 183, 177, 179, 182, 169, 184,
    162, 186, 162, 188, 163, 191, 164, 195, 166, 196, 169, 201, 171, 203, 172, 205, 176, 207, 177, 208, 179, 208, 184, 205,
    186, 203, 191, 200, 195, 197, 200, 196, 205, 194, 206, 188, 209, 185, 214, 182, 216, 179, 222, 172, 224, 167, 226, 163,
    231, 161, 233, 156, 203, 153, 198, 146, 196, 145, 188, 142, 155, 141, 150, 139, 146, 135, 141, 134, 139, 133, 138, 132,
    135, 129, 140, 128, 142, 129, 147, 130, 151, 133, 154, 134, 157, 137, 159, 141, 162, 144, 169, 149, 176, 151, 182, 154,
    186, 159, 197, 166, 201, 166, 187, 159, 179, 156, 175, 153, 170, 147, 162, 144, 157, 142, 151, 131, 144, 127, 141, 125,
    136, 122, 133, 120, 129, 119, 124, 119, 120, 120, 117, 122, 115, 124, 112, 126, 115, 128, 117, 135, 120, 137, 125, 140,
    126, 142, 129, 143, 133, 145, 140, 145, 143, 147, 145, 148, 154, 142, 160, 129, 164, 126, 176, 123, 183, 118, 188, 115,
    199, 114, 188, 117, 184, 119, 172, 121, 155, 122, 145, 123, 134, 125, 125, 126, 122, 128, 119, 128, 114, 128, 109, 129,
    105, 105, 99, 99, 96, 91, 93, 88, 88, 86, 86, 81, 84, 79, 79, 74, 77, 72, 71, 68, 69, 67, 67, 62,
    63, 60, 65, 59, 67, 55, 70, 51, 73, 50, 79, 50, 82, 54, 84, 60, 87, 65, 91, 69, 94, 72, 97, 74,
    99, 78, 127, 82, 136, 85, 145, 88, 157, 93, 170, 97, 164, 101, 158, 108, 147, 113, 141, 119, 136, 123, 130, 127,
    124, 128, 118, 133, 112, 135, 104, 134, 102, 132, 96, 129, 92, 124, 91, 121, 89, 108, 85, 102, 78, 95, 74, 83,
    71, 52, 68, 48, 65, 46, 52, 45, 50, 46, 48, 47, 45, 49, 39, 51, 36, 52, 33, 53, 31, 54, 30, 56,
    34, 58, 37, 59, 42, 60, 44, 61, 46, 62, 51, 63, 55, 64, 58, 65, 60, 67, 64, 68, 67, 70, 79, 72,
    85, 73, 93, 74, 104, 74, 106, 74, 111, 75, 112, 76, 105, 77, 94, 77, 91, 78, 88, 78, 83, 78, 80, 76,
    75, 72, 67, 71, 64, 69, 60, 66, 56, 64, 55, 61, 50, 58, 47, 56, 42, 49, 38, 47, 36, 43, 29, 38,
    23, 37, 22, 35, 17, 37, 13, 39, 12, 40, 10, 42, 8, 45, 7, 46, 9, 47, 12, 50, 16, 51, 21, 52,
    24, 53, 27, 56, 34, 57, 37, 58, 43, 60, 48, 62, 54, 64, 56, 66, 59, 67, 63, 68, 65, 69, 70, 70,
    73, 69, 77, 67, 79, 61, 68, 59, 67, 56, 62, 53, 57, 49, 54, 45, 52, 43, 50, 40, 46, 37, 43, 33,
    39, 32, 36, 31, 34, 27, 29, 25, 27, 22, 21, 20, 19, 19, 17, 17, 14, 16, 13, 16, 11, 19, 9, 21,
    7, 24, 8, 29, 10, 32, 14, 35, 16, 36, 19, 37, 24, 38, 26, 38, 29, 38, 34, 39, 35, 39, 43, 40,
    40, 40, 37, 40, 33, 40, 30, 40, 28, 40, 23, 40, 21, 40, 20, 40, 17, 39, 16, 38, 12, 37, 9, 35,
    10, 33, 10, 31, 13, 30, 15, 29, 16, 26, 16, 24, 18, 22, 18, 21, 18, 19, 14, 18, 14, 16, 11, 14,
    9, 12, 9, 12, 11, 11, 12, 10, 14, 10, 15, 9, 17, 9, 20, 9, 21, 11, 18, 17, 17, 19, 15, 21,
    13, 27, 12, 28, 11, 29, 11, 29, 11, 28, 11, 24, 12, 18, 13, 15, 14, 11, 14, 10, 14, 7, 15, 3,
    15, 1, 11, 0, 11, 0, 7, 0, 8, 0, 9, 0, 10, 0, 11, 0, 10, 1, 10, 1, 8, 1, 7, 1,
    6, 1, 6, 1, 5, 1, 5, 1, 5, 1, 5, 1, 5, 1, 3, 1, 2, 1, 1, 1, 1, 1, 0, 1,
    0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1,
}};
EXFixedArray<unsigned char, 1400> Rumble5 = {{
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255, 0, 255,
    0, 255, 0, 255, 0, 255, 0, 255,
}};
EXFixedArray<unsigned char, 8> Rumble6 = {{
    255, 255, 255, 255, 255, 255, 255, 255,
}};
EXFixedArray<unsigned char, 24> Rumble7 = {{
    255, 255, 255, 255, 255, 255, 255, 255, 224, 255, 191, 225, 159, 192, 126, 160, 94, 127, 61, 94, 29, 62, 0, 29,
}};
EXFixedArray<unsigned char, 66> Rumble8 = {{
    250, 254, 233, 248, 224, 240, 215, 232, 207, 224, 198, 217, 189, 209, 181, 201, 172, 193, 163, 186, 155, 178, 146, 170,
    137, 162, 129, 155, 120, 147, 112, 139, 103, 131, 94, 124, 86, 116, 77, 108, 68, 100, 60, 92, 51, 85, 42, 77,
    34, 69, 25, 61, 16, 54, 8, 46, 0, 38, 0, 30, 0, 23, 0, 15, 0, 7,
}};
EXFixedArray<unsigned char, 188> Rumble9 = {{
    249, 250, 245, 248, 241, 246, 235, 239, 232, 236, 229, 233, 226, 230, 222, 227, 215, 223, 211, 219, 207, 216, 203, 213,
    199, 209, 196, 204, 193, 199, 189, 197, 185, 195, 181, 193, 176, 190, 171, 185, 166, 180, 160, 173, 158, 163, 156, 162,
    150, 168, 146, 172, 138, 195, 128, 196, 125, 185, 121, 179, 117, 175, 114, 171, 111, 163, 109, 156, 107, 122, 105, 124,
    102, 125, 113, 126, 116, 128, 118, 129, 122, 131, 125, 133, 128, 135, 130, 136, 136, 139, 203, 141, 179, 143, 171, 145,
    164, 147, 160, 149, 156, 151, 152, 152, 141, 154, 129, 155, 124, 156, 118, 157, 108, 158, 89, 158, 107, 158, 113, 158,
    124, 158, 126, 157, 128, 156, 129, 155, 76, 153, 71, 151, 68, 148, 62, 144, 60, 140, 58, 137, 36, 133, 38, 87,
    38, 71, 38, 73, 38, 75, 33, 77, 31, 79, 6, 81, 0, 29, 0, 17, 0, 17, 0, 17, 0, 17, 0, 18,
    0, 18, 0, 18, 0, 18, 0, 17, 0, 16, 0, 14, 0, 10, 0, 8, 0, 5, 0, 3,
}};
EXFixedArray<unsigned char, 238> Rumble10 = {{
    55, 74, 56, 79, 57, 85, 58, 91, 59, 97, 60, 103, 61, 109, 62, 115, 63, 122, 65, 128, 66, 134, 67, 140,
    68, 146, 69, 152, 70, 158, 71, 165, 72, 171, 73, 177, 74, 183, 75, 189, 76, 195, 77, 201, 78, 208, 77, 214,
    79, 220, 81, 226, 82, 232, 84, 238, 86, 244, 88, 250, 90, 255, 92, 255, 94, 255, 95, 255, 97, 255, 99, 255,
    101, 255, 103, 255, 105, 255, 107, 255, 108, 255, 110, 255, 112, 255, 114, 255, 116, 255, 118, 255, 120, 255, 121, 255,
    123, 255, 125, 255, 127, 255, 129, 255, 131, 255, 132, 255, 139, 255, 145, 255, 151, 255, 157, 255, 163, 255, 169, 255,
    175, 255, 181, 255, 187, 255, 193, 255, 199, 255, 205, 255, 211, 255, 217, 255, 223, 255, 229, 255, 235, 255, 241, 255,
    247, 255, 253, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 244,
    255, 237, 255, 230, 255, 223, 255, 216, 255, 209, 255, 202, 255, 195, 255, 188, 255, 181, 255, 174, 242, 168, 226, 161,
    210, 154, 194, 147, 178, 140, 162, 133, 146, 126, 130, 119, 114, 112, 98, 105, 82, 98, 66, 91, 50, 85, 34, 78,
    18, 71, 2, 64, 0, 57, 0, 50, 0, 43, 0, 36, 0, 29, 0, 22, 0, 15, 0, 8, 0, 1,
}};
EXFixedArray<unsigned char, 64> Rumble11 = {{
    22, 16, 44, 32, 65, 48, 87, 64, 109, 81, 130, 97, 152, 113, 174, 129, 196, 146, 255, 162, 253, 178, 241, 194,
    229, 211, 217, 227, 205, 243, 193, 255, 181, 244, 168, 229, 156, 213, 144, 198, 132, 182, 120, 167, 108, 151, 96, 136,
    83, 120, 71, 105, 59, 89, 47, 74, 35, 58, 23, 43, 11, 27, 0, 12,
}};
EXFixedArray<unsigned char, 70> Rumble12 = {{
    165, 175, 166, 176, 166, 176, 166, 176, 166, 176, 166, 176, 166, 176, 166, 176, 175, 189, 182, 201, 189, 213, 196, 225,
    203, 237, 210, 249, 217, 255, 224, 249, 231, 236, 238, 223, 245, 211, 252, 198, 255, 185, 239, 173, 219, 160, 198, 147,
    178, 134, 157, 122, 137, 109, 116, 96, 96, 84, 75, 71, 55, 58, 34, 45, 14, 33, 0, 20, 0, 7,
}};
EXFixedArray<_RUMBLEDATA, 13> RumblePointers = {{
    {Rumble0, 35},
    {Rumble1, 32},
    {Rumble2, 89},
    {Rumble3, 118},
    {Rumble4, 430},
    {Rumble5, 700},
    {Rumble6, 4},
    {Rumble7, 12},
    {Rumble8, 33},
    {Rumble9, 94},
    {Rumble10, 119},
    {Rumble11, 32},
    {Rumble12, 35},
}};
int Default_ControllerConfig;
EXFixedArray<_ACTIVE_RUMBLES, 3> ActiveRumbles;

void XGamePad::SetActions(int config)
{
    m_Config = config;
    m_bActionsChanged = 1;
    SetActionMap((u8)GameControllerRemovedSys::Controller(), 15, PadRemap[config]);
}

// NEAR: pointer and action registers swapped (eax/ecx).
EXPad::AnalogChan XGamePad::GetAnalogueFromAction(unsigned int action)
{
    EXPadActionMap *p = PadRemap[m_Config];
    while (p->m_Action != action) p++;
    EXPad::AnalogChan chan = (EXPad::AnalogChan)14;
    if (p->m_Buttons == 0x2000) chan = (EXPad::AnalogChan)9;
    return chan;
}

s8 XGamePad::Init(XGamePad *pPad)
{
    EXBaseGamePad::Init(pPad);
    if (!((EXGamePad *)GetGamePad())->OpenPads()) return 0;
    pPad->m_Config = Default_ControllerConfig;
    pPad->m_bActionsChanged = 1;
    pPad->SetActionMap((u8)GameControllerRemovedSys::Controller(), 15, PadRemap[pPad->m_Config]);
    pPad->EnableAllRumble(1);
    return 1;
}

EXString XGamePad::GetActionString(int action)
{
    EXPadActionMap *pMap = PadRemap[m_Config];
    for (int i = 0; i < 15; i++) {
        if (pMap[i].m_Action != action) continue;
        switch (pMap[i].m_Buttons) {
    case 1: return EXString("~=");
    case 2: return EXString("~>");
    case 4: return EXString("~?");
    case 8: return EXString("~@");
    case 0x1000: return EXString("~Y");
    case 0x8000: return EXString("~B");
    case 0x4000: return EXString("~X");
    case 0x2000: return EXString("~A");
    case 0x10000: return EXString("~N");
    case 0x80000: return EXString("~K");
    case 0x40000: return EXString("~J");
    case 0x20000: return EXString("~P");
    case 0x100000: return EXString("~L");
    case 0x800000: return EXString("~T");
    case 0x400000: return EXString("~R");
    case 0x200000: return EXString("~S");
        }
        break;
    }
    return EXString("");
}

XButton *XGamePad::LookupButton(char trigger)
{
    int idx = trigger - '#';
    if (!ButtonArray[idx].Action) return &ButtonArray[idx];
    EXPadActionMap *pMap = PadRemap[m_Config];
    for (int i = 0; i < 15; i++) {
        if (pMap[i].m_Action != ButtonArray[idx].Action) continue;
        switch (pMap[i].m_Buttons) {
    case 1: ButtonArray[idx].Graphic = &GraphicArray[16]; return &ButtonArray[idx];
    case 2: ButtonArray[idx].Graphic = &GraphicArray[17]; return &ButtonArray[idx];
    case 4: ButtonArray[idx].Graphic = &GraphicArray[18]; return &ButtonArray[idx];
    case 8: ButtonArray[idx].Graphic = &GraphicArray[19]; return &ButtonArray[idx];
    case 0x4000: ButtonArray[idx].Graphic = &GraphicArray[3]; return &ButtonArray[idx];
    case 0x2000: ButtonArray[idx].Graphic = &GraphicArray[2]; return &ButtonArray[idx];
    case 0x1000: ButtonArray[idx].Graphic = &GraphicArray[1]; return &ButtonArray[idx];
    case 0x8000: ButtonArray[idx].Graphic = &GraphicArray[4]; return &ButtonArray[idx];
    case 0x100000: ButtonArray[idx].Graphic = &GraphicArray[7]; return &ButtonArray[idx];
    case 0x20000: ButtonArray[idx].Graphic = &GraphicArray[6]; return &ButtonArray[idx];
    case 0x10000: ButtonArray[idx].Graphic = &GraphicArray[5]; return &ButtonArray[idx];
    case 0x200000: ButtonArray[idx].Graphic = &GraphicArray[8]; return &ButtonArray[idx];
    case 0x800000: ButtonArray[idx].Graphic = &GraphicArray[10]; return &ButtonArray[idx];
    case 0x400000: ButtonArray[idx].Graphic = &GraphicArray[9]; break;
        }
        return &ButtonArray[idx];
    }
    return &ButtonArray[0];
}

unsigned int XGamePad::SetMask(unsigned int mask)
{
    if (gpPlayerItem[0] && 0.0f >= XItemHandler_Player::Instance()->m_Health) mask &= 0xffff8600;
    return mask;
}

XGamePad::~XGamePad() {}

EXString XGamePad::GetControllerString(unsigned int hash)
{
    unsigned int buttons;
    switch (hash) {
    case 0x100000d7: buttons = 0x1000; break;
    case 0x100000d8: buttons = 0x2000; break;
    case 0x100000d9: buttons = 0x4000; break;
    case 0x100000da: buttons = 0x8000; break;
    case 0x100000db: buttons = 0x10000; break;
    case 0x100000dc: buttons = 0x20000; break;
    case 0x100000dd: buttons = 0x100000; break;
    case 0x100000de: buttons = 0x200000; break;
    case 0x100000df: buttons = 0x400000; break;
    case 0x100000e0: buttons = 0x800000; break;
    case 0x100000e3: buttons = 1; break;
    case 0x100000e4: buttons = 2; break;
    case 0x100000e5: buttons = 4; break;
    case 0x100000e6: buttons = 8; break;
    case 0x100000e7: return Text_Get(0x510004b3);
    case 0x100000e8: return Text_Get(0x510004b4);
    case 0x100000e1: return Text_Get(0x510004b1);
    case 0x100000e2: return Text_Get(0x510004b2);
    default: return gText.m_Empty;
    }
    EXPadActionMap *pMap = PadRemap[m_Config];
    int i;
    for (i = 0; i < 15; i++)
        if (pMap[i].m_Buttons == buttons) goto found;
    return gText.m_Empty;
found:
    {
        switch (pMap[i].m_Action) {
    case 8: return Text_Get(0x510004a3);
    case 1: return Text_Get(0x510004a4);
    case 2: return Text_Get(0x510004a5);
    case 4: return Text_Get(0x510004a6);
    case 0x10: return Text_Get(0x510004a8);
    case 0x20: return Text_Get(0x510004a7);
    case 0x40: return Text_Get(0x510004a9);
    case 0x80: return gText.Get(0x510004aa);
    case 0x800: return Text_Get(0x510004ad);
    case 0x400: return Text_Get(0x510004ae);
    case 0x100: return Text_Get(0x510004ab);
    case 0x1000: return gText.Get(0x510004af);
    case 0x4000: return Text_Get(0x510004ac);
    case 0x2000: return gText.Get(0x510004b0);
        }
    }
    return gText.m_Empty;
}

unsigned int XGamePad::XActions(u8 pad) { return SetMask(m_Actions[pad]); }
unsigned int XGamePad::XActionPressed(u8 pad) { return SetMask(m_ActionsPressed[pad]); }
s8 XGamePad::XActionPressed(u8 pad, unsigned int action) { return ActionPressed(pad, SetMask(action)); }
unsigned int XGamePad::XActionReleased(u8 pad) { return SetMask(m_ActionsReleased[pad]); }
s8 XGamePad::XActionReleased(u8 pad, unsigned int action) { return ActionReleased(pad, SetMask(action)); }
s8 XGamePad::XActionUp(u8 pad, unsigned int action) { return ActionUp(pad, SetMask(action)); }
s8 XGamePad::XActionDown(u8 pad, unsigned int action) { return ActionDown(pad, SetMask(action)); }

void XGamePad::InitRumbleManager()
{
    for (int i = 0; i < 3; i++) ActiveRumbles[i].m_bActive = 0;
}

void XGamePad::StopAllRumble()
{
    for (int i = 0; i < 3; i++) ActiveRumbles[i].m_bActive = 0;
    KillAllRumble();
}

int XGamePad::GetRumbleSlot()
{
    int i;
    for (i = 0; i < 3; i++)
        if (!ActiveRumbles[i].m_bActive) return i;
    float best = 0.0f;
    int slot = 0;
    for (i = 0; i < 3; i++) {
        if (ActiveRumbles[i].m_bActive && ActiveRumbles[i].m_Time > best) {
            best = ActiveRumbles[i].m_Time;
            slot = i;
        }
    }
    return slot;
}

void XGamePad::StopRumbleID(unsigned int id)
{
    for (int i = 0; i < 3; i++)
        if (ActiveRumbles[i].m_bActive && ActiveRumbles[i].m_Id == id) ActiveRumbles[i].m_Length = 0;
}

// NEAR: xmm register assignment and the clamp/ftol order before StartRumbleLo differ.
void XGamePad::UpdateRumble()
{
    XItem *pPlayer = gpPlayerItem[0];
    for (int pad = 0; pad < 8; pad++) {
        if (!PadAttached((u8)pad)) continue;
        float lo = 0.0f;
        s8 bAny = 0;
        float hi = 0.0f;
        for (int i = 0; i < 3; i++) {
            _ACTIVE_RUMBLES &r = ActiveRumbles[i];
            if (!r.m_bActive || r.m_Pad != pad) continue;
            int t = (int)r.m_Time;
            bAny = 1;
            float a, b;
            if (t < r.m_Length) {
                u8 *p = &r.m_pData[t * 2];
                float mult = r.m_Mult;
                float vHi = (float)p[1];
                float vLo = (float)p[0];
                r.m_Time += XApp::GameFrameRateMul() * 0.5f;
                if (r.m_b3D && pPlayer) {
                    EXVector d = pPlayer->Position() - r.m_Pos;
                    float dist = EXSqrt(d.LengthXYZSquared()) - r.m_MinDist;
                    if (dist > 0.0f) {
                        float f = r.m_Range - dist;
                        if (f > 0.0f) mult = f / r.m_Range * mult;
                        else mult = 0.0f;
                    }
                }
                a = mult * vLo;
                b = mult * vHi;
            } else {
                b = 0.0f;
                a = 0.0f;
                r.m_bActive = 0;
            }
            lo = max(lo, a);
            hi = max(hi, b);
        }
        if (bAny) {
            StartRumbleLo((u8)pad, (u8)limit(lo, 0.0f, 255.0f), 10);
            StartRumbleHi((u8)pad, (u8)limit(hi, 0.0f, 255.0f), 10);
            pPlayer = gpPlayerItem[0];
        }
    }
}

s8 XGamePad::IsRumbleActive(unsigned int id, int slot)
{
    if (slot >= 0 && slot < 3) {
        _ACTIVE_RUMBLES *r = &ActiveRumbles[slot];
        if (r->m_bActive) return r->m_Id == id;
    }
    return 0;
}

void XGamePad::AlterRumbleMult(int slot, float mult)
{
    if (slot >= 0 && slot < 3) ActiveRumbles[slot].m_Mult = mult;
}

// NEAR: retail keeps id in esi (register choice only).
int XGamePad::PlayRumble(unsigned int id, float mult, u8 pad)
{
    if (pad >= 8) pad = (u8)GameControllerRemovedSys::Controller();
    int slot = GetRumbleSlot();
    _ACTIVE_RUMBLES &r = ActiveRumbles[slot];
    r.m_Pad = pad;
    r.m_pData = RumblePointers[id - 0xc000000].m_pData;
    r.m_Id = id;
    r.m_bActive = 1;
    r.m_b3D = 0;
    r.m_Time = 0.0f;
    r.m_Mult = mult;
    r.m_Unk1c = 0.0f;
    r.m_Length = RumblePointers[id - 0xc000000].m_Length;
    return slot;
}

// NEAR: zero/mult register order and the active-flag stores are scheduled differently.
void XGamePad::PlayRumble3D(unsigned int id, EXVector &pos, float minDist, float maxDist, float mult, u8 pad)
{
    if (pad >= 8) pad = (u8)GameControllerRemovedSys::Controller();
    _ACTIVE_RUMBLES &r = ActiveRumbles[GetRumbleSlot()];
    r.m_Id = id;
    r.m_Pad = pad;
    r.m_Length = RumblePointers[id - 0xc000000].m_Length;
    r.m_Mult = mult;
    r.m_Time = 0.0f;
    r.m_Unk1c = 0.0f;
    r.m_bActive = 1;
    r.m_b3D = 1;
    r.m_pData = RumblePointers[id - 0xc000000].m_pData;
    r.m_Pos = pos;
    r.m_MinDist = minDist;
    r.m_Range = maxDist - minDist;
}

XGamePad::XGamePad()
{
    m_Config = Default_ControllerConfig;
    m_bActionsChanged = 0;
    for (int i = 0; i < 3; i++) ActiveRumbles[i].m_bActive = 0;
}
