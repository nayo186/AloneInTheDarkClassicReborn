$input v_texcoord0

// Foreground mask over a custom background (customBackgrounds.cpp): the mask
// shape is the original one; its pixels come from the player's picture,
// except where the game drew something 2D (as customBackground_ps)

#include "bgfx_shader.sh"
#include "palette.sh"

SAMPLER2D(s_maskPaletteTexture, 0);
USAMPLER2D(s_backgroundTexture, 2);
USAMPLER2D(s_originalBackground, 3);
SAMPLER2D(s_customBackground, 4);

uniform vec4 u_customBackground;  // xy: U range of the 4:3 centre of the picture, z: 1 = the player's mask has the picture's shape (else the scene's), w: 1 = the player's mask (edge antialiased)

void main()
{
    vec2 pictureUV = vec2(mix(u_customBackground.x, u_customBackground.y, v_texcoord0.x), v_texcoord0.y);
    vec2 maskUV = (u_customBackground.z > 0.5) ? pictureUV : v_texcoord0;
    // The player's mask: its edge is the middle of its grey, antialiased over
    // one screen pixel (whatever the mask's resolution or the width of its
    // grey); the game's mask covers or not
    float coverage = texture2D(s_maskPaletteTexture, maskUV).r;
    float alpha = 1.0;
    if (u_customBackground.w > 0.5)
    {
        float pixel = max(fwidth(coverage), 0.0001);
        alpha = clamp((coverage - 0.5) / pixel + 0.5, 0.0, 1.0);
        if (alpha < 0.01)
            discard;
    }
    else if (coverage < 0.5)
        discard;

    ivec2 p = ivec2(min(v_texcoord0.xy * vec2(320.0, 200.0), vec2(319.0, 199.0)));
    uint current = texelFetch(s_backgroundTexture, p, 0).r;
    uint original = texelFetch(s_originalBackground, p, 0).r;
    if (current != original)
    {
        gl_FragColor = vec4(getColorFromRawOffset(current).rgb, alpha);
        return;
    }
    gl_FragColor = vec4(texture2D(s_customBackground, pictureUV).rgb, alpha);
}
