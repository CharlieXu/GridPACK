// Emacs Mode Line: -*- Mode:c++;-*-
/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   hash_distr.hpp
 * @author Bruce Palmer
 * @date   2014-07-28 09:25:03 d3g293
 * 
 * @brief  
 * This is a utility that is designed to move a set of data for a collection of
 * buses and/or branches based on the process that owns the buses or branches.
 * Ownership is based on the original index of the bus or branch
 * 
 */

// -------------------------------------------------------------

#ifndef _hash_distr_hpp_
#define _hash_distr_hpp_

#define SYSTOLIC

#include <ga.h>
#include <boost/unordered_map.hpp>
#include "gridpack/parallel/index_hash.hpp"

namespace gridpack {
namespace hash_distr {

// -------------------------------------------------------------
//  class HashDistribution
// -------------------------------------------------------------
template <typename _network,
          typename _bus_data_type,
          typename _branch_data_type>
 class HashDistribution {
private:

#ifdef SYSTOLIC
  typedef struct{int idx;
           _bus_data_type data;
  } bus_data_pair;

  typedef struct{int idx1;
           int idx2;
           _branch_data_type data;
  } branch_data_pair;
#else
  typedef struct{bool flag;
           _bus_data_type data;
  } bus_data_pair;

  typedef struct{bool flag;
           _branch_data_type data;
  } branch_data_pair;
#endif

public:
  typedef _network NetworkType;
  typedef boost::shared_ptr<NetworkType> NetworkPtr;

  // Default constructor
  // @param network network over which hash distribution function extends
  HashDistribution(const boost::shared_ptr<_network> network)
    : p_network(network)
  {
    p_hashMap.reset(new
        gridpack::hash_map::GlobalIndexHashMap(p_network->communicator()));
    p_GAgrp = p_network->communicator().getGroup();

    // Initialize hash map using original bus indices and global indices
    int i,ikey,ival,idx1,idx2;
    std::vector<std::pair<int,int> > busPairs;
    int nbus = p_network->numBuses();
    for (i=0; i<nbus; i++) {
      ikey = p_network->getOriginalBusIndex(i);
      ival = p_network->getGlobalBusIndex(i);
      busPairs.push_back(std::pair<int,int>(ikey,ival));
    }
    p_hashMap->addPairs(busPairs);
    busPairs.clear();
    std::pair<int,int> key;
    std::vector<std::pair<std::pair<int,int>, int> >
      branchPairs;
    int nbranch = p_network->numBranches();
    for (i=0; i<nbranch; i++) {
      p_network->getOriginalBranchEndpoints(i,&idx1,&idx2);
      key = std::pair<int,int>(idx1,idx2);
      ival =
        p_network->getGlobalBranchIndex(i);
      branchPairs.push_back(std::pair<std::pair<int,int>,int>(key,ival));
    }
    p_hashMap->addPairs(branchPairs);
    branchPairs.clear();

    //store size of data items
    p_size_bus_data = sizeof(bus_data_pair);
    p_size_branch_data = sizeof(branch_data_pair);
  }

  // Default destructor
  ~HashDistribution(void)
  {
  }

