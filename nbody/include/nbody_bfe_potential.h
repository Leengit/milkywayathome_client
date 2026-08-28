#ifndef _NBODY_BFE_POTENTIAL_H_
#define _NBODY_BFE_POTENTIAL_H_

#ifdef __cplusplus
/*
  Note: this next section is an unfortunate but apparently necessary hack.

  We need to know how milkyway_math defines the `real` type.  However, both C++ and Milkyway@Home
  define `real` so we avoid using "real" in our C++ code by using `bfe_real` instead.

  We cannot include milkyway_math.h, within extern "C" or not, because, either way, it causes
  collisions between C and C++.  That is why we are are duplicating code found there and in its
  dependencies.  Fortunately, we can include milkyway_config.h.  We are checking DOUBLEPREC (and
  OUBLEPREC) directly and explicitly defining bfe_real, mwvector, and ZERO_VECTOR.
*/
extern "C" {
#include "milkyway_config.h"
#ifdef OUBLEPREC
    #define DOUBLEPREC OUBLEPREC
#endif
#ifndef DOUBLEPREC
    #error DOUBLEPREC not defined
#endif
#if DOUBLEPREC
    typedef double bfe_real;
#else
    typedef float bfe_real;
#endif
typedef struct MW_ALIGN_TYPE_V(4 * sizeof(bfe_real))
{
    bfe_real x, y, z, w;
} mwvector;
#define ZERO_VECTOR { 0.0, 0.0, 0.0, 0.0 }
} /* end of extern "C" */
#else  /* #ifndef __cplusplus */
/*
  This file is being included in code that is not C++, just ordinary C.  We can safely include
  milkyway_math.h.  Most of what we use is directly from milkyway_math.h.  However, we use the
  `real` that we get from there to create the `bfe_real` that we will use.
*/
#include "milkyway_math.h"
typedef real bfe_real;
#endif /* __cplusplus */

/*
  Now that we have fetched (if C) or simulated (if C++) types provided by milkyway_math.h for
  Milkyway@Home, we can define the interface that makes use of them.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
 * Note that struct exp_bfe_t is declared here, but it is deliberately not defined here, so that C
 * code that tries to know about its internals will not compile!
*/
struct exp_bfe_t;
typedef struct exp_bfe_t exp_bfe_t;

/*
  exp_bfe_open: Given the location of the Milkyway@Home master configuration file (YAML) describing
  the desired BFE use, interpret the instructions in the YAML file and call to EXP to load in the
  relevant data.
*/
exp_bfe_t *exp_bfe_open(const char *yaml_filename);

/*
  exp_bfe_get_acceleration: Given a position in space `xyz` (note that xyz.w is ignored) and a time,
  use the BFE functionality and coefficients provided by EXP to compute the acceleration due to
  gravity.
*/
mwvector exp_bfe_get_acceleration(exp_bfe_t *exp_bfe, mwvector xyz, bfe_real t);

/*
  exp_bfe_get_density: Given a position in space `xyz` (note that xyz.w is ignored) and a time,
  use the BFE functionality and coefficients provided by EXP to compute the density.
*/
bfe_real exp_bfe_get_density(exp_bfe_t *exp_bfe, mwvector xyz, bfe_real t);

/*
  exp_bfe_close: The BFE information is no longer needed, clean up and release it.
*/
void exp_bfe_close(exp_bfe_t *exp_bfe);

#ifdef __cplusplus
} /* end of extern "C" */
#endif /* __cplusplus */

#endif /* _NBODY_BFE_POTENTIAL_H_ */
