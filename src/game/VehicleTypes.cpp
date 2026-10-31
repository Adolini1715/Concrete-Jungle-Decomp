#include "FEText.h"
#include "VehicleTypes.h"
#include "Traffic.h"
#include "TrafficSystem.h"

// Sub-file reference counts and load states (0 none, 1 loading, 2 loaded), per sub-file.
static EXFixedArray<short, 19> s_subFileRefs;
static EXFixedArray<unsigned char, 19> s_subFileState;

inline unsigned char vehicleSubFileFromType(unsigned char type)
{
    switch (type) {
    case Vcl::GRAFICAR: return 1;
    case Vcl::FIREENGINE: return 4;
    case Vcl::POLICE: return 5;
    case Vcl::GUNSHIP: return 7;
    case Vcl::HUMMER: return 8;
    case Vcl::CHEMTRUCK: return 9;
    case Vcl::MATADOR: return 10;
    case 10: return 11;
    case 11: return 12;
    case 12: return 13;
    case 13: return 14;
    case 14: return 15;
    case Vcl::VERSION2: case Vcl::VERSION2LOW: return 2;
    case Vcl::MODERN: case Vcl::MODERNLOW: return 3;
    case Vcl::SPORTS2: case Vcl::SPORTS2LOW: return 6;
    }
    return 0;
}

unsigned char vehicleTypeFromHash(unsigned int hash)
{
    switch (hash) {
    case 0x4a000001: return 0;
    case 0x4a000002: return 1;
    case 0x4a000003: return 2;
    case 0x4a000004: return 3;
    case 0x4a000005: return 4;
    case 0x4a000006: return 5;
    case 0x4a000007: return 6;
    case 0x4a000008: return 7;
    case 0x4a000009: return 8;
    case 0x4a00000a: return 9;
    case 0x4a00000b: return 10;
    case 0x4a00000c: return 11;
    case 0x4a00000d: return 12;
    case 0x4a00000e: return 13;
    case 0x4a00000f: return 14;
    case 0x4a000010: return 15;
    case 0x4a000011: return 16;
    case 0x4a000012: return 17;
    }
    return 0;
}

// NEAR (all 13 type constructors): ~250 bytes differ each. Retail keeps every store in source order
// and stores m_xDim/m_yDim/m_zDim twice (no dead-store elimination); ours drops the first set and
// interleaves the rest. Something in the original (aliasing through a sub-object or helper?) blocks
// both optimisations; not found yet.
class Graficar : public VehicleTypeBase {
public:
    Graficar()
    {
        m_type = Vcl::GRAFICAR;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 0;
        m_entityHash = 0x2000034;
        m_xDim = 2.02f;
        m_yDim = 1.4f;
        m_zDim = 3.882f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.64f;
        m_f4c = 0.2f;
        m_bodyHeightAdjust = -0.09f;
        m_frontWheelAdjustZ = -0.08f;
        m_frontWheelAdjustY = -0.04f;
        m_rearWheelAdjustZ = -0.03f;
        m_rearWheelAdjustY = -0.06f;
        m_xDim = 2.02f;
        m_yDim = 1.53f;
        m_zDim = 3.882f;
        m_zDistToFront = 1.941f;
        m_zDistToRear = 1.941f;
        m_yOffset = -0.24f;
        m_f40 = 1.437f;
        m_f3c = 1.492f;
        m_shadowYOffset = -0.03f;
        m_phys_wheelBase = 2.929f;
        m_attachments[3] = 0x2000035;
        m_attachments[2] = 0x2000035;
        m_attachments[1] = 0x2000035;
        m_attachments[0] = 0x2000035;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0x200003a;
        m_attachments[16] = 0x200011e;
        m_attachments[15] = 0x200011e;
    }
};

