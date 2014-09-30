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
// Base class for a Pacjeka type Magic formula 2002 tire model
//
// =============================================================================

#ifndef CH_PACEJKATIRE_H
#define CH_PACEJKATIRE_H

#include <string>
#include <fstream>

#include "physics/ChBody.h"

#include "subsys/ChTire.h"
#include "subsys/ChTerrain.h"

namespace chrono {

// Forward declarations for private structures
struct slips;
struct Pac2002_data;
struct pureLongCoefs;
struct pureLatCoefs;
struct pureTorqueCoefs;
struct combinedLongCoefs;
struct combinedLatCoefs;
struct combinedTorqueCoefs;
struct zetaCoefs;
struct relaxationL;

///
/// Concrete tire class that implements the Pacejka tire model.
/// Detailed description goes here...
///
class CH_SUBSYS_API ChPacejkaTire : public ChTire
{
public:

  /// Default constructor for a Pacejka tire.
  /// Construct a Pacejka tire for which the vertical load is calculated
  /// internally.  The model includes transient slip calculations.
  ChPacejkaTire(
    const std::string& pacTire_paramFile,
    const ChTerrain&   terrain
    );

  /// Construct a Pacejka tire with specified vertical load.
  ChPacejkaTire(
    const std::string& pacTire_paramFile,
    const ChTerrain&   terrain,
    double             Fz_override,
    bool               use_transient_slip = true
    );

  /// Copy constructor, only tire side will be different
  ChPacejkaTire(
    const ChPacejkaTire& tire,   ///< [in] source object
    ChWheelId            which   ///< [in] 
    );

  ~ChPacejkaTire();
  /// return the reactions for the combined slip EQs, in global coords
  virtual ChTireForce GetTireForce() const;

  ///  Return the reactions for the pure slip EQs, in local or global coords
  ChTireForce GetTireForce_pureSlip(const bool local = true) const;

  /// Return the reactions for the combined slip EQs, in local or global coords
  ChTireForce GetTireForce_combinedSlip(const bool local = true) const;

  /// Update the state of this tire system at the current time.
  /// Set the PacTire spindle state data from the global wheel body state.
  virtual void Update(
    double               time,          ///< [in] current time
    const ChWheelState&  wheel_state    ///< [in] current state of associated wheel body
    );

  /// Advance the state of this tire by the specified time step.
  /// Use the new body state, calculate all the relevant quantities over the
  /// time increment.
  virtual void Advance(double step);

  /// Write output data to a file.
  void WriteOutData(
    double             time,
    const std::string& outFilename
    );

  /// Manually set the vertical wheel load as an input.
  void set_Fz_override(double Fz) { m_Fz_override = Fz; }

  /// Return orientation, Vx (global) and omega/omega_y (global).
  /// Assumes the tire is going straight forward (global x-dir), and the
  /// returned state's orientation yields gamma and alpha, as x and z NASA angles
  ChWheelState getState_from_KAG(
    double kappa,   ///< [in] ...
    double alpha,   ///< [in] ...
    double gamma,   ///< [in] ...
    double Vx      ///< [in] tire forward velocity x-dir
    );

  /// Return kappa, alpha, gamma.
  ChVector<> getKAG_from_State(const ChWheelState& state);

  /// Get current long slip rate.
  double get_kappa() const;

  /// Get current slip angle.
  double get_alpha() const;

  /// Get current camber angle
  double get_gamma() const;

  /// Get minimum longitudinal slip rate.
  double get_min_long_slip() const;

  /// Get maximum longitudinal slip rate.
  double get_max_long_slip() const;

  /// Get the minimum allowable lateral slip angle, alpha.
  double get_min_lat_slip() const;

  /// Get the maximum allowable lateral slip angle, alpha.
  double get_max_lat_slip() const;

  /// Get the longitudinal velocity.
  double get_longvl() const;

  /// Get the tire rolling radius, ideally updated each step.
  double get_tire_rolling_rad() const { return m_R_eff; }

  /// Set the value of the integration step size.
  void SetStepsize(double val) { m_step_size = val; }

  /// Get the current value of the integration step size.
  double GetStepsize() const { return m_step_size; }

private:

  // where to find the input parameter file
  const std::string& getPacTireParamFile() const { return m_paramFile; }

  // specify the file name to read the Pactire input from
  void Initialize();

  // look for this data file
  virtual void loadPacTireParamFile();

  // once Pac tire input text file has been succesfully opened, read the input
  // data, and populate the data struct
  virtual void readPacTireInput(std::ifstream& inFile);

  // functions for reading each section in the paramter file
  void readSection_UNITS(std::ifstream& inFile);
  void readSection_MODEL(std::ifstream& inFile);
  void readSection_DIMENSION(std::ifstream& inFile);
  void readSection_SHAPE(std::ifstream& inFile);
  void readSection_VERTICAL(std::ifstream& inFile);
  void readSection_RANGES(std::ifstream& inFile);
  void readSection_scaling(std::ifstream& inFile);
  void readSection_longitudinal(std::ifstream& inFile);
  void readSection_overturning(std::ifstream& inFile);
  void readSection_lateral(std::ifstream& inFile);
  void readSection_rolling(std::ifstream& inFile);
  void readSection_aligning(std::ifstream& inFile);

