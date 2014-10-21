// Emacs Mode Line: -*- Mode:c++;-*-
// -------------------------------------------------------------
/*
 *     Copyright (c) 2013 Battelle Memorial Institute
 *     Licensed under modified BSD License. A copy of this license can be found
 *     in the LICENSE file in the top level directory of this distribution.
 */
// -------------------------------------------------------------
/**
 * @file   matrix.hpp
 * @author William A. Perkins
 * @date   2014-10-21 13:37:57 d3g096
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------
// -------------------------------------------------------------
// Created March 22, 2013 by William A. Perkins
// Last Change: Thu Jun  3 06:45:08 2010 by William A. Perkins <d3g096@PE10900.pnl.gov>
// -------------------------------------------------------------

#ifndef _matrix_hpp_
#define _matrix_hpp_

#include <boost/scoped_ptr.hpp>
#include <gridpack/parallel/distributed.hpp>
#include <gridpack/utilities/uncopyable.hpp>
#include <gridpack/math/matrix_implementation.hpp>
#include <gridpack/math/vector.hpp>


namespace gridpack {
namespace math {

// -------------------------------------------------------------
//  class Matrix
// -------------------------------------------------------------
/// A parallel or serial matrix of real values
/**
 * This class uses the Pimpl idiom for implementation in order so the
 * interface is completely free of the underlying library.  If
 * constructed with a parallel environment with only one process, a
 * serial storage scheme is created, otherwise it's parallel. 
 *
 */
