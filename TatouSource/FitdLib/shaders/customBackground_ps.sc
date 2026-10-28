$input v_texcoord0

// Custom background (customBackgrounds.cpp): the player's picture replaces
// the camera background, except where the game drew something 2D over it
// (messages, 2D animations): there the screen differs from the original
// camera picture and keeps the game's pixels.

#include "bgfx_shader.sh"
#include "palette.sh"

SAMPLER2D(s_customBackground, 0);
USAMPLER2D(s_backgroundTexture, 2);   // screen as the game drew it (palette indexes)
USAMPLER2D(s_originalBackground, 3);  // camera picture as loaded (palette indexes)

uniform vec4 u_customBackground;  // xy: U range of the 4:3 centre of the picture

void main()
{
    ivec2 p = ivec2(min(v_texcoord0.xy * vec2(320.0, 200.0), vec2(319.0, 199.0)));
    uint current = texelFetch(s_backgroundTexture, p, 0).r;
    uint original = texelFetch(s_originalBackground, p, 0).r;
    if (current != original)
    {
        gl_FragColor = getColorFromRawOffset(current);
        return;
    }
    vec2 uv = vec2(mix(u_customBackground.x, u_customBackground.y, v_texcoord0.x), v_texcoord0.y);
    gl_FragColor = vec4(texture2D(s_customBackground, uv).rgb, 1.0);
}
