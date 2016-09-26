#ifndef CHILL_ERROR_HH
#define CHILL_ERROR_HH

/*!
 * \file
 * \brief CHiLL runtime exceptions
 */

namespace chill {
  namespace error {
    //! for loop transformation problem
    struct loop : public std::runtime_error {
      loop(const std::string &msg) : std::runtime_error(msg) {}
    };

    //! for generic compiler intermediate code handling problem
    struct ir : public std::runtime_error {
      ir(const std::string &msg) : std::runtime_error(msg) {}
    };

    //! for specific for expression to preburger math translation problem
    struct ir_exp : public ir {
      ir_exp(const std::string &msg) : ir(msg) {}
    };

    struct omega : public std::runtime_error {
      omega(const std::string &msg) : std::runtime_error(msg) {}
    };
  }
}
#endif
