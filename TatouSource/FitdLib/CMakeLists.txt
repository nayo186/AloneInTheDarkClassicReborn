cmake_minimum_required(VERSION 3.9)

# Common include directories
include_directories(
    "."
    # Layer subfolders: files keep including each other by bare name
    "core"
    "util"
    "scripts"
    "scripts/native"
    "platform"
    "remaster/graphics"
    "remaster/ui"
    "remaster/config"
    # Shared with the launcher (advanced option texts)
    "../Shared"
    "${THIRD_PARTY}/imgui"
    "${THIRD_PARTY}/bgfx.cmake/bgfx/include"
    "${THIRD_PARTY}/bgfx.cmake/bx/include"
    "${THIRD_PARTY}/bgfx.cmake/bgfx/3rdparty"
    "${THIRD_PARTY}/bgfx.cmake/bimg/include"
)

# SDL and SoLoud
include_directories(
    "${THIRD_PARTY}/SDL/include/SDL3"
    "${THIRD_PARTY}/SDL/include"
    "${THIRD_PARTY}/soloud.cmake/soloud/include"
)

IF(${MSVC})
    include_directories("${THIRD_PARTY}/bgfx.cmake/bx/include/compat/msvc")
endif()

if(UNIX AND NOT APPLE)
    include_directories("${THIRD_PARTY}/bgfx.cmake/bx/include/compat/linux")
elseif(APPLE)
    include_directories("${THIRD_PARTY}/bgfx.cmake/bx/include/compat/osx")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_ROOT
    "*.cpp"
    "*.h"
)



# Base source files
set(SOURCES
 ${SOURCES}
 ${SRC_ROOT}
 "${THIRD_PARTY}/imgui/imgui.cpp"
 "${THIRD_PARTY}/imgui/imgui.h"
 "${THIRD_PARTY}/imgui/imgui_draw.cpp"
 "${THIRD_PARTY}/imgui/imgui_tables.cpp"
 "${THIRD_PARTY}/imgui/imgui_widgets.cpp"
)

# ImGui's SDL3 backend
list(APPEND SOURCES "${THIRD_PARTY}/imgui/backends/imgui_impl_sdl3.cpp")

function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

macro(addShaderProgramPermutation vsname psname varyingname)
    bgfx_compile_shaders(SHADERS shaders/${vsname}.sc AS_HEADERS OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated TYPE VERTEX VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${varyingname}.sc INCLUDE_DIRS ${BGFX_DIR}/src AS_HEADERS)
    bgfx_compile_shaders(SHADERS shaders/${psname}.sc AS_HEADERS OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated TYPE FRAGMENT VARYING_DEF ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${varyingname}.sc INCLUDE_DIRS ${BGFX_DIR}/src AS_HEADERS)
    #shaderc(FILE shaders/${vsname}.sc BIN2C ${vsname}_${api} OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated/${vsname}_${api}.debug.h DEBUG VERTEX ${platform} VARYINGDEF shaders/${varyingname}.sc INCLUDES ${BGFX_DIR}/src PROFILE ${profileVS})
    #shaderc(FILE shaders/${psname}.sc BIN2C ${psname}_${api} OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated/${psname}_${api}.debug.h DEBUG FRAGMENT ${platform} VARYINGDEF shaders/${varyingname}.sc INCLUDES ${BGFX_DIR}/src PROFILE ${profilePS})
    #set(SOURCES
     #   ${SOURCES}
    #    ${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated/${vsname}_${api}.h
     #   ${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated/${psname}_${api}.h
        #${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated/${vsname}_${api}.debug.h
        #${CMAKE_CURRENT_SOURCE_DIR}/shaders/generated/${psname}_${api}.debug.h
    #)
endmacro()

macro(addShaderProgram vsname psname varyingname)

    set(SOURCES
        ${SOURCES}
        shaders/${vsname}.sc
        shaders/${psname}.sc
        shaders/${varyingname}.sc
    )

    addShaderProgramPermutation(${vsname} ${psname} ${varyingname})
    addShaderProgramPermutation(${vsname} ${psname} ${varyingname})

