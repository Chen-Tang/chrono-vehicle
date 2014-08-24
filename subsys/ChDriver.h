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
// Base class for a vehicle driver. A driver object must be able to report the
// current values of the inputs (throttle, steering, braking). To set these
// values, a concrete driver class can implement the virtual method Update()
// which will be invoked at each time step.
//
// =============================================================================

#ifndef CH_DRIVER_H
#define CH_DRIVER_H

#include "core/ChShared.h"
#include "physics/ChSystem.h"

#include "subsys/ChApiSubsys.h"

namespace chrono {


class CH_SUBSYS_API ChDriver : public ChShared
{
public:
  ChDriver();
  virtual ~ChDriver() {}

  double getThrottle() const { return m_throttle; }
  double getSteering() const { return m_steering; }
  double getBraking() const  { return m_braking; }

  virtual void Update(double time) {}
  virtual void Advance(double step) {}

protected:
  void setSteering(double val, double min_val = -1, double max_val = 1);
  void setThrottle(double val, double min_val = 0, double max_val = 1);
  void setBraking(double val, double min_val = 0, double max_val = 1);

  double m_throttle;
  double m_steering;
  double m_braking;

};


} // end namespace chrono


#endif