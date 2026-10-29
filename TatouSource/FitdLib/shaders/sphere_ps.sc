$input v_texcoord0, v_sphereParams, v_screenSpacePosition

#include "bgfx_shader.sh"
#include "palette.sh"

// Spheres and points: their colour from the palette (colorIdx/15, bank/15 in
// the UVs), by material

void main()
{
    vec2 sphereCenter = v_sphereParams.xy;
    float sphereSize = v_sphereParams.z;

    float distanceToCircleOnScanline;
    vec2 normalizedPosition = (v_screenSpacePosition.xy - sphereCenter.xy) / sphereSize;

    float xScalingRatio = 5.f/6.f;

    normalizedPosition.x *= xScalingRatio;
    float distToCenter = length(normalizedPosition);
    if(distToCenter > 1.f)
        discard;

    int material = int(v_sphereParams.w);

    int color = int(v_texcoord0.x * 15.f);
    int bank = int(v_texcoord0.y * 15.f);

    switch(material) {
    case 0: // flat
        gl_FragColor = getColor(bank, color);
        break;
    case 2: //transparent
        gl_FragColor = getColor(bank, color);
        gl_FragColor.w = 0.5f;
        break;
    case 3: //marbre
        float angle = asin(normalizedPosition.y);
        distanceToCircleOnScanline = ((normalizedPosition.x / cos(angle)) / 2.f) + 0.5f;
        color = int((distanceToCircleOnScanline) * 15.f);
        gl_FragColor = getColor(bank, color);
        break;
    default:
        gl_FragColor = getColor(bank, color);
        break;
    }
}
