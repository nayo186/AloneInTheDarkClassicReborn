$input v_texcoord0

// Bloom: halves the image (5 taps). The first pass also keeps only the
// bright part of the scene (soft threshold), and only from the painted
// backgrounds: the 3D models do not glow (a white eye or shirt otherwise
// burnt away its details), though the glow of a painted lamp still reaches them.

#include <bgfx_shader.sh>

SAMPLER2D(s_fxScene, 0);
SAMPLER2D(s_fxDepth, 1);

uniform vec4 u_fxBloom;  // xy: source texel size, z: threshold (0 = no threshold), w: knee
uniform vec4 u_fxDepth;  // x: number of depth slices (0 = plain depth), y: 1 = depth range -1..1

// A 3D model was drawn at uv (as in effectsAntialias_ps)
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

vec3 brightPart(vec2 uv)
{
    vec3 c = texture2D(s_fxScene, uv).rgb;
    if (u_fxBloom.z <= 0.0)
        return c;
    if (isModel(uv))
        return vec3_splat(0.0);
    float brightness = max(c.r, max(c.g, c.b));
    float knee = u_fxBloom.w;
    float soft = clamp(brightness - u_fxBloom.z + knee, 0.0, 2.0 * knee);
    soft = soft * soft / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - u_fxBloom.z) / max(brightness, 0.00001);
    return c * contribution;
}

void main()
{
    vec2 t = u_fxBloom.xy;
    vec2 uv = v_texcoord0;
    vec3 sum = brightPart(uv) * 4.0;
    sum += brightPart(uv + vec2(-t.x, -t.y));
    sum += brightPart(uv + vec2( t.x, -t.y));
    sum += brightPart(uv + vec2(-t.x,  t.y));
    sum += brightPart(uv + vec2( t.x,  t.y));
    gl_FragColor = vec4(sum * 0.125, 1.0);
}
