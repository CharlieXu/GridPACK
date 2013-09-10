// -------------------------------------------------------------
/**
 * @file   matrix.cpp
 * @author William A. Perkins
 * @date   2013-09-10 11:42:28 d3g096
 * 
 * @brief  Generic part of Matrix implementation
 * 
 * 
 */
// -------------------------------------------------------------

#include <boost/assert.hpp>
#include "matrix.hpp"
#include "gridpack/utilities/exception.hpp"

namespace gridpack {
namespace math {

// -------------------------------------------------------------
// Matrix constructor
// -------------------------------------------------------------
Matrix::Matrix(MatrixImplementation *impl)
  : parallel::WrappedDistributed(impl), 
    utility::Uncopyable(),
    p_matrix_impl(impl)
{
  BOOST_ASSERT(p_matrix_impl);
}

Matrix::~Matrix(void)
{
  
}

// -------------------------------------------------------------
// Matrix::p_check_compatible
// -------------------------------------------------------------
void
Matrix::p_check_compatible(const Matrix& A) const
{
  if (this->communicator() != A.communicator()) {
    throw gridpack::Exception("incompatible: communicators do not match");
  }

  if ((this->rows() != A.rows()) || (this->cols() != A.cols())) {
    throw gridpack::Exception("incompatible: sizes do not match");
  }
  return;
}

// -------------------------------------------------------------
// Matrix operations
// -------------------------------------------------------------

// -------------------------------------------------------------
// add
// -------------------------------------------------------------
void
add(const Matrix& A, const Matrix& B, Matrix& result) 
{
  result.equate(A);
  result.add(B);
}


Matrix *
add(const Matrix& A, const Matrix& B) 
{
  Matrix *result = A.clone();
  add(A, B, *result);
  return result;
}


// -------------------------------------------------------------
// transpose
// -------------------------------------------------------------
Matrix *
transpose(const Matrix& A)
{
  Matrix *result = A.clone();
  transpose(A, *result);
  return result;
}

// -------------------------------------------------------------
// column
// -------------------------------------------------------------
Vector *
column(const Matrix& A, const int& cidx)
{
  Vector *colv(new Vector(A.communicator(), A.local_rows()));
  column(A, cidx, *colv);
  return colv;
}

// -------------------------------------------------------------
// diagonal
// -------------------------------------------------------------
Vector *
diagonal(const Matrix& A)
{
  Vector *colv(new Vector(A.communicator(), A.local_rows()));
  diagonal(A, *colv);
  return colv;
}

// -------------------------------------------------------------
// multiply
// -------------------------------------------------------------
Vector *
multiply(const Matrix& A, const Vector& x)
{
  Vector *result(new Vector(x.communicator(), x.local_size()));
  multiply(A, x, *result);
  return result;
}


} // namespace math
} // namespace gridpack
