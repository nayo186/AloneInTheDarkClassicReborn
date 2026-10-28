$input v_texcoord0

// Ambient occlusion: 4x4 blur that only mixes pixels of the same surface
// (similar depth); it removes the 4x4 rotation pattern of effectsSsao_ps

#include <bgfx_shader.sh>

SAMPLER2D(s_fxAO, 0);
SAMPLER2D(s_fxDepth, 1);

uniform vec4 u_fxTexel;  // xy: 1 / size
uniform vec4 u_fxSsao;   // w: 1 = depth range -1..1
uniform vec4 u_fxDepth;  // x: number of depth slices (0 = plain depth)

// Z of the model at uv, or -1 where there is no model (as effectsSsao_ps)
float modelZ(vec2 uv)
{
    float d = texture2DLod(s_fxDepth, uv, 0.0).x;
    if (d >= 0.9999)
        return -1.0;
    if (u_fxSsao.w > 0.5)
        d = d * 2.0 - 1.0;
    if (u_fxDepth.x > 0.0)
        d = fract(d * u_fxDepth.x);
    float z = d * 40960.0;
    return (z < 1.0 || z > 40000.0) ? -1.0 : z;
}

void main()
{
    vec2 uv = v_texcoord0;
    float centerZ = modelZ(uv);
    if (centerZ <= 0.0)
    {
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    float sum = 0.0;
    float weight = 0.0;
    for (int y = -2; y < 2; y++)
    {
        for (int x = -2; x < 2; x++)
        {
            // Any 4x4 block holds the 16 rotations of the pattern
            vec2 sampleUV = uv + vec2(float(x), float(y)) * u_fxTexel.xy;
            float z = modelZ(sampleUV);
            float w = (z > 0.0 && abs(z - centerZ) < centerZ * 0.02) ? 1.0 : 0.0;
            sum += texture2DLod(s_fxAO, sampleUV, 0.0).r * w;
            weight += w;
        }
    }

    float ao = (weight > 0.0) ? sum / weight : 1.0;
    gl_FragColor = vec4(ao, ao, ao, 1.0);
}
