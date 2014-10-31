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
// Authors: Justin Madsen
// =============================================================================
//
// Suspension testing mechanism, using force or motion inputs to the locked wheels
//  to simulate the effect of a post-testing mechanism
//
// The Irrlicht interface used to observe the suspension test, and also to provide
// any steering inputs.
//
// The vehicle reference frame has Z up, X towards the front of the vehicle, and
// Y pointing to the left.
//
// =============================================================================

#include <vector>

#include "core/ChFileutils.h"
#include "core/ChStream.h"
#include "core/ChRealtimeStep.h"
#include "physics/ChSystem.h"
#include "physics/ChLinkDistance.h"

#include "utils/ChUtilsInputOutput.h"
#include "utils/ChUtilsData.h"

#include "models/ModelDefs.h"
#include "models/testing_mechanisms/HMMWV_SuspensionTest.h"

#include "models/hmmwv/tire/HMMWV_RigidTire.h"
#include "subsys/terrain/FlatTerrain.h"
#include "models/hmmwv/HMMWV_FuncDriver.h"

// If Irrlicht support is available...
#if IRRLICHT_ENABLED
  // ...include additional headers
# include "unit_IRRLICHT/ChIrrApp.h"
# include "subsys/driver/ChIrrGuiST.h"

  // ...and specify whether the demo should actually use Irrlicht
# define USE_IRRLICHT
#endif

// DEBUGGING:  Uncomment the following line to print shock data
//#define DEBUG_LOG

using namespace chrono;
using namespace hmmwv;

// =============================================================================

// Initial vehicle position
ChVector<> initLoc(0, 0, 1.0);

// Initial vehicle orientation
ChQuaternion<> initRot(1, 0, 0, 0);

// Simulation step size
double step_size = 0.001;

// Time interval between two render frames
double render_step_size = 1.0 / 50;   // FPS = 50

// Time interval between two output frames
double output_step_size = 1.0 / 1;    // once a second

#ifdef USE_IRRLICHT
  // Point on chassis tracked by the camera
  ChVector<> trackPoint(0.0, 0.0, 1.75);
#else
  double tend = 20.0;

  const std::string out_dir = "../HMMWV";
  const std::string pov_dir = out_dir + "/POVRAY";
#endif

// =============================================================================

