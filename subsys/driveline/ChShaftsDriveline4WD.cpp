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
// Authors: Alessandro Tasora, Radu Serban
// =============================================================================
//
// 4WD driveline model template based on ChShaft objects.
//
// =============================================================================

#include "physics/ChSystem.h"

#include "subsys/driveline/ChShaftsDriveline4WD.h"

namespace chrono {


// -----------------------------------------------------------------------------
// dir_motor_block specifies the direction of the driveshaft, i.e. the input of
// the conic gear pair, in chassis local coords.
//
// dir_axle specifies the direction of the axle, i.e. the output of the conic
// conic gear pair, in chassis local coords. This is needed because ChShaftsBody
// could transfer pitch torque to the chassis.
// -----------------------------------------------------------------------------
ChShaftsDriveline4WD::ChShaftsDriveline4WD(ChVehicle*         car,
                                           const ChVector<>&  dir_motor_block,
                                           const ChVector<>&  dir_axle)
: ChDriveline(car, ChDriveline::AWD),
  m_dir_motor_block(dir_motor_block),
  m_dir_axle(dir_axle)
{
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChShaftsDriveline4WD::Initialize(ChSharedPtr<ChBody>  chassis,
                                      ChSharedPtr<ChShaft> axle_front_L,
                                      ChSharedPtr<ChShaft> axle_front_R,
                                      ChSharedPtr<ChShaft> axle_rear_L,
                                      ChSharedPtr<ChShaft> axle_rear_R)
{
  assert(chassis);
  assert(axle_front_L);
  assert(axle_front_R);
  assert(axle_rear_L);
  assert(axle_rear_R);
  assert(chassis->GetSystem());

  ChSystem* my_system = chassis->GetSystem();


  // CREATE  the driveshaft, a 1 d.o.f. object with rotational inertia which 
  // represents the connection of the driveline to the transmission box.
  m_driveshaft = ChSharedPtr<ChShaft>(new ChShaft);
  m_driveshaft->SetInertia(GetDriveshaftInertia());
  my_system->Add(m_driveshaft);

  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the shaft that connecting central differential to front
  // differential.
  m_front_shaft = ChSharedPtr<ChShaft>(new ChShaft);
  m_front_shaft->SetInertia(GetToFrontDiffShaftInertia());
  my_system->Add(m_front_shaft);

  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the shaft that connecting central differential to rear
  // differential.
  m_rear_shaft = ChSharedPtr<ChShaft>(new ChShaft);
  m_rear_shaft->SetInertia(GetToRearDiffShaftInertia());
  my_system->Add(m_rear_shaft);

  // CREATE the central differential, i.e. an epicycloidal mechanism that
  // connects three rotating members. This class of mechanisms can be simulated
  // using ChShaftsPlanetary; a proper 'ordinary' transmission ratio t0 must be
  // assigned according to Willis formula. The case of the differential is
  // simple: t0=-1.
  m_central_differential = ChSharedPtr<ChShaftsPlanetary>(new ChShaftsPlanetary);
  m_central_differential->Initialize(m_driveshaft, // the carrier
                                     m_rear_shaft,
                                     m_front_shaft);
  m_rear_differential->SetTransmissionRatioOrdinary(GetCentralDifferentialRatio());
  my_system->Add(m_rear_differential);

  // ---Rear differential and axles:

  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the inertia of the rotating box of the differential.
  m_rear_differentialbox = ChSharedPtr<ChShaft>(new ChShaft);
  m_rear_differentialbox->SetInertia(GetRearDifferentialBoxInertia());
  my_system->Add(m_rear_differentialbox);

  // CREATE an angled gearbox, i.e a transmission ratio constraint between two
  // non parallel shafts. This is the case of the 90� bevel gears in the
  // differential. Note that, differently from the basic ChShaftsGear, this also
  // provides the possibility of transmitting a reaction torque to the box
  // (the truss).
  m_rear_conicalgear = ChSharedPtr<ChShaftsGearboxAngled>(new ChShaftsGearboxAngled);
  m_rear_conicalgear->Initialize(m_rear_shaft,
                                 m_rear_differentialbox,
                                 chassis,
                                 m_dir_motor_block,
                                 m_dir_axle);
  m_rear_conicalgear->SetTransmissionRatio(GetRearConicalGearRatio());
  my_system->Add(m_rear_conicalgear);

  // CREATE a differential, i.e. an apicycloidal mechanism that connects three 
  // rotating members. This class of mechanisms can be simulated using 
  // ChShaftsPlanetary; a proper 'ordinary' transmission ratio t0 must be
  // assigned according to Willis formula. The case of the differential is
  // simple: t0=-1.
  m_rear_differential = ChSharedPtr<ChShaftsPlanetary>(new ChShaftsPlanetary);
  m_rear_differential->Initialize(m_rear_differentialbox, // the carrier
                                  axle_rear_L,
                                  axle_rear_R);
  m_rear_differential->SetTransmissionRatioOrdinary(GetRearDifferentialRatio());
  my_system->Add(m_rear_differential);

  // ---Front differential and axles:

  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the inertia of the rotating box of the differential.
  m_front_differentialbox = ChSharedPtr<ChShaft>(new ChShaft);
  m_front_differentialbox->SetInertia(GetRearDifferentialBoxInertia());
  my_system->Add(m_front_differentialbox);

  // CREATE an angled gearbox, i.e a transmission ratio constraint between two
  // non parallel shafts. This is the case of the 90� bevel gears in the
  // differential. Note that, differently from the basic ChShaftsGear, this also
  // provides the possibility of transmitting a reaction torque to the box
  // (the truss).
  m_front_conicalgear = ChSharedPtr<ChShaftsGearboxAngled>(new ChShaftsGearboxAngled);
  m_front_conicalgear->Initialize(m_front_shaft,
                                  m_front_differentialbox,
                                  chassis,
                                  m_dir_motor_block,
                                  m_dir_axle);
  m_front_conicalgear->SetTransmissionRatio(GetFrontConicalGearRatio());
  my_system->Add(m_front_conicalgear);

  // CREATE a differential, i.e. an apicycloidal mechanism that connects three 
  // rotating members. This class of mechanisms can be simulated using 
  // ChShaftsPlanetary; a proper 'ordinary' transmission ratio t0 must be
  // assigned according to Willis formula. The case of the differential is
  // simple: t0=-1.
  m_front_differential = ChSharedPtr<ChShaftsPlanetary>(new ChShaftsPlanetary);
  m_front_differential->Initialize(m_front_differentialbox, // the carrier
                                   axle_front_L,
                                   axle_front_R);
  m_front_differential->SetTransmissionRatioOrdinary(GetFrontDifferentialRatio());
  my_system->Add(m_front_differential);
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
double ChShaftsDriveline4WD::GetWheelTorque(ChWheelId which) const
{
  switch (which) {
  case FRONT_LEFT:
    return -m_front_differential->GetTorqueReactionOn2();
  case FRONT_RIGHT:
    return -m_front_differential->GetTorqueReactionOn3();
  case REAR_LEFT:
    return -m_rear_differential->GetTorqueReactionOn2();
  case REAR_RIGHT:
    return -m_rear_differential->GetTorqueReactionOn3();
  default:
    return -1;  // should not happen
  }
}


} // end namespace chrono
