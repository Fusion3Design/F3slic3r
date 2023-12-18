///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_BITSET_HPP
#define VGCODE_BITSET_HPP

#include <atomic>
#include <vector>

namespace libvgcode {

// By default, types are not atomic,
template<typename T> auto constexpr is_atomic = false;

// but std::atomic<T> types are,
template<typename T> auto constexpr is_atomic<std::atomic<T>> = true;

template<typename T = unsigned long long>
struct BitSet
{
    BitSet() = default;
    BitSet(unsigned int size) : size(size), blocks(1 + (size / (sizeof(T) * 8))) { clear(); }

    void clear() {
        for (size_t i = 0; i < blocks.size(); ++i) {
            blocks[i] &= T(0);
        }
    }

    void setAll() {
        for (size_t i = 0; i < blocks.size(); ++i) {
            blocks[i] |= ~T(0);
        }
    }

    //return true if bit changed
    bool set(unsigned int index) {
        const auto [block_idx, bit_idx] = get_coords(index);
        T mask = (T(1) << bit_idx);
        bool flip = mask xor blocks[block_idx];
        blocks[block_idx] |= mask;
        return flip;
    }

    //return true if bit changed
    bool reset(unsigned int index) {
        const auto [block_idx, bit_idx] = get_coords(index);
        T mask = (T(1) << bit_idx);
        bool flip = mask xor blocks[block_idx];
        blocks[block_idx] &= (~mask);
        return flip;
    }

    bool operator [] (unsigned int index) const {
        const auto [block_idx, bit_idx] = get_coords(index);
        return ((blocks[block_idx] >> bit_idx) & 1) != 0;
    }

    template<typename U>
    BitSet& operator &= (const BitSet<U>& other) {
        static_assert(sizeof(T) == sizeof(U), "Type1 and Type2 must be of the same size.");
        for (size_t i = 0; i < blocks.size(); ++i) {
            blocks[i] &= other.blocks[i];
        }
        return *this;
    }

    // Atomic set operation (enabled only for atomic types), return true if bit changed
    template<typename U = T>
    inline typename std::enable_if<is_atomic<U>, bool>::type set_atomic(unsigned int index) {
        const auto [block_idx, bit_idx] = get_coords(index);
        T mask = static_cast<T>(1) << bit_idx;
        T oldval = blocks[block_idx].fetch_or(mask, std::memory_order_relaxed);
        return oldval xor (oldval or mask);
    }

    // Atomic reset operation (enabled only for atomic types), return true if bit changed
    template<typename U = T>
    inline typename std::enable_if<is_atomic<U>, bool>::type reset_atomic(unsigned int index) {
        const auto [block_idx, bit_idx] = get_coords(index);
        T mask = ~(static_cast<T>(1) << bit_idx);
        T oldval = blocks[block_idx].fetch_and(mask, std::memory_order_relaxed);
        return oldval xor (oldval and mask);
    }

    std::pair<unsigned int, unsigned int> get_coords(unsigned int index) const
    {
        unsigned int block_idx = index / (sizeof(T) * 8);
        unsigned int bit_idx = index % (sizeof(T) * 8);
        return std::make_pair(block_idx, bit_idx);
    }

    unsigned int   size{ 0 };
    std::vector<T> blocks{ 0 };
};

} // namespace libvgcode

#endif // VGCODE_BITSET_HPP