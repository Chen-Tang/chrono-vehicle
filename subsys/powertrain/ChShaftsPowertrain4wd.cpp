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
// Powertrain model template based on ChShaft objects.
//
// =============================================================================

#include "physics/ChSystem.h"

#include "subsys/powertrain/ChShaftsPowertrain4wd.h"

namespace chrono {


// -----------------------------------------------------------------------------
// dir_motor_block specifies the direction of the motor block, i.e. the
// direction of the crankshaft, in chassis local coords. This is needed because
// ChShaftsBody could transfer rolling torque to the chassis.
//
// dir_axle specifies the direction of the (rear) axle, i.e. the output of the
// conic gear pair, in chassis local coords. This is needed because ChShaftsBody
// could transfer pitch torque to the chassis.
// -----------------------------------------------------------------------------
ChShaftsPowertrain4wd::ChShaftsPowertrain4wd(ChVehicle*         car,
                                             const ChVector<>&  dir_motor_block,
                                             const ChVector<>&  dir_axle)
: ChPowertrain(car, ChPowertrain::RWD),
  m_dir_motor_block(dir_motor_block),
  m_dir_axle(dir_axle)
{
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChShaftsPowertrain4wd::Initialize(ChSharedPtr<ChBody>  chassis,
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


  // Let the derived class specify the gear ratios
  SetGearRatios(m_gear_ratios);
  assert(m_gear_ratios.size() > 1);
  m_current_gear = 1;


  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // In this case it is the motor block. This because the ChShaftsThermalEngine
  // needs two 1dof items to apply the torque in-between them (the other will be
  // the crankshaft). In simplier models, one could just use SetShaftFixed() on
  // this object, but here we prefer to leave it free and use a ChShaftsBody
  // constraint to connect it to the car chassis (so the car can 'roll' when 
  // pressing the throttle, like in muscle cars)
  m_motorblock = ChSharedPtr<ChShaft>(new ChShaft);
  m_motorblock->SetInertia(GetMotorBlockInertia());
  my_system->Add(m_motorblock);


  // CREATE  a connection between the motor block and the 3D rigid body that
  // represents the chassis. This allows to get the effect of the car 'rolling'
  // when the longitudinal engine accelerates suddenly.
  m_motorblock_to_body = ChSharedPtr<ChShaftsBody>(new ChShaftsBody);
  m_motorblock_to_body->Initialize(m_motorblock,
                                   chassis,
                                   m_dir_motor_block);
  my_system->Add(m_motorblock_to_body);

  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the crankshaft plus flywheel.
  m_crankshaft = ChSharedPtr<ChShaft>(new ChShaft);
  m_crankshaft->SetInertia(GetCrankshaftInertia());
  my_system->Add(m_crankshaft);
  

  // CREATE  a thermal engine model beteen motor block and crankshaft (both
  // receive the torque, but with opposite sign).
  m_engine = ChSharedPtr<ChShaftsThermalEngine>(new ChShaftsThermalEngine);
  m_engine->Initialize(m_crankshaft,
                       m_motorblock);
  my_system->Add(m_engine);
    // The thermal engine requires a torque curve: 
  ChSharedPtr<ChFunction_Recorder> mTw(new ChFunction_Recorder);
  SetEngineTorqueMap(mTw);
  m_engine->SetTorqueCurve(mTw);


  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the shaft that collects all inertias from torque converter to the gear.
  m_shaft_ingear = ChSharedPtr<ChShaft>(new ChShaft);
  m_shaft_ingear->SetInertia(GetIngearShaftInertia());
  my_system->Add(m_shaft_ingear);


  // CREATE a torque converter and connect the shafts: 
  // A (input),B (output), C(truss stator). 
  // The input is the m_crankshaft, output is m_shaft_ingear; for stator, reuse the motor block 1D item.
  m_torqueconverter = ChSharedPtr<ChShaftsTorqueConverter>(new ChShaftsTorqueConverter);
  m_torqueconverter->Initialize(m_crankshaft,
                                m_shaft_ingear,
                                m_motorblock);
  my_system->Add(m_torqueconverter);
   // To complete the setup of the torque converter, a capacity factor curve is needed:
  ChSharedPtr<ChFunction_Recorder> mK(new ChFunction_Recorder);
  SetTorqueConverterCapacityFactorMap(mK);
  m_torqueconverter->SetCurveCapacityFactor(mK);
   // To complete the setup of the torque converter, a torque ratio curve is needed:	
  ChSharedPtr<ChFunction_Recorder> mT(new ChFunction_Recorder);
  SetTorqeConverterTorqueRatioMap(mT);
  m_torqueconverter->SetCurveTorqueRatio(mT);


  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the shaft that collects all inertias from the gear 
  // to the central differential box (including the inertial of the central differential box)
  m_shaft_outgear = ChSharedPtr<ChShaft>(new ChShaft);
  m_shaft_outgear->SetInertia(GetOutgearShaftInertia() + GetCentralDifferentialBoxInertia());
  my_system->Add(m_shaft_outgear);


  // CREATE a gearbox, i.e a transmission ratio constraint between two
  // shafts. Note that differently from the basic ChShaftsGear, this also provides
  // the possibility of transmitting a reaction torque to the box (the truss).
  m_gears = ChSharedPtr<ChShaftsGearbox>(new ChShaftsGearbox);
  m_gears->Initialize(m_shaft_ingear,
                      m_shaft_outgear,
                      chassis, 
                      m_dir_motor_block);
  m_gears->SetTransmissionRatio(m_gear_ratios[m_current_gear]);
  my_system->Add(m_gears);



  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the shaft that connecting central differential to front differential.
  m_shaft_to_front_differential = ChSharedPtr<ChShaft>(new ChShaft);
  m_shaft_to_front_differential->SetInertia(GetToFrontDiffShaftInertia());
  my_system->Add(m_shaft_to_front_differential);

  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the shaft that connecting central differential to rear differential.
  m_shaft_to_rear_differential = ChSharedPtr<ChShaft>(new ChShaft);
  m_shaft_to_rear_differential->SetInertia(GetToRearDiffShaftInertia());
  my_system->Add(m_shaft_to_rear_differential);


  // CREATE the central differential, i.e. an epicycloidal mechanism that connects three 
  // rotating members. This class of mechanisms can be simulated using 
  // ChShaftsPlanetary; a proper 'ordinary' transmission ratio t0 must be assigned according
  // to Willis formula. The case of the differential is simple: t0=-1.
  m_central_differential = ChSharedPtr<ChShaftsPlanetary>(new ChShaftsPlanetary);
  m_central_differential->Initialize(m_shaft_outgear, // the carrier
                                  m_shaft_to_rear_differential,
                                  m_shaft_to_front_differential);
  m_rear_differential->SetTransmissionRatioOrdinary(GetCentralDifferentialRatio());
  my_system->Add(m_rear_differential);


  // ---Rear differential and axles:

  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the inertia of the rotating box of the differential.
  m_shaft_rear_differentialbox = ChSharedPtr<ChShaft>(new ChShaft);
  m_shaft_rear_differentialbox->SetInertia(GetRearDifferentialBoxInertia());
  my_system->Add(m_shaft_rear_differentialbox);


  // CREATE an angled gearbox, i.e a transmission ratio constraint between two
  // non parallel shafts. This is the case of the 90� bevel gears in the differential.
  // Note that differently from the basic ChShaftsGear, this also provides
  // the possibility of transmitting a reaction torque to the box (the truss).
  m_rear_conicalgear = ChSharedPtr<ChShaftsGearboxAngled>(new ChShaftsGearboxAngled);
  m_rear_conicalgear->Initialize(m_shaft_to_rear_differential,
                                 m_shaft_rear_differentialbox,
                                 chassis,
                                 m_dir_motor_block,
                                 m_dir_axle);
  m_rear_conicalgear->SetTransmissionRatio(GetRearConicalGearRatio());
  my_system->Add(m_rear_conicalgear);


  // CREATE a differential, i.e. an apicycloidal mechanism that connects three 
  // rotating members. This class of mechanisms can be simulated using 
  // ChShaftsPlanetary; a proper 'ordinary' transmission ratio t0 must be assigned according
  // to Willis formula. The case of the differential is simple: t0=-1.
  m_rear_differential = ChSharedPtr<ChShaftsPlanetary>(new ChShaftsPlanetary);
  m_rear_differential->Initialize(m_shaft_rear_differentialbox, // the carrier
                                  axle_rear_L,
                                  axle_rear_R);
  m_rear_differential->SetTransmissionRatioOrdinary(GetRearDifferentialRatio());
  my_system->Add(m_rear_differential);


  // ---Front differential and axles:

  // CREATE  a 1 d.o.f. object: a 'shaft' with rotational inertia.
  // This represents the inertia of the rotating box of the differential.
  m_shaft_front_differentialbox = ChSharedPtr<ChShaft>(new ChShaft);
  m_shaft_front_differentialbox->SetInertia(GetRearDifferentialBoxInertia());
  my_system->Add(m_shaft_front_differentialbox);


  // CREATE an angled gearbox, i.e a transmission ratio constraint between two
  // non parallel shafts. This is the case of the 90� bevel gears in the differential.
  // Note that differently from the basic ChShaftsGear, this also provides
  // the possibility of transmitting a reaction torque to the box (the truss).
  m_front_conicalgear = ChSharedPtr<ChShaftsGearboxAngled>(new ChShaftsGearboxAngled);
  m_front_conicalgear->Initialize(m_shaft_to_front_differential,
                                 m_shaft_front_differentialbox,
                                 chassis,
                                 m_dir_motor_block,
                                 m_dir_axle);
  m_front_conicalgear->SetTransmissionRatio(GetFrontConicalGearRatio());
  my_system->Add(m_front_conicalgear);


  // CREATE a differential, i.e. an apicycloidal mechanism that connects three 
  // rotating members. This class of mechanisms can be simulated using 
  // ChShaftsPlanetary; a proper 'ordinary' transmission ratio t0 must be assigned according
  // to Willis formula. The case of the differential is simple: t0=-1.
  m_front_differential = ChSharedPtr<ChShaftsPlanetary>(new ChShaftsPlanetary);
  m_front_differential->Initialize(m_shaft_front_differentialbox, // the carrier
                                   axle_front_L,
                                   axle_front_R);
  m_front_differential->SetTransmissionRatioOrdinary(GetFrontDifferentialRatio());
  my_system->Add(m_front_differential);


  // -------
  // Finally, update the gear ratio according to the selected gear in the 
  // array of gear ratios:
  SetSelectedGear(1);
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChShaftsPowertrain4wd::SetSelectedGear(int igear)
{
  assert(igear >= 0);
  assert(igear < m_gear_ratios.size());

  m_current_gear = igear;
  if (m_gears)
  {
    m_gears->SetTransmissionRatio(m_gear_ratios[igear]);
  }
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
double ChShaftsPowertrain4wd::GetWheelTorque(ChWheelId which) const
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


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChShaftsPowertrain4wd::Update(double time,
                                double throttle)
{
  // Just update the throttle level in the thermal engine
  m_engine->SetThrottle(throttle);
}


} // end namespace chrono
