$input v_texcoord0

// RGBA picture over the play area (frozen scene behind the menus, sequence
// frames replaced by PNG files), darkened by the fade level

#include "bgfx_shader.sh"

SAMPLER2D(s_backgroundTexture, 0);
uniform vec4 u_fadeLevel;       // x = fade level (0.0 = black, 1.0 = full brightness)

void main()
{
    vec4 color = texture2D(s_backgroundTexture, v_texcoord0);
    gl_FragColor = vec4(color.rgb * u_fadeLevel.x, color.a);
}
