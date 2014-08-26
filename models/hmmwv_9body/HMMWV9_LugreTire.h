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
// Authors: Radu Serban
// =============================================================================
//
// HMMWV LuGre tire subsystem
//
// =============================================================================

#ifndef HMMWV9_LUGRETIRE_H
#define HMMWV9_LUGRETIRE_H


#include "subsys/tire/ChLugreTire.h"

namespace hmmwv9 {

class HMMWV9_LugreTire : public chrono::ChLugreTire {
public:
  HMMWV9_LugreTire(const chrono::ChTerrain& terrain);
  ~HMMWV9_LugreTire() {}

  virtual int getNumDiscs() const                { return m_numDiscs; }
  virtual double getRadius() const               { return m_radius; }
  virtual const double* getDiscLocations() const { return m_discLocs; }

private:
  static const double  m_radius;
  static const int     m_numDiscs = 3;
  static const double  m_discLocs[m_numDiscs];
};


} // end namespace hmmwv9


#endif
