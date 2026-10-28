$input v_texcoord0

// Sides of a custom background wider than 4:3, drawn in the black bars of
// fullscreen (screenEffects.cpp), with the screen fade

#include <bgfx_shader.sh>

SAMPLER2D(s_fxScene, 0);

uniform vec4 u_fxComposite;  // w: fade (0 black, 1 normal)

void main()
{
    gl_FragColor = vec4(texture2D(s_fxScene, v_texcoord0).rgb * u_fxComposite.w, 1.0);
}