class Version2 : public VehicleTypeBase {
public:
    Version2()
    {
        m_type = Vcl::VERSION2;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x2000022;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = -0.19f;
        m_frontWheelAdjustZ = -0.08f;
        m_frontWheelAdjustY = -0.18f;
        m_rearWheelAdjustZ = -0.03f;
        m_rearWheelAdjustY = -0.2f;
        m_xDim = 2.0073f;
        m_yDim = 1.4944f;
        m_zDim = 3.89544f;
        m_zDistToFront = 1.9402f;
        m_zDistToRear = 1.95524f;
        m_yOffset = -0.16f;
        m_shadowYOffset = -0.21f;
        m_f40 = 1.432f;
        m_f3c = 1.494f;
        m_phys_wheelBase = 2.926f;
        m_attachments[3] = 0x2000033;
        m_attachments[2] = 0x2000033;
        m_attachments[1] = 0x2000033;
        m_attachments[0] = 0x2000033;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0x200003c;
        m_attachments[16] = 0x200011e;
        m_attachments[15] = 0x200011e;
    }
};

class Modern : public VehicleTypeBase {
public:
    Modern()
    {
        m_type = Vcl::MODERN;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 2;
        m_entityHash = 0x200001e;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = 0.0f;
        m_frontWheelAdjustZ = -0.04f;
        m_frontWheelAdjustY = -0.06f;
        m_rearWheelAdjustZ = 0.0f;
        m_rearWheelAdjustY = -0.04f;
        m_xDim = 2.0073f;
        m_yDim = 1.2844f;
        m_zDim = 4.355448f;
        m_zDistToFront = 2.1702f;
        m_zDistToRear = 2.18524f;
        m_yOffset = -0.17f;
        m_shadowYOffset = -0.01f;
        m_f40 = 1.432f;
        m_f3c = 1.494f;
        m_phys_wheelBase = 2.926f;
        m_attachments[3] = 0x2000036;
        m_attachments[2] = 0x2000036;
        m_attachments[1] = 0x2000036;
        m_attachments[0] = 0x2000036;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0x2000038;
        m_attachments[16] = 0x200011e;
        m_attachments[15] = 0x200011e;
    }
};

class FireEngine : public VehicleTypeBase {
public:
    FireEngine()
    {
        m_shadowYOffset = 0.0f;
        m_type = Vcl::FIREENGINE;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x2000084;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_f3c = 1.494f;
        m_f40 = 1.432f;
        m_phys_wheelBase = 2.926f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = 0.41f;
        m_frontWheelAdjustZ = -0.02f;
        m_frontWheelAdjustY = 0.0f;
        m_rearWheelAdjustY = 0.0f;
        m_rearWheelAdjustZ = -0.01f;
        m_xDim = 3.307299f;
        m_yDim = 2.3644f;
        m_zDim = 6.415439f;
        m_zDistToFront = 3.190199f;
        m_zDistToRear = 3.225239f;
        m_yOffset = -0.12f;
        m_attachments[3] = 0;
        m_attachments[2] = 0;
        m_attachments[1] = 0;
        m_attachments[0] = 0;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0;
    }
};

class Police : public VehicleTypeBase {
public:
    Police()
    {
        m_type = Vcl::POLICE;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x2000085;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = 0.0f;
        m_frontWheelAdjustZ = -0.04f;
        m_frontWheelAdjustY = -0.06f;
        m_rearWheelAdjustZ = 0.0f;
        m_rearWheelAdjustY = -0.04f;
        m_xDim = 2.0073f;
        m_yDim = 1.2844f;
        m_zDim = 4.355448f;
        m_zDistToFront = 2.1702f;
        m_zDistToRear = 2.18524f;
        m_yOffset = -0.17f;
        m_shadowYOffset = -0.01f;
        m_f40 = 1.432f;
        m_f3c = 1.494f;
        m_phys_wheelBase = 2.926f;
        m_attachments[3] = 0x2000036;
        m_attachments[2] = 0x2000036;
        m_attachments[1] = 0x2000036;
        m_attachments[0] = 0x2000036;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0x2000038;
        m_attachments[16] = 0x200011e;
        m_attachments[15] = 0x200011e;
    }
};

class Sports2 : public VehicleTypeBase {
public:
    Sports2()
    {
        m_shadowYOffset = 0.0f;
        m_type = Vcl::SPORTS2;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x2000086;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_f3c = 1.494f;
        m_f40 = 1.432f;
        m_phys_wheelBase = 2.926f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = 0.05f;
        m_rearWheelAdjustY = 0.05f;
        m_frontWheelAdjustZ = 0.0f;
        m_frontWheelAdjustY = -0.04f;
        m_rearWheelAdjustZ = 0.03f;
        m_xDim = 2.0373f;
        m_yDim = 1.4344f;
        m_zDim = 4.125443f;
        m_zDistToFront = 2.0202f;
        m_zDistToRear = 2.10524f;
        m_yOffset = -0.13f;
        m_attachments[1] = 0x2000087;
        m_attachments[0] = 0x2000087;
        m_attachments[2] = 0x2000088;
        m_attachments[3] = 0;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0;
    }
};

