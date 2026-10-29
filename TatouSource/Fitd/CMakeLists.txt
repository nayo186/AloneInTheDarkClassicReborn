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

add_executable (Fitd ${SOURCES})

# Output executable as Tatou / Tatou.app on macOS, in Fitd/<config> with every
# generator (Visual Studio already does it; Ninja and Make on Linux need it),
# next to the launcher
set_target_properties(Fitd PROPERTIES
    OUTPUT_NAME "Tatou"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Fitd/$<CONFIG>")

IF(${CMAKE_SYSTEM_NAME} MATCHES Darwin)
    # Build as a proper .app bundle
    set_target_properties(Fitd PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
    )

    # Collect game assets to embed in Resources when they exist at configure time.
    # Release assets may be staged either next to the executable in the build tree
    # or in the source tree, depending on which packaging path produced them.
    set(MACOS_ASSETS
        "${CMAKE_BINARY_DIR}/Fitd/aitd_remaster.cfg"
        "${CMAKE_SOURCE_DIR}/aitd_remaster.cfg"
        "${CMAKE_BINARY_DIR}/Fitd/atlases.hda"
        "${CMAKE_SOURCE_DIR}/atlases.hda"
        "${CMAKE_BINARY_DIR}/Fitd/audio.hda"
        "${CMAKE_SOURCE_DIR}/audio.hda"
        "${CMAKE_BINARY_DIR}/Fitd/audio_fre.hda"
        "${CMAKE_SOURCE_DIR}/audio_fre.hda"
        "${CMAKE_BINARY_DIR}/Fitd/backgrounds_hd.hda"
        "${CMAKE_SOURCE_DIR}/backgrounds_hd.hda"
        "${CMAKE_BINARY_DIR}/Fitd/BLKCHCRY.TTF"
        "${CMAKE_SOURCE_DIR}/BLKCHCRY.TTF"
        "${CMAKE_BINARY_DIR}/Fitd/version.txt"
        "${CMAKE_SOURCE_DIR}/version.txt"
    )

    # Mark asset files so CMake places them in Contents/Resources
    foreach(_asset ${MACOS_ASSETS})
        if(EXISTS "${_asset}")
            target_sources(Fitd PRIVATE "${_asset}")
            set_source_files_properties("${_asset}" PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources"
            )
        endif()
    endforeach()

    # Copy any assets that might not exist yet at configure time via a post-build step.
    # Missing optional assets should not make the executable link fail.
    add_custom_command(TARGET Fitd POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DDESTINATION="$<TARGET_BUNDLE_CONTENT_DIR:Fitd>/Resources"
            "-DFILES=${MACOS_ASSETS}"
            -P "${CMAKE_SOURCE_DIR}/cmake/copy_existing_files.cmake"
        COMMAND chmod +x
            "$<TARGET_BUNDLE_CONTENT_DIR:Fitd>/MacOS/Tatou"
        COMMENT "Copying game assets into Tatou.app/Contents/Resources"
    )
endif()


TARGET_LINK_LIBRARIES(Fitd FitdLib bgfx zlibstatic SDL3-static soloud ${EXTRA_LIBS})
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



