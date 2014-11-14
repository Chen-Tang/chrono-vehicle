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
// Authors: Radu Serban, Justin Madsen, Daniel Melanz, Alessandro Tasora
// =============================================================================
//
// Articulated vehicle model. 
// Can be constructed either with solid-axle or with multi-link suspensions.
// Always uses a articulated rack-pinion steering and a 2WD driveline model.
//
// =============================================================================

#ifndef ARTICULATED_VEHICLE_H
#define ARTICULATED_VEHICLE_H

#include "core/ChCoordsys.h"
#include "physics/ChSystem.h"

#include "subsys/ChVehicle.h"
#include "subsys/suspension/ChSolidAxle.h"

#include "models/ModelDefs.h"
#include "models/articulated/Articulated_Wheel.h"
#include "models/articulated/Articulated_RackPinion.h"
#include "models/articulated/Articulated_Driveline2WD.h"
#include "models/articulated/Articulated_BrakeSimple.h"

class Articulated_Vehicle : public chrono::ChVehicle
{
public:

  Articulated_Vehicle(const bool        fixed,
                  SuspensionType    suspType,
                  VisualizationType wheelVis);

  ~Articulated_Vehicle() {}

  virtual int GetNumberAxles() const { return 2; }

  virtual chrono::ChCoordsys<> GetLocalDriverCoordsys() const { return m_driverCsys; }

  double GetSpringForce(const chrono::ChWheelID& wheel_id) const;
  double GetSpringLength(const chrono::ChWheelID& wheel_id) const;
  double GetSpringDeformation(const chrono::ChWheelID& wheel_id) const;

  double GetShockForce(const chrono::ChWheelID& wheel_id) const;
  double GetShockLength(const chrono::ChWheelID& wheel_id) const;
  double GetShockVelocity(const chrono::ChWheelID& wheel_id) const;

  virtual void Initialize(const chrono::ChCoordsys<>& chassisPos);
  virtual void Update(double                      time,
                      double                      steering,
                      double                      braking,
                      double                      powertrain_torque,
                      const chrono::ChTireForces& tire_forces);

  // Log debugging information
  void LogHardpointLocations(); /// suspension hardpoints at design
  void DebugLog(int what);      /// shock forces and lengths, constraints, etc.

private:

  SuspensionType m_suspType;

  chrono::ChSharedPtr<Articulated_Wheel> m_front_right_wheel;
  chrono::ChSharedPtr<Articulated_Wheel> m_front_left_wheel;
  chrono::ChSharedPtr<Articulated_Wheel> m_rear_right_wheel;
  chrono::ChSharedPtr<Articulated_Wheel> m_rear_left_wheel;

  chrono::ChSharedPtr<Articulated_BrakeSimple> m_front_right_brake;
  chrono::ChSharedPtr<Articulated_BrakeSimple> m_front_left_brake;
  chrono::ChSharedPtr<Articulated_BrakeSimple> m_rear_right_brake;
  chrono::ChSharedPtr<Articulated_BrakeSimple> m_rear_left_brake;

  // Chassis mass properties
  static const double             m_chassisMass;
  static const chrono::ChVector<> m_chassisCOM;
  static const chrono::ChVector<> m_chassisInertia;

  // Driver local coordinate system
  static const chrono::ChCoordsys<> m_driverCsys;
};


#endif
