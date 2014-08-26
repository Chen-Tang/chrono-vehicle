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
// Base class for a double-A arm suspension modeled with bodies and constraints.
//
// The suspension subsystem is modeled with respect to a right-handed frame,
// with X pointing towards the rear, Y to the right, and Z up. By default, a
// right suspension is constructed.  This can be mirrored to obtain a left
// suspension. Note that this is done by reflecting the y coordinates of the
// hardpoints. As such, the orientation of the suspension reference frame must
// be as specified above. However, its location relative to the chassis is
// arbitrary (and left up to a derived class).
//
// If marked as 'driven', the suspension subsystem also creates the ChShaft axle
// element and its connection to the spindle body (which provides the interface
// to the powertrain subsystem).
//
// =============================================================================

#include "assets/ChCylinderShape.h"
#include "assets/ChColorAsset.h"

#include "subsys/suspension/ChDoubleWishbone.h"


namespace chrono {


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
ChDoubleWishbone::ChDoubleWishbone(const std::string& name,
                                                 ChSuspension::Side side,
                                                 bool               driven)
: ChSuspension(name, side, driven)
{
  // Create the upright and spindle bodies
  m_spindle = ChSharedBodyPtr(new ChBody);
  m_spindle->SetNameString(name + "_spindle");

  m_upright = ChSharedBodyPtr(new ChBody);
  m_upright->SetNameString(name + "_upright");

  // Revolute joint between spindle and upright
  m_revolute = ChSharedPtr<ChLinkLockRevolute>(new ChLinkLockRevolute);
  m_revolute->SetNameString(name + "_revolute");

  // Bodies and constraints to model upper control arm
  m_bodyUCA = ChSharedBodyPtr(new ChBody);
  m_bodyUCA->SetNameString(name + "_bodyUCA");
  m_sphericalUCA_F = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_sphericalUCA_F->SetNameString(name + "_sphericalUCA_F");
  m_sphericalUCA_B = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_sphericalUCA_B->SetNameString(name + "_sphericalUCA_B");
  m_sphericalUCA_U = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_sphericalUCA_U->SetNameString(name + "_sphericalUCA_U");

  // Bodies and constraints to model lower control arm
  m_bodyLCA = ChSharedBodyPtr(new ChBody);
  m_bodyLCA->SetNameString(name + "_bodyLCA");
  m_sphericalLCA_F = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_sphericalLCA_F->SetNameString(name + "_sphericalLCA_F");
  m_sphericalLCA_B = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_sphericalLCA_B->SetNameString(name + "_sphericalLCA_B");
  m_sphericalLCA_U = ChSharedPtr<ChLinkLockSpherical>(new ChLinkLockSpherical);
  m_sphericalLCA_U->SetNameString(name + "_sphericalLCA_U");

  // Distance constraint to model the tierod
  m_distTierod = ChSharedPtr<ChLinkDistance>(new ChLinkDistance);
  m_distTierod->SetNameString(name + "_distTierod");

  // Spring-damper
  m_shock = ChSharedPtr<ChLinkSpring>(new ChLinkSpring);
  m_shock->SetNameString(name + "_shock");

  // If driven, create the axle shaft and its connection to the spindle.
  if (m_driven) {
    m_axle = ChSharedPtr<ChShaft>(new ChShaft);
    m_axle->SetNameString(name + "_axle");

    m_axle_to_spindle = ChSharedPtr<ChShaftsBody>(new ChShaftsBody);
    m_axle_to_spindle->SetNameString(name + "_axle_to_spindle");
  }
}


// -----------------------------------------------------------------------------
// TODO: does it make sense to ask for a complete Coordsys (i.e. allow for a 
// non-identity rotation) when attaching a suspension subsystem to the chassis?
// -----------------------------------------------------------------------------
void
ChDoubleWishbone::Initialize(ChSharedBodyPtr   chassis,
                                    const ChVector<>& location)
{
  // Transform all points to absolute frame
  for (int i = 0; i < NUM_POINTS; i++) {
    ChVector<> rel_pos = getLocation(static_cast<PointId>(i));
    if (m_side == LEFT)
      rel_pos.y = -rel_pos.y;
    m_points[i] = chassis->GetCoord().TransformLocalToParent(location + rel_pos);
  }

  // Set body positions and rotations, mass properties, etc.
  m_spindle->SetPos(m_points[SPINDLE]);
  m_spindle->SetRot(chassis->GetCoord().rot);
  m_spindle->SetMass(getSpindleMass());
  m_spindle->SetInertiaXX(getSpindleInertia());
  OnInitializeSpindle();
  chassis->GetSystem()->AddBody(m_spindle);

  m_bodyUCA->SetPos((m_points[UCA_F]+m_points[UCA_B]+m_points[UCA_U])/3);
  //m_bodyUCA->SetRot(chassis->GetCoord().rot); // TODO: Should be quaternion formed by avg(UCA_B,UCA_F) and UCA_U
  m_bodyUCA->SetMass(getUCAMass());
  m_bodyUCA->SetInertiaXX(getUCAInertia());
  AddVisualizationUCA();
  OnInitializeUCA();
  chassis->GetSystem()->AddBody(m_bodyUCA);

  m_bodyLCA->SetPos((m_points[LCA_F]+m_points[LCA_B]+m_points[LCA_U])/3);
  //m_bodyLCA->SetRot(chassis->GetCoord().rot); // TODO: Should be quaternion formed by avg(LCA_B,LCA_F) and LCA_U
  m_bodyLCA->SetMass(getLCAMass());
  m_bodyLCA->SetInertiaXX(getLCAInertia());
  AddVisualizationLCA();
  OnInitializeLCA();
  chassis->GetSystem()->AddBody(m_bodyLCA);

  m_upright->SetPos(m_points[UPRIGHT]);
  m_upright->SetRot(chassis->GetCoord().rot);
  m_upright->SetMass(getUprightMass());
  m_upright->SetInertiaXX(getUprightInertia());
  AddVisualizationUpright();
  OnInitializeUpright();
  chassis->GetSystem()->AddBody(m_upright);

  // Initialize joints
  ChCoordsys<> rev_csys(m_points[UPRIGHT], Q_from_AngAxis(CH_C_PI / 2.0, VECT_X));
  m_revolute->Initialize(m_spindle, m_upright, rev_csys);
  chassis->GetSystem()->AddLink(m_revolute);

  // TODO: Replace m_sphericalUCA_F and m_sphericalUCA_U with a single revolute joint
  m_sphericalUCA_F->Initialize(chassis, m_bodyUCA, ChCoordsys<>(m_points[UCA_F], QUNIT));
  chassis->GetSystem()->AddLink(m_sphericalUCA_F);
  m_sphericalUCA_B->Initialize(chassis, m_bodyUCA, ChCoordsys<>(m_points[UCA_B], QUNIT));
  chassis->GetSystem()->AddLink(m_sphericalUCA_B);
  m_sphericalUCA_U->Initialize(m_bodyUCA, m_upright, ChCoordsys<>(m_points[UCA_U], QUNIT));
  chassis->GetSystem()->AddLink(m_sphericalUCA_U);

  // TODO: Replace m_sphericalLCA_F and m_sphericalLCA_U with a single revolute joint
  m_sphericalLCA_F->Initialize(chassis, m_bodyLCA, ChCoordsys<>(m_points[LCA_F], QUNIT));
  chassis->GetSystem()->AddLink(m_sphericalLCA_F);
  m_sphericalLCA_B->Initialize(chassis, m_bodyLCA, ChCoordsys<>(m_points[LCA_B], QUNIT));
  chassis->GetSystem()->AddLink(m_sphericalLCA_B);
  m_sphericalLCA_U->Initialize(m_bodyLCA, m_upright, ChCoordsys<>(m_points[LCA_U], QUNIT));
  chassis->GetSystem()->AddLink(m_sphericalLCA_U);

  m_distTierod->Initialize(chassis, m_upright, false, m_points[TIEROD_C], m_points[TIEROD_U]);
  chassis->GetSystem()->AddLink(m_distTierod);

  // Initialize the spring/damper
  m_shock->Initialize(chassis, m_upright, false, m_points[SHOCK_C], m_points[SHOCK_U]);
  m_shock->Set_SpringK(getSpringCoefficient());
  m_shock->Set_SpringR(getDampingCoefficient());
  m_shock->Set_SpringRestLength(getSpringRestLength());
  chassis->GetSystem()->AddLink(m_shock);

  // Save initial relative position of marker 1 of the tierod distance link,
  // to be used in steering.
  m_tierod_marker = m_distTierod->GetEndPoint1Rel();

  // Initialize the axle shaft and its connection to the spindle. Note that the
  // spindle rotates about the Y axis.
  if (m_driven) {
    m_axle->SetInertia(getAxleInertia());
    chassis->GetSystem()->Add(m_axle);
    m_axle_to_spindle->Initialize(m_axle, m_spindle, ChVector<>(0, 1, 0));
    chassis->GetSystem()->Add(m_axle_to_spindle);
  }

}


// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
void ChDoubleWishbone::AddVisualizationLCA()
{
  // Express hardpoint locations in body frame.
  ChVector<> p_F = m_bodyLCA->TransformPointParentToLocal(m_points[LCA_F]);
  ChVector<> p_B = m_bodyLCA->TransformPointParentToLocal(m_points[LCA_B]);
  ChVector<> p_U = m_bodyLCA->TransformPointParentToLocal(m_points[LCA_U]);

  ChSharedPtr<ChCylinderShape> cyl_F(new ChCylinderShape);
  cyl_F->GetCylinderGeometry().p1 = p_F;
  cyl_F->GetCylinderGeometry().p2 = p_U;
  cyl_F->GetCylinderGeometry().rad = getLCARadius();
  m_bodyLCA->AddAsset(cyl_F);

  ChSharedPtr<ChCylinderShape> cyl_B(new ChCylinderShape);
  cyl_B->GetCylinderGeometry().p1 = p_B;
  cyl_B->GetCylinderGeometry().p2 = p_U;
  cyl_B->GetCylinderGeometry().rad = 0.02;
  m_bodyLCA->AddAsset(cyl_B);

  ChSharedPtr<ChColorAsset> col(new ChColorAsset);
  switch (m_side) {
  case RIGHT: col->SetColor(ChColor(0.6f, 0.4f, 0.4f)); break;
  case LEFT:  col->SetColor(ChColor(0.4f, 0.4f, 0.6f)); break;
  }
  m_bodyLCA->AddAsset(col);
}

void ChDoubleWishbone::AddVisualizationUCA()
{
  // Express hardpoint locations in body frame.
  ChVector<> p_F = m_bodyUCA->TransformPointParentToLocal(m_points[UCA_F]);
  ChVector<> p_B = m_bodyUCA->TransformPointParentToLocal(m_points[UCA_B]);
  ChVector<> p_U = m_bodyUCA->TransformPointParentToLocal(m_points[UCA_U]);

  ChSharedPtr<ChCylinderShape> cyl_F(new ChCylinderShape);
  cyl_F->GetCylinderGeometry().p1 = p_F;
  cyl_F->GetCylinderGeometry().p2 = p_U;
  cyl_F->GetCylinderGeometry().rad = getUCARadius();
  m_bodyUCA->AddAsset(cyl_F);

  ChSharedPtr<ChCylinderShape> cyl_B(new ChCylinderShape);
  cyl_B->GetCylinderGeometry().p1 = p_B;
  cyl_B->GetCylinderGeometry().p2 = p_U;
  cyl_B->GetCylinderGeometry().rad = 0.02;
  m_bodyUCA->AddAsset(cyl_B);

  ChSharedPtr<ChColorAsset> col(new ChColorAsset);
  switch (m_side) {
  case RIGHT: col->SetColor(ChColor(0.6f, 0.4f, 0.4f)); break;
  case LEFT:  col->SetColor(ChColor(0.4f, 0.4f, 0.6f)); break;
  }
  m_bodyUCA->AddAsset(col);
}

void ChDoubleWishbone::AddVisualizationUpright()
{
  // Express hardpoint locations in body frame.
  ChVector<> p_U = m_upright->TransformPointParentToLocal(m_points[UCA_U]);
  ChVector<> p_L = m_upright->TransformPointParentToLocal(m_points[LCA_U]);

  ChSharedPtr<ChCylinderShape> cyl(new ChCylinderShape);
  cyl->GetCylinderGeometry().p1 = p_U;
  cyl->GetCylinderGeometry().p2 = p_L;
  cyl->GetCylinderGeometry().rad = getUprightRadius();
  m_upright->AddAsset(cyl);

  ChSharedPtr<ChColorAsset> col(new ChColorAsset);
  switch (m_side) {
  case RIGHT: col->SetColor(ChColor(0.6f, 0.1f, 0.1f)); break;
  case LEFT:  col->SetColor(ChColor(0.1f, 0.1f, 0.6f)); break;
  }
  m_upright->AddAsset(col);
}


// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void ChDoubleWishbone::ApplySteering(double displ)
{
  ChVector<> r_bar = m_tierod_marker;
  r_bar.y += displ;
  m_distTierod->SetEndPoint1Rel(r_bar);
}


} // end namespace chrono
