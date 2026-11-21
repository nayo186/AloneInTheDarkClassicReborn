$input v_texcoord0

// Custom background (customBackgrounds.cpp): the player's picture replaces
// the camera background, except where the game drew something 2D over it
// (messages, 2D animations, the black lines of an erased drawing): there the
// game's pixels stay. A part copied from a second picture (a portrait beside
// a page) shows that picture's image. Which one each pixel shows is decided
// once per change of the screen, for its 320x200 pixels (keepGameTexture,
// BUGS.md C85, C86, C88).

#include "bgfx_shader.sh"
#include "palette.sh"

SAMPLER2D(s_customBackground, 0);
USAMPLER2D(s_backgroundTexture, 2);   // screen as the game drew it (palette indexes)
USAMPLER2D(s_keepGame, 3);            // 0 the picture, 1 the game's pixel, 2 the second picture
SAMPLER2D(s_secondBackground, 4);

uniform vec4 u_customBackground;  // xy: U range of the 4:3 centre of the picture; zw: of the second

void main()
{
    ivec2 p = ivec2(min(v_texcoord0.xy * vec2(320.0, 200.0), vec2(319.0, 199.0)));
    uint source = texelFetch(s_keepGame, p, 0).r;
    if (source == 1u)
    {
        gl_FragColor = getColorFromRawOffset(texelFetch(s_backgroundTexture, p, 0).r);
        return;
    }
    if (source == 2u)
    {
        vec2 uv2 = vec2(mix(u_customBackground.z, u_customBackground.w, v_texcoord0.x), v_texcoord0.y);
        gl_FragColor = vec4(texture2D(s_secondBackground, uv2).rgb, 1.0);
        return;
    }
    vec2 uv = vec2(mix(u_customBackground.x, u_customBackground.y, v_texcoord0.x), v_texcoord0.y);
    gl_FragColor = vec4(texture2D(s_customBackground, uv).rgb, 1.0);
}