class Gunship : public VehicleTypeBase {
public:
    Gunship()
    {
        m_shadowYOffset = 0.0f;
        m_type = Vcl::GUNSHIP;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x2000024;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_bodyHeightAdjust = 0.0f;
        m_frontWheelAdjustZ = 0.0f;
        m_frontWheelAdjustY = 0.0f;
        m_rearWheelAdjustZ = 0.0f;
        m_rearWheelAdjustY = 0.0f;
        m_yOffset = 0.0f;
        m_phys_wheelBase = 6.7f;
        m_phys_mass = 5e+02f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_zDistToFront = 2.8f;
        m_zDistToRear = 5.4f;
        m_f3c = 2.8f;
        m_f40 = 5.4f;
        m_xDim = 5.0f;
        m_yDim = 3.4f;
        m_zDim = 8.2f;
        m_attachments[3] = 0;
        m_attachments[2] = 0;
        m_attachments[1] = 0;
        m_attachments[0] = 0;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0;
    }
};

class Hummer : public VehicleTypeBase {
public:
    Hummer()
    {
        m_type = Vcl::HUMMER;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x2000094;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_f3c = 1.494f;
        m_f40 = 1.432f;
        m_phys_wheelBase = 2.926f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = -0.04f;
        m_frontWheelAdjustY = -0.04f;
        m_rearWheelAdjustZ = 0.01f;
        m_frontWheelAdjustZ = -0.02f;
        m_rearWheelAdjustY = -0.02f;
        m_xDim = 2.3973f;
        m_yDim = 2.024399f;
        m_zDim = 4.965462f;
        m_zDistToFront = 2.7302f;
        m_zDistToRear = 2.23524f;
        m_yOffset = -0.14f;
        m_shadowYOffset = 0.0f;
        m_attachments[3] = 0x2000036;
        m_attachments[2] = 0x2000036;
        m_attachments[1] = 0x2000036;
        m_attachments[0] = 0x2000036;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0;
    }
};

class ChemTruck : public VehicleTypeBase {
public:
    ChemTruck()
    {
        m_type = Vcl::CHEMTRUCK;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x2000106;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = 0.37f;
        m_frontWheelAdjustZ = -0.02f;
        m_frontWheelAdjustY = 0.12f;
        m_rearWheelAdjustY = 0.12f;
        m_rearWheelAdjustZ = -0.01f;
        m_xDim = 3.687299f;
        m_yDim = 2.1744f;
        m_zDim = 6.335437f;
        m_zDistToFront = 3.110199f;
        m_zDistToRear = 3.225239f;
        m_yOffset = 0.52f;
        m_shadowYOffset = -0.3f;
        m_f40 = 2.191999f;
        m_f3c = 1.774f;
        m_phys_wheelBase = 3.985999f;
        m_attachments[3] = 0x2000107;
        m_attachments[2] = 0x2000107;
        m_attachments[1] = 0x2000107;
        m_attachments[0] = 0x2000107;
        m_attachments[7] = 0x2000111;
        m_attachments[6] = 0x2000111;
        m_attachments[9] = 0x200011a;
        m_attachments[8] = 0x200011a;
        m_attachments[11] = 0x200011d;
        m_attachments[10] = 0x200011d;
        m_attachments[13] = 0x200011b;
        m_attachments[12] = 0x200011b;
        m_attachments[14] = 0x2000121;
        m_attachments[16] = 0x200011e;
        m_attachments[15] = 0x200011e;
    }
};

