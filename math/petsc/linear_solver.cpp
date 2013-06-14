// -------------------------------------------------------------
/**
 * @file   linear_solver.cpp
 * @author William A. Perkins
 * @date   2013-06-14 12:08:17 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------
// -------------------------------------------------------------
// Created June 14, 2013 by William A. Perkins
// Last Change: 2013-05-03 12:23:12 d3g096
// -------------------------------------------------------------

#include "linear_solver.hpp"
#include "petsc/petsc_linear_solver_implementation.hpp"

namespace gridpack {
namespace math {


// -------------------------------------------------------------
//  class LinearSolver
// -------------------------------------------------------------

// -------------------------------------------------------------
// LinearSolver:: constructors / destructor
// -------------------------------------------------------------
LinearSolver::LinearSolver(const Matrix& A)
  : parallel::Distributed(A.communicator()),
    utility::Uncopyable(),
    p_solver(new PETScLinearSolverImplementation(A))
{
  // empty
}


} // namespace math
} // namespace gridpack
