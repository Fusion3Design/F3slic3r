///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_OPTIONTEMPLATE_HPP
#define VGCODE_OPTIONTEMPLATE_HPP

#include <cstdint>

namespace libvgcode {

class OptionTemplate
{
public:
    OptionTemplate() = default;
    ~OptionTemplate();
    OptionTemplate(const OptionTemplate& other) = delete;
    OptionTemplate(OptionTemplate&& other) = delete;
    OptionTemplate& operator = (const OptionTemplate& other) = delete;
    OptionTemplate& operator = (OptionTemplate&& other) = delete;

    void init(uint8_t resolution);
    void render(size_t count);

private:
    uint8_t m_resolution{ 0 };
    uint8_t m_vertices_count{ 0 };
    unsigned int m_top_vao_id{ 0 };
    unsigned int m_top_vbo_id{ 0 };
    unsigned int m_bottom_vao_id{ 0 };
    unsigned int m_bottom_vbo_id{ 0 };
};

} // namespace libvgcode

#endif // VGCODE_OPTIONTEMPLATE_HPP