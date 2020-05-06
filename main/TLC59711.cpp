#include <machine/endian.h>

#include "TLC59711.h"

namespace TLC59711 {

// symmetric encode/decode for 16 bit value
static uint16_t translate(uint16_t value) {
    return
#if BYTE_ORDER == BIG_ENDIAN
    value;
#else
    __builtin_bswap16(value);
#endif
}

// symmetric encode/decode for 32 bit value
static uint32_t translate(uint32_t value) {
    return
#if BYTE_ORDER == BIG_ENDIAN
    value;
#else
    __builtin_bswap32(value);
#endif
}

GS::GS(uint16_t value) : encoding {translate(value)} {}
GS::operator uint16_t() const {return translate(encoding);}

static auto constexpr latch {0x25};

Message::Message(
    unsigned	rbc,
    unsigned	gbc,
    unsigned	bbc,
    bool	blank,
    bool	dsprpt,
    bool	tmgrst,
    bool	extclk,
    bool	outtmg
) :
    control {translate(0
	| (0b00111111 & latch	) << 26
	| (0b00000001 & outtmg	) << 25
	| (0b00000001 & extclk	) << 24
	| (0b00000001 & tmgrst	) << 23
	| (0b00000001 & dsprpt	) << 22
	| (0b00000001 & blank	) << 21
	| (0b01111111 & bbc	) << 14
	| (0b01111111 & gbc	) <<  7
	| (0b01111111 & rbc	) <<  0
    )},
    gs	{}
{}

}
