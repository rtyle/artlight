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
    sector	(0),
    onSector	(0),
    onRingSize	(2 * *foldedSize + *unfoldedSize)
{
    inRing[0] = inRing[1] = 0.0f;
}
InRing & FoldsInRing::operator++() {
    inRing.clear();
    ++ordinal;
    ++onSector;
    while (onSector == *foldedSize + *unfoldedSize) {
	++foldedSize;
	++unfoldedSize;
	onRingSize = 2 * *foldedSize + *unfoldedSize;
	++sector;
	onSector = 0;
    }
    if (onSector < *foldedSize) {
	float place = (sector - static_cast<float>(onSector) / onRingSize) / sectors;
	inRing.push_back(0.0f > place ? 1.0f + place : place);
    }
    inRing.push_back((sector + static_cast<float>(onSector) / onRingSize) / sectors);
    return *this;
}
