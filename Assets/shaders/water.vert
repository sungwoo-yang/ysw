#version 330 core
// 2D water quad — positions are already in NDC (pre-multiplied by VP on CPU).
layout(location = 0) in vec2 a_ndc_pos;
layout(location = 1) in vec2 a_uv;       // (0,0)=bottom-left  (1,1)=top-right

out vec2 v_uv;

void main()
{
    v_uv        = a_uv;
    gl_Position = vec4(a_ndc_pos, 0.0, 1.0);
}
