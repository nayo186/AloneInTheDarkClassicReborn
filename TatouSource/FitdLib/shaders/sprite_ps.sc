$input v_texcoord0

#include "bgfx_shader.sh"
#include "palette.sh"

// A 2D drawing of AITD2 and later (core/hybrid.cpp, AffHyb), on its own
// layer: r = colour, g = 1 where the drawing covers the pixel (its colour 0
// is black, not see-through)
USAMPLER2D(s_spriteTexture, 0);

void main()
{
    vec2 position = vec2(v_texcoord0.xy) * vec2(320, 200);
    uvec4 texel = texelFetch(s_spriteTexture, ivec2(position), 0);
    if (texel.g == 0u)
        discard;

    gl_FragColor = vec4(getColorFromRawOffset(texel.r).rgb, 1.0);
}
