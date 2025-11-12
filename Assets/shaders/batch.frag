#version 300 es

precision mediump float;

/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#ifndef MAX_TEXTURE_SLOTS
#    define MAX_TEXTURE_SLOTS 8
#endif

uniform sampler2D uTextures[MAX_TEXTURE_SLOTS];

in vec2      vTexCoord;
flat in vec4 vTint; // flat means don't do interpolation
flat in int  vTexureIndex;

layout(location = 0) out vec4 FragColor;

void main()
{
    // FragColor = texture(uTextures[vTexureIndex], vTexCoord) * vTint;

    vec4 tex_color;

    switch (vTexureIndex)
    {
        case 0: tex_color = texture(uTextures[0], vTexCoord); break;

        case 1: tex_color = texture(uTextures[1], vTexCoord); break;

#if MAX_TEXTURE_SLOTS > 2
        case 2: tex_color = texture(uTextures[2], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 3
        case 3: tex_color = texture(uTextures[3], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 4
        case 4: tex_color = texture(uTextures[4], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 5
        case 5: tex_color = texture(uTextures[5], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 6
        case 6: tex_color = texture(uTextures[6], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 7
        case 7: tex_color = texture(uTextures[7], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 8
        case 8: tex_color = texture(uTextures[8], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 9
        case 9: tex_color = texture(uTextures[9], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 10
        case 10: tex_color = texture(uTextures[10], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 11
        case 11: tex_color = texture(uTextures[11], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 12
        case 12: tex_color = texture(uTextures[12], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 13
        case 13: tex_color = texture(uTextures[13], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 14
        case 14: tex_color = texture(uTextures[14], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 15
        case 15: tex_color = texture(uTextures[15], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 16
        case 16: tex_color = texture(uTextures[16], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 17
        case 17: tex_color = texture(uTextures[17], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 18
        case 18: tex_color = texture(uTextures[18], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 19
        case 19: tex_color = texture(uTextures[19], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 20
        case 20: tex_color = texture(uTextures[20], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 21
        case 21: tex_color = texture(uTextures[21], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 22
        case 22: tex_color = texture(uTextures[22], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 23
        case 23: tex_color = texture(uTextures[23], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 24
        case 24: tex_color = texture(uTextures[24], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 25
        case 25: tex_color = texture(uTextures[25], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 26
        case 26: tex_color = texture(uTextures[26], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 27
        case 27: tex_color = texture(uTextures[27], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 28
        case 28: tex_color = texture(uTextures[28], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 29
        case 29: tex_color = texture(uTextures[29], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 30
        case 30: tex_color = texture(uTextures[30], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 31
        case 31: tex_color = texture(uTextures[31], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 32
        case 32: tex_color = texture(uTextures[32], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 33
        case 33: tex_color = texture(uTextures[33], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 34
        case 34: tex_color = texture(uTextures[34], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 35
        case 35: tex_color = texture(uTextures[35], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 36
        case 36: tex_color = texture(uTextures[36], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 37
        case 37: tex_color = texture(uTextures[37], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 38
        case 38: tex_color = texture(uTextures[38], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 39
        case 39: tex_color = texture(uTextures[39], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 40
        case 40: tex_color = texture(uTextures[40], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 41
        case 41: tex_color = texture(uTextures[41], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 42
        case 42: tex_color = texture(uTextures[42], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 43
        case 43: tex_color = texture(uTextures[43], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 44
        case 44: tex_color = texture(uTextures[44], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 45
        case 45: tex_color = texture(uTextures[45], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 46
        case 46: tex_color = texture(uTextures[46], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 47
        case 47: tex_color = texture(uTextures[47], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 48
        case 48: tex_color = texture(uTextures[48], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 49
        case 49: tex_color = texture(uTextures[49], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 50
        case 50: tex_color = texture(uTextures[50], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 51
        case 51: tex_color = texture(uTextures[51], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 52
        case 52: tex_color = texture(uTextures[52], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 53
        case 53: tex_color = texture(uTextures[53], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 54
        case 54: tex_color = texture(uTextures[54], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 55
        case 55: tex_color = texture(uTextures[55], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 56
        case 56: tex_color = texture(uTextures[56], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 57
        case 57: tex_color = texture(uTextures[57], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 58
        case 58: tex_color = texture(uTextures[58], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 59
        case 59: tex_color = texture(uTextures[59], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 60
        case 60: tex_color = texture(uTextures[60], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 61
        case 61: tex_color = texture(uTextures[61], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 62
        case 62: tex_color = texture(uTextures[62], vTexCoord); break;
#endif
#if MAX_TEXTURE_SLOTS > 63
        case 63: tex_color = texture(uTextures[63], vTexCoord); break;
#endif
    }

    tex_color *= vTint;

    FragColor = tex_color;

    if (FragColor.a == 0.0)
        discard;
}
