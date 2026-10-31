#ifndef EXGEOHEADER_H
#define EXGEOHEADER_H
#include "types.h"
#include "EXRelPtr.h"
#include "EXGeoCommonArray.h"

class EXFile;
class EXGeoFile;
class EXGeoSectionFile;
class EXGeoDebugFile;
class EXGeoDebugHeader;
class EXGeoBaseDebug;
class EXGeoDebugStats;
class EXGeoCommonObject;
class EXTexture;
class EXTextureArray;
class EXGeoTexture;
class EXGeoEntity;
class EXGeoAnim;
class EXGeoAnimSkin;
class EXGeoScript;
class EXGeoMap;
class EXGeoParticle;
class EXGeoSwoosh;
class EXGeoSpreadSheet;
class EXGeoTextItem;
class EXGeoMaterial;
class EXu32Array;
struct EXGeoFont;
struct EXGeoAnimMode;
struct EXGeoAnimSet;
struct EXGeoForceFeedback;
struct EXGeoFileDataBlock;
struct EXBoundsBox;

// List elements. Each starts with EXGeoCommonArrayElement (hash, section/created, addresses);
// sizeof is the stride the header's lists use.
class EXGeoObjectHeader : public EXGeoCommonArrayElement<EXGeoCommonObject> {};  // 0x10
class EXGeoRefPointerHeader : public EXGeoCommonArrayElement<EXGeoCommonObject> {};  // 0x10
class EXGeoEntityHeader : public EXGeoCommonArrayElement<EXGeoEntity> {  // 0x14
public:
    u32 m_10;
};
class EXGeoAnimHeader : public EXGeoCommonArrayElement<EXGeoAnim> {  // 0x20
public:
    u8 *MotionData() const
    {
        if (!this) return 0;
        return (u8 *)m_Motion.m_pObject;
    }
    u32 DataSize() const { return m_DataSize; }
    EXGeoCommonAddr m_Motion;   // +0x10
    u32 m_DataSize;             // +0x18
    u32 m_1c;
};
class EXGeoAnimSkinHeader : public EXGeoCommonArrayElement<EXGeoAnimSkin> {  // 0x24
public:
    u32 MipRef() const { return m_MipRef; }
    u32 m_10;
    u32 m_MipRef;               // +0x14 next lod (index in the low 24 bits), 0: none
    float m_LodDistance;        // +0x18
    u8 m_1c[8];
    static const EXGeoAnimSkinHeader *CalcAnimSkinLod(struct EXGeoComplexRefObject &ref, unsigned char &lod,
                                                      unsigned int minLod, unsigned int maxLod, float dist);
};
class EXGeoScriptHeader : public EXGeoCommonArrayElement<EXGeoScript> {};  // 0x10
class EXGeoMapHeader : public EXGeoCommonArrayElement<EXGeoMap> {};  // 0x10
class EXGeoAnimModeHeader : public EXGeoCommonArrayElement<EXGeoAnimMode> {  // 0x14
public:
    u32 m_10;
};
class EXGeoAnimSetHeader : public EXGeoCommonArrayElement<EXGeoAnimSet> {  // 0x14
public:
    u32 m_10;
};
class EXGeoParticleHeader : public EXGeoCommonArrayElement<EXGeoParticle> {};  // 0x10
class EXGeoSwooshHeader : public EXGeoCommonArrayElement<EXGeoSwoosh> {};  // 0x10
class EXGeoSpreadSheetHeader : public EXGeoCommonArrayElement<EXGeoSpreadSheet> {  // 0x14
public:
    u32 Type() const { return m_Type; }
    u32 m_Type;                 // +0x10
};
class EXGeoFontHeader : public EXGeoCommonArrayElement<EXGeoFont> {};  // 0x10
class EXGeoForceFeedbackHeader : public EXGeoCommonArrayElement<EXGeoForceFeedback> {};  // 0x10
class EXGeoMaterialHeader : public EXGeoCommonArrayElement<EXGeoMaterial> {};  // 0x10
class EXGeoTextureHeader : public EXGeoCommonArrayElement<EXGeoTexture> {  // 0x1c
public:
    u32 Flags() const { return m_Flags18; }
    void SetRuntimeValues(EXGeoTexture *pTex, unsigned short a, unsigned short b, unsigned int c, unsigned int d);
    u16 m_10;
    u16 m_12;
    u32 m_14;
    u32 m_Flags18;              // +0x18
};