class Matador : public VehicleTypeBase {
public:
    Matador()
    {
        m_type = Vcl::MATADOR;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x200016a;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = -0.009999f;
        m_frontWheelAdjustZ = -0.02f;
        m_frontWheelAdjustY = -0.09f;
        m_rearWheelAdjustZ = 0.07f;
        m_rearWheelAdjustY = 0.01f;
        m_xDim = 2.2973f;
        m_yDim = 1.4744f;
        m_zDim = 4.385435f;
        m_zDistToFront = 2.200199f;
        m_zDistToRear = 2.185241f;
        m_yOffset = -0.14f;
        m_shadowYOffset = -0.3f;
        m_f40 = 1.481999f;
        m_f3c = 1.564f;
        m_phys_wheelBase = 3.066f;
        m_attachments[1] = 0x2000209;
        m_attachments[0] = 0x2000209;
        m_attachments[3] = 0x200020b;
        m_attachments[2] = 0x200020b;
    }
};

class Version2Low : public VehicleTypeBase {
public:
    Version2Low()
    {
        m_type = Vcl::VERSION2LOW;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x200027c;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = -0.19f;
        m_frontWheelAdjustZ = -0.08f;
        m_frontWheelAdjustY = -0.18f;
        m_rearWheelAdjustZ = -0.03f;
        m_rearWheelAdjustY = -0.2f;
        m_xDim = 2.0073f;
        m_yDim = 1.4944f;
        m_zDim = 3.89544f;
        m_zDistToFront = 1.9402f;
        m_zDistToRear = 1.95524f;
        m_yOffset = -0.16f;
        m_shadowYOffset = -0.21f;
        m_f40 = 1.432f;
        m_f3c = 1.494f;
        m_phys_wheelBase = 2.926f;
        m_attachments[3] = 0;
        m_attachments[2] = 0;
        m_attachments[1] = 0;
        m_attachments[0] = 0;
        m_attachments[7] = 0;
        m_attachments[6] = 0;
        m_attachments[9] = 0;
        m_attachments[8] = 0;
        m_attachments[11] = 0;
        m_attachments[10] = 0;
        m_attachments[13] = 0;
        m_attachments[12] = 0;
        m_attachments[14] = 0;
        m_attachments[16] = 0;
        m_attachments[15] = 0;
    }
};

class ModernLow : public VehicleTypeBase {
public:
    ModernLow()
    {
        m_type = Vcl::MODERNLOW;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 2;
        m_entityHash = 0x200027d;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = 0.0f;
        m_frontWheelAdjustZ = -0.04f;
        m_frontWheelAdjustY = -0.06f;
        m_rearWheelAdjustZ = 0.0f;
        m_rearWheelAdjustY = -0.04f;
        m_xDim = 2.0073f;
        m_yDim = 1.2844f;
        m_zDim = 4.355448f;
        m_zDistToFront = 2.1702f;
        m_zDistToRear = 2.18524f;
        m_yOffset = -0.17f;
        m_shadowYOffset = -0.01f;
        m_f40 = 1.432f;
        m_f3c = 1.494f;
        m_phys_wheelBase = 2.926f;
        m_attachments[3] = 0;
        m_attachments[2] = 0;
        m_attachments[1] = 0;
        m_attachments[0] = 0;
        m_attachments[7] = 0;
        m_attachments[6] = 0;
        m_attachments[9] = 0;
        m_attachments[8] = 0;
        m_attachments[11] = 0;
        m_attachments[10] = 0;
        m_attachments[13] = 0;
        m_attachments[12] = 0;
        m_attachments[14] = 0;
        m_attachments[16] = 0;
        m_attachments[15] = 0;
    }
};

