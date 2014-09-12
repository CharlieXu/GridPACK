// -------------------------------------------------------------
/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   petsc_nonlinear_solver_implementation.hpp
 * @author William A. Perkins
 * @date   2014-09-12 14:07:23 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------

#include <petscsnes.h>
#include "nonlinear_solver_implementation.hpp"
#include "petsc_configurable.hpp"

namespace gridpack {
namespace math {

// -------------------------------------------------------------
//  class PetscNonlinearSolverImplementation
// -------------------------------------------------------------
class PetscNonlinearSolverImplementation 
  : public NonlinearSolverImplementation,
    private PETScConfigurable
{
public:

  /// Default constructor.
  PetscNonlinearSolverImplementation(const parallel::Communicator& comm,
                                     const int& local_size,
                                     JacobianBuilder form_jacobian,
                                     FunctionBuilder form_function);

  /// Construct with an existing Jacobian Matrix
  PetscNonlinearSolverImplementation(Matrix& J,
                                     JacobianBuilder form_jacobian,
                                     FunctionBuilder form_function);

  /// Destructor
  ~PetscNonlinearSolverImplementation(void);

protected:

  /// The PETSc nonlinear solver instance
  SNES p_snes;

  /// A pointer to PETSc matrix part of ::p_J
  Mat *p_petsc_J;

  /// A pointer to the PETSc vector part of ::p_F
  Vec *p_petsc_F;

  /// A pointer to the PETSc vector part of ::p_X
  Vec *p_petsc_X;

  /// Do what is necessary to build this instance
  void p_build(const std::string& option_prefix);

  /// Solve w/ using the specified initial guess (specialized)
  void p_solve(void);

  /// Specialized way to configure from property tree
  void p_configure(utility::Configuration::CursorPtr props);


#if PETSC_VERSION_LT(3,5,0)
  /// Routine to assemble Jacobian that is sent to PETSc
  static PetscErrorCode FormJacobian(SNES snes, Vec x, Mat *jac, Mat *B, 
                                     MatStructure *flag, void *dummy);
#else
  /// Routine to assemble Jacobian that is sent to PETSc
  static PetscErrorCode FormJacobian(SNES snes, Vec x, Mat jac, Mat B, 
                                     void *dummy);
#endif

  /// Routine to assemble RHS that is sent to PETSc
  static PetscErrorCode FormFunction(SNES snes, Vec x, Vec f, void *dummy);
};



} // namespace math
} // namespace gridpack
