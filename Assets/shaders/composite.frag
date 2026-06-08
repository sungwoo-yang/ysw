#version 330 core

in  vec2 v_uv;
out vec4 fragColor;

uniform sampler2D u_scene;
uniform sampler2D u_bloom;
uniform sampler2D u_grain;

// Bloom
uniform float u_bloom_intensity;

// Contrast / Brightness
uniform float u_contrast;    // default 1.0
uniform float u_brightness;  // default 0.0

// Per-channel bezier color correction (cubic, 4 control points each)
uniform vec4  u_bezier_r;
uniform vec4  u_bezier_g;
uniform vec4  u_bezier_b;

// Desaturation  [0,1]
uniform float u_desat;

// Film grain
uniform vec4  u_grain_offset_scale;  // xy=offset, zw=tiling scale

// Directional blur (motion blur)
uniform float u_blur_strength;
uniform vec2  u_blur_dir;    // normalized direction

// Twirl
uniform float u_twirl_angle;
uniform vec4  u_twirl_center_radius;  // xy=center uv, zw=radius

// ─── helpers ──────────────────────────────────────────────────────────────────

float bezier_remap(float t, vec4 c)
{
    // cubic bezier p0..p3 evaluated at t ∈ [0,1]
    float s  = clamp(t, 0.0, 1.0);
    float a  = mix(c.x, c.y, s);
    float b  = mix(c.y, c.z, s);
    float cc = mix(c.z, c.w, s);
    float d  = mix(a,   b,   s);
    float e  = mix(b,   cc,  s);
    return mix(d, e, s);
}

vec2 apply_twirl(vec2 uv)
{
    vec2  center = u_twirl_center_radius.xy;
    vec2  radius = u_twirl_center_radius.zw;
    vec2  delta  = uv - center;
    float dist   = length(delta / radius);
    float angle  = u_twirl_angle * clamp(1.0 - dist, 0.0, 1.0);
    float s = sin(angle), c = cos(angle);
    delta = vec2(c * delta.x - s * delta.y,
                 s * delta.x + c * delta.y);
    return center + delta;
}

void main()
{
    vec2 uv = v_uv;

    // 1) Twirl
    if (u_twirl_angle != 0.0)
        uv = apply_twirl(uv);

    // 2) Directional / motion blur  (8-tap)
    vec3 scene;
    if (u_blur_strength > 0.0)
    {
        scene = vec3(0.0);
        for (int i = 0; i < 8; ++i)
            scene += texture(u_scene, uv - u_blur_dir * u_blur_strength
                             * (float(i) / 7.0 - 0.5)).rgb;
        scene /= 8.0;
    }
    else
    {
        scene = texture(u_scene, uv).rgb;
    }

    // 3) Bloom add
    vec3 bloom = texture(u_bloom, uv).rgb;
    scene += bloom * u_bloom_intensity;

    // 4) Contrast + brightness
    scene = clamp(scene * u_contrast + u_brightness, 0.0, 1.0);

    // 5) Per-channel bezier color correction
    scene.r = bezier_remap(scene.r, u_bezier_r);
    scene.g = bezier_remap(scene.g, u_bezier_g);
    scene.b = bezier_remap(scene.b, u_bezier_b);

    // 6) Desaturation
    if (u_desat > 0.0)
    {
        float lum = dot(scene, vec3(0.2126, 0.7152, 0.0722));
        scene = mix(scene, vec3(lum), u_desat);
    }

    // 7) Film grain
    vec2  grain_uv = v_uv * u_grain_offset_scale.zw + u_grain_offset_scale.xy;
    float grain    = texture(u_grain, grain_uv).r * 2.0 - 1.0;
    scene += grain * 0.02;

    fragColor = vec4(clamp(scene, 0.0, 1.0), 1.0);
}
