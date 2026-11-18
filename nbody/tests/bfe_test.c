/* This program tests the Basis Function Expansion (bfe) code that we
   implement via the EXP public library.

   * Tests include:
   * 1. abcd

   * We test the whole process starting from parsing the LUA file.
   * This finds the location of the YML file that was written out by
   * the EXP run that created the BFE files; this file now serves as a
   * description of the output produced by EXP and allows us to read
   * in that EXP output.

   * Subsequent steps read in the coefficients (for the basis function
   * expansions) and use them to compute a contribution to the
   * acceleration of each particle at each time step of the n-body
   * simulation.

   */

// Do we need all these?!!!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "nbody_bfe_potential.h"
#include "nbody_mixeddwarf.h"
#include "nbody_particle_data.h"
#include "nbody_potential_types.h"
#include "test_env_util.h"

/* Struct to hold all simulation variables and allocations */
typedef struct {
    char* input_lua_file;
    real* baryon_bin_centers;
    real* dark_bin_centers;
    real* baryon_bin_edges;
    real* dark_bin_edges;
    real* baryon_theoretical_density;
    real* dark_theoretical_density;
    real* baryon_theoretical_probability;
    real* dark_theoretical_probability;
    real* baryon_simulation_density;
    real* dark_simulation_density;
    real* baryon_simulation_probability;
    real* dark_simulation_probability;
    ParticleCollection* particle_data;
    real nbody;
    real nbody_baryon;
    real nbody_dark;
    Dwarf* comp1;
    Dwarf* comp2;
    real timestep;
    real mass_per_particle_baryon;
    real mass_per_particle_dark;
    real baryon_bin_width;
    real dark_bin_width;
    int num_bins_baryon;
    int num_bins_dark;

} TestContext;

/* Clean all memory allocated by the program */
static void cleanup_all_memory(TestContext* tctx) {
    if (!tctx) return;
    free(tctx->input_lua_file);
    free(tctx->baryon_bin_centers);
    free(tctx->dark_bin_centers);
    free(tctx->baryon_bin_edges);
    free(tctx->dark_bin_edges);
    free(tctx->baryon_theoretical_density);
    free(tctx->dark_theoretical_density);
    free(tctx->baryon_theoretical_probability);
    free(tctx->dark_theoretical_probability);
    free(tctx->baryon_simulation_density);
    free(tctx->dark_simulation_density);
    free(tctx->baryon_simulation_probability);
    free(tctx->dark_simulation_probability);
    free_particle_collection(tctx->particle_data);
    free(tctx->comp1);
    free(tctx->comp2);
    memset(tctx, 0, sizeof(TestContext));
}

/* Dwarf galaxy parameters */
/* These scale radius and mass parameters are known in the group as Sidd's parameters */
/* Optimized for a Plummer-Plummer model */
#define EVOLUTION_TIME "2.0"                  /* Evolution time in Gyr */
#define EVOLUTION_RATIO "0.0"                 /* Evolution time ratio */
#define BARYON_SCALE_RADIUS "0.2"             /* Baryon Scale radius in kpc */
#define SCALE_RADIUS_RATIO "0.2"              /* Scale radius ratio */
#define BARYON_MASS "12.0"                    /* Baryon Mass in SMU */
#define MASS_RATIO "0.2"                      /* Mass ratio */

