// -------------------------------------------------------------
/**
 * @file   base_component.cpp
 * @author Bruce Palmer
 * @date   2013-07-11 12:25:44 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#include "gridpack/parallel/distributed.hpp"
#include "gridpack/component/base_component.hpp"

// Base implementation of the MatVecInterface. These functions should be
// overwritten in actual components

/**
 * Constructor
 */
gridpack::component::MatVecInterface::MatVecInterface(void)
{
  p_ival = 0;
  p_idx = 0;
  p_jdx = 0;
}

/**
 * Constructor
 */
gridpack::component::MatVecInterface::~MatVecInterface(void)
{
}

/**
 * Return size of matrix block on the diagonal contributed by component
 * @param isize, jsize: number of rows and columns of matrix
 *        block
 * @return: false if network component does not contribute
 *        matrix element
 */
bool gridpack::component::MatVecInterface::matrixDiagSize( int *isize,
             int *jsize) const
{
  *isize = 0;
  *jsize = 0;
  return false;
}

/**
 * Return the values of for a diagonal matrix block. The values are
 * returned in row-major order.
 * @param values: pointer to matrix block values
 * @return: false if network component does not contribute
 *        matrix element
 */
bool gridpack::component::MatVecInterface::matrixDiagValues(void *values)
{
  return false;
}

/**
 * Return size of off-diagonal matrix block contributed by component. The
 * values are for the forward direction.
 * @param isize, jsize: number of rows and columns of matrix
 *        block
 * @return: false if network component does not contribute
 *        matrix element
 */
bool gridpack::component::MatVecInterface::matrixForwardSize(int *isize,
       int *jsize) const
{
  *isize = 0;
  *jsize = 0;
  return false;
}

/**
 * Return the values of for an off-diagonl matrix block. The values are
 * for the forward direction and are returned in row-major order.
 * @param values: pointer to matrix block values
 * @return: false if network component does not contribute
 *        matrix element
 */
bool gridpack::component::MatVecInterface::matrixForwardValues(void *values)
{
  return false;
}

/**
 * Return size of off-diagonal matrix block contributed by component. The
 * values are for the reverse direction.
 * @param isize, jsize: number of rows and columns of matrix
 *        block
 * @return: false if network component does not contribute
 *        matrix element
 */
bool gridpack::component::MatVecInterface::matrixReverseSize(int *isize,
       int *jsize) const
{
  *isize = 0;
  *jsize = 0;
  return false;
}

/**
 * Return the values of for an off-diagonl matrix block. The values are
 * for the reverse direction and are returned in row-major order.
 * @param values: pointer to matrix block values
 * @return: false if network component does not contribute
 *        matrix element
 */
bool gridpack::component::MatVecInterface::matrixReverseValues(void *values)
{
  return false;
}

/**
 * Return size of vector block contributed by component
 * @param isize: number of vector elements
 * @return: false if network component does not contribute
 *        vector element
 */
bool gridpack::component::MatVecInterface::vectorSize(int *isize) const
{
  *isize = 0;
  return false;
}

/**
 * Return the values of the vector block
 * @param values: pointer to vector values
 * @return: false if network component does not contribute
 *        vector element
 */
bool gridpack::component::MatVecInterface::vectorValues(void *values)
{
  return false;
}

/**
 * Set the matrix index for diagonal matrix components or vector component,
 * based on location of component in network
 * @param idx: value of index
 */
void gridpack::component::MatVecInterface::setMatVecIndex(int idx)
{
  p_ival = idx;
}

/**
 * Get the matrix index for diagonal matrix components or vector component,
 * based on location of component in network
 * @return: value of index
 */
void gridpack::component::MatVecInterface::getMatVecIndex(int *idx) const
{
  *idx = p_ival;
}

/**
 * Set the matrix indices for matrix components, based on location of
 * component
 * in network
 * @param idx, jdx: value of indices
 */
