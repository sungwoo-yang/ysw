#version 330 core
// 레이저 빌보드 버텍스 셰이더.
// CPU에서 쿼드(4 vertex) 를 넘겨줌:
//   vertex.xy  = 월드 위치 (레이저 방향에 수직으로 ±width/2 벌려놓은 것)
//   vertex.z   = 0 (2D)
//   texcoord.x = 레이저 축방향 0→1
//   texcoord.y = 수직방향  0=가장자리  0.5=중앙  1=가장자리

layout(location = 0) in vec3  a_pos;
layout(location = 1) in vec2  a_uv;

out vec2 v_uv;

uniform mat4 u_mvp;

void main()
{
    v_uv        = a_uv;
    gl_Position = u_mvp * vec4(a_pos, 1.0);
}
