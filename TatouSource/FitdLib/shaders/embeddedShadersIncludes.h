#ifndef EMBEDDED_SHADERS_INCLUDES_H
#define EMBEDDED_SHADERS_INCLUDES_H

// Fallback definitions for missing embedded shader files
#define SHADER_DATA_FALLBACK static const uint8_t shader_fallback_data[] = { 0 }; static const uint32_t shader_fallback_size = 1;

#if BGFX_PLATFORM_SUPPORTS_SPIRV
#if __has_include("shaders/generated/spirv/ui_vs.sc.bin.h")
#include "shaders/generated/spirv/ui_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/ui_ps.sc.bin.h")
#include "shaders/generated/spirv/ui_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/textSmooth_ps.sc.bin.h")
#include "shaders/generated/spirv/textSmooth_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/background_vs.sc.bin.h")
#include "shaders/generated/spirv/background_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/background_ps.sc.bin.h")
#include "shaders/generated/spirv/background_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/imageBackground_ps.sc.bin.h")
#include "shaders/generated/spirv/imageBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/maskBackground_vs.sc.bin.h")
#include "shaders/generated/spirv/maskBackground_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/maskBackground_ps.sc.bin.h")
#include "shaders/generated/spirv/maskBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/ramp_vs.sc.bin.h")
#include "shaders/generated/spirv/ramp_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/ramp_ps.sc.bin.h")
#include "shaders/generated/spirv/ramp_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/noise_vs.sc.bin.h")
#include "shaders/generated/spirv/noise_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/noise_ps.sc.bin.h")
#include "shaders/generated/spirv/noise_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/flat_vs.sc.bin.h")
#include "shaders/generated/spirv/flat_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/flat_ps.sc.bin.h")
#include "shaders/generated/spirv/flat_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/glass_ps.sc.bin.h")
#include "shaders/generated/spirv/glass_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/customBackground_ps.sc.bin.h")
#include "shaders/generated/spirv/customBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/customMask_ps.sc.bin.h")
#include "shaders/generated/spirv/customMask_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/effectsSides_ps.sc.bin.h")
#include "shaders/generated/spirv/effectsSides_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/sphere_vs.sc.bin.h")
#include "shaders/generated/spirv/sphere_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/sphere_ps.sc.bin.h")
#include "shaders/generated/spirv/sphere_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/effects_vs.sc.bin.h")
#include "shaders/generated/spirv/effects_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/effectsComposite_ps.sc.bin.h")
#include "shaders/generated/spirv/effectsComposite_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/effectsBloomDown_ps.sc.bin.h")
#include "shaders/generated/spirv/effectsBloomDown_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/effectsBloomUp_ps.sc.bin.h")
#include "shaders/generated/spirv/effectsBloomUp_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/effectsSsao_ps.sc.bin.h")
#include "shaders/generated/spirv/effectsSsao_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/effectsSsaoBlur_ps.sc.bin.h")
#include "shaders/generated/spirv/effectsSsaoBlur_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/spirv/effectsAntialias_ps.sc.bin.h")
#include "shaders/generated/spirv/effectsAntialias_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#endif

#if BGFX_PLATFORM_SUPPORTS_METAL
#if __has_include("shaders/generated/metal/ui_vs.sc.bin.h")
#include "shaders/generated/metal/ui_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/ui_ps.sc.bin.h")
#include "shaders/generated/metal/ui_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/textSmooth_ps.sc.bin.h")
#include "shaders/generated/metal/textSmooth_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/background_vs.sc.bin.h")
#include "shaders/generated/metal/background_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/background_ps.sc.bin.h")
#include "shaders/generated/metal/background_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/imageBackground_ps.sc.bin.h")
#include "shaders/generated/metal/imageBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/maskBackground_vs.sc.bin.h")
#include "shaders/generated/metal/maskBackground_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/maskBackground_ps.sc.bin.h")
#include "shaders/generated/metal/maskBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/ramp_vs.sc.bin.h")
#include "shaders/generated/metal/ramp_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/ramp_ps.sc.bin.h")
#include "shaders/generated/metal/ramp_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/noise_vs.sc.bin.h")
#include "shaders/generated/metal/noise_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/noise_ps.sc.bin.h")
#include "shaders/generated/metal/noise_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/flat_vs.sc.bin.h")
#include "shaders/generated/metal/flat_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/flat_ps.sc.bin.h")
#include "shaders/generated/metal/flat_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/glass_ps.sc.bin.h")
#include "shaders/generated/metal/glass_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/customBackground_ps.sc.bin.h")
#include "shaders/generated/metal/customBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/customMask_ps.sc.bin.h")
#include "shaders/generated/metal/customMask_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/effectsSides_ps.sc.bin.h")
#include "shaders/generated/metal/effectsSides_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/sphere_vs.sc.bin.h")
#include "shaders/generated/metal/sphere_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/sphere_ps.sc.bin.h")
#include "shaders/generated/metal/sphere_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/effects_vs.sc.bin.h")
#include "shaders/generated/metal/effects_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/effectsComposite_ps.sc.bin.h")
#include "shaders/generated/metal/effectsComposite_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/effectsBloomDown_ps.sc.bin.h")
#include "shaders/generated/metal/effectsBloomDown_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/effectsBloomUp_ps.sc.bin.h")
#include "shaders/generated/metal/effectsBloomUp_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/effectsSsao_ps.sc.bin.h")
#include "shaders/generated/metal/effectsSsao_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/effectsSsaoBlur_ps.sc.bin.h")
#include "shaders/generated/metal/effectsSsaoBlur_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/metal/effectsAntialias_ps.sc.bin.h")
#include "shaders/generated/metal/effectsAntialias_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#endif

