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
InRing::pointer InRing::operator->() const {return &inRing;}
InRing::~InRing() {}

OrdinalsInRing::OrdinalsInRing(size_t size) : InRing(size, 1) {inRing[0] = 0.0f;}
InRing & OrdinalsInRing::operator++(int) {
    inRing[0] = static_cast<float>(++ordinal) / size;
    return *this;
}

FoldsInRing::FoldsInRing(size_t folds_, size_t foldedSize_, size_t unfoldedSize_)
:
    InRing	(folds_ * (1 + foldedSize_ + unfoldedSize_), 2),
    folds	(folds_),
    foldedSize	(foldedSize_),
    unfoldedSize(unfoldedSize_),
    ringSize	(folds * (1 + 2 * foldedSize + unfoldedSize)),
    folding	(0),
    unfolding	(0),
    upOnRing	(0),
    downOnRing	(ringSize)
{}
InRing & FoldsInRing::operator++(int) {
    ++ordinal;
    inRing.clear();
    if (folding) {
	inRing.push_back(static_cast<float>(upOnRing) / ringSize);
	inRing.push_back(static_cast<float>(--downOnRing) / ringSize);
	--folding;
    } else if (unfolding) {
	inRing.push_back(static_cast<float>(upOnRing) / ringSize);
	--unfolding;
    } else {
	if (upOnRing) {
	    upOnRing	+= foldedSize;
	    downOnRing	 = upOnRing;
	}
	float upInRing = static_cast<float>(upOnRing) / ringSize;
	inRing.push_back(upInRing);
	inRing.push_back(upInRing);
	folding		= foldedSize;
	unfolding	= unfoldedSize;
    }
    ++upOnRing;
    return *this;
}

SectorsInRing::SectorsInRing(size_t sectors_, size_t const * sectorSize_)
:
	InRing		([](size_t count, size_t const * addend){
			    size_t sum = 0;
			    while (count--) sum += *addend++;
			    return sum;
			}(sectors_, sectorSize_), 1),
	sectors		(sectors_),
	sectorSize	(sectorSize_),
	sector		(0),
	onSector	(0)
{}
InRing & SectorsInRing::operator++(int) {
    ++ordinal;
    while (onSector == *sectorSize) {
	++sector;
	onSector = 0;
	++sectorSize;
    }
    inRing[0] =
	(sector + static_cast<float>(onSector) / *sectorSize) / sectors;
    ++onSector;
    return *this;
}
