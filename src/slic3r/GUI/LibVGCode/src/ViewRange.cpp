//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "ViewRange.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

namespace libvgcode {

const Interval& ViewRange::get_full() const
{
    return m_full.get();
}

void ViewRange::set_full(const Range& other)
{
    set_full(other.get());
}

void ViewRange::set_full(const Interval& range)
{
    set_full(range[0], range[1]);
}

void ViewRange::set_full(Interval::value_type min, Interval::value_type max)
{
    // is the full range being extended ?
    const bool new_max = max > m_full.get_max();
    m_full.set(min, max);
    // force the enabled range to stay inside the modified full range
    m_full.clamp(m_enabled);
    // force the visible range to stay inside the modified enabled range
    m_enabled.clamp(m_visible);
    if (new_max)
        // force the enabled range to fill the extended full range
        m_enabled.set_max(max);
}

const Interval& ViewRange::get_enabled() const
{
    return m_enabled.get();
}

void ViewRange::set_enabled(const Range& other)
{
    set_enabled(other.get());
}

void ViewRange::set_enabled(const Interval& range)
{
    set_enabled(range[0], range[1]);
}

void ViewRange::set_enabled(Interval::value_type min, Interval::value_type max)
{
    // is the enabled range being extended ?
    const bool new_max = max > m_enabled.get_max();
    m_enabled.set(min, max);
    // force the visible range to stay inside the modified enabled range
    m_enabled.clamp(m_visible);
    if (new_max)
        // force the visible range to fill the extended enabled range
        m_visible.set_max(max);
}

const Interval& ViewRange::get_visible() const
{
    return m_visible.get();
}

void ViewRange::set_visible(const Range& other)
{
    set_visible(other.get());
}

void ViewRange::set_visible(const Interval& range)
{
    set_visible(range[0], range[1]);
}

void ViewRange::set_visible(Interval::value_type min, Interval::value_type max)
{
    m_visible.set(min, max);
    // force the visible range to stay inside the enabled range
    m_enabled.clamp(m_visible);
}

void ViewRange::reset()
{
    m_full.reset();
    m_enabled.reset();
    m_visible.reset();
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
