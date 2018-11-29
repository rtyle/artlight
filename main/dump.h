#ifndef Dump_h_
#define Dump_h_

#include <codecvt>
#include <iomanip>
#include <sstream>

#include <esp_log.h>

template <typename T = unsigned char>
void dump(char const * name, char const * label, void const * base, size_t size) {
    size_t cardinality = (size + sizeof(T) - 1) / sizeof(T);
    T const * next = reinterpret_cast<T const *>(base);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
    while (cardinality) {
	std::ostringstream decoding;
	decoding << std::hex << std::setfill('0') << '+' << std::setw(4)
	    << next - static_cast<T const *>(base) << "\t";
	std::ostringstream print;
	for (size_t stride = 32 / sizeof(T); stride--;) {
	    if (cardinality) {
		decoding << std::setw(2 * sizeof(T))
		    << static_cast<unsigned int>(*next) << " ";
		if (std::iswcntrl(static_cast<std::wint_t>(*next))) {
		    print << '.';
		} else {
		    print << convert.to_bytes(static_cast<wchar_t>(*next));
		}
		--cardinality;
		++next;
	    } else {
		decoding << "   ";
	    }
	}
	ESP_LOGI(name, "%s %s%s", label, decoding.str().c_str(), print.str().c_str());
    }
}

#endif
