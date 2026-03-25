#version 300 es
precision mediump float;

out vec4 FragColor;

in vec2 vTexCoord;

uniform vec2 u_resolution;
uniform float u_time;

void main() {
    vec2 st = vTexCoord;

    vec3 colorSky = vec3(0.392, 0.710, 0.965); 
    vec3 colorDeep = vec3(0.102, 0.137, 0.494);

    float mixValue = smoothstep(0.0, 1.2, st.y + 0.2);
    
    float glow = sin(u_time * 0.5) * 0.05;
    mixValue += glow;

    vec3 finalColor = mix(colorSky, colorDeep, mixValue);

    FragColor = vec4(finalColor, 1.0);
}