class Matrix 
  : public parallel::WrappedDistributed,
    private utility::Uncopyable,
    public BaseMatrixInterface<ComplexType>
{
public:

  /// The types of matrices that can be created
  /**
   * The gridpack::math library provides two storage schemes for
   * matrices. This is used by Matrix and MatrixImplementation
   * subclasses.
   *
   * The actual storage scheme and memory used is dependent upon the
   * underlying math library implementation.
   * 
   */
  enum StorageType { 
    Dense,                      /**< dense matrix storage scheme */
    Sparse                      /**< sparse matrix storage scheme */
  };

  /// Constructor.
  /** 
   * A Matrix must be instantiated simulutaneously on all processes
   * involved in the specified \ref parallel::Communicator
   * "communicator". Each process in the communicator will own the
   * number of rows requested.
   * 
   * @param dist parallel environment
   * @param local_rows matrix rows to be owned by the local process
   * @param local_cols matrix columns to be owned by the local process
   * @param storage_type specify dense or sparse storage 
   * 
   * @return 
   */
  Matrix(const parallel::Communicator& dist,
         const int& local_rows,
         const int& local_cols,
         const StorageType& storage_type = Sparse);

  /// Sparse matrix constructor with maximum number of nonzeros in a row
  /** 
   * If the underlying math implementation supports it, this
   * constructs a sparse matrix and pre-allocates it to allow @c
   * max_nz_per_row nonzeros in each row. If the underlying math
   * library supports it, @c max_nz_per_row does not need to be the
   * same on all processors.
   * 
   * @param dist parallel environment
   * @param local_rows matrix rows to be owned by the local process
   * @param local_cols matrix columns to be owned by the local process
   * @param max_nz_per_row maximum number of nonzeros in a row
   * 
   * @return new Matrix
   */
  Matrix(const parallel::Communicator& dist,
         const int& local_rows,
         const int& local_cols,
         const int& max_nz_per_row);
  
  /// Sparse matrix constructor with number of nonzeros for each row
  /** 
   * 
   * 
   * @param dist parallel environment
   * @param local_rows matrix rows to be owned by the local process
   * @param local_cols matrix columns to be owned by the local process
   * @param nz_by_row 
   * 
   * @return 
   */
  Matrix(const parallel::Communicator& dist,
         const int& local_rows,
         const int& local_cols,
         const int *nz_by_row);

  /// Construct with an existing (allocated) implementation 
  /** 
   * For internal use only.
   * 
   * @param impl 
   * 
   * @return 
   */
  Matrix(MatrixImplementation *impl);

  /// Destructor
  /** 
   * A matrix must be destroyed simulutaneously on all processes
   * involved in the \ref parallel::Communicator "communicator" used
   * to instantiate it.
   */
  ~Matrix(void);

  /// Get the storage type of this matrix
  StorageType storageType(void) const;

  /// Make this matrix the identity matrix
  /** 
   * @e Collective
   *
   * 
   * 
   */
  void identity(void);

  //! @cond DEVDOC

  /// Allow visits by implemetation visitor
  void accept(ImplementationVisitor& visitor)
  {
    p_matrix_impl->accept(visitor);
  }

  /// Allow visits by implemetation visitor
  void accept(ConstImplementationVisitor& visitor) const
  {
    p_matrix_impl->accept(visitor);
  }

  //! @endcond

  /// Make an exact replica of this instance
  /** 
   * @e Collective.
   *
   * 
   * 
   * 
   * @return pointer to new (allocated) matrix instance
   */
  Matrix *clone(void) const
  {
    MatrixImplementation *pimpl_clone =
      this->p_matrix_impl->clone();
    Matrix *result = new Matrix(pimpl_clone);
    return result;
  }

  /// Print to named file or standard output
  /** 
   * @e Collective
   * 
   * @param filename name of file to write, or NULL for standard output
   */
  void print(const char* filename = NULL) const;

  /// Save, in MatLAB format, to named file
  /** 
   * @e Collective
   * 
   * @param filename name of file to write
   */
  void save(const char *filename) const;

  /// Load from a named file of whatever binary format the math library uses
  /** 
   * @e Collective.
   *
   * The underlying math library generally supports some way to save a
   * Matrix to a file. This will load elements from a file of that
   * format.
   * 
   * @param filename 
   */
  void loadBinary(const char *filename);

  /// Save to named file in whatever binary format the math library uses
  /** 
   * @e Collective.
   *
   * The underlying math library generally supports some way to save a
   * Matrix to a file.  This routine uses whatever format that can be
   * read by ::loadBinary(). 
   * 
   * @param filename 
   */
  void saveBinary(const char *filename) const;

  // -------------------------------------------------------------
  // Matrix Operation Methods
  // -------------------------------------------------------------

  /// Get a part of the matrix (new instance allocated)
  // Matrix *submatrix(const int& istart, const int& jstart,
  //                   const int& iend, const int& jend) const;

  // -------------------------------------------------------------
  // In-Place Matrix Operation Methods (change this instance)
  // -------------------------------------------------------------

  /// Make this Matrix equal to another
  /** 
   * @e Collective.
   * 
   * @param A 
   */
  void equate(const Matrix& A);

  /// Scale this entire Matrix by the given value
  /** 
   * @e Collective.
   * 
   * @param x factor by which all elements in the matrix are multiplied
   */
  void scale(const ComplexType& x);

  /// Multiply this matrix diagonal by the specified vector
  /** 
   * @e Collective.
   *
   * This is element by element multiplication
   * 
   * @param x factor by which all diagonal elements in the matrix are multiplied
   */
  void multiplyDiagonal(const Vector& x);

  /// Add the specified vector to the diagonal of this matrix
  /** 
   * @c Collective.
   * 
   * @param x 
   */
  void addDiagonal(const Vector& x);

  /// Add another matrix to this one, in place
  /** 
   * @e Collective.
   *
   * The specified matrix @c A must be the same global size (rows and
   * columns) as this instance, but local ownership is not important,
   * nor are any differences in nonzero entry patterns.
   * 
   * @param A matrix to add to this instance
   */
  void add(const Matrix& A);

  /// Zero all entries in the matrix
  /** 
   * @e Collective.
   * 
   */
  void zero(void);

  // -------------------------------------------------------------
  // Matrix Operations 
  //
  // all allocate new instances and throw when arguments are
  // inconsistent
  // -------------------------------------------------------------
  // friend Matrix *factorize(const Matrix& A);
  // friend Matrix *inverse(const Matrix& A);
  // friend Matrix *reorder(const Matrix& A, const Reordering& r);
  // friend Matrix *identity(const Matrix& A);
  friend Matrix *multiply(const Matrix& A, const Matrix& B);
  friend Matrix *transpose(const Matrix& A);

protected:

  /// The actual implementation
  boost::scoped_ptr<MatrixImplementation> p_matrix_impl;

  /// Get the global index range of the locally owned rows (specialized)
  void p_localRowRange(IdxType& lo, IdxType& hi) const
  { p_matrix_impl->localRowRange(lo, hi); }

  /// Get the total number of rows in this matrix (specialized)
  IdxType p_rows(void) const
  { return p_matrix_impl->rows(); }

  /// Get the number of local rows in this matirx (specialized)
  IdxType p_localRows(void) const
  { return p_matrix_impl->localRows(); }

  /// Get the number of columns in this matrix (specialized)
  IdxType p_cols(void) const
  { return p_matrix_impl->cols(); }

  /// Get the number of local rows in this matirx (specialized)
  IdxType p_localCols(void) const
  { return p_matrix_impl->localCols(); }

  /// Set an individual element
  void p_setElement(const IdxType& i, const IdxType& j, const TheType& x)
  { p_matrix_impl->setElement(i, j, x); }

  /// Set an several element
  void p_setElements(const IdxType& n, 
                     const IdxType *i, const IdxType *j, 
                     const TheType *x)
  { p_matrix_impl->setElements(n, i, j, x); }

  /// Add to  an individual element
  void p_addElement(const IdxType& i, const IdxType& j, const TheType& x)
  { p_matrix_impl->addElement(i, j, x); }

  /// Add to  an several element
  void p_addElements(const IdxType& n, 
                     const IdxType *i, const IdxType *j, 
                     const TheType *x)
  { p_matrix_impl->addElements(n, i, j, x); }

  /// Get an individual element
  void p_getElement(const IdxType& i, const IdxType& j, TheType& x) const
  { p_matrix_impl->getElement(i, j, x); }

  /// Get an several element
  void p_getElements(const IdxType& n, 
                     const IdxType *i, const IdxType *j, 
                     TheType *x) const
  { p_matrix_impl->getElements(n, i, j, x); }

  /// Replace all elements with their real parts
  void p_real(void)
  { p_matrix_impl->real(); }

  /// Replace all elements with their imaginary parts
  void p_imaginary(void)
  { p_matrix_impl->imaginary(); }

  /// Replace all elements with their complex gradient
  void p_conjugate(void)
  { p_matrix_impl->conjugate(); }

  /// Compute the matrix L<sup>2</sup> norm (specialized)
  double p_norm2(void) const
  { return p_matrix_impl->norm2(); }

  /// Make this instance ready to use
  void p_ready(void)
  { p_matrix_impl->ready(); }

  /// Is a Matrix compatible with this one (throws if not)
  void p_check_compatible(const Matrix& A) const;

};

