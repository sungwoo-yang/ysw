#version 300 es

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

// per vertex
layout(location = 0) in vec2 aModelPosition;
layout(location = 1) in vec2 aTexCoord;

// per instance
layout(location = 2) in vec3 aModelRow0;
layout(location = 3) in vec3 aModelRow1;
layout(location = 4) in vec4 aTint;
layout(location = 5) in vec2 aTexCoordScale;
layout(location = 6) in vec2 aTexCoordOffset;
layout(location = 7) in int aTextureIndex;

out vec2 vTexCoord;
flat out vec4 vTint; // flat means don't do interpolation
flat out int vTexureIndex;

uniform mat3 uToNDC;

void main()
{
    vec2 world_position;
    world_position.x = aModelPosition.x * aModelRow0[0] + aModelPosition.y * aModelRow0[1] + aModelRow0[2];
    world_position.y = aModelPosition.x * aModelRow1[0] + aModelPosition.y * aModelRow1[1] + aModelRow1[2];
    vec3 ndc_point = uToNDC * vec3(world_position, 1.0);
    gl_Position = vec4(ndc_point.xy, 0.0, 1.0);
    vTexCoord = aTexCoord * aTexCoordScale + aTexCoordOffset;
    vTint = aTint;
    vTexureIndex = aTextureIndex;
}
