$input a_position, a_texcoord0, a_texcoord1
$output v_texcoord0, v_screenSpacePosition, v_modelPosition

#include "bgfx_shader.sh"

// Painter's order (Original mode): x = slice of this object (0 = nearest),
// y = size of a slice (1 = no slices, the plain depth)
uniform vec4 u_depthSlice;

void main()
{
    gl_Position = vec4(a_position.x/160.0-1.0, 1.0-a_position.y/100.0, (u_depthSlice.x + a_position.z/40960.f) * u_depthSlice.y, 1.0);
    v_texcoord0 = a_texcoord0;
    v_screenSpacePosition = a_position.xyz;
    v_modelPosition = a_texcoord1;
}
