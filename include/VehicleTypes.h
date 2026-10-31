#ifndef VEHICLETYPES_H
#define VEHICLETYPES_H
#include "types.h"
#include "EXArray.h"
#include "EXGeoFile.h"
#include "EXStdFuncs.h"

// Vehicle type ids (enum Vcl::__unnamed in the map).
namespace Vcl {
enum {
    GRAFICAR = 0, VERSION2 = 1, MODERN = 2, FIREENGINE = 3, POLICE = 4, SPORTS2 = 5, GUNSHIP = 6,
    HUMMER = 7, CHEMTRUCK = 8, MATADOR = 9, VERSION2LOW = 15, MODERNLOW = 16, SPORTS2LOW = 17,
    NUM_TYPES = 18
};
}

// Static per-type description (VehicleTypes.obj). sizeof 0x94.
class VehicleTypeBase {
public:
    VehicleTypeBase()
    {
        m_yOffset = 0.0f;
        m_frontWheelAdjustY = 0.0f;
        m_rearWheelAdjustY = 0.0f;
        m_shadowYOffset = 0.0f;
        m_subFile = 0;
        for (u32 i = 0; i < 17; i++) m_attachments[i] = 0;
    }
    float getZDistToFront() const { return m_zDistToFront; }
    float getZDistToRear() const { return m_zDistToRear; }
    float getRadius() { return max(getZDistToFront(), getZDistToRear()); }
    float getZDim() const { return m_zDim; }
    unsigned char getSubFile() const { return m_subFile; }
    unsigned int getEntityHash() const { return m_entityHash; }
    float getYDim() const { return m_yDim; }
    unsigned char getType() const { return m_type; }
    float getBodyHeightAdjust() const { return m_bodyHeightAdjust; }
    float getFrontWheelAdjustZ() const { return m_frontWheelAdjustZ; }
    float getRearWheelAdjustZ() const { return m_rearWheelAdjustZ; }
    float getFrontWheelAdjustY() const { return m_frontWheelAdjustY; }
    float getRearWheelAdjustY() const { return m_rearWheelAdjustY; }
    float getYOffset() const { return m_yOffset; }
    float getShadowYOffset() const { return m_shadowYOffset; }
    signed char isAirborneType() const { return m_type == Vcl::GUNSHIP; }
    EXGeoFile *geoFile() const { return EXGeoFile::LoadGeoFile(0x1000026, 0, 0); }
    unsigned int getAttachmentHash(int i) { return m_attachments[i]; }
    float getXDim() const { return m_xDim; }
    float getPhys_wheelBase() const { return m_phys_wheelBase; }
    float getPhys_mass() const { return m_phys_mass; }
    float getPhys_wheelLength() const { return m_phys_wheelLength; }
    unsigned char getSfxCarType() const { return m_sfxCarType; }
    EXGeoHeader *geoHeader() const { return geoFile()->GeoHeader(); }
    signed char loadSubFile(signed char);
    void deLoadSubFile();

    u8 m_type;                              // 0x00
    u8 m_subFile;                           // 0x01
    u8 m_sfxCarType;                        // 0x02
    u32 m_entityHash;                       // 0x04
    float m_zDistToFront;                   // 0x08
    float m_zDistToRear;                    // 0x0c
    float m_bodyHeightAdjust;               // 0x10
    float m_yOffset;                        // 0x14
    float m_shadowYOffset;                  // 0x18
    float m_frontWheelAdjustZ;              // 0x1c
    float m_frontWheelAdjustY;              // 0x20
    float m_rearWheelAdjustZ;               // 0x24
    float m_rearWheelAdjustY;               // 0x28
    float m_xDim;                           // 0x2c
    float m_yDim;                           // 0x30
    float m_zDim;                           // 0x34
    float m_phys_wheelBase;                 // 0x38
    float m_f3c;                            // 0x3c TODO name
    float m_f40;                            // 0x40 TODO name
    float m_phys_mass;                      // 0x44
    float m_phys_wheelLength;               // 0x48
    float m_f4c;                            // 0x4c TODO name
    EXFixedArray<unsigned int, 17> m_attachments;  // 0x50
};

unsigned char vehicleTypeFromHash(unsigned int);
VehicleTypeBase *vehicleTypeObjFromType(unsigned char);
void resetVehicleSubFiles();
void updateVehicleSubFiles();

#endif
