/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   base_network.hpp
 * @author Bruce Palmer, William Perkins
 * @date   2014-03-06 09:40:19 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#ifndef _base_network_h_
#define _base_network_h_

#include <fstream>
#include <iomanip>
#include <vector>
#include <map>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/type_traits.hpp>
#include <ga.h>
#include "gridpack/parallel/distributed.hpp"
#include "gridpack/component/base_component.hpp"
#include "gridpack/component/data_collection.hpp"
#include "gridpack/partition/graph_partitioner.hpp"
#include "gridpack/parallel/shuffler.hpp"
#include "gridpack/parallel/ga_shuffler.hpp"
#include "gridpack/timer/coarse_timer.hpp"

namespace gridpack {
namespace network {
/** @cond */
// -------------------------------------------------------------
// A simple data class to assemble all bus related elements in
// the network into a single struct.
// -------------------------------------------------------------
template <class _bus>
class BusData {
public:

/**
 *  Default constructor
 */
BusData(void)
  : p_activeBus(true),
    p_originalBusIndex(-1),
    p_globalBusIndex(-1),
    p_branchNeighbors(),
    p_bus(new _bus),
    p_data(new gridpack::component::DataCollection),
    p_refFlag(false)
{
}

/// Copy constructor
BusData(const BusData& old)
  : p_activeBus(old.p_activeBus),
    p_originalBusIndex(old.p_originalBusIndex),
    p_globalBusIndex(old.p_globalBusIndex),
    p_branchNeighbors(old.p_branchNeighbors),
    p_bus(old.p_bus),
    p_data(old.p_data),
    p_refFlag(old.p_refFlag)
{}

/**
 *  Default destructor
 */
~BusData(void)
{
}

/**
 *  Assigment operator
 */
BusData<_bus> & operator=(const BusData<_bus> & rhs)
{
  if (this == &rhs) return *this;
  p_activeBus = rhs.p_activeBus;
  p_originalBusIndex = rhs.p_originalBusIndex;
  p_globalBusIndex = rhs.p_globalBusIndex;
  p_branchNeighbors = rhs.p_branchNeighbors;
  p_bus = rhs.p_bus;
  p_data = rhs.p_data;
  p_refFlag = rhs.p_refFlag;
  return *this;
}

/**
 * Data elements on bus
 * p_activeBus: flag to identify buses that are "owned" by processor
 * p_originalBusIndex: original index of a bus (from the network topology file)
 * p_globalBusIndex: unique global index of a bus assigned by the partitioner
 * p_branchNeighbors: local indices of branches that are connected to a local bus
 * p_bus: pointer to bus object
 * p_data: pointer to data collection object
 * p_refFlag: true if this bus is the reference bus
 */
  bool                                                   p_activeBus;
  int                                                    p_originalBusIndex;
  int                                                    p_globalBusIndex;
  std::vector<int>                                       p_branchNeighbors;
  boost::shared_ptr<_bus>                                p_bus;
  boost::shared_ptr<component::DataCollection>           p_data;
  bool                                                   p_refFlag;

private: 

  friend class boost::serialization::access;

  /// Serialization method
  template<class Archive> void serialize(Archive &ar, const unsigned int)
  {
    ar & p_activeBus
      & p_originalBusIndex
      & p_globalBusIndex
      & p_branchNeighbors
      & p_bus
      & p_data
      & p_refFlag;
  }

};

// -------------------------------------------------------------
// A simple data class to assemble all branch related elements
// in the network into a single struct.
// -------------------------------------------------------------
template <class _branch>
class BranchData {
public:

/**
 *  Default constructor
 */
BranchData(void)
  : p_activeBranch(true),
    p_globalBranchIndex(-1),
    p_originalBusIndex1(-1),
    p_originalBusIndex2(-1),
    p_globalBusIndex1(-1),
    p_globalBusIndex2(-1),
    p_localBusIndex1(-1),
    p_localBusIndex2(-1),
    p_branch(new _branch),
    p_data(new gridpack::component::DataCollection)
{
}

/// Copy constructor
BranchData(const BranchData& old)
  : p_activeBranch(old.p_activeBranch),
    p_globalBranchIndex(old.p_globalBranchIndex),
    p_originalBusIndex1(old.p_originalBusIndex1),
    p_originalBusIndex2(old.p_originalBusIndex2),
    p_globalBusIndex1(old.p_globalBusIndex1),
    p_globalBusIndex2(old.p_globalBusIndex2),
    p_localBusIndex1(old.p_localBusIndex1),
    p_localBusIndex2(old.p_localBusIndex2),
    p_branch(old.p_branch),
    p_data(old.p_data)
{}

/**
 *  Default destructor
 */
~BranchData(void)
{
}

/**
 *  Assigment operator
 */
BranchData<_branch> & operator=(const BranchData<_branch> & rhs)
{
  if (this == &rhs) return *this;
  p_activeBranch = rhs.p_activeBranch;
  p_globalBranchIndex = rhs.p_globalBranchIndex;
  p_originalBusIndex1 = rhs.p_originalBusIndex1;
  p_originalBusIndex2 = rhs.p_originalBusIndex2;
  p_globalBusIndex1 = rhs.p_globalBusIndex1;
  p_globalBusIndex2 = rhs.p_globalBusIndex2;
  p_localBusIndex1 = rhs.p_localBusIndex1;
  p_localBusIndex2 = rhs.p_localBusIndex2;
  p_branch = rhs.p_branch;
  p_data = rhs.p_data;
  return *this;
}

/**
 * Data elements on branch
 * p_activeBranch: flag to identify buses that are "owned" by processor
 * p_globalBranchIndex: unique global identifier for this branch
 * p_originalBusIndex1: original index of bus at "from" end of branch
 *      (from the network topology file)
 * p_originalBusIndex2: original index of bus at "to" end of branch
 *      (from the network topology file)
 * p_globalBusIndex1: global index of bus at "from" end of branch
 * p_globalBusIndex2: global index of bus at "to" end of branch
 * p_localBusIndex1: local index of bus at "from" end of branch
 * p_localBusIndex2: local index of bus at "to" end of branch
 * p_branch: pointer to branch object
 * p_data: pointer to data collection object
 */
  bool                                                   p_activeBranch;
  int                                                    p_globalBranchIndex;
  int                                                    p_originalBusIndex1;
  int                                                    p_originalBusIndex2;
  int                                                    p_globalBusIndex1;
  int                                                    p_globalBusIndex2;
  int                                                    p_localBusIndex1;
  int                                                    p_localBusIndex2;
  boost::shared_ptr<_branch>                             p_branch;
  boost::shared_ptr<component::DataCollection>           p_data;

private: 

  friend class boost::serialization::access;

