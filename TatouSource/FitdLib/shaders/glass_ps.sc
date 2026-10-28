$input v_texcoord0

// Glass (transparent polygons), drawn after the scene over a copy of it
// (rendererBGFX.cpp, osystem_drawDeferredGlass; BUGS.md V20). As in the
// original, what is behind keeps its light and takes the colour of the glass;
// unlike the original (which swapped the palette group of each 256-colour
// pixel), it works with real colours: any background (original or custom HD),
// models and effects behind it keep all their detail, at any resolution.

#include "bgfx_shader.sh"
#include "palette.sh"

SAMPLER2D(s_sceneCopy, 0);

uniform vec4 u_glassParams;  // xy: 1 / size of the scene image

float luminance(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec3 behind = texture2D(s_sceneCopy, gl_FragCoord.xy * u_glassParams.xy).rgb;
    float light = luminance(behind);

    // The colours of the glass: the 16 shades of its palette group. Take the
    // two shades around the light behind and blend them (any shade order).
    uint bank = uint(v_texcoord0.y * 15.0 + 0.5);
    vec3 below = vec3_splat(0.0);
    vec3 above = vec3_splat(0.0);
    float belowLight = -1.0;
    float aboveLight = 2.0;
    for (uint i = 0u; i < 16u; i++)
    {
        vec3 shade = getColorFromRawOffset(bank * 16u + i).rgb;
        float shadeLight = luminance(shade);
        if (shadeLight <= light && shadeLight > belowLight)
        {
            belowLight = shadeLight;
            below = shade;
        }
        if (shadeLight >= light && shadeLight < aboveLight)
        {
            aboveLight = shadeLight;
            above = shade;
        }
    }

    vec3 color;
    if (belowLight < 0.0)
        color = above * (light / max(aboveLight, 0.001));  // darker than every shade
    else if (aboveLight > 1.5)
        color = below;                                     // lighter than every shade
    else
        color = mix(below, above, (aboveLight > belowLight) ? (light - belowLight) / (aboveLight - belowLight) : 0.0);

    gl_FragColor = vec4(color, 1.0);
}
