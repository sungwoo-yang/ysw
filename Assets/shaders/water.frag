#version 330 core
// Procedural water — no external textures required.
// v_uv.y : 0 = zone bottom,  1 = water surface

in  vec2 v_uv;
out vec4 fragColor;

uniform float u_time;

void main()
{
    float x = v_uv.x;
    float y = v_uv.y;   // depth ratio: 0=bottom, 1=surface

    // ── Wave layers ──────────────────────────────────────────────────────────
    float w1 = sin(x * 14.0 + u_time * 1.9) * 0.5 + 0.5;
    float w2 = sin(x *  7.5 - u_time * 1.2 + 1.57) * 0.5 + 0.5;
    float w3 = sin(x * 22.0 + u_time * 2.8 + x * 3.0) * 0.5 + 0.5;
    float wave = w1 * 0.5 + w2 * 0.35 + w3 * 0.15;

    // ── Depth colour ─────────────────────────────────────────────────────────
    vec3 deep    = vec3(0.00, 0.05, 0.28);
    vec3 mid     = vec3(0.03, 0.22, 0.60);
    vec3 surface = vec3(0.10, 0.42, 0.82);
    vec3 col     = mix(deep, mid, y * y);
    col          = mix(col, surface, y * y * y);

    // ── Wave shimmer (bright ribbons near surface) ────────────────────────────
    float shimmer = w1 * w2;
    col += vec3(0.04, 0.16, 0.35) * shimmer * (y * y) * 1.4;

    // ── Caustics (internal light scatter deeper) ──────────────────────────────
    float caus = abs(sin(x * 18.0 + u_time * 0.6 + y * 8.0)) * 0.18
               + abs(sin(x * 11.0 - u_time * 0.9 + y * 5.0)) * 0.10;
    col += caus * (1.0 - y) * vec3(0.05, 0.18, 0.35);

    // ── Surface foam strip (top 8%) ───────────────────────────────────────────
    float surfaceEdge = clamp((y - 0.92) * 12.5, 0.0, 1.0);
    float foam        = step(0.70, wave) * surfaceEdge;
    col = mix(col, vec3(0.65, 0.85, 1.00), foam * 0.55);

    // ── Alpha: opaque near surface, more transparent at bottom ───────────────
    float alpha = mix(0.52, 0.88, y);
    fragColor = vec4(col, alpha);
}