  // update the wheel state, and associated variables;
  void update_stateVars(const ChWheelState& state);

  // update the tire coordinate system with new global ChWheelState data
  void update_tireFrame();

  // calculate the various stiffness/relaxation lengths
  void calc_relaxationLengths();

  // calculate the slips assume the steer/throttle/brake input to wheel has an
  // instantaneous effect on contact patch slips
  void calc_slip_kinematic();

  // calculate transient slip properties, using first order ODEs to find slip
  // displacements from velocities
  void advance_slip_transient(double step_size);

  // calculate the increment delta_x using RK 45 integration
  double calc_ODE_RK_uv(
    double V_s,          // slip velocity
    double sigma,        // either sigma_kappa or sigma_alpha
    double V_cx,         // wheel body center velocity
    double step_size,    // the simulation timestep size
    double x_curr);      // f(x_curr)

  // calculate the increment delta_gamma of the gamma ODE
  double calc_ODE_RK_gamma(
    double C_Fgamma,
    double C_Falpha,
    double sigma_alpha,
    double V_cx,
    double step_size,
    double gamma,
    double v_gamma);

  // calculate the ODE dphi/dt = f(phi), return the increment delta_phi
  double calc_ODE_RK_phi(
    double C_Fphi,
    double C_Falpha,
    double V_cx,
    double psi_dot,
    double w_y,
    double gamma,
    double sigma_alpha,
    double v_phi,
    double eps_gamma,
    double step_size);

  // calculate and set the transient slip values (kappaP, alphaP, gammaP) from
  // u, v deflections
  void calc_slip_from_uv();

  /// calculate the reaction forces and moments, pure slip cases
  /// assign longitudinal, lateral force, aligning moment:
  /// Fx, Fy and Mz
  void calc_pureSlipReactions();

  /// calculate combined slip reactions
  /// assign Fx, Fy, Mz
  void calc_combinedSlipReactions();

  // calculate the current effective rolling radius, w.r.t. wy, Fz as inputs
  void calc_rho(double F_z);

  /// calculate the longitudinal force, alpha ~= 0
  /// assign to m_FM.force.x
  /// assign m_pureLong, trionometric function calculated constants
  void calcFx_pureLong();

  /// calculate the lateral force,  kappa ~= 0
  /// assign to m_FM.force.y
  /// assign m_pureLong, trionometric function calculated constants
  void calcFy_pureLat();

  // find the vertical load, based on the wheel center and ground height
  void calc_Fz();

  /// calculate the aligning moment,  kappa ~= 0
  /// assign to m_FM.force.z
  /// assign m_pureLong, trionometric function calculated constants
  void calcMz_pureLat();

  /// calculate longitudinal force, combined slip (general case)
  /// assign m_FM_combined.force.x
  /// assign m_combinedLong
  void calcFx_combined();

  /// calculate lateral force, combined slip (general case)
  /// assign m_FM_combined.force.y
  /// assign m_combinedLat
  void calcFy_combined();

  // calculate aligning torque, combined slip (gernal case)
  /// assign m_FM_combined.moment.z
  /// assign m_combinedTorque
  void calcMz_combined();

  /// calculate the overturning couple moment
  /// assign m_FM.moment.x and m_FM_combined.moment.x
  void calc_Mx();

  /// calculate the rolling resistance moment,
  /// assign m_FM.moment.y and m_FM_combined.moment.y
  void calc_My();

  // ----- Data members

  bool m_use_transient_slip;

  ChWheelState m_tireState;     // current tire state
  ChCoordsys<> m_tire_frame;    // current tire coordinate system

  double m_R0;              // unloaded radius
  double m_R_eff;           // current effect rolling radius
  double m_R_l;             // relaxation length
  double m_rho;             // vertical deflection w.r.t. R0

  double m_dF_z;            // (Fz - Fz,nom) / Fz,nom
  bool m_use_Fz_override;   // calculate Fz using collision, or user input
  double m_Fz_override;     // if manually inputting the vertical wheel load

  double m_step_size;       // integration step size

  // OUTPUTS

  ChTireForce m_FM;            // based on pure slip
  ChTireForce m_FM_combined;   // based on combined slip

  std::string m_paramFile;     // input parameter file
  std::string m_outFilename;   // output filename

  int m_Num_WriteOutData;      // number of times WriteOut was called

  bool m_params_defined;       // indicates if model params. have been defined/loaded

  // MODEL PARAMETERS

  // important slip quantities
  slips*               m_slip;

  // model parameter factors stored here
  Pac2002_data*        m_params;

  // for keeping track of intermediate factors in the PacTire model
  pureLongCoefs*       m_pureLong;
  pureLatCoefs*        m_pureLat;
  pureTorqueCoefs*     m_pureTorque;

  combinedLongCoefs*   m_combinedLong;
  combinedLatCoefs*    m_combinedLat;
  combinedTorqueCoefs* m_combinedTorque;

  zetaCoefs*           m_zeta;

  // for transient contact point tire model
  relaxationL*         m_relaxation;

};


} // end namespace chrono


#endif
