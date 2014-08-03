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
// Base class for all suspension subsystems
//
// =============================================================================


#include "ChSuspension.h"


namespace chrono {


ChSuspension::ChSuspension(const std::string& name,
                           bool               driven)
: m_name(name),
  m_driven(driven)
{
}


}  // end namespace chrono
