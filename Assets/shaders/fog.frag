#version 330 core
// Post-process fog. fullscreen.vert와 함께 사용.
// 씬 렌더 후 FBO → 이 셰이더로 blit.

in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_scene;
uniform sampler2D u_depth;       // GL_DEPTH_COMPONENT 텍스처

uniform vec3  u_fog_color;
uniform float u_fog_density;     // EXP2 밀도
uniform float u_fog_start;       // LINEAR 시작 거리 (월드 단위)
uniform float u_fog_end;         // LINEAR 끝  거리
uniform float u_near;            // 카메라 near plane
uniform float u_far;             // 카메라 far  plane
uniform int   u_fog_mode;        // 0=EXP2  1=EXP  2=LINEAR

float linearize_depth(float d)
{
    // NDC [-1,1] depth → 선형 뷰 공간 거리
    return (2.0 * u_near * u_far) /
           (u_far + u_near - (d * 2.0 - 1.0) * (u_far - u_near));
}

void main()
{
    vec3  scene = texture(u_scene, v_uv).rgb;
    float raw   = texture(u_depth, v_uv).r;
    float depth = linearize_depth(raw);

    float fog_factor;
    if (u_fog_mode == 0)
    {
        // FOG_EXP2: 가장 자연스러움
        float f = u_fog_density * depth;
        fog_factor = exp2(-f * f);
    }
    else if (u_fog_mode == 1)
    {
        // FOG_EXP
        fog_factor = exp2(-u_fog_density * depth);
    }
    else
    {
        // FOG_LINEAR
        fog_factor = clamp((u_fog_end - depth) /
                           max(u_fog_end - u_fog_start, 0.0001), 0.0, 1.0);
    }

    vec3 col = mix(u_fog_color, scene, clamp(fog_factor, 0.0, 1.0));
    fragColor = vec4(col, 1.0);
}
