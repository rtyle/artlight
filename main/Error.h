#pragma once

#include <esp_err.h>

namespace Error {

/// Throw an object of type T (default esp_err_t) if it tests true (not ESP_OK).
template<typename T = esp_err_t>
static inline void throwIf(T t) {if (t) throw t;}

/// Throw an object of type T (default esp_err_t) if it is equal to another
/// (default, ESP_FAIL)
template<typename T = esp_err_t, T is = ESP_FAIL>
static inline T throwIfIs(T t) {if (is == t) throw t; return t;}

}
