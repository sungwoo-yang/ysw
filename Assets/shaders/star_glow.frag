#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D u_texture;
uniform float u_time;

void main()
{
    vec4 texColor = texture(u_texture, TexCoord);

    // 1. Pulsating effect (Time based)
    float pulse = (sin(u_time * 2.0) + 1.0) * 0.5; // 0.0 ~ 1.0
    pulse = 0.8 + (pulse * 0.2); // 0.8 ~ 1.0 (Minimum brightness guaranteed)

    // 2. Glow calculation (Distance from center)
    // Assumes UV is 0.0~1.0, Center is 0.5, 0.5
    float dist = distance(TexCoord, vec2(0.5));
    float glowRadius = 0.6;
    // Stronger glow near center, fades out
    float glowIntensity = smoothstep(glowRadius, 0.0, dist); 
    
    // 3. Gold/Yellow Glow Color
    vec3 glowColor = vec3(1.0, 0.8, 0.4); 

    // Combine: Texture Color * Pulse + Glow * Pulse
    vec3 finalRGB = (texColor.rgb * pulse) + (glowColor * glowIntensity * 0.5 * pulse);

    // Alpha handling (Texture alpha + Glow alpha)
    float finalAlpha = texColor.a + (glowIntensity * 0.5);
    
    FragColor = vec4(finalRGB, clamp(finalAlpha, 0.0, 1.0));
}