/**
 * @file   newton_raphson_solver_implementation.cpp
 * @author William A. Perkins
 * @date   2013-09-09 13:10:59 d3g096
 * 
 * @brief  
 * 
 * 
 */

#include <iostream>
#include "newton_raphson_solver_implementation.hpp"

namespace gridpack {
namespace math {

// -------------------------------------------------------------
//  class NewtonRaphsonSolverImplementation
// -------------------------------------------------------------

// -------------------------------------------------------------
// NewtonRaphsonSolverImplementation:: constructors / destructor
// -------------------------------------------------------------
NewtonRaphsonSolverImplementation::NewtonRaphsonSolverImplementation(const parallel::Communicator& comm,
                                                                     const int& local_size,
                                                                     JacobianBuilder form_jacobian,
                                                                     FunctionBuilder form_function)
  : NonlinearSolverImplementation(comm, local_size, form_jacobian, form_function),
  p_tolerance(1.0e-06),
  p_max_iterations(100),
  p_linear_solver()
{
  
}

NewtonRaphsonSolverImplementation::~NewtonRaphsonSolverImplementation(void)
{
}

// -------------------------------------------------------------
// NewtonRaphsonSolverImplementation::p_solve
// -------------------------------------------------------------
void
NewtonRaphsonSolverImplementation::p_solve(void)
{
  ComplexType tol(1.0e+30);
  int iter(0);
  
  std::cout << "Entered NewtonRaphsonSolverImplementation::p_solve()"
            << std::endl;
  boost::scoped_ptr<Vector> deltaX(p_X->clone());
  deltaX->zero();
  while (real(tol) > p_tolerance && iter <= p_max_iterations) {
    p_X->print();
    p_function(*p_X, *p_F);
    p_F->scale(-1.0);
    p_jacobian(*p_X, *p_J);
    if (!p_linear_solver) {
      p_linear_solver.reset(new LinearSolver(*p_J));
    } else {
      p_linear_solver->set_matrix(*p_J);
    }
    deltaX->zero();
    p_linear_solver->solve(*p_F, *deltaX);
    tol = deltaX->norm2();
    p_X->add(*deltaX);
    p_X->print();
    iter += 1;
    std::cout << "NewtonRaphsonSolverImplementation::p_solve: "
              << "iteration " << iter << ": "
              << tol << std::endl;
  }
  std::cout << "Leaving NewtonRaphsonSolverImplementation::p_solve()"
            << std::endl;
}


} // namespace math
} // namespace gridpack
