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
// Authors: Mike Taylor, Radu Serban
// =============================================================================
//
// Test for revolute joint
//
// Recall that Irrlicht uses a left-hand frame, so everything is rendered with
// left and right flipped.
//
// =============================================================================
// TO DO:
//    Report test run time & test pass/fail (determine what the criteria is)
// =============================================================================

#include <ostream>
#include <fstream>

#include "core/ChFileutils.h"

#include "physics/ChSystem.h"
#include "physics/ChBody.h"

#include "unit_IRRLICHT/ChIrrApp.h"

#include "ChronoT_config.h"
#include "utils/ChUtilsData.h"
#include "utils/ChUtilsInputOutput.h"
#include "utils/ChUtilsValidation.h"

using namespace chrono;
using namespace irr;


// =============================================================================
//
// Utility function to create a CSV output stream and set output format options.
//
utils::CSV_writer OutStream()
{
  utils::CSV_writer out("\t");

  out.stream().setf(std::ios::scientific | std::ios::showpos);
  out.stream().precision(6);

  return out;
}


// =============================================================================

bool TestRevolute(const ChVector<>&     jointLoc,         // absolute location of joint
                  const ChQuaternion<>& jointRot,         // orientation of joint
                  double                simTimeStep,      // simulation time step
                  double                outTimeStep,      // output time step
                  const std::string&    outDir,           // output directory
                  const std::string&    testName,         // name of this test
                  bool                  animate)          // if true, animate with Irrlich
{

  // Settings
  //---------

  // There are no units in Chrono, so values must be consistent
  // (MKS is used in this example)

  double mass = 1.0;              // mass of pendulum
  double length = 4.0;            // length of pendulum
  ChVector<> inertiaXX(1, 1, 1);  // mass moments of inertia of pendulum (centroidal frame)
  double g = 9.80665;

  double timeRecord = 5;          // Stop recording to the file after this much simulated time

  // Create the mechanical system
  // ----------------------------

  // Create a ChronoENGINE physical system: all bodies and constraints will be
  // handled by this ChSystem object.

  ChSystem my_system;
  my_system.Set_G_acc(ChVector<>(0.0, 0.0, -g));

  my_system.SetIntegrationType(ChSystem::INT_ANITESCU);
  my_system.SetIterLCPmaxItersSpeed(100);
  my_system.SetIterLCPmaxItersStab(100); //Tasora stepper uses this, Anitescu does not
  my_system.SetLcpSolverType(ChSystem::LCP_ITERATIVE_SOR);

  // Create the ground body

  ChSharedBodyPtr  ground(new ChBody);
  my_system.AddBody(ground);
  ground->SetBodyFixed(true);
  // Add some geometry to the ground body for visualizing the revolute joint
  ChSharedPtr<ChCylinderShape> cyl_g(new ChCylinderShape);
  cyl_g->GetCylinderGeometry().p1 = jointLoc + jointRot.Rotate(ChVector<>(0, 0, -0.2));
  cyl_g->GetCylinderGeometry().p2 = jointLoc + jointRot.Rotate(ChVector<>(0, 0, 0.2));
  cyl_g->GetCylinderGeometry().rad = 0.1;
  ground->AddAsset(cyl_g);

  // Create the pendulum body, in an initial configuration at rest, aligned with
  // the global X axis. The pendulum CG is assumed to be at half its length.

  ChSharedBodyPtr  pendulum(new ChBody);
  my_system.AddBody(pendulum);
  pendulum->SetPos(jointLoc + ChVector<>(length / 2, 0, 0));
  pendulum->SetMass(mass);
  pendulum->SetInertiaXX(inertiaXX);
  // Add some geometry to the pendulum for visualization
  ChSharedPtr<ChCylinderShape> cyl_p(new ChCylinderShape);
  cyl_p->GetCylinderGeometry().p1 = ChVector<>(-length / 2, 0, 0);
  cyl_p->GetCylinderGeometry().p2 = ChVector<>(length / 2, 0, 0);
  cyl_p->GetCylinderGeometry().rad = 0.1;
  pendulum->AddAsset(cyl_p);

  // Create revolute joint between pendulum and ground at "loc" in the global
  // reference frame. The revolute joint's axis of rotation will be the Z axis
  // of the specified rotation matrix.

  ChSharedPtr<ChLinkLockRevolute>  revoluteJoint(new ChLinkLockRevolute);
  revoluteJoint->Initialize(pendulum, ground, ChCoordsys<>(jointLoc, jointRot));
  my_system.AddLink(revoluteJoint);

  // Perform the simulation (animation with Irrlicht)
  // ------------------------------------------------

  if (animate)
  {
    // Create the Irrlicht application for visualization
    ChIrrApp * application = new ChIrrApp(&my_system, L"ChLinkRevolute demo", core::dimension2d<u32>(800, 600), false, true);
    application->AddTypicalLogo();
    application->AddTypicalSky();
    application->AddTypicalLights();
    core::vector3df lookat((f32)jointLoc.x, (f32)jointLoc.y, (f32)jointLoc.z);
    application->AddTypicalCamera(lookat + core::vector3df(0, 3, -6), lookat);

    // Now have the visulization tool (Irrlicht) create its geometry from the
    // assets defined above
    application->AssetBindAll();
    application->AssetUpdateAll();

    application->SetTimestep(simTimeStep);

    // Simulation loop
    while (application->GetDevice()->run())
    {
      application->BeginScene();
      application->DrawAll();

      // Draw an XZ grid at the global origin to add in visualization
      ChIrrTools::drawGrid(
        application->GetVideoDriver(), 1, 1, 20, 20,
        ChCoordsys<>(ChVector<>(0, 0, 0), Q_from_AngX(CH_C_PI_2)),
        video::SColor(255, 80, 100, 100), true);

      application->DoStep();  //Take one step in time
      application->EndScene();
    }

    return true;
  }

  // Perform the simulation (record results)
  // ------------------------------------------------

  // Create the CSV_Writer output objects (TAB delimited)
  utils::CSV_writer out_pos = OutStream();
  utils::CSV_writer out_vel = OutStream();
  utils::CSV_writer out_acc = OutStream();

  utils::CSV_writer out_quat = OutStream();
  utils::CSV_writer out_avel = OutStream();
  utils::CSV_writer out_aacc = OutStream();

  utils::CSV_writer out_rfrc = OutStream();
  utils::CSV_writer out_rtrq = OutStream();

  utils::CSV_writer out_energy = OutStream();

  utils::CSV_writer out_cnstr = OutStream();

  // Write headers
  out_pos << "Time" << "X_Pos" << "Y_Pos" << "Z_Pos" << std::endl;
  out_vel << "Time" << "X_Vel" << "Y_Vel" << "Z_Vel" << std::endl;
  out_acc << "Time" << "X_Acc" << "Y_Acc" << "Z_Acc" << std::endl;

  out_quat << "Time" << "e0" << "e1" << "e2" << "e3" << std::endl;
  out_avel << "Time" << "X_AngVel" << "Y_AngVel" << "Z_AngVel" << std::endl;
  out_aacc << "Time" << "X_AngAcc" << "Y_AngAcc" << "Z_AngAcc" << std::endl;

  out_rfrc << "Time" << "X_Force" << "Y_Force" << "Z_Force" << std::endl;
  out_rfrc << "Time" << "X_Torque" << "Y_Torque" << "Z_Torque" << std::endl;

  out_energy << "Time" << "Total_KE" << "Transl_KE" << "Rot_KE" << "Delta_PE" << std::endl;

  out_cnstr << "Time" << "Cnstr_1" << "Cnstr_2" << "Cnstr_3" << "Constraint_4" << "Cnstr_5" << std::endl;

  // Simulation loop

  double simTime = 0;
  double outTime = 0;

  while (simTime <= timeRecord + simTimeStep / 2)
  {
    // Ensure that the final data point is recorded.
    if (simTime >= outTime - simTimeStep / 2)
    {

      // CM position, velocity, and acceleration (expressed in global frame).
      const ChVector<>& position = pendulum->GetPos();
      const ChVector<>& velocity = pendulum->GetPos_dt();
      out_pos << simTime << position << std::endl;
      out_vel << simTime << velocity << std::endl;
      out_acc << simTime << pendulum->GetPos_dtdt() << std::endl;

      // Orientation, angular velocity, and angular acceleration (expressed in
      // global frame).
      out_quat << simTime << pendulum->GetRot() << std::endl;
      out_avel << simTime << pendulum->GetWvel_par() << std::endl;
      out_aacc << simTime << pendulum->GetWacc_par() << std::endl;

      // Reaction Force and Torque
      // These are expressed in the link coordinate system. We convert them to
      // the coordinate system of Body2 (in our case this is the ground).
      ChCoordsys<> linkCoordsys = revoluteJoint->GetLinkRelativeCoords();
      ChVector<> reactForce = revoluteJoint->Get_react_force();
      ChVector<> reactForceGlobal = linkCoordsys.TransformDirectionLocalToParent(reactForce);
      out_rfrc << simTime << reactForceGlobal << std::endl;

      ChVector<> reactTorque = revoluteJoint->Get_react_torque();
      ChVector<> reactTorqueGlobal = linkCoordsys.TransformDirectionLocalToParent(reactTorque);
      out_rtrq << simTime << reactTorqueGlobal << std::endl;

      // Conservation of Energy
      // Translational Kinetic Energy (1/2*m*||v||^2)
      // Rotational Kinetic Energy (1/2 w'*I*w)
      // Delta Potential Energy (m*g*dz)
      ChMatrix33<> inertia = pendulum->GetInertia();
      ChVector<> angVelLoc = pendulum->GetWvel_loc();
      double transKE = 0.5 * mass * velocity.Length2();
      double rotKE = 0.5 * Vdot(angVelLoc, inertia * angVelLoc);
      double deltaPE = mass * g * (position.z - jointLoc.z);
      double totalKE = transKE + rotKE;
      out_energy << simTime << totalKE << transKE << rotKE << deltaPE << std::endl;;

      // Constraint violations
      ChMatrix<>* C = revoluteJoint->GetC();
      out_cnstr << simTime
                << C->GetElement(0, 0)
                << C->GetElement(1, 0)
                << C->GetElement(2, 0)
                << C->GetElement(3, 0)
                << C->GetElement(4, 0) << std::endl;

      // Increment output time
      outTime += outTimeStep;
    }

    // Advance simulation by one step
    my_system.DoStepDynamics(simTimeStep);

    // Increment simulation time
    simTime += simTimeStep;
  }

  // Write output files
  out_pos.write_to_file(outDir + testName + "_CHRONO_Pos.txt", testName + "\n\n");
  out_vel.write_to_file(outDir + testName + "_CHRONO_Vel.txt", testName + "\n\n");
  out_acc.write_to_file(outDir + testName + "_CHRONO_Acc.txt", testName + "\n\n");

  out_quat.write_to_file(outDir + testName + "_CHRONO_Quat.txt", testName + "\n\n");
  out_avel.write_to_file(outDir + testName + "_CHRONO_Avel.txt", testName + "\n\n");
  out_aacc.write_to_file(outDir + testName + "_CHRONO_Aacc.txt", testName + "\n\n");

  out_rfrc.write_to_file(outDir + testName + "_CHRONO_Rforce.txt", testName + "\n\n");
  out_rtrq.write_to_file(outDir + testName + "_CHRONO_Rtorque.txt", testName + "\n\n");

  out_energy.write_to_file(outDir + testName + "_CHRONO_Energy.txt", testName + "\n\n");

  out_cnstr.write_to_file(outDir + testName + "_CHRONO_Constraints.txt", testName + "\n\n");

  return true;
}

