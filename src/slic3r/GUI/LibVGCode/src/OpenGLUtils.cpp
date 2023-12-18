///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak, Vojtěch Bubník @bubnikv
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#include "OpenGLUtils.hpp"

#include <iostream>
#include <assert.h>
#include <cctype>

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

bool check_opengl_version()
{
    bool ret = false;
    const GLubyte* version = glGetString(GL_VERSION);
    if (version != nullptr) {
        const std::string version_str(reinterpret_cast<const char*>(version));
        if (version_str.length() > 4 && isdigit(version_str[0]) && isdigit(version_str[2])) {
            const int major = version_str[0] - '0';
            const int minor = version_str[2] - '0';
            ret = major > 3 || (major == 3 && minor >= 2);
        }
    }
    return ret;
}

} // namespace libvgcode