// -------------------------------------------------------------
// Matrix Operations 
//
// these allocate new instances and throw when arguments are
// inconsistent
// -------------------------------------------------------------

/// Add two Matrix instances
/** 
 * @e Collective.
 *
 * @c A and @c B must have the same communicator and the same
 * size. Different parallel distributions and nonzero patterns are OK,
 * though.
 * 
 * @param A 
 * @param B 
 * 
 * @return pointer to new Matrix containing A+B
 */
extern Matrix *add(const Matrix& A, const Matrix& B);

/// Make the transpose of a Matrix
/** 
 * @e Collective.
 *
 * 
 * 
 * @param A 
 * 
 * @return pointer to a new Matrix containing A<sup>T</sup>
 */
extern Matrix *transpose(const Matrix& A);

/// Get a column from the Matrix and put in new Vector
/** 
 * 
 * 
 * @param A 
 * @param cidx 
 * 
 * @return 
 */
extern Vector *column(const Matrix& A, const int& cidx);

/// Get the diagonal from a Matrix and put in new Vector
/** 
 * @e Collective.
 * 
 * @param A 
 * 
 * @return pointer to new, allocated Vector containing the diagonal elements of @c A
 */
extern Vector *diagonal(const Matrix& A);

/// Make a diagonal Matrix from a Vector
/** 
 * @e Collective.
 *
 * This 
 * 
 * @param x 
 * 
 * @return 
 */
