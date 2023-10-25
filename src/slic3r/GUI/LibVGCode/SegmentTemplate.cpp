//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "SegmentTemplate.hpp"
#include "OpenGLUtils.hpp"

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

#include <cstdint>
#include <array>

namespace libvgcode {

//     /1-------6\    
//    / |       | \  
//   2--0-------5--7
//    \ |       | /  
//      3-------4    
static constexpr const std::array<uint8_t, 24> VERTEX_DATA = {
    0, 1, 2, // front spike
    0, 2, 3, // front spike
    0, 3, 4, // right/bottom body 
    0, 4, 5, // right/bottom body 
    0, 5, 6, // left/top body 
    0, 6, 1, // left/top body 
    5, 4, 7, // back spike
    5, 7, 6, // back spike
};

SegmentTemplate::~SegmentTemplate()
{
    if (m_vbo_id != 0)
        glsafe(glDeleteBuffers(1, &m_vbo_id));

    if (m_vao_id != 0)
        glsafe(glDeleteVertexArrays(1, &m_vao_id));
}

void SegmentTemplate::init()
{
    if (m_vao_id != 0)
        return;

    int curr_vertex_array;
    glsafe(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &curr_vertex_array));
    int curr_array_buffer;
    glsafe(glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &curr_array_buffer));

    glsafe(glGenVertexArrays(1, &m_vao_id));
    glsafe(glBindVertexArray(m_vao_id));

    glsafe(glGenBuffers(1, &m_vbo_id));
    glsafe(glBindBuffer(GL_ARRAY_BUFFER, m_vbo_id));
    glsafe(glBufferData(GL_ARRAY_BUFFER, VERTEX_DATA.size() * sizeof(uint8_t), VERTEX_DATA.data(), GL_STATIC_DRAW));
    glsafe(glEnableVertexAttribArray(0));
    glsafe(glVertexAttribIPointer(0, 1, GL_UNSIGNED_BYTE, sizeof(uint8_t), (const void*)0));

    glsafe(glBindBuffer(GL_ARRAY_BUFFER, curr_array_buffer));
    glsafe(glBindVertexArray(curr_vertex_array));
}

void SegmentTemplate::render(size_t count)
{
    if (m_vao_id == 0 || m_vbo_id == 0 || count == 0)
        return;

    int curr_vertex_array;
    glsafe(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &curr_vertex_array));

    glsafe(glBindVertexArray(m_vao_id));
    glsafe(glDrawArraysInstanced(GL_TRIANGLES, 0, VERTEX_DATA.size(), count));

    glsafe(glBindVertexArray(curr_vertex_array));
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
