#include <array>
#include <cassert>
#include <cmath>
#include <limits>
#include <random>
#include <set>
#include <sstream>
#include <vector>

#include <esp_log.h>
#include <esp_heap_caps.h>

#include "clip.h"
#include "fromString.h"
#include "Blend.h"
#include "Contrast.h"
#include "GoldenArtTask.h"
#include "Curve.h"
#include "PerlinNoise.hpp"
#include "Timer.h"

extern "C" uint64_t get_time_since_boot();

using APA102::LED;
using LEDI = APA102::LED<int>;

constexpr float phi	{(1.0f + std::sqrt(5.0f)) / 2.0f};
constexpr float pi	{std::acos(-1.0f)};
constexpr float tau	{2.0f * pi};

// https://en.wikipedia.org/wiki/Fibonacci_number
// 0, 1, 2, 3, 4, 5, 6,  7,  8,  9, 10, 11,  12, ...
// 0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, ...
static constexpr unsigned fibonacci_(unsigned a, unsigned b, unsigned i) {
    return i ? fibonacci_(b, a + b, i - 1) : a + b;
}
constexpr unsigned n0 {0};
constexpr unsigned n1 {1};
static constexpr unsigned fibonacci(unsigned i) {
    return i > 1 ? fibonacci_(n0, n1, i - 2) : i ? n1 : n0;
}

constexpr unsigned millisecondsPerSecond	{1000u};
constexpr unsigned microsecondsPerSecond	{1000000u};

constexpr size_t ledCount {1024};

char const * const GoldenArtTask::Mode::string[]
    {"clock", "swirl", "solid"};
GoldenArtTask::Mode::Mode(Value value_) : value(value_) {}
GoldenArtTask::Mode::Mode(char const * value) : value(
    [value](){
	size_t i {0};
	for (auto e: string) {
	    if (0 == std::strcmp(e, value)) return static_cast<Value>(i);
	    ++i;
	}
	return clock;
    }()
) {}
char const * GoldenArtTask::Mode::toString() const {
    return string[value];
}

namespace {
// A Path may be used to construct a standard input iterator
// that begins at an unsigned value,
// decrements a unsigned step (default 1) as it is advanced
// until it exhausts all unsigned values on this path.
class Path {
public:
    class Iterator: public std::iterator<std::input_iterator_tag, unsigned,
	unsigned, unsigned const *, unsigned> {
    public:
	explicit Iterator(Path const * path_)
	    : path{path_}, value{path->value} {}
	explicit Iterator()
	    : path{nullptr}, value{std::numeric_limits<value_type>::max()} {}
	Iterator & operator++() {
	    if (path) {
		if (value < path->step)
		    value = std::numeric_limits<value_type>::max();
		else
		    value -= path->step;
	    }
	    return *this;
	}
	bool operator==(Iterator that) const {return value == that.value;}
	bool operator!=(Iterator that) const {return !(*this == that);}
	reference operator*() const {return value;}
    private:
	Path const * path;
	value_type value;
    };
    Path(Iterator::value_type value_, Iterator::value_type step_ = 1) : value{value_}, step{step_} {}
    Iterator begin() {return Iterator(this);}
    Iterator end() {return Iterator();}
private:
    Iterator::value_type const value;
    Iterator::value_type const step;
};
}

static constexpr float phaseIn(uint64_t time, uint64_t period) {
    return (time % period) / static_cast<float>(period);
}

