$input v_texcoord0

// Final image of the screen effects: scene x ambient occlusion + bloom,
// then film grain and the screen fade

#include <bgfx_shader.sh>

SAMPLER2D(s_fxScene, 0);
SAMPLER2D(s_fxBloom, 1);   // black when bloom is off
SAMPLER2D(s_fxAO, 2);      // white when SSAO is off

uniform vec4 u_fxComposite;  // x: bloom intensity, y: grain intensity, z: grain frame, w: fade (0 black, 1 normal)
uniform vec4 u_fxGrain;      // x: grain cell size in pixels

// Hash without sin() (Dave Hoskins), stable on every GPU
float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main()
{
    vec3 color = texture2D(s_fxScene, v_texcoord0).rgb;
    color *= texture2D(s_fxAO, v_texcoord0).r;
    color += texture2D(s_fxBloom, v_texcoord0).rgb * u_fxComposite.x;

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
