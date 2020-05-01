#pragma once

#include <cstdint>

namespace TLC59711 {

// http://www.ti.com/lit/ds/sbvs181a/sbvs181a.pdf

// The TLC59711 has a 224-bit shift register.
// A bit value is sampled from the signal on the SDTI (Serial DaTa In) pin
// on the rising edge of the signal on the SCKI (Serial ClocK In) pin.
// The entire register is shifted one bit toward its most significant bit (MSB)
// and the sampled value is put in the least significant bit (LSB).
// Each MSB that is shifted out is reflected on the SDTO and SCKO
// (Serial DaTa and ClocK Out) pins
// so that multiple devices might be daisy chained together.
//
// When the SCKI signal does not rise for 8 periods
// (where period is determined by the last rising edge to rising edge cycle),
// the six MSBs of the shift register are evaluated.
// If these six bits are 0x25 (0b100101),
// the remaining 218 bits are latched.
//
// The 26 MSB of the latch are, from most to least significant
//	Function Control (FC),				5 bits
//	Blue	(B)	global Brightness Control (BC)	7 bits
//	Green	(G)	global BC			7 bits
//	Red	(R)	global BC			7 bits
// These are followed by 192 Gray Scale (GS) bits.
// These are 12 16 bit values that are ordered (most to least) as 4 BGR triplets
//	BGR3
//	BGR2
//	BGR1
//	BGR0
//
// The FC bits are (from most to least significant)
//	OUTTMG	GS reference clock edge select bit for on-off timing control
//		0	turn on/off on falling edge
//		1	turn on/off on rising edge
//	EXTCLK	GS reference clock select bit
//		0	internal oscillator clock (10MHz)
//		1	SCKI clock
//	TMGRST
//		0	GS counter not reset, outputs unaffected
//		1	GS counter is reset to 0, outputs are forced off
//	DSPRPT	Display Repeat mode
//		0	disabled, outputs off after GS counter wraps
//		1	enabled, outputs repeat according to GS/BC values
//	BLANK	Blank output
//		0	Outputs controlled by GS/BC values
//		1	Outputs off
//
// Typically, the outputs are used to control RGB LEDs
// but this need not be the case (although BGR BC might not make sense).
//
// BC values are used as a numerator in a fraction with a 0x7F denominator
// to determine how to scale down the current, globally, to B, G and R channels.
//
// Each 16 BGR GS value is used to Pulse Width Modulate (PWM)
// its associated output pin.
// For a period of 2**16 GS clock pulses, the 16 bit GS value specifies the
// number of GS clock pulses where the (BC scaled) output current is on;
// during the rest the output is off.
// Rather than the on time being contiguous,
// it is split as evenly as possible between 2**7 segments
// where the duration of each segment is 2**9 GS clock pulses.

// TLC59711 expects SPI clock to idle low (CPOL 0) and
// the first data sample to be taken on the first (rising) clock edge (CPHA 0)
auto constexpr spiCPOL {0};
auto constexpr spiCPHA {0};
// these are encoded in the SPI mode
auto constexpr spiMode {(spiCPOL << 1) | (spiCPHA << 0)};

/// A GS encodes (and decodes) a TLC59711 16 bit grayscale value
struct GS {
private:
    uint16_t	encoding;
public:
    GS(uint16_t value = 0);
    operator uint16_t() const;
};

/// A Message contains what is needed to set TLC59711 latched data.
/// If multiple TLC59711 are daisy chained together,
/// an array of Message objects (ordered from last in chain to first)
/// may be used to affect them all in one SPI transaction.
/// The control bits are set at construction time.
/// The GS values are zeroed by the constructor
/// but can be publicly accessed later as an array.
struct Message {
private:
    uint32_t	control;
public:
    GS	gs[12];

    Message(
	unsigned	rbc	= ~0,
	unsigned	gbc	= ~0,
	unsigned	bbc	= ~0,
	bool		blank	= false,
	bool		dsprpt	= true,
	bool		tmgrst	= false,
	bool		extclk	= false,
	bool		outtmg	= false
    );
};

}
