#version 300 es
precision mediump float;

out vec4 FragColor;
in  vec2 vTexCoord;

uniform vec2  u_resolution;
uniform float u_time;
uniform vec2  u_camPos;      // camera bottom-left in world pixels
uniform float u_parallax;    // 0=screen-fixed  1=world-fixed  (default 0.25)

// Hash for per-brick variation
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

void main() {
    vec2 st = vTexCoord;

    // ── World-space offset (parallax) ─────────────────────────────────────
    // Shift brick UVs by camera movement so the background scrolls with the world.
    // u_parallax = 0.25 → background moves at 25% of camera speed (distant wall).
    vec2 worldOffset = (u_camPos / u_resolution) * u_parallax;
    vec2 stBrick = st + worldOffset;   // world-shifted UVs for the brick pattern

    // ── Brick pattern ─────────────────────────────────────────────────────
    // Small bricks — as if wall is far in the background
    vec2 brickScale = vec2(32.0, 18.0);
    vec2 brickUV    = stBrick * brickScale;   // uses world-shifted UVs

    float row       = floor(brickUV.y);
    float offset    = mod(row, 2.0) * 0.5;
    vec2  cell      = vec2(brickUV.x + offset, brickUV.y);
    vec2  f         = fract(cell);

    // Thin, soft joints at small scale
    float mx = smoothstep(0.0, 0.06, f.x) * smoothstep(1.0, 0.94, f.x);
    float my = smoothstep(0.0, 0.09, f.y) * smoothstep(1.0, 0.91, f.y);
    float brickFace = mx * my;

    // Very subtle per-brick variation (far away = detail compressed)
    float var = hash(floor(cell)) * 0.025 - 0.012;

    // Stone and mortar nearly the same dark tone — low contrast = distance
    vec3 stoneCol  = vec3(0.18, 0.14, 0.11) + var;
    vec3 mortarCol = vec3(0.11, 0.09, 0.07);

    vec3 brickColor = mix(mortarCol, stoneCol, brickFace);

    // ── Ambient lighting ──────────────────────────────────────────────────
    // Warm torch glow pooling near the bottom-center
    vec2  torchUV   = vec2(st.x - 0.5, st.y);
    float torchDist = length(torchUV * vec2(0.8, 1.2));
    float torchGlow = exp(-torchDist * torchDist * 4.5) * 0.18;

    // Subtle flicker
    float flicker = 1.0
        + sin(u_time * 5.7 + 0.3) * 0.025
        + sin(u_time * 11.3)       * 0.012;
    torchGlow *= flicker;

    vec3 torchColor = vec3(0.55, 0.28, 0.06) * torchGlow;

    // Cold ambient from above (magical faint blue)
    float skyAmbient = st.y * st.y * 0.04;
    vec3  skyColor   = vec3(0.10, 0.12, 0.22) * skyAmbient;

    // ── Vignette ──────────────────────────────────────────────────────────
    vec2  vd       = st * 2.0 - 1.0;
    float vignette = 1.0 - dot(vd * vec2(0.45, 0.65), vd * vec2(0.45, 0.65));
    vignette       = pow(clamp(vignette, 0.0, 1.0), 0.6);

    // ── Combine ───────────────────────────────────────────────────────────
    vec3 col = brickColor * 0.55        // brick, kept dark
             + torchColor               // warm pool of light
             + skyColor;                // faint cool ambient
    col *= (vignette * 0.55 + 0.45);    // vignette darkens edges

    FragColor = vec4(col, 1.0);
}
