#version 330 core
// 레이저 코어 (Pass 1).
// 블렌딩: glBlendFunc(GL_ONE, GL_ONE)  ← 완전 가산
// Internal-Flare 방식과 동일.

in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_noise_tex;   // 에너지 흐름용 노이즈 텍스처 (선택)
uniform vec4      u_color;       // 레이저 색상 (HDR 허용, w=강도)
uniform float     u_time;        // 시간 (에너지 스크롤용)
uniform float     u_core_width;  // 코어 날카로움 [0.01 ~ 0.5]
uniform float     u_noise_strength; // 노이즈 강도 [0 ~ 0.3]

void main()
{
    // v_uv.y: 0=가장자리, 0.5=중앙, 1=가장자리  → 중앙 기준 거리
    float dist_from_center = abs(v_uv.y - 0.5) * 2.0;  // [0, 1]

    // 코어: 날카로운 gaussian
    float core = exp2(-dist_from_center * dist_from_center
                      / max(u_core_width * u_core_width, 0.0001));

    // 에너지 스크롤 노이즈 (노이즈 텍스처 없으면 무시됨)
    vec2  noise_uv = vec2(v_uv.x - u_time * 0.5, v_uv.y);
    float noise    = texture(u_noise_tex, noise_uv).r;
    core *= 1.0 + noise * u_noise_strength;

    vec3 col = u_color.rgb * u_color.w * core;
    fragColor = vec4(col, 1.0);
}
