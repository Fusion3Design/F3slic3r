cmake_minimum_required(VERSION 2.8.12)
project(libvgcode)

add_library(libvgcode STATIC
	# API
    include/ColorRange.hpp
    include/GCodeInputData.hpp
    include/PathVertex.hpp
    include/Types.hpp
    include/Viewer.hpp
	# source
	src/Bitset.hpp
	src/Bitset.cpp
	src/CogMarker.hpp
	src/CogMarker.cpp
	src/ColorRange.cpp
	src/ExtrusionRoles.hpp
	src/ExtrusionRoles.cpp
	src/GCodeInputData.cpp
	src/Layers.hpp
	src/Layers.cpp
	src/OpenGLUtils.hpp
	src/OpenGLUtils.cpp
	src/OptionTemplate.hpp
	src/OptionTemplate.cpp
	src/PathVertex.cpp
	src/Range.hpp
	src/Range.cpp
	src/SegmentTemplate.hpp
	src/SegmentTemplate.cpp
	src/Settings.hpp
	src/Settings.cpp
	src/Shaders.hpp
	src/ShadersES.hpp
	src/ToolMarker.hpp
	src/ToolMarker.cpp
	src/Types.cpp
	src/Utils.hpp
	src/Utils.cpp
	src/Viewer.cpp
	src/ViewerImpl.hpp
	src/ViewerImpl.cpp
	src/ViewRange.hpp
	src/ViewRange.cpp
	# glad
	glad/include/glad/glad.h
	glad/include/KHR/khrplatform.h
	glad/src/glad.c
)

# glad includes
include_directories(glad/include)