void GoldenArtTask::update_() {
    // LED i [0, ledCount) is rendered like a seed in a sunflower head
    // at polar coordinate
    //	(scale * sqrt(1 + i), i * tau / phi)
    // where scale was chosen to space the LEDs adequately
    // (18 mil for SK9822 5050 (5.0 x 5.0 mm) LEDs).
    //
    // such rendering creates a family of spirals for each fibonacci number N
    // where
    // * N is the number of spirals in the family and
    // * N separates neighboring LEDs in any spiral.

    // for rendering a family of spirals,
    // it is convenient to have the outermost rim of indeces (from the end)
    // of each ordered in a clockwise manner from the top.
    // these were imported from exploratory rendering code.
    // see easyeda/projects/golden/golden.html.
    static constexpr uint8_t rim1024_0[fibonacci(0)] {
    };
    static constexpr uint8_t rim1024_1[fibonacci(1)] {
        0,
    };
    static constexpr uint8_t rim1024_2[fibonacci(2)] {
	0,
    };
    static constexpr uint8_t rim1024_3[fibonacci(3)] {
        0, 1,
    };
    static constexpr uint8_t rim1024_4[fibonacci(4)] {
        0, 2, 1,
    };
    static constexpr uint8_t rim1024_5[fibonacci(5)] {
        0, 2, 4, 1, 3,
    };
    static constexpr uint8_t rim1024_6[fibonacci(6)] {
        0, 5, 2, 7, 4, 1, 6, 3,
    };
    static constexpr uint8_t rim1024_7[fibonacci(7)] {
         0,  5, 10,  2,  7, 12,  4,  9,  1,  6, 11,  3,  8,
    };
    static constexpr uint8_t rim1024_8[fibonacci(8)] {
         0, 13,  5, 18, 10,  2, 15,  7, 20, 12,  4, 17,  9,  1, 14,  6,
        19, 11,  3, 16,  8,
    };
    static constexpr uint8_t rim1024_9[fibonacci(9)] {
         0, 13, 26,  5, 18, 31, 10, 23,  2, 15, 28,  7, 20, 33, 12, 25,
         4, 17, 30,  9, 22,  1, 14, 27,  6, 19, 32, 11, 24,  3, 16, 29,
         8, 21,
    };
    static constexpr uint8_t rim1024_10[fibonacci(10)] {
         0, 34, 13, 47, 26,  5, 39, 18, 52, 31, 10, 44, 23,  2, 36, 15,
        49, 28,  7, 41, 20, 54, 33, 12, 46, 25,  4, 38, 17, 51, 30,  9,
        43, 22,  1, 35, 14, 48, 27,  6, 40, 19, 53, 32, 11, 45, 24,  3,
        37, 16, 50, 29,  8, 42, 21,
    };
    static constexpr uint8_t rim1024_11[fibonacci(11)] {
         0, 34, 68, 13, 47, 81, 26, 60,  5, 39, 73, 18, 52, 86, 31, 65,
        10, 44, 78, 23, 57,  2, 36, 70, 15, 49, 83, 28, 62,  7, 41, 75,
        20, 54, 88, 33, 67, 12, 46, 80, 25, 59,  4, 38, 72, 17, 51, 85,
        30, 64,  9, 43, 77, 22, 56,  1, 35, 69, 14, 48, 82, 27, 61,  6,
        40, 74, 19, 53, 87, 32, 66, 11, 45, 79, 24, 58,  3, 37, 71, 16,
        50, 84, 29, 63,  8, 42, 76, 21, 55,
    };
    static constexpr uint8_t rim1024_12[fibonacci(12)] {
          0,  89,  34, 123,  68,  13, 102,  47, 136,  81,  26, 115,  60,   5,  94,  39,
        128,  73,  18, 107,  52, 141,  86,  31, 120,  65,  10,  99,  44, 133,  78,  23,
        112,  57,   2,  91,  36, 125,  70,  15, 104,  49, 138,  83,  28, 117,  62,   7,
         96,  41, 130,  75,  20, 109,  54, 143,  88,  33, 122,  67,  12, 101,  46, 135,
         80,  25, 114,  59,   4,  93,  38, 127,  72,  17, 106,  51, 140,  85,  30, 119,
         64,   9,  98,  43, 132,  77,  22, 111,  56,   1,  90,  35, 124,  69,  14, 103,
         48, 137,  82,  27, 116,  61,   6,  95,  40, 129,  74,  19, 108,  53, 142,  87,
         32, 121,  66,  11, 100,  45, 134,  79,  24, 113,  58,   3,  92,  37, 126,  71,
         16, 105,  50, 139,  84,  29, 118,  63,   8,  97,  42, 131,  76,  21, 110,  55,
    };
    static constexpr uint8_t const * const rim1024[] {
        rim1024_0,
        rim1024_1,
        rim1024_2,
        rim1024_3,
        rim1024_4,
        rim1024_5,
        rim1024_6,
        rim1024_7,
        rim1024_8,
        rim1024_9,
        rim1024_10,
        rim1024_11,
        rim1024_12,
    };

    // the natural rendering indeces need to be mapped to the path indeces
    // that reflect the way the LEDs are actually wired (addressed).
    // this order is conducive for board/trace layout.
    // see easyeda/projects/golden/scripts/goldenPath.js
    static constexpr uint16_t layout[ledCount] {
	   0,    4,    8,    2,    6,    9,    3,    7,    1,    5,   22,   13,   18,   10,   15,   20,
	  12,   17,   23,   14,   19,   11,   16,   21,   29,   38,   24,   33,   42,   27,   36,   45,
	  31,   40,   25,   34,   43,   28,   37,   47,   32,   41,   26,   35,   44,   30,   39,   49,
	  61,   73,   54,   66,   46,   58,   70,   51,   63,   75,   56,   68,   48,   60,   72,   53,
	  65,   77,   57,   69,   50,   62,   74,   55,   67,   79,   59,   71,   52,   64,   76,   89,
	 102,   81,   94,  107,   86,   99,   78,   91,  104,   83,   96,  109,   88,  101,   80,   93,
	 106,   85,   98,  111,   90,  103,   82,   95,  108,   87,  100,  114,   92,  105,   84,   97,
	 110,  130,  151,  117,  138,  159,  125,  146,  112,  133,  154,  120,  141,  162,  128,  149,
	 115,  136,  157,  123,  144,  165,  131,  152,  118,  139,  160,  126,  147,  113,  134,  155,
	 121,  142,  163,  129,  150,  116,  137,  158,  124,  145,  166,  132,  153,  119,  140,  161,
	 127,  148,  169,  135,  156,  122,  143,  164,  185,  206,  172,  193,  214,  180,  201,  167,
	 188,  209,  175,  196,  217,  183,  204,  170,  191,  212,  178,  199,  220,  186,  207,  173,
	 194,  215,  181,  202,  168,  189,  210,  176,  197,  218,  184,  205,  171,  192,  213,  179,
	 200,  221,  187,  208,  174,  195,  216,  182,  203,  224,  190,  211,  177,  198,  219,  240,
	 261,  227,  248,  269,  235,  256,  222,  243,  264,  230,  251,  272,  238,  259,  225,  246,
	 267,  233,  254,  275,  241,  262,  228,  249,  270,  236,  257,  223,  244,  265,  231,  252,
	 273,  239,  260,  226,  247,  268,  234,  255,  276,  242,  263,  229,  250,  271,  237,  258,
	 279,  245,  266,  232,  253,  274,  296,  317,  283,  304,  325,  291,  312,  277,  299,  320,
	 286,  307,  328,  294,  315,  281,  302,  323,  289,  310,  331,  297,  318,  284,  305,  326,
	 292,  313,  278,  300,  321,  287,  308,  329,  295,  316,  282,  303,  324,  290,  311,  333,
	 298,  319,  285,  306,  327,  293,  314,  280,  301,  322,  288,  309,  330,  353,  374,  340,
	 361,  382,  348,  369,  335,  356,  377,  343,  364,  385,  351,  372,  338,  359,  380,  346,
	 367,  332,  354,  375,  341,  362,  383,  349,  370,  336,  357,  378,  344,  365,  387,  352,
	 373,  339,  360,  381,  347,  368,  334,  355,  376,  342,  363,  384,  350,  371,  337,  358,
	 379,  345,  366,  389,  415,  441,  399,  425,  451,  409,  435,  393,  419,  445,  403,  429,
	 386,  413,  439,  397,  423,  449,  407,  433,  391,  417,  443,  401,  427,  453,  411,  437,
	 395,  421,  447,  405,  431,  388,  414,  440,  398,  424,  450,  408,  434,  392,  418,  444,
	 402,  428,  454,  412,  438,  396,  422,  448,  406,  432,  390,  416,  442,  400,  426,  452,
	 410,  436,  394,  420,  446,  404,  430,  456,  482,  508,  466,  492,  518,  476,  502,  460,
	 486,  512,  470,  496,  522,  480,  506,  464,  490,  516,  474,  500,  458,  484,  510,  468,
	 494,  520,  478,  504,  462,  488,  514,  472,  498,  455,  481,  507,  465,  491,  517,  475,
	 501,  459,  485,  511,  469,  495,  521,  479,  505,  463,  489,  515,  473,  499,  457,  483,
	 509,  467,  493,  519,  477,  503,  461,  487,  513,  471,  497,  523,  557,  591,  536,  570,
	 604,  549,  583,  528,  562,  596,  541,  575,  609,  554,  588,  533,  567,  601,  546,  580,
	 525,  559,  593,  538,  572,  606,  551,  585,  530,  564,  598,  543,  577,  611,  556,  590,
	 535,  569,  603,  548,  582,  527,  561,  595,  540,  574,  608,  553,  587,  532,  566,  600,
	 545,  579,  524,  558,  592,  537,  571,  605,  550,  584,  529,  563,  597,  542,  576,  610,
	 555,  589,  534,  568,  602,  547,  581,  526,  560,  594,  539,  573,  607,  552,  586,  531,
	 565,  599,  544,  578,  612,  646,  680,  625,  659,  693,  638,  672,  617,  651,  685,  630,
	 664,  698,  643,  677,  622,  656,  690,  635,  669,  614,  648,  682,  627,  661,  695,  640,
	 674,  619,  653,  687,  632,  666,  700,  645,  679,  624,  658,  692,  637,  671,  616,  650,
	 684,  629,  663,  697,  642,  676,  621,  655,  689,  634,  668,  613,  647,  681,  626,  660,
	 694,  639,  673,  618,  652,  686,  631,  665,  699,  644,  678,  623,  657,  691,  636,  670,
	 615,  649,  683,  628,  662,  696,  641,  675,  620,  654,  688,  633,  667,  701,  735,  769,
	 714,  748,  782,  727,  761,  706,  740,  774,  719,  753,  787,  732,  766,  711,  745,  779,
	 724,  758,  703,  737,  771,  716,  750,  784,  729,  763,  708,  742,  776,  721,  755,  789,
	 734,  768,  713,  747,  781,  726,  760,  705,  739,  773,  718,  752,  786,  731,  765,  710,
	 744,  778,  723,  757,  702,  736,  770,  715,  749,  783,  728,  762,  707,  741,  775,  720,
	 754,  788,  733,  767,  712,  746,  780,  725,  759,  704,  738,  772,  717,  751,  785,  730,
	 764,  709,  743,  777,  722,  756,  790,  824,  858,  803,  837,  871,  816,  850,  795,  829,
	 863,  808,  842,  876,  821,  855,  800,  834,  868,  813,  847,  792,  826,  860,  805,  839,
	 873,  818,  852,  797,  831,  865,  810,  844,  878,  823,  857,  802,  836,  870,  815,  849,
	 794,  828,  862,  807,  841,  875,  820,  854,  799,  833,  867,  812,  846,  791,  825,  859,
	 804,  838,  872,  817,  851,  796,  830,  864,  809,  843,  877,  822,  856,  801,  835,  869,
	 814,  848,  793,  827,  861,  806,  840,  874,  819,  853,  798,  832,  866,  811,  845,  879,
	 934,  989,  900,  955, 1010,  921,  976,  887,  942,  997,  908,  963, 1018,  929,  984,  895,
	 950, 1005,  916,  971,  882,  937,  992,  903,  958, 1013,  924,  979,  890,  945, 1000,  911,
	 966, 1021,  932,  987,  898,  953, 1008,  919,  974,  885,  940,  995,  906,  961, 1016,  927,
	 982,  893,  948, 1003,  914,  969,  880,  935,  990,  901,  956, 1011,  922,  977,  888,  943,
	 998,  909,  964, 1019,  930,  985,  896,  951, 1006,  917,  972,  883,  938,  993,  904,  959,
	1014,  925,  980,  891,  946, 1001,  912,  967, 1022,  933,  988,  899,  954, 1009,  920,  975,
	 886,  941,  996,  907,  962, 1017,  928,  983,  894,  949, 1004,  915,  970,  881,  936,  991,
	 902,  957, 1012,  923,  978,  889,  944,  999,  910,  965, 1020,  931,  986,  897,  952, 1007,
	 918,  973,  884,  939,  994,  905,  960, 1015,  926,  981,  892,  947, 1002,  913,  968, 1023,
    };

    // construct static PerlinNoise objects
    static std::mt19937 rng;
    static PerlinNoise perlinNoise[] {rng, rng, rng, rng, rng};
    // Perlin noise repeats every 256 units.
    constexpr unsigned perlinNoisePeriod {256};
    constexpr uint64_t perlinNoisePeriodMicroseconds
	{perlinNoisePeriod * microsecondsPerSecond};
    // Perlin noise at an integral grid point is 0 (0.5 for noise0_1).
    // To avoid this, cut between them.

    uint64_t const microsecondsSinceBoot {get_time_since_boot()};

    Contrast const levelContrast {20.0f};

    APA102::Message<1> message0;
    {
	constexpr auto levelEnd {64.0f};	// [0, levelEnd)
	float const x {(microsecondsSinceBoot % perlinNoisePeriodMicroseconds)
	    / static_cast<float>(microsecondsPerSecond)};
	for (auto & e: message0.encodings) {
	    e = LED<> {
		static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[0].noise0_1(x)), 0.0f)),
		static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[1].noise0_1(x)), 0.0f)),
		static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[2].noise0_1(x)), 0.0f))
	    };
	}
    }

    APA102::Message<ledCount> message1;
    switch (mode.value) {
	case Mode::Value::clock: {
	    static constexpr uint8_t const * const rim[] {
		rim1024_8,
		rim1024_10,
		rim1024_12,
	    };
	    static constexpr unsigned rimEnd[] {
		1024u,
		1024u,
		1024u,
	    };
	    static constexpr unsigned rimIndex[] {	// fibonacci indexes
		 8u,
		10u,
		12u,
	    };
	    static SawtoothCurve const sawtooth[] {
		{0.0f, 60.0f * 60.0f * 12.0f},	// 12 hour clock
		{0.0f, 60.0f * 60.0f},
		{0.0f, 60.0f},
	    };

	    float const secondsSinceTwelveLocaltime {
		smoothTime.millisecondsSinceTwelveLocaltime(microsecondsSinceBoot)
		    / static_cast<float>(millisecondsPerSecond)};

	    auto * width_	{width};
	    auto * color_	{color};
	    auto * rim_		{rim};
	    auto * rimEnd_	{rimEnd};
	    auto * rimIndex_	{rimIndex};
	    auto * sawtooth_	{sawtooth};
	    for (auto i = 0u; i < 3; ++i) {
		if (*width_) {
		    static LED<> const black{0, 0, 0};

		    auto position		{(*sawtooth_)(secondsSinceTwelveLocaltime)};
		    auto rimSize		{fibonacci(*rimIndex_)};
		    auto width__		{static_cast<float>(*width_) / rimSize};
		    Blend<LED<>> const blend	{black, *color_ / 4};
		    Dial const dial		{position};
		    HalfCurve const half	{0.0f, 1 & *rimIndex_};
		    BellCurve<> const bell	{0.0f, width__};

		    std::function<LED<>(float)> render {[](float){return LED<>();}};
		    switch (shape[i].value) {
			case Shape::Value::bell: {
			    render = [dial, half, bell, blend](float place) {
				float const offset {dial(place)};
				return blend(half(offset) * bell(offset));
			    };
			} break;
			case Shape::Value::wave: {
			    float const wavePosition {phaseIn(microsecondsSinceBoot,
				microsecondsPerSecond * rimSize)
			    };
			    float const waveWidth {2.0f / rimSize};
			    BellStandingWaveDial wave{position,
				width__,
				wavePosition,
				waveWidth};
			    render = [dial, half, wave, blend](float place) {
				float const offset {dial(place)};
				return blend(half(offset) * wave(place));
			    };
			} break;
			case Shape::Value::bloom: {
			    BumpCurve bump{0.0f, width__};
			    BloomCurve bloom{0.0f, width__,
				phaseIn(microsecondsSinceBoot,
				    microsecondsPerSecond << 1)};
			    render = [dial, half, bump, bloom, blend](float place) {
				float const offset {dial(place)};
				return blend(half(offset) * bump(offset) * bloom(offset));
			    };
			} break;
		    }

		    auto const * kp {*rim_};
		    for (auto j {0u}; j < rimSize; ++j) {
			auto const k {*kp++};
			auto const place {static_cast<float>(j) / rimSize};
			LED<> addend = render(place);
			for (auto const & l: Path{*rimEnd_ - 1u - k, rimSize}) {
			    uint32_t & encoding {message1.encodings[layout[l]]};
			    encoding = LED<>(encoding) + addend;
			}
		    }
		}

		++width_;
		++color_;
		++rim_;
		++rimEnd_;
		++rimIndex_;
		++sawtooth_;
	    }
	    for (auto & e: message1.encodings) {
		e &= ~0 << 5 | 1;	// scale by 1/31
	    }
	} break;
	case Mode::Value::swirl: {
	    constexpr auto levelEnd	{64.0f};	// [0, levelEnd)
	    constexpr auto radiusMax	{1.5f};
	    constexpr auto iBegin	{7u};	// first (fibonacci(i)) swirl
	    constexpr auto iCount	{6u};	// number of swirls in cycle
	    constexpr auto iSeconds	{60u};	// covers one swirl in cycle
	    constexpr auto zSeconds	{8u};	// covers perlin noise period

	    float const z {((microsecondsSinceBoot / zSeconds) % perlinNoisePeriodMicroseconds)
		/ static_cast<float>(microsecondsPerSecond)};

	    Contrast const radiusContrast {10.0f};
	    float const r {radiusMax * radiusContrast(perlinNoise[3].noise0_1(z))};

	    // ramp up by even offsets for first half, then ramp down by odds.
	    // this results in each half having swirls going in the same direction.
	    auto const iEven {2u * static_cast<unsigned>(
		((microsecondsSinceBoot / iSeconds) % (iCount * microsecondsPerSecond))
		    / static_cast<float>(microsecondsPerSecond)
	    )};
	    auto const i {static_cast<unsigned>(iBegin +
		(iCount > iEven
		    ? iEven
		    : 2 * iCount - iEven - 1)
	    )};
	    auto const n {fibonacci(i)};

	    auto kp {rim1024[i]};
	    for (auto j = 0; j < n; ++j) {
		auto k {*kp++};
		float const a {tau * j / n};
		float const x {r * std::cos(a)};
		float const y {r * std::sin(a)};
		LED<> led {
		    static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[0].noise0_1(x, y, z)), 0.0f)),
		    static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[1].noise0_1(x, y, z)), 0.0f)),
		    static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[2].noise0_1(x, y, z)), 0.0f))
		};
		led.part.control = ~0 << 5 | 1;	// scale by 1/31
		for (auto const & l: Path{ledCount - 1 - k, n}) {
		    message1.encodings[layout[l]] = led;
		}
	    }
	} break;
	default: {
	    constexpr auto levelEnd {64.0f};	// [0, levelEnd)
	    float const x {(microsecondsSinceBoot % perlinNoisePeriodMicroseconds)
		/ static_cast<float>(microsecondsPerSecond)};
	    LED<> led {
		static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[0].noise0_1(x)), 0.0f)),
		static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[1].noise0_1(x)), 0.0f)),
		static_cast<uint8_t>(levelEnd * std::nextafter(levelContrast(perlinNoise[2].noise0_1(x)), 0.0f))
	    };
	    led.part.control = ~0 << 5 | 1;	// scale by 1/31
	    for (auto & e: message1.encodings) {
		e = led;
	    }
	} break;
    }

    // SPI::Transaction constructor queues the message.
    // SPI::Transaction destructor waits for result.
    {
	SPI::Transaction transaction0(spiDevice[0], SPI::Transaction::Config()
	    .tx_buffer_(&message0)
	    .length_(message0.length()));
	SPI::Transaction transaction1(spiDevice[1], SPI::Transaction::Config()
	    .tx_buffer_(&message1)
	    .length_(message1.length()));
    }
}

