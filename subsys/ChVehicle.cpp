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
// Base class for a vehicle model.
//
// =============================================================================

#include <algorithm>

#include "subsys/ChVehicle.h"


namespace chrono {


// -----------------------------------------------------------------------------
// Constructor for a ChVehicle: specify default step size and solver parameters.
// -----------------------------------------------------------------------------
ChVehicle::ChVehicle()
: m_stepsize(1e-3)
{
  Set_G_acc(ChVector<>(0, 0, -9.81));

  // Integration and Solver settings
  SetLcpSolverType(ChSystem::LCP_ITERATIVE_SOR);
  SetIterLCPmaxItersSpeed(150);
  SetIterLCPmaxItersStab(150);
  SetMaxPenetrationRecoverySpeed(4.0);
}


// -----------------------------------------------------------------------------
// Advance the state of the system, taking as many steps as needed to exactly
// reach the specified value 'step'.
// ---------------------------------------------------------------------------- -
void ChVehicle::Advance(double step)
{
  double t = 0;
  while (t < step) {
    double h = std::min<>(m_stepsize, step - t);
    DoStepDynamics(h);
    t += h;
  }
}

// -----------------------------------------------------------------------------
// Return the complete state (expressed in the global frame) for the specified
// wheel body.
// -----------------------------------------------------------------------------
ChBodyState ChVehicle::GetWheelState(ChWheelId which)
{
  ChBodyState state;

  state.pos = GetWheelPos(which);
  state.rot = GetWheelRot(which);
  state.lin_vel = GetWheelLinVel(which);
  state.ang_vel = GetWheelAngVel(which);

  return state;
}


}  // end namespace chrono
