#version 330 core
// God Rays (빛줄기) 포스트 프로세싱.
// fullscreen.vert와 함께 사용.
//
// 렌더 순서:
//   1. 씬을 sceneFbo에 렌더
//   2. occluderFbo에 "가리는 오브젝트만" 검정으로 렌더
//      (하늘/빛 배경은 흰색, 오브젝트는 검정)
//   3. 이 셰이더로 occluder 텍스처 → god rays 텍스처 생성
//   4. god rays를 씬 위에 가산 블렌딩으로 합성

in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_occlusion;   // occluder 마스크 텍스처

// 빛 위치 (스크린 공간 UV, 0~1)
uniform vec2  u_light_pos;

// 방사형 블러 파라미터
uniform int   u_num_samples;     // 샘플 수 [32 ~ 128], 기본 64
uniform float u_density;         // 샘플 간격 밀도  [0.5 ~ 1.0]
uniform float u_weight;          // 각 샘플 가중치  [0.005 ~ 0.02]
uniform float u_decay;           // 거리별 감쇠     [0.95 ~ 0.99]
uniform float u_exposure;        // 최종 노출       [0.1 ~ 1.0]

void main()
{
    vec2  uv       = v_uv;
    vec2  delta    = (uv - u_light_pos) * (1.0 / float(u_num_samples)) * u_density;
    float illumination = 1.0;
    vec3  color    = vec3(0.0);

    for (int i = 0; i < u_num_samples; i++)
    {
        uv    -= delta;
        vec3  s = texture(u_occlusion, uv).rgb;
        s      *= illumination * u_weight;
        color  += s;
        illumination *= u_decay;
    }

    fragColor = vec4(color * u_exposure, 1.0);
}
