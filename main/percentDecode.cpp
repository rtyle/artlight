#include <cctype>
#include <cstdint>

#include "percentDecode.h"

// decode a hex nibble if we can from c (0-15); otherwise, -1
static int8_t nibble(char c) {
    if (std::isdigit(c)) {
	return c - '0';
    } else if (std::isxdigit(c)) {
	return 10 + (std::tolower(c) - 'a');
    } else {
	return -1;
    }
}

void percentDecode(char * & t, char const * & s, char d, size_t n) {
    while (n-- && *s && !(' ' == * s || d == *s)) {
	if ('%' == *s) {
	    int8_t a, b;
	    if (0 > (a = nibble(s[1])) || 0 > (b = nibble(s[2]))) {
		return;
	    }
	    *t++ = a << 4 | b;
	    s += 3;
	} else {
	    *t++ = *s++;
	}
    }
}