  // Send values corresponding to keys to the processors that own them. On
  // completion, keys contain the local indices of the buses recieving data and
  // values contains the data
  // @param keys on input, a list of integer keys corresponding to the original
  // bus indices of the buses that receive the data, on output, a list of local
  // bus indices
  // @param values on input, a list of values corresponding to the list of keys,
  // on output, the values of the received data
  void distributeBusValues(std::vector<int> &keys, std::vector<_bus_data_type> &values)
  {
#ifdef SYSTOLIC
    int ksize = keys.size();
    int vsize = values.size();
    int me = GA_Pgroup_nodeid(p_GAgrp);
    int nprocs = GA_Pgroup_nnodes(p_GAgrp);
    if (vsize != ksize) {
      //TODO: some kind of error
      printf("p[%d] (distributeBusValues) ERROR: length of keys and values"
          " arrays don't match ksize: %d vsize: %d\n",me,ksize,vsize);
      return;
    }
    
    // Construct mapc array for irregular distribution
    int i;
    int *sizes = new int[nprocs];
    for (i=0; i<nprocs; i++) {
      sizes[i] = 0;
    }
    sizes[me] = ksize;
    GA_Pgroup_igop(p_GAgrp,sizes,nprocs,"+");
    int *mapc = new int[nprocs];
    mapc[0] = 0;
    int total_values = sizes[0];
    for (i=1; i<nprocs; i++) {
      mapc[i] = mapc[i-1]+sizes[i-1];
      total_values += sizes[i];
    }

    // Copy values to a straight array
    bus_data_pair *list = new bus_data_pair[ksize];
    int g_type = NGA_Register_type(p_size_bus_data);
    for (i=0; i<ksize; i++) {
      list[i].idx = keys[i];
      list[i].data = values[i];
    }

    // Create global array and store all values in it
    int lo, hi;
    lo = 0;
    for (i=0; i<me; i++) {
      lo += sizes[i];
    }
    hi = lo + sizes[me] - 1;
    int one = 1;
    int g_vals = GA_Create_handle();
    GA_Set_data(g_vals,one,&total_values,g_type);
//    GA_Set_irreg_distr(g_vals,mapc,&nprocs);
    GA_Set_pgroup(g_vals,p_GAgrp);
    if (GA_Allocate(g_vals)) {
      //TODO: some kind of error
    }
    if (lo <= hi) NGA_Put(g_vals, &lo, &hi, list, &one);
    GA_Pgroup_sync(p_GAgrp);
    NGA_Deregister_type(g_type);
    delete [] list;
    delete [] mapc;
    delete [] sizes;

    // Create a map that maps original bus indices to local indices
    int idx;
    int nbus = p_network->numBuses();
    boost::unordered_map<int,int> hmap;
    for (i=0; i<nbus; i++) {
      idx = p_network->getOriginalBusIndex(i);
      hmap.insert(std::pair<int,int>(idx,i));
    }
    boost::unordered_map<int,int>::iterator it;
    
    // Get values from global array and copy them into output arrays
    keys.clear();
    values.clear();
    double delta = static_cast<double>(total_values)/static_cast<double>(nprocs);
    for (i = 0; i<nprocs; i++) {
      idx = (i+me)%nprocs;
      lo = static_cast<int>(delta*static_cast<double>(idx));
      if (idx<nprocs-1) {
        hi = static_cast<int>(delta*static_cast<double>(idx+1))-1;
      } else {
        hi = total_values-1;
      }
      int nsize = hi - lo + 1;
      list = new bus_data_pair[nsize];
      if (lo <= hi) NGA_Get(g_vals, &lo, &hi, list, &one);
      int j;
      for (j=0; j<nsize; j++) {
        it = hmap.find(list[j].idx);
        if (it != hmap.end()) {
          keys.push_back(it->second);
          values.push_back(list[j].data);
        }
      }
      delete [] list;
    }
    GA_Pgroup_sync(p_GAgrp);
#else
    // Get global indices corresponding to keys
    std::vector<int> g_idx;
    p_hashMap->getValues(keys,g_idx);

    // construct map containing number of entries for each global index value
    int nsize = g_idx.size();
    int i, j;
    boost::unordered_map<int,int> index_map;
    boost::unordered_map<int,int>::iterator ip;
    for (i=0; i<nsize; i++) {
      ip = index_map.find(g_idx[i]);
      if (ip == index_map.end()) {
        index_map.insert(std::pair<g_idx[i],1>);
      } else {
        ip->second++;
      }
    }

    int nprocs = GA_Pgroup_nnodes(p_GAgrp);
    int me = GA_Pgroup_nodeid(p_GAgrp);
    int totalBuses = p_network->totalBuses();
    int one = 1;
    int two = 2;
    // Create global array to store measurement counts
    int dims[2];
    dims[0] = totalBuses;
    dims[1] = nprocs;
    int blocks[2];
    blocks[0] = -1;
    blocks[1] = nprocs;
    int g_type = NGA_Register_type(p_size_bus_data);
    int g_cnt = GA_Create_handle();
    GA_Set_data(g_cnt,two,dims,g_type);
    GA_Set_chunk(g_cnt,blocks);
    GA_Set_pgroup(g_bus,p_GAgrp);
    GA_Allocate(g_bus);
    GA_Zero(g_bus);

    // Set up data structures to scatter counts to g_cnt array
    nsize = index_map.size();
    int **indices = new int*[nsize];
    int *idx = new int[2*nsize];
    int *ival = new int[nsize];
    int ncnt = 0;
    ip = index_map.begin();
    while (ip != index_map.end()) {
      idx[2*ncnt] = ip->first;
      idx[2*ncnt+1] = me;
      index[ncnt] = &idx[2*ncnt];
      ival[ncnt] = ip->second;
      ncnt++;
    }
    NGA_Scatter_acc(g_cnt, ival, index, ncnt, one);
    // A complete set of counts is now in g_cnt. Figure out total offsets on
    // each processor and figure out individual offsets
    int lo[2], hi[2], ld;
    int *cnt_ptr;
    NGA_Distribution(g_cnt,me,lo,hi);
    NGA_Access(g_cnt,lo,hi,&cnt_ptr,&ld);
    int idim = hi[0]-lo[0]+1;
    int jdim = hi[1]-lo[1]+1;
    int offset = 0;
    ncnt = 0
    for (i = 0; i<idim; i++) {
      // second index is fast index
      for (j = 0; j<jdim; j++) {
        offset += cnt_ptr[ncnt];
        cnt_ptr[ncnt] = offset;
        ncnt++;
      }
    }
    int *offsets = new int[nprocs];
    for (i=0; i<nprocs; i++) {
      offsets[i] = 0;
    }
    offsets[me] = offset;
    GA_Pgroup_igop(p_GAgrp,offsets,nprocs,"+");
    offset = offsets[me];
    ncnt = 0
    for (i = 0; i<idim; i++) {
      for (j = 0; j<jdim; j++) {
        cnt_ptr[ncnt] += offset;
        ncnt++;
      }
    }
    NGA_Release(g_cnt,lo,hi);
    GA_Pgroup_sync(p_GAgrp);
    int total_values = 0;
    for (i=0; i<nprocs; i++) {
      total_values += offsets[i];
    }
    // get offset for each bus that has values
    NGA_Gather(g_cnt, ival, index, ncnt);
    for (i=0; i<ncnt; i++) {
      j = idx[2*i];
      ip = index_map.find(j);
      ip->second = ival[i];
    }
    delete [] indices;
    delete [] idx;
    delete [] ival;
    // set up arrays to push data to GA
    int **indices = new int*[nsize];
    int *idx = new int[nsize];
    nsize = keys.size();
    bus_data_pair *values_buf = new bus_data_pair[nsize];
    for (i=0; i<nsize; i++) {
      ip = index_map.find(g_idx[i]);
      idx[i] = ip->second;
      index[i] = &idx[i]
      ip->second++;
      values_buf[i].flag = true;
      values_buf[i].data = values[i];
    }
    

    // Create global array to distribute data
    int g_bus = GA_Create_handle();
    int one = 1;
    GA_Set_data(g_bus,one,&total_values,g_type);
    GA_Set_pgroup(g_bus,p_GAgrp);
    GA_Allocate(g_bus);
    NGA_Deregister_type(g_type);

    // initialize all data pairs in global array to false
    bus_data_pair *list;
    NGA_Distribution(g_bus,me,lo,hi);
    nsize = hi[0] - lo[0] + 1;
    NGA_Access(g_bus,lo,hi,&list,&ld);
    for (i=0; i<nsize; i++) {
      list[i].flag = false;
    }
    NGA_Release(g_bus,lo,hi);

    // Scatter data to global array
    NGA_Scatter(g_bus,values_buf,idx,nsize);
    GA_Pgroup_sync(p_GAgrp);
    delete [] index;
    delete [] idx;
    delete [] values_buf;
    keys.clear();
    values.clear();

    // Set up local arrays to receive data
    nsize = p_network->numBuses();
    index = new int[nsize];
    idx = new int*[nsize];
    values_buf = new bus_data_pair[nsize];
    int icnt = 0;
    for (i=0; i<nsize; i++) {
      if (p_network->getActiveBus(i)) {
        index[icnt] = p_network->getGlobalBusIndex(i);
        idx[icnt] = &index[icnt];
        icnt++;
      }
    }

    // Gather data to local buffers
    NGA_Gather(g_bus,values_buf,idx,icnt);

    // Copy data back to vectors
    icnt = 0;
    for (i=0; i<nsize; i++) {
      if (p_network->getActiveBus(i)) {
        if (values_buf[icnt].flag) {
          keys.push_back(i);
          values.push_back(values_buf[icnt].data);
        }
        icnt++;
      }
    }
    delete [] index;
    delete [] idx;
    delete [] values_buf;
    GA_Destroy(g_bus);
#endif
  }

