#version 330 core
out vec4 FragColor;

uniform vec3 color;
// 0.3f, 1.0f, 0.5f
// 1.0f, 0.3f, 0.5f

void main()
{
   FragColor = vec4(color, 1.f);
}