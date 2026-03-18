cmake_minimum_required(VERSION 3.21)

project(RayTracer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use Release by default for single-config generators to prioritize runtime performance.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Enable link-time optimization where the toolchain supports it.
include(CheckIPOSupported)
check_ipo_supported(RESULT RAYTRACER_IPO_SUPPORTED OUTPUT RAYTRACER_IPO_OUTPUT)
if(RAYTRACER_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
else()
    message(STATUS "IPO/LTO not supported: ${RAYTRACER_IPO_OUTPUT}")
endif()

option(RAYTRACER_USE_BUNDLED_DEPS "Use vendored third-party dependencies on Windows/Linux" ON)
set(RAYTRACER_THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party" CACHE PATH "Path to third-party dependencies")

set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(INC_DIR "${CMAKE_SOURCE_DIR}/include")

set(SRC_FILES "${CMAKE_SOURCE_DIR}/main.cpp")
file(GLOB_RECURSE RAYTRACER_CPP_SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
file(GLOB_RECURSE RAYTRACER_SCENE_INL_FILES CONFIGURE_DEPENDS "${SRC_DIR}/scenes/*.inl")
list(APPEND SRC_FILES ${RAYTRACER_CPP_SOURCES} ${RAYTRACER_SCENE_INL_FILES})

# Set default output directories before target creation.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

add_executable(RayTracer ${SRC_FILES})

target_include_directories(RayTracer PRIVATE "${INC_DIR}")

execute_process(
    COMMAND brew --prefix
    RESULT_VARIABLE BREW_RESULT
    OUTPUT_VARIABLE BREW_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

set(GLM_HINT_DIRS "/opt/homebrew/include" "/usr/local/include")
if(BREW_RESULT EQUAL 0)
    list(PREPEND GLM_HINT_DIRS "${BREW_PREFIX}/include")
endif()

if((WIN32 OR (UNIX AND NOT APPLE)) AND RAYTRACER_USE_BUNDLED_DEPS)
    set(GLM_INCLUDE_DIR "${RAYTRACER_THIRD_PARTY_DIR}/include")
    set(RT_BUNDLED_GLM_HEADER "${GLM_INCLUDE_DIR}/glm/glm.hpp")
    if(NOT EXISTS "${RT_BUNDLED_GLM_HEADER}")
        message(FATAL_ERROR
            "Missing bundled dependency file: ${RT_BUNDLED_GLM_HEADER}\n"
            "Restore third_party/include/glm or set RAYTRACER_THIRD_PARTY_DIR."
        )
    endif()
else()
    find_path(GLM_INCLUDE_DIR NAMES glm/glm.hpp HINTS ${GLM_HINT_DIRS})
    if(NOT GLM_INCLUDE_DIR)
        message(FATAL_ERROR
            "Could not find glm headers. Install glm and reconfigure.\n"
            "Set GLM_INCLUDE_DIR to the folder containing glm/glm.hpp."
        )
    endif()
endif()
target_include_directories(RayTracer PRIVATE "${GLM_INCLUDE_DIR}")

if(MSVC)
    target_compile_definitions(RayTracer PRIVATE _USE_MATH_DEFINES)
    target_compile_options(RayTracer PRIVATE
        /W3
        /wd4244
        /wd4996
        $<$<CONFIG:Release>:/O2 /Oi /Ot /Gy /Gw>
        $<$<CONFIG:RelWithDebInfo>:/O2 /Oi /Ot /Gy /Gw>
        $<$<CONFIG:MinSizeRel>:/O1 /Oi /Gy /Gw>
    )
    target_link_options(RayTracer PRIVATE
        $<$<CONFIG:Release>:/OPT:REF /OPT:ICF>
        $<$<CONFIG:RelWithDebInfo>:/OPT:REF /OPT:ICF>
        $<$<CONFIG:MinSizeRel>:/OPT:REF /OPT:ICF>
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(RayTracer PRIVATE
        -Wno-deprecated-register
        -Wno-deprecated-declarations
        -DGL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
        $<$<CONFIG:Release>:-O3 -DNDEBUG -ffunction-sections -fdata-sections>
        $<$<CONFIG:RelWithDebInfo>:-O3 -g -DNDEBUG -ffunction-sections -fdata-sections>
        $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG -ffunction-sections -fdata-sections>
        $<$<CONFIG:Debug>:-O0 -g3>
    )
    if(UNIX AND NOT APPLE)
        target_link_options(RayTracer PRIVATE
            -no-pie
            $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-Wl,--gc-sections>
        )
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(RayTracer PRIVATE
        -Wno-deprecated-register
        -Wno-deprecated-declarations
        -DGL_DO_NOT_WARN_IF_MULTI_GL_VERSION_HEADERS_INCLUDED
        $<$<CONFIG:Release>:-O3 -DNDEBUG -ffunction-sections -fdata-sections>
        $<$<CONFIG:RelWithDebInfo>:-O3 -g -DNDEBUG -ffunction-sections -fdata-sections>
        $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG -ffunction-sections -fdata-sections>
        $<$<CONFIG:Debug>:-O0 -g3>
    )
    if(APPLE)
        target_link_options(RayTracer PRIVATE
            $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-Wl,-dead_strip>
        )
    else()
        target_link_options(RayTracer PRIVATE
            $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:-Wl,--gc-sections>
        )
    endif()
endif()

if(WIN32)
    if(RAYTRACER_USE_BUNDLED_DEPS)
        set(RT_BUNDLED_INCLUDE_DIR "${RAYTRACER_THIRD_PARTY_DIR}/include")
        set(RT_WIN_LIB_DIR "${RAYTRACER_THIRD_PARTY_DIR}/lib/windows/x64")

        set(RT_REQUIRED_WIN_FILES
            "${RT_BUNDLED_INCLUDE_DIR}/GL/glew.h"
            "${RT_BUNDLED_INCLUDE_DIR}/GL/freeglut.h"
            "${RT_BUNDLED_INCLUDE_DIR}/glm/glm.hpp"
            "${RT_WIN_LIB_DIR}/glew32.lib"
            "${RT_WIN_LIB_DIR}/freeglut.lib"
            "${RT_WIN_LIB_DIR}/FreeImage.lib"
            "${RT_WIN_LIB_DIR}/glew32.dll"
            "${RT_WIN_LIB_DIR}/freeglut.dll"
            "${RT_WIN_LIB_DIR}/FreeImage.dll"
        )
        foreach(required_file IN LISTS RT_REQUIRED_WIN_FILES)
            if(NOT EXISTS "${required_file}")
                message(FATAL_ERROR
                    "Missing bundled dependency file: ${required_file}\n"
                    "Set RAYTRACER_THIRD_PARTY_DIR or disable RAYTRACER_USE_BUNDLED_DEPS."
                )
            endif()
        endforeach()

        target_include_directories(RayTracer PRIVATE "${RT_BUNDLED_INCLUDE_DIR}")

        target_link_libraries(RayTracer PRIVATE
            opengl32
            "${RT_WIN_LIB_DIR}/glew32.lib"
            "${RT_WIN_LIB_DIR}/freeglut.lib"
            "${RT_WIN_LIB_DIR}/FreeImage.lib"
        )

        add_custom_command(TARGET RayTracer POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${RT_WIN_LIB_DIR}/glew32.dll"
                "${RT_WIN_LIB_DIR}/freeglut.dll"
                "${RT_WIN_LIB_DIR}/FreeImage.dll"
                "$<TARGET_FILE_DIR:RayTracer>"
            VERBATIM
        )
    else()
        find_package(OpenGL REQUIRED)
        find_package(GLEW REQUIRED)
        find_package(GLUT REQUIRED)
        find_path(FREEIMAGE_INCLUDE_DIR NAMES FreeImage.h REQUIRED)
        find_library(FREEIMAGE_LIBRARY NAMES FreeImage freeimage REQUIRED)

        target_include_directories(RayTracer PRIVATE "${FREEIMAGE_INCLUDE_DIR}")
        target_link_libraries(RayTracer PRIVATE OpenGL::GL GLEW::GLEW GLUT::GLUT "${FREEIMAGE_LIBRARY}")
    endif()
elseif(APPLE)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    find_library(GLUT_FRAMEWORK GLUT REQUIRED)

    set(FREEIMAGE_HINT_DIRS "/opt/homebrew/lib" "/usr/local/lib")
    if(BREW_RESULT EQUAL 0)
        list(PREPEND FREEIMAGE_HINT_DIRS "${BREW_PREFIX}/lib")
    endif()

    find_library(FREEIMAGE_LIBRARY NAMES FreeImage freeimage HINTS ${FREEIMAGE_HINT_DIRS})
    if(NOT FREEIMAGE_LIBRARY)
        message(FATAL_ERROR
            "Could not find FreeImage on macOS.\n"
            "Install it with Homebrew (brew install freeimage), then reconfigure."
        )
    endif()

    target_link_libraries(RayTracer PRIVATE "${OPENGL_FRAMEWORK}" "${GLUT_FRAMEWORK}" "${FREEIMAGE_LIBRARY}")
else()
    if(RAYTRACER_USE_BUNDLED_DEPS)
        set(RT_BUNDLED_INCLUDE_DIR "${RAYTRACER_THIRD_PARTY_DIR}/include")
        set(RT_LINUX_LIB_DIR "${RAYTRACER_THIRD_PARTY_DIR}/lib/linux/x64")

        set(RT_REQUIRED_LINUX_FILES
            "${RT_BUNDLED_INCLUDE_DIR}/GL/glew.h"
            "${RT_BUNDLED_INCLUDE_DIR}/GL/freeglut.h"
            "${RT_BUNDLED_INCLUDE_DIR}/glm/glm.hpp"
            "${RT_LINUX_LIB_DIR}/libGLEW.a"
            "${RT_LINUX_LIB_DIR}/libfreeimage.a"
        )
        foreach(required_file IN LISTS RT_REQUIRED_LINUX_FILES)
            if(NOT EXISTS "${required_file}")
                message(FATAL_ERROR
                    "Missing bundled dependency file: ${required_file}\n"
                    "Set RAYTRACER_THIRD_PARTY_DIR or disable RAYTRACER_USE_BUNDLED_DEPS."
                )
            endif()
        endforeach()

        find_package(OpenGL REQUIRED)
        find_package(X11 REQUIRED)
        find_package(Threads REQUIRED)
        find_library(GLUT_LIBRARY NAMES glut freeglut REQUIRED)
        find_library(GLU_LIBRARY NAMES GLU REQUIRED)

        target_include_directories(RayTracer PRIVATE "${RT_BUNDLED_INCLUDE_DIR}")

        target_link_libraries(RayTracer PRIVATE
            "${RT_LINUX_LIB_DIR}/libGLEW.a"
            "${RT_LINUX_LIB_DIR}/libfreeimage.a"
            "${GLUT_LIBRARY}"
            "${GLU_LIBRARY}"
            OpenGL::GL
            X11::X11
            Threads::Threads
            dl
            m
        )
    else()
        find_package(OpenGL REQUIRED)
        find_package(GLEW REQUIRED)
        find_package(GLUT REQUIRED)
        find_library(FREEIMAGE_LIBRARY NAMES FreeImage freeimage REQUIRED)

        target_link_libraries(RayTracer PRIVATE OpenGL::GL GLEW::GLEW GLUT::GLUT "${FREEIMAGE_LIBRARY}" dl m)
    endif()
endif()
