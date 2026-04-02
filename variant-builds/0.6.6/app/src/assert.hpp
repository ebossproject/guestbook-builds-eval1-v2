#pragma once

#undef assert

#define assert(expr) \
  (static_cast<bool>(expr) ? void(0) :                 \
  assert_fail(#expr, __FILE__, __LINE__,                \
              __extension__ __PRETTY_FUNCTION__))

#define assert_zero(expr) \
  (static_cast<bool>(0 == expr) ? void(0) :             \
   assert_zero_fail(#expr, __FILE__, __LINE__,        \
                      __extension__ __PRETTY_FUNCTION__))

[[ noreturn ]] void assert_fail(const char* assertion,
                 const char* file,
                 unsigned int line,
                 const char* function);

[[ noreturn ]] void assert_zero_fail(const char* assertion,
                 const char* file,
                 unsigned int line,
                 const char* function);
