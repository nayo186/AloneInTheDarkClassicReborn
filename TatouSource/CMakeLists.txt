cmake_minimum_required(VERSION 3.9)
project(FITD)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBX_CONFIG_DEBUG=0")
#set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DBX_CONFIG_DEBUG=1")

IF(MSVC)
    #set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /D_MSVC_STL_HARDENING=1")
    #set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /D_MSVC_STL_HARDENING=1")

    # The C++ runtime goes inside the executables (/MT): they run on any
    # Windows without installing the Visual C++ Redistributable. Every
    # library must use the same runtime: projects with the new CMake policy
    # read CMAKE_MSVC_RUNTIME_LIBRARY, the older ones the compiler flags.
    option(CLASSIC_REBORN_STATIC_RUNTIME "C++ runtime inside the executables (no Visual C++ Redistributable needed)" ON)
    if(CLASSIC_REBORN_STATIC_RUNTIME)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        foreach(flags CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL
                      CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_MINSIZEREL)
            string(REPLACE "/MDd" "/MTd" ${flags} "${${flags}}")
            string(REPLACE "/MD" "/MT" ${flags} "${${flags}}")
        endforeach()
    endif()
ENDIF(MSVC)

# Linux-specific: find required system packages
if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    find_package(Threads REQUIRED)
    find_package(X11 REQUIRED)
    pkg_check_modules(GL REQUIRED gl)

    # Additional Linux libraries needed by bgfx/SDL
    set(EXTRA_LIBS
        ${CMAKE_THREAD_LIBS_INIT}
        ${X11_LIBRARIES}
        ${GL_LIBRARIES}
        dl
        rt
    )
endif()

set(THIRD_PARTY "${CMAKE_SOURCE_DIR}/ThirdParty")

# bgfx: the renderer and its shader compiler (shaderc, used to build our
# shaders); not its examples, model and texture tools nor convenience targets
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory( ${THIRD_PARTY}/bgfx.cmake )
# bgfx forces its example-common project while its tools are on; it is not
# compiled (nothing here uses it)
if(TARGET example-common)
    set_target_properties(example-common PROPERTIES EXCLUDE_FROM_ALL ON EXCLUDE_FROM_DEFAULT_BUILD ON)
endif()

# Audio (SoLoud) and window, input and controllers (SDL, static, without
# its test library)
add_subdirectory( ${THIRD_PARTY}/soloud.cmake )

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
add_subdirectory( ${THIRD_PARTY}/SDL )

# zlib (save previews in PNG, compressed audio): only its static library;
# its own CMake would also build a shared library and two examples
file(GLOB ZLIB_SOURCES "${THIRD_PARTY}/zlib/*.c")
configure_file("${THIRD_PARTY}/zlib/zconf.h.in" "${CMAKE_BINARY_DIR}/zlib/zconf.h" COPYONLY)
add_library(zlibstatic STATIC ${ZLIB_SOURCES})
target_include_directories(zlibstatic PUBLIC "${THIRD_PARTY}/zlib" "${CMAKE_BINARY_DIR}/zlib")
if(MSVC)
    target_compile_definitions(zlibstatic PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
endif()

add_subdirectory( FitdLib )
add_subdirectory( Fitd )
add_subdirectory( Launcher )
# F5 in Visual Studio starts the launcher; to debug the game itself use
# Fitd > Debug > Start new instance
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT Launcher)

#set(USE_SANITIZER ON)
if(USE_SANITIZER)
    IF(MSVC)
        set(SANITIZER_TYPE "address")
    ELSE()
        set(SANITIZER_TYPE "address,undefined,leak")
    ENDIF()
	
	target_link_options(Fitd PRIVATE "-fsanitize=${SANITIZER_TYPE}")
	target_compile_options(Fitd PRIVATE "-fsanitize=${SANITIZER_TYPE}")
	target_compile_options(FitdLib PRIVATE "-fsanitize=${SANITIZER_TYPE}")
	target_compile_options(bimg PRIVATE "-fsanitize=${SANITIZER_TYPE}")
	target_compile_options(bgfx PRIVATE "-fsanitize=${SANITIZER_TYPE}")
	target_compile_options(soloud PRIVATE "-fsanitize=${SANITIZER_TYPE}")
endif()
