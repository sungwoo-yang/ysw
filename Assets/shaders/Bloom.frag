#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D u_SceneTexture;
uniform sampler2D u_BloomMaskTexture;

void main()
{
    vec4 baseColor = texture(u_SceneTexture, TexCoords);
    vec4 maskPixel = texture(u_BloomMaskTexture, TexCoords);

    if (maskPixel.a > 0.1)
    {
        baseColor.rgb = mix(baseColor.rgb, vec3(1.0, 1.0, 1.0), 0.3);
    }

    vec3 glow = vec3(0.0);
    vec2 texOffset = 1.0 / textureSize(u_SceneTexture, 0);

    float blurRadius = 1.8;
    float totalWeight = 0.0;
    vec3 lightTint = vec3(0.6, 0.9, 1.0);

    for(float x = -6.0; x <= 6.0; x += 1.0)
    {
        for(float y = -6.0; y <= 6.0; y += 1.0)
        {
            vec2 offset = vec2(x, y) * texOffset * blurRadius;
            vec3 sampleColor = texture(u_BloomMaskTexture, TexCoords + offset).rgb;

            float weight = exp(-(x * x + y * y) / 16.0);

            glow += sampleColor * lightTint * weight;
            totalWeight += weight;
        }
    }

    glow /= totalWeight;

    float glowStrength = 2.5;
    vec3 finalColor = baseColor.rgb + (glow * glowStrength);

    FragColor = vec4(finalColor, baseColor.a);
}