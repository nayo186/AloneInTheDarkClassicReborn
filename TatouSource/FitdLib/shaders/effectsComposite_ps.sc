$input v_texcoord0

// Final image of the screen effects: scene x ambient occlusion + bloom,
// then film grain and the screen fade

#include <bgfx_shader.sh>

SAMPLER2D(s_fxScene, 0);
SAMPLER2D(s_fxBloom, 1);   // black when bloom is off
SAMPLER2D(s_fxAO, 2);      // white when SSAO is off

uniform vec4 u_fxComposite;  // x: bloom intensity, y: grain intensity, z: grain frame, w: fade (0 black, 1 normal)
uniform vec4 u_fxGrain;      // x: grain cell size in pixels
uniform vec4 u_fxSmoke;      // cigar smoke: xy centre, zw radii (0 = none), in texture coordinates
uniform vec4 u_fxSmokeTime;  // x: seconds, for the moving shape of the smoke

// Hash without sin() (Dave Hoskins), stable on every GPU
float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// Smooth value noise (0..1) and a few octaves of it: the shape of the smoke
float valueNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash12(cell);
    float b = hash12(cell + vec2(1.0, 0.0));
    float c = hash12(cell + vec2(0.0, 1.0));
    float d = hash12(cell + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float cloudNoise(vec2 p)
{
    return valueNoise(p) * 0.5 + valueNoise(p * 2.03 + 17.0) * 0.3 + valueNoise(p * 4.01 + 41.0) * 0.2;
}

void main()
{
    vec3 color = texture2D(s_fxScene, v_texcoord0).rgb;
    color *= texture2D(s_fxAO, v_texcoord0).r;
    color += texture2D(s_fxBloom, v_texcoord0).rgb * u_fxComposite.x;

    // Cigar smoke: everything it covers turns grey, a little hazy (as the
    // original game drew it), in clouds turning around the cigars: a ragged
    // edge and thicker and thinner patches inside. The same shape decides the
    // cough: smokeShapeDistance in core/sceneDraw.cpp must match this
    if (u_fxSmoke.z > 0.0)
    {
        // The shape on the game's 320x200 grid, so its edge steps in the
        // game's own pixels
        vec2 gamePixel = (floor(v_texcoord0 * vec2(320.0, 200.0)) + 0.5) / vec2(320.0, 200.0);
        vec2 local = (gamePixel - u_fxSmoke.xy) / u_fxSmoke.zw;   // the ellipse is the unit circle
        float t = u_fxSmokeTime.x;
        float angle = t * 0.25;
        vec2 swirl = vec2(local.x * cos(angle) - local.y * sin(angle), local.x * sin(angle) + local.y * cos(angle));
        float edge = cloudNoise(swirl * 2.2 + vec2(t * 0.03, -t * 0.04));
        float distance = length(local) + (edge - 0.5) * 0.7;
        float patches = cloudNoise(swirl * 3.5 + vec2(-t * 0.02, t * 0.03));
        // Edge in flat bands, like hand-painted cartoon smoke, not a smooth fade
        float edgeBands = (1.0 - step(0.95, distance)) * 0.35 + (1.0 - step(0.82, distance)) * 0.3
            + (1.0 - step(0.68, distance)) * 0.35;
        float amount = edgeBands * mix(0.55, 1.0, patches);
        float luma = dot(color, vec3(0.299, 0.587, 0.114));
        vec3 smoke = vec3_splat(luma * 0.9 + 0.08);
        color = mix(color, smoke, amount * 0.9);
    }

    if (u_fxComposite.y > 0.0)
    {
        // Two uniform values make a triangular (-1..1) noise, new 24 times a second
        vec2 cell = floor(gl_FragCoord.xy / u_fxGrain.x);
        vec2 frameOffset = vec2(u_fxComposite.z * 17.0, u_fxComposite.z * 59.0);
        float noise = hash12(cell + frameOffset) + hash12(cell * 1.37 + frameOffset + 71.0) - 1.0;
        // Strongest in the mid tones, like film; weaker in black and white
        float luma = dot(color, vec3(0.299, 0.587, 0.114));
        float weight = 0.4 + 0.6 * (1.0 - abs(luma * 2.0 - 1.0));
        color += vec3_splat(noise * u_fxComposite.y * weight);
    }

    color *= u_fxComposite.w;
    gl_FragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
