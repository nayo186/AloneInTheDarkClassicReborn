$input v_texcoord0

// Antialiasing of the 3D models (FXAA, screenEffects.cpp). Only pixels of a
// model or next to one are smoothed: the backgrounds are pictures without
// depth (as in effectsSsao_ps) and stay sharp.
// FXAA after Timothy Lottes' compact version: the edge direction comes from
// the brightness of the four corners, and the pixel is blended along it.

#include <bgfx_shader.sh>

SAMPLER2D(s_fxScene, 0);
SAMPLER2D(s_fxDepth, 1);

uniform vec4 u_fxTexel;   // xy: 1 / size of the scene image
uniform vec4 u_fxDepth;   // x: number of depth slices (0 = plain depth), y: 1 = depth range -1..1

// A 3D model was drawn at uv (linear depth Z / 40960, see effectsSsao_ps)
bool isModel(vec2 uv)
{
    float d = texture2DLod(s_fxDepth, uv, 0.0).x;
    if (d >= 0.9999)
        return false;
    if (u_fxDepth.y > 0.5)
        d = d * 2.0 - 1.0;
    if (u_fxDepth.x > 0.0)
        d = fract(d * u_fxDepth.x);
    float z = d * 40960.0;
    // The start (or, rounded, the end) of a slice is a mask, not a model
    return z >= 1.0 && z <= 40000.0;
}

bool nearModel(vec2 uv, vec2 t)
{
    return isModel(uv)
        || isModel(uv + vec2(t.x, 0.0)) || isModel(uv - vec2(t.x, 0.0))
        || isModel(uv + vec2(0.0, t.y)) || isModel(uv - vec2(0.0, t.y))
        || isModel(uv + t) || isModel(uv - t)
        || isModel(uv + vec2(t.x, -t.y)) || isModel(uv + vec2(-t.x, t.y));
}

float luma(vec3 c)
{
    return dot(c, vec3(0.299, 0.587, 0.114));
}

void main()
{
    vec2 uv = v_texcoord0;
    vec2 t = u_fxTexel.xy;
    vec4 centre = texture2DLod(s_fxScene, uv, 0.0);
    if (!nearModel(uv, t))
    {
        gl_FragColor = centre;
        return;
    }

    vec3 rgbNW = texture2DLod(s_fxScene, uv + vec2(-t.x, -t.y), 0.0).rgb;
    vec3 rgbNE = texture2DLod(s_fxScene, uv + vec2( t.x, -t.y), 0.0).rgb;
    vec3 rgbSW = texture2DLod(s_fxScene, uv + vec2(-t.x,  t.y), 0.0).rgb;
    vec3 rgbSE = texture2DLod(s_fxScene, uv + vec2( t.x,  t.y), 0.0).rgb;
    float lumaNW = luma(rgbNW);
    float lumaNE = luma(rgbNE);
    float lumaSW = luma(rgbSW);
    float lumaSE = luma(rgbSE);
    float lumaM = luma(centre.rgb);
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    // Edge direction (perpendicular to the brightness change)
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 / 8.0), 1.0 / 128.0);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-8.0, -8.0), vec2(8.0, 8.0)) * t;

    vec3 rgbA = 0.5 * (texture2DLod(s_fxScene, uv + dir * (1.0 / 3.0 - 0.5), 0.0).rgb
                     + texture2DLod(s_fxScene, uv + dir * (2.0 / 3.0 - 0.5), 0.0).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture2DLod(s_fxScene, uv - dir * 0.5, 0.0).rgb
                                   + texture2DLod(s_fxScene, uv + dir * 0.5, 0.0).rgb);
    float lumaB = luma(rgbB);
    vec3 rgb = (lumaB < lumaMin || lumaB > lumaMax) ? rgbA : rgbB;
    gl_FragColor = vec4(rgb, centre.a);
}