void gridpack::component::MatVecInterface::setMatVecIndices(int idx, int jdx)
{
  p_idx = idx;
  p_jdx = jdx;
}

/**
 * Get the matrix indices for matrix components, * based on location of
 * component
 * in network
 * @param idx, jdx: value of indices
 */
void gridpack::component::MatVecInterface::getMatVecIndices(int *idx, int *jdx) const
{
  *idx = p_idx;
  *jdx = p_jdx;
}

// The base implementation for bus and branch components.

/**
 * Simple constructor
 */
gridpack::component::BaseComponent::BaseComponent(void)
  : p_XCBuf(NULL), p_XCBufSize(0), p_mode(0)
{
}

/**
 * Destructor
 */
gridpack::component::BaseComponent::~BaseComponent(void)
{
}

/**
 * Load data from DataCollection object into corresponding
 * component. This needs to be implemented by every component
 * @param data: data collection associated with component
 */
void gridpack::component::BaseComponent::load(
  const boost::shared_ptr<gridpack::component::DataCollection> &data)
{
  // This implementation is a no-op and is included in BaseComponent so that
  // a generic load method can be defined in the base factory class.
}

/**
 * Return the size of the buffer needed for data exchanges. Note that this
 * must be the same size for all bus and all branch objects (branch buffers
 * do not need to be the same size as bus buffers), even if all objects
 * do not require the same parameters. Thus, the buffer must be big enough
 * to exchange all variables that an object might need, even if individual
 * objects don't need all the variables
 */
int gridpack::component::BaseComponent::getXCBufSize(void)
{
  return p_XCBufSize;
}
/**
 * Assign the location of the data exchange buffer. These buffers are
 * allocated and deallocated by the network
 * @param buf: void pointer to exchange buffer
 */
void gridpack::component::BaseComponent::setXCBuf(void *buf)
{
  // FIXME: ?
  if (buf == NULL) {
    gridpack::component::BaseComponent::p_XCBuf = NULL;
  } else {
    gridpack::component::BaseComponent::p_XCBuf = buf;
  }
}

/**
 * Set an internal variable that can be used to control the behavior of the
 * component. This function doesn't need to be implemented, but if needed,
 * it can be used to change the behavior of the network in different phases
 * of the calculation. For example, if a different matrix needs to be
 * generated at different times, the mode of the calculation can changed to
 * get different values from the MatVecInterface functions
 * @param mode: integer indicating which mode should be used
 */
void gridpack::component::BaseComponent::setMode(int mode)
{
  p_mode = mode;
}

/**
 * Copy a string for output into buffer. The behavior of this method can be
 * altered by inputting different values for the signal string
 * @param string: buffer containing string to be written to output
 * @param signal: string to control behavior of routine (e.g. what
 * properties to write
 * @return: true if component is writing a contribution, false otherwise
 */
bool gridpack::component::BaseComponent::serialWrite(char *string, char *signal)
{
  // This is defined so that generic operations for writing strings from buses
  // and branches can be built
}

// Base implementation for a bus object. Provides a mechanism for the bus to
// provide a list of the branches that are directly connected to it as well as a
// mechanism for returning a list of the buses that are connected to it via a
// single branch

/**
 * Simple constructor
 */
gridpack::component::BaseBusComponent::BaseBusComponent(void)
  : p_refBus(false)
{
  
}

/**
 * Simple destructor
 */
gridpack::component::BaseBusComponent::~BaseBusComponent(void)
{
}

/**
 * Add a pointer to the list of branches that a bus is connected to
 * @param branch: pointer to a branch that is connected to bus
 */
void
gridpack::component::BaseBusComponent::addBranch(const
  boost::shared_ptr<gridpack::component::BaseComponent> & branch)
{
  boost::weak_ptr<BaseComponent> tbranch(branch);
  p_branches.push_back(tbranch);
}

/**
 * Add a pointer to the list of buses that a bus is connected to via
 * a branch
 * @param bus: pointer to a branch that is connected to bus
 */