class Sports2Low : public VehicleTypeBase {
public:
    Sports2Low()
    {
        m_shadowYOffset = 0.0f;
        m_type = Vcl::SPORTS2LOW;
        m_subFile = vehicleSubFileFromType(m_type);
        m_sfxCarType = 1;
        m_entityHash = 0x200027e;
        m_xDim = 2.0073f;
        m_yDim = 1.4644f;
        m_zDim = 3.89544f;
        m_f3c = 1.494f;
        m_f40 = 1.432f;
        m_phys_wheelBase = 2.926f;
        m_phys_mass = 2000.0f;
        m_phys_wheelLength = 0.667f;
        m_f4c = 0.207f;
        m_bodyHeightAdjust = 0.05f;
        m_rearWheelAdjustY = 0.05f;
        m_frontWheelAdjustZ = 0.0f;
        m_frontWheelAdjustY = -0.04f;
        m_rearWheelAdjustZ = 0.03f;
        m_xDim = 2.0373f;
        m_yDim = 1.4344f;
        m_zDim = 4.125443f;
        m_zDistToFront = 2.0202f;
        m_zDistToRear = 2.10524f;
        m_yOffset = -0.13f;
        m_attachments[1] = 0;
        m_attachments[0] = 0;
        m_attachments[2] = 0;
        m_attachments[3] = 0;
        m_attachments[7] = 0;
        m_attachments[6] = 0;
        m_attachments[9] = 0;
        m_attachments[8] = 0;
        m_attachments[11] = 0;
        m_attachments[10] = 0;
        m_attachments[13] = 0;
        m_attachments[12] = 0;
        m_attachments[14] = 0;
    }
};

VehicleTypeBase *vehicleTypeObjFromType(unsigned char type)
{
    static Graficar s_graficar;
    static Version2 s_version2;
    static Modern s_modern;
    static FireEngine s_fireEngine;
    static Police s_police;
    static Sports2 s_sports2;
    static Gunship s_gunship;
    static Hummer s_hummer;
    static ChemTruck s_chemTruck;
    static Matador s_matador;
    static Version2Low s_version2Low;
    static ModernLow s_modernLow;
    static Sports2Low s_sports2Low;
    switch (type) {
    case 3: case 6: case 7: case 8: case 10: case 11: case 12: case 13: case 14:
        type = 0;
        break;
    }
    switch (type) {
    case Vcl::GRAFICAR: return &s_graficar;
    case Vcl::VERSION2: return &s_version2;
    case Vcl::MODERN: return &s_modern;
    case Vcl::FIREENGINE: return &s_fireEngine;
    case Vcl::POLICE: return &s_police;
    case Vcl::SPORTS2: return &s_sports2;
    case Vcl::GUNSHIP: return &s_gunship;
    case Vcl::HUMMER: return &s_hummer;
    case Vcl::CHEMTRUCK: return &s_chemTruck;
    case Vcl::MATADOR: return &s_matador;
    case Vcl::VERSION2LOW: return &s_version2Low;
    case Vcl::MODERNLOW: return &s_modernLow;
    case Vcl::SPORTS2LOW: return &s_sports2Low;
    }
    return &s_graficar;
}

void resetVehicleSubFiles()
{
    for (unsigned int i = 1; i < 19; i++) {
        s_subFileRefs[i] = 0;
        s_subFileState[i] = 0;
    }
    s_subFileState[0] = 2;
}

void updateVehicleSubFiles()
{
    EXGeoHeader *pHdr = EXGeoFile::LoadGeoFile(0x1000026, 0, 0)->GeoHeader();
    for (unsigned int i = 1; i <= 18; i++) {
        if (s_subFileState[i] == 1) {
            switch (pHdr->IsSectionLoaded(i + 0x8000000, 1)) {
            case 0:
            case 5:
                pHdr->LoadSectionFile(i + 0x8000000, 1, 0);
                break;
            case 3:
                s_subFileState[i] = 2;
                TrafficGetPopulace().OnSubFileLoaded(i);
                break;
            }
        }
    }
}

signed char VehicleTypeBase::loadSubFile(signed char bAsync)
{
    if (!m_subFile) return 1;
    if (s_subFileRefs[m_subFile] >= 25) return 0;
    if (1.0f > frameTime) bAsync = 0;
    geoHeader()->LoadSectionFile(m_subFile + 0x8000000, bAsync, 0);
    signed char bLoaded = geoHeader()->IsSectionLoaded(m_subFile + 0x8000000, 1) == 3;
    s_subFileRefs[m_subFile]++;
    if (!s_subFileState[m_subFile]) s_subFileState[m_subFile] = bLoaded ? 2 : 1;
    return bLoaded;
}

void VehicleTypeBase::deLoadSubFile()
{
    if (m_subFile && s_subFileRefs[m_subFile] > 0) {
        if (--s_subFileRefs[m_subFile] == 0) {
            geoHeader()->DeLoadSectionFile(m_subFile + 0x8000000);
            s_subFileState[m_subFile] = 0;
        }
    }
}
