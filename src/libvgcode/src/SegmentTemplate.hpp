///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_SEGMENTTEMPLATE_HPP
#define VGCODE_SEGMENTTEMPLATE_HPP

namespace libvgcode {

class SegmentTemplate
{
public:
    SegmentTemplate() = default;
    ~SegmentTemplate();
    SegmentTemplate(const SegmentTemplate& other) = delete;
    SegmentTemplate(SegmentTemplate&& other) = delete;
    SegmentTemplate& operator = (const SegmentTemplate& other) = delete;
    SegmentTemplate& operator = (SegmentTemplate&& other) = delete;

    void init();
    void render(size_t count);

private:
    unsigned int m_vao_id{ 0 };
    unsigned int m_vbo_id{ 0 };
};

} // namespace libvgcode

#endif // VGCODE_SEGMENTTEMPLATE_HPP