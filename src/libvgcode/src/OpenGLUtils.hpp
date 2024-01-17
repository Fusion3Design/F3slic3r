///|/ Copyright (c) Prusa Research 2023 Enrico Turri @enricoturri1966, Pavel Mikuš @Godrak, Vojtěch Bubník @bubnikv
///|/
///|/ libvgcode is released under the terms of the AGPLv3 or higher
///|/
#ifndef VGCODE_OPENGLUTILS_HPP
#define VGCODE_OPENGLUTILS_HPP

// OpenGL loader
#include "../glad/include/glad/glad.h"

namespace libvgcode {
#ifndef NDEBUG
#define HAS_GLSAFE
#endif // NDEBUG

#ifdef HAS_GLSAFE
extern void glAssertRecentCallImpl(const char* file_name, unsigned int line, const char* function_name);
inline void glAssertRecentCall() { glAssertRecentCallImpl(__FILE__, __LINE__, __FUNCTION__); }
#define glsafe(cmd) do { cmd; glAssertRecentCallImpl(__FILE__, __LINE__, __FUNCTION__); } while (false)
#define glcheck() do { glAssertRecentCallImpl(__FILE__, __LINE__, __FUNCTION__); } while (false)
#else
inline void glAssertRecentCall() { }
#define glsafe(cmd) cmd
#define glcheck()
#endif // HAS_GLSAFE

class OpenGLWrapper
{
public:
    OpenGLWrapper() = delete;

    static bool load_opengl();
    static bool is_detected() { return s_detected; }
    static bool is_valid() { return s_valid; }
    static bool is_es() { return s_es; }

private:
    static bool s_detected;
    static bool s_valid;
    static bool s_es;
};

} // namespace libvgcode

#endif // VGCODE_OPENGLUTILS_HPP