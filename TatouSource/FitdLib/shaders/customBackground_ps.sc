$input v_texcoord0

// Custom background (customBackgrounds.cpp): the player's picture replaces
// the camera background, except where the game drew something 2D over it
// (messages, 2D animations): there the screen differs from the original
// camera picture and keeps the game's pixels.
//
// A pixel can be the same as the original only by chance inside what the
// game changed: the black lines of a drawing the game erased to black (the
// armadillo of the Infogrames logo, which becomes the 3D one). Those keep
// the game's pixels too: a dark pixel with a changed pixel within 2, or any
// pixel whose 8 neighbours are mostly changed. The light paper around the
// letters of a page is not touched (BUGS.md C85).

#include "bgfx_shader.sh"
#include "palette.sh"

SAMPLER2D(s_customBackground, 0);
USAMPLER2D(s_backgroundTexture, 2);   // screen as the game drew it (palette indexes)
USAMPLER2D(s_originalBackground, 3);  // camera picture as loaded (palette indexes)

uniform vec4 u_customBackground;  // xy: U range of the 4:3 centre of the picture

bool changedAt(ivec2 p)
{
    ivec2 q = clamp(p, ivec2(0, 0), ivec2(319, 199));
    return texelFetch(s_backgroundTexture, q, 0).r != texelFetch(s_originalBackground, q, 0).r;
}

void main()
{
    ivec2 p = ivec2(min(v_texcoord0.xy * vec2(320.0, 200.0), vec2(319.0, 199.0)));
    uint current = texelFetch(s_backgroundTexture, p, 0).r;
    uint original = texelFetch(s_originalBackground, p, 0).r;
    vec4 gameColor = getColorFromRawOffset(current);
    if (current != original)
    {
        gl_FragColor = gameColor;
        return;
    }

    bool dark = dot(gameColor.rgb, vec3(0.299, 0.587, 0.114)) < 0.12;
    int near = 0;    // changed among the 8 neighbours
    int around = 0;  // changed within 2
    for (int y = -2; y <= 2; y++)
    {
        for (int x = -2; x <= 2; x++)
        {
            if (x == 0 && y == 0)
                continue;
            if (changedAt(p + ivec2(x, y)))
            {
                around++;
                if (x >= -1 && x <= 1 && y >= -1 && y <= 1)
                    near++;
            }
        }
    }
    if (near >= 5 || (dark && around > 0))
    {
        gl_FragColor = gameColor;
        return;
    }

    vec2 uv = vec2(mix(u_customBackground.x, u_customBackground.y, v_texcoord0.x), v_texcoord0.y);
    gl_FragColor = vec4(texture2D(s_customBackground, uv).rgb, 1.0);
}
