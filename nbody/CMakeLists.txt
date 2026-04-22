# Copyright 2010-2011 Matthew Arsenault, Travis Desell, Dave Przybylo,
# Nathan Cole, Boleslaw Szymanski, Heidi Newberg, Carlos Varela, Malik
# Magdon-Ismail and Rensselaer Polytechnic Institute.
#
# This file is part of Milkway@Home.
#
# Milkyway@Home is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Milkyway@Home is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Milkyway@Home.  If not, see <http://www.gnu.org/licenses/>.
#

cmake_minimum_required(VERSION 2.6)
project(milkyway_nbody)

set(nbody_VERSION_MAJOR 1
          CACHE INTERNAL "N-body version number")
set(nbody_VERSION_MINOR 95
          CACHE INTERNAL "N-body version number")
set(nbody_VERSION "${nbody_VERSION_MAJOR}.${nbody_VERSION_MINOR}"
          CACHE INTERNAL "N-body version number")

include(MaybeDlCheck)
include(MWLink)
include(SpecialLibs)
include(PrintInfo)
include(CheckSSE2)
include(BOINCPaths)
include(BOINCBinNames)
include(CMakeDependentOption)

cmake_dependent_option(ENABLE_CURSES "Report progress with curses" ON
                                       "NOT BOINC_APPLICATION;CURSES_FOUND" OFF)

if(NBODY_GL)
  include_directories(${OPENGL_INCLUDE_DIR})
  include_directories(${GLFW_INCLUDE_DIR})
endif()

cmake_dependent_option(NBODY_STATIC "Build Nbody as fully static binary" ON
                                      "NOT NBODY_OPENCL;NOT ENABLE_CURSES" OFF)
mark_as_advanced(NBODY_STATIC)
maybe_static(${NBODY_STATIC})

include_directories(${MILKYWAY_INCLUDE_DIR})
include_directories(${MILKYWAY_INSTALL_INCLUDE_DIR})
include_directories(${DSFMT_INCLUDE_DIR})

if(ENABLE_CURSES)
  include_directories(${CURSES_INCLUDE_DIR})
endif()


