$input a_position, a_texcoord0
$output v_texcoord0

#include "bgfx_shader.sh"

// Painter's order (Original mode, rendererBGFX.cpp): z = 1 while drawing an
// object, then a mask goes to the front of its slice (in front of the
// object and everything farther, behind nearer objects); otherwise far
uniform vec4 u_depthSlice;

void main()
{
    gl_Position = vec4(a_position.x/160.0-1.0, 1.0-a_position.y/100.0, (u_depthSlice.z > 0.5) ? u_depthSlice.x * u_depthSlice.y : 1.0, 1.0);
    v_texcoord0 = a_texcoord0;
}
