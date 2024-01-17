///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak, Vojtěch Bubník @bubnikv
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "OpenGLUtils.hpp"

#include <iostream>
#include <assert.h>
#include <cctype>
#include <stdio.h>

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

static const std::string OPENGL_ES_STR = "OpenGL ES";

bool OpenGLWrapper::s_detected = false;
bool OpenGLWrapper::s_valid = false;
bool OpenGLWrapper::s_es = false;

bool OpenGLWrapper::load_opengl()
{
    if (gladLoadGL() == 0)
        return false;

    s_detected = true;

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    if (version == nullptr)
        return false;

    std::string version_str(version);
    const size_t pos = version_str.find(OPENGL_ES_STR.c_str());
    if (pos == 0) {
        s_es = true;
        version_str = version_str.substr(OPENGL_ES_STR.length() + 1);
    }
    GLint major = 0;
    GLint minor = 0;
    const int res = sscanf(version_str.c_str(), "%d.%d", &major, &minor);
    if (res != 2)
        return false;

    s_valid = s_es ? major > 2 || (major == 2 && minor >= 0) : major > 3 || (major == 3 && minor >= 2);
    return s_valid;
}

} // namespace libvgcode
