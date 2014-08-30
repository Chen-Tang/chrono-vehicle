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

#include "assets/ChBoxShape.h"
#include "assets/ChTriangleMeshShape.h"

#include "utils/ChUtilsData.h"
#include "utils/ChUtilsInputOutput.h"

#include "models/hmmwv/vehicle/HMMWV_VehicleReduced.h"

using namespace chrono;

namespace hmmwv {

// -----------------------------------------------------------------------------
// Static variables
// -----------------------------------------------------------------------------

static const double in2m = 0.0254;

const double     HMMWV_VehicleReduced::m_chassisMass = 7500.0 / 2.2;
const ChVector<> HMMWV_VehicleReduced::m_chassisInertia(125.8, 497.4, 531.4); // chassis inertia (roll,pitch,yaw)

const std::string HMMWV_VehicleReduced::m_chassisMeshName = "hmmwv_chassis";
const std::string HMMWV_VehicleReduced::m_chassisMeshFile = utils::GetModelDataFile("hmmwv/humvee4_scaled_rotated_decimated_centered.obj");

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
HMMWV_VehicleReduced::HMMWV_VehicleReduced(const bool           fixed,
                                           VisualizationType    chassisVis,
                                           VisualizationType    wheelVis)
{
  // -------------------------------------------
  // Create the chassis body
  // -------------------------------------------

  m_chassis = ChSharedBodyPtr(new ChBody);

  m_chassis->SetIdentifier(0);
  m_chassis->SetName("chassis");
  m_chassis->SetMass(m_chassisMass);
  m_chassis->SetInertiaXX(m_chassisInertia);
  m_chassis->SetBodyFixed(fixed);

  switch (chassisVis) {
  case PRIMITIVES:
  {
    ChSharedPtr<ChBoxShape> box1(new ChBoxShape);
    box1->GetBoxGeometry().SetLengths(ChVector<>(5, 1.7, 0.4));
    box1->Pos = ChVector<>(0, 0, -0.4);
    m_chassis->AddAsset(box1);

    ChSharedPtr<ChBoxShape> box2(new ChBoxShape);
    box2->GetBoxGeometry().SetLengths(ChVector<>(4, 1.7, 0.4));
    box2->Pos = ChVector<>(0.5, 0, 0);
    m_chassis->AddAsset(box2);

    break;
  }
  case MESH:
  {
    geometry::ChTriangleMeshConnected trimesh;
    trimesh.LoadWavefrontMesh(m_chassisMeshFile, false, false);

    ChSharedPtr<ChTriangleMeshShape> trimesh_shape(new ChTriangleMeshShape);
    trimesh_shape->SetMesh(trimesh);
    trimesh_shape->SetName(m_chassisMeshName);
    m_chassis->AddAsset(trimesh_shape);

    break;
  }
  }

  Add(m_chassis);

  // -------------------------------------------
  // Create the suspension subsystems
  // -------------------------------------------

  m_front_right_susp = ChSharedPtr<HMMWV_DoubleWishboneReducedFront>(new HMMWV_DoubleWishboneReducedFront("FRsusp", ChSuspension::RIGHT));
  m_front_left_susp  = ChSharedPtr<HMMWV_DoubleWishboneReducedFront>(new HMMWV_DoubleWishboneReducedFront("FLsusp", ChSuspension::LEFT));
  m_rear_right_susp  = ChSharedPtr<HMMWV_DoubleWishboneReducedRear>(new HMMWV_DoubleWishboneReducedRear("RRsusp", ChSuspension::RIGHT, true));
  m_rear_left_susp   = ChSharedPtr<HMMWV_DoubleWishboneReducedRear>(new HMMWV_DoubleWishboneReducedRear("RLsusp", ChSuspension::LEFT, true));

  // -----------------
  // Create the wheels
  // -----------------

  m_front_right_wheel = ChSharedPtr<HMMWV_Wheel>(new HMMWV_WheelRight(wheelVis));
  m_front_left_wheel = ChSharedPtr<HMMWV_Wheel>(new HMMWV_WheelLeft(wheelVis));
  m_rear_right_wheel = ChSharedPtr<HMMWV_Wheel>(new HMMWV_WheelRight(wheelVis));
  m_rear_left_wheel = ChSharedPtr<HMMWV_Wheel>(new HMMWV_WheelLeft(wheelVis));

  // -------------------------------
  // Create the powertrain subsystem
  //--------------------------------

  ////m_powertrain = ChSharedPtr<HMMWV_SimplePowertrain>(new HMMWV_SimplePowertrain(this));
  m_powertrain = ChSharedPtr<HMMWV_Powertrain>(new HMMWV_Powertrain(this));

}


HMMWV_VehicleReduced::~HMMWV_VehicleReduced()
{
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void HMMWV_VehicleReduced::Initialize(const ChCoordsys<>& chassisPos)
{
  m_chassis->SetPos(chassisPos.pos);
  m_chassis->SetRot(chassisPos.rot);

  // Initialize the suspension subsystems
  m_front_right_susp->Initialize(m_chassis, in2m * ChVector<>(-85.39, 12.10, -18.914));
  m_front_left_susp->Initialize(m_chassis, in2m * ChVector<>(-85.39, -12.10, -18.914));
  m_rear_right_susp->Initialize(m_chassis, in2m * ChVector<>(47.60, 12.10, -18.914));
  m_rear_left_susp->Initialize(m_chassis, in2m * ChVector<>(47.60, -12.10, -18.914));

  // Initialize wheels
  m_front_right_wheel->Initialize(m_front_right_susp->GetSpindle());
  m_front_left_wheel->Initialize(m_front_left_susp->GetSpindle());
  m_rear_right_wheel->Initialize(m_rear_right_susp->GetSpindle());
  m_rear_left_wheel->Initialize(m_rear_left_susp->GetSpindle());

  // Initialize the powertrain subsystem
  m_powertrain->Initialize(m_chassis, m_rear_left_susp->GetAxle(), m_rear_right_susp->GetAxle());

}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
ChSharedBodyPtr HMMWV_VehicleReduced::GetWheelBody(ChWheelId which) const
{
  switch (which) {
  case FRONT_LEFT:
    return m_front_left_susp->GetSpindle();
  case FRONT_RIGHT:
    return m_front_right_susp->GetSpindle();
  case REAR_LEFT:
    return m_rear_left_susp->GetSpindle();
  case REAR_RIGHT:
    return m_rear_right_susp->GetSpindle();
  default:
    return m_front_left_susp->GetSpindle();  // should not happen
  }
}

const ChVector<>& HMMWV_VehicleReduced::GetWheelPos(ChWheelId which) const
{
  switch (which) {
  case FRONT_LEFT:
    return m_front_left_susp->GetSpindlePos();
  case FRONT_RIGHT:
    return m_front_right_susp->GetSpindlePos();
  case REAR_LEFT:
    return m_rear_left_susp->GetSpindlePos();
  case REAR_RIGHT:
    return m_rear_right_susp->GetSpindlePos();
  default:
    return m_front_left_susp->GetSpindlePos();  // should not happen
  }
}

const ChQuaternion<>& HMMWV_VehicleReduced::GetWheelRot(ChWheelId which) const
{
  switch (which) {
  case FRONT_LEFT:
    return m_front_left_susp->GetSpindleRot();
  case FRONT_RIGHT:
    return m_front_right_susp->GetSpindleRot();
  case REAR_LEFT:
    return m_rear_left_susp->GetSpindleRot();
  case REAR_RIGHT:
    return m_rear_right_susp->GetSpindleRot();
  default:
    return m_front_left_susp->GetSpindleRot();  // should not happen
  }
}

const ChVector<>& HMMWV_VehicleReduced::GetWheelLinVel(ChWheelId which) const
{
  switch (which) {
  case FRONT_LEFT:
    return m_front_left_susp->GetSpindleLinVel();
  case FRONT_RIGHT:
    return m_front_right_susp->GetSpindleLinVel();
  case REAR_LEFT:
    return m_rear_left_susp->GetSpindleLinVel();
  case REAR_RIGHT:
    return m_rear_right_susp->GetSpindleLinVel();
  default:
    return m_front_left_susp->GetSpindleLinVel();  // should not happen
  }
}

ChVector<> HMMWV_VehicleReduced::GetWheelAngVel(ChWheelId which) const
{
  switch (which) {
  case FRONT_LEFT:
    return m_front_left_susp->GetSpindleAngVel();
  case FRONT_RIGHT:
    return m_front_right_susp->GetSpindleAngVel();
  case REAR_LEFT:
    return m_rear_left_susp->GetSpindleAngVel();
  case REAR_RIGHT:
    return m_rear_right_susp->GetSpindleAngVel();
  default:
    return m_front_left_susp->GetSpindleAngVel();  // should not happen
  }
}

double HMMWV_VehicleReduced::GetWheelOmega(ChWheelId which) const
{
  switch (which) {
  case FRONT_LEFT:
    return m_front_left_susp->GetAxleSpeed();
  case FRONT_RIGHT:
    return m_front_right_susp->GetAxleSpeed();
  case REAR_LEFT:
    return m_rear_left_susp->GetAxleSpeed();
  case REAR_RIGHT:
    return m_rear_right_susp->GetAxleSpeed();
  default:
    return -1;  // should not happen
  }
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void HMMWV_VehicleReduced::Update(double              time,
                                  double              throttle,
                                  double              steering,
                                  const ChTireForces& tire_forces)
{
  // Apply steering input.
  double displ = 0.08 * steering;

  m_front_left_susp->ApplySteering(displ);
  m_front_right_susp->ApplySteering(displ);

  // Let the powertrain subsystem process the throttle input.
  m_powertrain->Update(time, throttle);

  // Apply tire forces to spindle bodies.
  m_front_right_susp->ApplyTireForce(tire_forces[FRONT_RIGHT]);
  m_front_left_susp->ApplyTireForce(tire_forces[FRONT_LEFT]);
  m_rear_right_susp->ApplyTireForce(tire_forces[REAR_RIGHT]);
  m_rear_left_susp->ApplyTireForce(tire_forces[REAR_LEFT]);
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void HMMWV_VehicleReduced::ExportMeshPovray(const std::string& out_dir)
{
  utils::WriteMeshPovray(m_chassisMeshFile, m_chassisMeshName, out_dir);
}


} // end namespace hmmwv
