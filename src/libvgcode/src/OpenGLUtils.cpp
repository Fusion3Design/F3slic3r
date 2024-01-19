///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak, Vojtěch Bubník @bubnikv
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "OpenGLUtils.hpp"

#include <iostream>
#include <assert.h>
#include <cctype>
#include <stdio.h>
#include <cstring>

namespace libvgcode {

#ifdef HAS_GLSAFE
void glAssertRecentCallImpl(const char* file_name, unsigned int line, const char* function_name)
{
    const GLenum err = glGetError();
    if (err == GL_NO_ERROR)
        return;
    const char* sErr = 0;
    switch (err) {
    case GL_INVALID_ENUM:      { sErr = "Invalid Enum"; break; }
    case GL_INVALID_VALUE:     { sErr = "Invalid Value"; break; }
    // be aware that GL_INVALID_OPERATION is generated if glGetError is executed between the execution of glBegin / glEnd 
    case GL_INVALID_OPERATION: { sErr = "Invalid Operation"; break; }
    case GL_STACK_OVERFLOW:    { sErr = "Stack Overflow"; break; }
    case GL_STACK_UNDERFLOW:   { sErr = "Stack Underflow"; break; }
    case GL_OUT_OF_MEMORY:     { sErr = "Out Of Memory"; break; }
    default:                   { sErr = "Unknown"; break; }
    }
    std::cout << "OpenGL error in " << file_name << ":" << line << ", function " << function_name << "() : " << (int)err << " - " << sErr << "\n";
    assert(false);
}
#endif // HAS_GLSAFE

static const char* OPENGL_ES_PREFIXES[] = { "OpenGL ES-CM ", "OpenGL ES-CL ", "OpenGL ES ", nullptr };

bool OpenGLWrapper::load_opengl(const std::string& context_version)
{
    m_valid_context = false;
    m_opengl_es = false;

    const char* version = context_version.c_str();
    for (int i = 0; OPENGL_ES_PREFIXES[i] != nullptr; ++i) {
        const size_t length = strlen(OPENGL_ES_PREFIXES[i]);
        if (strncmp(version, OPENGL_ES_PREFIXES[i], length) == 0) {
            version += length;
            m_opengl_es = true;
            break;
        }
    }

    GLint major = 0;
    GLint minor = 0;
#ifdef _MSC_VER
    const int res = sscanf_s(version, "%d.%d", &major, &minor);
#else
    const int res = sscanf(version, "%d.%d", &major, &minor);
#endif // _MSC_VER
    if (res != 2)
        return false;

    m_valid_context = m_opengl_es ? major > 2 || (major == 2 && minor >= 0) : major > 3 || (major == 3 && minor >= 2);

    const int glad_res = gladLoadGL();
    if (glad_res == 0)
        return false;

    return m_valid_context;
}

} // namespace libvgcode
