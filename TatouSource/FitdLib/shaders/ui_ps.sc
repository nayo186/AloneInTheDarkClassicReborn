$input v_texcoord0

#include "bgfx_shader.sh"
#include "palette.sh"

USAMPLER2D(s_backgroundTexture, 0);
uniform vec4 u_fadeLevel;   // x: brightness (the texts outside the scene, zoomed aspect ratios, follow the screen's fades)

void main()
{
    vec2 position = vec2(v_texcoord0.xy) * vec2(320, 200);
    uvec4 rawTexel = texelFetch(s_backgroundTexture, ivec2(position), 0);
    if(rawTexel.r == uint(0))
        discard;
    vec4 color = getColorFromRawOffset(rawTexel.r);
    gl_FragColor = vec4(color.rgb * u_fadeLevel.x, color.a);
}
