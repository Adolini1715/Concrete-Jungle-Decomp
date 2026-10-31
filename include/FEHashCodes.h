#ifndef FEHASHCODES_H
#define FEHASHCODES_H

// Asset hash IDs the front end compares against or passes around. HT_* names with known values come
// from the Aug 2004 build's Albert/hashcodes.h; HT_Sfx_* values are retail-only (names are ours).
#define HT_File_HUD               0x01000025
#define HT_Texture_FEWhite        0x06000054
#define HT_Texture_FEScrollUp     0x06000082
#define HT_Texture_FEScrollDown   0x06000083
#define HT_Texture_FEScrollLeft   0x06000084
#define HT_Texture_FEScrollRight  0x06000085

// Ritual levels (FELevels.obj).
#define HT_File_RitualAssault1Day                0x010000b1
#define HT_File_RitualAssault2Day                0x010000b2
#define HT_File_GWarfare_Ritual_Challenge        0x010000e4
#define HT_File_RazeTheDepot_Ritual_Challenge    0x010000e6
#define HT_File_Copycat_Final_Ritual_Challenge   0x010000ea
#define HT_File_RA_POTC2                         0x0100012c
#define HT_File_Penthouse_RoS                    0x01000135
#define HT_File_Ritual8                          0x01000178  // not in the Aug 2004 list
#define HT_File_Training1                        0x01000160  // not in the Aug 2004 list
#define HT_File_Training2                        0x01000161
#define HT_File_Training3                        0x01000162

#define HT_Sfx_FE_Back            0x1af00158
#define HT_Sfx_FE_Select          0x1af0015c
#define HT_Sfx_FE_Change          0x1af00548
#endif
