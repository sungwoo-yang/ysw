#version 330 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D u_screenTexture;
uniform float u_playerHp;
uniform float u_maxHp;
uniform float u_time;

float random(vec2 st)
{
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float noise(vec2 st)
{
    vec2 i = floor(st);
    vec2 f = fract(st);

    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

float fbm(vec2 st)
{
    float value = 0.0;
    float amplitude = 0.5;

    for (int i = 0; i < 4; i++)
    {
        value += amplitude * noise(st);
        st *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}

void main()
{
    vec3 baseColor = texture(u_screenTexture, vTexCoord).rgb;

    float healthRatio = clamp(u_playerHp / u_maxHp, 0.0, 1.0);

    if (healthRatio >= 0.99)
    {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    vec2 uv = vTexCoord;
    float dist = distance(uv, vec2(0.5, 0.5));

    vec2 noiseUV = uv * 3.0;
    float n = fbm(noiseUV + vec2(0.0, -u_time * 1.5));

    float organicShape = dist + (n * 0.4) - 0.2;

    vec3 healthyColor = vec3(0.05, 0.0, 0.0);
    vec3 dangerColor  = vec3(0.7, 0.0, 0.0);
    vec3 edgeColor    = mix(dangerColor, healthyColor, healthRatio);

    float effectSpread = mix(0.1, 0.7, healthRatio);
    float intensity    = mix(1.0, 0.0, healthRatio);

    float vignette = smoothstep(effectSpread, effectSpread + 0.3, organicShape) * intensity;

    float pulse = (sin(u_time * 6.0) * 0.5 + 0.5) * 0.3 + 0.7;
    vignette *= mix(1.0, pulse, 1.0 - healthRatio);

    vec3 finalColor = mix(baseColor, edgeColor, vignette);

    FragColor = vec4(finalColor, 1.0);
}