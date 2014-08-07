// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban, Justin Madsen
// =============================================================================
//
// HMMWV 9-body vehicle model...
//
// =============================================================================

#ifndef HMMWV9_VEHICLE_H
#define HMMWV9_VEHICLE_H

#include "core/ChCoordsys.h"
#include "physics/ChSystem.h"

#include "subsys/ChVehicle.h"

#include "HMMWV9.h"
#include "HMMWV9_DoubleWishbone.h"
#include "HMMWV9_Wheel.h"
#include "HMMWV9_Powertrain.h"

namespace hmmwv9 {

// Forward reference
class HMMWV9_Powertrain;
class HMMWV9_Powertrain_NEW;

class HMMWV9_Vehicle : public chrono::ChVehicle {
public:

  HMMWV9_Vehicle(chrono::ChSystem&            my_system,
                 const chrono::ChCoordsys<>&  chassisPos,
                 const bool                   fixed = false,
                 VisualizationType            chassisVis = NONE,
                 VisualizationType            wheelVis = PRIMITIVES);

  ~HMMWV9_Vehicle();

  virtual const chrono::ChVector<>& GetWheelPos(chrono::ChWheelId which) const;
  virtual const chrono::ChQuaternion<>& GetWheelRot(chrono::ChWheelId which) const;
  virtual double GetWheelAngSpeed(chrono::ChWheelId which);

  virtual void Update(double time, double throttle, double steering);

  // Access to private static members
  static const std::string& ChassisMeshName() { return m_chassisMeshName; }
  static const std::string& ChassisMeshFile() { return m_chassisMeshFile; }

public:

  chrono::ChSharedPtr<HMMWV9_DoubleWishboneFront>   m_front_right_susp;
  chrono::ChSharedPtr<HMMWV9_DoubleWishboneFront>   m_front_left_susp;
  chrono::ChSharedPtr<HMMWV9_DoubleWishboneRear>    m_rear_right_susp;
  chrono::ChSharedPtr<HMMWV9_DoubleWishboneRear>    m_rear_left_susp;

  //chrono::ChSharedPtr<HMMWV9_Powertrain>  m_powertrain;
	chrono::ChSharedPtr<HMMWV9_Powertrain_NEW>  m_powertrain;

private:

  // Chassis visualization mesh
  static const std::string m_chassisMeshName;
  static const std::string m_chassisMeshFile;

  // Chassis mass properties
  static const double  m_chassisMass;
  static const chrono::ChVector<>  m_chassisInertia;

};


} // end namespace hmmwv9


#endif