/* Run the nbody simulation for a given bfe potential type */
int run_nbody_simulation(const char* bfe_potential_type_lua, TestContext* tctx) {
    int failed = 0;
    if (!tctx) {
	failed = 1;
	return failed;
    }
    memset(tctx, 0, sizeof(TestContext));

    const char* default_params[] = {
	EVOLUTION_TIME,
	EVOLUTION_RATIO,
	BARYON_SCALE_RADIUS,
	SCALE_RADIUS_RATIO,
	BARYON_MASS,
	MASS_RATIO
    };
    const char** bfe_params;
    bfe_params = default_params;

    // Clean up any output files from previous runs
    printf("Cleaning up any output files from previous runs...\n");
    fflush(stdout);
    const char* initial_output_file = "initial.out";
    remove(initial_output_file); // Ignore errors
    char output_filename[1024];
    for (int i = 0; i < 10000; i++) {
        snprintf(output_filename, sizeof(output_filename), "%d", i);
        remove(output_filename); // Ignore errors
    }
    const char* final_output_files[] = {"output.out", "output.hist"};
    for (int i = 0; i < 2; i++) {
        remove(final_output_files[i]); // Ignore errors
    }
    // Find the lua file  /* Note find_lua_file hardcodes mixeddwarf_models!!! */
    tctx->input_lua_file = find_lua_file(bfe_potential_type_lua);
    if (!tctx->input_lua_file) {
        fprintf(stderr, "Error: Could not find %s.lua in any expected location\n", bfe_potential_type_lua);
        failed = 1;
        return failed;
    }
    printf("Input file path: %s\n", tctx->input_lua_file);
    fflush(stdout);
    // Read the parameters from the Lua file
    printf("Reading Lua parameters...\n");
    fflush(stdout);
    lua_State* lua_state_out;  // Ask for state, so that it isn't closed / freed
    if (read_lua_parameters(tctx->input_lua_file, bfe_params, &tctx->nbody, &tctx->nbody_baryon, &tctx->comp1, &tctx->comp2, &tctx->timestep, &lua_state_out) != 0) {
	fprintf(stderr, "Error: Failed to read Lua parameters\n");
	fflush(stderr);
	failed = 1;
	return failed;
    }
    // Run a simulation
    printf("Running N-body simulation...\n");
    fflush(stdout);
    if (run_nbody(bfe_params, tctx->input_lua_file) != 0) {
	fprintf(stderr, "Error: N-body simulation failed\n");
	fflush(stderr);
	failed = 1;
	return failed;
    }
    printf("N-body simulation completed successfully\n");
    fflush(stdout);
    return failed;
}

int main() {

    int total_failed = 0;

    // Arrays to store failed models and their failed tests
    const char* failed_models[7];
    const char* failed_tests[7][3];
    int num_failed_tests[7];
    int num_failures = 0;

    // List of bfe models to test
    const char* bfe_models[] = {
	"bfe.lua"
	    };

    // Number of models to test
    const int num_models = sizeof(bfe_models) / sizeof(bfe_models[0]);

    // Test each model
    for (int i = 0; i < num_models; i++) {
	int failed = 0;
	int test_idx = 0;

	printf("\n=== Testing %s ===\n", bfe_models[i]);
	TestContext tctx;
	memset(&tctx, 0, sizeof(TestContext));
	// Run the N-body simulation and fill context
	if (run_nbody_simulation(bfe_models[i], &tctx) != 0) {
	    fprintf(stderr, "Error: N-body simulation failed\n");
	    fflush(stderr);
	    cleanup_all_memory(&tctx);
	    return 1;
	}
	/*
	if (test_stability(&tctx) != 0) {
	    failed++;
	    failed_tests[num_failures][test_idx++] = "Stability test";
	}
	*/
	free_particle_collection(tctx.particle_data);
	printf("free_particle_collection is done.\n");
	fflush(stdout);
	tctx.particle_data = NULL;
	if (failed == 0) {
	    printf("%s passed all tests!\n", bfe_models[i]);
	} else {
	    printf("%s failed %d test(s)!\n", bfe_models[i], failed);
	    failed_models[num_failures] = bfe_models[i];
	    num_failed_tests[num_failures] = test_idx;
	    num_failures++;
	    total_failed += failed;
	}
	fflush(stdout);
	cleanup_all_memory(&tctx);
    }
    if(total_failed == 0)
	{
	    printf("\n=== SUMMARY ===\n");
	    printf("All bfe generation tests successful!\n");
	}
    else
	{
	    printf("\n=== SUMMARY ===\n");
	    printf("Failed %d test(s) across all models\n", total_failed);
	    for (int i = 0; i < num_failures; i++)
		{
		    printf("%s failed the following tests:\n", failed_models[i]);
		    for (int j = 0; j < num_failed_tests[i]; j++)
			{
			    printf("  - %s\n", failed_tests[i][j]);
			}
		}
	}
    fflush(stdout);
    return total_failed > 0 ? 1 : 0;
}
