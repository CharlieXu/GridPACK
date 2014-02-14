/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   ca_factory.hpp
 * @author Yousu Chen 
 * @date   Feb 11, 2014 
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#ifndef _ca_factory_h_
#define _ca_factory_h_

#include "boost/smart_ptr/shared_ptr.hpp"
#include "gridpack/factory/base_factory.hpp"
#include "ca_components.hpp"
#include "ca_driver.hpp"
#include "gridpack/math/matrix.hpp"

namespace gridpack {
namespace contingency_analysis {

//enum CAMode{YBus, Jacobian};

class CAFactory
  : public gridpack::factory::BaseFactory<CANetwork> {
  public:
    /**
     * Basic constructor
     * @param network: network associated with factory
     */
    CAFactory(NetworkPtr network);

    /**
     * Basic destructor
     */
    ~CAFactory();

    /**
     * Create the admittance (Y-Bus) matrix
     */
    void setYBus(void);

    /**
     * Find GBus vector 
     */
    void setGBus(void);

    /**
     * Make SBus vector 
     */
    void setSBus(void);

    /**
     * Create the PQ 
     */
    void setPQ(void);

    /**
     * Create the Jacobian matrix
     */
    void setJacobian(void);
#if 1
    /**
     * Set contingency
     * @param contingency the contigency that is to be set
     */
    void setContingency(gridpack::contingency_analysis::Contingency contingency);

    /**
     * Clear contingency and set branch to its pre-contingency state
     */
    void clearContingency(gridpack::contingency_analysis::Contingency
        contingency);
#endif
  private:

    NetworkPtr p_network;
    std::vector<bool> p_saveStatus;
};

} // contingency_analysis
} // gridpack
#endif
