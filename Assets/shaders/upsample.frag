#version 330 core

in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_tex;
uniform vec2      u_texel_size;
uniform float     u_radius;  // e.g. 0.5 ~ 1.0

// 3x3 tent filter upsample
void main()
{
    vec2 t = u_texel_size * u_radius;
    vec3 s = vec3(0.0);

    s += texture(u_tex, v_uv + vec2(-t.x,  t.y)).rgb;
    s += texture(u_tex, v_uv + vec2( 0.0,  t.y * 2.0)).rgb * 2.0;
    s += texture(u_tex, v_uv + vec2( t.x,  t.y)).rgb;
    s += texture(u_tex, v_uv + vec2(-t.x * 2.0, 0.0)).rgb * 2.0;
    s += texture(u_tex, v_uv).rgb * 4.0;
    s += texture(u_tex, v_uv + vec2( t.x * 2.0, 0.0)).rgb * 2.0;
    s += texture(u_tex, v_uv + vec2(-t.x, -t.y)).rgb;
    s += texture(u_tex, v_uv + vec2( 0.0, -t.y * 2.0)).rgb * 2.0;
    s += texture(u_tex, v_uv + vec2( t.x, -t.y)).rgb;

    fragColor = vec4(s / 16.0, 1.0);
}