// Section table entry (0x10): hash, file range, section file while loaded.
class EXGeoSectionHeader {
public:
    EXFile *File() const;
    u32 Length() const { return m_End - m_SeekPos; }
    u32 SeekPos() const { return m_SeekPos; }
    u32 HashCode() const { return m_HashCode; }  // [fi] in ActorUtil.obj
    u32 m_HashCode;                 // +0
    union {
        u32 m_SeekPos;              // +4
        ex::EXABSOFFSET32 m_Seek;
    };
    u32 m_End;                      // +8
    EXGeoSectionFile *m_pFile;      // +0xc
};

// In-memory .edb header (fixed up in place after load).
class EXGeoHeader {
public:
    unsigned int HashCode() const { return m_HashCode; }
    unsigned int Version() const { return m_Version; }
    unsigned int Flags() const { return m_Flags; }
    unsigned int Time() const { return m_Time; }
    unsigned int FileSize() const { return m_FileSize; }
    unsigned int BaseFileSize() const { return m_BaseFileSize; }
    EXGeoDebugFile *DebugFile() const { return m_pDebugFile; }
    EXGeoFile *GeoFile() const { return m_pGeoFile; }
    EXTextureArray &TextureArray() { return *m_pTextureArray; }
    EXGeoFileDataBlock *GeoFileDataBlock() const { return m_pDataBlock; }
    void SetAnimTextureSpeedScale(float f) { *(float *)m_pDataBlock = f; }
    unsigned int *MapZoneSectionList() { return (unsigned int *)m_pDataBlock + 1; }

    const EXGeoCommonArray<EXGeoSectionHeader> &SectionList() const { return m_Sections; }
    const EXGeoCommonArray<EXGeoRefPointerHeader> &RefPointerList() const { return m_RefPointers; }
    const EXGeoCommonArray<EXGeoEntityHeader> &EntityList() const { return m_Entities; }
    const EXGeoCommonArray<EXGeoAnimHeader> &AnimList() const { return m_Anims; }
    const EXGeoCommonArray<EXGeoAnimSkinHeader> &AnimSkinList() const { return m_AnimSkins; }
    const EXGeoCommonArray<EXGeoScriptHeader> &ScriptList() const { return m_Scripts; }
    const EXGeoCommonArray<EXGeoMapHeader> &MapList() const { return m_Maps; }
    const EXGeoCommonArray<EXGeoAnimModeHeader> &AnimModeList() const { return m_AnimModes; }
    const EXGeoCommonArray<EXGeoAnimSetHeader> &AnimSetList() const { return m_AnimSets; }
    const EXGeoCommonArray<EXGeoParticleHeader> &ParticleList() const { return m_Particles; }
    const EXGeoCommonArray<EXGeoSwooshHeader> &SwooshList() const { return m_Swooshes; }
    const EXGeoCommonArray<EXGeoSpreadSheetHeader> &SpreadSheetList() const { return m_SpreadSheets; }
    const EXGeoCommonArray<EXGeoFontHeader> &FontList() const { return m_Fonts; }
    const EXGeoCommonArray<EXGeoForceFeedbackHeader> &ForceFeedbackList() const { return m_ForceFeedbacks; }
    const EXGeoCommonArray<EXGeoMaterialHeader> &MaterialList() const { return m_Materials; }
    const EXGeoCommonArray<EXGeoTextureHeader> &TextureList() const { return m_Textures; }

