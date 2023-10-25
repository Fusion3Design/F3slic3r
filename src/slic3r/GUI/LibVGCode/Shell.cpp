//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#include "libslic3r/Technologies.hpp"
//################################################################################################################################

///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "Shell.hpp"
#include "OpenGLUtils.hpp"
#include "Utils.hpp"

#include <algorithm>

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#if ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################

namespace libvgcode {

Shell::~Shell()
{
  if (m_ibo_id != 0)
      glsafe(glDeleteBuffers(1, &m_ibo_id));
  if (m_vbo_id != 0)
      glsafe(glDeleteBuffers(1, &m_vbo_id));
  if (m_vao_id != 0)
      glsafe(glDeleteVertexArrays(1, &m_vao_id));
}

void Shell::init()
{
    if (m_vao_id != 0)
        return;

}

void Shell::render()
{
    if (m_vao_id == 0 || m_vbo_id == 0 || m_ibo_id == 0)
        return;

    int curr_vertex_array;
    glsafe(glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &curr_vertex_array));
    glcheck();

    glsafe(glBindVertexArray(m_vao_id));
    glsafe(glDrawElements(GL_TRIANGLES, m_indices_count, GL_UNSIGNED_INT, (const void*)0));
    glsafe(glBindVertexArray(curr_vertex_array));
}

const Vec3f& Shell::get_position() const
{
    return m_position;
}

void Shell::set_position(const Vec3f& position)
{
    m_position = position;
}

const Color& Shell::get_color() const
{
    return m_color;
}

void Shell::set_color(const Color& color)
{
    m_color = color;
}

float Shell::get_alpha() const
{
    return m_alpha;
}

void Shell::set_alpha(float alpha)
{
    m_alpha = std::clamp(alpha, 0.25f, 0.75f);
}

} // namespace libvgcode

//################################################################################################################################
// PrusaSlicer development only -> !!!TO BE REMOVED!!!
#endif // ENABLE_NEW_GCODE_VIEWER
//################################################################################################################################
