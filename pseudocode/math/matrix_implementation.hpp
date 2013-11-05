// Emacs Mode Line: -*- Mode:c++;-*-
// -------------------------------------------------------------
/**
 * @file   matrix_implementation.h
 * @author William A. Perkins
 * @date   Mon Mar 25 12:10:01 2013
 * 
 * @brief  
 * 
 * 
 */
// -------------------------------------------------------------
// -------------------------------------------------------------
// Created March 25, 2013 by William A. Perkins
// Last Change: Thu Jun  3 06:45:08 2010 by William A. Perkins <d3g096@PE10900.pnl.gov>
// -------------------------------------------------------------

// SCCS ID: $Id$ Battelle PNL

#ifndef _matrix_implementation_h_
#define _matrix_implementation_h_

#include "gridpack/parallel/distributable.hpp"
#include "gridpack/utlity/uncopyable.hpp"
#include "gridpack/math/math_type.hpp"

namespace gridpack {
namespace math {

// -------------------------------------------------------------
//  class MatrixImplementation
// -------------------------------------------------------------
class MatrixImplementation 
  : public parallel::Distributable, 
    private utility::UnCopyable 
{
public:

  /// Default constructor.
  MatrixImplementation(const parallel::Distribution& dist, 
                       const int& rows, const int& cols);

  /// Destructor
  virtual ~MatrixImplementation(void);

  /// Set an individual element
  void set_element(const int& i, const int& j, const complex_type& x)
  {
    this->set_element_(i, j, x);
  }

  /// Set an several elements
  void set_elements(cont int& n, const int *i, const int *j, const complex_type *x)
  {
    this->set_elements_(n, i, j, x);
  }

  /// Set all elements in a row
  void set_row(const int& nj, const int& i, const int *j, const complex_type *x)
  {
    this->set_row_(nj, i, j, x);
  }

  /// Set all elements in a row
  void set_region(const int& ni, const int& nj, const int *i, const int *j, const complex_type *x)
  {
    this->set_region_(ni, nj, i, j, x);
  }

  /// Add to an individual element
  void add_element(const int& i, const int& j, const complex_type& x)
  {
    this->add_element_(i, j, x);
  }

  /// Add to an several elements
  void add_elements(const int& n, const int *i, const int *j, const complex_type *x)
  {
    this->add_elements_(n, i, j, x);
  }

  /// Add to all elements in a row
  void add_row(const int& nj, const int& i, const int *j, const complex_type *x)
  {
    this->add_row_(nj, i, j, x);
  }

  /// Get an individual element
  void get_element(const int& i, const int& j, const complex_type& x)
  {
    this->get_element_(i, j, x);
  }

  /// Get an several elements
  void get_elements(cont int& n, const int *i, const int *j, const complex_type *x)
  {
    this->get_elements_(n, i, j, x);
  }

  /// Get all elements in a row
  void get_row(const int& nj, const int& i, const int *j, const complex_type *x)
  {
    this->get_row_(nj, i, j, x);
  }

  /// Get all elements in a row
  void get_region(const int& ni, const int& nj, 
                  const int *i, const int *j, const complex_type *x)
  {
    this->get_region_(ni, nj, i, j, x);
  }

  /// Allow visits by implemetation visitor
  void accept(ImplementationVisitor& visitor)
  {
    this->accept_(visitor);
  }

protected:

  /// Set an individual element
  virtual void set_element_(const int& i, const int& j, const complex_type& x) = 0;

  /// Set an several element
  virtual void set_elements_(const int *i, const int *j, const complex_type *x) = 0;

  /// Set all elements in a row
  virtual void set_row_(const int& i, const int *j, const complex_type *x) = 0;

  /// Set all elements in a region
  virtual void set_region_(const int& ni, const int& nj, 
                           const int *i, const int *j, const complex_type *x) = 0;

  /// Add to  an individual element
  virtual void add_element_(const int& i, const int& j, const complex_type& x) = 0;

  /// Add to  an several element
  virtual void add_elements_(const int *i, const int *j, const complex_type *x) = 0;

  /// Add to  all elements in a row
  virtual void add_row_(const int& i, const int *j, const complex_type *x) = 0;

  /// Get an individual element
  virtual void get_element_(const int& i, const int& j, complex_type& x) const = 0;

  /// Get an several element
  virtual void get_elements_(const int *i, const int *j, complex_type *x) const = 0;

  /// Get all elements in a row
  virtual void get_row_(const int& i, const int *j, complex_type *x) const = 0;

  /// Get all elements in a region
  virtual void get_region_(const int& ni, const int& nj, 
                           const int *i, const int *j, complex_type *x) const = 0;

  /// Allow visits by implementation visitors
  virtual void accept(ImplementationVisitor& visitor) = 0;

};

} // namespace utility
} // namespace gridpack



#endif
