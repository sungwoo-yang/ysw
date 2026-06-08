#version 330 core
// 레이저 글로우 (Pass 2).
// 블렌딩: glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE)  ← Internal-Halo 방식
// 이미 렌더된 코어 위에 그리면 부드러운 발광이 됨.

in  vec2 v_uv;
out vec4 fragColor;

uniform vec4  u_glow_color;    // 글로우 색상 (w=강도)
uniform float u_glow_radius;   // 글로우 퍼짐 [0.1 ~ 1.0]
uniform float u_glow_falloff;  // 감쇠 지수, 클수록 날카로움 [1.0 ~ 4.0]

void main()
{
    float dist = abs(v_uv.y - 0.5) * 2.0;

    // 소프트 글로우: pow falloff
    float glow = pow(max(1.0 - dist / max(u_glow_radius, 0.001), 0.0),
                     u_glow_falloff);

    // OneMinusDstColor blend에서는 alpha도 같이 써야 효과가 맞음
    float intensity = glow * u_glow_color.w;
    fragColor = vec4(u_glow_color.rgb * intensity, intensity);
}