void
gridpack::component::BaseBusComponent::addBus(const
  boost::shared_ptr<gridpack::component::BaseComponent> & bus)
{
  boost::weak_ptr<BaseComponent> tbus(bus);
  p_buses.push_back(tbus);
}

/**
 * Get pointers to branches that are connected to bus
 * @param nghbrs: list of pointers to neighboring branches
 */
void gridpack::component::BaseBusComponent::getNeighborBranches(
  std::vector<boost::shared_ptr<gridpack::component::BaseComponent> > &nghbrs) const
{
  nghbrs.clear();
  int i;
  int size = p_branches.size();
  for (i=0; i<size; i++) {
    boost::shared_ptr<gridpack::component::BaseComponent> branch = p_branches[i].lock();
    nghbrs.push_back(branch);
  }
}

/**
 * Get pointers to buses that are connected to calling bus via a branch
 * @param nghbrs: list of pointers to neighboring buses
 */
void gridpack::component::BaseBusComponent::getNeighborBuses(
  std::vector<boost::shared_ptr<gridpack::component::BaseComponent> > &nghbrs) const
{
  nghbrs.clear();
  int i;
  int size = p_buses.size();
  for (i=0; i<size; i++) {
    boost::shared_ptr<gridpack::component::BaseComponent> bus = p_buses[i].lock();
    nghbrs.push_back(bus);
  }
}

/**
 * Clear all pointers to neighboring branches
 */
void gridpack::component::BaseBusComponent::clearBranches(void)
{
  p_branches.clear();
}

/**
 * Clear all pointers to neighboring buses
 */
void gridpack::component::BaseBusComponent::clearBuses(void)
{
  p_buses.clear();
}

/**
 * Set reference bus status
 * @param status: reference bus status
 */
void gridpack::component::BaseBusComponent::setReferenceBus(bool status)
{
  p_refBus = status;
}

/**
 * Get reference bus status
 * @return: reference bus status
 */
bool gridpack::component::BaseBusComponent::getReferenceBus(void) const
{
  return p_refBus;
}

// Base implementation for a branch object. Provides a mechanism for the branch to
// provide the buses at either end of the branch

/**
 * Simple constructor
 */
gridpack::component::BaseBranchComponent::BaseBranchComponent(void)
{
}

/**
 * Simple destructor
 */
gridpack::component::BaseBranchComponent::~BaseBranchComponent(void)
{
}

/**
 * Set pointer to bus at one end of branch
 * @param: pointer to bus
 */
void gridpack::component::BaseBranchComponent::setBus1(const
  boost::shared_ptr<gridpack::component::BaseComponent> & bus)
{
  p_bus1 = boost::weak_ptr<BaseComponent>(bus);
}

/**
 * Set pointer to bus at other end of branch
 * @param: pointer to bus
 */
void gridpack::component::BaseBranchComponent::setBus2(const
  boost::shared_ptr<gridpack::component::BaseComponent> & bus)
{
  p_bus2 = boost::weak_ptr<BaseComponent>(bus);
}

/**
 * Get pointer to bus at one end of branch
 * @return: pointer to bus 1
 */
boost::shared_ptr<gridpack::component::BaseComponent>
  gridpack::component::BaseBranchComponent::getBus1(void) const
{
  boost::shared_ptr<gridpack::component::BaseComponent> ret(p_bus1);
  return ret;
}

/**
 * Get pointer to bus at other end of branch
 * @return: pointer to bus 2
 */
boost::shared_ptr<gridpack::component::BaseComponent>
  gridpack::component::BaseBranchComponent::getBus2(void) const
{
  boost::shared_ptr<gridpack::component::BaseComponent> ret(p_bus2);
  return ret;
}

/**
 * Clear bus pointers
 */
void gridpack::component::BaseBranchComponent::clearBuses(void)
{
  p_bus1.reset();
  p_bus2.reset();
}