void GoldenArtTask::update() {
    // if the rate at which we complete work
    // is less than the periodic demand for such work
    // the work queue will eventually exhaust all memory.
    if (updated++) {
	// catch up to the rate of demand
	ESP_LOGW(name, "update rate too fast. %u", updated - 1);
	return;
    }
    update_();
    // ignore any queued update work
    io.poll();
    updated = 0;
}

GoldenArtTask::GoldenArtTask(
    KeyValueBroker &	keyValueBroker)
:
    AsioTask		{"GoldenArtTask", 5, 0x10000, 1},
    TimePreferences	{io, keyValueBroker, 512},
    DialPreferences	{io, keyValueBroker},

    tinyPicoLedPower {GPIO_NUM_13, GPIO_MODE_OUTPUT},

    spiBus {
	{HSPI_HOST, SPI::Bus::Config()	// tinyPico on-oboard LED
	    .mosi_io_num_(2)
	    .sclk_io_num_(12),
	0},
	{VSPI_HOST, SPI::Bus::Config()
	    .mosi_io_num_(SPI::Bus::VspiConfig.mosi_io_num)
	    .sclk_io_num_(SPI::Bus::VspiConfig.sclk_io_num)
	    .max_transfer_sz_(APA102::messageBits(ledCount)),
	1},
    },

    spiDevice {
	{&spiBus[0], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_( 100000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)		// no chip select
	    .queue_size_(1)
	},
	{&spiBus[1], SPI::Device::Config()
	    .mode_(APA102::spiMode)
	    .clock_speed_hz_(8000000)	// see SPI_MASTER_FREQ_*
	    .spics_io_num_(-1)		// no chip select
	    .queue_size_(1)
	},
    },

    mode(Mode::clock),
    modeObserver(keyValueBroker, "mode", mode.toString(),
	[this](char const * value){
	    Mode mode_(value);
	    io.post([this, mode_](){
		mode = mode_;
	    });
	}),

    updated(0)
{
    tinyPicoLedPower.set_level(0);	// high side switch, low (0) turns it on
}

void GoldenArtTask::run() {
    // asio timers are not supported
    // adapt a FreeRTOS timer to post timeout to this task.
    Timer updateTimer(name, 4, true, [this](){
	io.post([this](){
	    this->update();
	});
    });

    updateTimer.start();

    // create some dummy work ...
    asio::io_service::work work(io);

    // ... so that we will run forever
    AsioTask::run();
}

GoldenArtTask::~GoldenArtTask() {
    stop();
    ESP_LOGI(name, "~GoldenArtTask");
}