  // Send values corresponding to keys to the processors that own them. On
  // completion, keys contain the local indices of the branches recieving data
  // and
  // values contains the data
  // @param keys on input, a list of integer pair keys corresponding to the
  // branches that receive the data, on output, a list of local branch indices
  // @param values on input, a list of values corresponding to the list of keys,
  // on output, the values of the received data
  void distributeBranchValues(std::vector<std::pair<int,int> > &keys,
      std::vector<int> &branch_ids,
      std::vector<_branch_data_type> &values)
  {
#ifdef SYSTOLIC
    int ksize = keys.size();
    int vsize = values.size();
    int me = GA_Pgroup_nodeid(p_GAgrp);
    int nprocs = GA_Pgroup_nnodes(p_GAgrp);
    if (vsize != ksize) {
      //TODO: some kind of error
      printf("p[%d] (distributeBranchValues) ERROR: length of keys and values"
          " arrays don't match ksize: %d vsize: %d\n",me,ksize,vsize);
      return;
    }
    
    // Construct mapc array for irregular distribution
    int i;
    int *sizes = new int[nprocs];
    for (i=0; i<nprocs; i++) {
      sizes[i] = 0;
    }
    sizes[me] = ksize;
    GA_Pgroup_igop(p_GAgrp,sizes,nprocs,"+");
    int *mapc = new int[nprocs];
    mapc[0] = 0;
    int total_values = sizes[0];
    for (i=1; i<nprocs; i++) {
      mapc[i] = mapc[i-1]+sizes[i-1];
      total_values += sizes[i];
    }

    // Copy values to a straight array
    branch_data_pair *list = new branch_data_pair[ksize];
    int g_type = NGA_Register_type(p_size_branch_data);
    for (i=0; i<ksize; i++) {
      list[i].idx1 = keys[i].first;
      list[i].idx2 = keys[i].second;
      list[i].data = values[i];
    }

    // Create global array and store all values in it
    int lo, hi;
    lo = 0;
    for (i=0; i<me; i++) {
      lo += sizes[i];
    }
    hi = lo + sizes[me] - 1;
    int one = 1;
    int g_vals = GA_Create_handle();
    GA_Set_data(g_vals,one,&total_values,g_type);
//    GA_Set_irreg_distr(g_vals,mapc,&nprocs);
    GA_Set_pgroup(g_vals,p_GAgrp);
    if (GA_Allocate(g_vals)) {
      //TODO: some kind of error
    }
    if (lo <= hi) NGA_Put(g_vals, &lo, &hi, list, &one);
    GA_Pgroup_sync(p_GAgrp);
    NGA_Deregister_type(g_type);
    delete [] list;
    delete [] mapc;
    delete [] sizes;

    // Create a map that maps original branch indices to local indices
    int idx,idx1,idx2;
    int nbranch = p_network->numBranches();
    boost::unordered_map<std::pair<int,int>,int> hmap;
    for (i=0; i<nbranch; i++) {
      p_network->getOriginalBranchEndpoints(i,&idx1,&idx2);
      hmap.insert(std::pair<std::pair<int,int>,int>(std::pair<int,int>(idx1,idx2),i));
    }
    boost::unordered_map<std::pair<int,int>,int>::iterator it;
    
    // Get values from global array and copy them into output arrays
    branch_ids.clear();
    values.clear();
    double delta = static_cast<double>(total_values)/static_cast<double>(nprocs);
    for (i = 0; i<nprocs; i++) {
      idx = (i+me)%nprocs;
      lo = static_cast<int>(delta*static_cast<double>(idx));
      if (idx<nprocs-1) {
        hi = static_cast<int>(delta*static_cast<double>(idx+1))-1;
      } else {
        hi = total_values-1;
      }
      int nsize = hi - lo + 1;
      list = new branch_data_pair[nsize];
      if (lo <= hi) NGA_Get(g_vals, &lo, &hi, list, &one);
      int j;
      std::pair<int,int> key;
      for (j=0; j<nsize; j++) {
        key = std::pair<int,int>(list[j].idx1,list[j].idx2);
        it = hmap.find(key);
        if (it != hmap.end()) {
          branch_ids.push_back(it->second);
          values.push_back(list[j].data);
        }
      }
      delete [] list;
    }
    GA_Pgroup_sync(p_GAgrp);
#else
    // Get global indices corresponding to keys
    std::vector<int> g_idx;
    p_hashMap->getValues(keys,g_idx);
    // Create global array to distribute data
    int g_branch = GA_Create_handle();
    int g_type = NGA_Register_type(p_size_branch_data);
    int one = 1;
    int totalBranches = p_network->totalBranches();
    GA_Set_data(g_branch,one,&totalBranches,g_type);
    GA_Set_pgroup(g_branch,p_GAgrp);
    GA_Allocate(g_branch);
    NGA_Deregister_type(g_type);

    // initialize all data pairs in global array to false
    int i, nsize, lo, hi, ld;
    int me = GA_Nodeid();
    branch_data_pair *list;
    NGA_Distribution(g_branch,me,&lo,&hi);
    NGA_Access(g_branch,&lo,&hi,&list,&ld);
    nsize = hi - lo + 1;
    for (i=0; i<nsize; i++) {
      list[i].flag = false;
    }
    NGA_Release(g_branch,&lo,&hi);

    // Copy keys and values to local arrays
    nsize = values.size();
    int *index = new int[nsize];
    int **idx = new int*[nsize];
    branch_data_pair *values_buf = new branch_data_pair[nsize];
    for (i=0; i<nsize; i++) {
      index[i] = g_idx[i];
      idx[i] = &index[i];
      values_buf[i].flag = true;
      values_buf[i].data = values[i];
    }
    // Scatter data to global array
    NGA_Scatter(g_branch,values_buf,idx,nsize);
    GA_Pgroup_sync(p_GAgrp);
    delete [] index;
    delete [] idx;
    delete [] values_buf;
    g_idx.clear();
    values.clear();

    // Set up local arrays to receive data
    nsize = p_network->numBranches();
    index = new int[nsize];
    idx = new int*[nsize];
    values_buf = new branch_data_pair[nsize];
    int icnt = 0;
    for (i=0; i<nsize; i++) {
      if (p_network->getActiveBranch(i)) {
        index[icnt] = p_network->getGlobalBranchIndex(i);
        idx[icnt] = &index[icnt];
        icnt++;
      }
    }

    // Gather data to local buffers
    NGA_Gather(g_branch,values_buf,idx,icnt);

    // Copy data back to vectors
    icnt = 0;
    for (i=0; i<nsize; i++) {
      if (p_network->getActiveBranch(i)) {
        if (values_buf[icnt].flag) {
          branch_ids.push_back(i);
          values.push_back(values_buf[icnt].data);
        }
        icnt++;
      }
    }
    delete [] index;
    delete [] idx;
    delete [] values_buf;
    GA_Destroy(g_branch);
#endif
  }

private:

  boost::shared_ptr<gridpack::hash_map::GlobalIndexHashMap> p_hashMap;

  NetworkPtr p_network;

  int p_size_bus_data;
  int p_size_branch_data;

  int p_GAgrp;

};


} // namespace gridpack
} // namespace hash_distr

#endif