set(NBODY_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include")
include_directories(${NBODY_INCLUDE_DIR})


set(CMAKE_POSITION_INDEPENDENT_CODE ON CACHE BOOL "" FORCE)
if(OPENMP_FOUND AND NBODY_OPENMP)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
elseif(NOT OPENMP_FOUND AND NBODY_OPENMP)
  message(WARNING "Did not find OpenMP support, but was enabled. Continuing without OpenMP")
endif()

if(NOT DOUBLEPREC AND NBODY_CRLIBM)
  message(WARNING "Attempting to use single precision with crlibm. Disabling use of crlibm")
  set(NBODY_CRLIBM OFF)
endif()

include(ExternalProject)

ExternalProject_Add(
  fftw
  URL https://www.fftw.org/fftw-3.3.10.tar.gz
  URL_HASH SHA256=56c932549852cddcfafdab3820b0200c7742675be92179e59e6215b340e26467
  CONFIGURE_COMMAND
    "<SOURCE_DIR>/configure"
    --disable-shared
    --enable-static
    --prefix=<INSTALL_DIR>
    --with-pic
    CFLAGS=-fPIC
    CXXFLAGS=-fPIC
    FCFLAGS=-fPIC
  BUILD_COMMAND make -j 20
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libfftw3.a
)
ExternalProject_Get_Property(fftw SOURCE_DIR)
set(MY_FFTW_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(fftw INSTALL_DIR)
set(MY_FFTW_INSTALL_DIR ${INSTALL_DIR})

ExternalProject_Add(
  openmpi
  DEPENDS fftw
  URL https://download.open-mpi.org/release/open-mpi/v5.0/openmpi-5.0.9.tar.bz2
  URL_HASH SHA256=dfb72762531170847af3e4a0f21d77d7b23cf36f67ce7ce9033659273677d80b
  CONFIGURE_COMMAND
    "<SOURCE_DIR>/configure"
    --disable-shared
    --enable-static
    --prefix=<INSTALL_DIR>
    --with-pic
    CFLAGS=-fPIC
    CXXFLAGS=-fPIC
    FCFLAGS=-fPIC
  BUILD_COMMAND make -j 20
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/libmpi_usempif08.a
    <INSTALL_DIR>/lib/libmpi_usempi_ignore_tkr.a
    <INSTALL_DIR>/lib/libmpi_mpifh.a
    <INSTALL_DIR>/lib/libmpi.a
    <INSTALL_DIR>/lib/libopen-pal.a
    <INSTALL_DIR>/lib/libprrte.a
    <INSTALL_DIR>/lib/libpmix.a
    <INSTALL_DIR>/lib/libhwloc.a
    <INSTALL_DIR>/lib/libevent_pthreads.a
    <INSTALL_DIR>/lib/libevent_extra.a
    <INSTALL_DIR>/lib/libevent_core.a
    <INSTALL_DIR>/lib/libevent.a
)
ExternalProject_Get_Property(openmpi SOURCE_DIR)
set(MY_OPENMPI_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(openmpi INSTALL_DIR)
set(MY_OPENMPI_INSTALL_DIR ${INSTALL_DIR})

ExternalProject_Add(
  boost
  DEPENDS openmpi fftw
  URL https://archives.boost.io/release/1.89.0/source/boost_1_89_0.tar.gz
  URL_HASH SHA256=9de758db755e8330a01d995b0a24d09798048400ac25c03fc5ea9be364b13c93
  CONFIGURE_COMMAND
    "<SOURCE_DIR>/bootstrap.sh"
    --prefix=<INSTALL_DIR>
    CFLAGS=-fPIC
    CXXFLAGS=-fPIC
    FCFLAGS=-fPIC
  BUILD_COMMAND
    "<SOURCE_DIR>/b2"
    --prefix=<INSTALL_DIR>
    --with-mpi
    cflags=-fPIC
    cxxflags=-fPIC
    link=static
    runtime-link=static
  INSTALL_COMMAND
    "<SOURCE_DIR>/b2"
    --prefix=<INSTALL_DIR>
    --with-mpi
    cflags=-fPIC
    cxxflags=-fPIC
    link=static
    runtime-link=static
    install
  BUILD_IN_SOURCE 1
)
ExternalProject_Get_Property(boost SOURCE_DIR)
set(MY_BOOST_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(boost INSTALL_DIR)
set(MY_BOOST_INSTALL_DIR ${INSTALL_DIR})

ExternalProject_Add(
  gmp
  DEPENDS boost openmpi fftw
  URL https://ftp.gnu.org/gnu/gmp/gmp-6.3.0.tar.xz
  URL_HASH SHA256=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898
  CONFIGURE_COMMAND
    "<SOURCE_DIR>/configure"
    --disable-shared
    --enable-static
    --prefix=<INSTALL_DIR>
    --with-pic
    CFLAGS=-fPIC
    CXXFLAGS=-fPIC
    FCFLAGS=-fPIC
  BUILD_COMMAND make -j 20
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libgmp.a
)
ExternalProject_Get_Property(gmp SOURCE_DIR)
set(MY_GMP_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(gmp INSTALL_DIR)
set(MY_GMP_INSTALL_DIR ${INSTALL_DIR})

ExternalProject_Add(
  mpfr
  DEPENDS gmp boost openmpi fftw
  URL https://www.mpfr.org/mpfr-current/mpfr-4.2.2.tar.gz
  URL_HASH SHA256=826cbb24610bd193f36fde172233fb8c009f3f5c2ad99f644d0dea2e16a20e42
  CONFIGURE_COMMAND touch "<SOURCE_DIR>/aclocal.m4" "<SOURCE_DIR>/acinclude.m4" "<SOURCE_DIR>/Makefile.in" "<SOURCE_DIR>/Makefile.am" && "<SOURCE_DIR>/configure"
    --disable-shared
    --enable-static
    --prefix=<INSTALL_DIR>
    --with-gmp=${MY_GMP_INSTALL_DIR}
    --with-pic
    CFLAGS=-fPIC
    CXXFLAGS=-fPIC
    FCFLAGS=-fPIC
  BUILD_COMMAND make -j 20
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libmpfr.a
)
ExternalProject_Get_Property(mpfr SOURCE_DIR)
set(MY_MPFR_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(mpfr INSTALL_DIR)
set(MY_MPFR_INSTALL_DIR ${INSTALL_DIR})

ExternalProject_Add(
  eigen
  DEPENDS mpfr gmp boost openmpi fftw
  URL https://gitlab.com/libeigen/eigen/-/archive/3.4.1/eigen-3.4.1.tar.gz
  URL_HASH SHA256=b93c667d1b69265cdb4d9f30ec21f8facbbe8b307cf34c0b9942834c6d4fdbe2
  CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/libeigen_lapack_static.a
    <INSTALL_DIR>/lib/libeigen_blas_static.a
)
ExternalProject_Get_Property(eigen SOURCE_DIR)
set(MY_EIGEN_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(eigen INSTALL_DIR)
set(MY_EIGEN_INSTALL_DIR ${INSTALL_DIR})

ExternalProject_Add(
  hdf5
  DEPENDS eigen mpfr gmp boost openmpi fftw
  URL https://github.com/HDFGroup/hdf5/releases/download/hdf5-1_12_3/hdf5-1_12_3.tar.gz
  URL_HASH SHA256=9fb7aefa8d3f9b8bc73ec7aeb1cfe61607931f0976ee42795a0172fe1762abaa
  CONFIGURE_COMMAND
    "<SOURCE_DIR>/configure"
    --disable-shared
    --enable-cxx
    --enable-fortran
    --enable-hl
    --enable-static
    --enable-static-exec
    --prefix=<INSTALL_DIR>
    --with-pic
    CFLAGS=-fPIC
    CXXFLAGS=-fPIC
    FCFLAGS=-fPIC
  BUILD_COMMAND make -j 20
  INSTALL_COMMAND make install
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/libhdf5_hl_fortran.a
    <INSTALL_DIR>/lib/libhdf5hl_fortran.a
    <INSTALL_DIR>/lib/libhdf5_hl_cpp.a
    <INSTALL_DIR>/lib/libhdf5_hl.a
    <INSTALL_DIR>/lib/libhdf5_fortran.a
    <INSTALL_DIR>/lib/libhdf5_cpp.a
    <INSTALL_DIR>/lib/libhdf5.a
)
ExternalProject_Get_Property(hdf5 SOURCE_DIR)
set(MY_HDF5_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(hdf5 INSTALL_DIR)
set(MY_HDF5_INSTALL_DIR ${INSTALL_DIR})

ExternalProject_Add(
  exp
  DEPENDS hdf5 eigen mpfr gmp boost openmpi fftw
  GIT_REPOSITORY https://github.com/EXP-code/EXP.git
  GIT_TAG devel
  LIST_SEPARATOR |
  CMAKE_ARGS
    "-DBUILD_SHARED_LIBS:BOOL=OFF"
    "-DBUILD_STATIC_LIBS:BOOL=ON"
    "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
    "-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON"
    "-DCMAKE_PREFIX_PATH:PATH=${MY_HDF5_INSTALL_DIR}|${MY_EIGEN_INSTALL_DIR}|${MY_MPFR_INSTALL_DIR}|${MY_GMP_INSTALL_DIR}|${MY_BOOST_INSTALL_DIR}|${MY_OPENMPI_INSTALL_DIR}|${MY_FFTW_INSTALL_DIR}"
    "-DENABLE_CUDA:BOOL=OFF"
    "-DENABLE_MINIMAL:BOOL=ON"
    "-DENABLE_PNG:BOOL=OFF"
    "-DBOOST_ROOT:PATH=${MY_BOOST_INSTALL_DIR}"
    "-DBoost_NO_SYSTEM_PATHS:BOOL=ON"
    "-DEigen3_DIR:PATH=${MY_EIGEN_INSTALL_DIR}"
    "-DEigen3_ROOT:PATH=${MY_EIGEN_INSTALL_DIR}"
    "-DFFTW_DIR:PATH=${MY_FFTW_INSTALL_DIR}"
    "-DFFTW_ROOT:PATH=${MY_FFTW_INSTALL_DIR}"
    "-DGMP_ROOT:PATH=${MY_GMP_INSTALL_DIR}"
    "-DHDF5_ROOT:PATH=${MY_HDF5_INSTALL_DIR}"
    "-DMPFR_ROOT:PATH=${MY_MPFR_INSTALL_DIR}"
    "-DMPI_HOME:PATH=${MY_OPENMPI_INSTALL_DIR}"
    "-DMPI_ROOT:PATH=${MY_OPENMPI_INSTALL_DIR}"
  BUILD_BYPRODUCTS
    <INSTALL_DIR>/lib/libexpui.a
    <INSTALL_DIR>/lib/libexputil.a
    <INSTALL_DIR>/lib/libyaml-cpp.a
)
ExternalProject_Get_Property(exp SOURCE_DIR)
set(MY_EXP_SOURCE_DIR ${SOURCE_DIR})
ExternalProject_Get_Property(exp INSTALL_DIR)
set(MY_EXP_INSTALL_DIR ${INSTALL_DIR})

print_separator()
message("   Building:          ${PROJECT_NAME}")
message("   BOINC Application: ${BOINC_APPLICATION}")
message("   Double precision:  ${DOUBLEPREC}")
message("   Enable crlibm:     ${NBODY_CRLIBM}")
message("  ----")
print_libs()
print_separator()

if(NBODY_OPENCL)
  set(nbody_inline_src "${LIBRARY_OUTPUT_PATH}/nbody_kernels_inline.c")
#   if(CrossCompile)
    add_custom_command(
      OUTPUT "${nbody_inline_src}"
      COMMAND xxd -i "nbody_kernels.cl" ${nbody_inline_src}
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/kernels"
      DEPENDS "${PROJECT_SOURCE_DIR}/kernels/nbody_kernels.cl"
      MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/kernels/nbody_kernels.cl"
      COMMENT "Inlining N-body kernel source")
#   else()
#     add_custom_command(
#       OUTPUT "${nbody_inline_src}"
#       COMMAND $<TARGET_FILE:xxd> -i "nbody_kernels.cl" ${nbody_inline_src}
#       WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/kernels"
#       DEPENDS "${PROJECT_SOURCE_DIR}/kernels/nbody_kernels.cl"
#       MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/kernels/nbody_kernels.cl"
#       COMMENT "Inlining N-body kernel source")
#   endif()
  add_library(nbody_kernels STATIC ${nbody_inline_src})
endif()

if(BOINC_APPLICATION AND NOT BOINC_LIBRARIES)
  message(FATAL_ERROR "Trying to build as a BOINC application, but the BOINC library wasn't found")
endif()



#CHECKME: We don't actually need special number math
#except for assignment in a couple places. Not sure how useful this is
#anyway.
#if(NOT ENABLE_FDLIBM)
#  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffinite-math-only")
#endif()

set(NBODY_SRC_DIR "${PROJECT_SOURCE_DIR}/src/")
set(NBODY_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include/")
set(nbody_lib_src ${NBODY_SRC_DIR}/nbody_chisq.c
                  ${NBODY_SRC_DIR}/nbody_grav.c
                  ${NBODY_SRC_DIR}/nbody_io.c
                  ${NBODY_SRC_DIR}/nbody_curses.c
                  ${NBODY_SRC_DIR}/nbody_types.c
                  ${NBODY_SRC_DIR}/nbody_tree.c
                  ${NBODY_SRC_DIR}/nbody_orbit_integrator.c
                  ${NBODY_SRC_DIR}/nbody_potential.c
                  ${NBODY_SRC_DIR}/nbody_bfe_potential.cpp
                  ${NBODY_SRC_DIR}/nbody_bessel.c
                  ${NBODY_SRC_DIR}/nbody_virial.c
                  ${NBODY_SRC_DIR}/nbody_density.c
                  ${NBODY_SRC_DIR}/nbody_friction.c
                  ${NBODY_SRC_DIR}/nbody.c
                  ${NBODY_SRC_DIR}/nbody_plain.c
                  ${NBODY_SRC_DIR}/nbody_check_params.c
                  ${NBODY_SRC_DIR}/nbody_isotropic.c
                  ${NBODY_SRC_DIR}/nbody_mixeddwarf.c
                  ${NBODY_SRC_DIR}/nbody_manual_bodies.c
                  ${NBODY_SRC_DIR}/nbody_dwarf_potential.c
                  ${NBODY_SRC_DIR}/nbody_plummer.c
                  ${NBODY_SRC_DIR}/nbody_nfw.c
                  ${NBODY_SRC_DIR}/nbody_hernq.c
                  ${NBODY_SRC_DIR}/nbody_show.c
                  ${NBODY_SRC_DIR}/nbody_checkpoint.c
                  ${NBODY_SRC_DIR}/nbody_defaults.c
                  ${NBODY_SRC_DIR}/nbody_coordinates.c
                  ${NBODY_SRC_DIR}/nbody_shmem.c
                  ${NBODY_SRC_DIR}/nbody_util.c
                  ${NBODY_SRC_DIR}/nbody_emd.c
                  ${NBODY_SRC_DIR}/nbody_mass.c
                  ${NBODY_SRC_DIR}/nbody_devoptions.c
                  ${NBODY_SRC_DIR}/nbody_likelihood.c
                  ${NBODY_SRC_DIR}/nbody_histogram.c
                  ${NBODY_SRC_DIR}/nbody_caustic.c
                  ${NBODY_SRC_DIR}/nbody_particle_data.c
                  ${NBODY_SRC_DIR}/blender_visualizer.c)

set(nbody_lib_headers ${NBODY_INCLUDE_DIR}/nbody_chisq.h
                      ${NBODY_INCLUDE_DIR}/nbody_grav.h
                      ${NBODY_INCLUDE_DIR}/nbody_config.h.in
                      ${NBODY_INCLUDE_DIR}/nbody_io.h
                      ${NBODY_INCLUDE_DIR}/nbody_curses.h
                      ${NBODY_INCLUDE_DIR}/nbody_tree.h
                      ${NBODY_INCLUDE_DIR}/nbody_orbit_integrator.h
                      ${NBODY_INCLUDE_DIR}/nbody_potential.h
                      ${NBODY_INCLUDE_DIR}/nbody_bfe_potential.h
                      ${NBODY_INCLUDE_DIR}/nbody_bessel.h
                      ${NBODY_INCLUDE_DIR}/nbody_virial.h
                      ${NBODY_INCLUDE_DIR}/nbody_density.h
                      ${NBODY_INCLUDE_DIR}/nbody_friction.h
                      ${NBODY_INCLUDE_DIR}/nbody_check_params.h
                      ${NBODY_INCLUDE_DIR}/nbody_isotropic.h
                      ${NBODY_INCLUDE_DIR}/nbody_mixeddwarf.h
                      ${NBODY_INCLUDE_DIR}/nbody_manual_bodies.h
                      ${NBODY_INCLUDE_DIR}/nbody_dwarf_potential.h
                      ${NBODY_INCLUDE_DIR}/nbody_plummer.h
                      ${NBODY_INCLUDE_DIR}/nbody_nfw.h
                      ${NBODY_INCLUDE_DIR}/nbody_hernq.h
                      ${NBODY_INCLUDE_DIR}/nbody.h
                      ${NBODY_INCLUDE_DIR}/nbody_plain.h
                      ${NBODY_INCLUDE_DIR}/nbody_show.h
                      ${NBODY_INCLUDE_DIR}/nbody_priv.h
                      ${NBODY_INCLUDE_DIR}/nbody_types.h
                      ${NBODY_INCLUDE_DIR}/nbody_checkpoint.h
                      ${NBODY_INCLUDE_DIR}/nbody_defaults.h
                      ${NBODY_INCLUDE_DIR}/nbody_coordinates.h
                      ${NBODY_INCLUDE_DIR}/nbody_shmem.h
                      ${NBODY_INCLUDE_DIR}/nbody_util.h
                      ${NBODY_INCLUDE_DIR}/nbody_graphics.h
                      ${NBODY_INCLUDE_DIR}/nbody_emd.h
                      ${NBODY_INCLUDE_DIR}/nbody_mass.h
                      ${NBODY_INCLUDE_DIR}/nbody_devoptions.h
                      ${NBODY_INCLUDE_DIR}/nbody_likelihood.h
                      ${NBODY_INCLUDE_DIR}/nbody_histogram.h
                      ${NBODY_INCLUDE_DIR}/nbody_caustic.h
                      ${NBODY_INCLUDE_DIR}/nbody_particle_data.h
                      ${NBODY_INCLUDE_DIR}/blender_visualizer.h)



if(NBODY_OPENCL)
  list(APPEND nbody_lib_src ${NBODY_SRC_DIR}/nbody_cl.c)
  list(APPEND nbody_lib_headers ${NBODY_INCLUDE_DIR}/nbody_cl.h)
endif()



set(nbody_lua_src ${NBODY_SRC_DIR}/nbody_lua.c
                  ${NBODY_SRC_DIR}/nbody_lua_models.c
                  ${NBODY_SRC_DIR}/nbody_lua_util.c
                  ${NBODY_SRC_DIR}/nbody_lua_misc.c

                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_types.c
                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_type_marshal.c

                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_nbodystate.c

                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_nbodyctx.c
                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_body.c
                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_halo.c
                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_disk.c
                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_spherical.c
		  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_bfe.c
                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_dwarf.c
                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_potential.c
                  ${NBODY_SRC_DIR}/nbody_lua_types/nbody_lua_histogram_params.c)



set(nbody_lua_headers ${NBODY_INCLUDE_DIR}/nbody_lua.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_models.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_util.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_misc.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_types.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_type_marshal.h

                      ${NBODY_INCLUDE_DIR}/nbody_lua_nbodystate.h

                      ${NBODY_INCLUDE_DIR}/nbody_lua_nbodyctx.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_body.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_halo.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_disk.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_spherical.h
		      ${NBODY_INCLUDE_DIR}/nbody_lua_bfe.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_dwarf.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_potential.h
                      ${NBODY_INCLUDE_DIR}/nbody_lua_histogram_params.h)


add_library(nbody_lua STATIC ${nbody_lua_src} ${nbody_lua_headers})
set_target_properties(nbody_lua PROPERTIES
                        COMPILE_DEFINITIONS "NBODY_LUA_TYPES_COMPILATION")
if(SYSTEM_IS_X86)
  enable_sse2(nbody_lua)
endif()

include_directories(${CMAKE_CURRENT_BINARY_DIR}/../boinc/boinc)

#TODO: The tests dSFMT has

# Require SSE and SSE2 (needed for doubles) This avoids a host of
# issues with x87, ranging from different intermediate precisions on
# different places, and removes many of the places where different
# compiler optimizations can change the precision.
#
# Some of these issues are summarized here: There are others to be
# aware of.
# http://www.yosefk.com/blog/consistency-how-to-defeat-the-purpose-of-ieee-floating-point.html
#
# Also as a nice advantage, SSE for a quick benchmark I ran is ~20%
# faster than using the antique x87 junk. There is also a later
# opportunity to do hand vectorization for an even bigger improvement.
#
# The GPU floating point stuff is
# "similar" to SSE, but not SSE.  OpenCL demands a stricter
# conformance to standards for doubles, so hopefully when we start
# using GPUs the results will be the same as using SSE/SSE2 and
# doubles. No promises however.
#
# Summary: we abandon old, old processors for the sake of convenience
# and consistency, and a nice speed boost on modern x86/x86_64
# hardware.
#

set(nbody_link_libs )

if(NBODY_BLENDER_OUTPUT)
  add_definitions("-DNBODY_BLENDER_OUTPUT")
endif()

if(NBODY_DEV_OPTIONS)
  add_definitions("-DNBODY_DEV_OPTIONS")
endif()

if(NBODY_OPENCL)
  include_directories(${OPENCL_INCLUDE_DIRS})
  list(APPEND nbody_link_libs ${OPENCL_LIBRARIES} nbody_kernels)
endif()

if(ENABLE_CURSES)
  list(APPEND nbody_link_libs ${CURSES_LIBRARIES})
endif()

if(HAVE_LIBRT)
  list(APPEND nbody_link_libs rt)
endif()

message(STATUS "MY_EXP_SOURCE_DIR = ${MY_EXP_SOURCE_DIR}")
message(STATUS "MY_HDF5_SOURCE_DIR = ${MY_HDF5_SOURCE_DIR}")
message(STATUS "MY_EIGEN_SOURCE_DIR = ${MY_EIGEN_SOURCE_DIR}")
message(STATUS "MY_MPFR_SOURCE_DIR = ${MY_MPFR_SOURCE_DIR}")
message(STATUS "MY_GMP_SOURCE_DIR = ${MY_GMP_SOURCE_DIR}")
message(STATUS "MY_BOOST_SOURCE_DIR = ${MY_BOOST_SOURCE_DIR}")
message(STATUS "MY_OPENMPI_SOURCE_DIR = ${MY_OPENMPI_SOURCE_DIR}")
message(STATUS "MY_FFTW_SOURCE_DIR = ${MY_FFTW_SOURCE_DIR}")
message(STATUS "MY_EXP_INSTALL_DIR = ${MY_EXP_INSTALL_DIR}")
message(STATUS "MY_HDF5_INSTALL_DIR = ${MY_HDF5_INSTALL_DIR}")
message(STATUS "MY_EIGEN_INSTALL_DIR = ${MY_EIGEN_INSTALL_DIR}")
message(STATUS "MY_MPFR_INSTALL_DIR = ${MY_MPFR_INSTALL_DIR}")
message(STATUS "MY_GMP_INSTALL_DIR = ${MY_GMP_INSTALL_DIR}")
message(STATUS "MY_BOOST_INSTALL_DIR = ${MY_BOOST_INSTALL_DIR}")
message(STATUS "MY_OPENMPI_INSTALL_DIR = ${MY_OPENMPI_INSTALL_DIR}")
message(STATUS "MY_FFTW_INSTALL_DIR = ${MY_FFTW_INSTALL_DIR}")

# Find the Python interpreter and development headers
find_package(Python COMPONENTS Interpreter Development REQUIRED)

add_library(nbody STATIC ${nbody_lib_src} ${nbody_lib_headers})
add_dependencies(nbody exp hdf5 eigen mpfr gmp boost openmpi fftw)
# In this order: exp hdf5 eigen mpfr gmp boost openmpi fftw
target_include_directories(nbody PRIVATE
    "${MY_EXP_INSTALL_DIR}/include"
    "${MY_EXP_INSTALL_DIR}/include/EXP"
    "${MY_EXP_SOURCE_DIR}/include"
    "${MY_EXP_SOURCE_DIR}/extern/yaml-cpp/include"
    "${MY_HDF5_INSTALL_DIR}/include"
    "${MY_EIGEN_INSTALL_DIR}/include"
    "${MY_EIGEN_INSTALL_DIR}/include/eigen3"
    "${MY_MPFR_INSTALL_DIR}/include"
    "${MY_GMP_INSTALL_DIR}/include"
    "${MY_BOOST_INSTALL_DIR}/include"
    "${MY_OPENMPI_INSTALL_DIR}/include"
    "${MY_FFTW_INSTALL_DIR}/include"
    "${Python_INCLUDE_DIRS}"
)
target_link_directories(nbody PRIVATE
    "${MY_EXP_INSTALL_DIR}/lib"
    "${MY_HDF5_INSTALL_DIR}/lib"
    "${MY_EIGEN_INSTALL_DIR}/lib"
    "${MY_MPFR_INSTALL_DIR}/lib"
    "${MY_GMP_INSTALL_DIR}/lib"
    "${MY_BOOST_INSTALL_DIR}/lib"
    "${MY_OPENMPI_INSTALL_DIR}/lib"
    "${MY_FFTW_INSTALL_DIR}/lib"
)
target_link_libraries(nbody
    "${MY_EXP_INSTALL_DIR}/lib/libexpui.a"
    "${MY_EXP_INSTALL_DIR}/lib/libexputil.a"
    "${MY_EXP_INSTALL_DIR}/lib/libyaml-cpp.a"
    "${MY_HDF5_INSTALL_DIR}/lib/libhdf5_hl_fortran.a"
    "${MY_HDF5_INSTALL_DIR}/lib/libhdf5hl_fortran.a"
    "${MY_HDF5_INSTALL_DIR}/lib/libhdf5_hl_cpp.a"
    "${MY_HDF5_INSTALL_DIR}/lib/libhdf5_hl.a"
    "${MY_HDF5_INSTALL_DIR}/lib/libhdf5_fortran.a"
    "${MY_HDF5_INSTALL_DIR}/lib/libhdf5_cpp.a"
    "${MY_HDF5_INSTALL_DIR}/lib/libhdf5.a"
    "${MY_EIGEN_INSTALL_DIR}/lib/libeigen_lapack_static.a"
    "${MY_EIGEN_INSTALL_DIR}/lib/libeigen_blas_static.a"
    "${MY_MPFR_INSTALL_DIR}/lib/libmpfr.a"
    "${MY_GMP_INSTALL_DIR}/lib/libgmp.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libmpi_usempif08.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libmpi_usempi_ignore_tkr.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libmpi_mpifh.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libmpi.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libopen-pal.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libprrte.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libpmix.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libhwloc.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libevent_pthreads.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libevent_extra.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libevent_core.a"
    "${MY_OPENMPI_INSTALL_DIR}/lib/libevent.a"
    "${MY_FFTW_INSTALL_DIR}/lib/libfftw3.a"
)
if(SYSTEM_IS_X86)
  enable_sse2(nbody)
endif()



set(nbody_exe_link_libs nbody
                        nbody_lua
                        milkyway_lua
                        nbody
                        milkyway

                        ${POPT_LIBRARY}
                        ${LUA_LIBRARIES}
                        ${OS_SPECIFIC_LIBS}
                        ${nbody_link_libs})

if(NBODY_OPENCL)
  list(INSERT nbody_exe_link_libs 5 milkyway_cl)
endif()



if(NBODY_CRLIBM)
  include_directories(${CRLIBM_INCLUDE_DIR})
  list(APPEND nbody_exe_link_libs ${CRLIBM_LIBRARY})
endif()

if(NOT HAVE_SSE2 AND SYSTEM_IS_x86)
  message(FATAL_ERROR "SSE2 is required for x86 systems")
endif()

add_executable(milkyway_nbody ${NBODY_SRC_DIR}/main.c)
milkyway_link(milkyway_nbody ${BOINC_APPLICATION} ${NBODY_STATIC} "${nbody_exe_link_libs}")

if(NBODY_GL AND BOINC_APPLICATION AND NOT BOINC_GRAPHICS_FOUND)
  message(FATAL "BOINC graphics library not found")
endif()

# Add a vertex, fragment shader pair
macro(add_inline_shader inline_src_list name type)
  set(inline_src "${LIBRARY_OUTPUT_PATH}/nbody_${name}_${type}_shaders_inline.c")
  add_custom_command(
    OUTPUT "${inline_src}"
    COMMAND xxd -i "${name}_${type}.glsl" ${inline_src}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/shaders"
    DEPENDS "${PROJECT_SOURCE_DIR}/shaders/${name}_${type}.glsl"
    MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/shaders/${name}_${type}.glsl"
    COMMENT "Inlining N-body GL ${type} shader source")

  list(APPEND ${inline_src_list} ${inline_src})
endmacro()

include_directories(${OPA_INCLUDE_DIR})

if(NBODY_GL)
  include_directories(${OPENGL_INCLUDE_DIR})
  include_directories(${GLFW_INCLUDE_DIR})

  set(nbody_screensaver_src ${NBODY_SRC_DIR}/nbody_gl.cpp
                            ${NBODY_SRC_DIR}/nbody_gl_util.cpp
                            ${NBODY_SRC_DIR}/nbody_gl_text.cpp
                            ${NBODY_SRC_DIR}/nbody_gl_axes.cpp
                            ${NBODY_SRC_DIR}/nbody_gl_orbit_trace.cpp
                            ${NBODY_SRC_DIR}/nbody_gl_galaxy_model.cpp
                            ${NBODY_SRC_DIR}/nbody_gl_particle_texture.cpp
                            ${NBODY_SRC_DIR}/MousePoles.cpp
                            ${NBODY_SRC_DIR}/graphics_main.c)

  set(nbody_screensaver_headers ${NBODY_INCLUDE_DIR}/nbody_gl.h
                                ${NBODY_INCLUDE_DIR}/nbody_gl_util.h
                                ${NBODY_INCLUDE_DIR}/nbody_graphics.h
                                ${NBODY_INCLUDE_DIR}/nbody_gl_text.h
                                ${NBODY_INCLUDE_DIR}/nbody_gl_axes.h
                                ${NBODY_INCLUDE_DIR}/nbody_gl_orbit_trace.h
                                ${NBODY_INCLUDE_DIR}/nbody_gl_galaxy_model.h
                                ${NBODY_INCLUDE_DIR}/nbody_gl_particle_texture.h
                                ${NBODY_INCLUDE_DIR}/nbody_gl_shaders.h
                                ${NBODY_INCLUDE_DIR}/nbody_milkyway_image.h
                                ${NBODY_INCLUDE_DIR}/MousePoles.h
                                ${NBODY_INCLUDE_DIR}/nbody_gl_private.h)

  if(APPLE)
    # Cocoa headers depend on stuff that Apple never merged back into
    # real GCC because they are terrible
    # So we need Apple GCC or recent clang
    list(APPEND nbody_screensaver_src ${NBODY_SRC_DIR}/nbody_gl_osx_main.m)
    set_source_files_properties(${NBODY_SRC_DIR}/nbody_gl_osx_main.m
                                LANGUAGE C
                                PROPERTIES COMPILE_FLAGS "-x objective-c")
  endif()

  set(vis_link_libs milkyway ${OPA_LIBRARY} ${POPT_LIBRARY} ${GLFW_LIBRARY} ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})
  if(BOINC_APPLICATION)
    list(APPEND vis_link_libs ${BOINC_GRAPHICS_LIBRARY})
  endif()

  if(HAVE_LIBRT)
    list(APPEND vis_link_libs rt)
  endif()

  if(USE_GL3W)
    list(APPEND vis_link_libs ${GL3W_LIBRARY})
    if(HAVE_LIBDL)
      list(APPEND vis_link_libs dl)
    endif()
  endif()

  if(NOT WIN32)
    list(APPEND vis_link_libs m)
  endif()

  set(nbody_gl_inline_srcs )
  add_inline_shader(nbody_gl_inline_srcs "particle" "vertex")
  add_inline_shader(nbody_gl_inline_srcs "particle_texture" "fragment")
  add_inline_shader(nbody_gl_inline_srcs "particle_point" "fragment")
  add_inline_shader(nbody_gl_inline_srcs "galaxy" "vertex")
  add_inline_shader(nbody_gl_inline_srcs "galaxy" "fragment")
  add_inline_shader(nbody_gl_inline_srcs "text" "vertex")
  add_inline_shader(nbody_gl_inline_srcs "text" "fragment")
  add_inline_shader(nbody_gl_inline_srcs "axes" "vertex")
  add_inline_shader(nbody_gl_inline_srcs "axes" "fragment")
  add_inline_shader(nbody_gl_inline_srcs "orbit_trace" "vertex")
  add_inline_shader(nbody_gl_inline_srcs "orbit_trace" "fragment")
  add_library(nbody_shaders STATIC ${nbody_gl_inline_srcs})

  set(milkyway_image_src "${LIBRARY_OUTPUT_PATH}/nbody_milkyway_image.c")
  add_custom_command(
    OUTPUT ${milkyway_image_src}
    COMMAND xxd -i "nbody_milkyway_image.bin" ${milkyway_image_src}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/images"
    DEPENDS "${PROJECT_SOURCE_DIR}/images/nbody_milkyway_image.bin"
    MAIN_DEPENDENCY "${PROJECT_SOURCE_DIR}/images/nbody_milkyway_image.bin"
    COMMENT "Inlining N-body Milkyway image")
  add_library(milkyway_image STATIC ${milkyway_image_src})


  #set(MACOSX_BUNDLE_GUI_IDENTIFIER "edu.rpi.cs.milkyway.\${PRODUCT_NAME:milkyway_nbody_graphics}")
  set(MACOSX_BUNDLE_GUI_IDENTIFIER "edu.rpi.cs.milkyway")
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${nbody_VERSION_MAJOR}.${nbody_VERSION}")
  set(MACOSX_BUNDLE_BUNDLE_VERSION 3.0)

  #add_executable(milkyway_nbody_graphics MACOSX_BUNDLE ${nbody_screensaver_src} ${nbody_screensaver_headers} )

  if(WIN32)
    list(APPEND nbody_screensaver_src ${NBODY_SRC_DIR}/graphics_main_win.c)
  endif()

  add_executable(milkyway_nbody_graphics  ${nbody_screensaver_src} ${nbody_screensaver_headers} )
  target_link_libraries(milkyway_nbody_graphics nbody_shaders milkyway_image ${vis_link_libs})
endif()



if(BOINC_RELEASE_NAMES)
  if(NBODY_OPENMP)
    set(NBODY_PLAN "mt")
  else()
    if(HAVE_SSE2)
      set(NBODY_PLAN "sse2")
    endif()
  endif()

  get_boinc_bin_name("milkyway_nbody" "${nbody_VERSION}" "${NBODY_PLAN}")
  set(NBODY_BIN_NAME "${BOINC_BIN_NAME}" CACHE INTERNAL PARENT_SCOPE)
  set_target_properties(milkyway_nbody
                          PROPERTIES
                            OUTPUT_NAME "${NBODY_BIN_NAME}")

  if(NBODY_GL)
    get_boinc_bin_name("milkyway_nbody_graphics" "${nbody_VERSION}" "")
    set(NBODY_GL_BIN_NAME "${BOINC_BIN_NAME}" CACHE INTERNAL PARENT_SCOPE)
    set_target_properties(milkyway_nbody_graphics
                            PROPERTIES
                              OUTPUT_NAME "${NBODY_GL_BIN_NAME}")
  endif()
endif()

if(INSTALL_BOINC)
  install_boinc(milkyway_nbody)
else()
    install(TARGETS milkyway_nbody
            BUNDLE DESTINATION bin
            RUNTIME DESTINATION bin)
endif()

set(NBODY_BIN_NAME "$<TARGET_FILE_NAME:milkyway_nbody>")

if(SYSTEM_IS_X86)
  enable_sse2(milkyway_nbody)
endif()


if(NBODY_GL)
  if(INSTALL_BOINC)
    install_boinc(milkyway_nbody_graphics)
  else()
#    install(TARGETS milkyway_nbody_graphics
#              RUNTIME DESTINATION bin)
  endif()

  set(NBODY_GRAPHICS_BIN_NAME "$<TARGET_FILE_NAME:milkyway_nbody_graphics>")

  set_target_properties(milkyway_nbody_graphics PROPERTIES
                          COMPILE_FLAGS "${FAST_MATH_FLAGS}")
  if(MINGW)
    set_target_properties(milkyway_nbody_graphics
                          PROPERTIES
                            LINK_FLAGS "-mwindows")
  endif()
endif()

configure_file (
  "${NBODY_INCLUDE_DIR}/nbody_config.h.in"
  "${PROJECT_INCLUDE_DIR}/nbody_config.h")

if(NOT OPENSSL_FOUND)
  message(WARNING "OpenSSL not found, not using N-body tests")
else()
  add_subdirectory(tests EXCLUDE_FROM_ALL)
endif()


add_subdirectory(doc EXCLUDE_FROM_ALL)