extern Matrix *diagonal(const Vector& x, 
                        const Matrix::StorageType& stype = Matrix::Sparse);

/// Multiply two Matrix instances and make a new one
/** 
 * 
 * 
 * @param A 
 * @param B 
 * 
 * @return 
 */
extern Matrix *multiply(const Matrix& A, const Matrix& B);

/// Multiply a Matrix by a Vector and make a new Vector for the result
/** 
 * 
 * @param A 
 * @param x 
 * 
 * @return 
 */
extern Vector *multiply(const Matrix& A, const Vector& x);

/// Multiply the transpose of a Matrix by a Vector and make a new Vector for the result
/** 
 * 
 * 
 * @param A 
 * @param x 
 * 
 * @return 
 */
extern Vector *transposeMultiply(const Matrix& A, const Vector& x);

/// Make an identity matrix with the same ownership as the specified matrix
extern Matrix *identity(const Matrix& A);

/// Create a new matrix containing the real part of the specified matrix
extern Matrix *real(const Matrix& A);

/// Create a new matrix containing the imaginary part of the specified matrix
extern Matrix *imaginary(const Matrix& A);

/// Create a new matrix containing the complex conjugate of the specified matrix
extern Matrix *conjugate(const Matrix& A);

/// Create a copy of a Matrix, possibly with a different storage type
extern Matrix *storageType(const Matrix& A, const Matrix::StorageType& new_type);

// -------------------------------------------------------------
// Matrix Operations
//
// put results in existing instance and throw when arguments are
// inconsistent
// -------------------------------------------------------------

/// Add two Matrix instances and put the result in a third
extern void add(const Matrix& A, const Matrix& B, Matrix& result);

/// Make the transpose of a Matrix and put it in another
/** 
 * 
 * 
 * @param A 
 * @param result 
 */
extern void transpose(const Matrix& A, Matrix& result);

/// Get a column from the Matrix and put in specified Vector
/** 
 * 
 * 
 * @param A 
 * @param cidx 
 * @param x 
 */
extern void column(const Matrix& A, const int& cidx, Vector& x);

/// Get the diagonal from a Matrix and put it in specified Vector
/** 
 * 
 * @param A 
 * @param x 
 */
extern void diagonal(const Matrix& A, Vector& x);

/// Multiply two Matrix instances and put result in existing Matrix
/** 
 * 
 * 
 * @param A 
 * @param B 
 * @param result 
 */
extern void multiply(const Matrix& A, const Matrix& B, Matrix& result);

/// Multiply a Matrix by a Vector and put result in existing Vector
/** 
 * @c A, @c x, and @c result must all have the same \ref
 * parallel::Communicator "communicator". @c x and @c result must be
 * the same size. The length of @c x must be the number of columns in
 * @c A. If these conditions are not met, an exception is thrown.
 * 
 * @param A 
 * @param x 
 * @param result 
 */
extern void multiply(const Matrix& A, const Vector& x, Vector& result);

/// Multiply the transpose of a Matrix by a Vector and put the result in existing Vector
/** 
 * 
 * 
 * @param A 
 * @param x 
 * @param result same size and distribution as @c x
 */
extern void transposeMultiply(const Matrix& A, const Vector& x, Vector& result);


} // namespace math
} // namespace gridpack



#endif
