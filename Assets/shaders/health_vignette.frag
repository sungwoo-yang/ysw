#version 330 core
// Health vignette — edge-only, 5-stage progression.
// Effect is ONLY at screen edges (bottom/sides), never in the interior.

in  vec2 v_uv;
out vec4 fragColor;

uniform float u_hp;
uniform float u_max_hp;
uniform float u_time;
uniform float u_blackout;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i),             hash(i + vec2(1,0)), f.x),
               mix(hash(i+vec2(0,1)),   hash(i + vec2(1,1)), f.x), f.y);
}

float fbm(vec2 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 4; i++) {
        v += a * noise(p);
        p  = p * 2.1 + vec2(0.31, 0.17);
        a *= 0.5;
    }
    return v;
}

void main()
{
    // Death blackout
    if (u_blackout > 0.001) {
        fragColor = vec4(0.0, 0.0, 0.0, clamp(u_blackout, 0.0, 1.0));
        return;
    }

    float hr     = clamp(u_hp / u_max_hp, 0.0, 1.0);
    float damage = 1.0 - hr;

    if (hr >= 0.999) { fragColor = vec4(0.0); return; }

    // ── How far the effect extends from each edge ────────────────────────────
    // damage 0.2 → 18%, 0.4 → 27%, 0.6 → 36%, 0.8 → 50%
    // Wider reach for stages 1-3, max for stage 4
    float reach = mix(0.30, 0.60, damage);

    // ── Per-edge distance from edge (0=at edge, 1=reach away) ───────────────
    float distB = v_uv.y;                          // 0 at bottom
    float distT = 1.0 - v_uv.y;                   // 0 at top
    float distL = v_uv.x;                          // 0 at left
    float distR = 1.0 - v_uv.x;                   // 0 at right

    // Sharp edge strip masks — 1 at edge, 0 at 'reach' distance
    float maskB = clamp(1.0 - distB / reach, 0.0, 1.0);
    float maskT = clamp(1.0 - distT / (reach * 0.5), 0.0, 1.0);  // top: half-reach
    float maskS = max(clamp(1.0 - distL / (reach * 0.6), 0.0, 1.0),
                      clamp(1.0 - distR / (reach * 0.6), 0.0, 1.0));

    float edgeMask = clamp(max(max(maskB, maskT), maskS), 0.0, 1.0);

    // No mask → no effect (hard cutoff prevents interior bleed)
    if (edgeMask < 0.001) { fragColor = vec4(0.0); return; }

    // ── Animated smoke texture (slow upward drift) ───────────────────────────
    float animT  = u_time * 0.05;
    vec2  fUV    = vec2(v_uv.x * 2.6, (1.0 - v_uv.y) * 2.0 - animT);
    float fN     = fbm(fUV + vec2(sin(animT * 0.3) * 0.08, 0.0));

    // Secondary layer for depth
    vec2  fUV2   = vec2(v_uv.x * 3.2 + 0.5, v_uv.y * 1.7 + animT * 0.4);
    float fN2    = fbm(fUV2);

    float smokeVal = fN * 0.6 + fN2 * 0.4;

    // ── Threshold: lower → more smoke visible ───────────────────────────────
    // Stage 1(0.2): thresh=0.52, Stage 4(0.8): thresh=0.18
    float thresh = mix(0.52, 0.18, damage);
    float mist   = smoothstep(thresh - 0.05, thresh + 0.22, smokeVal * edgeMask);
    mist         = clamp(mist, 0.0, 1.0);

    // ── Color: black → dark red → bright red by stage ───────────────────────
    // damage: 0.2=stage1, 0.4=stage2, 0.6=stage3, 0.8=stage4
    vec3 col = vec3(damage * damage * 0.55, 0.0, 0.0);   // R only, quadratic ramp
    col     += smokeVal * vec3(0.06 * damage, 0.0, 0.0); // slight wisp brightness

    // ── Stage 4 flicker ──────────────────────────────────────────────────────
    float flicker = (damage >= 0.75) ? (0.72 + sin(u_time * 9.0) * 0.28) : 1.0;

    // Stronger alpha so stages 1-2 are actually visible
    float alpha = mist * mix(0.82, 0.95, damage) * flicker;

    fragColor = vec4(col, clamp(alpha, 0.0, 0.93));
}
