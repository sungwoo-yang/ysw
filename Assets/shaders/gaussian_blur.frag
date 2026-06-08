#version 330 core

in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_tex;
uniform vec2      u_texel_size;  // 1/width, 1/height
uniform vec2      u_direction;   // (1,0) = horizontal, (0,1) = vertical

// 9-tap gaussian weights (sigma ≈ 1.5)
const float W[5] = float[](0.2270270270, 0.1945945946, 0.1216216216,
                            0.0540540541, 0.0162162162);

void main()
{
    vec2  step = u_texel_size * u_direction;
    vec3  col  = texture(u_tex, v_uv).rgb * W[0];

    for (int i = 1; i < 5; ++i)
    {
        col += texture(u_tex, v_uv + step * float(i)).rgb * W[i];
        col += texture(u_tex, v_uv - step * float(i)).rgb * W[i];
    }

    fragColor = vec4(col, 1.0);
}
