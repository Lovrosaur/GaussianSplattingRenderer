#version 430 core
out vec4 FragColor;

in vec4 outColor;
in vec2 uv;

uniform int effect;

void main()
{
    float power = -0.5 * dot(uv,uv);
    if (power < -4.5) discard;
    
    float alfa = exp(power) * outColor.a;
    if (alfa < 0.01) discard;

    if (effect == 1) {
        alfa *= 0.15;
        // alfa = (alfa + 0.3*fbm(vec3(uv.x,uv.y,power)));
    }
    FragColor = vec4(outColor.rgb, alfa);
}