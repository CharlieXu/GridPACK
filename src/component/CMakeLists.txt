# -*- mode: cmake -*-
# -------------------------------------------------------------
# file: CMakeLists.txt
# -------------------------------------------------------------
# -------------------------------------------------------------
# Created May  6, 2013 by William A. Perkins
# Last Change: 2013-07-04 08:31:28 d3g096
# -------------------------------------------------------------

set(target_libraries gridpack_parallel ${Boost_LIBRARIES} ${MPI_CXX_LIBRARIES})

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(gridpack_components
  base_component.cpp
  data_collection.cpp
)

# -------------------------------------------------------------
# component serialization tests
# -------------------------------------------------------------
add_executable(component_serialization_test test/component_serialization.cpp)
target_link_libraries(component_serialization_test gridpack_components ${target_libraries})
gridpack_add_unit_test(component_serialization component_serialization_test)