    EXGeoSectionHeader *SectionHeader(unsigned int hash) const
    {
        if (hash & 0xff000000) return SectionList().Find(hash);
        return &SectionList().Data()[hash];
    }
    unsigned int SectionIndex(unsigned int sec) const
    {
        if (sec & 0xff000000) sec = SectionList().FindIndex(sec);
        return sec;
    }
    EXGeoEntityHeader *GeoEntityHeader(unsigned int hash) const { return EntityList().Find(hash); }
    EXGeoEntity *GeoEntity(unsigned int hash) const { return EntityList().Find(hash)->Object(0); }
    EXGeoAnimHeader *GeoAnimHeader(unsigned int hash) const { return AnimList().Find(hash); }
    EXGeoAnim *GeoAnim(unsigned int hash) const { return AnimList().Find(hash)->Object(0); }
    EXGeoAnimSkinHeader *GeoAnimSkinHeader(unsigned int hash) const { return AnimSkinList().Find(hash); }
    EXGeoAnimSkin *GeoAnimSkin(unsigned int hash) const { return AnimSkinList().Find(hash)->Object(0); }
    EXGeoScriptHeader *GeoScriptHeader(unsigned int hash) const { return ScriptList().Find(hash); }
    EXGeoScript *GeoScript(unsigned int hash) const { return ScriptList().Find(hash)->Object(0); }
    EXGeoMapHeader *GeoMapHeader(unsigned int hash) const { return MapList().Find(hash); }
    EXGeoMap *GeoMap(unsigned int hash) const { return MapList().Find(hash)->Object(0); }
    EXGeoAnimModeHeader *GeoAnimModeHeader(unsigned int hash) const { return AnimModeList().Find(hash); }
    int GeoAnimModeIndex(unsigned int hash) const { return AnimModeList().FindIndex(hash); }
    EXGeoAnimSetHeader *GeoAnimSetHeader(unsigned int hash) const { return AnimSetList().Find(hash); }
    EXGeoParticleHeader *GeoParticleHeader(unsigned int hash) const { return ParticleList().Find(hash); }
    EXGeoParticle *GeoParticle(unsigned int hash) const { return ParticleList().Find(hash)->Object(0); }
    EXGeoSwooshHeader *GeoSwooshHeader(unsigned int hash) const { return SwooshList().Find(hash); }
    EXGeoSwoosh *GeoSwoosh(unsigned int hash) const { return SwooshList().Find(hash)->Object(0); }
    EXGeoSpreadSheetHeader *GeoSpreadSheetHeader(unsigned int hash) const { return SpreadSheetList().Find(hash); }
    EXGeoSpreadSheet *GeoSpreadSheet(unsigned int hash) const { return SpreadSheetList().Find(hash)->Object(0); }
    EXGeoFontHeader *GeoFontHeader(unsigned int hash) const { return FontList().Find(hash); }
    EXGeoFont *GeoFont(unsigned int hash) const { return FontList().Find(hash)->Object(0); }
    EXGeoForceFeedbackHeader *GeoForceFeedbackHeader(unsigned int hash) const { return ForceFeedbackList().Find(hash); }
    EXGeoMaterialHeader *GeoMaterialHeader(unsigned int hash) const { return MaterialList().Find(hash); }
    EXGeoTextureHeader *GeoTextureHeader(unsigned int hash) const { return TextureList().Find(hash); }
    EXGeoTexture *GeoTexture(unsigned int hash) const { return TextureList().Find(hash)->Object(0); }
    int GeoTextureIndex(unsigned int hash) const { return TextureList().FindIndex(hash); }

