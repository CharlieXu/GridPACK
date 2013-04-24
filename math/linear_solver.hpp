// Emacs Mode Line: -*- Mode:c++;-*-
// -------------------------------------------------------------
/**
 * @file   linear_solver.hpp
 * @author William A. Perkins
 * @date   Mon Apr  1 07:35:48 2013
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

// SCCS ID: $Id$ Battelle PNL

#ifndef _linear_solver_hpp_
#define _linear_solver_hpp_

#include <boost/scoped_ptr.hpp>
#include "gridpack/utility/noncopyable.hpp"
#include "gridpack/math/linear_solver_implementation.hpp"

namespace gridpack {
namespace math {

class ImplementationVisitor;

// -------------------------------------------------------------
//  class LinearSolver
// -------------------------------------------------------------
class LinearSolver 
  : public parallel::Distributed,
    public utility::Configurable,
    private utility::UnCopyable
{
public:
  
  /// Default constructor.
  LinearSolver(const parallel::Distribution& dist, 
               const Matrix& A);
  
  /// Destructor
  ~LinearSolver(void);

  /// Solve w/ the specified RHS, put result in specified vector
  void solve(const Vector& b, Vector& x) const
  {
    solver_->solve(b, x);
  }

  /// Allow visits by implemetation visitor
  void accept(ImplementationVisitor& visitor)
  {
    solver_->accept(visitor);
  }

protected:

  boost::scoped_ptr<LinearSolverImplementation> solver_;

};

} // namespace math
} // namespace gridpack

#endif
