#ifndef VEHICLESUBCLASS_H
#define VEHICLESUBCLASS_H
#include "types.h"

class Vehicle;

// Base of the per-vehicle components (VehiclePos, VehicleMove, VehicleSfx, VehicleInLane ...):
// a back pointer to the owning vehicle.
class VehicleSubClass {
public:
    Vehicle *getVehicle() const { return m_pVehicle; }
    void setVehicle(Vehicle *p) { m_pVehicle = p; }
    Vehicle *m_pVehicle;    // 0x00
};
#endif
