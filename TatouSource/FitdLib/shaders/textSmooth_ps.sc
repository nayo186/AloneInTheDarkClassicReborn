$input v_texcoord0

// The text layer (uiLayer: the game's bitmap font at 320x200) shown at the
// screen's resolution (rendererBGFX.cpp, osystem_drawUILayer), instead of
// ui_ps, when the player turns on smooth text or the text outline.
// Smooth: the 1-bit letters are rebuilt as a smooth shape (bilinear field
// of the letter pixels, cut a little below the middle): staircases become
// straight, strokes slightly thicker, the edge antialiased over one screen
// pixel. Outline: a thin soft dark band around that shape.

#include "bgfx_shader.sh"
#include "palette.sh"

USAMPLER2D(s_backgroundTexture, 0);

uniform vec4 u_textStyle;  // x: 1 = smooth, y: 1 = outline, z: stroke cut (0.5 = original width, lower = bolder), w: outline opacity

uint letterIndex(ivec2 p)
{
    p = clamp(p, ivec2(0, 0), ivec2(319, 199));
    return texelFetch(s_backgroundTexture, p, 0).r;
}

float isLetter(ivec2 p)
{
    return (letterIndex(p) != uint(0)) ? 1.0 : 0.0;
}

// Coverage over one screen pixel of a smooth field cut at 'cut'
float antialiased(float field, float cut)
{
    float pixel = max(fwidth(field), 0.0001);
    return clamp((field - cut) / pixel + 0.5, 0.0, 1.0);
}

void main()
{
    vec2 position = v_texcoord0.xy * vec2(320.0, 200.0);
    float smoothText = u_textStyle.x;

    // Texel centres are at .5: the four around this point and the weights
    vec2 p = position - 0.5;
    ivec2 base = ivec2(floor(p));
    vec2 f = p - floor(p);
    ivec2 p00 = base;
    ivec2 p10 = base + ivec2(1, 0);
    ivec2 p01 = base + ivec2(0, 1);
    ivec2 p11 = base + ivec2(1, 1);

    // Smooth field of the letter pixels around this point (bilinear)
    float w00 = (1.0 - f.x) * (1.0 - f.y), w10 = f.x * (1.0 - f.y);
    float w01 = (1.0 - f.x) * f.y,         w11 = f.x * f.y;
    float l00 = isLetter(p00), l10 = isLetter(p10), l01 = isLetter(p01), l11 = isLetter(p11);
    float field = l00 * w00 + l10 * w10 + l01 * w01 + l11 * w11;

    float textAlpha;
    uint index;
    if (smoothText > 0.5)
    {
        textAlpha = antialiased(field, u_textStyle.z);

        // Colour of the nearest letter pixel among the four
        float best = -1.0;
        index = uint(0);
        if (l00 * w00 > best && l00 > 0.5) { best = l00 * w00; index = letterIndex(p00); }
        if (l10 * w10 > best && l10 > 0.5) { best = l10 * w10; index = letterIndex(p10); }
        if (l01 * w01 > best && l01 > 0.5) { best = l01 * w01; index = letterIndex(p01); }
        if (l11 * w11 > best && l11 > 0.5) { best = l11 * w11; index = letterIndex(p11); }
    }
    else
    {
        // Original pixels
        index = letterIndex(ivec2(position));
        textAlpha = (index != uint(0)) ? 1.0 : 0.0;
    }

    // Outline: a thin dark band just outside the smooth letter shape (about
    // a third of an original pixel), fading outwards like a soft shadow
    float outlineAlpha = 0.0;
    if (u_textStyle.y > 0.5)
        outlineAlpha = smoothstep(0.03, 0.30, field) * u_textStyle.w;

    // The letter over its dark outline (the outline adds coverage, not colour)
    float alpha = textAlpha + outlineAlpha * (1.0 - textAlpha);
    if (alpha < 0.01)
        discard;
    vec3 letter = (index != uint(0)) ? getColorFromRawOffset(index).rgb : vec3(0.0, 0.0, 0.0);
    gl_FragColor = vec4(letter * textAlpha / alpha, alpha);
}
