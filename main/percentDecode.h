#ifndef percentDecode_h_
#define percentDecode_h_

#include <cstdlib>

/// decode a percent encoded string
/// https://en.wikipedia.org/wiki/Percent-encoding
/// from source into target,
/// returning source and target pointers where the decoding stopped.
/// decoding stops when
///	* target is full
///	* source is null, space, delimeter or undecodable
/// the source can be decoded onto itself (can be the target)
/// but two pointers must still be used.
void percentDecode(
    char * &		target,
    char const * &	source,
    char		delimeter,
    size_t		size = ~0);

#endif
