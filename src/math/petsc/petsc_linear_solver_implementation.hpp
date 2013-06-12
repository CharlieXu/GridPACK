// Emacs Mode Line: -*- Mode:c++;-*-
// -------------------------------------------------------------
/**
 * @file   petsc_linear_solver_implementation.hpp
 * @author William A. Perkins
 * @date   2013-06-11 13:58:09 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------
// -------------------------------------------------------------
// Created April  1, 2013 by William A. Perkins
// Last Change: Thu Jun  3 06:45:08 2010 by William A. Perkins <d3g096@PE10900.pnl.gov>
// -------------------------------------------------------------

#ifndef _petsc_linear_solver_implementation_hpp_
#define _petsc_linear_solver_implementation_hpp_

#include <petscksp.h>
#include "linear_solver_implementation.hpp"

namespace gridpack {
namespace math {

// -------------------------------------------------------------
//  class PETScLinearSolverImplementation
// -------------------------------------------------------------
class PETScLinearSolverImplementation 
  : public LinearSolverImplementation {
public:

  /// Default constructor.
  PETScLinearSolverImplementation(const Matrix& A);

  /// Destructor
  ~PETScLinearSolverImplementation(void);

protected:

  /// The PETSc linear solver 
  KSP p_KSP;

  /// Solve w/ the specified RHS and estimate (result in x)
  void p_solve(const Vector& b, Vector& x) const;

  /// Allow visits by implementation visitors
  void p_accept(ImplementationVisitor& visitor);

  /// Allow visits by implementation visitors
  void p_accept(ConstImplementationVisitor& visitor) const;

};

} // namespace math
} // namespace gridpack

#endif