endmacro()

addShaderProgram(ui_vs ui_ps ui.varying.def)
addShaderProgram(ui_vs textSmooth_ps ui.varying.def)
addShaderProgram(background_vs background_ps background.varying.def)
addShaderProgram(background_vs imageBackground_ps background.varying.def)
addShaderProgram(maskBackground_vs maskBackground_ps maskBackground.varying.def)
addShaderProgram(flat_vs flat_ps flat.varying.def)
addShaderProgram(flat_vs glass_ps flat.varying.def)
addShaderProgram(background_vs customBackground_ps background.varying.def)
addShaderProgram(maskBackground_vs customMask_ps maskBackground.varying.def)
addShaderProgram(effects_vs effectsSides_ps effects.varying.def)
addShaderProgram(noise_vs noise_ps noise.varying.def)
addShaderProgram(ramp_vs ramp_ps ramp.varying.def)
addShaderProgram(sphere_vs sphere_ps sphere.varying.def)
addShaderProgram(effects_vs effectsComposite_ps effects.varying.def)
addShaderProgram(effects_vs effectsBloomDown_ps effects.varying.def)
addShaderProgram(effects_vs effectsBloomUp_ps effects.varying.def)
addShaderProgram(effects_vs effectsSsao_ps effects.varying.def)
addShaderProgram(effects_vs effectsSsaoBlur_ps effects.varying.def)
addShaderProgram(effects_vs effectsAntialias_ps effects.varying.def)

assign_source_group(${SOURCES})

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
    add_compile_options(/Zc:preprocessor)
endif()

add_library (FitdLib ${SOURCES})

# Development tools: the object editor (F10) and the item window (F9). ON in
# our builds; the version given to players is built with OFF, so they do not
# exist in its Tatou.exe (the fixes made with them are still compiled in).
option(CLASSIC_REBORN_DEV_TOOLS "Object editor (F10) and item window (F9)" ON)
if(CLASSIC_REBORN_DEV_TOOLS)
    target_compile_definitions(FitdLib PRIVATE CLASSIC_REBORN_DEV_TOOLS=1
        OBJECT_FIXES_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets/objectFixes")
else()
    target_compile_definitions(FitdLib PRIVATE CLASSIC_REBORN_DEV_TOOLS=0)
endif()

# Visual fixes of 3D objects compiled in (assets/objectFixes -> objectFixesData.cpp)
set(OBJECT_FIXES_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/objectFixes/aitd1.cfg"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/objectFixes/jack.cfg"
)
set(OBJECT_FIXES_DATA "${CMAKE_CURRENT_BINARY_DIR}/objectFixesData.cpp")
add_custom_command(
    OUTPUT "${OBJECT_FIXES_DATA}"
    COMMAND ${CMAKE_COMMAND} -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR} -DOUTPUT=${OBJECT_FIXES_DATA}
            -P "${CMAKE_CURRENT_SOURCE_DIR}/embedObjectFixes.cmake"
    DEPENDS ${OBJECT_FIXES_FILES} "${CMAKE_CURRENT_SOURCE_DIR}/embedObjectFixes.cmake"
    COMMENT "Embedding the visual fixes of 3D objects"
)
target_sources(FitdLib PRIVATE "${OBJECT_FIXES_DATA}")

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
target_sources(FitdLib PRIVATE "${MENU_FONT_DATA}")

if(MSVC)
    # UTF-8 texts (Shared/advancedOptionsText.h, the object editor)
    set_source_files_properties(remaster/ui/advancedOptions.cpp remaster/ui/objectEditor.cpp PROPERTIES COMPILE_OPTIONS "/utf-8")
endif()



#set( CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /DBX_CONFIG_DEBUG=1")
#set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /DBX_CONFIG_DEBUG=1")

set_property(TARGET FitdLib PROPERTY CXX_STANDARD 20)


# Position-independent code outside Windows
if(UNIX)
    target_compile_options(FitdLib PRIVATE -fPIC)
endif()

# zlib's headers come with the library (no copy of them in util/)
target_link_libraries(FitdLib PUBLIC zlibstatic)
