#version 330 core

in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_scene;
uniform float     u_threshold;  // e.g. 0.8

void main()
{
    vec3  col = texture(u_scene, v_uv).rgb;
    float lum = dot(col, vec3(0.2126, 0.7152, 0.0722));

    // soft knee
    float knee = u_threshold * 0.5;
    float w    = clamp((lum - (u_threshold - knee)) / max(knee * 2.0, 0.0001), 0.0, 1.0);

    fragColor = vec4(col * w, 1.0);
}
