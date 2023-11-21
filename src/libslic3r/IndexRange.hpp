#ifndef slic3r_IndexRange_hpp_
#define slic3r_IndexRange_hpp_

#include <cstdint>
#include <cassert>

namespace Slic3r {
// Range of indices, providing support for range based loops.
template<typename T>
class IndexRange
{
public:
    IndexRange(T ibegin, T iend) : m_begin(ibegin), m_end(iend) {}
    IndexRange() = default;

    // Just a bare minimum functionality iterator required by range-for loop.
    class Iterator {
    public:
        T    operator*() const { return m_idx; }
        bool operator!=(const Iterator &rhs) const { return m_idx != rhs.m_idx; }
        void operator++() { ++ m_idx; }
    private:
        friend class IndexRange<T>;
        Iterator(T idx) : m_idx(idx) {}
        T m_idx;
    };

    Iterator begin()  const { assert(m_begin <= m_end); return Iterator(m_begin); };
    Iterator end()    const { assert(m_begin <= m_end); return Iterator(m_end); };

    bool     empty()  const { assert(m_begin <= m_end); return m_begin >= m_end; }
    T        size()   const { assert(m_begin <= m_end); return m_end - m_begin; }

private:
    // Index of the first extrusion in LayerRegion.
    T    m_begin   { 0 };
    // Index of the last extrusion in LayerRegion.
    T    m_end     { 0 };
};

template class IndexRange<uint32_t>;
} // Slic3r

#endif // slic3r_IndexRange_hpp_
