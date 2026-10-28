$input v_texcoord0

// Bloom: doubles the smaller level with a soft tent filter (8 taps) and adds
// the level of the same size, so the glow mixes several widths. The wider
// levels weigh less, so the glow stays close to the lights.

#include <bgfx_shader.sh>

SAMPLER2D(s_fxScene, 0);   // smaller level
SAMPLER2D(s_fxBloom, 1);   // level of this size (from the halving passes)

uniform vec4 u_fxBloom;  // xy: texel size of the smaller level, z: weight of the smaller level

void main()
{
    vec2 t = u_fxBloom.xy;
    vec2 uv = v_texcoord0;
    vec3 sum = texture2D(s_fxScene, uv + vec2(-2.0 * t.x, 0.0)).rgb;
    sum += texture2D(s_fxScene, uv + vec2( 2.0 * t.x, 0.0)).rgb;
    sum += texture2D(s_fxScene, uv + vec2(0.0, -2.0 * t.y)).rgb;
    sum += texture2D(s_fxScene, uv + vec2(0.0,  2.0 * t.y)).rgb;
    sum += texture2D(s_fxScene, uv + vec2(-t.x, -t.y)).rgb * 2.0;
    sum += texture2D(s_fxScene, uv + vec2( t.x, -t.y)).rgb * 2.0;
    sum += texture2D(s_fxScene, uv + vec2(-t.x,  t.y)).rgb * 2.0;
    sum += texture2D(s_fxScene, uv + vec2( t.x,  t.y)).rgb * 2.0;
    vec3 color = sum / 12.0 * u_fxBloom.z + texture2D(s_fxBloom, uv).rgb;
    gl_FragColor = vec4(color, 1.0);
}
