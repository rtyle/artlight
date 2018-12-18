#ifndef Error_h_
#define Error_h_

#include <esp_err.h>

namespace Error {

template<typename T = esp_err_t>
static inline void throwIf(T t) {if (t) throw t;}

template<typename T = esp_err_t, T is = -1>
static inline T throwIfIs(T t) {if (is == t) throw t; return t;}

}

#endif
