/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
/* *****************************************************************************
 * full_map.hpp
 * gridpack
 * kglass, bjpalmer
 * Jul 22, 2013
 *
 * Documentation status
 * Revision status
 * File contents
 **************************************************************************** */

#ifndef FULLMATRIXMAP_HPP_
#define FULLMATRIXMAP_HPP_

#include <boost/smart_ptr/shared_ptr.hpp>
#include <ga.h>
#include "gridpack/parallel/parallel.hpp"
#include <gridpack/parallel/distributed.hpp>
#include <gridpack/component/base_component.hpp>
#include <gridpack/network/base_network.hpp>
#include <gridpack/math/matrix.hpp>

#define DBG_CHECK

namespace gridpack {
namespace mapper {

template <class _network>
class FullMatrixMap {
  public: 
/**
 * Initialize mapper for the given network and the current mode. Create global
 * arrays that contain offsets that will be used to create matrix from the
 * network component objects
 * @param network network that will generate matrix
 */
FullMatrixMap(boost::shared_ptr<_network> network)
  : p_me (GA_Nodeid()), p_nNodes(GA_Nnodes()), p_network(network)
{
  int                     iSize    = 0;
  int                     jSize    = 0;

  p_nBuses = p_network->numBuses();
  p_nBranches = p_network->numBranches();

  p_activeBuses         = getActiveBuses();

  setupGlobalArrays(p_activeBuses);  // allocate globalIndex arrays

  setupIndexingArrays();

  setupOffsetArrays();

  contributions();

}

~FullMatrixMap()
{
  GA_Destroy(gaOffsetI);
  GA_Destroy(gaOffsetJ);
}

/**
 * Generate matrix from current component state on network
 * @return return a pointer to new matrix
 */
boost::shared_ptr<gridpack::math::Matrix> mapToMatrix(void)
{
  gridpack::parallel::Communicator comm = p_network->communicator();
  boost::shared_ptr<gridpack::math::Matrix>
    Ret(new gridpack::math::Matrix(comm, p_rowBlockSize, p_jDim, p_maxrow));
  loadBusData(Ret,true);
  loadBranchData(Ret,true);
  GA_Sync();
  Ret->ready();
  return Ret;
}


/**
 * Reset existing matrix from current component state on network
 * @param matrix existing matrix (should be generated from same mapper)
 */
void mapToMatrix(gridpack::math::Matrix &matrix)
{
  matrix.zero();
  loadBusData(matrix,false);
  loadBranchData(matrix,false);
  GA_Sync();
  matrix.ready();
}

/**
 * Reset existing matrix from current component state on network
 * @param matrix existing matrix (should be generated from same mapper)
 */
void mapToMatrix(boost::shared_ptr<gridpack::math::Matrix> &matrix)
{
  mapToMatrix(*matrix);
}

/**
 * Check to see if matrix looks well formed. This method runs through all
 * branches and verifies that the dimensions of the branch contributions match
 * the dimensions of the bus contributions at each end. If there is a
 * discrepancy, then an error message is generated.
 */

bool check(void)
{
  int i,idx,jdx,isize,jsize,icnt;
  int msize, nsize;
  gridpack::component::BaseBusComponent *bus1;
  gridpack::component::BaseBusComponent *bus2;
  bool ok;
  for (i=0; i<p_nBranches; i++) {
    bus1 = dynamic_cast<gridpack::component::BaseBusComponent*>
      (p_network->getBranch(i)->getBus1().get());
    bus2 = dynamic_cast<gridpack::component::BaseBusComponent*>
      (p_network->getBranch(i)->getBus2().get());
    isize = 0;
    jsize = 0;
    if (p_network->getBranch(i)->matrixForwardSize(&isize,&jsize)) {
      p_network->getBranch(i)->getMatVecIndices(&idx, &jdx);
      if (idx >= p_minRowIndex && idx <= p_maxRowIndex) {
        ok = true;
        msize = 0;
        nsize = 0;
        bus1->matrixDiagSize(&msize, &nsize);
        if (nsize != isize) ok = false;
        msize = 0;
        nsize = 0;
        bus2->matrixDiagSize(&msize, &nsize);
        if (msize != jsize) ok = false;
        if (!ok) {
          printf("Forward mismatch for branch between %d and %d\n",
              bus1->getOriginalIndex(), bus2->getOriginalIndex());
        }
      }
    }
    isize = 0;
    jsize = 0;
    if (p_network->getBranch(i)->matrixReverseSize(&isize,&jsize)) {
      p_network->getBranch(i)->getMatVecIndices(&idx, &jdx);
      if (jdx >= p_minRowIndex && jdx <= p_maxRowIndex) {
        ok = true;
        msize = 0;
        nsize = 0;
        bus1->matrixDiagSize(&msize, &nsize);
        if (msize != jsize) ok = false;
        msize = 0;
        nsize = 0;
        bus2->matrixDiagSize(&msize, &nsize);
        if (nsize != isize) ok = false;
        if (!ok) {
          printf("Reverse mismatch for branch between %d and %d\n",
              bus1->getOriginalIndex(), bus2->getOriginalIndex());
        }
      }
    }
  }
}

private:
/**
 * Return the number of active buses on this process
 * @return number of active buses
 */
int getActiveBuses(void)
{
  int nActiveBuses = 0;
  for (int i = 0; i<p_nBuses; i++) {
    if (p_network->getActiveBus(i)) {
      nActiveBuses++;
    }
  }
  return nActiveBuses;
}

/**
 * Allocate the gaMatBlksI and gaMatBlksJ global arrays
 * @param nActiveBuses the number of active buses on this process
 */
void setupGlobalArrays(int nActiveBuses)
{
  int one = 1;

  p_totalBuses = nActiveBuses;

  GA_Igop(&p_totalBuses,one,"+");

  // the gaMatBlksI and gaMatBlksJ arrays contain the matrix blocks sizes for
  // individual block contributions
  createIndexGA(&gaMatBlksI, p_totalBuses);
  createIndexGA(&gaMatBlksJ, p_totalBuses);
}

/**
 * Create a global array of integers
 * @param size size of global array
 */
void createIndexGA(int * handle, int size)
{
  int one = 1;
  *handle = GA_Create_handle();
  GA_Set_data(*handle, one, &size, C_INT);
  if (!GA_Allocate(*handle)) {
    // TODO: some kind of error
  }
  GA_Zero(*handle);
}

/**
 * Set up global arrays that contain all matrix block sizes along the I and J
 * axes. These will then be used to create offset arrays.
 */
void setupIndexingArrays()
{
  int                    * iSizeArray     = NULL;
  int                    * jSizeArray     = NULL;
  int                   ** iIndexArray    = NULL;
  int                   ** jIndexArray    = NULL;
  int                      icount          = 0;
  int                      jcount          = 0;

  // set up bus indexing
  allocateIndexArray(p_nBuses, &iSizeArray, &jSizeArray, &iIndexArray,
      &jIndexArray);

  loadBusArrays(iSizeArray, jSizeArray, iIndexArray, jIndexArray,
      &icount, &jcount);
  scatterIndexingArrays(iSizeArray, jSizeArray, iIndexArray, jIndexArray,
      icount, jcount);
  deleteIndexArrays(p_nBuses, iSizeArray, jSizeArray, iIndexArray,
      jIndexArray);
  GA_Sync();

  // set up branch indexing
  icount               = 0;
  jcount               = 0;
  allocateIndexArray(p_nBranches, &iSizeArray, &jSizeArray, &iIndexArray,
      &jIndexArray);
  loadForwardBranchArrays(iSizeArray, jSizeArray, iIndexArray, jIndexArray,
      &icount, &jcount);
  scatterIndexingArrays(iSizeArray, jSizeArray, iIndexArray, jIndexArray,
      icount, jcount);

  icount               = 0;
  jcount               = 0;
  loadReverseBranchArrays(iSizeArray, jSizeArray, iIndexArray, jIndexArray,
      &icount, &jcount);
  scatterIndexingArrays(iSizeArray, jSizeArray, iIndexArray, jIndexArray,
      icount, jcount);

  deleteIndexArrays(p_nBranches, iSizeArray, jSizeArray, iIndexArray,
      jIndexArray);
  GA_Sync();
}

/**
 * Allocate arrays that hold sizes and approximate indices of matrix elements
 * @param n number of elements in array
 * @param iSizeArray array containing size of matrix block along i axis
 * @param jSizeArray array containing size of matrix block along j axis
 * @param iIndexArray array containing i index of matrix block
 * @param jIndexArray array containing j index of matrix block
 */
void allocateIndexArray(int n, int ** iSizeArray, int ** jSizeArray,
        int *** iIndexArray, int *** jIndexArray)
{
  *iSizeArray         = new int[n];
  *jSizeArray         = new int[n];
  *iIndexArray        = new int*[n];
  *jIndexArray        = new int*[n];

  for(int i = 0; i < n; i++) {
    (*iIndexArray)[i]  = new int;
    (*jIndexArray)[i]  = new int;
  }
}

/**
 * Load arrays containing matrix block sizes and indices along diagonal of matrix.
 * These come from buses
 * @param iSizeArray array containing size of matrix block along i axis
 * @param jSizeArray array containing size of matrix block along j axis
 * @param iIndexArray array containing i index of matrix block
 * @param jIndexArray array containing j index of matrix block
 * @param icount return total number of non-zero blocks along i axis
 * @param jcount return total number of non-zero blocks along j axis
 */
void loadBusArrays(int * iSizeArray, int * jSizeArray,
        int ** iIndexArray, int ** jIndexArray, int *icount, int *jcount)
{
  int                      index          = 0;
  int                      iSize          = 0;
  int                      jSize          = 0;
  bool                     status         = true;

  *icount = 0;
  *jcount = 0;

  int maxrow,idx,jdx;

  p_maxrow = 0;
  std::vector<boost::shared_ptr<gridpack::component::BaseComponent> > branches;
  
  bool chk;

  for (int i = 0; i < p_nBuses; i++) {
    status = p_network->getBus(i)->matrixDiagSize(&iSize, &jSize);
    if (status) {
      maxrow = 0;
      p_network->getBus(i)->getMatVecIndex(&index);
      if (iSize > 0) {
        iSizeArray[*icount]     = iSize;
        *(iIndexArray[*icount])  = index;
        (*icount)++;
      }
      if (jSize > 0) {
        jSizeArray[*jcount]     = jSize;
        *(jIndexArray[*jcount])  = index;
        (*jcount)++;
      }
      maxrow += jSize;
      branches.clear();
      p_network->getBus(i)->getNeighborBranches(branches);
      for(int j = 0; j<branches.size(); j++) {
        branches[j]->getMatVecIndices(&idx, &jdx);
        if (index == idx) {
          chk = branches[j]->matrixForwardSize(&iSize, &jSize);
        } else {
          chk = branches[j]->matrixReverseSize(&iSize, &jSize);
        }
        if (chk) maxrow += jSize;
      }
      if (p_maxrow < maxrow) p_maxrow = maxrow;
    }
  }
}

/**
 * Load arrays containing matrix block sizes and indices for off-diagonal
 * matrix blocks. These come from branches.
 * @param iSizeArray array containing size of matrix block along i axis
 * @param jSizeArray array containing size of matrix block along j axis
 * @param iIndexArray array containing i index of matrix block
 * @param jIndexArray array containing j index of matrix block
 * @param icount total number of non-zero blocks along i axis
 * @param jcount total number of non-zero blocks along j axis
 */
void loadForwardBranchArrays(int * iSizeArray, int * jSizeArray,
        int ** iIndexArray, int ** jIndexArray, int * icount,
        int * jcount)
{
  int                      iIndex         = 0;
  int                      jIndex         = 0;
  int                      iSize          = 0;
  int                      jSize          = 0;
  bool                     status         = true;

  *icount = 0;
  *jcount = 0;
  for (int i = 0; i < p_nBranches; i++) {
    status = p_network->getBranch(i)->matrixForwardSize(&iSize, &jSize);
    if (status) {
      p_network->getBranch(i)->getMatVecIndices(&iIndex, &jIndex);
      if (iSize > 0) {
        iSizeArray[*icount]      = iSize;
        *(iIndexArray[*icount])  = iIndex;
        (*icount)++;
      }
      if (jSize > 0) {
        jSizeArray[*jcount]      = jSize;
        *(jIndexArray[*jcount])  = jIndex;
        (*jcount)++;
      }
    }
  }
}

void loadReverseBranchArrays(int * iSizeArray, int * jSizeArray,
        int ** iIndexArray, int ** jIndexArray, int * icount,
        int * jcount)
{
  int                      iIndex         = 0;
  int                      jIndex         = 0;
  int                      iSize          = 0;
  int                      jSize          = 0;
  bool                     status         = true;

  *icount = 0;
  *jcount = 0;
  for (int i = 0; i < p_nBranches; i++) {
    status = p_network->getBranch(i)->matrixReverseSize(&iSize, &jSize);
    if (status) {
      p_network->getBranch(i)->getMatVecIndices(&iIndex, &jIndex);
      if (iSize > 0) {
        iSizeArray[*icount]      = iSize;
        *(iIndexArray[*icount])  = jIndex;
        (*icount)++;
      }
      if (jSize > 0) {
        jSizeArray[*jcount]      = jSize;
        *(jIndexArray[*jcount])  = iIndex;
        (*jcount)++;
      }
    }
  }
}

/**
 *  Clean up index arrays
 *  @param n array size
 *  @param iSizeArray array containing size of matrix block along i axis
 *  @param jSizeArray array containing size of matrix block along j axis
 *  @param iIndexArray array containing i index of matrix block
 *  @param jIndexArray array containing j index of matrix block
 * @param nflag number of indices being used (1 or 2)
 */
void deleteIndexArrays(int n, int * iSizeArray, int * jSizeArray,
        int ** iIndexArray, int ** jIndexArray)
{
  for(int i = 0; i < n; i++) {
    delete iIndexArray[i];
    delete jIndexArray[i];
  }
  delete [] iIndexArray;
  delete [] jIndexArray;

  delete [] iSizeArray;
  delete [] jSizeArray;
}

/**
 * Scatter elements into global arrays
 * @param iSizeArray array containing size of matrix block along i axis
 * @param jSizeArray array containing size of matrix block along j axis
 * @param iIndexArray array containing i index of matrix block
 * @param jIndexArray array containing j index of matrix block
 * @param icount number of i index elements to be scattered
 * @param jcount number of j index elements to be scattered
 */
void scatterIndexingArrays(int * iSizeArray, int * jSizeArray,
                                  int ** iIndexArray, int ** jIndexArray,
                                  int icount, int jcount)
{
  if (icount > 0) NGA_Scatter(gaMatBlksI, iSizeArray, iIndexArray, icount);
  if (jcount > 0) NGA_Scatter(gaMatBlksJ, jSizeArray, jIndexArray, jcount);
}

/**
 * Set up the offset arrays that will be used to find the exact location of
 * each matrix block in the matrix produced by the mapper
 */
void setupOffsetArrays()
{
  int *itmp = new int[p_nNodes];
  int *jtmp = new int[p_nNodes];

  int *rptr;
  int i, idx;
  int one = 1;

  // We need to decompose this matrix by rows so that each processor has a
  // contiguous set of rows. The MatVec indices are set up so that the active
  // buses on each processor are indexed consecutively. This index can be used
  // to set up this partition.
  p_minRowIndex = p_totalBuses;
  p_maxRowIndex = 0;
  for (i=0; i<p_nBuses; i++) {
    if (p_network->getActiveBus(i)) {
      p_network->getBus(i)->getMatVecIndex(&idx);
      if (idx > p_maxRowIndex) p_maxRowIndex = idx;
      if (idx < p_minRowIndex) p_minRowIndex = idx;
    }
  }
//  printf("p[%d] (FullMatrixMap) minRow: %d maxRow: %d\n",p_me,p_minRowIndex,p_maxRowIndex);
  // Create array to hold information about desired rows
  int nRows = p_maxRowIndex-p_minRowIndex+1;
  int *iSizes = new int[nRows]; 
  int *jSizes = new int[nRows]; 
  GA_Sync();
  NGA_Get(gaMatBlksI,&p_minRowIndex,&p_maxRowIndex,iSizes,&one);
  NGA_Get(gaMatBlksJ,&p_minRowIndex,&p_maxRowIndex,jSizes,&one);

  // Calculate total number of elements associated with row block and column
  // block associated with this processor
  int iSize = 0;
  int jSize = 0;
  p_maxIBlock = 0;
  p_maxJBlock = 0;
  for (i=0; i<nRows; i++) {
    if (p_maxIBlock < iSizes[i]) p_maxIBlock = iSizes[i];
    if (p_maxJBlock < jSizes[i]) p_maxJBlock = jSizes[i];
    if (iSizes[i] > 0) iSize += iSizes[i];
    if (jSizes[i] > 0) jSize += jSizes[i];
//    if (iSizes[i] == 0 || jSizes[i] == 0) {
//      printf("p[%d] Sizes[%d] I: %d J: %d\n",p_me,i,iSizes[i],jSizes[i]);
//    }
  }
  p_rowBlockSize = iSize;
  GA_Igop(&p_maxIBlock,one,"max");
  GA_Igop(&p_maxJBlock,one,"max");

  for (i = 0; i<p_nNodes; i++) {
    itmp[i] = 0;
    jtmp[i] = 0;
  }
  itmp[p_me] = iSize;
  jtmp[p_me] = jSize;
//  printf("p[%d] (FullMatrixMap) iSize: %d jSize: %d\n",p_me,iSize,jSize);

  GA_Igop(itmp, p_nNodes, "+");
  GA_Igop(jtmp, p_nNodes, "+");

  int offsetArrayISize = 0;
  int offsetArrayJSize = 0;
  for (i = 0; i < p_me; i++) {
    offsetArrayISize += itmp[i];
    offsetArrayJSize += jtmp[i];
  }

  // Calculate matrix dimension
  p_iDim = 0;
  p_jDim = 0;
  for (i=0; i<p_nNodes; i++) {
    p_iDim += itmp[i];
    p_jDim += jtmp[i];
  }
//  printf("p[%d] (FullMatrixMap) iDim: %d jDim: %d\n",p_me,p_iDim,p_jDim);

  // Create map array so that offset arrays can be created with a specified
  // distribution
  int *offset = new int[p_nNodes];
  for (i=0; i<p_nNodes; i++) {
    offset[i] = 0;
  }
  offset[p_me] = p_activeBuses;
//  printf("p[%d] (FullMatrixMap) activeBuses: %d\n",p_me,p_activeBuses);
  GA_Igop(offset,p_nNodes,"+");

  int *mapc = new int[p_nNodes];
  mapc[0]=0;
  for (i=1; i<p_nNodes; i++) {
    mapc[i] = mapc[i-1] + offset[i-1];
  }

  delete [] offset;
  delete [] itmp;
  delete [] jtmp;

  // Create arrays that will hold final offsets
  gaOffsetI = GA_Create_handle();
  GA_Set_data(gaOffsetI, one, &p_totalBuses, C_INT);
  GA_Set_irreg_distr(gaOffsetI, mapc, &p_nNodes);
  if (!GA_Allocate(gaOffsetI)) {
    // TODO: some kind of error
  }
  GA_Zero(gaOffsetI);

  gaOffsetJ = GA_Create_handle();
  GA_Set_data(gaOffsetJ, one, &p_totalBuses, C_INT);
  GA_Set_irreg_distr(gaOffsetJ, mapc, &p_nNodes);
  if (!GA_Allocate(gaOffsetJ)) {
    // TODO: some kind of error
  }
  GA_Zero(gaOffsetJ);

  // Evaluate offsets for this processor
  int *iOffsets = new int[nRows]; 
  int *jOffsets = new int[nRows]; 
  iOffsets[0] = offsetArrayISize;
  jOffsets[0] = offsetArrayJSize;
  for (i=1; i<nRows; i++) {
    iOffsets[i] = iOffsets[i-1] + iSizes[i-1];
    jOffsets[i] = jOffsets[i-1] + jSizes[i-1];
  }

  // Put offsets into global arrays
  if (nRows > 0) {
    NGA_Put(gaOffsetI,&p_minRowIndex,&p_maxRowIndex,iOffsets,&one);
    NGA_Put(gaOffsetJ,&p_minRowIndex,&p_maxRowIndex,jOffsets,&one);
  }
  GA_Sync();

  // Clean up arrays that are no longer needed
  GA_Destroy(gaMatBlksI);
  GA_Destroy(gaMatBlksJ);

  delete [] mapc;
  delete [] iSizes;
  delete [] jSizes;
  delete [] iOffsets;
  delete [] jOffsets;
}

/**
 * Add diagonal block contributions from buses to matrix
 * @param matrix matrix to which contributions are added
 * @param flag flag to distinguish new matrix (true) from old (false)
 */
void loadBusData(gridpack::math::Matrix &matrix, bool flag)
{
  int i,idx,jdx,isize,jsize,icnt;
  int **indices = new int*[p_busContribution];
  icnt = 0;
  for (i=0; i<p_nBuses; i++) {
    if (p_network->getActiveBus(i)) {
      if (p_network->getBus(i)->matrixDiagSize(&isize,&jsize)) {
        indices[icnt] = new int;
        p_network->getBus(i)->getMatVecIndex(&idx);
        *(indices[icnt]) = idx;
        icnt++;
      }
    }
  }
  if (icnt != p_busContribution) {
    // TODO: some kind of error
  }

  // Gather matrix offsets
  int *i_offsets;
  int *j_offsets;
  if (p_busContribution > 0) {
    i_offsets = new int[p_busContribution];
    j_offsets = new int[p_busContribution];
    NGA_Gather(gaOffsetI,i_offsets,indices,p_busContribution);
    NGA_Gather(gaOffsetJ,j_offsets,indices,p_busContribution);
  }

  // Add matrix elements
  ComplexType *values = new ComplexType[p_maxIBlock*p_maxJBlock];
  int j,k;
  int jcnt = 0;
  for (i=0; i<p_nBuses; i++) {
    if (p_network->getActiveBus(i)) {
      if (p_network->getBus(i)->matrixDiagSize(&isize,&jsize)) {
#ifdef DBG_CHECK
        int ijsize = isize*jsize;
        for (k=0; k<ijsize; k++) values[k] = 0.0;
#endif
        if (p_network->getBus(i)->matrixDiagValues(values)) {
          icnt = 0;
          for (k=0; k<jsize; k++) {
            jdx = j_offsets[jcnt] + k;
            for (j=0; j<isize; j++) {
              idx = i_offsets[jcnt] + j;
//              if (flag) {
//                printf("p[%d] Adding value [%d,%d]: %f+i%f\n",GA_Nodeid(),idx,jdx,real(values[icnt]),imag(values[icnt]));
                matrix.addElement(idx, jdx, values[icnt]);
//              } else {
//                matrix.setElement(idx, jdx, values[icnt]);
//              }
              icnt++;
            }
          }
        }
        delete indices[jcnt];
        jcnt++;
      }
    }
  }

  // Clean up arrays
  delete [] indices;
  if (p_busContribution > 0) {
    delete [] i_offsets;
    delete [] j_offsets;
  }
  delete [] values;
}

/**
 * Add diagonal block contributions from buses to matrix
 * @param matrix matrix to which contributions are added
 * @param flag flag to distinguish new matrix (true) from old (false)
 */
void loadBusData(boost::shared_ptr<gridpack::math::Matrix> &matrix, bool flag)
{
  loadBusData(*matrix, flag);
}

/**
 * Add off-diagonal block contributions from branches to matrix
 * @param matrix matrix to which contributions are added
 * @param flag flag to distinguish new matrix (true) from old (false)
 */
void loadBranchData(gridpack::math::Matrix &matrix, bool flag)
{
  int i,idx,jdx,isize,jsize,icnt;
  int **i_indices = new int*[p_branchContribution];
  int **j_indices = new int*[p_branchContribution];
  icnt = 0;
  for (i=0; i<p_nBranches; i++) {
    if (p_network->getBranch(i)->matrixForwardSize(&isize,&jsize)) {
      p_network->getBranch(i)->getMatVecIndices(&idx, &jdx);
      if (idx >= p_minRowIndex && idx <= p_maxRowIndex) {
        i_indices[icnt] = new int;
        j_indices[icnt] = new int;
        *(i_indices[icnt]) = idx;
        *(j_indices[icnt]) = jdx;
        icnt++;
      } else {
        // TODO: some kind of error
      }
    }
    if (p_network->getBranch(i)->matrixReverseSize(&isize,&jsize)) {
      p_network->getBranch(i)->getMatVecIndices(&idx, &jdx);
      if (jdx >= p_minRowIndex && jdx <= p_maxRowIndex) {
        i_indices[icnt] = new int;
        j_indices[icnt] = new int;
        *(i_indices[icnt]) = jdx;
        *(j_indices[icnt]) = idx;
        icnt++;
      }
    }
  }
  if (icnt != p_branchContribution) {
    printf("p[%d] Mismatch in loadBranchData icnt: %d branchContribution: %d\n",
        p_me,icnt,p_branchContribution);
    // TODO: some kind of error
  }

  // Gather matrix offsets
  int *i_offsets = new int[p_branchContribution];
  int *j_offsets = new int[p_branchContribution];
  if (p_branchContribution > 0) {
    NGA_Gather(gaOffsetI,i_offsets,i_indices,p_branchContribution);
    NGA_Gather(gaOffsetJ,j_offsets,j_indices,p_branchContribution);
  }

  // Add matrix elements
  ComplexType *values = new ComplexType[p_maxIBlock*p_maxJBlock];
  int j,k;
  int jcnt = 0;
  for (i=0; i<p_nBranches; i++) {
    if (p_network->getBranch(i)->matrixForwardSize(&isize,&jsize)) {
      p_network->getBranch(i)->getMatVecIndices(&idx, &jdx);
      if (idx >= p_minRowIndex && idx <= p_maxRowIndex) {
#ifdef DBG_CHECK
        int ijsize = isize*jsize;
        for (k=0; k<ijsize; k++) values[k] = 0.0;
#endif
        if (p_network->getBranch(i)->matrixForwardValues(values)) {
          icnt = 0;
          for (k=0; k<jsize; k++) {
            jdx = j_offsets[jcnt] + k;
            for (j=0; j<isize; j++) {
              idx = i_offsets[jcnt] + j;
//              if (flag) {
//                printf("p[%d] Adding value [%d,%d]: %f+i%f\n",GA_Nodeid(),idx,jdx,real(values[icnt]),imag(values[icnt]));
                matrix.addElement(idx, jdx, values[icnt]);
//              } else {
//                matrix.setElement(idx, jdx, values[icnt]);
//              }
              icnt++;
            }
          }
        }
        delete i_indices[jcnt];
        delete j_indices[jcnt];
        jcnt++;
      }
    }
    if (p_network->getBranch(i)->matrixReverseSize(&isize,&jsize)) {
      p_network->getBranch(i)->getMatVecIndices(&idx, &jdx);
      if (jdx >= p_minRowIndex && jdx <= p_maxRowIndex) {
#ifdef DBG_CHECK
        int ijsize = isize*jsize;
        for (k=0; k<ijsize; k++) values[k] = 0.0;
#endif
        if (p_network->getBranch(i)->matrixReverseValues(values)) {
          icnt = 0;
          // Note that because the indices have been reversed, we need to switch
          // the ordering of the offsets as well
          for (k=0; k<jsize; k++) {
            idx = j_offsets[jcnt] + k;
            for (j=0; j<isize; j++) {
              jdx = i_offsets[jcnt] + j;
//              if (flag) {
//                printf("p[%d] Adding value [%d,%d]: %f+i%f\n",GA_Nodeid(),idx,jdx,real(values[icnt]),imag(values[icnt]));
                matrix.addElement(jdx, idx, values[icnt]);
//              } else {
//                matrix.setElement(jdx, idx, values[icnt]);
//              }
              icnt++;
            }
          }
        }
        delete i_indices[jcnt];
        delete j_indices[jcnt];
        jcnt++;
      }
    }
  }

  // Clean up arrays
  delete [] i_indices;
  delete [] j_indices;
  delete [] i_offsets;
  delete [] j_offsets;
  delete [] values;
}

/**
 * Add off-diagonal block contributions from branches to matrix
 * @param matrix matrix to which contributions are added
 * @param flag flag to distinguish new matrix (true) from old (false)
 */
void loadBranchData(boost::shared_ptr<gridpack::math::Matrix> &matrix, bool flag)
{
  loadBranchData(*matrix, flag);
}

/**
 * Calculate how many buses and branches contribute to matrix
 */
void contributions(void)
{
  int i;
  // Get number of contributions from buses
  int isize, jsize;
  p_busContribution = 0;
  for (i=0; i<p_nBuses; i++) {
    if (p_network->getActiveBus(i)) {
      if (p_network->getBus(i)->matrixDiagSize(&isize, &jsize)) p_busContribution++;
    }
  }

  // Get number of contributions from branches
  int idx, jdx;
  p_branchContribution = 0;
  for (i=0; i<p_nBranches; i++) {
    if (p_network->getBranch(i)->matrixForwardSize(&isize, &jsize)) {
      p_network->getBranch(i)->getMatVecIndices(&idx, &jdx);
      if (idx >= p_minRowIndex && idx <= p_maxRowIndex) {
        p_branchContribution++;
      } else {
        // TODO: some kind of error
      }
    }
    if (p_network->getBranch(i)->matrixReverseSize(&isize, &jsize)) {
      p_network->getBranch(i)->getMatVecIndices(&idx, &jdx);
      if (jdx >= p_minRowIndex && jdx <= p_maxRowIndex) {
        p_branchContribution++;
      }
    }
  }
}

    // GA information
int                         p_me;
int                         p_nNodes;

    // network information
boost::shared_ptr<_network> p_network;
int                         p_nBuses;
int                         p_nBranches;
int                         p_totalBuses;
int                         p_activeBuses;

    // matrix information
int                         p_iDim;
int                         p_jDim;
int                         p_minRowIndex;
int                         p_maxRowIndex;
int                         p_rowBlockSize;
int                         p_minColIndex;
int                         p_maxColIndex;
int                         p_busContribution;
int                         p_branchContribution;
int                         p_maxIBlock;
int                         p_maxJBlock;
int                         p_maxrow;

    // global matrix block size array
int                         gaMatBlksI; // g_idx
int                         gaMatBlksJ; // g_jdx
int                         gaOffsetI; // g_ioff
int                         gaOffsetJ; // g_joff

};

} /* namespace mapper */
} /* namespace gridpack */

#endif //FULLMATRIXMAP_HPP_
