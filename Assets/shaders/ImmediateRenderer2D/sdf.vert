#version 300 es

/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout (location = 0) in vec2 a_position;

uniform mat3 u_ndc_matrix;
uniform mat3 u_model_matrix;
uniform vec2 u_world_size;
uniform vec2 u_quad_size;

out vec2 v_sdf_coord;

void main()
{
    v_sdf_coord = a_position;
    vec3 ndc_pos = u_ndc_matrix * u_model_matrix * vec3(a_position, 1.0);
    gl_Position = vec4(ndc_pos.xy, 0.0, 1.0);
}
