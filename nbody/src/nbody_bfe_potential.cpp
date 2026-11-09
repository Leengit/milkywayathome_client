#include "nbody_bfe_potential.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include <Eigen/Eigen>
#include <H5Cpp.h>
#include "BiorthBasis.H"

// We don't need to define MW_H5FileHandle!!!
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
struct exp_bfe_t {
    // Struct that contains information about each component
    struct exp_bfe_component_t {
	std::string component_name;
	const YAML::Node component_node;
	std::shared_ptr<BasisClasses::BiorthBasis> basis;
	const std::shared_ptr<CoefClasses::Coefs> coefs;
    };
    std::vector<exp_bfe_component_t> exp_bfe_components;
    bfe_real target_time = std::numeric_limits<bfe_real>::quiet_NaN();
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
    std::unique_ptr<exp_bfe_t> exp_bfe = std::unique_ptr<exp_bfe_t>(new exp_bfe_t);
    try
    {
	// Load in the configuration file
	const YAML::Node config_node = YAML::LoadFile(config_filename);

	// Get information from Global
	const YAML::Node global_node = config_node["Global"];
	const std::string runtag = global_node["runtag"].as<std::string>();

	// Get information from Output
	const YAML::Node outputs_node = config_node["Output"];

	std::unordered_set<std::string> outcoeff_set;
	for(YAML::const_iterator it_outputs = outputs_node.begin();
	    it_outputs != outputs_node.end();
	    ++it_outputs)
	{
	    const YAML::Node output_node = *it_outputs;
	    const std::string output_id = output_node["id"].as<std::string>();
	    if (output_id == "outcoef") {
		const std::string output_name = output_node["parameters"]["name"].as<std::string>();
		outcoeff_set.insert(output_name);
	    }
	}

	// Get information from Components and save it for later
	const YAML::Node components_node = config_node["Components"];
	for(YAML::const_iterator it_components = components_node.begin();
	    it_components != components_node.end();
	    ++it_components)
	{
	    const YAML::Node component_node = *it_components;
	    const std::string component_name = component_node["name"].as<std::string>();
	    const std::unordered_set<std::string>::iterator it = outcoeff_set.find(component_name);
	    // Don't bother with this component if we did not output its outcoef.
	    if (it != outcoeff_set.end()) {
		// Instantiate a BiorthBasis structure
		const YAML::Node force_node = component_node["force"];
		const std::string force_id = force_node["id"].as<std::string>();
		std::shared_ptr<BasisClasses::BiorthBasis> basis = BasisClasses::BiorthBasis::factory(force_node);
		// Load in coefficents.
		// Or should we do this only when we know what timestep we are interested in?!!!
		std::filesystem::path outcoef_path = config_filename;
		outcoef_path.replace_filename("outcoef." + component_name + "." + runtag);
		const std::string outcoef_filename = outcoef_path.string();
		const std::shared_ptr<CoefClasses::Coefs> coefs = CoefClasses::Coefs::factory(outcoef_filename);
		// Save everything we've loaded for later use
		exp_bfe->exp_bfe_components.push_back({component_name, component_node, basis, coefs});
	    }
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

    // If this event has a new time then load the coefs into the basis
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    if (exp_bfe->target_time != t) {
#pragma GCC diagnostic pop
	exp_bfe->target_time = t;
	// Does this interpolate time for us?!!!  What about extrapolate?!!!
	for(std::vector<exp_bfe_t::exp_bfe_component_t>::iterator it_exp_bfe_components = exp_bfe->exp_bfe_components.begin();
	    it_exp_bfe_components != exp_bfe->exp_bfe_components.end();
	    ++it_exp_bfe_components)
	{
	    it_exp_bfe_components->basis->set_coefs(it_exp_bfe_components->coefs->getCoefStruct(t));
	}
    }

    // Ask for the acceleration based upon the basis and coefficients
    Eigen::Vector3d accel{ Eigen::Vector3d::Zero() };
    Eigen::Vector3d accelContribution;
    for(std::vector<exp_bfe_t::exp_bfe_component_t>::iterator it_exp_bfe_components = exp_bfe->exp_bfe_components.begin();
	it_exp_bfe_components != exp_bfe->exp_bfe_components.end();
	++it_exp_bfe_components)
    {
	accelContribution = it_exp_bfe_components->basis->getAccel(xyz.x, xyz.y, xyz.z);
	accel += accelContribution;
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
    delete exp_bfe; // This will call exp_bfe->exp_bfe_components.clear(), and recursively.

    return;
}
