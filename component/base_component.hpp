// -------------------------------------------------------------
/**
 * @file   base_component.hpp
 * @author Bruce Palmer
 * @date   April 8, 2013
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#ifndef _base_component_h_
#define _base_component_h_

#include <vector>
#include "boost/smart_ptr/shared_ptr.hpp"
#include "boost/smart_ptr/weak_ptr.hpp"

// TODO: Might want to put MatrixIndices and VectorIndex operations into a
//       separate class since these can probably be implemented once for all
//       network components
namespace gridpack{
namespace component{

class MatVecInterface {
  public:
    /**
     * Constructor
     */
    MatVecInterface(void);

    /**
     * Destructor
     */
    virtual ~MatVecInterface(void);

    /**
     * Return size of matrix block on the diagonal contributed by component
     * @param isize, jsize: number of rows and columns of matrix
     *        block
     * @return: false if network component does not contribute
     *        matrix element
     */
    bool matrixDiagSize(int *isize, int *jsize) const;

    /**
     * Return the values of for a diagonal matrix block. The values are
     * returned in row-major order.
     * @param values: pointer to matrix block values
     * @return: false if network component does not contribute
     *        matrix element
     */
    bool matrixDiagValues(void *values);

    /**
     * Return size of off-diagonal matrix block contributed by component. The
     * values are for the forward direction
     * @param isize, jsize: number of rows and columns of matrix
     *        block
     * @return: false if network component does not contribute
     *        matrix element
     */
    bool matrixForwardSize(int *isize, int *jsize) const;

    /**
     * Return the values of for an off-diagonl matrix block. The values are
     * for the forward direction and are returned in row-major order.
     * @param values: pointer to matrix block values
     * @return: false if network component does not contribute
     *        matrix element
     */
    bool matrixForwardValues(void *values);

    /**
     * Return size of off-diagonal matrix block contributed by component. The
     * values are for the reverse direction
     * @param isize, jsize: number of rows and columns of matrix
     *        block
     * @return: false if network component does not contribute
     *        matrix element
     */
    bool matrixReverseSize(int *isize, int *jsize) const;

    /**
     * Return the values of for an off-diagonl matrix block. The values are
     * for the reverse direction and are returned in row-major order.
     * @param values: pointer to matrix block values
     * @return: false if network component does not contribute
     *        matrix element
     */
    bool matrixReverseValues(void *values);

    /**
     * Return size of vector block contributed by component
     * @param isize: number of vector elements
     * @return: false if network component does not contribute
     *        vector element
     */
    virtual bool vectorSize(int *isize) const;

    /**
     * Return the values of the vector block.
     * @param values: pointer to vector values
     * @return: false if network component does not contribute
     *        vector element
     */
    virtual bool vectorValues(void *values);

    //TODO: May need to include routines that support moving values from vectors
    //      back into network components.

  private:

};

// -------------------------------------------------------------
//  class BaseField:
//  This class implements some basic functions that can be
//  expected from any field on the network.
// -------------------------------------------------------------
class BaseComponent
  : public MatVecInterface {
  public:
    /**
     * Simple constructor
     */
    BaseComponent(void);

    /**
     * Destructor
     */
    virtual ~BaseComponent(void);

    /**
     * Return the size of the component for use in packing and
     * unpacking routines. This might not be needed, but throw
     * it in for now.
     * @return: size of network component
     */
    virtual int size(void) const;

  private:

};

class BaseBusComponent
  : public BaseComponent {
  public:
    /**
     * Simple constructor
     */
    BaseBusComponent(void);

    /**
     * Simple destructor
     */
    virtual ~BaseBusComponent(void);

    /**
     * Add a pointer to the list of branches that a bus is connected to
     * @param branch: pointer to a branch that is connected to bus
     */
    void addBranch(const boost::shared_ptr<BaseComponent> & branch);

    /**
     * Add a pointer to the list of buses that a bus is connected to via
     * a branch
     * @param bus: pointer to a branch that is connected to bus
     */
    void addBus(const boost::shared_ptr<BaseComponent> & bus);

    /**
     * Get pointers to branches that are connected to bus
     * @param nghbrs: list of pointers to neighboring branches
     */
    void getNeighborBranches(std::vector<boost::shared_ptr<BaseComponent> > &nghbrs) const;

    /**
     * Get pointers to buses that are connected to calling bus via a branch
     * @param nghbrs: list of pointers to neighboring buses
     */
    void getNeighborBuses(std::vector<boost::shared_ptr<BaseComponent> > &nghbrs) const;

    /**
     * Clear all pointers to neighboring branches
     */
    void clearBranches();

    /**
     * Clear all pointers to neighboring buses
     */
    void clearBuses();

  private:
    /**
     * Branches that are connect to bus
     */
    std::vector<boost::weak_ptr<BaseComponent> > p_branches;
    /**
     * Buses that are connect to bus via a branch
     */
    std::vector<boost::weak_ptr<BaseComponent> > p_buses;
};

class BaseBranchComponent
  : public BaseComponent {
  public:
    /**
     * Simple constructor
     */
    BaseBranchComponent(void);

    /**
     * Simple destructor
     */
    virtual ~BaseBranchComponent(void);

    /**
     * Set pointer to bus at one end of branch
     * @param bus: pointer to bus
     */
    void setBus1(const boost::shared_ptr<BaseComponent> & bus);

    /**
     * Set pointer to bus at other end of branch
     * @param bus: pointer to bus
     */
    void setBus2(const boost::shared_ptr<BaseComponent> & bus);

    /**
     * Get pointer to bus at one end of branch
     * @return: pointer to bus 1
     */
    boost::shared_ptr<BaseComponent> getBus1(void) const;

    /**
     * Get pointer to bus at other end of branch
     * @return: pointer to bus 2
     */
    boost::shared_ptr<BaseComponent> getBus2(void) const;

    /**
     * Clear bus pointers
     */
    void clearBuses(void);

  private:
    /**
     *  Pointers to buses at either end of branch
     */
    boost::weak_ptr<BaseComponent> p_bus1;
    boost::weak_ptr<BaseComponent> p_bus2;

};

}    // component
}    // gridpack
#endif
