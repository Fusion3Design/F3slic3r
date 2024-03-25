
if (MSVC)

    # Update the following variables if updating WebView2 SDK
    set(WEBVIEW2_VERSION "1.0.705.50")
    set(WEBVIEW2_URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${WEBVIEW2_VERSION}")
    set(WEBVIEW2_SHA256 "6a34bb553e18cfac7297b4031f3eac2558e439f8d16a45945c22945ac404105d")

    set(WEBVIEW2_DEFAULT_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/dep_WebView2-prefix/packages/Microsoft.Web.WebView2.${WEBVIEW2_VERSION}")
    set(WEBVIEW2_DOWNLOAD_DIR "${CMAKE_CURRENT_BINARY_DIR}/dep_WebView2-prefix/download")

    #message(STATUS "WEBVIEW2_DEFAULT_PACKAGE_DIR = ${WEBVIEW2_DEFAULT_PACKAGE_DIR}")

    if(NOT EXISTS ${WEBVIEW2_PACKAGE_DIR})
        unset(WEBVIEW2_PACKAGE_DIR CACHE)
    endif()
    
    set(WEBVIEW2_PACKAGE_DIR ${WEBVIEW2_DEFAULT_PACKAGE_DIR} CACHE PATH "WebView2 SDK PATH" FORCE)
    
    #file(MAKE_DIRECTORY ${DEP_DOWNLOAD_DIR}/WebView2)

    message(STATUS "WEBVIEW2_URL = ${WEBVIEW2_URL}")
    message(STATUS "WEBVIEW2_DOWNLOAD_DIR = ${WEBVIEW2_DOWNLOAD_DIR}")
    file(DOWNLOAD
        ${WEBVIEW2_URL}
        ${WEBVIEW2_DOWNLOAD_DIR}/WebView2.nuget
        EXPECTED_HASH SHA256=${WEBVIEW2_SHA256})

    file(MAKE_DIRECTORY ${WEBVIEW2_PACKAGE_DIR})

    execute_process(COMMAND
        ${CMAKE_COMMAND} -E tar x ${WEBVIEW2_DOWNLOAD_DIR}/WebView2.nuget
        WORKING_DIRECTORY ${WEBVIEW2_PACKAGE_DIR}
    )    

    set(_srcdir ${WEBVIEW2_PACKAGE_DIR}/build/native)
    set(_dstdir ${DESTDIR}/usr/local)

    set(_output  ${_dstdir}/include/WebView2.h
                 ${_dstdir}/bin/WebView2Loader.dll)

    if(NOT EXISTS ${_dstdir}/include)
        file(MAKE_DIRECTORY ${_dstdir}/include)
    endif()

    if(NOT EXISTS ${_dstdir}/bin)
        file(MAKE_DIRECTORY ${_dstdir}/bin)
    endif()

    add_custom_command(
        OUTPUT  ${_output}
        COMMAND ${CMAKE_COMMAND} -E copy ${_srcdir}/include/WebView2.h ${_dstdir}/include/
        COMMAND ${CMAKE_COMMAND} -E copy ${_srcdir}/x${DEPS_BITS}/WebView2Loader.dll ${_dstdir}/bin/
    )

    add_custom_target(dep_WebView2 SOURCES ${_output})

    set(WEBVIEW2_PACKAGE_DIR ${WEBVIEW2_PACKAGE_DIR} CACHE INTERNAL "" FORCE)

endif ()