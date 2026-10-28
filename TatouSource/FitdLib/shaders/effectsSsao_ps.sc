$input v_texcoord0

// Ambient occlusion (SSAO) on the 3D models only. The backgrounds are
// pictures without depth (the masks write the far plane, or the start of a
// slice), so pixels without a model keep 1.0 and do not occlude.
// Depth is linear: the models write Z / 40960 (flat_vs.sc), the same Z the
// engine divides by to project (screen = X * fov / Z + center, in 320x200);
// in Original mode inside a slice per object (painter's order, BUGS.md V21).

#include <bgfx_shader.sh>

SAMPLER2D(s_fxDepth, 0);

uniform vec4 u_fxSsao;    // x: radius (world units), y: intensity, z: 1 = texture origin at the bottom, w: 1 = depth range -1..1
uniform vec4 u_fxCamera;  // x: fovX, y: fovY, z: centerX, w: centerY (320x200 screen)
uniform vec4 u_fxTexel;   // xy: 1 / size of the depth texture
uniform vec4 u_fxDepth;   // x: number of depth slices (0 = plain depth)

// Z of the model at uv, or -1 where there is no model
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
    // The start (or, rounded, the end) of a slice is a mask, not a model
    return (z < 1.0 || z > 40000.0) ? -1.0 : z;
}

vec3 viewPosition(vec2 uv, float z)
{
    vec2 s = uv * vec2(320.0, 200.0);
    if (u_fxSsao.z > 0.5)
        s.y = 200.0 - s.y;
    return vec3((s.x - u_fxCamera.z) * z / u_fxCamera.x, (s.y - u_fxCamera.w) * z / u_fxCamera.y, z);
}

// Side step for the normal: the neighbour on the same surface (smaller jump)
vec3 sideStep(vec3 p, vec2 uv, vec2 step)
{
    float z1 = modelZ(uv + step);
    float z0 = modelZ(uv - step);
    vec3 forward = (z1 > 0.0) ? viewPosition(uv + step, z1) - p : vec3_splat(0.0);
    vec3 backward = (z0 > 0.0) ? p - viewPosition(uv - step, z0) : vec3_splat(0.0);
    if (z1 <= 0.0) return backward;
    if (z0 <= 0.0) return forward;
    return (abs(forward.z) < abs(backward.z)) ? forward : backward;
}

void main()
{
    vec2 uv = v_texcoord0;
    float z = modelZ(uv);
    if (z <= 0.0)
    {
        gl_FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        return;
    }

    vec3 p = viewPosition(uv, z);
    vec3 dx = sideStep(p, uv, vec2(u_fxTexel.x, 0.0));
    vec3 dy = sideStep(p, uv, vec2(0.0, u_fxTexel.y));
    vec3 n = cross(dx, dy);
    if (dot(n, n) < 1e-12)
        n = vec3(0.0, 0.0, -1.0);
    n = normalize(n);
    if (dot(n, p) > 0.0)
        n = -n;  // face the camera

    // Fixed rotation per pixel in a 4x4 pattern: the blur removes it and,
    // unlike a rotation per frame, the shading does not shimmer
    vec2 cell = mod(floor(gl_FragCoord.xy), 4.0);
    float index = cell.x * 4.0 + cell.y;
    float angle = fract(index * 0.618034) * 6.2831853;
    float c = cos(angle);
    float s = sin(angle);

    float radius = u_fxSsao.x;
    vec2 radiusUV = vec2(radius * u_fxCamera.x / z / 320.0, radius * u_fxCamera.y / z / 200.0);

    vec2 offsets[12];
    offsets[ 0] = vec2( 0.355512, -0.709318);
    offsets[ 1] = vec2(-0.894878,  0.122717);
    offsets[ 2] = vec2( 0.263014,  0.394172);
    offsets[ 3] = vec2(-0.366623, -0.674568);
    offsets[ 4] = vec2( 0.842890,  0.157139);
    offsets[ 5] = vec2(-0.127852,  0.889610);
    offsets[ 6] = vec2( 0.693171, -0.526748);
    offsets[ 7] = vec2(-0.643845, -0.398119);
    offsets[ 8] = vec2( 0.141613,  0.789750);
    offsets[ 9] = vec2(-0.786443,  0.568012);
    offsets[10] = vec2( 0.500000, -0.114112);
    offsets[11] = vec2(-0.321630,  0.213853);

    float occlusion = 0.0;
    for (int i = 0; i < 12; i++)
    {
        vec2 o = offsets[i];
        vec2 rotated = vec2(o.x * c - o.y * s, o.x * s + o.y * c);
        vec2 sampleUV = clamp(uv + rotated * radiusUV, vec2_splat(0.0), vec2_splat(1.0));
        float sampleZ = modelZ(sampleUV);
        if (sampleZ <= 0.0)
            continue;
        vec3 v = viewPosition(sampleUV, sampleZ) - p;
        float distance = length(v);
        if (distance < 0.001)
            continue;
        // Occludes when it rises above the surface, less the farther it is
        float rise = max(0.0, dot(v / distance, n) - 0.15);
        float falloff = clamp(1.0 - distance / radius, 0.0, 1.0);
        occlusion += rise * falloff;
    }

    float ao = clamp(1.0 - occlusion / 12.0 * 2.0 * u_fxSsao.y, 0.0, 1.0);
    gl_FragColor = vec4(ao, ao, ao, 1.0);
}
