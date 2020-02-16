#pragma once

#include <iterator>
#include <vector>

/// A Class derived from InRing is an input iterator
/// that generates a sequence of std::vector<float> values
/// whose elements reflect the positions in a ring
/// according to the place in the sequence.
class InRing
    : public std::iterator<std::input_iterator_tag, std::vector<float>> {
protected:
    size_t const	size;
    size_t		ordinal;
    std::vector<float>	inRing;
public:
    using iterator_category	= std::input_iterator_tag;
    using value_type		= std::vector<float>;
    using reference		= value_type const &;
    using pointer		= value_type const *;
    using difference_type	= ptrdiff_t;
    InRing(size_t size, size_t inRingSize = 0);
    explicit operator bool() const;
    reference operator*() const;
    virtual InRing & operator++() = 0;
    virtual ~InRing();
};

/// OrdinalsInRing is an input iterator (based on InRing)
/// that generates ring positions for ordinals
/// that are evenly spaced in the ring.
class OrdinalsInRing : public InRing {
public:
    OrdinalsInRing(size_t size);
    InRing & operator++() override;
};

/// SectorsInRing is an input iterator (based on InRing)
/// that generates ring positions for ordinals according to the sector
/// that they are in.
/// Sectors may be unequally sized.
class SectorsInRing : public InRing {
private:
    size_t const	sectors;
    size_t const *	sectorSize;
    size_t		sector;
    size_t		onSector;
public:
    SectorsInRing(size_t sectors, size_t const * sectorSize);
    InRing & operator++();
};

/// FoldsInRing is an input iterator (based on InRing)
/// that generates ring positions for ordinals
/// according to where they are with respect to evenly spaced folds.
/// On creases, the same position is given twice.
/// Off the crease but in a fold, the two positions that are folded together
/// are given.
/// Otherwise, the single position outside the fold is given.
class FoldsInRing : public InRing {
private:
    size_t const	sectors;
    size_t const *	foldedSize;
    size_t const *	unfoldedSize;
    size_t const	foldedSizeSum;
    size_t const	unfoldedSizeSum;
    float const		foldedPart;
    float const		unfoldedPart;
    size_t		sector;
    size_t		onSector;
public:
    FoldsInRing(size_t folds, size_t const * foldedSize, size_t const * unfoldedSize);
    InRing & operator++() override;
};