// =============================================================================

int main(int argc, char* argv[])
{
  bool animate = (argc > 1);

  // Set the path to the Chrono data folder
  // --------------------------------------

  SetChronoDataPath(CHRONO_DATA_DIR);

  // Create output directory (if it does not already exist)
  if (ChFileutils::MakeDirectory("../VALIDATION") < 0) {
    std::cout << "Error creating directory '../VALIDATION'" << std::endl;
    return 1;
  }
  if (ChFileutils::MakeDirectory("../VALIDATION/REVOLUTE_JOINT") < 0) {
    std::cout << "Error creating directory '../VALIDATION/REVOLUTE_JOINT'" << std::endl;
    return 1;
  }

  std::string out_dir = "../VALIDATION/REVOLUTE_JOINT/";
  std::string ref_dir = "validation/revolute_joint/";

  bool check;
  bool test_passed = true;

  // Case 1 - Joint at the origin, and aligned with the global Y axis
  // --------------------------------------------------------------------------

  // Note: the axis of rotation of a revolute joint is the Z-axis.
  // Therefore, the joint must be rotated -pi/2 about the global X-axis
  std::cout << "\nRevolute Test Case 01\n";
  TestRevolute(ChVector<>(0, 0, 0), Q_from_AngX(-CH_C_PI_2), 1e-3, 1e-2, out_dir, "Revolute_Case01", animate);

  // Validate positions
  check = utils::Validate(
    out_dir + "Revolute_Case01_CHRONO_Pos.txt",
    utils::GetModelDataFile(ref_dir + "Revolute_Case01_ADAMS_Pos.txt"),
    utils::RMS_NORM, 2e-2);
  test_passed &= check;
  std::cout << "   validate positions     " << (check ? "Passed" : "Failed") << std::endl;

  // Validate energy
  check = utils::Validate(
    out_dir + "Revolute_Case01_CHRONO_Energy.txt",
    utils::GetModelDataFile(ref_dir + "Revolute_Case01_ADAMS_Energy.txt"),
    utils::RMS_NORM, 2e-2);
  test_passed &= check;
  std::cout << "   validate energy        " << (check ? "Passed" : "Failed") << std::endl;

  // Validate constraint violations
  check = utils::Validate(
    out_dir + "Revolute_Case01_CHRONO_Constraints.txt",
    utils::RMS_NORM, 1e-5);
  test_passed &= check;
  std::cout << "   validate constraints   " << (check ? "Passed" : "Failed") << std::endl;

  // Case 2 - Joint at (1,2,3), and aligned with the global axis along Y = Z
  // -----------------------------------------------------------------------

  // Note: the axis of rotation of a revolute joint is the Z-axis.
  // Therefore, the joint must be rotated -pi/4 about the global X-axis
  std::cout << "\nRevolute Test Case 02\n";
  TestRevolute(ChVector<>(1, 2, 3), Q_from_AngX(-CH_C_PI_4), 1e-3, 1e-2, out_dir, "Revolute_Case02", animate);

  // Validate constraint violations
  check = utils::Validate(
    out_dir + "Revolute_Case01_CHRONO_Constraints.txt",
    utils::RMS_NORM, 1e-5);
  test_passed &= check;
  std::cout << "   validate constraints   " << (check ? "Passed" : "Failed") << std::endl;

  // Return 0 if all test passed and 1 otherwise
  // -------------------------------------------

  return !test_passed;
}
