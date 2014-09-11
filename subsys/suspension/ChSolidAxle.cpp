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
// Authors: Radu Serban, Justin Madsen, Daniel Melanz
// =============================================================================
//
// Base class for a solid axle suspension modeled with bodies and constraints.
//
// The suspension subsystem is modeled with respect to a right-handed frame,
// with X pointing towards the rear, Y to the right, and Z up. All point
// locations are assumed to be given for the right half of the supspension and
// will be mirrored (reflecting the y coordinates) to construct the left side.
//
// If marked as 'driven', the suspension subsystem also creates the ChShaft axle
// element and its connection to the spindle body (which provides the interface
// to the driveline subsystem).
//
// =============================================================================

#include "assets/ChCylinderShape.h"
#include "assets/ChColorAsset.h"

#include "subsys/suspension/ChSolidAxle.h"


namespace chrono {


// -----------------------------------------------------------------------------
// Static variables
// -----------------------------------------------------------------------------
const std::string ChSolidAxle::m_pointNames[] = {
    "AXLE_OUTER ",
    "SHOCK_A    ",
    "SHOCK_C    ",
    "KNUCKLE_L  ",
    "KNUCKLE_U  ",
    "LL_A       ",
    "LL_A_X     ",   
    "LL_A_Z     ",
    "LL_C       ",
    "LL_C_X     ",
    "LL_C_Z     ",
    "UL_A       ",
    "UL_A_X     ",     
    "UL_A_Z     ",
    "UL_C       ",
    "UL_C_X     ",
    "UL_C_Z     ",
    "SPRING_A   ",
    "SPRING_C   ",
    "TIEROD_C   ",
    "TIEROD_K   ",
    "SPINDLE    ",
    "KNUCKLE_CM ",
    "LL_CM      ",
    "UL_CM      ",
    "AXLE_CM    "
};


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ChSolidAxle::ChSolidAxle(const std::string& name,
                                   bool               steerable,
                                   bool               driven)
: ChSuspension(name, steerable, driven)
{
  // Create the axle body
  m_axleTube = ChSharedBodyPtr(new ChBody);
  m_axleTube->SetNameString(m_name + "_axleTube");

  CreateSide(LEFT, "_L");
  CreateSide(RIGHT, "_R");
}

void ChSolidAxle::CreateSide(ChSuspension::Side side,
                                  const std::string& suffix)
{
  // Create the knuckle bodies
  m_knuckle[side] = ChSharedBodyPtr(new ChBody);
  m_knuckle[side]->SetNameString(m_name + "_knuckle" + suffix);

  // Create the knuckle - axle revolute joints
  m_revoluteKingpin[side] = ChSharedPtr<ChLinkLockRevolute>(new ChLinkLockRevolute);
  m_revoluteKingpin[side]->SetNameString(m_name + "_revoluteKingpin" + suffix);

  // Create the spindle bodies
  m_spindle[side] = ChSharedBodyPtr(new ChBody);
  m_spindle[side]->SetNameString(m_name + "_spindle" + suffix);

  // Create the spindle - knuckle revolute joins
  m_revolute[side] = ChSharedPtr<ChLinkLockRevolute>(new ChLinkLockRevolute);
  m_revolute[side]->SetNameString(m_name + "_revolute" + suffix);

  // Create the upper link bodies
  m_upperLink[side] = ChSharedBodyPtr(new ChBody);
  m_upperLink[side]->SetNameString(m_name + "_upperLink" + suffix);

  // Create the upper link - axle spherical joints
  m_sphericalUpperLink[side] = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_sphericalUpperLink[side]->SetNameString(m_name + "_sphericalUpperLink" + suffix);

  // Create the upper link - chassis universal joints
  m_universalUpperLink[side] = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_universalUpperLink[side]->SetNameString(m_name + "_universalUpperLink" + suffix);

  // Create the lower link bodies
  m_lowerLink[side] = ChSharedBodyPtr(new ChBody);
  m_lowerLink[side]->SetNameString(m_name + "_lowerLink" + suffix);

  // Create the lower link - axle spherical joints
  m_sphericalLowerLink[side] = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_sphericalLowerLink[side]->SetNameString(m_name + "_sphericalLowerLink" + suffix);

  // Create the lower link - chassis universal joints
  m_universalLowerLink[side] = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_universalLowerLink[side]->SetNameString(m_name + "_universalLowerLink" + suffix);

  // Distance constraint to model the tierod
  m_distTierod[side] = ChSharedPtr<ChLinkDistance>(new ChLinkDistance);
  m_distTierod[side]->SetNameString(m_name + "_distTierod" + suffix);

  // Spring-damper
  m_shock[side] = ChSharedPtr<ChLinkSpring>(new ChLinkSpring);
  m_shock[side]->SetNameString(m_name + "_shock" + suffix);
  m_spring[side] = ChSharedPtr<ChLinkSpring>(new ChLinkSpring);
  m_spring[side]->SetNameString(m_name + "_spring" + suffix);

  // If driven, create the axle shaft and its connection to the spindle.
  if (m_driven) {
    m_axle[side] = ChSharedPtr<ChShaft>(new ChShaft);
    m_axle[side]->SetNameString(m_name + "_axle" + suffix);
    m_axle_to_spindle[side] = ChSharedPtr<ChShaftsBody>(new ChShaftsBody);
    m_axle_to_spindle[side]->SetNameString(m_name + "_axle_to_spindle" + suffix);
  }
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidAxle::Initialize(ChSharedBodyPtr   chassis,
                                  const ChVector<>& location)
{
  std::vector<ChVector<> > points(NUM_POINTS);

  // Transform all points to absolute frame.
  for (int i = 0; i < NUM_POINTS; i++) {
    ChVector<> rel_pos = getLocation(static_cast<PointId>(i));
    points[i] = chassis->GetCoord().TransformLocalToParent(location + rel_pos);
  }

  // Initialize axle body.
  m_axleTube->SetPos(points[AXLE_CM]);
  m_axleTube->SetRot(chassis->GetCoord().rot);
  m_axleTube->SetMass(getAxleTubeMass());
  m_axleTube->SetInertiaXX(getAxleTubeInertia());
  AddVisualizationAxleTube(m_axleTube, points[AXLE_OUTER], points[LL_A], points[UL_A], getAxleTubeRadius(), getULRadius());
  chassis->GetSystem()->AddBody(m_axleTube);

  // Transform all points to absolute frame and initialize left side.
  for (int i = 0; i < NUM_POINTS; i++) {
    ChVector<> rel_pos = getLocation(static_cast<PointId>(i));
    rel_pos.y = -rel_pos.y;
    points[i] = chassis->GetCoord().TransformLocalToParent(location + rel_pos);
  }

  InitializeSide(LEFT, chassis, points);

  // Transform all points to absolute frame and initialize right side.
  for (int i = 0; i < NUM_POINTS; i++) {
    ChVector<> rel_pos = getLocation(static_cast<PointId>(i));
    points[i] = chassis->GetCoord().TransformLocalToParent(location + rel_pos);
  }

  InitializeSide(RIGHT, chassis, points);
}

void ChSolidAxle::InitializeSide(ChSuspension::Side              side,
                                      ChSharedBodyPtr                 chassis,
                                      const std::vector<ChVector<> >& points)
{
  // Initialize knuckle body.
  m_knuckle[side]->SetPos(points[KNUCKLE_CM]);
  m_knuckle[side]->SetRot(chassis->GetCoord().rot);
  m_knuckle[side]->SetMass(getKnuckleMass());
  m_knuckle[side]->SetInertiaXX(getKnuckleInertia());
  // TODO: ADD KNUCKLE VISUALIZATION
  //AddVisualizationKnuckle(m_knuckle[side], points[KNUCKLE_U], points[KNUCKLE_L], points[SPINDLE], getKnuckleRadius());
  chassis->GetSystem()->AddBody(m_knuckle[side]);

  // Initialize spindle body.
  m_spindle[side]->SetPos(points[SPINDLE]);
  m_spindle[side]->SetRot(chassis->GetCoord().rot);
  m_spindle[side]->SetMass(getSpindleMass());
  m_spindle[side]->SetInertiaXX(getSpindleInertia());
  AddVisualizationSpindle(m_spindle[side], getSpindleRadius(), getSpindleWidth());
  chassis->GetSystem()->AddBody(m_spindle[side]);

  // Initialize upper link body.
  m_upperLink[side]->SetPos(points[UL_CM]);
  m_upperLink[side]->SetRot(chassis->GetCoord().rot);
  m_upperLink[side]->SetMass(getULMass());
  m_upperLink[side]->SetInertiaXX(getULInertia());
  AddVisualizationLink(m_upperLink[side], points[UL_A], points[UL_C], getULRadius());
  chassis->GetSystem()->AddBody(m_upperLink[side]);

  // Initialize lower link body.
  m_lowerLink[side]->SetPos(points[LL_CM]);
  m_lowerLink[side]->SetRot(chassis->GetCoord().rot);
  m_lowerLink[side]->SetMass(getLLMass());
  m_lowerLink[side]->SetInertiaXX(getLLInertia());
  AddVisualizationLink(m_lowerLink[side], points[LL_A], points[LL_C], getLLRadius());
  chassis->GetSystem()->AddBody(m_lowerLink[side]);

  // Unit vectors for orientation matrices.
  ChVector<> u;
  ChVector<> v;
  ChVector<> w;
  ChMatrix33<> rot;

  // Initialize the revolute joint between axle and knuckle.
  // Determine the joint orientation matrix from the hardpoint locations by
  // constructing a rotation matrix with the z axis along the joint direction
  // and the y axis normal to the plane of the knuckle.
  v = Vcross(points[KNUCKLE_U] - points[SPINDLE], points[KNUCKLE_L] - points[SPINDLE]);
  v.Normalize();
  w = points[KNUCKLE_L] - points[KNUCKLE_U];
  w.Normalize();
  u = Vcross(v, w);
  rot.Set_A_axis(u, v, w);

  m_revoluteKingpin[side]->Initialize(m_axleTube, m_knuckle[side], ChCoordsys<>((points[KNUCKLE_U]+points[KNUCKLE_L])/2, rot.Get_A_quaternion()));
  chassis->GetSystem()->AddLink(m_revoluteKingpin[side]);

  // Initialize the spherical joint between axle and upper link.
  m_sphericalUpperLink[side]->Initialize(m_axleTube, m_upperLink[side], ChCoordsys<>(points[UL_A], QUNIT));
  chassis->GetSystem()->AddLink(m_sphericalUpperLink[side]);

  // Initialize the spherical joint between axle and lower link.
  m_sphericalLowerLink[side]->Initialize(m_axleTube, m_lowerLink[side], ChCoordsys<>(points[LL_A], QUNIT));
  chassis->GetSystem()->AddLink(m_sphericalLowerLink[side]);

  // Initialize the universal joint between chassis and upper link.
  m_universalUpperLink[side]->Initialize(chassis, m_upperLink[side], ChCoordsys<>(points[UL_C], QUNIT));
  chassis->GetSystem()->AddLink(m_universalUpperLink[side]);

  // Initialize the universal joint between chassis and lower link.
  m_universalLowerLink[side]->Initialize(chassis, m_lowerLink[side], ChCoordsys<>(points[LL_C], QUNIT));
  chassis->GetSystem()->AddLink(m_universalLowerLink[side]);

  // Initialize the revolute joint between upright and spindle.
  ChCoordsys<> rev_csys(points[SPINDLE], Q_from_AngAxis(CH_C_PI / 2.0, VECT_X));
  m_revolute[side]->Initialize(m_spindle[side], m_knuckle[side], rev_csys);
  chassis->GetSystem()->AddLink(m_revolute[side]);

  // Initialize the spring/damper
  m_shock[side]->Initialize(chassis, m_axleTube, false, points[SHOCK_C], points[SHOCK_A], true, getSpringRestLength());
  m_shock[side]->Set_SpringK(0.0);
  m_shock[side]->Set_SpringR(getDampingCoefficient());
  chassis->GetSystem()->AddLink(m_shock[side]);

  m_spring[side]->Initialize(chassis, m_axleTube, false, points[SPRING_C], points[SPRING_A], true, getSpringRestLength());
  m_spring[side]->Set_SpringK(getSpringCoefficient());
  m_spring[side]->Set_SpringR(0.0);
  chassis->GetSystem()->AddLink(m_spring[side]);

  // Initialize the tierod distance constraint between chassis and upright.
  m_distTierod[side]->Initialize(chassis, m_knuckle[side], false, points[TIEROD_C], points[TIEROD_K]);
  chassis->GetSystem()->AddLink(m_distTierod[side]);

  // Save initial relative position of marker 1 of the tierod distance link,
  // to be used in steering.
  m_tierod_marker[side] = m_distTierod[side]->GetEndPoint1Rel();

  // Initialize the axle shaft and its connection to the spindle. Note that the
  // spindle rotates about the Y axis.
  if (m_driven) {
    m_axle[side]->SetInertia(getAxleInertia());
    chassis->GetSystem()->Add(m_axle[side]);

    m_axle_to_spindle[side]->Initialize(m_axle[side], m_spindle[side], ChVector<>(0, 1, 0));
    chassis->GetSystem()->Add(m_axle_to_spindle[side]);
  }
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
double ChSolidAxle::GetSpringForce(ChSuspension::Side side)
{
  return  m_spring[side]->Get_SpringReact();
}

double ChSolidAxle::GetSpringLen(ChSuspension::Side side)
{
  return (m_spring[side]->GetMarker1()->GetAbsCoord().pos - m_spring[side]->GetMarker2()->GetAbsCoord().pos).Length();
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidAxle::LogHardpointLocations(const ChVector<>& ref,
                                             bool              inches)
{
  double unit = inches ? 1 / 0.0254 : 1.0;

  for (int i = 0; i < NUM_POINTS; i++) {
    ChVector<> pos = ref + unit * getLocation(static_cast<PointId>(i));

    GetLog() << "   " << m_pointNames[i].c_str() << "  " << pos.x << "  " << pos.y << "  " << pos.z << "\n";
  }
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidAxle::LogConstraintViolations(ChSuspension::Side side)
{
  /*
  // Revolute joints
  {
    ChMatrix<>* C = m_revoluteLCA[side]->GetC();
    GetLog() << "LCA revolute\n";
    GetLog() << "  " << C->GetElement(0, 0) << "\n";
    GetLog() << "  " << C->GetElement(1, 0) << "\n";
    GetLog() << "  " << C->GetElement(2, 0) << "\n";
    GetLog() << "  " << C->GetElement(3, 0) << "\n";
    GetLog() << "  " << C->GetElement(4, 0) << "\n";
  }
  {
    ChMatrix<>* C = m_revoluteUCA[side]->GetC();
    GetLog() << "UCA revolute\n";
    GetLog() << "  " << C->GetElement(0, 0) << "\n";
    GetLog() << "  " << C->GetElement(1, 0) << "\n";
    GetLog() << "  " << C->GetElement(2, 0) << "\n";
    GetLog() << "  " << C->GetElement(3, 0) << "\n";
    GetLog() << "  " << C->GetElement(4, 0) << "\n";
  }
  {
    ChMatrix<>* C = m_revolute[side]->GetC();
    GetLog() << "Spindle revolute\n";
    GetLog() << "  " << C->GetElement(0, 0) << "\n";
    GetLog() << "  " << C->GetElement(1, 0) << "\n";
    GetLog() << "  " << C->GetElement(2, 0) << "\n";
    GetLog() << "  " << C->GetElement(3, 0) << "\n";
    GetLog() << "  " << C->GetElement(4, 0) << "\n";
  }

  // Spherical joints
  {
    ChMatrix<>* C = m_sphericalLCA[side]->GetC();
    GetLog() << "LCA spherical\n";
    GetLog() << "  " << C->GetElement(0, 0) << "\n";
    GetLog() << "  " << C->GetElement(1, 0) << "\n";
    GetLog() << "  " << C->GetElement(2, 0) << "\n";
  }
  {
    ChMatrix<>* C = m_sphericalUCA[side]->GetC();
    GetLog() << "UCA spherical\n";
    GetLog() << "  " << C->GetElement(0, 0) << "\n";
    GetLog() << "  " << C->GetElement(1, 0) << "\n";
    GetLog() << "  " << C->GetElement(2, 0) << "\n";
  }

  // Distance constraint
  GetLog() << "Tierod distance\n";
  GetLog() << "  " << m_distTierod[side]->GetCurrentDistance() - m_distTierod[side]->GetImposedDistance() << "\n";
  */
}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidAxle::AddVisualizationAxleTube(ChSharedBodyPtr    axle,
                                                  const ChVector<>&  pt_axleOuter,
                                                  const ChVector<>&  pt_lowerLinkAxle,
                                                  const ChVector<>&  pt_upperLinkAxle,
                                                  double             radius_axle,
                                                  double             radius_link)
{
  // Express hardpoint locations in body frame.
  ChVector<> pt_axleOuterR = axle->TransformPointParentToLocal(pt_axleOuter);
  ChVector<> pt_axleOuterL = pt_axleOuter;
  pt_axleOuterL.y = -pt_axleOuterL.y;
  pt_axleOuterL = axle->TransformPointParentToLocal(pt_axleOuterL);
  ChVector<> pt_lowerLinkAxleR = axle->TransformPointParentToLocal(pt_lowerLinkAxle);
  ChVector<> pt_lowerLinkAxleL = pt_lowerLinkAxle;
  pt_lowerLinkAxleL.y = -pt_lowerLinkAxleL.y;
  pt_lowerLinkAxleL = axle->TransformPointParentToLocal(pt_lowerLinkAxleL);
  ChVector<> pt_upperLinkAxleR = axle->TransformPointParentToLocal(pt_upperLinkAxle);
  ChVector<> pt_upperLinkAxleL = pt_upperLinkAxle;
  pt_upperLinkAxleL.y = -pt_upperLinkAxleL.y;
  pt_upperLinkAxleL = axle->TransformPointParentToLocal(pt_upperLinkAxleL);

  ChSharedPtr<ChCylinderShape> cyl_axle(new ChCylinderShape);
  cyl_axle->GetCylinderGeometry().p1 = pt_axleOuterR;
  cyl_axle->GetCylinderGeometry().p2 = pt_axleOuterL;
  cyl_axle->GetCylinderGeometry().rad = radius_axle;
  axle->AddAsset(cyl_axle);

  ChSharedPtr<ChCylinderShape> cyl_linkR(new ChCylinderShape);
  cyl_linkR->GetCylinderGeometry().p1 = pt_upperLinkAxleR;
  cyl_linkR->GetCylinderGeometry().p2 = pt_lowerLinkAxleR;
  cyl_linkR->GetCylinderGeometry().rad = radius_link;
  axle->AddAsset(cyl_linkR);

  ChSharedPtr<ChCylinderShape> cyl_linkL(new ChCylinderShape);
  cyl_linkL->GetCylinderGeometry().p1 = pt_upperLinkAxleL;
  cyl_linkL->GetCylinderGeometry().p2 = pt_lowerLinkAxleL;
  cyl_linkL->GetCylinderGeometry().rad = radius_link;
  axle->AddAsset(cyl_linkL);

  ChSharedPtr<ChColorAsset> col(new ChColorAsset);
  col->SetColor(ChColor(0.7f, 0.7f, 0.7f));
  axle->AddAsset(col);
}

void ChSolidAxle::AddVisualizationLink(ChSharedBodyPtr    link,
                                               const ChVector<>&  pt_linkAxle,
                                               const ChVector<>&  pt_linkChassis,
                                               double             radius_link)
{
  // Express hardpoint locations in body frame.
  ChVector<> pt_linkA = link->TransformPointParentToLocal(pt_linkAxle);
  ChVector<> pt_linkC = link->TransformPointParentToLocal(pt_linkChassis);

  ChSharedPtr<ChCylinderShape> cyl_link(new ChCylinderShape);
  cyl_link->GetCylinderGeometry().p1 = pt_linkA;
  cyl_link->GetCylinderGeometry().p2 = pt_linkC;
  cyl_link->GetCylinderGeometry().rad = radius_link;
  link->AddAsset(cyl_link);

  ChSharedPtr<ChColorAsset> col(new ChColorAsset);
  col->SetColor(ChColor(0.2f, 0.2f, 0.6f));
  link->AddAsset(col);
  
}

void ChSolidAxle::AddVisualizationSpindle(ChSharedBodyPtr spindle,
                                               double          radius,
                                               double          width)
{
  ChSharedPtr<ChCylinderShape> cyl(new ChCylinderShape);
  cyl->GetCylinderGeometry().p1 = ChVector<>(0, width / 2, 0);
  cyl->GetCylinderGeometry().p2 = ChVector<>(0, -width / 2, 0);
  cyl->GetCylinderGeometry().rad = radius;
  spindle->AddAsset(cyl);
}

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChSolidAxle::ApplySteering(double displ)
{
  {
    ChVector<> r_bar = m_tierod_marker[LEFT];
    r_bar.y += displ;
    m_distTierod[LEFT]->SetEndPoint1Rel(r_bar);
  }
  {
    ChVector<> r_bar = m_tierod_marker[RIGHT];
    r_bar.y += displ;
    m_distTierod[RIGHT]->SetEndPoint1Rel(r_bar);
  }
}


} // end namespace chrono