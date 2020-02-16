#include "InRing.h"

InRing::InRing(
    size_t	size_,
    size_t	inRingSize)
:
    size	(size_),
    ordinal	(0),
    inRing	(inRingSize)
{}
InRing::operator bool() const {return ordinal < size;}
InRing::reference InRing::operator*() const {return inRing;}
InRing::~InRing() {}

OrdinalsInRing::OrdinalsInRing(size_t size) : InRing(size, 1) {inRing[0] = 0.0f;}
InRing & OrdinalsInRing::operator++() {
    inRing[0] = static_cast<float>(++ordinal) / size;
    return *this;
}

SectorsInRing::SectorsInRing(size_t sectors_, size_t const * sectorSize_)
:
    InRing	([](size_t count, size_t const * value){
		    size_t result = 0;
		    while (count--) result += *value++;
		    return result;
		}(sectors_, sectorSize_), 1),
    sectors	(sectors_),
    sectorSize	(sectorSize_),
    sector	(0),
    onSector	(0)
{
    inRing[0] = 0.0f;
}
InRing & SectorsInRing::operator++() {
    ++ordinal;
    ++onSector;
    while (onSector == *sectorSize) {
	++sectorSize;
	++sector;
	onSector = 0;
    }
    inRing[0] = (sector + static_cast<float>(onSector) / *sectorSize) / sectors;
    return *this;
}

FoldsInRing::FoldsInRing(size_t sectors_, size_t const * foldedSize_, size_t const * unfoldedSize_)
:
    InRing	([](size_t count, size_t const * value0, size_t const * value1){
		    size_t result = 0;
		    while (count--) result += *value0++ + *value1++;
		    return result;
		}(sectors_, foldedSize_, unfoldedSize_), 2),
    sectors	(sectors_),
    foldedSize	(foldedSize_),
    unfoldedSize(unfoldedSize_),
    foldedSizeSum([](size_t count, size_t const * value){
		    size_t result = 0;
		    while (count--) result += *value++;
		    return result;
		}(sectors, foldedSize)),
    unfoldedSizeSum([](size_t count, size_t const * value){
		    size_t result = 0;
		    while (count--) result += *value++;
		    return result;
		}(sectors, unfoldedSize)),
    foldedPart	(2.0f * foldedSizeSum / (2.0f * foldedSizeSum + unfoldedSizeSum)),
    unfoldedPart(1.0f - foldedPart),
    sector	(0),
    onSector	(0)
{
    inRing.clear();
    // assume 0 < *foldedSize
    float place = (foldedPart * (0.5f / *foldedSize / 2.0f)) / sectors;
    inRing.push_back(place);
    inRing.push_back(1.0f - place);
}
InRing & FoldsInRing::operator++() {
    inRing.clear();
    ++ordinal;
    ++onSector;
    while (onSector == *foldedSize + *unfoldedSize) {
	++foldedSize;
	++unfoldedSize;
	++sector;
	onSector = 0;
    }
    if (onSector < *foldedSize) {
	float place = foldedPart * ((0.5f +  onSector) / *foldedSize / 2.0f);
	inRing.push_back((sector + place) / sectors);
	inRing.push_back(((sector ? sector : sectors) - place) / sectors);
    } else {
	inRing.push_back((sector
	    + (foldedPart + unfoldedPart) / 2.0f
	    + unfoldedPart * (((onSector - *foldedSize) - (*unfoldedSize / 2.0f - 0.5f)) / *unfoldedSize)
	) / sectors);
    }
    return *this;
}
