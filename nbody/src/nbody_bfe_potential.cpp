#include "nbody_bfe_potential.h"
#include <memory>
#include <Eigen/Eigen>
#include "BiorthBasis.H"

/*
  These are the structures we will use in our C++ code, to enable us to call EXP.
*/
struct exp_bfe_timestep_t {
    /* Each BiorthBasis is for a particular time */
    bfe_real time = std::numeric_limits<bfe_real>::quiet_NaN();
    /* The acceleration for this contribution should be scaled by this weight */
    bfe_real weight = std::numeric_limits<bfe_real>::quiet_NaN();
    /* This is the raw data that EXP uses to compute acceleration and density. */
    std::shared_ptr<BasisClasses::BiorthBasis> bfe;
};

struct exp_bfe_t {
    /* Interpolate values in aTimesteps to get values for time target_time */
    bfe_real target_time = std::numeric_limits<bfe_real>::quiet_NaN();
    std::vector<std::shared_ptr<exp_bfe_timestep_t> > aTimesteps;
};

/*
  exp_bfe_open: Given the location of the Milkyway@Home master configuration file (YAML) describing
  the desired BFE use, interpret the instructions in the YAML file and call to EXP to load in the
  relevant data.
*/
exp_bfe_t *exp_bfe_open(const char *yaml_filename)
{
    // Write me!!!
    // Should we silently do nothing if yaml_filename is NULL or ""?!!!
    return nullptr;
}

/*
  exp_bfe_get_acceleration: Given a position in space `xyz` (note that xyz.w is ignored) and a time,
  use the BFE functionality and coefficients provided by EXP to compute the acceleration due to
  gravity.
*/
mwvector exp_bfe_get_acceleration(exp_bfe_t *exp_bfe, mwvector xyz, bfe_real t)
{
    mwvector response = ZERO_VECTOR;
    if (!exp_bfe) {
	// Should we silently do nothing if exp_bfe is NULL?!!!
	return response;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    if (exp_bfe->target_time != t) {
#pragma GCC diagnostic pop
	exp_bfe->target_time = t;
	// Write me!!!
	// Figure out which timesteps we will need BFE coefficients for, and figure out their weights.
	// From exp_bfe->aTimesteps, delete uneeded timesteps.
	// To exp_bfe->aTimesteps, add needed timesteps.
	// In exp_bfe->aTimesteps, set weights
    }
    Eigen::Vector3d accel{ Eigen::Vector3d::Zero() };
    Eigen::Vector3d accelContribution;
    for (const auto &timestep : exp_bfe->aTimesteps) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
	if (timestep && timestep->weight != 0.0) {
#pragma GCC diagnostic pop
	    accelContribution = timestep->bfe->getAccel(xyz.x, xyz.y, xyz.z);
	    accel += accelContribution * timestep->weight;
	}
    }
    response = { accel.x(), accel.y(), accel.z(), 0.0 };
    return response;
}

/*
  exp_bfe_get_dark_density: Given a position in space `xyz` (note that xyz.w is ignored) and a time,
  use the BFE functionality and coefficients provided by EXP to compute the dark-matter density.
*/
bfe_real exp_bfe_get_dark_density(exp_bfe_t *exp_bfe, mwvector xyz, bfe_real t)
{
    // Write me!!!
    return 0.0;
}

void exp_bfe_close(exp_bfe_t *exp_bfe)
{
    if (!exp_bfe) {
	// Nothing to free
	return;
    }
    // Write me!!!
    return;
}
