$input v_texcoord0, v_screenSpacePosition, v_modelPosition

#include "bgfx_shader.sh"
#include "palette.sh"

USAMPLER2D(s_noiseTexture, 0);

// The grain value (0..1) of the cell of the model where this pixel is, with
// cells of 2^level units of the model
float grainAt(vec3 modelPosition, float level)
{
    float cellSize = exp2(level);
    ivec3 cell = ivec3(floor(modelPosition / cellSize));
    int seed = int(level) + 16;
    ivec2 noiseCell = ivec2((cell.x + cell.z * 37 + seed * 11) & 255, (cell.y + cell.z * 71 + seed * 23) & 255);
    return float(texelFetch(s_noiseTexture, noiseCell, 0).r) / 255.f;
}

// "Pierre" (stone) faces of the 3D models: the colour darkened by a random
// amount in small cells (as the 1992 game draws them: a grain of darker
// pixels). The cells are fixed to the model, so the grain moves and turns
// with the object, and are about one pixel of the game's 320 x 200: their
// size follows only the distance (v_modelPosition.w: the model's size of a
// pixel of the game there), never the angle of the face. Between two sizes
// (each twice the other) the grain fades from one to the other instead of
// jumping. Only the colour changes, never the alpha.
void main()
{
    int color = int(v_texcoord0.x * 15.f);
    int bank = int(v_texcoord0.y * 15.f);
    vec4 baseColor = getColor(bank, color);

    float level = log2(max(v_modelPosition.w, 0.0625f));
    float lower = floor(level);
    float blend = smoothstep(0.25f, 0.75f, level - lower);
    float fine = grainAt(v_modelPosition.xyz, lower);
    float coarse = grainAt(v_modelPosition.xyz, lower + 1.0f);
    // The mix of two grains is softer: its spread is brought back to that of one
    float spread = sqrt((1.0f - blend) * (1.0f - blend) + blend * blend);
    float noise = clamp((mix(fine, coarse, blend) - 0.5f) / spread + 0.5f, 0.0f, 1.0f);

    gl_FragColor = vec4(baseColor.rgb * (0.5f + noise * 0.5f), baseColor.a);
}
