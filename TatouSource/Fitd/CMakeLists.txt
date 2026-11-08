cmake_minimum_required(VERSION 3.9)

include_directories(
    "."
    "${CMAKE_SOURCE_DIR}/FitdLib"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB FITD_DATA
    "${CMAKE_SOURCE_DIR}/data/*"
)

file(GLOB SRC_ROOT
    "*.cpp"
    "*.h"
    "*.rc"
)

set(SOURCES
 ${SOURCES}
 ${SRC_ROOT}
)

IF(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    set(SOURCES
     ${SOURCES}
     "bgfxPatch.mm"
    )
    set_source_files_properties("bgfxPatch.mm" PROPERTIES
        LANGUAGE CXX
        COMPILE_FLAGS "-x objective-c++"
    )
endif()

source_group("src" FILES ${SRC_ROOT})

if(MSVC)
    set( CMAKE_EXE_LINKER_FLAGS_SHIPPING "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO} /LTCG /OPT:ICF /OPT:REF /SUBSYSTEM:WINDOWS /DEBUG" )
endif()

# Android: a library (libmain.so) that SDL's Java side loads and starts
if(ANDROID)
    add_library(Fitd SHARED ${SOURCES})
else()
    add_executable (Fitd ${SOURCES})
endif()

# Output executable as Tatou (a plain program on macOS too, next to the launcher), in Fitd/<config> with every
# generator (Visual Studio already does it; Ninja and Make on Linux need it),
# next to the launcher
set_target_properties(Fitd PROPERTIES
    OUTPUT_NAME "Tatou"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Fitd/$<CONFIG>")

if(ANDROID)
    set_target_properties(Fitd PROPERTIES OUTPUT_NAME "main")
endif()

TARGET_LINK_LIBRARIES(Fitd FitdLib bgfx zlibstatic SDL3-static soloud ${EXTRA_LIBS})
if(ANDROID)
    # The launcher's screen comes first (before FitdLib: it uses its ImGui)
    target_link_libraries(Fitd Launcher FitdLib SDL3-static android log EGL GLESv3)
endif()
if(APPLE)
    target_link_libraries(Fitd "-framework CoreFoundation")
endif()

set_target_properties(Fitd PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:Fitd>")

# Linux: function names in the crash report's stack trace (util/exceptionHandler.h)
if(UNIX AND NOT APPLE)
    set_target_properties(Fitd PROPERTIES ENABLE_EXPORTS ON)
endif()

# No console window except in Debug builds (all output goes to tatou.log).
# A generator expression is needed: multi-config generators such as Visual
# Studio leave CMAKE_BUILD_TYPE empty.
set_target_properties(Fitd PROPERTIES WIN32_EXECUTABLE $<NOT:$<CONFIG:Debug>>)



