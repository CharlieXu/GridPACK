#
#     Copyright (c) 2013 Battelle Memorial Institute
#     Licensed under modified BSD License. A copy of this license can be
#     found
#     in the LICENSE file in the top level directory of this distribution.
#
# -*- mode: cmake -*-
# -------------------------------------------------------------
# file: CMakeLists.txt
# -------------------------------------------------------------
# -------------------------------------------------------------
# Created January 19, 2015 by William A. Perkins, Bruce Palmer
# -------------------------------------------------------------

set(target_libraries
    gridpack_components
    gridpack_partition
    gridpack_parallel
    gridpack_math
    gridpack_configuration
    gridpack_timer
    gridpack_sematrix_components
    gridpack_ymatrix_components
    ${PARMETIS_LIBRARY} ${METIS_LIBRARY} 
    ${Boost_LIBRARIES}
    ${GA_LIBRARIES}
    ${MPI_CXX_LIBRARIES}
    ${PETSC_LIBRARIES})

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})
if (GA_FOUND)
  include_directories(AFTER ${GA_INCLUDE_DIRS})
endif()

add_library(gridpack_state_estimation_module
  se_app_module.cpp
  se_factory_module.cpp
)

# -------------------------------------------------------------
# component serialization tests
# -------------------------------------------------------------
target_link_libraries(gridpack_state_estimation_module ${target_libraries})

# -------------------------------------------------------------
# installation
# -------------------------------------------------------------
install(FILES 
  se_app_module.hpp
  se_factory_module.hpp
  DESTINATION include/gridpack/applications/modules/state_estimation
)

install(TARGETS 
  gridpack_state_estimation_module
  DESTINATION lib
)

# -------------------------------------------------------------
# Create simple test that runs state estimation code
# -------------------------------------------------------------
add_executable(se_test test/se_test.cpp)
target_link_libraries (se_test gridpack_state_estimation_module ${target_libraries})

add_custom_target(se_test_input

  COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_CURRENT_SOURCE_DIR}/test/input.xml
  ${CMAKE_CURRENT_BINARY_DIR}

  COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_CURRENT_SOURCE_DIR}/test/IEEE14.raw
  ${CMAKE_CURRENT_BINARY_DIR}

  COMMAND ${CMAKE_COMMAND} -E copy 
  ${CMAKE_CURRENT_SOURCE_DIR}/test/IEEE14_meas.xml
  ${CMAKE_CURRENT_BINARY_DIR}

  DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/test/input.xml
  ${CMAKE_CURRENT_SOURCE_DIR}/test/IEEE14.raw
  ${CMAKE_CURRENT_SOURCE_DIR}/test/IEEE14_meas.xml
)

add_dependencies (se_test se_test_input)

add_test(se_test se_test)
