#version 300 es
precision mediump float;

/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

in vec2 v_sdf_coord;

uniform int u_shape_type;
uniform vec4 u_fill_color;
uniform vec4 u_line_color;
uniform float u_line_width;
uniform highp vec2 u_world_size;
uniform highp vec2 u_quad_size;

out vec4 frag_color;

float circle_sdf(vec2 p, float radius)
{
    return length(p) - radius;
}

float rectangle_sdf(vec2 p, vec2 half_size)
{
    vec2 d = abs(p) - half_size;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

void main()
{
    vec2 scaled_pos = v_sdf_coord * u_quad_size;
    vec2 half_dims = u_world_size * 0.5;

    float dist;
    if (u_shape_type == 0)
    {
        dist = circle_sdf(scaled_pos, half_dims.x);
    }
    else
    {
        dist = rectangle_sdf(scaled_pos, half_dims);
    }

    float half_line_width = u_line_width * 0.5;
    float inner_edge = -half_line_width;
    float outer_edge = half_line_width;
    
    vec4 shape_color;
    
    if(u_line_color.a < 0.01)
    {
        shape_color = u_fill_color;
        outer_edge = 0.0;
    }
    else if(u_fill_color.a < 0.01)
    {
        float line_mix = smoothstep(inner_edge, inner_edge + fwidth(dist), dist);
        shape_color = mix(vec4(0.0), u_line_color, line_mix);
    }
    else
    {
        float inner_mix = smoothstep(inner_edge, inner_edge + fwidth(dist), dist);
        shape_color = mix(u_fill_color, u_line_color, inner_mix);
    }   

    float outer_mix = smoothstep(outer_edge, outer_edge + fwidth(dist), dist);
    frag_color = mix(shape_color, vec4(0.0), outer_mix);

    if (frag_color.a < 0.01)
    {
        discard;
    }
}