int main(int argc, char* argv[])
{
  SetChronoDataPath(CHRONO_DATA_DIR);

  // Create the testing mechanism
  HMMWV_SuspensionTest tester(MESH);

  tester.Initialize(ChCoordsys<>(initLoc, initRot));


  // Create and initialize two rigid wheels
  ChSharedPtr<ChTire> tire_front_right;
  ChSharedPtr<ChTire> tire_front_left;


  // flat rigid terrain, height = 0 for all (x,y)
  FlatTerrain flat_terrain(0);

  ChSharedPtr<HMMWV_RigidTire> tire_FL(new HMMWV_RigidTire("FL", flat_terrain, 0.7f));
  ChSharedPtr<HMMWV_RigidTire> tire_FR(new HMMWV_RigidTire("FR", flat_terrain, 0.7f));
   
  tire_FL->Initialize(tester.GetWheelBody(FRONT_LEFT));
  tire_FR->Initialize(tester.GetWheelBody(FRONT_RIGHT));
   
  tire_front_left = tire_FL;
  tire_front_right = tire_FR;

#ifdef USE_IRRLICHT
  irr::ChIrrApp application(&tester,
                            L"HMMWV Suspension test",
                            irr::core::dimension2d<irr::u32>(1000, 800),
                            false,
                            true);

  // make a skybox that has Z pointing up (default application.AddTypicalSky() makes Y up) 
  std::string mtexturedir = GetChronoDataFile("skybox/");
  std::string str_lf = mtexturedir + "sky_lf.jpg";
  std::string str_up = mtexturedir + "sky_up.jpg";
  std::string str_dn = mtexturedir + "sky_dn.jpg";
  irr::video::ITexture* map_skybox_side = 
      application.GetVideoDriver()->getTexture(str_lf.c_str());
  irr::scene::ISceneNode* mbox = application.GetSceneManager()->addSkyBoxSceneNode(
      application.GetVideoDriver()->getTexture(str_up.c_str()),
      application.GetVideoDriver()->getTexture(str_dn.c_str()),
      map_skybox_side,
      map_skybox_side,
      map_skybox_side,
      map_skybox_side);
  mbox->setRotation( irr::core::vector3df(90,0,0));
 
  bool do_shadows = true; // shadow map is experimental
  irr::scene::ILightSceneNode* mlight = 0;

  if (do_shadows)
  {
    mlight = application.AddLightWithShadow(
      irr::core::vector3df(10.f, 30.f, 60.f),
      irr::core::vector3df(0.f, 0.f, 0.f),
      150, 60, 80, 15, 512, irr::video::SColorf(1, 1, 1), false, false);
  }
  else
  {
    application.AddTypicalLights(
      irr::core::vector3df(30.f, -30.f, 100.f),
      irr::core::vector3df(30.f, 50.f, 100.f),
      250, 130);
  }

  application.SetTimestep(step_size);

  ChIrrGuiST driver(application, tester, trackPoint, 6.0, 0.5, true);

  // Set the time response for steering and throttle keyboard inputs.
  // NOTE: this is not exact, since we do not render quite at the specified FPS.
  double steering_time = 1.0;  // time to go from 0 to +1 (or from 0 to -1)
  driver.SetSteeringDelta(render_step_size / steering_time);

  // Set up the assets for rendering
  application.AssetBindAll();
  application.AssetUpdateAll();
  if (do_shadows)
  {
    application.AddShadowAll();
  }
#else
  HMMWV_FuncDriver driver;
#endif


  // ---------------
  // Simulation loop
  // ---------------

#ifdef DEBUG_LOG
  GetLog() << "\n\n============ System Configuration ============\n";
  vehicle.LogHardpointLocations();
#endif

  // Inter-module communication data
  ChWheelState  wheel_states[2];
  double        steering_input;

  // Number of simulation steps between two 3D view render frames
  int render_steps = (int)std::ceil(render_step_size / step_size);

  // Number of simulation steps between two output frames
  int output_steps = (int)std::ceil(output_step_size / step_size);

  // Initialize simulation frame counter and simulation time
  int step_number = 0;
  double time = 0;

#ifdef USE_IRRLICHT

  ChRealtimeStepTimer realtime_timer;

  while (application.GetDevice()->run())
  {
    // update the position of the shadow mapping so that it follows the car
    if (do_shadows)
    {
      ChVector<> lightaim = tester.GetChassisPos();
      ChVector<> lightpos = tester.GetChassisPos() + ChVector<>(10, 30, 60);
      irr::core::vector3df mlightpos((irr::f32)lightpos.x, (irr::f32)lightpos.y, (irr::f32)lightpos.z);
      irr::core::vector3df mlightaim((irr::f32)lightaim.x, (irr::f32)lightaim.y, (irr::f32)lightaim.z);
      application.GetEffects()->getShadowLight(0).setPosition(mlightpos);
      application.GetEffects()->getShadowLight(0).setTarget(mlightaim);
      mlight->setPosition(mlightpos);
    }

    // Render scene
    if (step_number % render_steps == 0) {
      application.GetVideoDriver()->beginScene(true, true, irr::video::SColor(255, 140, 161, 192));
      driver.DrawAll();
      application.GetVideoDriver()->endScene();
    }

#ifdef DEBUG_LOG
    if (step_number % output_steps == 0) {
      GetLog() << "\n\n============ System Information ============\n";
      GetLog() << "Time = " << time << "\n\n";
      vehicle.DebugLog(DBG_SPRINGS | DBG_SHOCKS | DBG_CONSTRAINTS);
    }
#endif

    // Collect output data from modules (for inter-module communication)
    steering_input = driver.GetSteering();

  
    wheel_states[FRONT_LEFT.id()] = tester.GetWheelState(FRONT_LEFT);
    wheel_states[FRONT_RIGHT.id()] = tester.GetWheelState(FRONT_RIGHT);


    // Update modules (process inputs from other modules)
    time = vehicle.GetChTime();

    driver.Update(time);

    terrain.Update(time);

    tire_front_left->Update(time, wheel_states[FRONT_LEFT.id()]);
    tire_front_right->Update(time, wheel_states[FRONT_RIGHT.id()]);
    tire_rear_left->Update(time, wheel_states[REAR_LEFT.id()]);
    tire_rear_right->Update(time, wheel_states[REAR_RIGHT.id()]);

    powertrain->Update(time, throttle_input, driveshaft_speed);

    vehicle.Update(time, steering_input, braking_input, powertrain_torque, tire_forces);

    // Advance simulation for one timestep for all modules
    double step = realtime_timer.SuggestSimulationStep(step_size);

    driver.Advance(step);

    terrain.Advance(step);

    tire_front_left->Advance(step);
    tire_front_right->Advance(step);
    tire_rear_right->Advance(step);
    tire_rear_left->Advance(step);

    powertrain->Advance(step);

    vehicle.Advance(step);

    // Increment frame number
    step_number++;
  }

  application.GetDevice()->drop();

#else

  int render_frame = 0;

  if(ChFileutils::MakeDirectory(out_dir.c_str()) < 0) {
    std::cout << "Error creating directory " << out_dir << std::endl;
    return 1;
  }
  if(ChFileutils::MakeDirectory(pov_dir.c_str()) < 0) {
    std::cout << "Error creating directory " << pov_dir << std::endl;
    return 1;
  }

  vehicle.ExportMeshPovray(out_dir);

  char filename[100];

  while (time < tend)
  {
    if (step_number % render_steps == 0) {
      // Output render data
      sprintf(filename, "%s/data_%03d.dat", pov_dir.c_str(), render_frame + 1);
      utils::WriteShapesPovray(&vehicle, filename);
      std::cout << "Output frame:   " << render_frame << std::endl;
      std::cout << "Sim frame:      " << step_number << std::endl;
      std::cout << "Time:           " << time << std::endl;
      std::cout << "             throttle: " << driver.GetThrottle() << " steering: " << driver.GetSteering() << std::endl;
      std::cout << std::endl;
      render_frame++;
    }

#ifdef DEBUG_LOG
    if (step_number % output_steps == 0) {
      GetLog() << "\n\n============ System Information ============\n";
      GetLog() << "Time = " << time << "\n\n";
      vehicle.DebugLog(DBG_SHOCKS);
    }
#endif

    // Collect output data from modules (for inter-module communication)
    throttle_input = driver.GetThrottle();
    steering_input = driver.GetSteering();
    braking_input = driver.GetBraking();

    powertrain_torque = powertrain->GetOutputTorque();

    tire_forces[FRONT_LEFT.id()] = tire_front_left->GetTireForce();
    tire_forces[FRONT_RIGHT.id()] = tire_front_right->GetTireForce();
    tire_forces[REAR_LEFT.id()] = tire_rear_left->GetTireForce();
    tire_forces[REAR_RIGHT.id()] = tire_rear_right->GetTireForce();

    driveshaft_speed = vehicle.GetDriveshaftSpeed();

    wheel_states[FRONT_LEFT.id()] = vehicle.GetWheelState(FRONT_LEFT);
    wheel_states[FRONT_RIGHT.id()] = vehicle.GetWheelState(FRONT_RIGHT);
    wheel_states[REAR_LEFT.id()] = vehicle.GetWheelState(REAR_LEFT);
    wheel_states[REAR_RIGHT.id()] = vehicle.GetWheelState(REAR_RIGHT);

    // Update modules (process inputs from other modules)
    time = vehicle.GetChTime();

    driver.Update(time);

    terrain.Update(time);

    tire_front_left->Update(time, wheel_states[FRONT_LEFT.id()]);
    tire_front_right->Update(time, wheel_states[FRONT_RIGHT.id()]);
    tire_rear_left->Update(time, wheel_states[REAR_LEFT.id()]);
    tire_rear_right->Update(time, wheel_states[REAR_RIGHT.id()]);

    powertrain->Update(time, throttle_input, driveshaft_speed);

    vehicle.Update(time, steering_input, braking_input, powertrain_torque, tire_forces);

    // Advance simulation for one timestep for all modules
    driver.Advance(step_size);

    terrain.Advance(step_size);

    tire_front_right->Advance(step_size);
    tire_front_left->Advance(step_size);
    tire_rear_right->Advance(step_size);
    tire_rear_left->Advance(step_size);

    powertrain->Advance(step_size);

    vehicle.Advance(step_size);

    // Increment frame number
    step_number++;
  }

#endif

  return 0;
}
