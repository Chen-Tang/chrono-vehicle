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
// Authors: Radu Serban, Alessandro Tasora
// =============================================================================
//
// Articulated wheel subsystem
//
// =============================================================================

#ifndef ARTICULATED_WHEEL_H
#define ARTICULATED_WHEEL_H

#include "assets/ChCylinderShape.h"
#include "assets/ChTexture.h"

#include "subsys/ChWheel.h"
#include "subsys/ChVehicleModelData.h"

#include "models/ModelDefs.h"

class Articulated_Wheel : public chrono::ChWheel
{
public:

  Articulated_Wheel(VisualizationType  visType) : m_visType(visType) {}
  ~Articulated_Wheel() {}

  virtual double GetMass() const { return 45.4; }
  virtual chrono::ChVector<> GetInertia() const { return chrono::ChVector<>(0.113, 0.113, 0.113); }

  virtual void Initialize(chrono::ChSharedBodyPtr spindle)
  {
    // First, invoke the base class method
    chrono::ChWheel::Initialize(spindle);

    // Attach visualization
    if (m_visType == PRIMITIVES) {
      double radius = 0.47;
      double width = 0.25;
      chrono::ChSharedPtr<chrono::ChCylinderShape> cyl(new chrono::ChCylinderShape);
      cyl->GetCylinderGeometry().rad = radius;
      cyl->GetCylinderGeometry().p1 = chrono::ChVector<>(0, width / 2, 0);
      cyl->GetCylinderGeometry().p2 = chrono::ChVector<>(0, -width / 2, 0);
      spindle->AddAsset(cyl);

      chrono::ChSharedPtr<chrono::ChTexture> tex(new chrono::ChTexture);
      tex->SetTextureFilename(chrono::GetChronoDataFile("bluwhite.png"));
      spindle->AddAsset(tex);
    }
  }

private:

  VisualizationType  m_visType;
};


#endif
