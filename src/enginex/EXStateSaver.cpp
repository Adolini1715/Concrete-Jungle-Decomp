#include "EXStateSaver.h"
#include "EXMemory.h"

s8 EXStateSaver::m_Loaded;
EXString EXStateSaver::m_StateFile;
EXStringArray EXStateSaver::m_Data(0, -1);

void EXStateSaver::SetSection(const EXString &section)
{
    m_Section.Format("[%s]", (const char *)section);
    m_SectionIndex = -1;
    for (int i = 0; i < m_Data.Size(); i++) {
        if (m_Data[i] == m_Section) {
            m_SectionIndex = i;
            return;
        }
    }
}

void EXStateSaver::Save(const EXString &, const EXString &) {}
void EXStateSaver::Save(const EXString &, u32) {}
void EXStateSaver::Savef(const EXString &, float) {}
void EXStateSaver::Save(const EXString &) {}

EXString EXStateSaver::Restore(const EXString &, const EXString &def) const
{
    return def;
}

u32 EXStateSaver::Restore(const EXString &, u32 def) const { return def; }
float EXStateSaver::Restoref(const EXString &, float def) const { return def; }

s8 EXStateSaver::LoadFile()
{
    m_Loaded = 1;
    return 1;
}

s8 EXStateSaver::SaveFile() { return 1; }

EXStateSaver::EXStateSaver(const EXString &section)
{
    if (!m_Loaded) LoadFile();
    SetSection(section);
    m_Unk0C = 0;
}

EXStateSaver::~EXStateSaver() {}
