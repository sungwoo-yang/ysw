#version 300 es

/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout (location = 0) in vec2 a_vertex_pos;
layout (location = 1) in vec2 a_vertex_uv;
layout (location = 2) in vec3 a_model_row0;   
layout (location = 3) in vec3 a_model_row1;   
layout (location = 4) in vec4 a_color;     
layout (location = 5) in vec2 a_uv_scale;     
layout (location = 6) in vec2 a_uv_offset;   
layout (location = 7) in int a_tex_id;

uniform mat3 u_ndc_matrix;

out vec2 v_uv;
flat out vec4 v_color;
flat out int v_tex_id;

void main()
{
    vec2 world_position;
    world_position.x = a_vertex_pos.x * a_model_row0[0] + a_vertex_pos.y * a_model_row0[1] + a_model_row0[2];
    world_position.y = a_vertex_pos.x * a_model_row1[0] + a_vertex_pos.y * a_model_row1[1] + a_model_row1[2];

    vec3 ndc_pos = u_ndc_matrix * vec3(world_position, 1.0);
    gl_Position = vec4(ndc_pos.xy, 0.0, 1.0);

    v_uv = a_vertex_uv * a_uv_scale + a_uv_offset;

    v_color = a_color;
    v_tex_id = a_tex_id;
}
