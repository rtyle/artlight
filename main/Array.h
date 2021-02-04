#pragma once

/// Like std::array, Array describes an array by its size and data.
/// Unlike std::array, an array of ragged Array objects can be created
/// because their can be a common_type (the types don't depend on their sizes).
/// Given an array, the constructor can deduce its size and point to the data.
template <typename T>
struct Array {
    template <std::size_t size__>
    static constexpr std::size_t size_(T (&)[size__]) {return size__;}

    std::size_t const size;
    T * const data;

    template <std::size_t size_>
    constexpr Array(T (&data_)[size_]) : size{size_}, data{data_} {}
};

