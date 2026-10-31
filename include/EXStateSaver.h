#ifndef EXSTATESAVER_H
#define EXSTATESAVER_H
#include "types.h"
#include "EXString.h"

// Section/key state persistence (retail build: saving is stubbed out, restore returns defaults).
class EXStateSaver {
public:
    virtual ~EXStateSaver();
    static void SetStateFile(const EXString &file) { m_StateFile = file; }  // copy emitted in App.obj
    void SetSection(const EXString &section);
    void Save(const EXString &key, const EXString &value);
    void Save(const EXString &key, u32 value);
    void Savef(const EXString &key, float value);
    void Save(const EXString &key);
    EXString Restore(const EXString &key, const EXString &def) const;
    u32 Restore(const EXString &key, u32 def) const;
    float Restoref(const EXString &key, float def) const;
    EXStateSaver(const EXString &section);
protected:
    static s8 LoadFile();
    static s8 SaveFile();
    static s8 m_Loaded;
    static EXString m_StateFile;
    static EXStringArray m_Data;
private:
    EXString m_Section;   // 0x04 "[name]"
    int m_SectionIndex;   // 0x08 index in m_Data, -1 if absent
    s8 m_Unk0C;           // 0x0c
};
#endif
