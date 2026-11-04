#include "nbody_bfe_potential.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include <Eigen/Eigen>
#include "BiorthBasis.H"

/*
  Useful object for freeing H5 file handles
 */
class MW_H5FileHandle {
private:
    hid_t id_;

public:
    // Initialize to -1 by default
    MW_H5FileHandle() : id_(-1) {}

    // Construct from an existing HDF5 file ID
    explicit MW_H5FileHandle(hid_t id) : id_(id) {}

    // Destructor closes the file if the ID is valid
    ~MW_H5FileHandle() {
        if (this->id_ >= 0) {
            H5Fclose(this->id_);
        }
    }

    // Delete copy semantics to prevent double-free/double-close errors
    MW_H5FileHandle(const MW_H5FileHandle&) = delete;
    MW_H5FileHandle& operator=(const MW_H5FileHandle&) = delete;

    // Enable move semantics for safe ownership transfer
    MW_H5FileHandle(MW_H5FileHandle&& other) noexcept : id_(other.id_) {
        other.id_ = -1;
    }

    MW_H5FileHandle& operator=(MW_H5FileHandle&& other) noexcept {
        if (this != &other) {
            if (this->id_ >= 0) {
                H5Fclose(this->id_);
            }
            this->id_ = other.id_;
            other.id_ = -1;
        }
        return *this;
    }

    // Implicit conversion operator to behave seamlessly like a hid_t
    operator hid_t() const { return this->id_; }

    // Explicit getter for convenience
    hid_t get() const { return this->id_; }

    // Manually replace the handle
    void reset(hid_t new_id = -1) {
        if (this->id_ >= 0) {
            H5Fclose(this->id_);
        }
        this->id_ = new_id;
    }

    // Get the value and relinquish responsibility for garbage collection
    hid_t release() {
	return std::exchange(this->id_, -1);
    }
};

/*
  These are the structures we will use in our C++ code, to enable us to call EXP.
*/
struct exp_bfe_timestep_t {
    // Each BasisClasses::BiorthBasis is for a particular time
    bfe_real time = std::numeric_limits<bfe_real>::quiet_NaN();
    // The acceleration for this contribution should be scaled by this weight
    bfe_real weight = std::numeric_limits<bfe_real>::quiet_NaN();
    // This is the raw data that EXP uses to compute acceleration and density.
    std::shared_ptr<BasisClasses::BiorthBasis> bfe;
};

struct exp_bfe_t {
    // Struct that contains information about each component
    struct exp_bfe_component {
	// Remove any of these that we don't need!!!
	std::string name;
	std::string force_id;
	const YAML::Node* force_node;
	std::shared_ptr<BasisClasses::BiorthBasis> basis;
	std::string filename;
	MW_H5FileHandle file_id;
	MW_H5FileHandle dataset_id;
    };
    std::vector<exp_bfe_component> components;

    // Interpolate values in aTimesteps to get values for time target_time
    bfe_real target_time = std::numeric_limits<bfe_real>::quiet_NaN();
    std::vector<std::shared_ptr<exp_bfe_timestep_t> > aTimesteps;
};

/*
  exp_bfe_open: Given the location of the Milkyway@Home master configuration file (YAML) describing
  the desired BFE use, interpret the instructions in the YAML file and call to EXP to load in the
  relevant data.
*/
exp_bfe_t *exp_bfe_open(const char *config_filename)
{
    // We will return exp_bfe as an ordinary pointer if there is no exception thrown.
    // If there is an error it will instead be automatically deleted.
    auto exp_bfe = std::unique_ptr<exp_bfe_t>(new exp_bfe_t);
    try
    {
	// Load in the configuration file
	const YAML::Node config_node = YAML::LoadFile(config_filename);

	// Get information from Global
	const YAML::Node& global_node = config_node["Global"];
	const std::string runtag = global_node["runtag"].as<std::string>();

	// Get information from Output
	const YAML::Node& outputs_node = config_node["Output"];

	std::unordered_set<std::string> outcoeff_set;
	for(YAML::const_iterator it_outputs = outputs_node.begin();
	    it_outputs != outputs_node.end();
	    ++it_outputs)
	{
	    const YAML::Node& output_node = *it_outputs;
	    const std::string output_id = output_node["id"].as<std::string>();
	    if (output_id == "outcoef") {
		const std::string output_name = output_node["parameters"]["name"].as<std::string>();
		outcoeff_set.insert(output_name);
	    }
	}

	// Get information from Components and save it later
	const YAML::Node& components_node = config_node["Components"];
	for(YAML::const_iterator it_components = components_node.begin();
	    it_components != components_node.end();
	    ++it_components)
	{
	    const YAML::Node& component_node = *it_components;
	    const std::string component_name = component_node["name"].as<std::string>();
	    const std::unordered_set<std::string>::iterator it = outcoeff_set.find(component_name);
	    // Don't bother with this component if we did not output its outcoef.
	    if (it != outcoeff_set.end()) {
		// Instantiate a BiorthBasis structure
		const YAML::Node& force_node = component_node["force"];
		const std::string force_id = force_node["id"].as<std::string>();
		std::shared_ptr<BasisClasses::BiorthBasis> basis = BasisClasses::BiorthBasis::factory(force_node);
		// Load in coefficents.
		// Or should we do this only when we know what timestep we are interested in?!!!
		std::filesystem::path outcoef_path = config_filename;
		outcoef_path.replace_filename("outcoef." + component_name + "." + runtag);
		const std::string outcoef_filename = outcoef_path.string();
		MW_H5FileHandle file_id(H5Fopen(outcoef_filename.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT));
		if (file_id < 0) {
		    throw YAML::Exception(YAML::Mark::null_mark(), "Error opening coefficients file " + outcoef_filename);
		}
		MW_H5FileHandle dataset_id(H5Dopen2(file_id, "/coefficients", H5P_DEFAULT));
		if (dataset_id < 0) {
		    throw YAML::Exception(YAML::Mark::null_mark(), "Error opening coefficients dataset file " + outcoef_filename + "/coefficients");
		}
		// Save everything we've loaded for later use
		exp_bfe->components.push_back({component_name, force_id, &force_node, basis, outcoef_filename, std::move(file_id), std::move(dataset_id)});
	    }

	    // Needed???!!! const int max_l = config_node["bfe"]["max_l"].as<int>();

	    // 3. Extract the coefficient vector corresponding to each target time snapshot
	    std::vector<double> aTimes; // (time index)
	    std::vector<std::vector<double>> aCoefficients; // (time index, coef index)
	}

    } catch (const YAML::Exception& e) {
	std::cerr << "Error parsing YAML file "
		  << (config_filename ? '\'' + std::string(config_filename) + '\'' : "(NULL)")
		  << ": " << e.what() << std::endl;
	throw;
    }

    return exp_bfe.release();
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
  exp_bfe_get_density: Given a position in space `xyz` (note that xyz.w is ignored) and a time,
  use the BFE functionality and coefficients provided by EXP to compute the density.
*/
bfe_real exp_bfe_get_density(exp_bfe_t *exp_bfe, mwvector xyz, bfe_real t)
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
