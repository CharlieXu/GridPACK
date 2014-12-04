// -------------------------------------------------------------
/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   matrix.cpp
 * @author William A. Perkins
 * @date   2014-09-12 13:42:40 d3g096
 * 
 * @brief  PETSc specific part of Matrix
 * 
 * 
 */
// -------------------------------------------------------------

#include "boost/assert.hpp"
#include "boost/format.hpp"
#include "matrix.hpp"
#include "petsc/petsc_exception.hpp"
#include "petsc/petsc_matrix_implementation.hpp"
#include "petsc/petsc_matrix_extractor.hpp"
#include "petsc/petsc_vector_extractor.hpp"


namespace gridpack {
namespace math {

// -------------------------------------------------------------
//  class Matrix
// -------------------------------------------------------------

// -------------------------------------------------------------
// Matrix:: constructors / destructor
// -------------------------------------------------------------
Matrix::Matrix(const parallel::Communicator& comm,
               const int& local_rows,
               const int& cols,
               const StorageType& storage_type)
  : parallel::WrappedDistributed(), utility::Uncopyable(),
    p_matrix_impl()
{
  switch (storage_type) {
  case Sparse:
    p_matrix_impl.reset(new PETScMatrixImplementation(comm,
                                                      local_rows, cols, false));
    break;
  case Dense:
    p_matrix_impl.reset(new PETScMatrixImplementation(comm,
                                                      local_rows, cols, true));
    break;
  default:
    BOOST_ASSERT(false);
  }
  BOOST_ASSERT(p_matrix_impl);
  p_setDistributed(p_matrix_impl.get());
}


Matrix::Matrix(const parallel::Communicator& comm,
               const int& local_rows,
               const int& cols,
               const int& max_nz_per_row)
  : parallel::WrappedDistributed(), utility::Uncopyable(),
    p_matrix_impl()
{
  p_matrix_impl.reset(new PETScMatrixImplementation(comm,
                                                    local_rows, cols, 
                                                    max_nz_per_row));
  BOOST_ASSERT(p_matrix_impl);
  p_setDistributed(p_matrix_impl.get());
}

Matrix::Matrix(const parallel::Communicator& comm,
               const int& local_rows,
               const int& cols,
               const int *nz_by_row)
  : parallel::WrappedDistributed(), utility::Uncopyable(),
    p_matrix_impl()
{
  p_matrix_impl.reset(new PETScMatrixImplementation(comm,
                                                    local_rows, cols, 
                                                    nz_by_row));
  BOOST_ASSERT(p_matrix_impl);
  p_setDistributed(p_matrix_impl.get());
}



// -------------------------------------------------------------
// Matrix::equate
// -------------------------------------------------------------
void
Matrix::equate(const Matrix& B)
{
  this->p_check_compatible(B);
  Mat *pA(PETScMatrix(*this));
  const Mat *pB(PETScMatrix(B));

  PetscErrorCode ierr(0);
  try {
    PetscScalar one(1.0);
    ierr = MatCopy(*pB, *pA, DIFFERENT_NONZERO_PATTERN); CHKERRXX(ierr);
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}

// -------------------------------------------------------------
// Matrix::scale
// -------------------------------------------------------------
void
Matrix::scale(const ComplexType& xin)
{
  Mat *pA(PETScMatrix(*this));

  PetscErrorCode ierr(0);
  
  try {
    PetscScalar x(xin);
    ierr = MatScale(*pA, x); CHKERRXX(ierr);
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}
    

// -------------------------------------------------------------
// Matrix::add
// -------------------------------------------------------------
void
Matrix::add(const Matrix& B)
{
  this->p_check_compatible(B);
  Mat *pA(PETScMatrix(*this));
  const Mat *pB(PETScMatrix(B));

  PetscErrorCode ierr(0);
  try {
    PetscScalar one(1.0);
    ierr = MatAXPY(*pA, one, *pB, DIFFERENT_NONZERO_PATTERN); CHKERRXX(ierr);
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}

// -------------------------------------------------------------
// Matrix::addDiagonal
// -------------------------------------------------------------
void
Matrix::addDiagonal(const Vector& x)
{
  const Vec *pX(PETScVector(x));
  Mat *pA(PETScMatrix(*this));
  PetscErrorCode ierr(0);
  try {
    ierr = MatDiagonalSet(*pA, *pX, ADD_VALUES); CHKERRXX(ierr);
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}

// -------------------------------------------------------------
// Matrix::identity
// -------------------------------------------------------------
void
Matrix::identity(void)
{
  Mat *pA(PETScMatrix(*this));

  PetscErrorCode ierr(0);
  try {
    PetscBool flag;
    PetscScalar one(1.0);
    ierr = MatAssembled(*pA, &flag); CHKERRXX(ierr);
    if (!flag) {
      int lo, hi;
      this->localRowRange(lo, hi);
      for (int i = lo; i < hi; ++i) {
        this->setElement(i, i, 1.0);
      }
      this->ready();
    } else {
      ierr = MatZeroEntries(*pA); CHKERRXX(ierr);
      ierr = MatShift(*pA, one); CHKERRXX(ierr);
    }
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}

// -------------------------------------------------------------
// Matrix::zero
// -------------------------------------------------------------
void
Matrix::zero(void)
{
  Mat *pA(PETScMatrix(*this));

  PetscErrorCode ierr(0);
  try {
    ierr = MatZeroEntries(*pA); CHKERRXX(ierr);
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}

// -------------------------------------------------------------
// Matrix::multiplyDiagonal
// -------------------------------------------------------------
void
Matrix::multiplyDiagonal(const Vector& x)
{
  const Vec *pscale(PETScVector(x));
  Mat *pA(PETScMatrix(*this));
  PetscErrorCode ierr(0);
  try {
    Vec diagorig, diagnew;
    ierr = VecDuplicate(*pscale, &diagorig);  CHKERRXX(ierr);
    ierr = VecDuplicate(*pscale, &diagnew);  CHKERRXX(ierr);
    ierr = MatGetDiagonal(*pA, diagorig); CHKERRXX(ierr);
    ierr = VecPointwiseMult(diagnew, diagorig, *pscale); CHKERRXX(ierr);
    ierr = MatDiagonalSet(*pA, diagnew, INSERT_VALUES); CHKERRXX(ierr);
    ierr = VecDestroy(&diagorig); CHKERRXX(ierr); 
    ierr = VecDestroy(&diagnew); CHKERRXX(ierr); 
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}

// -------------------------------------------------------------
// petsc_print_matrix
// -------------------------------------------------------------
static void
petsc_print_matrix(const Mat mat, const char* filename, PetscViewerFormat format)
{
  PetscErrorCode ierr;
  try {
    PetscViewer viewer;
    MPI_Comm comm = PetscObjectComm((PetscObject)mat);
    int me, nproc;
    ierr = MPI_Comm_rank(comm, &me);
    ierr = MPI_Comm_size(comm, &nproc);
    if (filename != NULL) {
      ierr = PetscViewerASCIIOpen(comm, filename, &viewer); CHKERRXX(ierr);
    } else {
      ierr = PetscViewerASCIIGetStdout(comm, &viewer); CHKERRXX(ierr);
    }
    ierr = PetscViewerSetFormat(viewer, format);
    PetscInt grow, gcol, lrow, lcol;
    switch (format) {
    case PETSC_VIEWER_DEFAULT:
      ierr = MatGetSize(mat, &grow, &gcol); CHKERRXX(ierr);
      ierr = MatGetLocalSize(mat, &lrow, &lcol); CHKERRXX(ierr);
      ierr = PetscViewerASCIISynchronizedAllow(viewer, PETSC_TRUE); CHKERRXX(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,             
                                    "# Matrix distribution\n"); CHKERRXX(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,             
                                    "# proc   rows     cols\n");  CHKERRXX(ierr);
      ierr = PetscViewerASCIIPrintf(viewer,             
                                    "# ---- -------- --------\n"); CHKERRXX(ierr);
      ierr = PetscViewerASCIISynchronizedPrintf(viewer, "# %4d %8d %8d\n",
                                                me, lrow, lcol);  CHKERRXX(ierr);
      ierr = PetscViewerFlush(viewer); CHKERRXX(ierr);
      ierr = MPI_Barrier(comm);
      ierr = PetscViewerASCIIPrintf(viewer,             
                                    "# ---- -------- --------\n");  CHKERRXX(ierr);
      ierr = PetscViewerASCIIPrintf(viewer, "# %4d %8d %8d\n", 
                                    nproc, grow, gcol);  CHKERRXX(ierr);
    }
    ierr = MatView(mat, viewer); CHKERRXX(ierr);
    if (filename != NULL) {
      ierr = PetscViewerDestroy(&viewer); CHKERRXX(ierr);
    } else {
      // FIXME: causes a SEGV?
      // ierr = PetscViewerDestroy(&viewer); CHKERRXX(ierr);
    }
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}


// -------------------------------------------------------------
// Matrix::print
// -------------------------------------------------------------
void
Matrix::print(const char* filename) const
{
  const Mat *mat(PETScMatrix(*this));
  
  petsc_print_matrix(*mat, filename, PETSC_VIEWER_DEFAULT);
}

// -------------------------------------------------------------
// Matrix::save
// -------------------------------------------------------------
void
Matrix::save(const char* filename) const
{
  const Mat *mat(PETScMatrix(*this));
  petsc_print_matrix(*mat, filename, PETSC_VIEWER_ASCII_MATLAB);
}

// -------------------------------------------------------------
// Vector::loadBinary
// -------------------------------------------------------------
void
Matrix::loadBinary(const char* filename)
{
  PetscErrorCode ierr;
  Mat *mat(PETScMatrix(*this));
  try {
    PetscViewer viewer;
    ierr = PetscViewerBinaryOpen(this->communicator(),
                                 filename,
                                 FILE_MODE_READ,
                                 &viewer); CHKERRXX(ierr);
    ierr = PetscViewerSetFormat(viewer, PETSC_VIEWER_NATIVE); CHKERRXX(ierr);
    ierr = MatLoad(*mat, viewer); CHKERRXX(ierr);
    ierr = PetscViewerDestroy(&viewer); CHKERRXX(ierr);
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}

// -------------------------------------------------------------
// Matrix::saveBinary
// -------------------------------------------------------------
void
Matrix::saveBinary(const char* filename) const
{
  PetscErrorCode ierr;
  const Mat *mat(PETScMatrix(*this));
  try {
    PetscViewer viewer;
    ierr = PetscViewerBinaryOpen(this->communicator(),
                                 filename,
                                 FILE_MODE_WRITE,
                                 &viewer); CHKERRXX(ierr);
    ierr = PetscViewerSetFormat(viewer, PETSC_VIEWER_NATIVE); CHKERRXX(ierr);
    ierr = MatView(*mat, viewer); CHKERRXX(ierr);
    ierr = PetscViewerDestroy(&viewer); CHKERRXX(ierr);
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  }
}


// -------------------------------------------------------------
// Matrix::storageType
// -------------------------------------------------------------
Matrix::StorageType
Matrix::storageType(void) const
{
  StorageType result;
  PetscErrorCode ierr;
  const Mat *mat(PETScMatrix(*this));
  try {
    MatType thetype;
    ierr = MatGetType(*mat, &thetype); CHKERRXX(ierr);

    // this relies on the fact that MatType is actual a pointer to a
    // char string -- need to test string contents, not pointers
    std::string stype(thetype);

    if (stype == MATSEQDENSE ||
        stype == MATDENSE ||
        stype == MATMPIDENSE) {
      result = Dense;
    } else if (stype == MATSEQAIJ || 
               stype == MATMPIAIJ) {
      result = Sparse;
    } else {
      std::string msg("Matrix: unexpected PETSc storage type: ");
      msg += "\"";
      msg += stype;
      msg += "\"";
      throw Exception(msg);
    }
  } catch (const PETSC_EXCEPTION_TYPE& e) {
    throw PETScException(ierr, e);
  } catch (const Exception& e) {
    throw;
  }
  return result;
}


} // namespace math
} // namespace gridpack
