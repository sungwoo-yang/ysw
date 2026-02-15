#version 330 core

out vec4 FragColor;

uniform vec2 u_resolution; 
uniform float u_time;
uniform vec2 u_starPos;    // Light Source Position
// Removed u_charPos, u_charSize as we don't calculate shadow in shader anymore

void main()
{
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;

    // --- Sky Gradient ---
    vec3 colTop = vec3(0.00, 0.005, 0.03);
    vec3 colMid = vec3(0.01, 0.06, 0.22);
    vec3 colBot = vec3(0.18, 0.08, 0.35);

    float topMixFactor = pow(uv.y, 2.5);
    vec3 baseSky = mix(colMid, colTop, topMixFactor);

    float bottomPresence = 1.0 - uv.y;
    bottomPresence = pow(bottomPresence, 8.0);

    vec3 finalColor = mix(baseSky, colBot, bottomPresence);
    finalColor *= 0.9;

    // --- Ground & Light Logic ---
    float groundHeight = 0.30;

    if (uv.y < groundHeight)
    {
        // 1. Base Ground Color
        vec3 groundBase = vec3(0.02, 0.02, 0.05);

        // 2. Light Calculation
        float dist = distance(gl_FragCoord.xy, u_starPos);
        float lightRadius = u_resolution.y * 0.8; 
        float intensity = smoothstep(lightRadius, 0.0, dist);
        
        float horizonDist = abs(uv.y - groundHeight);
        float rimLight = smoothstep(0.05, 0.0, horizonDist) * intensity * 0.5;

        vec3 lightColor = vec3(1.0, 0.8, 0.4);

        // removed shadow logic block

        finalColor = groundBase + (lightColor * intensity * 0.3) + (lightColor * rimLight);
    }

    FragColor = vec4(finalColor, 1.0);
}