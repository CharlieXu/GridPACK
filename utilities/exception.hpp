/**
 * @file   exception.hpp
 * @author William A. Perkins
 * @date   2013-05-08 13:45:00 d3g096
 * 
 * @brief  Declaration of a GridPACK-specific exception class
 * 
 * 
 */

#include <string>
#include <stdexcept>

namespace gridpack {

// -------------------------------------------------------------
//  class Exception
// -------------------------------------------------------------
class Exception : public std::exception {
public:

  /// Default constructor.
  Exception(void) 
    : std::exception(), message_("Unknown GridPACK Error") 
  {}
  
  /// Construct w/ a (string) message
  explicit Exception(const std::string& s)
    : std::exception(), message_(s) 
  {}

  /// Construct w/ a char message
  explicit Exception(const char *s)
    : std::exception(), message_(s) 
  {}

  /// Copy constructor
  Exception(const Exception& old)
    : std::exception(old), message_(old.message_)
  {}

  /// Destructor
  ~Exception(void) throw()
  {}

  /// Report the error
  const char *what(void) const throw()
  {
    return message_.c_str();
  }

protected:

  /// The error message
  std::string message_;

};



} // namespace gridpack
