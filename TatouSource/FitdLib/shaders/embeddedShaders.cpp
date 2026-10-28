#include "embeddedShadersMacro.h"
#include "config.h"
#include "embeddedShadersIncludes.h"

static const bgfx::EmbeddedShader s_embeddedShaders[] =
{
    BGFX_EMBEDDED_SHADER(ui_vs),
    BGFX_EMBEDDED_SHADER(ui_ps),
BGFX_EMBEDDED_SHADER(textSmooth_ps),
    BGFX_EMBEDDED_SHADER(background_vs),
BGFX_EMBEDDED_SHADER(background_ps),
BGFX_EMBEDDED_SHADER(imageBackground_ps),
BGFX_EMBEDDED_SHADER(maskBackground_vs),
BGFX_EMBEDDED_SHADER(maskBackground_ps),
BGFX_EMBEDDED_SHADER(flat_vs),
BGFX_EMBEDDED_SHADER(flat_ps),
BGFX_EMBEDDED_SHADER(glass_ps),
BGFX_EMBEDDED_SHADER(customBackground_ps),
BGFX_EMBEDDED_SHADER(customMask_ps),
BGFX_EMBEDDED_SHADER(effectsSides_ps),
BGFX_EMBEDDED_SHADER(noise_vs),
BGFX_EMBEDDED_SHADER(noise_ps),
BGFX_EMBEDDED_SHADER(ramp_vs),
BGFX_EMBEDDED_SHADER(ramp_ps),
    BGFX_EMBEDDED_SHADER(sphere_vs),
    BGFX_EMBEDDED_SHADER(sphere_ps),
BGFX_EMBEDDED_SHADER(effects_vs),
BGFX_EMBEDDED_SHADER(effectsComposite_ps),
BGFX_EMBEDDED_SHADER(effectsBloomDown_ps),
BGFX_EMBEDDED_SHADER(effectsBloomUp_ps),
BGFX_EMBEDDED_SHADER(effectsSsao_ps),
BGFX_EMBEDDED_SHADER(effectsSsaoBlur_ps),
BGFX_EMBEDDED_SHADER(effectsAntialias_ps),

BGFX_EMBEDDED_SHADER_END()
};

bgfx::ProgramHandle loadBgfxProgram(const std::string& VSFile, const std::string& PSFile)
{
bgfx::RendererType::Enum type = bgfx::getRendererType();

bgfx::ProgramHandle ProgramHandle = bgfx::createProgram(
bgfx::createEmbeddedShader(s_embeddedShaders, type, VSFile.c_str())
, bgfx::createEmbeddedShader(s_embeddedShaders, type, PSFile.c_str())
, true
);
assert(bgfx::isValid(ProgramHandle));
return ProgramHandle;
}