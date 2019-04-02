#pragma once

// http://gabisoft.free.fr/articles/fltrsbf1.html

#include <streambuf>

template <class Extractor>
class InputFilter : public std::streambuf {
private:
    std::streambuf &	source;
    Extractor		extractor;
    char		buffer;
public:
    InputFilter(
	std::streambuf &	source_,
	Extractor &&		extractor_)
    :
	source			(source_),
	extractor		(extractor_),
	buffer			(0)
    {}

    int overflow(int) override {return traits_type::eof();}

    int underflow() override {
	if (gptr() < egptr())
	    return *gptr();
	else {
	    int	result = extractor(source);
	    if (traits_type::eof() == result) return result;
	    buffer = result;
	    setg(&buffer, &buffer, &buffer + 1);
	    return result;
	}
    }
};
