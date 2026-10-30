cmake_minimum_required(VERSION 3.9)

# Launcher.exe: chooses the game, mode and language, then starts Tatou.exe.
# Uses the SDL3 and Dear ImGui copies already in ThirdParty (SDL renderer
# backend, no bgfx).

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Game covers compiled into the executable (assets/covers -> coverData.cpp)
set(COVER_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/covers/aitd1.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/covers/aitd2.jpg"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/covers/aitd3.jpg"
)
set(COVER_DATA "${CMAKE_CURRENT_BINARY_DIR}/coverData.cpp")
add_custom_command(
    OUTPUT "${COVER_DATA}"
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DOUTPUT=${COVER_DATA}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/embedCovers.cmake"
    DEPENDS ${COVER_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/embedCovers.cmake"
    COMMENT "Embedding the game covers"
)

# The menus' own font compiled in, used where Georgia is missing
# (Shared/assets/fonts -> menuFontData.cpp, Shared/ui/menuFontData.h)
set(SHARED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../Shared")
set(MENU_FONT_DATA "${CMAKE_CURRENT_BINARY_DIR}/menuFontData.cpp")
add_custom_command(
    OUTPUT "${MENU_FONT_DATA}"
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${SHARED_DIR} -DOUTPUT=${MENU_FONT_DATA}
            -P "${SHARED_DIR}/embedFonts.cmake"
    DEPENDS "${SHARED_DIR}/assets/fonts/Gelasio-Regular.ttf" "${SHARED_DIR}/assets/fonts/Gelasio-Bold.ttf"
            "${SHARED_DIR}/embedFonts.cmake"
    COMMENT "Embedding the menu font"
)

add_executable(Launcher
    main.cpp
    launcherSettings.cpp
    launcherSettings.h
    gameCover.cpp
    gameCover.h
    folderDialog.cpp
    folderDialog.h
    updater.cpp
    updater.h
    Launcher.rc
    "${COVER_DATA}"
    "${MENU_FONT_DATA}"
    "${THIRD_PARTY}/imgui/imgui.cpp"
    "${THIRD_PARTY}/imgui/imgui_draw.cpp"
    "${THIRD_PARTY}/imgui/imgui_tables.cpp"
    "${THIRD_PARTY}/imgui/imgui_widgets.cpp"
    "${THIRD_PARTY}/imgui/backends/imgui_impl_sdl3.cpp"
    "${THIRD_PARTY}/imgui/backends/imgui_impl_sdlrenderer3.cpp"
)

target_include_directories(Launcher PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../Shared"
    "${THIRD_PARTY}/imgui"
    "${THIRD_PARTY}/imgui/backends"
    "${THIRD_PARTY}/SDL/include"
    "${THIRD_PARTY}/bgfx.cmake/bimg/3rdparty/stb"
)

target_link_libraries(Launcher PRIVATE SDL3-static)

# The version of this build, the release tag without its v (tools/makeRelease
# sets it); "dev" builds never look for updates (updater.h)
set(CLASSIC_REBORN_VERSION "dev" CACHE STRING "Version of this build (release tag without v); dev: no updates")
target_compile_definitions(Launcher PRIVATE CLASSIC_REBORN_VERSION="${CLASSIC_REBORN_VERSION}")

# A window application (no console), placed next to Tatou.exe so it can find it
set_target_properties(Launcher PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/Fitd/$<CONFIG>"
)

if(MSVC)
    # Interface texts are UTF-8
    target_compile_options(Launcher PRIVATE /utf-8)
    target_compile_definitions(Launcher PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Building the launcher also builds the game next to it, so F5 (which starts
# the launcher) always runs an up-to-date Tatou.exe
add_dependencies(Launcher Fitd)
