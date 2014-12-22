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
// Simple flat rigid terrain
//
// =============================================================================

#include "physics/ChBody.h"
#include "physics/ChBodyEasy.h"
#include "assets/ChColorAsset.h"
#include "assets/ChTexture.h"

#include "utils/ChUtilsCreators.h"
#include "utils/ChUtilsData.h"

#include "subsys/terrain/RigidTerrain.h"


namespace chrono {

// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
RigidTerrain::RigidTerrain(ChSystem&  system,
                           double     height,
                           double     sizeX,
                           double     sizeY,
                           double     mu,
                           const std::string road_file)
: m_system(system),
  m_height(height),
  m_sizeX(sizeX),
  m_sizeY(sizeY)
{
  ChSharedPtr<ChBody> ground(new ChBody);

  ground->SetIdentifier(-1);
  ground->SetName("ground");
  ground->SetPos(ChVector<>(0, 0, height));
  ground->SetRot(ChQuaternion<>(1, 0, 0, 0));
  ground->SetBodyFixed(true);
  ground->SetCollide(true);

  double hDepth = 10;

  ground->GetCollisionModel()->ClearModel();
  utils::AddBoxGeometry(ground.get_ptr(),
    ChVector<>(sizeX / 2, sizeY / 2, hDepth),
    ChVector<>(0, 0, -hDepth));
  ground->GetCollisionModel()->BuildModel();

  // if the user did not specify a texture to use for the ground
  if( road_file == "none"){
    ChSharedPtr<ChColorAsset> groundColor(new ChColorAsset);
    groundColor->SetColor(ChColor(0.4f, 0.4f, 0.6f));
    ground->AddAsset(groundColor);
  } else {
    AddTexture(road_file, ground);
  }

  system.AddBody(ground);

}

void RigidTerrain::AddMovingObstacles(int numObstacles)
{
  for (int i = 0; i < numObstacles; i++) {
    double o_sizeX = 1.0 + 3.0 * ChRandom();
    double o_sizeY = 0.3 + 0.2 * ChRandom();
    double o_sizeZ = 0.05 + 0.1 * ChRandom();
    ChSharedPtr<ChBodyEasyBox> obstacle(new ChBodyEasyBox(o_sizeX, o_sizeY, o_sizeZ, 2000.0, true, true));
    
    double o_posX = (ChRandom() - 0.5)*0.6*m_sizeX;
    double o_posY = (ChRandom() - 0.5)*0.6*m_sizeY;
    double o_posZ = m_height + 4;
    ChQuaternion<> rot(ChRandom(), ChRandom(), ChRandom(), ChRandom());
    rot.Normalize();
    obstacle->SetPos(ChVector<>(o_posX, o_posY, o_posZ));
    obstacle->SetRot(rot);

    m_system.AddBody(obstacle);
  }
}

void RigidTerrain::AddFixedObstacles()
{
  double radius = 3;
  double length = 10;
  ChSharedPtr<ChBodyEasyCylinder> obstacle(new ChBodyEasyCylinder(radius, length, 2000, true, true));

  obstacle->SetPos(ChVector<>(-20, 0, -2.7));
  obstacle->SetBodyFixed(true);

  m_system.AddBody(obstacle);

  for (int i= 0; i< 8; ++i) {
    ChSharedPtr<ChBodyEasyBox> stoneslab(new ChBodyEasyBox(0.5, 1.5, 0.2, 2000, true, true));
    stoneslab->SetPos(ChVector<>(-1.2*i + 22, -1, -0.05));
    stoneslab->SetRot(Q_from_AngAxis(15 * CH_C_DEG_TO_RAD, VECT_Y));
    stoneslab->SetBodyFixed(true);
    m_system.AddBody(stoneslab);
  }
}


void RigidTerrain::AddTexture(const std::string& filename, ChSharedPtr<ChBody> body)
{
  ChSharedPtr<ChTexture> tex(new ChTexture);
  tex->SetTextureFilename( utils::GetModelDataFile (filename));
  body->AddAsset(tex);
}


} // end namespace chrono
