#version 300 es
precision mediump float;
precision mediump sampler2D;

/**
 * \file
 * \author Sungwoo Yang
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */

#ifndef MAX_TEXTURE_SLOTS
#define MAX_TEXTURE_SLOTS 8
#endif

uniform sampler2D u_textures[MAX_TEXTURE_SLOTS];

in vec2 v_uv;
flat in vec4 v_color;
flat in int v_tex_id;

layout(location = 0) out vec4 frag_color;

void main()
{
    int tex_index = v_tex_id;
    vec4 tex_color;

    switch(tex_index)
    {
        case 0: tex_color = texture(u_textures[0], v_uv); break;

        case 1: tex_color = texture(u_textures[1], v_uv); break;

#if MAX_TEXTURE_SLOTS > 2
        case 2: tex_color = texture(u_textures[2], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 3
        case 3: tex_color = texture(u_textures[3], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 4
        case 4: tex_color = texture(u_textures[4], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 5
        case 5: tex_color = texture(u_textures[5], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 6
        case 6: tex_color = texture(u_textures[6], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 7
        case 7: tex_color = texture(u_textures[7], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 8
        case 8: tex_color = texture(u_textures[8], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 9
        case 9: tex_color = texture(u_textures[9], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 10
        case 10: tex_color = texture(u_textures[10], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 11
        case 11: tex_color = texture(u_textures[11], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 12
        case 12: tex_color = texture(u_textures[12], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 13
        case 13: tex_color = texture(u_textures[13], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 14
        case 14: tex_color = texture(u_textures[14], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 15
        case 15: tex_color = texture(u_textures[15], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 16
        case 16: tex_color = texture(u_textures[16], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 17
        case 17: tex_color = texture(u_textures[17], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 18
        case 18: tex_color = texture(u_textures[18], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 19
        case 19: tex_color = texture(u_textures[19], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 20
        case 20: tex_color = texture(u_textures[20], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 21
        case 21: tex_color = texture(u_textures[21], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 22
        case 22: tex_color = texture(u_textures[22], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 23
        case 23: tex_color = texture(u_textures[23], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 24
        case 24: tex_color = texture(u_textures[24], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 25
        case 25: tex_color = texture(u_textures[25], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 26
        case 26: tex_color = texture(u_textures[26], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 27
        case 27: tex_color = texture(u_textures[27], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 28
        case 28: tex_color = texture(u_textures[28], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 29
        case 29: tex_color = texture(u_textures[29], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 30
        case 30: tex_color = texture(u_textures[30], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 31
        case 31: tex_color = texture(u_textures[31], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 32
        case 32: tex_color = texture(u_textures[32], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 33
        case 33: tex_color = texture(u_textures[33], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 34
        case 34: tex_color = texture(u_textures[34], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 35
        case 35: tex_color = texture(u_textures[35], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 36
        case 36: tex_color = texture(u_textures[36], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 37
        case 37: tex_color = texture(u_textures[37], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 38
        case 38: tex_color = texture(u_textures[38], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 39
        case 39: tex_color = texture(u_textures[39], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 40
        case 40: tex_color = texture(u_textures[40], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 41
        case 41: tex_color = texture(u_textures[41], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 42
        case 42: tex_color = texture(u_textures[42], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 43
        case 43: tex_color = texture(u_textures[43], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 44
        case 44: tex_color = texture(u_textures[44], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 45
        case 45: tex_color = texture(u_textures[45], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 46
        case 46: tex_color = texture(u_textures[46], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 47
        case 47: tex_color = texture(u_textures[47], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 48
        case 48: tex_color = texture(u_textures[48], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 49
        case 49: tex_color = texture(u_textures[49], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 50
        case 50: tex_color = texture(u_textures[50], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 51
        case 51: tex_color = texture(u_textures[51], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 52
        case 52: tex_color = texture(u_textures[52], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 53
        case 53: tex_color = texture(u_textures[53], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 54
        case 54: tex_color = texture(u_textures[54], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 55
        case 55: tex_color = texture(u_textures[55], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 56
        case 56: tex_color = texture(u_textures[56], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 57
        case 57: tex_color = texture(u_textures[57], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 58
        case 58: tex_color = texture(u_textures[58], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 59
        case 59: tex_color = texture(u_textures[59], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 60
        case 60: tex_color = texture(u_textures[60], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 61
        case 61: tex_color = texture(u_textures[61], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 62
        case 62: tex_color = texture(u_textures[62], v_uv); break;
#endif
#if MAX_TEXTURE_SLOTS > 63
        case 63: tex_color = texture(u_textures[63], v_uv); break;
#endif
    }
    
    frag_color = tex_color * v_color;

    if (frag_color.a == 0.0)
    {
        discard;
    }
}