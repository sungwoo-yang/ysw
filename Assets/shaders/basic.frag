#version 300 es
precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

uniform sampler2D uTexture;

in vec2 vTexCoord;

layout(location = 0) out vec4 FragColor;

uniform vec4 uTint;

void main()
{
    // FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    FragColor = texture(uTexture, vTexCoord) * uTint;
    // FragColor = vec4(vTexCoord, 0.0, 1.0);
    if(FragColor.a == 0.0)
        discard;
}
