#version 330 core

// 버텍스 버퍼 없이 gl_VertexID만으로 fullscreen triangle 생성
// glDrawArrays(GL_TRIANGLES, 0, 3) 으로 호출

out vec2 v_uv;

void main()
{
    // 0 → (-1,-1,0)  uv(0,0)
    // 1 → ( 3,-1,0)  uv(2,0)
    // 2 → (-1, 3,0)  uv(0,2)
    vec2 pos = vec2(
        (gl_VertexID == 1) ? 3.0 : -1.0,
        (gl_VertexID == 2) ? 3.0 : -1.0
    );
    v_uv        = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