  /// Serialization method
  template<class Archive> void serialize(Archive &ar, const unsigned int)
  {
    ar & p_activeBranch
      & p_globalBranchIndex
      & p_originalBusIndex1
      & p_originalBusIndex2
      & p_globalBusIndex1
      & p_globalBusIndex2
      & p_localBusIndex1
      & p_localBusIndex2
      & p_branch
      & p_data;
  }

};
/** @endcond */

/**
 *  class BaseNetwork:
 *  This is the base class for creating distributed networks. It
 *  is basically a container that supports the network topology,
 *  allows user-defined buses and branches to be added to the
 *  network and has containers that can be used to store parameters
 *  for input or output. BaseNetwork also implements ghost bus and
 *  ghost branch update for the network. The BaseNetwork class also
 *  contains the partitioner, although this is implemented using
 *  several additional classes to encapsulate the partitioner from the
 *  remainder of the network class.
 */

template <class _bus, class _branch>
class BaseNetwork 
  : public parallel::Distributed
{

  // Check to make sure that "_bus" is a descendant of BaseBusComponent
  BOOST_STATIC_ASSERT((boost::is_base_of< component::BaseBusComponent, _bus >::value ));

  // Check to make sure that "_branch" is a descendant of BaseBranchComponent
  BOOST_STATIC_ASSERT((boost::is_base_of< component::BaseBranchComponent, _branch >::value ));

public:

/**
 * Type declarations that can be used external to the BaseNetwork class
 */
typedef _bus BusType;
typedef _branch BranchType;
typedef boost::shared_ptr<_bus> BusPtr;
typedef boost::shared_ptr<_branch> BranchPtr;

/**
 * Default constructor.
 */
explicit BaseNetwork(const parallel::Communicator& comm)
  : parallel::Distributed(comm)
{
  //TODO: Get default parallel configuration for world group
  p_refBus = -1;
  p_busXCBufSize = 0;
  p_branchXCBufSize = 0;
  p_busXCBufType = 0;
  p_branchXCBufType = 0;
  p_busGASet = false;
  p_branchGASet = false;
  p_activeBusIndices = NULL;
  p_busSndBuf = NULL;
  p_inactiveBusIndices = NULL;
  p_busRcvBuf = NULL;
  p_activeBranchIndices = NULL;
  p_branchSndBuf = NULL;
  p_inactiveBranchIndices = NULL;
  p_branchRcvBuf = NULL;
}

/**
 * Default destructor.
 */
virtual ~BaseNetwork(void)
{
  int i, size;
  // Clean up exchange buffers if they have been allocated
  if (p_busXCBufSize != 0) {
    if (p_busXCBuffers) {
      delete [] ((char*)p_busXCBuffers);
    } else {
      // TODO: some kind of error
    }
  }
  if (p_branchXCBufSize != 0) {
    if (p_branchXCBuffers) {
      delete [] ((char*)p_branchXCBuffers);
    } else {
      // TODO: some kind of error
    }
  }
  if (p_activeBusIndices) {
    for (i=0; i<0; i<p_numActiveBuses) {
      delete p_activeBusIndices[i];
    }
    delete [] p_activeBusIndices;
  }
  if (p_busSndBuf) {
    delete [] ((char*)p_busSndBuf);
  }
  if (p_inactiveBusIndices) {
    for (i=0; i<0; i<p_numInactiveBuses) {
      delete p_inactiveBusIndices[i];
    }
    delete [] p_inactiveBusIndices;
  }
  if (p_busRcvBuf) {
    delete [] ((char*)p_busRcvBuf);
  }
  if (p_activeBranchIndices) {
    for (i=0; i<0; i<p_numActiveBranches) {
      delete p_activeBranchIndices[i];
    }
    delete [] p_activeBranchIndices;
  }
  if (p_branchSndBuf) {
    delete [] ((char*)p_branchSndBuf);
  }
  if (p_inactiveBranchIndices) {
    for (i=0; i<0; i<p_numInactiveBranches) {
      delete p_inactiveBranchIndices[i];
    }
    delete [] p_inactiveBranchIndices;
  }
  if (p_branchSndBuf) {
    delete [] ((char*)p_branchRcvBuf);
  }
  if (p_branchGASet) {
    GA_Destroy(p_branchGA);
    NGA_Deregister_type(p_branchXCBufType);
  }
  if (p_busGASet) {
    GA_Destroy(p_busGA);
    NGA_Deregister_type(p_busXCBufType);
  }
}

/**
 * Add a bus locally to the network
 * @param idx original index of  bus
 */
void addBus(int idx)
{
  BusDataPtr bus(new BusData<BusType>());
  bus->p_originalBusIndex = idx;
  bus->p_globalBusIndex = -1;
  p_buses.push_back(*bus);
}

/**
 * Add a branch locally to the network. A branch is defined by
 * buses at either end
 * @param idx1 original bus index of bus 1
 * @param idx2 original bus index of bus 2
 */
void addBranch(int idx1, int idx2)
{
  BranchDataPtr branch(new BranchData<_branch>());
  branch->p_originalBusIndex1 = idx1;
  branch->p_originalBusIndex2 = idx2;
  branch->p_globalBusIndex1 = -1;
  branch->p_globalBusIndex2 = -1;
  p_branches.push_back(*branch);
}

/**
 * Number of local buses (both active and inactive) on processor
 * @return number of buses
 */
int numBuses(void)
{
  return p_buses.size();
}

/**
 * Return the total number of buses in the entire network
 * @return total number of buses
 */
int totalBuses(void)
{
  int nBus = p_buses.size();
  int i, total;
  total = 0;
  for (i=0; i<nBus; i++) {
    if (p_buses[i].p_activeBus) total++;
  }
  int grp = this->communicator().getGroup();
  GA_Pgroup_igop(grp,&total,1,"+");
  return total;
}

/**
 * Number of local branches (both active and inactive) on processor
 * @return number of branches
 */
int numBranches(void)
{
  return p_branches.size();
}

/**
 * Return the total number of branches in the entire network
 * @return total number of branches
 */
int totalBranches(void)
{
  int nBranch = p_branches.size();
  int i, total;
  total = 0;
  for (i=0; i<nBranch; i++) {
    if (p_branches[i].p_activeBranch) total++;
  }
  int grp = this->communicator().getGroup();
  GA_Pgroup_igop(grp,&total,1,"+");
  return total;
}

/**
 * Designate a bus as a reference bus.
 * @param idx local index of bus
 */
void setReferenceBus(int idx)
{
  p_refBus = idx;
  if (idx > 0 && idx <= p_buses.size()) {
    p_buses[idx].p_refFlag = true;
  } else {
    p_buses[idx].p_refFlag = false;
  }
}

/**
 * Return index of reference bus.
 * @return local index of reference bus. If reference bus is not on this
 * processor then return -1.
 */
int getReferenceBus(void) const
{
  return p_refBus;
}

// Bus and Branch modifiers

/**
 * Set the original index of the bus (from configuration file)
 * @param idx local index of bus
 * @param o_idx original index assigned to bus
 * @return false if no bus exists for idx
 */
bool setOriginalBusIndex(int idx, int o_idx)
{
  if (idx < 0 || idx >= p_buses.size()) {
    return false;
  } else {
    p_buses[idx].p_originalBusIndex = o_idx;
    return true;
  }
}

/**
 * Set the global index of the bus
 * @param idx local index of bus
 * @param g_idx global index to be assigned to bus
 * @return false if no bus exists for idx
 */
bool setGlobalBusIndex(int idx, int g_idx)
{
  if (idx < 0 || idx >= p_buses.size()) {
    return false;
  } else {
    p_buses[idx].p_globalBusIndex = g_idx;
    return true;
  }
}

/**
 * Set the global index of the branch
 * @param idx local index of branch
 * @param g_idx global index to be assigned to branch
 * @return false if no branch exists for idx
 */
bool setGlobalBranchIndex(int idx, int g_idx)
{
  if (idx < 0 || idx >= p_branches.size()) {
    return false;
  } else {
    p_branches[idx].p_globalBranchIndex = g_idx;
    return true;
  }
}

/**
 * Set the original index of the bus at the "from" end of branch
 * @param idx local index of branch
 * @param b_idx original index of "from" bus for this branch
 * @return false if no branch exists for idx
 */
bool setOriginalBusIndex1(int idx, int b_idx)
{
  if (idx < 0 || idx >= p_branches.size()) {
    return false;
  } else {
    p_branches[idx].p_originalBusIndex1 = b_idx;
    return true;
  }
}

/**
 * Set the original index of the bus at the "to" end of branch
 * @param idx local index of branch
 * @param b_idx original index of "to" bus for this branch
 * @return false if no branch exists for idx
 */
bool setOriginalBusIndex2(int idx, int b_idx)
{
  if (idx < 0 || idx >= p_branches.size()) {
    return false;
  } else {
    p_branches[idx].p_originalBusIndex2 = b_idx;
    return true;
  }
}

/**
 * Set the global index of the bus at the "from" end of branch
 * @param idx local index of branch
 * @param b_idx global index of "from" bus for this branch
 * @return false if no branch exists for idx
 */
bool setGlobalBusIndex1(int idx, int b_idx)
{
  if (idx < 0 || idx >= p_branches.size()) {
    return false;
  } else {
    p_branches[idx].p_globalBusIndex1 = b_idx;
    return true;
  }
}

/**
 * Set the global index of the bus at the "to" end of branch
 * @param idx local index of branch
 * @param b_idx global index of "to" bus for this branch
 * @return false if no branch exists for idx
 */
bool setGlobalBusIndex2(int idx, int b_idx)
{
  if (idx < 0 || idx >= p_branches.size()) {
    return false;
  } else {
    p_branches[idx].p_globalBusIndex2 = b_idx;
    return true;
  }
}

/**
 * Set the local index of the bus at the "from" end of branch
 * @param idx local index of branch
 * @param b_idx local index of "from" bus for this branch
 * @return false if no branch exists for idx
 */
bool setLocalBusIndex1(int idx, int b_idx)
{
  if (idx < 0 || idx >= p_branches.size()) {
    return false;
  } else {
    p_branches[idx].p_localBusIndex1 = b_idx;
    return true;
  }
}

/**
 * Set the local index of the bus at the "to" end of branch
 * @param idx local index of branch
 * @param b_idx local index of "to" bus for this branch
 * @return false if no branch exists for idx
 */
bool setLocalBusIndex2(int idx, int b_idx)
{
  if (idx < 0 || idx >= p_branches.size()) {
    return false;
  } else {
    p_branches[idx].p_localBusIndex2 = b_idx;
    return true;
  }
}

/**
 * Set the active flag of the bus
 * @param idx local index of bus
 * @param flag flag for setting bus as active or inactive
 * @return false if no bus exists for idx
 */
bool setActiveBus(int idx, bool flag)
{
  if (idx < 0 || idx >= p_buses.size()) {
    return false;
  } else {
    p_buses[idx].p_activeBus = flag;
    return true;
  }
}

/**
 * Set the active flag of the branch
 * @param idx local index of branch
 * @param flag flag for setting bus as active or inactive
 * @return false if no branch exists for idx
 */
bool setActiveBranch(int idx, bool flag)
{
  if (idx < 0 || idx >= p_branches.size()) {
    return false;
  } else {
    p_branches[idx].p_activeBranch = flag;
    return true;
  }
}

/**
 * Clear the list of neighbors for the bus at idx
 * @param idx local index of bus
 * @return false if no bus exists for idx
 */
bool clearBranchNeighbors(int idx)
{
  if (idx < 0 || idx >= p_buses.size()) {
    return false;
  } else {
    p_buses[idx].p_branchNeighbors.clear();
    return true;
  }
}

/**
 * Add local index for a branch attached to bus at idx
 * @param idx local index of bus
 * @param br_idx local index of branch attached to bus
 * @return false if no bus exists for idx
 */
bool addBranchNeighbor(int idx, int br_idx)
{
  if (idx < 0 || idx >= p_buses.size()) {
    return false;
  } else {
    p_buses[idx].p_branchNeighbors.push_back(br_idx);
    return true;
  }
}

// Bus and Branch accessors

/**
 * Get status of the bus (local or ghosted)
 * @param idx local index of bus
 * @return true if bus is locally held, false if it is ghosted 
 */
bool getActiveBus(int idx)
{
  if (idx >= 0 && idx < p_buses.size()) {
    return p_buses[idx].p_activeBus;
  } else {
    printf("gridpack::network::getActiveBus: illegal index: %d size: %d\n",
           idx, static_cast<int>(p_buses.size()));
    // TODO: some kind of error
  }
}

/**
 * Get original index of the bus
 * @param idx local index of bus
 * @return original index of bus 
 */
int getOriginalBusIndex(int idx)
{
  if (idx >= 0 && idx < p_buses.size()) {
    return p_buses[idx].p_originalBusIndex;
  } else {
    // TODO: some kind of error
  }
}

/**
 * Get global index of the bus
 * @param idx local index of bus
 * @return global index of bus 
 */
int getGlobalBusIndex(int idx)
{
  if (idx >= 0 && idx < p_buses.size()) {
    return p_buses[idx].p_globalBusIndex;
  } else {
    // TODO: some kind of error
  }
}

/**
 * Retrieve a pointer to an existing bus
 * @param idx local index of requested bus
 * @return a pointer to the requested bus.
 */
BusPtr getBus(int idx)
{
  if (idx<0 || idx >= p_buses.size()) {
    // TODO: Some kind of error
  } else {
    return p_buses[idx].p_bus;
  }
}

/**
 * Get status of the branch (local or ghosted)
 * @param idx local index of branch
 * @return true if branch is locally held, false if it is ghosted 
 */
bool getActiveBranch(int idx)
{
  if (idx >= 0 && idx < p_branches.size()) {
    return p_branches[idx].p_activeBranch;
  } else {
    // TODO: some kind of error
  }
}

/**
 * Get global index of the branch
 * @param idx local index of branch
 * @return global index of branch 
 */
int getGlobalBranchIndex(int idx)
{
  if (idx >= 0 && idx < p_branches.size()) {
    return p_branches[idx].p_globalBranchIndex;
  } else {
    // TODO: some kind of error
  }
}

/**
 * Retrieve a pointer to an existing branch
 * @param idx local index of requested branch
 * @return a pointer to the requested branch
 */
BranchPtr getBranch(int idx)
{
  if (idx<0 || idx >= p_branches.size()) {
    // TODO: Some kind of error
  } else {
    return p_branches[idx].p_branch;
  }
}

/**
 * Get original indices of the two buses at each end of a branch
 * @param idx local index of branch
 * @param idx1 original index of "from" bus 
 * @param idx1 original index of "to" bus 
 */
void getOriginalBranchEndpoints(int idx, int *idx1, int *idx2)
{
  if (idx >= 0 && idx < p_branches.size()) {
    *idx1 = p_branches[idx].p_originalBusIndex1;
    *idx2 = p_branches[idx].p_originalBusIndex2;
  } else {
    // TODO: some kind of error
  }
}

/**
 * Retrieve a pointer to the DataCollection object associated with bus indexed
 * by idx
 * @param idx local index of requested bus
 * @return a pointer to the requested bus data
 */
boost::shared_ptr<component::DataCollection> getBusData(int idx)
{
  if (idx<0 || idx >= p_buses.size()) {
    // TODO: Some kind of error
  } else {
    return p_buses[idx].p_data;
  }
}

/**
 * Retrieve a pointer to the DataCollection object associated with branch
 * indexed by idx
 * @param idx local index of requested branch
 * @return a pointer to the requested branch data
 */
boost::shared_ptr<component::DataCollection> getBranchData(int idx)
{
  if (idx<0 || idx >= p_branches.size()) {
    // TODO: Some kind of error
  } else {
    return p_branches[idx].p_data;
  }
}

#if 0
/**
 * Delete an existing bus field
 * @param name: a string representing the name of the field
 *       to be deleted
 */
void deleteBusField(std::string name)
{
  std::map<std::string,
boost::shared_ptr<gridpack::network::BusField<gridpack::component::BaseNetworkComponent> > >::iterator bus;
  bus = p_busFields.find(name);
  if (bus != p_busFields.end()) {
    p_busFields.erase(bus);
  }
}

/**
 * Delete an existing branch field
 * @param name: a string representing the name of the field
 *       to be deleted
 */
void deleteBranchField(std::string name)
{
  std::map<std::string,
boost::shared_ptr<gridpack::network::BranchField<gridpack::component::BaseNetworkComponent> > >::iterator branch;
  branch = p_branchFields.find(name);
  if (branch != p_branchFields.end()) {
    p_branchFields.erase(branch);
  }
}
#endif

/**
 * Return list of branches connected to bus
 * @param bus local bus index
 * @return vector of local branch indices
 */
std::vector<int> getConnectedBranches(int idx) const
{
  return p_buses[idx].p_branchNeighbors;
}

/**
 * Return list of buses connected to central bus via one branch
 * @param bus local bus index
 * @return vector of local bus indices
 */
std::vector<int> getConnectedBuses(int idx) const
{
  std::vector<int> branches = p_buses[idx].p_branchNeighbors;
  int size = branches.size();
  std::vector<int> ret;
  int i, j;
  for (i=0; i<size; i++) {
    j = branches[i];
    if (p_branches[j].p_localBusIndex1 != idx) {
      ret.push_back(p_branches[j].p_localBusIndex1);
    } else {
      ret.push_back(p_branches[j].p_localBusIndex2);
    }
  }
  return ret;
}



/**
 * Return indices of buses at either end of branch
 * @param idx local branch index
 * @param bus1 local index of bus at one end of branch
 * @param bus2 local index of bus at other end of branch
 */
void getBranchEndpoints(int idx, int *bus1, int *bus2) const
{
  if (idx<0 || idx >= p_branches.size()) {
    // TODO: some kind of error
  } else {
    *bus1 = p_branches[idx].p_localBusIndex1;
    *bus2 = p_branches[idx].p_localBusIndex2;
  }
}

  /// Assemble local part of network
  void assemble(void) 
  {
  }

  /// Partition the network over the available processes
  void partition(void)
  {
    gridpack::utility::CoarseTimer *timer;
    timer = NULL;
    timer = gridpack::utility::CoarseTimer::instance();
    
    int t_total, t_part, t_bus_dist, t_gbus_dist, t_branch_dist;

    if (timer != NULL) {
      t_total = timer->createCategory("BaseNetwork<>::partition(): Total");
      t_part = timer->createCategory("BaseNetwork<>::partition(): Partitioner");
      t_bus_dist = timer->createCategory("BaseNetwork<>::partition(): Bus Distribution");
      t_branch_dist = timer->createCategory("BaseNetwork<>::partition(): Branch Distribution");
    }

    if (timer != NULL) timer->start(t_total);

    if (timer != NULL) timer->start(t_part);

    // if (this->processor_size() <= 1) return;
    GraphPartitioner partitioner(this->communicator(),
                                 p_buses.size(), p_branches.size());

    for (BusIterator bus = p_buses.begin(); 
         bus != p_buses.end(); ++bus) {
      partitioner.add_node(bus->p_globalBusIndex);
    }
    for (BranchIterator branch = p_branches.begin(); 
         branch != p_branches.end(); ++branch) {
      partitioner.add_edge(branch->p_globalBranchIndex, 
                           branch->p_globalBusIndex1,
                           branch->p_globalBusIndex2);
    }
    partitioner.partition();

    if (timer != NULL) timer->stop(t_part);

    int me(this->processor_rank());
    GraphPartitioner::IndexVector dest, gdest;

#if 1
    typedef parallel::Shuffler<BusData<BusType>, GraphPartitioner::Index> BusShufflerType;
    typedef parallel::Shuffler<BranchData<BranchType>, GraphPartitioner::Index> BranchShufflerType;
#else 
    typedef parallel::gaShuffler<BusData<BusType>, GraphPartitioner::Index> BusShufflerType;
    typedef parallel::gaShuffler<BranchData<BranchType>, GraphPartitioner::Index> BranchShufflerType;
#endif

    BusShufflerType bus_shuffler(this->communicator());
    BranchShufflerType branch_shuffler(this->communicator());

    // Need to make copies of buses and branches that will be ghosted.
    // After active bus/branch distribution, they may not be on this
    // processor.

    BusDataVector ghostbuses;
    GraphPartitioner::MultiIndexVector gnodedest;
    GraphPartitioner::IndexVector ghostbusdest;
    BusIterator bus(p_buses.begin());
    partitioner.ghost_node_destinations(gnodedest);
    
    for (size_t i = 0; i < gnodedest.size(); ++i, ++bus) {
      for (GraphPartitioner::IndexVector::iterator d = gnodedest[i].begin();
           d != gnodedest[i].end(); ++d) {
        ghostbuses.push_back(*bus);
        ghostbusdest.push_back(*d);
      }
    }

    // Branches can only be ghosted on one other process, so they're
    // easy.

    partitioner.edge_destinations(dest);
    partitioner.ghost_edge_destinations(gdest);

    BranchDataVector ghostbranches;
    BranchIterator branch(p_branches.begin());
    GraphPartitioner::IndexVector ghostbranchdest;

    for (size_t i = 0; i < dest.size(); ++i, ++branch) {
      if (dest[i] != gdest[i]) {
        ghostbranches.push_back(*branch);
        ghostbranches.back().p_activeBranch = false;
        ghostbranchdest.push_back(gdest[i]);
      }
    }


    // distribute active nodes

    // std::cout << me << ": distributing " << p_buses.size() << " active buses" << std::endl;

    if (timer != NULL) timer->start(t_bus_dist);
    partitioner.node_destinations(dest);
    bus_shuffler(p_buses, dest);
    if (timer != NULL) timer->stop(t_bus_dist);

    // distribute active edges

    if (timer != NULL) timer->start(t_branch_dist);
    partitioner.edge_destinations(dest);
    branch_shuffler(p_branches, dest);
    if (timer != NULL) timer->stop(t_branch_dist);

    // At this point, active buses and branches are on the proper
    // process.  Now, we need to distribute and nodes and edges that
    // are ghosted.  

    // std::cout << me << ": distributing " << ghostbuses.size() << " ghost buses" << std::endl;

    if (timer != NULL) timer->start(t_bus_dist);
    bus_shuffler(ghostbuses, ghostbusdest);
    for (bus = ghostbuses.begin(); bus != ghostbuses.end(); ++bus) {
      bus->p_activeBus = false;
      p_buses.push_back(*bus);
    }
    ghostbuses.clear();
    if (timer != NULL) timer->stop(t_bus_dist);

    if (timer != NULL) timer->start(t_branch_dist);
    branch_shuffler(ghostbranches, ghostbranchdest);
    std::copy(ghostbranches.begin(), ghostbranches.end(),
              std::back_inserter(p_branches));
    ghostbranches.clear();
    if (timer != NULL) timer->stop(t_branch_dist);

    // At this point, each process should have a self-contained
    // network, update local and global indexes, etc.

    // make an index of global bus index to local index and update
    // the branch local bus indexes
    int active_buses(0), active_branches(0);
    {
      std::map<int, int> busindexes;
      int lidx(0);
      for (BusIterator b = p_buses.begin(); b != p_buses.end(); ++b, ++lidx) {
        clearBranchNeighbors(lidx);
        busindexes[b->p_globalBusIndex] = lidx;
        if (b->p_activeBus) active_buses += 1;
      }
      
      // go through the branches and set the local bus indexes and pointers
      lidx = 0;
      for (BranchIterator b = p_branches.begin(); b != p_branches.end(); ++b, ++lidx) {
        int gbus, lbus1, lbus2;
        BusPtr bus1, bus2;

        // set local indexes

        gbus = b->p_globalBusIndex1;
        lbus1 = busindexes[gbus];
        bus1 = p_buses[lbus1].p_bus;

        gbus = b->p_globalBusIndex2;
        lbus2 = busindexes[gbus];
        bus2 = p_buses[lbus2].p_bus;

        b->p_localBusIndex1 = lbus1;
        addBranchNeighbor(lbus1, lidx);

        b->p_localBusIndex2 = lbus2;
        addBranchNeighbor(lbus2, lidx);

        // set component pointers

        b->p_branch->setBus1(bus1);
        b->p_branch->setBus2(bus2);

        bus1->addBranch(b->p_branch);
        bus1->addBus(bus2);
        bus2->addBranch(b->p_branch);
        bus2->addBus(bus1);

        if (b->p_activeBranch) active_branches += 1;
      }
    }

    std::cout << me << ": "
              << "I have " 
              << p_buses.size() << " buses and "
              << p_branches.size() << " branches"
              << std::endl;

    if (timer != NULL) timer->stop(t_total);
  }

  

/**
 * Clean all ghost buses and branches from the system. This can be used
 * before repartitioning the network. This operation also removes all exchange
 * buffers, so these need to be reallocated after calling this method
 */
void clean(void)
{
  std::map<int, int> buses;
  std::map<int, int> branches;
  std::map<int, int>::iterator p;
  int i, j;
  // remove all exchange buffers
  freeXCBus();
  freeXCBranch();
  if (p_activeBusIndices) {
    for (i=0; i<0; i<p_numActiveBuses) {
      delete p_activeBusIndices[i];
    }
    delete [] p_activeBusIndices;
    p_activeBusIndices = NULL;
  }
  if (p_busSndBuf) {
    delete [] ((char*)p_busSndBuf);
    p_busSndBuf = NULL;
  }
  if (p_inactiveBusIndices) {
    for (i=0; i<0; i<p_numInactiveBuses) {
      delete p_inactiveBusIndices[i];
    }
    delete [] p_inactiveBusIndices;
    p_inactiveBusIndices = NULL;
  }
  if (p_busRcvBuf) {
    delete [] ((char*)p_busRcvBuf);
    p_busRcvBuf = NULL;
  }
  if (p_activeBranchIndices) {
    for (i=0; i<0; i<p_numActiveBranches) {
      delete p_activeBranchIndices[i];
    }
    delete [] p_activeBranchIndices;
    p_activeBranchIndices = NULL;
  }
  if (p_branchSndBuf) {
    delete [] ((char*)p_branchSndBuf);
    p_branchSndBuf = NULL;
  }
  if (p_inactiveBranchIndices) {
    for (i=0; i<0; i<p_numInactiveBranches) {
      delete p_inactiveBranchIndices[i];
    }
    delete [] p_inactiveBranchIndices;
    p_inactiveBranchIndices = NULL;
  }
  if (p_branchRcvBuf) {
    delete [] ((char*)p_branchRcvBuf);
    p_branchRcvBuf = NULL;
  }

  // remove inactive branches
  int size = p_branches.size();
  int new_id = 0;
  for (i=0; i<size; i++) {
    if (p_branches[i].p_activeBranch) {
      p_branches[new_id] = p_branches[i];
      branches.insert(std::pair<int, int>(i,new_id));
      new_id++;
    }
  }
  // clean up the ends of the branch vectors
  size = size - new_id;
  for (i=0; i<size; i++) {
    p_branches.pop_back();
  }

  // remove inactive buses
  size = p_buses.size();
  new_id = 0;
  for (i=0; i<size; i++) {
    if (p_buses[i].p_activeBus) {
      p_buses[new_id] = p_buses[i];
      buses.insert(std::pair<int, int>(i,new_id));
      new_id++;
    }
  }
  // clean up the ends of the bus vectors
  size = size - new_id;
  for (i=0; i<size; i++) {
    p_buses.pop_back();
  }

  // reset all local indices
  size = p_branches.size();
  for (i=0; i<size; i++) {
    p = buses.find(p_branches[i].p_localBusIndex1);
    if (p != buses.end()) {
      p_branches[i].p_localBusIndex1 = p->second;
    } else {
      p_branches[i].p_localBusIndex1 = -1;
    }
    p = buses.find(p_branches[i].p_localBusIndex2);
    if (p != buses.end()) {
      p_branches[i].p_localBusIndex2 = p->second;
    } else {
      p_branches[i].p_localBusIndex2 = -1;
    }
  }

  int jsize;
  size = p_buses.size();
  for (i=0; i<size; i++) {
    std::vector<int> neighbors = p_buses[i].p_branchNeighbors;
    jsize = neighbors.size();
    p_buses[i].p_branchNeighbors.clear();
    for (j=0; j<jsize; j++) {
      p = branches.find(neighbors[j]);
      if (p != branches.end()) {
        p_buses[i].p_branchNeighbors.push_back(p->second);
      }
    }
  }
  if (p_refBus != -1) {
    p_refBus = buses[p_refBus];
  }
}

/**
 * Allocate buffers for exchanging data for ghost buses
 * @param size size (in bytes) of buffer
 */
void allocXCBus(int size)
{
  if (size < 0) {
    // TODO: some kind of error
  }
  // Clean out existing buffers if they are allocated
  if (p_busXCBufSize != 0) {
    if (p_busXCBuffers) {
      delete [] ((char*)p_busXCBuffers);
    } else {
      // TODO: some kind of error
    }
    p_busXCBufSize = 0;
  }
  // Allocate new buffers if size is greater than zero
  int nsize = p_buses.size();
  if (size > 0 && nsize > 0) {
    p_busXCBufSize = size;
    p_busXCBuffers = (void*)(new char[size*nsize]);
  }
}

/**
 * Free buffers for exchange of bus data
 */
void freeXCBus(void)
{
  // Clean out existing buffers if they are allocated
  if (p_busXCBufSize != 0) {
    if (p_busXCBuffers) {
      delete [] ((char*)p_busXCBuffers);
    } else {
      // TODO: some kind of error
    }
    p_busXCBufSize = 0;
  }
}

/**
 * Return a pointer to exchange buffer for bus
 * @param idx local index of bus
 * @return pointer to exchange buffer
 */
void* getXCBusBuffer(int idx)
{
  if (idx < 0 || idx > p_buses.size()) {
    // TODO: some kind of error
    return NULL;
  } else {
    return (void*)(((char*)p_busXCBuffers)+idx*p_busXCBufSize);
  }
}

/**
 * Allocate buffers for exchanging data for ghost branches
 * @param size size (in bytes) of buffer
 */
void allocXCBranch(int size)
{
  if (size < 0) {
    // TODO: some kind of error
  }
  // Clean out existing buffers if they are allocated
  if (p_branchXCBufSize != 0) {
    if (p_branchXCBuffers) {
      delete [] ((char*)p_branchXCBuffers);
    } else {
      // TODO: some kind of error
    }
    p_branchXCBufSize = 0;
  }
  // Allocate new buffers if size is greater than zero
  int nsize = p_branches.size();
  if (size > 0 && nsize > 0) {
    p_branchXCBufSize = size;
    p_branchXCBuffers = (void*)(new char[size*nsize]);
  }
}

/**
 * Free buffers for exchange of bus data
 */
void freeXCBranch(void)
{
  // Clean out existing buffers if they are allocated
  if (p_branchXCBufSize != 0) {
    if (p_branchXCBuffers) {
      delete [] ((char*)p_branchXCBuffers);
    } else {
      // TODO: some kind of error
    }
    p_branchXCBufSize = 0;
  }
}

/**
 * Return a pointer to exchange buffer for bus
 * @param idx local index of bus
 * @return pointer to exchange buffer
 */
void* getXCBranchBuffer(int idx)
{
  if (idx < 0 || idx > p_branches.size()) {
    // TODO: some kind of error
    return NULL;
  } else {
    return (void*)(((char*)p_branchXCBuffers)+idx*p_branchXCBufSize);
  }
}

/**
 * This function must be called before calling the update bus routine.
 * It initializes data structures for the bus update
 */
void initBusUpdate(void)
{
  int i, size, numBuses;
  int grp = this->communicator().getGroup();
  GA_Pgroup_sync(grp);
  // Don't do anything if buffers are not allocated
  if (p_busXCBufSize > 0) {
    // Clean up old GA, if it exists
    if (p_busGASet) {
      GA_Destroy(p_busGA);
      NGA_Deregister_type(p_busXCBufType);
    }
    if (p_activeBusIndices) {
      for (i=0; i<0; i<p_numActiveBuses) {
        delete p_activeBusIndices[i];
      }
      delete [] p_activeBusIndices;
      p_activeBusIndices = NULL;
    }
    if (p_busSndBuf) {
      delete [] ((char*)p_busSndBuf);
      p_busSndBuf = NULL;
    }
    if (p_inactiveBusIndices) {
      for (i=0; i<0; i<p_numInactiveBuses) {
        delete p_inactiveBusIndices[i];
      }
      delete [] p_inactiveBusIndices;
      p_inactiveBusIndices = NULL;
    }
    if (p_busRcvBuf) {
      delete [] ((char*)p_busRcvBuf);
      p_busRcvBuf = NULL;
    }
    // Find out how many active buses exist
    size = p_buses.size();
    numBuses = 0;
    int idx, icnt = 0, lcnt=0;
    for (i=0; i<size; i++) {
      if (getActiveBus(i)) {
        numBuses++;
        lcnt++;
      } else {
        icnt++;
      }
    }
    // Construct GA that can hold exchange data for all active buses
    int nprocs = GA_Pgroup_nnodes(grp);
    int me = GA_Pgroup_nodeid(grp);
    int *totBuses = new int[nprocs];
    int *distr = new int[nprocs];
    for (i=0; i<nprocs; i++) {
      if (me == i) {
        totBuses[i] = numBuses;
      } else {
        totBuses[i] = 0;
      }
    }
    GA_Pgroup_igop(grp,totBuses,nprocs,"+");
    distr[0] = 0;
    p_busTotal = totBuses[0];
    for (i=1; i<nprocs; i++) {
      distr[i] = distr[i-1] + totBuses[i-1];
      p_busTotal += totBuses[i];
    }
    p_busGA = GA_Create_handle();
    int one = 1;
    p_busXCBufType = NGA_Register_type(p_busXCBufSize);
    GA_Set_data(p_busGA, one, &p_busTotal, p_busXCBufType);
    GA_Set_irreg_distr(p_busGA, distr, &nprocs);
    GA_Set_pgroup(p_busGA, grp);
    GA_Allocate(p_busGA);
    p_busGASet = true;

    if (lcnt > 0) {
      p_activeBusIndices = new int*[lcnt];
    }
    for (i=0; i<lcnt; i++) {
      p_activeBusIndices[i] = new int;
    }
    p_numActiveBuses = lcnt;
    if (lcnt > 0) {
      p_busSndBuf = new char[lcnt*p_busXCBufSize];
    }

    p_numInactiveBuses = icnt;
    if (icnt > 0) {
      p_inactiveBusIndices = new int*[icnt];
    }
    for (i=0; i<icnt; i++) {
      p_inactiveBusIndices[i] = new int;
    }
    if (icnt > 0) {
      p_busRcvBuf = new char[icnt*p_busXCBufSize];
    }
    lcnt = 0;
    icnt = 0;
    for (i=0; i<size; i++) {
      if (getActiveBus(i)) {
        *(p_activeBusIndices[lcnt]) = getGlobalBusIndex(i);
        idx = *(p_activeBusIndices[lcnt]);
        if (idx<0 || idx >= p_busTotal) {
          // TODO: some kind of error
        }
        lcnt++;
      } else {
        *(p_inactiveBusIndices[icnt]) = getGlobalBusIndex(i);
        idx = *(p_inactiveBusIndices[icnt]);
        if (idx<0 || idx >= p_busTotal) {
          // TODO: some kind of error
        }
        icnt++;
      }
    }
    delete [] totBuses;
    delete [] distr;
  }
  GA_Pgroup_sync(grp);
}

/**
 * Update the bus ghost values. This is a
 * collective operation across all processors.
 */
void updateBuses(void)
{
  int grp = this->communicator().getGroup();
  // Copy data from XC buffer to send buffer
  GA_Pgroup_sync(grp);
  int i, j, xc_off, rs_off, icnt, nbus;
  char *rs_ptr, *xc_ptr;
  nbus = numBuses();
  icnt = 0;
  for (i=0; i<nbus; i++) {
    if (getActiveBus(i)) {
      xc_off = i*p_busXCBufSize;
      rs_off = icnt*p_busXCBufSize;
      xc_ptr = ((char*)p_busXCBuffers)+xc_off;
      rs_ptr = ((char*)p_busSndBuf)+rs_off;
      memcpy(rs_ptr, xc_ptr, p_busXCBufSize);
      //    for (j=0; j<p_busXCBufSize; j++) {
      //      rs_ptr[j] = xc_ptr[j];
      //    }
      icnt++;
    }
  }

  // Scatter data to exchange GA and then gather it back to local buffers
  if (p_numActiveBuses > 0) {
    NGA_Scatter(p_busGA,p_busSndBuf,p_activeBusIndices,p_numActiveBuses);
  }
  GA_Pgroup_sync(grp);
  if (p_numInactiveBuses > 0) {
    NGA_Gather(p_busGA,p_busRcvBuf,p_inactiveBusIndices,p_numInactiveBuses);
  }
  GA_Pgroup_sync(grp);

  // Copy data from recieve buffer to XC buffer
  icnt = 0;
  for (i=0; i<nbus; i++) {
    if (!getActiveBus(i)) {
      xc_off = i*p_busXCBufSize;
      rs_off = icnt*p_busXCBufSize;
      xc_ptr = ((char*)p_busXCBuffers)+xc_off;
      rs_ptr = ((char*)p_busRcvBuf)+rs_off;
      memcpy(xc_ptr, rs_ptr, p_busXCBufSize);
      //    for (j=0; j<p_busXCBufSize; j++) {
      //      xc_ptr[j] = rs_ptr[j];
      //    }
      icnt++;
    }
  }
  GA_Pgroup_sync(grp);
}

/**
 * This function must be called before calling the update branch routine.
 * It initializes data structures for the branch update
 */
void initBranchUpdate(void)
{
  int i, size, numBranches;
  int grp = this->communicator().getGroup();
  GA_Pgroup_sync(grp);
  // Don't do anything if buffers are not allocated
  if (p_branchXCBufSize > 0) {
    // Clean up old GA, if it exists
    if (p_branchGASet) {
      GA_Destroy(p_branchGA);
      NGA_Deregister_type(p_branchXCBufType);
    }
    if (p_activeBranchIndices) {
      for (i=0; i<0; i<p_numActiveBranches) {
        delete p_activeBranchIndices[i];
      }
      delete [] p_activeBranchIndices;
      p_activeBranchIndices = NULL;
      if (p_branchSndBuf) {
        delete [] ((char*)p_branchSndBuf);
        p_branchSndBuf = NULL;
      }
    }
    if (p_inactiveBranchIndices) {
      for (i=0; i<0; i<p_numInactiveBranches) {
        delete p_inactiveBranchIndices[i];
      }
      delete [] p_inactiveBranchIndices;
      p_inactiveBranchIndices = NULL;
      if (p_branchRcvBuf) {
        delete [] ((char*)p_branchRcvBuf);
        p_branchRcvBuf = NULL;
      }
    }
    // Find out how many active branches exist
    size = p_branches.size();
    numBranches = 0;
    for (i=0; i<size; i++) {
      if (getActiveBranch(i)) {
        numBranches++;
      }
    }
    // Construct GA that can hold exchange data for all active branches
    int nprocs = GA_Pgroup_nnodes(grp);
    int me = GA_Pgroup_nodeid(grp);
    int *totBranches = new int(nprocs);
    int *distr = new int(nprocs);
    for (i=0; i<nprocs; i++) {
      if (me == i) {
        totBranches[i] = numBranches;
      } else {
        totBranches[i] = 0;
      }
    }
    GA_Pgroup_igop(grp,totBranches,nprocs,"+");
    distr[0] = 0;
    p_branchTotal = totBranches[0];
    for (i=1; i<nprocs; i++) {
      distr[i] = distr[i-1] + totBranches[i-1];
      p_branchTotal += totBranches[i];
    }
    p_branchGA = GA_Create_handle();
    int one = 1;
    p_branchXCBufType = NGA_Register_type(p_branchXCBufSize);
    GA_Set_data(p_branchGA, one, &p_branchTotal, p_branchXCBufType);
    GA_Set_irreg_distr(p_branchGA, distr, &nprocs);
    GA_Set_pgroup(p_branchGA, grp);
    GA_Allocate(p_branchGA);
    p_branchGASet = true;

    // Sort buses into local and ghost lists
    int idx, icnt = 0, lcnt=0;
    for (i=0; i<size; i++) {
      if (getActiveBranch(i)) {
        lcnt++;
      } else {
        icnt++;
      }
    }
    p_numActiveBranches = lcnt;
    p_activeBranchIndices = new int*[lcnt];
    p_branchSndBuf = new char[lcnt*p_branchXCBufSize];
    p_numInactiveBranches = icnt;
    p_inactiveBranchIndices = new int*[icnt];
    p_branchRcvBuf = new char[icnt*p_branchXCBufSize];
    lcnt = 0;
    icnt = 0;
    for (i=0; i<size; i++) {
      if (getActiveBranch(i)) {
        p_activeBranchIndices[lcnt] = new int(getGlobalBranchIndex(i));
        idx = *(p_activeBranchIndices[lcnt]);
        if (idx<0 || idx >= p_branchTotal) {
          // TODO: some kind of error
        }
        lcnt++;
      } else {
        p_inactiveBranchIndices[icnt] = new int(getGlobalBranchIndex(i));
        idx = *(p_inactiveBranchIndices[icnt]);
        if (idx<0 || idx >= p_branchTotal) {
          // TODO: some kind of error
        }
        icnt++;
      }
    }
    delete totBranches;
    delete distr;
  }
  GA_Pgroup_sync(grp);
}

/**
 * Update the branch ghost values. This is a
 * collective operation across all processors.
 */
void updateBranches(void)
{
  // Copy data from XC buffer to send buffer
  int grp = this->communicator().getGroup();
  GA_Pgroup_sync(grp);
  int i, j, xc_off, rs_off, icnt, nbranch;
  char *rs_ptr, *xc_ptr;
  nbranch = numBranches();
  icnt = 0;
  for (i=0; i<nbranch; i++) {
    if (getActiveBranch(i)) {
      xc_off = i*p_branchXCBufSize;
      rs_off = icnt*p_branchXCBufSize;
      xc_ptr = ((char*)p_branchXCBuffers)+xc_off;
      rs_ptr = ((char*)p_branchSndBuf)+rs_off;
      memcpy(rs_ptr, xc_ptr, p_branchXCBufSize);
      //    for (j=0; j<p_branchXCBufSize; j++) {
      //      rs_ptr[j] = xc_ptr[j];
      //    }
      icnt++;
    }
  }

  // Scatter data to exchange GA and then gather it back to local buffers
  if (p_numActiveBranches > 0) {
    NGA_Scatter(p_branchGA,p_branchSndBuf,p_activeBranchIndices,p_numActiveBranches);
  }
  GA_Pgroup_sync(grp);
  if (p_numInactiveBranches > 0) {
    NGA_Gather(p_branchGA,p_branchRcvBuf,p_inactiveBranchIndices,p_numInactiveBranches);
  }
  GA_Pgroup_sync(grp);

  // Copy data from recieve buffer to XC buffer
  icnt = 0;
  for (i=0; i<nbranch; i++) {
    if (!getActiveBranch(i)) {
      xc_off = i*p_branchXCBufSize;
      rs_off = icnt*p_branchXCBufSize;
      xc_ptr = ((char*)p_branchXCBuffers)+xc_off;
      rs_ptr = ((char*)p_branchRcvBuf)+rs_off;
      memcpy(xc_ptr, rs_ptr, p_branchXCBufSize);
      //    for (j=0; j<p_branchXCBufSize; j++) {
      //      xc_ptr[j] = rs_ptr[j];
      //    }
      icnt++;
    }
  }
  GA_Pgroup_sync(grp);
}

void
writeGraph(const std::string& outname) 
  {
    static const bool internal_indexes(false);
    std::ofstream out;
    if (this->processor_rank() == 0) {
      out.open(outname.c_str(), std::ofstream::out | std::ofstream::trunc);
      out << "digraph {" << std::endl;
      out.close();
    }

    // write out the (active) buses as nodes, one cluster per processor
    for (int p = 0; p < this->processor_size(); ++p) {
      if (p == this->processor_rank()) {
        out.open(outname.c_str(), std::ofstream::out | std::ofstream::app);
        out << "subgraph cluster_" << p << " {" << std::endl;
        out << "color=red" << std::endl;
        out << "label=" << p << ";" << std::endl;
        BusIterator bus;
        for (bus = p_buses.begin(); bus != p_buses.end(); ++bus) {
          if (bus->p_activeBus) {
            int bidx(internal_indexes ? bus->p_globalBusIndex : bus->p_originalBusIndex);
            out << " n" << bidx << "[label=" << bidx << "];" << std::endl;
          }
        }
        out << "}" << std::endl;
        out.close();
      }
      this->communicator().barrier();
    }

    // write out the (both active and inactive) branches as edges
    for (int p = 0; p < this->processor_size(); ++p) {
      if (p == this->processor_rank()) {
        out.open(outname.c_str(), std::ofstream::out | std::ofstream::app);
        BranchIterator branch;
        for (branch = p_branches.begin(); branch != p_branches.end(); ++branch) {
          if (branch->p_activeBranch) {
            int bidx1(internal_indexes ? branch->p_globalBusIndex1 : branch->p_originalBusIndex1);
            int bidx2(internal_indexes ? branch->p_globalBusIndex2 : branch->p_originalBusIndex2);
            out << "n" << bidx1 << " -> " 
                << "n" << bidx2 << ";" 
                << std::endl;
          }
        }
        out.close();
      }
      this->communicator().barrier();
    }

    this->communicator().barrier();
    if (this->processor_rank() == 0) {
      out.open(outname.c_str(), std::ofstream::out | std::ofstream::app);
      out << "   } /* end */" << std::endl;
      out.close();
    }

    // write a graph of each processors local network
    for (int p = 0; p < this->processor_size(); ++p) {
      if (p == this->processor_rank()) {
        out.open(outname.c_str(), std::ofstream::out | std::ofstream::app);
        out << "digraph \"" << p << "\" {" << std::endl;
        out << "label=\"Process " << p << "\";" << std::endl;
        out << "node [color=lightgrey];" << std::endl;
        BusIterator bus;
        for (bus = p_buses.begin(); bus != p_buses.end(); ++bus) {
          std::string color("black");
          std::string style("\"\"");
          if (!bus->p_activeBus) {
            color = "red";
            style = "dotted";
          }
          int bidx(internal_indexes ? bus->p_globalBusIndex : bus->p_originalBusIndex);
          out << " n" << bidx
              << " ["
              << "label=" << bidx << ", "
              << "color=" << color << ", "
              << "style=" << style 
              << "];" << std::endl;
        }
        BranchIterator branch;
        for (branch = p_branches.begin(); branch != p_branches.end(); ++branch) {
          std::string color("black");
          std::string style("solid");
          if (!branch->p_activeBranch) {
            color = "red";
            style = "dotted";
          }
          int bidx1(internal_indexes ? branch->p_globalBusIndex1 : branch->p_originalBusIndex1);
          int bidx2(internal_indexes ? branch->p_globalBusIndex2 : branch->p_originalBusIndex2);
          out << "n" << bidx1 << " -> " 
              << "n" << bidx2 << " " 
              << "[" 
              << "color=" << color << ", "
              << "style=" << style 
              << "]" << ";"
              << std::endl;
        }
        out << "} /* end process " << p << " */" << std::endl;
        out.close();
      }
      this->communicator().barrier();
    }
  }

protected:

/**
 * Protected copy constructor to avoid unwanted copies.
 */
BaseNetwork(const BaseNetwork& old)
{
}

private:

  // add some typedefs so things are more readable and we don't have
  // to type so much

  typedef boost::shared_ptr< BusData<BusType> > BusDataPtr;
  typedef std::vector<  BusData<BusType> > BusDataVector;
  typedef typename BusDataVector::iterator BusIterator;
  typedef boost::shared_ptr< BranchData<BranchType> > BranchDataPtr;
  typedef std::vector< BranchData<BranchType> > BranchDataVector;
  typedef typename BranchDataVector::iterator BranchIterator;

  /**
   * Vector of bus data and objects
   */
  BusDataVector p_buses;

  /**
   * Vector of branch data and objects
   */
  BranchDataVector p_branches;

  /**
   * Parameter for keeping track of reference bus
   */
  int p_refBus;

  /**
   * Vector of buffers for exchange of bus data to ghost buses
   */
  int p_busXCBufSize;
  void *p_busXCBuffers;

  /**
   * Vector of buffers for exchange of branch data to ghost branches
   */
  int p_branchXCBufSize;
  void *p_branchXCBuffers;

  /**
   * Global array handle and other parameters used for bus exchanges
   * Note that p_(in)activeBusIndices must be a int** pointer to match syntax of GA
   * gather/scatter calls
   */
  int p_busGA;
  bool p_busGASet;
  int p_busXCBufType;
  int p_busTotal;
  int **p_inactiveBusIndices;
  int p_numInactiveBuses;
  int **p_activeBusIndices;
  int p_numActiveBuses;
  void *p_busSndBuf;
  void *p_busRcvBuf;

  /**
   * Global array handle and other parameters used for branch exchanges
   * Note that p_(in)activeBranchIndices must be a int** pointer to match
   * syntax of GA gather/scatter calls
   */
  int p_branchGA;
  bool p_branchGASet;
  int p_branchXCBufType;
  int p_branchTotal;
  int **p_inactiveBranchIndices;
  int p_numInactiveBranches;
  int **p_activeBranchIndices;
  int p_numActiveBranches;
  void *p_branchSndBuf;
  void *p_branchRcvBuf;
};
}  //namespace network
}  //namespace gridpack

#endif
