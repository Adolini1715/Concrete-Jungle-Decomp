#ifndef EXGEOFILE_H
#define EXGEOFILE_H
#include "types.h"
#include "EXFile.h"
#include "EXGeoHeader.h"

#include "EXMemory.h"
#include "EXArray.h"
#include "EXBit128.h"

// A loaded .edb file (sizeof 0x2c). The header lives in the file's memory block.
class EXGeoFile : public EXFile {
    EX_DECLARE_DYNAMIC(EXGeoFile)
public:
    virtual ~EXGeoFile();
    virtual s8 OnLoad();
    virtual s8 OnLoaded();
    virtual s8 OnKill();
    EXGeoFile();
    static EXGeoFile *CreateObject();
    EXGeoHeader *GeoHeader() const { return this ? m_pGeoHeader : 0; }
    s8 IsLoaded() const { return m_pGeoHeader != 0; }  // App.obj copy
    unsigned int CheckCode() const { return m_CheckCode; }
    static signed char AutoLoadGroupFiles() { return m_AutoLoadGroupFiles; }
    void SetDeloadTime(signed char b) { m_DeloadTime = b; }
    short &DeloadSubFiles() { return m_DeloadSubFiles; }
    void SetForceUpdatePtrs(signed char b) { m_ForceUpdatePtrs = b; }
    signed char ForceUpdatePtrs() const { return m_ForceUpdatePtrs; }
    void SetGeoHeader(EXGeoHeader *p) { m_pGeoHeader = p; }
    void UpdateCheckCode() { m_CheckCode++; }
    static void EnableAutoLoadGroupFiles(signed char b) { m_AutoLoadGroupFiles = b; }
    static void EnableFailSafeLoadGeoFiles(signed char b) { m_FailSafeLoadFiles = b; }
    static signed char AllocCallbackFunc(void *, EXMcmds);
    static signed char DeLoadGeoFile(EXGeoFile *, signed char);
    static void DeloadAllGeoFiles();
    void OnDelete(unsigned int);
    void UpdatePtrs(unsigned int, signed char);
    static EXGeoFile *FailSafeLoadGeoFile(unsigned int, signed char);
    static EXGeoFile *LoadGeoFile(unsigned int, signed char, unsigned int);
    static EXGeoFile *LoadGeoFile(EXGeoFile *, signed char, unsigned int);
    static signed char LoadGeoObjectSectionFile(EXGeoFile *, unsigned int);
    static EXGeoFile *GetGeoFile(unsigned int);
    void Initialise();
    static signed char DeLoadGeoFile(unsigned int, signed char);
    static signed char LoadGeoObjectSectionFile(unsigned int, unsigned int);
    static void UpdateFiles(signed char);
protected:
    static signed char m_AutoLoadGroupFiles;
    static signed char m_FailSafeLoadFiles;
private:
    signed char _DeLoadFile(signed char);
    void AddFileToUpdateList();
    signed char _LoadFile(signed char, unsigned int);
    static EXGeoFile *_GetGeoFile(unsigned int);
    void Update(signed char);
public:
    EXGeoHeader *m_pGeoHeader;  // 0x20
    u32 m_CheckCode;            // 0x24
    s16 m_DeloadSubFiles;       // 0x28
    s8 m_DeloadTime;            // 0x2a
    s8 m_ForceUpdatePtrs;       // 0x2b
};

// Section numbers a section depends on: count then u16 indices.
// Runtime block hung off the header (EXGeoHeader +0x38).
struct EXGeoFileDataBlock {
    EXGeoFileDataBlock(const EXGeoHeader *pHdr);
    ~EXGeoFileDataBlock() {}
    ex::EXBit128 &MapZoneSections() { return m_MapZoneSections; }
    float m_AnimTexSpeedScale;          // 0x00
    ex::EXBit128 m_MapZoneSections;            // 0x04
    u32 m_14[4];                        // 0x14
    EXPtrArray<EXGeoFile> m_ExtFiles;   // 0x24 geo files the external references point into
};

struct EXGeoSectionGroupList {
    u16 m_Count;
    u16 m_Sections[1];
};

// Header at the start of a loaded section file.
class EXGeoSectionFileHeader {
public:
    EXGeoSectionGroupList *ChildSectionList() const { return m_ChildSections; }
    EXGeoSectionGroupList *ParentSectionList() const { return m_ParentSections; }
    ex::EXABSOFFSET32 SplitOffset() const { return m_SplitOffset; }
    unsigned int &UsageFlags() { return m_UsageFlags; }
    ex::EXABSOFFSET32 m_SplitOffset;    // 0x00
    u32 m_UsageFlags;                   // 0x04
    ex::EXRelPtr<EXGeoSectionGroupList> m_ChildSections;    // 0x08
    ex::EXRelPtr<EXGeoSectionGroupList> m_ParentSections;   // 0x0c
};

// One loadable section of a geo file (sizeof 0x2c).
class EXGeoSectionFile : public EXFile {
public:
    EXGeoSectionFile(EXGeoFile *pFile, const EXGeoSectionHeader *pHdr, short index);
    virtual ~EXGeoSectionFile();
    virtual s8 OnLoad();
    virtual s8 OnLoaded();
    virtual s8 OnKill();
    static signed char AllocCallbackFunc(void *, EXMcmds);
    EXGeoSectionFileHeader *GeoSectionHeader() const { return this ? (EXGeoSectionFileHeader *)m_pMem : 0; }
    void SetDeloadTime(signed char b) { m_DeloadTime = b; }
    signed char DeloadTime() const { return m_DeloadTime; }
    void SetGroupFilesLoaded(signed char b) { m_GroupFilesLoaded = b; }
    signed char GroupFilesLoaded() const { return m_GroupFilesLoaded; }
    void SetObjectsDelete(signed char b) { m_ObjectsDelete = b; }
    signed char ObjectsDelete() const { return m_ObjectsDelete; }
    int m_Index;                // 0x20
    EXGeoFile *m_pGeoFile;      // 0x24
    s8 m_DeloadTime;            // 0x28
    s8 m_GroupFilesLoaded;      // 0x29
    s8 m_ObjectsDelete;         // 0x2a
};

// Debug data file of a geo file.
class EXGeoDebugFile : public EXFile {
public:
    EXGeoDebugFile(const EXFILEINFO *pInfo, u32 length, u32 seekPos) : EXFile(pInfo, length, seekPos) {}
    virtual ~EXGeoDebugFile();
    virtual s8 OnLoad();
    virtual s8 OnLoaded();
};

inline EXFile *EXGeoSectionHeader::File() const { return m_pFile; }
#endif
