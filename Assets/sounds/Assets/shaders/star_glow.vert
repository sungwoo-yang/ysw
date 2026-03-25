#version 300 es
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 u_transform;
uniform mat4 u_projection;

void main()
{
    gl_Position = u_projection * u_transform * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}