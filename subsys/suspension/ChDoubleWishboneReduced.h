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
// Base class for a double-A arm suspension modeled with distance constraints.
// Derived from ChSuspension, but still an abstract bas class.
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

#ifndef CH_DOUBLEWISHBONEREDUCED_H
#define CH_DOUBLEWISHBONEREDUCED_H

#include "subsys/ChApiSubsys.h"
#include "subsys/ChSuspension.h"

namespace chrono {


class CH_SUBSYS_API ChDoubleWishboneReduced : public ChSuspension
{
public:

  ChDoubleWishboneReduced(const std::string& name,
                          bool               steerable = false,
                          bool               driven = false);
  virtual ~ChDoubleWishboneReduced() {}

  virtual void Initialize(ChSharedBodyPtr   chassis,
                          const ChVector<>& location);

  virtual void ApplySteering(double displ);

protected:

  enum PointId {
    SPINDLE,    // spindle location
    UPRIGHT,    // upright location
    UCA_F,      // upper control arm, chassis front
    UCA_B,      // upper control arm, chassis back
    UCA_U,      // upper control arm, upright
    LCA_F,      // lower control arm, chassis front
    LCA_B,      // lower control arm, chassis back
    LCA_U,      // lower control arm, upright
    SHOCK_C,    // shock, chassis
    SHOCK_U,    // shock, upright
    TIEROD_C,   // tierod, chassis
    TIEROD_U,   // tierod, upright
    NUM_POINTS
  };

  virtual const ChVector<> getLocation(PointId which) = 0;

  virtual double getSpindleMass() const = 0;
  virtual double getUprightMass() const = 0;

  virtual double getSpindleRadius() const = 0;
  virtual double getSpindleWidth() const = 0;
  virtual double getUprightRadius() const = 0;

  virtual const ChVector<>& getSpindleInertia() const = 0;
  virtual const ChVector<>& getUprightInertia() const = 0;

  virtual double getAxleInertia() const = 0;

  virtual double getSpringCoefficient() const = 0;
  virtual double getDampingCoefficient() const = 0;
  virtual double getSpringRestLength() const = 0;

  virtual void OnInitializeSpindle(ChSuspension::Side side) {}
  virtual void OnInitializeUpright(ChSuspension::Side side) {}

  ChVector<>                        m_points[NUM_POINTS];

  ChSharedBodyPtr                   m_upright[2];

  ChSharedPtr<ChLinkLockRevolute>   m_revolute[2];
  ChSharedPtr<ChLinkDistance>       m_distUCA_F[2];
  ChSharedPtr<ChLinkDistance>       m_distUCA_B[2];
  ChSharedPtr<ChLinkDistance>       m_distLCA_F[2];
  ChSharedPtr<ChLinkDistance>       m_distLCA_B[2];
  ChSharedPtr<ChLinkDistance>       m_distTierod[2];

  ChSharedPtr<ChLinkSpring>         m_shock[2];

  ChVector<>                        m_tierod_marker[2];

private:
  void   CreateSide(ChSuspension::Side side,
                    const std::string& suffix);
  void   InitializeSide(ChSuspension::Side side,
                        ChSharedBodyPtr    chassis);

  void   AddVisualizationSpindle(ChSuspension::Side side);
  void   AddVisualizationUpright(ChSuspension::Side side);
};


} // end namespace chrono


#endif