    void Debug_OnStats(EXGeoDebugStats &);
    unsigned int GeoObjectStructSize(unsigned int) const;
    EXGeoObjectArray *GeoObjectArray(unsigned int) const;
    void DeLoadDebugFile();
    EXGeoDebugHeader *DebugHeader() const;
    EXGeoBaseDebug *GetDebug(unsigned int) const;
    unsigned int DebugFileSize() const;
    void DeloadTextureArray();
    void AnimateTextureArray() const;
    unsigned int GeoObjectHashIndex(unsigned int) const;
    signed char IsSectionGroupLoaded(const EXGeoSectionFile *, int) const;
    signed char LoadDebugFile();
    const char *DebugFileName() const;
    void OnDelete(unsigned int, unsigned int);
    void OnSectionDelete();
    void InitialiseTextureArray();
    void UpdateTextureArray() const;
    EXTexture *Texture(unsigned int) const;
    void UpdateExternalReferenceObjects() const;
    void UpdateExternalReferences() const;
    void GetAnimMaxBoundsBox(EXBoundsBox &) const;
    void GetScriptMaxBoundsBox(EXBoundsBox &) const;
    EXGeoTextItem *FindTextItem(unsigned int) const;
    EXGeoCommonObject *RefPointer(unsigned int) const;
    EXGeoObjectHeader *GeoObjectHeader(unsigned int) const;
    unsigned int GeoObjectHashCode(unsigned int) const;
    EXGeoBaseDebug *GetObjectDebug(unsigned int) const;
    void *GeoObject(unsigned int) const;
    unsigned int IsSectionLoaded(unsigned int, signed char) const;
    void *SectionLoadAddr(unsigned int, signed char) const;
    signed char UpdateDeloadSectionFiles();
    void SectionGroupUsage(unsigned int &, unsigned int, EXu32Array &) const;
    signed char DeLoadSectionFile(unsigned int) const;
    EXGeoBaseDebug *GetDebug(const EXGeoObjectHeader *) const;
    void UpdatePtrs(unsigned int);
    signed char DeLoadTextSectionFile(unsigned int, unsigned char) const;
    signed char LoadSectionFile(unsigned int, signed char, unsigned int);
    void DeLoadSectionFiles();
    void Deload();
    signed char LoadTextSectionFile(unsigned int, unsigned char, signed char, unsigned int);
    signed char UpdateSectionFiles(signed char);
    void Initialise(EXGeoFile *);
    void InitialiseExternalReferences();
protected:
    void _DeLoadSectionFile(EXGeoSectionHeader *) const;
    signed char _DeLoadSectionFiles();
    static float ms_GlobalAnimTexSpeedScale;
public:
    u32 m_Magic;                    // 0x00
    u32 m_HashCode;                 // 0x04
    u32 m_Version;                  // 0x08
    u32 m_Flags;                    // 0x0c
    u32 m_Time;                     // 0x10
    u32 m_FileSize;                 // 0x14
    u32 m_BaseFileSize;             // 0x18
    u32 m_1c;
    u32 m_DebugStart;               // 0x20
    u32 m_DebugEnd;                 // 0x24
    EXGeoDebugFile *m_pDebugFile;   // 0x28
    EXGeoFile *m_pGeoFile;          // 0x2c
    u32 m_NumTextures;                  // 0x30
    EXTextureArray *m_pTextureArray;    // 0x34
    EXGeoFileDataBlock *m_pDataBlock;   // 0x38
    u32 m_3c;
    EXGeoCommonArray<EXGeoSectionHeader> m_Sections;            // 0x40
    EXGeoCommonArray<EXGeoRefPointerHeader> m_RefPointers;      // 0x48
    EXGeoCommonArray<EXGeoEntityHeader> m_Entities;             // 0x50
    EXGeoCommonArray<EXGeoAnimHeader> m_Anims;                  // 0x58
    EXGeoCommonArray<EXGeoAnimSkinHeader> m_AnimSkins;          // 0x60
    EXGeoCommonArray<EXGeoScriptHeader> m_Scripts;              // 0x68
    EXGeoCommonArray<EXGeoMapHeader> m_Maps;                    // 0x70
    EXGeoCommonArray<EXGeoAnimModeHeader> m_AnimModes;          // 0x78
    EXGeoCommonArray<EXGeoAnimSetHeader> m_AnimSets;            // 0x80
    EXGeoCommonArray<EXGeoParticleHeader> m_Particles;          // 0x88
    EXGeoCommonArray<EXGeoSwooshHeader> m_Swooshes;             // 0x90
    EXGeoCommonArray<EXGeoSpreadSheetHeader> m_SpreadSheets;    // 0x98
    EXGeoCommonArray<EXGeoFontHeader> m_Fonts;                  // 0xa0
    EXGeoCommonArray<EXGeoForceFeedbackHeader> m_ForceFeedbacks;  // 0xa8
    EXGeoCommonArray<EXGeoMaterialHeader> m_Materials;          // 0xb0
    EXGeoCommonArray<EXGeoTextureHeader> m_Textures;            // 0xb8
    EXRelArray<u16> m_AnimTextures;                             // 0xc0 animated texture indices
    EXRelArray<u32> m_ExternalRefs;                             // 0xc8 external texture references
    EXRelArray<u32> m_ExternalFiles;                            // 0xd0 hashes of geo files referenced
};

// Generic list view used by the non-template workers.
class EXGeoObjectArray : public EXGeoCommonArray<EXGeoObjectHeader> {};
#endif
