#version 300 es

/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_tex_coord;

uniform mat3 u_ndc_matrix;
uniform mat3 u_model_matrix;
uniform mat3 u_uv_matrix;

out vec2 v_uv;

void main()
{
    v_uv = (u_uv_matrix * vec3(a_tex_coord, 1.0)).xy;
    vec3 ndc_pos = u_ndc_matrix * u_model_matrix * vec3(a_position, 1.0);
    gl_Position = vec4(ndc_pos.xy, 0.0, 1.0);
}
