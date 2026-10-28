$input a_position, a_texcoord0
$output v_texcoord0

// Full-screen quad of the screen effects (screenEffects.cpp)

#include <bgfx_shader.sh>

void main()
{
    gl_Position = vec4(a_position.xy, 0.0, 1.0);
    v_texcoord0 = a_texcoord0;
}
