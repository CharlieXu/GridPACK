/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   ds_factory.hpp
 * @author Shuangshuang Jin 
 * @date   September 19, 2013
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#ifndef _ds_factory_h_
#define _ds_factory_h_

#include "boost/smart_ptr/shared_ptr.hpp"
#include "gridpack/factory/base_factory.hpp"
#include "gridpack/applications/dynamic_simulation/ds_components.hpp"
#include "gridpack/math/matrix.hpp"

namespace gridpack {
namespace dynamic_simulation {

class DSFactory
  : public gridpack::factory::BaseFactory<DSNetwork> {
  public:
    /**
     * Basic constructor
     * @param network: network associated with factory
     */
    DSFactory(NetworkPtr network);

    /**
     * Basic destructor
     */
    ~DSFactory();

    /**
     * Create the admittance (Y-Bus) matrix
     */
    void setYBus(void);

    /**
     * Get the updating factor for posfy11 stage ybus
     */
    gridpack::ComplexType setFactor(int sw2_2, int sw3_2);

  private:

    NetworkPtr p_network;
};

} // dynamic_simulation
} // gridpack
#endif
