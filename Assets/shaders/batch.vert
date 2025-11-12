#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

layout(location = 0) in vec2 aWorldPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aTint;
layout(location = 3) in int aTextureIndex;

out vec2 vTexCoord;
flat out vec4 vTint; // flat means don't do interpolation
flat out int vTexureIndex;

uniform mat3 uToNDC;

void main()
{
    vec3 ndc_point = uToNDC * vec3(aWorldPosition, 1.0);
    gl_Position = vec4(ndc_point.xy, 0.0, 1.0);
    vTexCoord = aTexCoord;
    vTint = aTint;
    vTexureIndex = aTextureIndex;
}