#if BGFX_PLATFORM_SUPPORTS_GLSL
#if __has_include("shaders/generated/glsl/ui_vs.sc.bin.h")
#include "shaders/generated/glsl/ui_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/ui_ps.sc.bin.h")
#include "shaders/generated/glsl/ui_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/textSmooth_ps.sc.bin.h")
#include "shaders/generated/glsl/textSmooth_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/background_vs.sc.bin.h")
#include "shaders/generated/glsl/background_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/background_ps.sc.bin.h")
#include "shaders/generated/glsl/background_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/imageBackground_ps.sc.bin.h")
#include "shaders/generated/glsl/imageBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/maskBackground_vs.sc.bin.h")
#include "shaders/generated/glsl/maskBackground_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/maskBackground_ps.sc.bin.h")
#include "shaders/generated/glsl/maskBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/ramp_vs.sc.bin.h")
#include "shaders/generated/glsl/ramp_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/ramp_ps.sc.bin.h")
#include "shaders/generated/glsl/ramp_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/noise_vs.sc.bin.h")
#include "shaders/generated/glsl/noise_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/noise_ps.sc.bin.h")
#include "shaders/generated/glsl/noise_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/flat_vs.sc.bin.h")
#include "shaders/generated/glsl/flat_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/flat_ps.sc.bin.h")
#include "shaders/generated/glsl/flat_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/glass_ps.sc.bin.h")
#include "shaders/generated/glsl/glass_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/customBackground_ps.sc.bin.h")
#include "shaders/generated/glsl/customBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/customMask_ps.sc.bin.h")
#include "shaders/generated/glsl/customMask_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/effectsSides_ps.sc.bin.h")
#include "shaders/generated/glsl/effectsSides_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/sphere_vs.sc.bin.h")
#include "shaders/generated/glsl/sphere_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/sphere_ps.sc.bin.h")
#include "shaders/generated/glsl/sphere_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/effects_vs.sc.bin.h")
#include "shaders/generated/glsl/effects_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/effectsComposite_ps.sc.bin.h")
#include "shaders/generated/glsl/effectsComposite_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/effectsBloomDown_ps.sc.bin.h")
#include "shaders/generated/glsl/effectsBloomDown_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/effectsBloomUp_ps.sc.bin.h")
#include "shaders/generated/glsl/effectsBloomUp_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/effectsSsao_ps.sc.bin.h")
#include "shaders/generated/glsl/effectsSsao_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/effectsSsaoBlur_ps.sc.bin.h")
#include "shaders/generated/glsl/effectsSsaoBlur_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/glsl/effectsAntialias_ps.sc.bin.h")
#include "shaders/generated/glsl/effectsAntialias_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#endif

#if BGFX_PLATFORM_SUPPORTS_DXBC
#if __has_include("shaders/generated/dx11/ui_vs.sc.bin.h")
#include "shaders/generated/dx11/ui_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/ui_ps.sc.bin.h")
#include "shaders/generated/dx11/ui_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/textSmooth_ps.sc.bin.h")
#include "shaders/generated/dx11/textSmooth_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/background_vs.sc.bin.h")
#include "shaders/generated/dx11/background_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/background_ps.sc.bin.h")
#include "shaders/generated/dx11/background_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/imageBackground_ps.sc.bin.h")
#include "shaders/generated/dx11/imageBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/maskBackground_vs.sc.bin.h")
#include "shaders/generated/dx11/maskBackground_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/maskBackground_ps.sc.bin.h")
#include "shaders/generated/dx11/maskBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/ramp_vs.sc.bin.h")
#include "shaders/generated/dx11/ramp_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/ramp_ps.sc.bin.h")
#include "shaders/generated/dx11/ramp_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/noise_vs.sc.bin.h")
#include "shaders/generated/dx11/noise_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/noise_ps.sc.bin.h")
#include "shaders/generated/dx11/noise_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/flat_vs.sc.bin.h")
#include "shaders/generated/dx11/flat_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/flat_ps.sc.bin.h")
#include "shaders/generated/dx11/flat_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/glass_ps.sc.bin.h")
#include "shaders/generated/dx11/glass_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/customBackground_ps.sc.bin.h")
#include "shaders/generated/dx11/customBackground_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/customMask_ps.sc.bin.h")
#include "shaders/generated/dx11/customMask_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/effectsSides_ps.sc.bin.h")
#include "shaders/generated/dx11/effectsSides_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/sphere_vs.sc.bin.h")
#include "shaders/generated/dx11/sphere_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/sphere_ps.sc.bin.h")
#include "shaders/generated/dx11/sphere_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/effects_vs.sc.bin.h")
#include "shaders/generated/dx11/effects_vs.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/effectsComposite_ps.sc.bin.h")
#include "shaders/generated/dx11/effectsComposite_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/effectsBloomDown_ps.sc.bin.h")
#include "shaders/generated/dx11/effectsBloomDown_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/effectsBloomUp_ps.sc.bin.h")
#include "shaders/generated/dx11/effectsBloomUp_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/effectsSsao_ps.sc.bin.h")
#include "shaders/generated/dx11/effectsSsao_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/effectsSsaoBlur_ps.sc.bin.h")
#include "shaders/generated/dx11/effectsSsaoBlur_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#if __has_include("shaders/generated/dx11/effectsAntialias_ps.sc.bin.h")
#include "shaders/generated/dx11/effectsAntialias_ps.sc.bin.h"
#else
SHADER_DATA_FALLBACK
#endif
#endif

#endif // EMBEDDED_SHADERS_INCLUDES_H
