#version 300 es

/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec4 a_color;
layout (location = 2) in vec2 a_tex_coord;
layout (location = 3) in float a_tex_id;

uniform mat3 u_ndc_matrix;

out vec2 v_uv;
out vec4 v_color;
flat out float v_tex_id;

void main()
{
    v_uv = a_tex_coord;
    v_color = a_color;
    v_tex_id = a_tex_id;
    vec3 ndc_pos = u_ndc_matrix * vec3(a_position, 1.0); 
    gl_Position = vec4(ndc_pos.xy, 0.0, 1.0);
}
